#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace tinyrtc {

class Dtls {
public:
    Dtls() = default;
    ~Dtls() = default;
    Dtls(const Dtls&) = delete;
    Dtls& operator=(const Dtls&) = delete;

    bool startServer();
    bool input(std::span<const std::uint8_t> packet);
    bool ready() const noexcept { return ready_; }
    bool exportKeyingMaterial(std::span<std::uint8_t> out);

private:
    bool ready_{};
};

}