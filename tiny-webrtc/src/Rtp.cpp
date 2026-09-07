#include "rtc/RtpPacket.h"
#include "rtc/Rtp.h"
#include <cstdint>
#include <cstring>

namespace tinyrtc {

static inline void put16(std::uint8_t* p, std::uint16_t v) noexcept
{
    p[0] = static_cast<std::uint8_t>(v >> 8);
    p[1] = static_cast<std::uint8_t>(v);
}

static inline void put32(std::uint8_t* p, std::uint32_t v) noexcept
{
    p[0] = static_cast<std::uint8_t>(v >> 24);
    p[1] = static_cast<std::uint8_t>(v >> 16);
    p[2] = static_cast<std::uint8_t>(v >> 8);
    p[3] = static_cast<std::uint8_t>(v);
}

static inline std::uint16_t get16(const std::uint8_t* p) noexcept
{
    return static_cast<std::uint16_t>((std::uint16_t(p[0]) << 8) | p[1]);
}

static inline std::uint32_t get32(const std::uint8_t* p) noexcept
{
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
           (std::uint32_t(p[2]) << 8) | p[3];
}

bool buildRtpPacket(RtpPacket& packet,
                    std::uint8_t payloadType,
                    bool marker,
                    std::uint16_t sequence,
                    std::uint32_t timestamp,
                    std::uint32_t ssrc,
                    std::span<const std::uint8_t> payload) noexcept
{
    if (payload.size() > RTP_MAX_PACKET - RTP_HEADER_SIZE)
        return false;

    packet.size = RTP_HEADER_SIZE + payload.size();
    packet.sequence = sequence;
    packet.timestamp = timestamp;
    packet.ssrc = ssrc;
    packet.payloadType = static_cast<std::uint8_t>(payloadType & 0x7f);
    packet.marker = marker;

    auto* p = packet.bytes.data();
    p[0] = 0x80; // V=2, no padding, no extension, CC=0
    p[1] = static_cast<std::uint8_t>(packet.payloadType | (marker ? 0x80 : 0));
    put16(p + 2, sequence);
    put32(p + 4, timestamp);
    put32(p + 8, ssrc);
    std::memcpy(p + RTP_HEADER_SIZE, payload.data(), payload.size());
    return true;
}

bool parseRtpHeader(std::span<const std::uint8_t> packet,
                    std::uint16_t& sequence,
                    std::uint32_t& timestamp,
                    std::uint32_t& ssrc,
                    std::uint8_t& payloadType,
                    bool& marker,
                    std::size_t& headerSize) noexcept
{
    if (packet.size() < RTP_HEADER_SIZE || (packet[0] >> 6) != 2)
        return false;

    const auto cc = packet[0] & 0x0f;
    const bool extension = (packet[0] & 0x10) != 0;
    headerSize = RTP_HEADER_SIZE + 4u * cc;
    if (headerSize > packet.size())
        return false;

    if (extension) {
        if (headerSize + 4 > packet.size())
            return false;
        const auto words = get16(packet.data() + headerSize + 2);
        headerSize += 4u + 4u * words;
        if (headerSize > packet.size())
            return false;
    }

    marker = (packet[1] & 0x80) != 0;
    payloadType = packet[1] & 0x7f;
    sequence = get16(packet.data() + 2);
    timestamp = get32(packet.data() + 4);
    ssrc = get32(packet.data() + 8);
    return true;
}

bool RtpSender::send(std::span<const std::uint8_t> payload,
                     std::uint32_t timestamp,
                     bool marker) noexcept
{
    if (!output)
        return false;

    RtpPacket packet;
    if (!buildRtpPacket(packet, payloadType, marker, sequence++, timestamp, ssrc, payload))
        return false;
    return output(packet);
}

}
