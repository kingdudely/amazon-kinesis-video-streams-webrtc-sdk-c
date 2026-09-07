#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace tinyrtc {

class Srtp {
public:
    bool init(std::span<const std::uint8_t> keyMaterial);
    bool protectRtp(std::span<std::uint8_t> packet, std::size_t& size);
    bool protectRtcp(std::span<std::uint8_t> packet, std::size_t& size);
    bool unprotectRtcp(std::span<std::uint8_t> packet, std::size_t& size);
private:
    bool ready_{};
};

}