#include "rtc/ice/Stun.h"
#include "rtc/CryptoTiny.h"
#include <algorithm>
#include <array>
#include <cstring>

namespace tinyrtc {
namespace {

constexpr std::uint16_t BINDING_REQUEST = 0x0001;
constexpr std::uint16_t BINDING_SUCCESS = 0x0101;
constexpr std::uint16_t ATTR_USERNAME = 0x0006;
constexpr std::uint16_t ATTR_MESSAGE_INTEGRITY = 0x0008;
constexpr std::uint16_t ATTR_XOR_MAPPED_ADDRESS = 0x0020;
constexpr std::uint16_t ATTR_FINGERPRINT = 0x8028;

static inline std::uint16_t get16(const std::uint8_t* p) noexcept
{
    return static_cast<std::uint16_t>((std::uint16_t(p[0]) << 8) | p[1]);
}

static inline std::uint32_t get32(const std::uint8_t* p) noexcept
{
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
           (std::uint32_t(p[2]) << 8) | p[3];
}

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

static std::uint32_t crc32(const std::uint8_t* data, std::size_t size) noexcept
{
    std::uint32_t crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned b = 0; b < 8; ++b)
            crc = (crc >> 1) ^ (0xEDB88320u & (-(crc & 1u)));
    }
    return ~crc;
}

static bool appendAttr(std::array<std::uint8_t, 256>& out, std::size_t& size,
                       std::uint16_t type, const std::uint8_t* value, std::size_t valueLen) noexcept
{
    const std::size_t padded = (valueLen + 3u) & ~std::size_t(3u);
    if (size + 4 + padded > out.size() || valueLen > 0xFFFF)
        return false;
    put16(out.data() + size, type);
    put16(out.data() + size + 2, static_cast<std::uint16_t>(valueLen));
    if (valueLen)
        std::memcpy(out.data() + size + 4, value, valueLen);
    std::memset(out.data() + size + 4 + valueLen, 0, padded - valueLen);
    size += 4 + padded;
    return true;
}

}

bool isStunPacket(std::span<const std::uint8_t> data) noexcept
{
    return data.size() >= STUN_HEADER_SIZE && (data[0] & 0xC0) == 0 &&
           get32(data.data() + 4) == STUN_MAGIC_COOKIE &&
           static_cast<std::size_t>(get16(data.data() + 2)) + STUN_HEADER_SIZE <= data.size();
}

bool isBindingRequest(std::span<const std::uint8_t> data) noexcept
{
    return isStunPacket(data) && get16(data.data()) == BINDING_REQUEST;
}

bool buildBindingRequest(std::array<std::uint8_t, 256>& out,
                         std::size_t& size,
                         const std::uint8_t transactionId[12],
                         const char* username,
                         const char* password) noexcept
{
    if (!transactionId || !username || !password)
        return false;

    out.fill(0);
    size = STUN_HEADER_SIZE;
    put16(out.data(), BINDING_REQUEST);
    put32(out.data() + 4, STUN_MAGIC_COOKIE);
    std::memcpy(out.data() + 8, transactionId, 12);

    const auto nameLen = std::strlen(username);
    if (!appendAttr(out, size, ATTR_USERNAME,
                    reinterpret_cast<const std::uint8_t*>(username), nameLen))
        return false;

    // MESSAGE-INTEGRITY covers the message through the end of this attribute.
    put16(out.data() + 2, static_cast<std::uint16_t>(size + 4 + 20 - STUN_HEADER_SIZE));
    const std::size_t integrityStart = size;
    std::array<std::uint8_t, 20> zero{};
    if (!appendAttr(out, size, ATTR_MESSAGE_INTEGRITY, zero.data(), zero.size()))
        return false;

    // HMAC covers exactly the bytes ending at the MESSAGE-INTEGRITY value.
    const auto digest = crypto::hmacSha1(
        std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(password), std::strlen(password)),
        std::span<const std::uint8_t>(out.data(), size));
    std::memcpy(out.data() + integrityStart + 4, digest.data(), digest.size());

    // Restore the final message length and append FINGERPRINT.
    const std::size_t beforeFingerprint = size;
    put16(out.data() + 2, static_cast<std::uint16_t>((beforeFingerprint + 8) - STUN_HEADER_SIZE));
    const std::size_t fpStart = size;
    std::array<std::uint8_t, 4> fingerprint{};
    if (!appendAttr(out, size, ATTR_FINGERPRINT, fingerprint.data(), fingerprint.size()))
        return false;

    // Fingerprint is CRC32 of the packet through the attribute header, XORed with the STUN constant.
    put32(out.data() + fpStart + 4, crc32(out.data(), fpStart) ^ 0x5354554Eu);
    return true;
}

bool buildBindingResponse(std::array<std::uint8_t, 256>& out,
                          std::size_t& size,
                          const std::uint8_t transactionId[12],
                          std::uint32_t ip,
                          std::uint16_t port) noexcept
{
    if (!transactionId)
        return false;

    out.fill(0);
    size = STUN_HEADER_SIZE;
    put16(out.data(), BINDING_SUCCESS);
    put32(out.data() + 4, STUN_MAGIC_COOKIE);
    std::memcpy(out.data() + 8, transactionId, 12);

    std::array<std::uint8_t, 8> value{};
    value[1] = 0x01; // IPv4
    const std::uint16_t xport = static_cast<std::uint16_t>(port ^ (STUN_MAGIC_COOKIE >> 16));
    put16(value.data() + 2, xport);
    put32(value.data() + 4, ip ^ STUN_MAGIC_COOKIE);
    return appendAttr(out, size, ATTR_XOR_MAPPED_ADDRESS, value.data(), 8);
}

bool parseXorMappedAddress(std::span<const std::uint8_t> data, StunAddress& out) noexcept
{
    if (!isStunPacket(data))
        return false;

    const std::size_t messageEnd = STUN_HEADER_SIZE + get16(data.data() + 2);
    std::size_t at = STUN_HEADER_SIZE;
    while (at + 4 <= messageEnd) {
        const std::uint16_t type = get16(data.data() + at);
        const std::uint16_t len = get16(data.data() + at + 2);
        const std::size_t padded = (std::size_t(len) + 3) & ~std::size_t(3);
        if (at + 4 + padded > messageEnd)
            return false;

        if (type == ATTR_XOR_MAPPED_ADDRESS && (len == 8 || len == 20)) {
            const auto* v = data.data() + at + 4;
            const std::uint8_t family = v[1];
            out.port = static_cast<std::uint16_t>(get16(v + 2) ^ (STUN_MAGIC_COOKIE >> 16));
            out.address.fill(0);
            if (family == 0x01 && len == 8) {
                const std::uint32_t addr = get32(v + 4) ^ STUN_MAGIC_COOKIE;
                out.address[0] = static_cast<std::uint8_t>(addr >> 24);
                out.address[1] = static_cast<std::uint8_t>(addr >> 16);
                out.address[2] = static_cast<std::uint8_t>(addr >> 8);
                out.address[3] = static_cast<std::uint8_t>(addr);
                out.v6 = false;
                return true;
            }
            if (family == 0x02 && len == 20) {
                // XOR with magic cookie followed by the 96-bit transaction ID.
                for (unsigned i = 0; i < 4; ++i)
                    out.address[i] = static_cast<std::uint8_t>(v[4 + i] ^ (STUN_MAGIC_COOKIE >> (24 - 8 * i)));
                for (unsigned i = 4; i < 16; ++i)
                    out.address[i] = static_cast<std::uint8_t>(v[4 + i] ^ data[8 + i - 4]);
                out.v6 = true;
                return true;
            }
        }
        at += 4 + padded;
    }
    return false;
}

}
