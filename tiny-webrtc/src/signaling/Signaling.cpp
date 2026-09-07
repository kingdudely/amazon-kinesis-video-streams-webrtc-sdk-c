#include "rtc/signaling/Signaling.h"
#include <algorithm>
#include <cstring>

namespace tinyrtc {

bool Signaling::parseCandidate(const std::uint8_t* data, std::size_t size,
                               std::array<std::uint8_t, 16>& address,
                               std::uint16_t& port, bool& v6) {
    if (!data || (size != V4_SIZE && size != V6_SIZE)) return false;
    address.fill(0);
    const std::size_t addressSize = size == V4_SIZE ? 4 : 16;
    std::copy_n(data, addressSize, address.begin());
    port = static_cast<std::uint16_t>(data[addressSize] << 8 | data[addressSize + 1]);
    v6 = size == V6_SIZE;
    return port != 0;
}

}