#include "rtc/platform/WebSocket.h"
#include "rtc/CryptoTiny.h"
#include "rtc/platform/Socket.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>

namespace tinyrtc::platform {
namespace {

constexpr std::size_t HTTP_BUFFER = 8192;
constexpr std::size_t FRAME_BUFFER = 2048;
constexpr std::string_view WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

struct Impl {
    SocketHandle listener = INVALID_SOCKET_HANDLE;
    SocketHandle client = INVALID_SOCKET_HANDLE;
    std::array<char, HTTP_BUFFER> http{};
    std::size_t httpSize = 0;
    std::array<std::uint8_t, FRAME_BUFFER> frame{};
    std::size_t frameSize = 0;
    std::size_t frameNeed = 0;
    std::array<std::uint8_t, 4> mask{};
    std::size_t maskOffset = 0;
    bool handshaken = false;
    std::string_view token{};
    WebSocketServer::OpenHandler onOpen;
    WebSocketServer::BinaryHandler onBinary;
    WebSocketServer::CloseHandler onClose;
};

static bool hasHeaderEnd(const char* data, std::size_t size, std::size_t& end) noexcept
{
    if (size < 4)
        return false;
    for (std::size_t i = 3; i < size; ++i) {
        if (data[i - 3] == '\r' && data[i - 2] == '\n' && data[i - 1] == '\r' && data[i] == '\n') {
            end = i + 1;
            return true;
        }
    }
    return false;
}

static bool findHeader(std::string_view request, std::string_view name, std::string_view& value) noexcept
{
    std::size_t pos = 0;
    while (pos < request.size()) {
        std::size_t e = request.find("\r\n", pos);
        if (e == std::string_view::npos)
            e = request.size();
        const auto line = request.substr(pos, e - pos);
        const auto colon = line.find(':');
        if (colon != std::string_view::npos) {
            auto key = line.substr(0, colon);
            while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.remove_suffix(1);
            if (key == name) {
                auto v = line.substr(colon + 1);
                while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.remove_prefix(1);
                while (!v.empty() && (v.back() == ' ' || v.back() == '\t')) v.remove_suffix(1);
                value = v;
                return true;
            }
        }
        if (e == request.size()) break;
        pos = e + 2;
    }
    return false;
}

static std::string_view queryToken(std::string_view request, std::array<char, 256>& scratch) noexcept
{
    const auto first = request.find(' ');
    if (first == std::string_view::npos)
        return {};
    const auto second = request.find(' ', first + 1);
    if (second == std::string_view::npos)
        return {};
    auto target = request.substr(first + 1, second - first - 1);
    const auto q = target.find("?token=");
    if (q == std::string_view::npos)
        return {};
    auto value = target.substr(q + 7);
    const auto amp = value.find('&');
    if (amp != std::string_view::npos)
        value = value.substr(0, amp);
    if (value.size() >= scratch.size())
        return {};
    std::memcpy(scratch.data(), value.data(), value.size());
    scratch[value.size()] = '\0';
    return std::string_view(scratch.data(), value.size());
}

static bool sendAll(SocketHandle s, const std::uint8_t* data, std::size_t size) noexcept
{
    while (size) {
        const int n = sendSocket(s, data, size);
        if (n <= 0)
            return false;
        data += n;
        size -= static_cast<std::size_t>(n);
    }
    return true;
}

static bool sendFrame(Impl& x, std::uint8_t opcode, std::span<const std::uint8_t> payload) noexcept
{
    if (payload.size() > 125 || x.client == INVALID_SOCKET_HANDLE)
        return false;
    std::array<std::uint8_t, 2 + 125> out{};
    out[0] = static_cast<std::uint8_t>(0x80u | (opcode & 0x0Fu));
    out[1] = static_cast<std::uint8_t>(payload.size());
    if (!payload.empty())
        std::memcpy(out.data() + 2, payload.data(), payload.size());
    return sendAll(x.client, out.data(), 2 + payload.size());
}

static void disconnect(Impl& x) noexcept
{
    const bool wasConnected = x.client != INVALID_SOCKET_HANDLE;
    closeSocket(x.client);
    x.client = INVALID_SOCKET_HANDLE;
    x.httpSize = x.frameSize = x.frameNeed = x.maskOffset = 0;
    x.handshaken = false;
    if (wasConnected && x.onClose)
        x.onClose();
}

static void handleHandshake(Impl& x) noexcept
{
    std::size_t end = 0;
    if (!hasHeaderEnd(x.http.data(), x.httpSize, end))
        return;

    const std::string_view request(x.http.data(), end);
    if (request.substr(0, 4) != "GET ") {
        disconnect(x);
        return;
    }

    std::string_view key;
    if (!findHeader(request, "Sec-WebSocket-Key", key)) {
        disconnect(x);
        return;
    }

    std::array<char, 256> tokenScratch{};
    const auto token = queryToken(request, tokenScratch);
    if (!x.token.empty() && token != x.token) {
        static constexpr char forbidden[] = "HTTP/1.1 403 Forbidden\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
        sendAll(x.client, reinterpret_cast<const std::uint8_t*>(forbidden), sizeof(forbidden) - 1);
        disconnect(x);
        return;
    }

    std::array<std::uint8_t, 256> concatenated{};
    if (key.size() + WS_GUID.size() >= concatenated.size()) {
        disconnect(x);
        return;
    }
    std::memcpy(concatenated.data(), key.data(), key.size());
    std::memcpy(concatenated.data() + key.size(), WS_GUID.data(), WS_GUID.size());
    const auto digest = crypto::sha1({concatenated.data(), key.size() + WS_GUID.size()});
    std::array<char, 64> encoded{};
    crypto::base64(digest, encoded.data(), encoded.size());

    char response[256];
    const int n = std::snprintf(response, sizeof(response),
                                 "HTTP/1.1 101 Switching Protocols\r\n"
                                 "Upgrade: websocket\r\n"
                                 "Connection: Upgrade\r\n"
                                 "Sec-WebSocket-Accept: %s\r\n\r\n", encoded.data());
    if (n <= 0 || static_cast<std::size_t>(n) >= sizeof(response) ||
        !sendAll(x.client, reinterpret_cast<const std::uint8_t*>(response), static_cast<std::size_t>(n))) {
        disconnect(x);
        return;
    }

    x.handshaken = true;
    x.httpSize = 0;
    if (x.onOpen)
        x.onOpen();
}

static bool processFrame(Impl& x) noexcept
{
    if (x.frameSize < 2)
        return true;

    const auto b0 = x.frame[0];
    const auto b1 = x.frame[1];
    const bool fin = (b0 & 0x80) != 0;
    const std::uint8_t opcode = b0 & 0x0f;
    const bool masked = (b1 & 0x80) != 0;
    std::size_t length = b1 & 0x7f;
    std::size_t header = 2;

    if (!fin || !masked || (opcode != 0x2 && opcode != 0x8 && opcode != 0x9 && opcode != 0xA)) {
        disconnect(x);
        return false;
    }
    if (length == 126 || length == 127) {
        disconnect(x);
        return false;
    }
    if (x.frameSize < header + 4 + length) {
        x.frameNeed = header + 4 + length;
        return true;
    }

    std::memcpy(x.mask.data(), x.frame.data() + 2, 4);
    auto* payload = x.frame.data() + 6;
    for (std::size_t i = 0; i < length; ++i)
        payload[i] ^= x.mask[i & 3];

    if (opcode == 0x2) {
        if ((length != 6 && length != 18) || !x.onBinary) {
            disconnect(x);
            return false;
        }
        x.onBinary({payload, length});
    } else if (opcode == 0x9) {
        sendFrame(x, 0xA, {payload, length});
    } else if (opcode == 0x8) {
        sendFrame(x, 0x8, {});
        disconnect(x);
        return false;
    }

    const std::size_t consumed = 6 + length;
    if (consumed < x.frameSize)
        std::memmove(x.frame.data(), x.frame.data() + consumed, x.frameSize - consumed);
    x.frameSize -= consumed;
    x.frameNeed = 0;
    return x.frameSize == 0 || processFrame(x);
}

static void receive(Impl& x) noexcept
{
    if (x.client == INVALID_SOCKET_HANDLE)
        return;

    if (!x.handshaken) {
        if (x.httpSize == x.http.size()) {
            disconnect(x);
            return;
        }
        const int n = recvSocket(x.client, x.http.data() + x.httpSize, x.http.size() - x.httpSize);
        if (n == 0) {
            disconnect(x); return;
        }
        if (n < 0)
            return;
        x.httpSize += static_cast<std::size_t>(n);
        handleHandshake(x);
        return;
    }

    if (x.frameSize == x.frame.size()) {
        disconnect(x); return;
    }
    const int n = recvSocket(x.client, x.frame.data() + x.frameSize, x.frame.size() - x.frameSize);
    if (n == 0) {
        disconnect(x); return;
    }
    if (n < 0)
        return;
    x.frameSize += static_cast<std::size_t>(n);
    processFrame(x);
}

}

WebSocketServer::~WebSocketServer()
{
    close();
}

bool WebSocketServer::listen(std::uint16_t port, const char* token)
{
    close();
    auto* x = new Impl;
    if (!socketsInit()) { delete x; return false; }
    x->listener = tcpListen(port);
    if (x->listener == INVALID_SOCKET_HANDLE) {
        socketsCleanup(); delete x; return false;
    }
    if (token)
        x->token = token;
    impl_ = x;
    return true;
}

void WebSocketServer::poll(int) 
{
    auto* x = static_cast<Impl*>(impl_);
    if (!x)
        return;
    if (x->client == INVALID_SOCKET_HANDLE) {
        const auto fd = tcpAccept(x->listener);
        if (fd != INVALID_SOCKET_HANDLE)
            x->client = fd;
    } else {
        // A second connection is intentionally accepted and replaces the current client.
        const auto fd = tcpAccept(x->listener);
        if (fd != INVALID_SOCKET_HANDLE) {
            disconnect(*x);
            x->client = fd;
        }
    }
    receive(*x);
}

void WebSocketServer::close()
{
    auto* x = static_cast<Impl*>(impl_);
    if (!x)
        return;
    disconnect(*x);
    closeSocket(x->listener);
    delete x;
    impl_ = nullptr;
    socketsCleanup();
}

bool WebSocketServer::connected() const noexcept
{
    const auto* x = static_cast<const Impl*>(impl_);
    return x && x->client != INVALID_SOCKET_HANDLE && x->handshaken;
}

bool WebSocketServer::sendBinary(std::span<const std::uint8_t> data)
{
    auto* x = static_cast<Impl*>(impl_);
    return x && x->handshaken && sendFrame(*x, 0x2, data);
}

void WebSocketServer::setHandlers(OpenHandler a, BinaryHandler b, CloseHandler c)
{
    auto* x = static_cast<Impl*>(impl_);
    if (!x) {
        x = new Impl;
        impl_ = x;
    }
    x->onOpen = std::move(a);
    x->onBinary = std::move(b);
    x->onClose = std::move(c);
}

}
