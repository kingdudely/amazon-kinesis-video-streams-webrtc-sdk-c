#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

namespace tinyrtc {
class H264Encoder {
public:
    using Output = std::function<void(const std::uint8_t*, std::size_t, std::uint64_t, bool)>;
    virtual ~H264Encoder() = default;
    virtual bool init(std::uint32_t width, std::uint32_t height, std::uint32_t fps, std::uint32_t bitrate) = 0;
    virtual bool encode(const std::uint8_t* data, std::size_t size, std::uint64_t timestamp) = 0;
    virtual void setOutput(Output output) = 0;
    virtual void requestKeyframe() = 0;
};
}