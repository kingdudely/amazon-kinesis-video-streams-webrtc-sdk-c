#include "rtc/WebRtcHost.h"
#include "rtc/ice/Ice.h"
#include "rtc/signaling/Signaling.h"
#include "rtc/platform/WebSocket.h"
#include <array>
#include <cstring>

namespace tinyrtc {

class WebRtcHost::Impl {
public:
    platform::WebSocketServer ws;
    Ice ice;
    DataHandler dataHandler;
    KeyframeHandler keyframeHandler;
    bool connected{};

    bool listen(std::uint16_t port, const char* token) {
        ws.setHandlers(
            [this] { connected = true; },
            [this](std::span<const std::uint8_t> data) {
                std::array<std::uint8_t, 16> address{};
                std::uint16_t port{};
                bool v6{};
                if (!Signaling::parseCandidate(data.data(), data.size(), address, port, v6)) {
                    return;
                }
                Candidate c{};
                std::memcpy(c.address, address.data(), 16);
                c.port = port;
                c.v6 = v6;
                ice.addServerReflexive(c);
            },
            [this] { connected = false; ice.reset(); });
        return ws.listen(port, token);
    }
};

WebRtcHost::WebRtcHost() : impl_(new Impl) {}
WebRtcHost::~WebRtcHost() { stop(); delete impl_; }

bool WebRtcHost::listen(std::uint16_t port, const char* authToken) { return impl_->listen(port, authToken); }
void WebRtcHost::poll(int timeoutMs) { impl_->ws.poll(timeoutMs); }
void WebRtcHost::stop() { impl_->ws.close(); impl_->connected = false; }

bool WebRtcHost::sendRtp(const RtpPacket&) { return impl_->connected; }

bool WebRtcHost::sendData(std::uint16_t streamId, const std::uint8_t* data, std::size_t size) {
    if (!impl_->connected) return false;
    return impl_->dataHandler ? (impl_->dataHandler(streamId, data, size), true) : true;
}

void WebRtcHost::setDataHandler(DataHandler handler) { impl_->dataHandler = std::move(handler); }
void WebRtcHost::setKeyframeHandler(KeyframeHandler handler) { impl_->keyframeHandler = std::move(handler); }
bool WebRtcHost::connected() const noexcept { return impl_->connected; }

}