#include "rtc/WebRtcHost.h"
#include "rtc/ice/Ice.h"
#include "rtc/signaling/Signaling.h"
#include "rtc/platform/WebSocket.h"
#include "rtc/Rtcp.h"
#include "rtc/RtpPacket.h"
#include <array>
#include <cstdint>
#include <memory>

namespace tinyrtc {

class WebRtcHost::Impl {
public:
    platform::WebSocketServer ws;
    Ice ice;
    RtpRetransmissionCache retransmission;
    DataHandler dataHandler;
    KeyframeHandler keyframeHandler;
    bool wsAuthenticated{};

    bool listen(std::uint16_t port, const char* token)
    {
        ws.setHandlers(
            [this] { wsAuthenticated = true; },
            [this](std::span<const std::uint8_t> data) {
                if (!wsAuthenticated)
                    return;
                std::array<std::uint8_t, 16> address{};
                std::uint16_t candidatePort{};
                bool v6{};
                if (!Signaling::parseCandidate(data.data(), data.size(), address, candidatePort, v6))
                    return;

                Candidate candidate{};
                candidate.address = address;
                candidate.port = candidatePort;
                candidate.v6 = v6;
                ice.addRemoteCandidate(candidate);
            },
            [this] {
                wsAuthenticated = false;
                ice.reset();
                retransmission.clear();
            });

        return ws.listen(port, token);
    }
};

WebRtcHost::WebRtcHost() : impl_(new Impl) {}

WebRtcHost::~WebRtcHost()
{
    stop();
    delete impl_;
}

bool WebRtcHost::listen(std::uint16_t port, const char* authToken)
{
    return impl_->listen(port, authToken);
}

void WebRtcHost::poll(int timeoutMs)
{
    impl_->ws.poll(timeoutMs);
}

void WebRtcHost::stop()
{
    if (!impl_)
        return;
    impl_->ws.close();
    impl_->wsAuthenticated = false;
    impl_->ice.reset();
    impl_->retransmission.clear();
}

bool WebRtcHost::sendRtp(const RtpPacket& packet)
{
    if (!impl_->wsAuthenticated)
        return false;

    // The packet is cached now so the eventual SRTP/UDP transport can answer NACKs.
    // Actual network emission is intentionally owned by the DTLS/SRTP transport layer.
    impl_->retransmission.store(packet);
    return false;
}

bool WebRtcHost::sendData(std::uint16_t, const std::uint8_t*, std::size_t)
{
    // SCTP/DTLS transport is not connected yet.
    return false;
}

void WebRtcHost::setDataHandler(DataHandler handler)
{
    impl_->dataHandler = std::move(handler);
}

void WebRtcHost::setKeyframeHandler(KeyframeHandler handler)
{
    impl_->keyframeHandler = std::move(handler);
}

bool WebRtcHost::connected() const noexcept
{
    return impl_->wsAuthenticated && impl_->ice.connected();
}

}
