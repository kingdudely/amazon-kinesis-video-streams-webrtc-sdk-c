#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace tinyrtc::crypto {

std::array<std::uint8_t, 20> sha1(std::span<const std::uint8_t> data) noexcept;
std::array<std::uint8_t, 20> hmacSha1(std::span<const std::uint8_t> key,
                                      std::span<const std::uint8_t> data) noexcept;
std::size_t base64(std::span<const std::uint8_t> data, char* out, std::size_t capacity) noexcept;

}
