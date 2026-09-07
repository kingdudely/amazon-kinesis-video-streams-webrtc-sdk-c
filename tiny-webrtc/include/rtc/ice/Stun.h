#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace tinyrtc {

struct StunAddress {
    std::array<std::uint8_t, 16> address{};
    std::uint16_t port{};
    bool v6{};
};

constexpr std::uint32_t STUN_MAGIC_COOKIE = 0x2112A442u;
constexpr std::size_t STUN_HEADER_SIZE = 20;

bool isStunPacket(std::span<const std::uint8_t> data) noexcept;
bool isBindingRequest(std::span<const std::uint8_t> data) noexcept;

bool buildBindingRequest(std::array<std::uint8_t, 256>& out,
                         std::size_t& size,
                         const std::uint8_t transactionId[12],
                         const char* username,
                         const char* password) noexcept;

bool buildBindingResponse(std::array<std::uint8_t, 256>& out,
                          std::size_t& size,
                          const std::uint8_t transactionId[12],
                          std::uint32_t ip,
                          std::uint16_t port) noexcept;

bool parseXorMappedAddress(std::span<const std::uint8_t> data, StunAddress& out) noexcept;

}
