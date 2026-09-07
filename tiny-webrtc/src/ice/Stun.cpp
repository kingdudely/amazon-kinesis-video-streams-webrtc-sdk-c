#include "rtc/ice/Stun.h"

namespace tinyrtc {

bool stunBindingRequest(const std::uint8_t*, std::size_t) {
    return false;
}

bool parseXorMappedAddress(const std::uint8_t*, std::size_t, StunAddress&) {
    return false;
}

}