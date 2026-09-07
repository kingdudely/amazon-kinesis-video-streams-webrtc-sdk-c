#include "rtc/platform/Socket.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace tinyrtc::platform {

bool socketsInit() noexcept
{
#if defined(_WIN32)
    WSADATA wsa{};
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return true;
#endif
}

void socketsCleanup() noexcept
{
#if defined(_WIN32)
    WSACleanup();
#endif
}

void closeSocket(SocketHandle s) noexcept
{
    if (s == INVALID_SOCKET_HANDLE)
        return;
#if defined(_WIN32)
    ::closesocket(static_cast<SOCKET>(s));
#else
    ::close(static_cast<int>(s));
#endif
}

bool setNonBlocking(SocketHandle s, bool enabled) noexcept
{
#if defined(_WIN32)
    u_long value = enabled ? 1UL : 0UL;
    return ioctlsocket(static_cast<SOCKET>(s), FIONBIO, &value) == 0;
#else
    const int fd = static_cast<int>(s);
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return false;
    flags = enabled ? flags | O_NONBLOCK : flags & ~O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) == 0;
#endif
}

bool setReuseAddr(SocketHandle s) noexcept
{
    int yes = 1;
    return setsockopt(static_cast<int>(s), SOL_SOCKET, SO_REUSEADDR,
                      reinterpret_cast<const char*>(&yes), sizeof(yes)) == 0;
}

SocketHandle tcpListen(std::uint16_t port) noexcept
{
#if defined(_WIN32)
    SOCKET fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == INVALID_SOCKET)
        return INVALID_SOCKET_HANDLE;
#else
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return INVALID_SOCKET_HANDLE;
#endif
    setReuseAddr(static_cast<SocketHandle>(fd));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (::bind(static_cast<int>(fd), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
        ::listen(static_cast<int>(fd), 1) != 0) {
        closeSocket(static_cast<SocketHandle>(fd));
        return INVALID_SOCKET_HANDLE;
    }
    setNonBlocking(static_cast<SocketHandle>(fd), true);
    return static_cast<SocketHandle>(fd);
}

SocketHandle tcpAccept(SocketHandle listener) noexcept
{
#if defined(_WIN32)
    SOCKET fd = ::accept(static_cast<SOCKET>(listener), nullptr, nullptr);
    if (fd == INVALID_SOCKET)
        return INVALID_SOCKET_HANDLE;
#else
    int fd = ::accept(static_cast<int>(listener), nullptr, nullptr);
    if (fd < 0)
        return INVALID_SOCKET_HANDLE;
#endif
    setNonBlocking(static_cast<SocketHandle>(fd), true);
    return static_cast<SocketHandle>(fd);
}

SocketHandle udpBind(std::uint16_t port) noexcept
{
#if defined(_WIN32)
    SOCKET fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == INVALID_SOCKET)
        return INVALID_SOCKET_HANDLE;
#else
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return INVALID_SOCKET_HANDLE;
#endif
    setReuseAddr(static_cast<SocketHandle>(fd));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (::bind(static_cast<int>(fd), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closeSocket(static_cast<SocketHandle>(fd));
        return INVALID_SOCKET_HANDLE;
    }
    setNonBlocking(static_cast<SocketHandle>(fd), true);
    return static_cast<SocketHandle>(fd);
}

int recvSocket(SocketHandle s, void* buffer, std::size_t size) noexcept
{
#if defined(_WIN32)
    return ::recv(static_cast<SOCKET>(s), static_cast<char*>(buffer), static_cast<int>(size), 0);
#else
    return static_cast<int>(::recv(static_cast<int>(s), buffer, size, 0));
#endif
}

int sendSocket(SocketHandle s, const void* buffer, std::size_t size) noexcept
{
#if defined(_WIN32)
    return ::send(static_cast<SOCKET>(s), static_cast<const char*>(buffer), static_cast<int>(size), 0);
#else
    return static_cast<int>(::send(static_cast<int>(s), buffer, size, 0));
#endif
}

int recvFrom(SocketHandle s, void* buffer, std::size_t size, void* address, std::size_t* addressSize) noexcept
{
    if (!address || !addressSize)
        return -1;
#if defined(_WIN32)
    int len = static_cast<int>(*addressSize);
    const int r = ::recvfrom(static_cast<SOCKET>(s), static_cast<char*>(buffer), static_cast<int>(size), 0,
                             static_cast<sockaddr*>(address), &len);
    *addressSize = static_cast<std::size_t>(len);
    return r;
#else
    socklen_t len = static_cast<socklen_t>(*addressSize);
    const int r = static_cast<int>(::recvfrom(static_cast<int>(s), buffer, size, 0,
                                              static_cast<sockaddr*>(address), &len));
    *addressSize = static_cast<std::size_t>(len);
    return r;
#endif
}

int sendTo(SocketHandle s, const void* buffer, std::size_t size, const void* address, std::size_t addressSize) noexcept
{
#if defined(_WIN32)
    return ::sendto(static_cast<SOCKET>(s), static_cast<const char*>(buffer), static_cast<int>(size), 0,
                    static_cast<const sockaddr*>(address), static_cast<int>(addressSize));
#else
    return static_cast<int>(::sendto(static_cast<int>(s), buffer, size, 0,
                                     static_cast<const sockaddr*>(address), static_cast<socklen_t>(addressSize)));
#endif
}

}
