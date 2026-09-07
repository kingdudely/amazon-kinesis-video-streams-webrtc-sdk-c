#pragma once

#include "RtpPacket.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace tinyrtc {

struct Nack {
    std::uint32_t mediaSsrc{};
    std::uint16_t pid{};
    std::uint16_t blp{};
};

class RtpRetransmissionCache {
public:
    static constexpr std::size_t CAPACITY = 1024;

    void clear() noexcept;
    void store(const RtpPacket& packet) noexcept;
    const RtpPacket* find(std::uint16_t sequence) const noexcept;

private:
    struct Slot {
        RtpPacket packet{};
        bool valid{};
    };
    std::array<Slot, CAPACITY> slots_{};
};

using RtcpNackHandler = std::function<void(const Nack&)>;
using RtcpPliHandler = std::function<void(std::uint32_t mediaSsrc)>;

bool parseRtcp(std::span<const std::uint8_t> data,
               const RtcpNackHandler& nack,
               const RtcpPliHandler& pli) noexcept;

bool buildRtcpSenderReport(std::uint32_t ssrc,
                           std::uint32_t rtpTimestamp,
                           std::uint32_t packetCount,
                           std::uint32_t octetCount,
                           std::uint64_t ntpTime,
                           std::array<std::uint8_t, 28>& out) noexcept;

}
