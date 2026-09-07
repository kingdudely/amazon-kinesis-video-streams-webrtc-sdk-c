#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace tinyrtc {

class CryptoProvider {
public:
    virtual ~CryptoProvider() = default;

    virtual bool random(std::span<std::uint8_t> out) noexcept = 0;
    virtual bool sha1(std::span<const std::uint8_t> in,
                      std::span<std::uint8_t, 20> out) noexcept = 0;
    virtual bool sha256(std::span<const std::uint8_t> in,
                        std::span<std::uint8_t, 32> out) noexcept = 0;
    virtual bool hmacSha1(std::span<const std::uint8_t> key,
                          std::span<const std::uint8_t> in,
                          std::span<std::uint8_t, 20> out) noexcept = 0;
    virtual bool hmacSha256(std::span<const std::uint8_t> key,
                            std::span<const std::uint8_t> in,
                            std::span<std::uint8_t, 32> out) noexcept = 0;
    virtual bool aesGcmEncrypt(std::span<std::uint8_t> data,
                               std::span<const std::uint8_t> aad,
                               std::span<const std::uint8_t> key,
                               std::span<const std::uint8_t, 12> iv,
                               std::span<std::uint8_t, 16> tag) noexcept = 0;
    virtual bool aesGcmDecrypt(std::span<std::uint8_t> data,
                               std::span<const std::uint8_t> aad,
                               std::span<const std::uint8_t> key,
                               std::span<const std::uint8_t, 12> iv,
                               std::span<const std::uint8_t, 16> tag) noexcept = 0;
};

} // namespace tinyrtc
