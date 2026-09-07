#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include "RtpPacket.h"

namespace tinyrtc {

class WebRtcHost {
public:
    using DataHandler = std::function<void(std::uint16_t, const std::uint8_t*, std::size_t)>;
    using KeyframeHandler = std::function<void()>;

    WebRtcHost();
    ~WebRtcHost();

    WebRtcHost(const WebRtcHost&) = delete;
    WebRtcHost& operator=(const WebRtcHost&) = delete;

    bool listen(std::uint16_t port, const char* authToken);
    void poll(int timeoutMs = 0);
    void stop();

    bool sendRtp(const RtpPacket& packet);
    bool sendData(std::uint16_t streamId, const std::uint8_t* data, std::size_t size);

    void setDataHandler(DataHandler handler);
    void setKeyframeHandler(KeyframeHandler handler);

    bool connected() const noexcept;

private:
    class Impl;
    Impl* impl_;
};

}