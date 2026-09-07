#include "rtc/video/H264Packetizer.h"
#include <algorithm>
#include <array>
#include <cstring>

namespace tinyrtc {
namespace {

static std::size_t startCode(const std::uint8_t* p, std::size_t n) noexcept
{
    if (n >= 4 && p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1) return 4;
    if (n >= 3 && p[0] == 0 && p[1] == 0 && p[2] == 1) return 3;
    return 0;
}

}

void H264Packetizer::packetize(const std::uint8_t* data, std::size_t size,
                               std::uint32_t timestamp, Output output) const
{
    if (!data || !size || !output || mtu_ <= 14)
        return;

    std::size_t pos = 0;
    while (pos < size) {
        const auto sc = startCode(data + pos, size - pos);
        if (!sc) {
            ++pos;
            continue;
        }
        pos += sc;

        const std::size_t nalStart = pos;
        std::size_t end = pos;
        while (end < size && !startCode(data + end, size - end))
            ++end;
        if (end <= nalStart)
            break;

        const auto* nal = data + nalStart;
        const std::size_t nalSize = end - nalStart;
        std::size_t next = end;
        while (next < size && data[next] == 0)
            ++next;
        const bool lastNal = next >= size || startCode(data + next, size - next) == 0;
        const std::size_t maxPayload = mtu_ - 12;

        if (nalSize <= maxPayload) {
            output(nal, nalSize, timestamp, lastNal);
        } else {
            const std::uint8_t naluHeader = nal[0];
            const std::size_t chunk = maxPayload - 2;
            std::array<std::uint8_t, 1500> payload{};
            std::size_t off = 1;

            while (off < nalSize) {
                const std::size_t take = std::min(chunk, nalSize - off);
                const bool first = off == 1;
                const bool last = off + take == nalSize;
                payload[0] = static_cast<std::uint8_t>((naluHeader & 0xE0) | 28);
                payload[1] = static_cast<std::uint8_t>((first ? 0x80 : 0) |
                                                       (last ? 0x40 : 0) |
                                                       (naluHeader & 0x1F));
                std::memcpy(payload.data() + 2, nal + off, take);
                output(payload.data(), take + 2, timestamp, last && lastNal);
                off += take;
            }
        }
        pos = end;
    }
}

}
