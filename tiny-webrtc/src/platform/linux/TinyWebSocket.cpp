#include "rtc/platform/WebSocket.h"
#include "rtc/CryptoTiny.h"
#include "rtc/platform/Socket.h"

#include <array>
#include <cstdio>
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
    std::array<std::uint8_t, 4> mask{};
    bool handshaken = false;
    std::string token;
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

static bool header(std::string_view request, std::string_view name, std::string_view& value) noexcept
{
    std::size_t pos = 0;
    while (pos < request.size()) {
        std::size_t e = request.find("\r\n", pos);
        if (e == std::string_view::npos) e = request.size();
        const auto line = request.substr(pos, e - pos);
        const auto colon = line.find(':');
        if (colon != std::string_view::npos) {
            auto key = line.substr(0, colon);
            auto val = line.substr(colon + 1);
            while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.remove_suffix(1);
            while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.remove_prefix(1);
            while (!val.empty() && (val.back() == ' ' || val.back() == '\t')) val.remove_suffix(1);
            if (key == name) { value = val; return true; }
        }
        if (e == request.size()) break;
        pos = e + 2;
    }
    return false;
}

static std::string_view tokenFromTarget(std::string_view request) noexcept
{
    const auto first = request.find(' ');
    if (first == std::string_view::npos) return {};
    const auto second = request.find(' ', first + 1);
    if (second == std::string_view::npos) return {};
    auto target = request.substr(first + 1, second - first - 1);
    const auto marker = target.find("?token=");
    if (marker == std::string_view::npos) return {};
    auto value = target.substr(marker + 7);
    const auto amp = value.find('&');
    if (amp != std::string_view::npos) value = value.substr(0, amp);
    return value;
}

static bool sendAll(SocketHandle s, const void* data, std::size_t size) noexcept
{
    const auto* p = static_cast<const std::uint8_t*>(data);
    while (size) {
        const int n = sendSocket(s, p, size);
        if (n <= 0) return false;
        p += n;
        size -= static_cast<std::size_t>(n);
    }
    return true;
}

static bool sendFrame(Impl& x, std::uint8_t opcode, std::span<const std::uint8_t> payload) noexcept
{
    if (x.client == INVALID_SOCKET_HANDLE || payload.size() > 125)
        return false;
    std::array<std::uint8_t, 127> frame{};
    frame[0] = static_cast<std::uint8_t>(0x80u | (opcode & 0x0fu));
    frame[1] = static_cast<std::uint8_t>(payload.size());
    if (!payload.empty()) std::memcpy(frame.data() + 2, payload.data(), payload.size());
    return sendAll(x.client, frame.data(), payload.size() + 2);
}

static void disconnect(Impl& x) noexcept
{
    const bool hadClient = x.client != INVALID_SOCKET_HANDLE;
    closeSocket(x.client);
    x.client = INVALID_SOCKET_HANDLE;
    x.httpSize = 0;
    x.frameSize = 0;
    x.handshaken = false;
    if (hadClient && x.onClose)
        x.onClose();
}

static bool handshake(Impl& x) noexcept
{
    std::size_t end = 0;
    if (!hasHeaderEnd(x.http.data(), x.httpSize, end))
        return true;

    const std::string_view request(x.http.data(), end);
    if (request.substr(0, 4) != "GET ") {
        disconnect(x); return false;
    }

    std::string_view key;
    if (!header(request, "Sec-WebSocket-Key", key)) {
        disconnect(x); return false;
    }

    const auto suppliedToken = tokenFromTarget(request);
    if (!x.token.empty() && suppliedToken != x.token) {
        static constexpr char forbidden[] = "HTTP/1.1 403 Forbidden\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
        sendAll(x.client, forbidden, sizeof(forbidden) - 1);
        disconnect(x); return false;
    }

    std::array<std::uint8_t, 256> input{};
    if (key.size() + WS_GUID.size() >= input.size()) {
        disconnect(x); return false;
    }
    std::memcpy(input.data(), key.data(), key.size());
    std::memcpy(input.data() + key.size(), WS_GUID.data(), WS_GUID.size());
    const auto digest = crypto::sha1({input.data(), key.size() + WS_GUID.size()});

    std::array<char, 64> accept{};
    crypto::base64(digest, accept.data(), accept.size());
    char response[256];
    const int n = std::snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n",
        accept.data());
    if (n <= 0 || static_cast<std::size_t>(n) >= sizeof(response) || !sendAll(x.client, response, static_cast<std::size_t>(n))) {
        disconnect(x); return false;
    }

    x.handshaken = true;
    x.httpSize = 0;
    if (x.onOpen) x.onOpen();
    return true;
}

static bool frames(Impl& x) noexcept
{
    while (x.frameSize >= 2) {
        const std::uint8_t first = x.frame[0];
        const std::uint8_t second = x.frame[1];
        const std::uint8_t opcode = first & 0x0f;
        const bool fin = (first & 0x80) != 0;
        const bool masked = (second & 0x80) != 0;
        const std::size_t len = second & 0x7f;
        if (!fin || !masked || len >= 126 || (opcode != 2 && opcode != 8 && opcode != 9 && opcode != 10)) {
            disconnect(x); return false;
        }
        const std::size_t need = 6 + len;
        if (x.frameSize < need)
            return true;

        std::memcpy(x.mask.data(), x.frame.data() + 2, 4);
        auto* payload = x.frame.data() + 6;
        for (std::size_t i = 0; i < len; ++i)
            payload[i] ^= x.mask[i & 3];

        if (opcode == 2) {
            if (len != 6 && len != 18) {
                disconnect(x); return false;
            }
            if (x.onBinary) x.onBinary({payload, len});
        } else if (opcode == 9) {
            if (!sendFrame(x, 10, {payload, len})) { disconnect(x); return false; }
        } else if (opcode == 8) {
            sendFrame(x, 8, {payload, len});
            disconnect(x); return false;
        }

        if (need < x.frameSize)
            std::memmove(x.frame.data(), x.frame.data() + need, x.frameSize - need);
        x.frameSize -= need;
    }
    return true;
}

}

WebSocketServer::~WebSocketServer() { close(); }

bool WebSocketServer::listen(std::uint16_t port, const char* token)
{
    // Preserve handlers installed before listen().
    auto* old = static_cast<Impl*>(impl_);
    Impl* x = new Impl;
    if (old) {
        x->onOpen = std::move(old->onOpen);
        x->onBinary = std::move(old->onBinary);
        x->onClose = std::move(old->onClose);
        closeSocket(old->listener);
        closeSocket(old->client);
        delete old;
    }
    if (!socketsInit()) { delete x; impl_ = nullptr; return false; }
    x->listener = tcpListen(port);
    if (x->listener == INVALID_SOCKET_HANDLE) {
        delete x; socketsCleanup(); impl_ = nullptr; return false;
    }
    if (token) x->token = token;
    impl_ = x;
    return true;
}

void WebSocketServer::poll(int)
{
    auto* x = static_cast<Impl*>(impl_);
    if (!x) return;

    const auto fd = tcpAccept(x->listener);
    if (fd != INVALID_SOCKET_HANDLE) {
        if (x->client != INVALID_SOCKET_HANDLE)
            disconnect(*x);
        x->client = fd;
    }

    if (x->client == INVALID_SOCKET_HANDLE)
        return;

    if (!x->handshaken) {
        if (x->httpSize == x->http.size()) { disconnect(*x); return; }
        const int n = recvSocket(x->client, x->http.data() + x->httpSize, x->http.size() - x->httpSize);
        if (n == 0) { disconnect(*x); return; }
        if (n > 0) { x->httpSize += static_cast<std::size_t>(n); handshake(*x); }
    } else {
        if (x->frameSize == x->frame.size()) { disconnect(*x); return; }
        const int n = recvSocket(x->client, x->frame.data() + x->frameSize, x->frame.size() - x->frameSize);
        if (n == 0) { disconnect(*x); return; }
        if (n > 0) { x->frameSize += static_cast<std::size_t>(n); frames(*x); }
    }
}

void WebSocketServer::close()
{
    auto* x = static_cast<Impl*>(impl_);
    if (!x) return;
    disconnect(*x);
    closeSocket(x->listener);
    delete x;
    impl_ = nullptr;
    socketsCleanup();
}

bool WebSocketServer::connected() const noexcept
{
    const auto* x = static_cast<const Impl*>(impl_);
    return x && x->handshaken;
}

bool WebSocketServer::sendBinary(std::span<const std::uint8_t> data)
{
    auto* x = static_cast<Impl*>(impl_);
    return x && x->handshaken && sendFrame(*x, 2, data);
}

void WebSocketServer::setHandlers(OpenHandler open, BinaryHandler binary, CloseHandler close)
{
    auto* x = static_cast<Impl*>(impl_);
    if (!x) { x = new Impl; impl_ = x; }
    x->onOpen = std::move(open);
    x->onBinary = std::move(binary);
    x->onClose = std::move(close);
}

}
