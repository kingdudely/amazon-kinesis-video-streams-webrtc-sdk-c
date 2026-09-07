#include "rtc/Rtcp.h"
#include <cstring>

namespace tinyrtc {

static inline std::uint16_t get16(const std::uint8_t* p) noexcept
{
    return static_cast<std::uint16_t>((std::uint16_t(p[0]) << 8) | p[1]);
}

static inline std::uint32_t get32(const std::uint8_t* p) noexcept
{
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
           (std::uint32_t(p[2]) << 8) | p[3];
}

static inline void put32(std::uint8_t* p, std::uint32_t v) noexcept
{
    p[0] = static_cast<std::uint8_t>(v >> 24);
    p[1] = static_cast<std::uint8_t>(v >> 16);
    p[2] = static_cast<std::uint8_t>(v >> 8);
    p[3] = static_cast<std::uint8_t>(v);
}

static inline void put16(std::uint8_t* p, std::uint16_t v) noexcept
{
    p[0] = static_cast<std::uint8_t>(v >> 8);
    p[1] = static_cast<std::uint8_t>(v);
}

void RtpRetransmissionCache::clear() noexcept
{
    for (auto& slot : slots_)
        slot.valid = false;
}

void RtpRetransmissionCache::store(const RtpPacket& packet) noexcept
{
    auto& slot = slots_[packet.sequence % CAPACITY];
    slot.packet = packet;
    slot.valid = true;
}

const RtpPacket* RtpRetransmissionCache::find(std::uint16_t sequence) const noexcept
{
    const auto& slot = slots_[sequence % CAPACITY];
    return slot.valid && slot.packet.sequence == sequence ? &slot.packet : nullptr;
}

bool parseRtcp(std::span<const std::uint8_t> data,
               const RtcpNackHandler& nack,
               const RtcpPliHandler& pli) noexcept
{
    std::size_t offset = 0;

    while (offset + 4 <= data.size()) {
        const auto* p = data.data() + offset;
        if ((p[0] >> 6) != 2)
            return false;

        const std::size_t bytes = (std::size_t(get16(p + 2)) + 1) * 4;
        if (bytes < 4 || offset + bytes > data.size())
            return false;

        const std::uint8_t fmt = p[0] & 0x1f;
        const std::uint8_t type = p[1];

        if (type == 205 && fmt == 1 && bytes >= 16) { // RTPFB / NACK
            const std::uint32_t mediaSsrc = get32(p + 8);
            for (std::size_t at = 12; at + 4 <= bytes; at += 4) {
                Nack n{mediaSsrc, get16(p + at), get16(p + at + 2)};
                if (nack)
                    nack(n);
            }
        } else if (type == 206 && fmt == 1 && bytes >= 12) { // PSFB / PLI
            const std::uint32_t mediaSsrc = get32(p + 8);
            if (pli)
                pli(mediaSsrc);
        }

        offset += bytes;
    }

    return offset == data.size();
}

bool buildRtcpSenderReport(std::uint32_t ssrc,
                           std::uint32_t rtpTimestamp,
                           std::uint32_t packetCount,
                           std::uint32_t octetCount,
                           std::uint64_t ntpTime,
                           std::array<std::uint8_t, 28>& out) noexcept
{
    out.fill(0);
    out[0] = 0x80;
    out[1] = 200; // SR
    put16(out.data() + 2, 6); // 28 bytes / 4 - 1
    put32(out.data() + 4, ssrc);
    put32(out.data() + 8, static_cast<std::uint32_t>(ntpTime >> 32));
    put32(out.data() + 12, static_cast<std::uint32_t>(ntpTime));
    put32(out.data() + 16, rtpTimestamp);
    put32(out.data() + 20, packetCount);
    put32(out.data() + 24, octetCount);
    return true;
}

}
