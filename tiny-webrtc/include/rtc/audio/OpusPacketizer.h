#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

namespace tinyrtc {

class OpusPacketizer {
public:
    using Output = std::function<void(const std::uint8_t*, std::size_t, std::uint32_t, bool)>;
    void packetize(const std::uint8_t* frame, std::size_t size, std::uint32_t timestamp, Output output) const;
};

}