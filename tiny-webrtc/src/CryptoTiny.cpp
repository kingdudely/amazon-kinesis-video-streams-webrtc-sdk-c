#include "rtc/CryptoTiny.h"
#include <algorithm>
#include <array>
#include <cstring>

namespace tinyrtc::crypto {
namespace {

static inline std::uint32_t rol(std::uint32_t v, unsigned n) noexcept
{
    return (v << n) | (v >> (32 - n));
}

static std::array<std::uint8_t, 20> sha1Impl(const std::uint8_t* data, std::size_t size) noexcept
{
    std::uint32_t h0 = 0x67452301u, h1 = 0xEFCDAB89u, h2 = 0x98BADCFEu, h3 = 0x10325476u, h4 = 0xC3D2E1F0u;
    const std::uint64_t bits = static_cast<std::uint64_t>(size) * 8u;
    const std::size_t padded = ((size + 9u + 63u) / 64u) * 64u;
    std::array<std::uint8_t, 128> block{};

    for (std::size_t off = 0; off < padded; off += 64) {
        const std::size_t n = std::min<std::size_t>(64, size > off ? size - off : 0);
        if (off < size)
            std::memcpy(block.data(), data + off, n);
        if (off + n == size) {
            block[n] = 0x80;
            if (n >= 56) {
                block[120] = static_cast<std::uint8_t>(bits >> 56);
                block[121] = static_cast<std::uint8_t>(bits >> 48);
                block[122] = static_cast<std::uint8_t>(bits >> 40);
                block[123] = static_cast<std::uint8_t>(bits >> 32);
                block[124] = static_cast<std::uint8_t>(bits >> 24);
                block[125] = static_cast<std::uint8_t>(bits >> 16);
                block[126] = static_cast<std::uint8_t>(bits >> 8);
                block[127] = static_cast<std::uint8_t>(bits);
            } else {
                block[56] = static_cast<std::uint8_t>(bits >> 56);
                block[57] = static_cast<std::uint8_t>(bits >> 48);
                block[58] = static_cast<std::uint8_t>(bits >> 40);
                block[59] = static_cast<std::uint8_t>(bits >> 32);
                block[60] = static_cast<std::uint8_t>(bits >> 24);
                block[61] = static_cast<std::uint8_t>(bits >> 16);
                block[62] = static_cast<std::uint8_t>(bits >> 8);
                block[63] = static_cast<std::uint8_t>(bits);
            }
        }

        const std::size_t limit = (off + n == size && n >= 56) ? 128 : 64;
        for (std::size_t base = 0; base < limit; base += 64) {
            std::uint32_t w[80]{};
            for (unsigned i = 0; i < 16; ++i) {
                const auto* p = block.data() + base + i * 4;
                w[i] = (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
                       (std::uint32_t(p[2]) << 8) | p[3];
            }
            for (unsigned i = 16; i < 80; ++i)
                w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

            std::uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
            for (unsigned i = 0; i < 80; ++i) {
                std::uint32_t f, k;
                if (i < 20) {
                    f = (b & c) | ((~b) & d); k = 0x5A827999u;
                } else if (i < 40) {
                    f = b ^ c ^ d; k = 0x6ED9EBA1u;
                } else if (i < 60) {
                    f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCu;
                } else {
                    f = b ^ c ^ d; k = 0xCA62C1D6u;
                }
                const std::uint32_t t = rol(a, 5) + f + e + k + w[i];
                e = d; d = c; c = rol(b, 30); b = a; a = t;
            }
            h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
        }

        block.fill(0);
    }

    std::array<std::uint8_t, 20> out{};
    const std::uint32_t h[5] = {h0, h1, h2, h3, h4};
    for (unsigned i = 0; i < 5; ++i) {
        out[i * 4] = static_cast<std::uint8_t>(h[i] >> 24);
        out[i * 4 + 1] = static_cast<std::uint8_t>(h[i] >> 16);
        out[i * 4 + 2] = static_cast<std::uint8_t>(h[i] >> 8);
        out[i * 4 + 3] = static_cast<std::uint8_t>(h[i]);
    }
    return out;
}

}

std::array<std::uint8_t, 20> sha1(std::span<const std::uint8_t> data) noexcept
{
    return sha1Impl(data.data(), data.size());
}

std::array<std::uint8_t, 20> hmacSha1(std::span<const std::uint8_t> key,
                                      std::span<const std::uint8_t> data) noexcept
{
    std::array<std::uint8_t, 64> k{};
    if (key.size() > 64) {
        const auto digest = sha1(key);
        std::memcpy(k.data(), digest.data(), digest.size());
    } else {
        std::memcpy(k.data(), key.data(), key.size());
    }

    std::array<std::uint8_t, 64> ipad{}, opad{};
    for (std::size_t i = 0; i < 64; ++i) {
        ipad[i] = static_cast<std::uint8_t>(k[i] ^ 0x36);
        opad[i] = static_cast<std::uint8_t>(k[i] ^ 0x5c);
    }

    std::array<std::uint8_t, 128 + 1500> inner{};
    if (data.size() > inner.size() - 64) {
        // STUN MESSAGE-INTEGRITY values are tiny. Refuse oversized input rather than allocate.
        return {};
    }
    std::memcpy(inner.data(), ipad.data(), 64);
    std::memcpy(inner.data() + 64, data.data(), data.size());
    const auto innerHash = sha1Impl(inner.data(), 64 + data.size());

    std::array<std::uint8_t, 84> outer{};
    std::memcpy(outer.data(), opad.data(), 64);
    std::memcpy(outer.data() + 64, innerHash.data(), 20);
    return sha1Impl(outer.data(), 84);
}

std::size_t base64(std::span<const std::uint8_t> data, char* out, std::size_t capacity) noexcept
{
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const std::size_t needed = ((data.size() + 2) / 3) * 4 + 1;
    if (!out || capacity < needed)
        return 0;

    std::size_t j = 0;
    for (std::size_t i = 0; i < data.size(); i += 3) {
        const std::uint32_t a = data[i];
        const std::uint32_t b = i + 1 < data.size() ? data[i + 1] : 0;
        const std::uint32_t c = i + 2 < data.size() ? data[i + 2] : 0;
        const std::uint32_t v = (a << 16) | (b << 8) | c;

        out[j++] = alphabet[(v >> 18) & 63];
        out[j++] = alphabet[(v >> 12) & 63];
        out[j++] = i + 1 < data.size() ? alphabet[(v >> 6) & 63] : '=';
        out[j++] = i + 2 < data.size() ? alphabet[v & 63] : '=';
    }
    out[j] = '\0';
    return j;
}

}
