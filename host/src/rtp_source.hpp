#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

class RtpSource {
public:
    using PacketCallback = std::function<void(std::vector<std::uint8_t>&&)>;

    RtpSource(uint16_t port, PacketCallback callback);
    ~RtpSource();

    void start();
    void stop();

private:
    void run();

    uint16_t port_;
    PacketCallback callback_;
    std::atomic_bool running_{false};
    std::thread thread_;
#ifdef _WIN32
    uintptr_t socket_ = static_cast<uintptr_t>(~0ull);
#else
    int socket_ = -1;
#endif
};
