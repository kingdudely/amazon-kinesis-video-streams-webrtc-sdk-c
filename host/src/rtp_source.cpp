#include "rtp_source.hpp"

#include <array>
#include <iostream>

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

RtpSource::RtpSource(uint16_t port, PacketCallback callback)
    : port_(port), callback_(std::move(callback)) {}

RtpSource::~RtpSource() { stop(); }

void RtpSource::start() {
    if (running_.exchange(true)) return;
    thread_ = std::thread(&RtpSource::run, this);
}

void RtpSource::stop() {
    if (!running_.exchange(false)) return;
#ifdef _WIN32
    if (socket_ != static_cast<uintptr_t>(~0ull)) {
        closesocket(static_cast<SOCKET>(socket_));
        socket_ = static_cast<uintptr_t>(~0ull);
    }
#else
    if (socket_ >= 0) {
        close(socket_);
        socket_ = -1;
    }
#endif
    if (thread_.joinable()) thread_.join();
}

void RtpSource::run() {
#ifdef _WIN32
    SOCKET sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) return;
    socket_ = static_cast<uintptr_t>(sock);
#else
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    socket_ = sock;
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port_);

    if (::bind(sock, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        running_ = false;
        return;
    }

    std::array<std::uint8_t, 65536> buffer{};
    while (running_) {
#ifdef _WIN32
        const int count = ::recv(sock, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
#else
        const int count = static_cast<int>(::recv(sock, buffer.data(), buffer.size(), 0));
#endif
        if (count <= 0) break;
        if (count < 12) continue;
        callback_(std::vector<std::uint8_t>(buffer.begin(), buffer.begin() + count));
    }
}
