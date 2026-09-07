#include "rtc/audio/OpusPacketizer.h"

namespace tinyrtc {

void OpusPacketizer::packetize(const std::uint8_t* frame, std::size_t size,
                               std::uint32_t timestamp, Output output) const
{
    if (frame && size && output)
        output(frame, size, timestamp, false);
}

}
