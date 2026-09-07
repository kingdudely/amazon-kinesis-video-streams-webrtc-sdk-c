#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace tinyrtc {

constexpr std::size_t RTP_HEADER_SIZE = 12;
constexpr std::size_t RTP_MAX_PACKET = 1500;

struct RtpPacket {
    std::array<std::uint8_t, RTP_MAX_PACKET> bytes{};
    std::size_t size{};
    std::uint16_t sequence{};
    std::uint32_t timestamp{};
    std::uint32_t ssrc{};
    std::uint8_t payloadType{};
    bool marker{};

    std::span<std::uint8_t> span() noexcept { return {bytes.data(), size}; }
    std::span<const std::uint8_t> span() const noexcept { return {bytes.data(), size}; }
};

bool buildRtpPacket(RtpPacket& packet,
                    std::uint8_t payloadType,
                    bool marker,
                    std::uint16_t sequence,
                    std::uint32_t timestamp,
                    std::uint32_t ssrc,
                    std::span<const std::uint8_t> payload) noexcept;

bool parseRtpHeader(std::span<const std::uint8_t> packet,
                    std::uint16_t& sequence,
                    std::uint32_t& timestamp,
                    std::uint32_t& ssrc,
                    std::uint8_t& payloadType,
                    bool& marker,
                    std::size_t& headerSize) noexcept;

}
