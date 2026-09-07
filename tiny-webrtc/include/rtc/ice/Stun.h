#pragma once
#include <cstddef>
#include <cstdint>

namespace tinyrtc {

bool stunBindingRequest(const std::uint8_t* data, std::size_t size);

struct StunAddress {
    std::uint8_t address[16]{};
    std::uint16_t port{};
    bool v6{};
};

bool parseXorMappedAddress(const std::uint8_t* data, std::size_t size, StunAddress& out);

}