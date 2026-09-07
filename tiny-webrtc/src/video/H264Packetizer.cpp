#include "rtc/video/H264Packetizer.h"
#include <algorithm>

namespace tinyrtc {

static std::size_t startCode(const std::uint8_t* p, std::size_t n) {
    if (n >= 4 && p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1) return 4;
    if (n >= 3 && p[0] == 0 && p[1] == 0 && p[2] == 1) return 3;
    return 0;
}

void H264Packetizer::packetize(const std::uint8_t* data, std::size_t size,
                               std::uint32_t timestamp, Output output) const {
    if (!data || !size || !output || mtu_ <= 2) return;

    std::size_t pos = 0;
    while (pos < size) {
        while (pos < size && data[pos] == 0) ++pos;
        if (pos >= size) break;
        std::size_t sc = startCode(data + pos, size - pos);
        if (sc) pos += sc;
        std::size_t end = pos;
        while (end < size && startCode(data + end, size - end) == 0) ++end;
        if (end == pos) break;

        const auto* nal = data + pos;
        const std::size_t nalSize = end - pos;
        const std::size_t maxPayload = mtu_ - 12;
        const std::size_t chunk = std::max<std::size_t>(1, maxPayload - 2);

        if (nalSize <= maxPayload) {
            output(nal, nalSize, timestamp, true);
        } else {
            const std::uint8_t naluHeader = nal[0];
            std::uint8_t fu[2];
            fu[0] = static_cast<std::uint8_t>((naluHeader & 0xE0) | 28);
            std::size_t off = 1;
            while (off < nalSize) {
                const std::size_t take = std::min(chunk, nalSize - off);
                fu[1] = static_cast<std::uint8_t>((off == 1 ? 0x80 : 0) | ((off + take) == nalSize ? 0x40 : 0) | (naluHeader & 0x1F));
                output(fu, 2, timestamp, (off + take) == nalSize);
                off += take;
            }
        }
        pos = end;
    }
}

}