#include "rtc/RtpPacket.h"
#include <cstdint>

namespace tinyrtc {

// Common RTP header serializer. Payload data is supplied by the codec packetizer.
// This intentionally does not know anything about H.264 or Opus.
bool buildRtpHeader(std::uint8_t* out, std::uint8_t payloadType, bool marker,
                    std::uint16_t sequence, std::uint32_t timestamp,
                    std::uint32_t ssrc) noexcept
{
    if (!out) return false;
    out[0] = 0x80;
    out[1] = static_cast<std::uint8_t>(payloadType | (marker ? 0x80 : 0));
    out[2] = static_cast<std::uint8_t>(sequence >> 8);
    out[3] = static_cast<std::uint8_t>(sequence);
    out[4] = static_cast<std::uint8_t>(timestamp >> 24);
    out[5] = static_cast<std::uint8_t>(timestamp >> 16);
    out[6] = static_cast<std::uint8_t>(timestamp >> 8);
    out[7] = static_cast<std::uint8_t>(timestamp);
    out[8] = static_cast<std::uint8_t>(ssrc >> 24);
    out[9] = static_cast<std::uint8_t>(ssrc >> 16);
    out[10] = static_cast<std::uint8_t>(ssrc >> 8);
    out[11] = static_cast<std::uint8_t>(ssrc);
    return true;
}

}