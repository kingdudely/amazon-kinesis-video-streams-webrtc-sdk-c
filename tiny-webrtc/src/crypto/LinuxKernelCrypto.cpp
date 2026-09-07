#include "rtc/crypto/CryptoProvider.h"

#if defined(TINYRTC_LINUX)

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../../kernel-crypto/include/tinycrypto_uapi.h"

namespace tinyrtc {

class LinuxKernelCrypto final : public CryptoProvider {
public:
    explicit LinuxKernelCrypto(const char* device = "/dev/tinycrypto") noexcept
        : fd_(::open(device, O_RDWR | O_CLOEXEC)) {}

    ~LinuxKernelCrypto() override {
        if (fd_ >= 0)
            ::close(fd_);
    }

    LinuxKernelCrypto(const LinuxKernelCrypto&) = delete;
    LinuxKernelCrypto& operator=(const LinuxKernelCrypto&) = delete;

    bool valid() const noexcept { return fd_ >= 0; }

    bool random(std::span<std::uint8_t> out) noexcept override {
        if (fd_ < 0 || out.empty()) return false;
        tinycrypto_random_req req{};
        req.output = reinterpret_cast<__u64>(out.data());
        req.output_len = static_cast<__u32>(out.size());
        return ::ioctl(fd_, TINYCRYPTO_RANDOM, &req) == 0;
    }

    bool sha1(std::span<const std::uint8_t> in,
              std::span<std::uint8_t, 20> out) noexcept override {
        if (fd_ < 0 || in.size() > UINT32_MAX) return false;
        tinycrypto_hash_req req{};
        req.input = reinterpret_cast<__u64>(in.data());
        req.input_len = static_cast<__u32>(in.size());
        if (::ioctl(fd_, TINYCRYPTO_SHA1, &req) != 0) return false;
        std::memcpy(out.data(), req.output, out.size());
        return true;
    }

    bool sha256(std::span<const std::uint8_t> in,
                std::span<std::uint8_t, 32> out) noexcept override {
        if (fd_ < 0 || in.size() > UINT32_MAX) return false;
        tinycrypto_hash_req req{};
        req.input = reinterpret_cast<__u64>(in.data());
        req.input_len = static_cast<__u32>(in.size());
        if (::ioctl(fd_, TINYCRYPTO_SHA256, &req) != 0) return false;
        std::memcpy(out.data(), req.output, out.size());
        return true;
    }

    bool hmacSha1(std::span<const std::uint8_t> key,
                  std::span<const std::uint8_t> in,
                  std::span<std::uint8_t, 20> out) noexcept override {
        if (fd_ < 0 || key.size() > TINYCRYPTO_MAX_KEY_SIZE || in.size() > UINT32_MAX) return false;
        tinycrypto_hmac_req req{};
        req.key = reinterpret_cast<__u64>(key.data());
        req.key_len = static_cast<__u32>(key.size());
        req.input = reinterpret_cast<__u64>(in.data());
        req.input_len = static_cast<__u32>(in.size());
        if (::ioctl(fd_, TINYCRYPTO_HMAC_SHA1, &req) != 0) return false;
        std::memcpy(out.data(), req.output, out.size());
        return true;
    }

    bool hmacSha256(std::span<const std::uint8_t> key,
                    std::span<const std::uint8_t> in,
                    std::span<std::uint8_t, 32> out) noexcept override {
        if (fd_ < 0 || key.size() > TINYCRYPTO_MAX_KEY_SIZE || in.size() > UINT32_MAX) return false;
        tinycrypto_hmac_req req{};
        req.key = reinterpret_cast<__u64>(key.data());
        req.key_len = static_cast<__u32>(key.size());
        req.input = reinterpret_cast<__u64>(in.data());
        req.input_len = static_cast<__u32>(in.size());
        if (::ioctl(fd_, TINYCRYPTO_HMAC_SHA256, &req) != 0) return false;
        std::memcpy(out.data(), req.output, out.size());
        return true;
    }

    bool aesGcmEncrypt(std::span<std::uint8_t> data,
                       std::span<const std::uint8_t> aad,
                       std::span<const std::uint8_t> key,
                       std::span<const std::uint8_t, 12> iv,
                       std::span<std::uint8_t, 16> tag) noexcept override {
        return aesGcm(data, aad, key, iv, tag, false);
    }

    bool aesGcmDecrypt(std::span<std::uint8_t> data,
                       std::span<const std::uint8_t> aad,
                       std::span<const std::uint8_t> key,
                       std::span<const std::uint8_t, 12> iv,
                       std::span<const std::uint8_t, 16> tag) noexcept override {
        return aesGcm(data, aad, key, iv, tag, true);
    }

private:
    bool aesGcm(std::span<std::uint8_t> data,
                std::span<const std::uint8_t> aad,
                std::span<const std::uint8_t> key,
                std::span<const std::uint8_t, 12> iv,
                std::span<std::uint8_t, 16> tag,
                bool decrypt) noexcept {
        if (fd_ < 0 || data.size() > UINT32_MAX || aad.size() > UINT32_MAX ||
            (key.size() != 16 && key.size() != 24 && key.size() != 32)) return false;

        tinycrypto_gcm_req req{};
        req.data = reinterpret_cast<__u64>(data.data());
        req.data_len = static_cast<__u32>(data.size());
        req.aad = reinterpret_cast<__u64>(aad.data());
        req.aad_len = static_cast<__u32>(aad.size());
        req.key_len = static_cast<__u8>(key.size());
        std::memcpy(req.key, key.data(), key.size());
        std::memcpy(req.iv, iv.data(), iv.size());
        std::memcpy(req.tag, tag.data(), tag.size());

        if (::ioctl(fd_, decrypt ? TINYCRYPTO_AES_GCM_DEC : TINYCRYPTO_AES_GCM_ENC, &req) != 0)
            return false;

        if (!decrypt)
            std::memcpy(tag.data(), req.tag, tag.size());
        return true;
    }

    int fd_;
};

} // namespace tinyrtc

#endif
