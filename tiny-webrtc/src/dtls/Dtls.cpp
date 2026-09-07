#include "rtc/dtls/Dtls.h"
namespace tinyrtc {
bool Dtls::startServer() { return false; }
bool Dtls::input(std::span<const std::uint8_t>) { return false; }
bool Dtls::exportKeyingMaterial(std::span<std::uint8_t>) { return false; }
}