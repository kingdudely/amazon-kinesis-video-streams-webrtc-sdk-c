#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

namespace tinyrtc {

class H264Packetizer {
public:
    using Output = std::function<void(const std::uint8_t*, std::size_t, std::uint32_t, bool)>;
    explicit H264Packetizer(std::size_t mtu = 1200) : mtu_(mtu) {}
    void packetize(const std::uint8_t* annexB, std::size_t size, std::uint32_t timestamp, Output output) const;
private:
    std::size_t mtu_;
};

}