#pragma once
#include <array>
#include <cstdint>

namespace tinyrtc {

class Signaling {
public:
    static constexpr std::size_t V4_SIZE = 6;
    static constexpr std::size_t V6_SIZE = 18;

    static bool parseCandidate(const std::uint8_t* data, std::size_t size,
                               std::array<std::uint8_t, 16>& address,
                               std::uint16_t& port,
                               bool& v6);
};

}