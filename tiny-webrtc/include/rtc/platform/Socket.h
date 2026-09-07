#pragma once

#include <cstddef>
#include <cstdint>

namespace tinyrtc::platform {

using SocketHandle = std::intptr_t;
constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;

bool socketsInit() noexcept;
void socketsCleanup() noexcept;
void closeSocket(SocketHandle s) noexcept;
bool setNonBlocking(SocketHandle s, bool enabled) noexcept;
bool setReuseAddr(SocketHandle s) noexcept;
SocketHandle tcpListen(std::uint16_t port) noexcept;
SocketHandle tcpAccept(SocketHandle listener) noexcept;
SocketHandle udpBind(std::uint16_t port) noexcept;
int recvSocket(SocketHandle s, void* buffer, std::size_t size) noexcept;
int sendSocket(SocketHandle s, const void* buffer, std::size_t size) noexcept;
int recvFrom(SocketHandle s, void* buffer, std::size_t size, void* address, std::size_t* addressSize) noexcept;
int sendTo(SocketHandle s, const void* buffer, std::size_t size, const void* address, std::size_t addressSize) noexcept;

}
