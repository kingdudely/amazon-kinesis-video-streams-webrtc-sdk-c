#include "rtc/srtp/Srtp.h"
#include <algorithm>
namespace tinyrtc {
bool Srtp::init(std::span<const std::uint8_t> keyMaterial) { ready_ = !keyMaterial.empty(); return ready_; }
bool Srtp::protectRtp(std::span<std::uint8_t>, std::size_t& size) { return ready_ && size != 0; }
bool Srtp::protectRtcp(std::span<std::uint8_t>, std::size_t& size) { return ready_ && size != 0; }
bool Srtp::unprotectRtcp(std::span<std::uint8_t>, std::size_t& size) { return ready_ && size != 0; }
}