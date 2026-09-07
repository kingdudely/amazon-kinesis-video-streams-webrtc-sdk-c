#pragma once
#include <cstddef>
#include <cstdint>

namespace tinyrtc {

struct RtpPacket {
    std::uint8_t* data{};
    std::size_t size{};
    std::uint16_t sequence{};
    std::uint32_t timestamp{};
    std::uint32_t ssrc{};
    std::uint8_t payloadType{};
    bool marker{};
};

}