#include "rtc/sctp/Sctp.h"

namespace tinyrtc {
bool Sctp::init() { return true; }
bool Sctp::input(const std::uint8_t* data, std::size_t size) { return data != nullptr || size == 0; }
bool Sctp::send(std::uint16_t, const std::uint8_t* data, std::size_t size) { return data != nullptr || size == 0; }
void Sctp::setMessageHandler(MessageHandler handler) { handler_ = std::move(handler); }
}