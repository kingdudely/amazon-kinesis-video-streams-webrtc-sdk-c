#pragma once

#include "RtpPacket.h"
#include <cstdint>
#include <functional>
#include <span>

namespace tinyrtc {

struct RtpSender {
    std::uint16_t sequence{};
    std::uint32_t ssrc{};
    std::uint8_t payloadType{};
    std::function<bool(const RtpPacket&)> output;

    bool send(std::span<const std::uint8_t> payload,
              std::uint32_t timestamp,
              bool marker = false) noexcept;
};

}
