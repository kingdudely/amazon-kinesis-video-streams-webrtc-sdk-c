#include "input.hpp"
#include "rtp_source.hpp"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocketServer.h>
#include <nlohmann/json.hpp>
#include <rtc/rtc.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

namespace {

constexpr uint16_t SIGNALING_PORT = 8080;
constexpr uint16_t VIDEO_RTP_PORT = 5000;
constexpr uint16_t AUDIO_RTP_PORT = 5002;
constexpr uint32_t VIDEO_SSRC = 0x10000001;
constexpr uint32_t AUDIO_SSRC = 0x10000002;
constexpr uint8_t VIDEO_PT = 96;
constexpr uint8_t AUDIO_PT = 111;

std::string authToken() {
    if (const char* token = std::getenv("WEBRTC_AUTH_TOKEN")) return token;
    return {};
}

void setRtpFields(std::vector<uint8_t>& packet, uint8_t payloadType, uint32_t ssrc) {
    if (packet.size() < 12) return;
    packet[1] = static_cast<uint8_t>((packet[1] & 0x80u) | (payloadType & 0x7Fu));
    packet[8] = static_cast<uint8_t>(ssrc >> 24);
    packet[9] = static_cast<uint8_t>(ssrc >> 16);
    packet[10] = static_cast<uint8_t>(ssrc >> 8);
    packet[11] = static_cast<uint8_t>(ssrc);
}

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(ix::WebSocket& ws) : ws_(ws) {}

    void message(const json& packet) {
        const auto type = packet.value("type", "");

        if (!authenticated_) {
            if (type != "auth") {
                ws_.close();
                return;
            }
            const auto expected = authToken();
            if (!expected.empty() && packet.value("message", "") != expected) {
                ws_.close();
                return;
            }
            authenticated_ = true;
            ws_.send(R"({"type":"ready"})");
            return;
        }

        try {
            if (type == "ping") {
                ws_.send(R"({"type":"ping"})");
            } else if (type == "offer") {
                answer(packet.at("message").get<std::string>());
            } else if (type == "ice-candidate") {
                const auto& m = packet.at("message");
                pc_->addRemoteCandidate(rtc::Candidate(
                    m.at("candidate").get<std::string>(),
                    m.value("sdpMid", "0")));
            }
        } catch (const std::exception& e) {
            std::cerr << "signaling error: " << e.what() << '\n';
        }
    }

    bool authenticated() const { return authenticated_; }

    void sendVideo(std::vector<uint8_t> packet) {
        auto track = videoTrack_;
        if (!track || !track->isOpen()) return;
        setRtpFields(packet, videoPayloadType_, VIDEO_SSRC);
        try {
            track->send(reinterpret_cast<const std::byte*>(packet.data()), packet.size());
        } catch (const std::exception& e) {
            std::cerr << "video send: " << e.what() << '\n';
        }
    }

    void sendAudio(std::vector<uint8_t> packet) {
        auto track = audioTrack_;
        if (!track || !track->isOpen()) return;
        setRtpFields(packet, audioPayloadType_, AUDIO_SSRC);
        try {
            track->send(reinterpret_cast<const std::byte*>(packet.data()), packet.size());
        } catch (const std::exception& e) {
            std::cerr << "audio send: " << e.what() << '\n';
        }
    }

private:
    void answer(const std::string& sdp) {
        rtc::Configuration config;
        config.iceServers.emplace_back("stun:stun.l.google.com:19302");
        config.disableAutoNegotiation = true;
        config.enableIceUdpMux = true;

        pc_ = std::make_shared<rtc::PeerConnection>(config);
        auto weakSelf = weak_from_this();

        pc_->onStateChange([weakSelf](rtc::PeerConnection::State state) {
            if (auto self = weakSelf.lock()) {
                std::cerr << "WebRTC state: " << state << '\n';
                if (state == rtc::PeerConnection::State::Failed ||
                    state == rtc::PeerConnection::State::Closed) {
                    self->videoTrack_.reset();
                    self->audioTrack_.reset();
                }
            }
        });

        pc_->onLocalDescription([weakSelf](rtc::Description description) {
            if (auto self = weakSelf.lock()) {
                json message = {
                    {"type", "answer"},
                    {"message", {
                        {"type", description.typeString()},
                        {"sdp", std::string(description)}
                    }}
                };
                self->ws_.send(message.dump());
            }
        });

        pc_->onLocalCandidate([weakSelf](rtc::Candidate candidate) {
            if (auto self = weakSelf.lock()) {
                json message = {
                    {"type", "ice-candidate"},
                    {"message", {
                        {"candidate", std::string(candidate)},
                        {"sdpMid", candidate.mid()}
                    }}
                };
                self->ws_.send(message.dump());
            }
        });

        // The browser creates these channels as negotiated channels. We therefore
        // create the matching IDs locally rather than waiting for DCEP messages.
        createDataChannel("pointer-movement", 0, false, 0);
        createDataChannel("pointer-click", 1, true, -1);
        createDataChannel("pointer-scroll", 2, false, 0);
        createDataChannel("keyboard-type", 3, true, -1);
        createDataChannel("clipboard-sync", 4, true, -1);

        rtc::Description offer(sdp, "offer");
        pc_->setRemoteDescription(offer);

        rtc::Description::Video video("video", rtc::Description::Direction::SendOnly);
        video.addH264Codec(VIDEO_PT);
        video.addSSRC(VIDEO_SSRC, "video-host", "stream", "video");
        videoTrack_ = pc_->addTrack(video);
        videoTrack_->onOpen([] { std::cerr << "video track open\n"; });

        rtc::Description::Audio audio("audio", rtc::Description::Direction::SendOnly);
        audio.addOpusCodec(AUDIO_PT);
        audio.addSSRC(AUDIO_SSRC, "audio-host", "stream", "audio");
        audioTrack_ = pc_->addTrack(audio);
        audioTrack_->onOpen([] { std::cerr << "audio track open\n"; });

        pc_->setLocalDescription();
    }

    void createDataChannel(const char* label, uint16_t id, bool ordered, int maxRetransmits) {
        rtc::DataChannelInit init;
        init.negotiated = true;
        init.id = id;
        init.reliability.unordered = !ordered;
        if (maxRetransmits >= 0) init.reliability.maxRetransmits = maxRetransmits;

        auto channel = pc_->createDataChannel(label, init);
        channel->onOpen([label] { std::cerr << "data channel open: " << label << '\n'; });

        if (id == 0) {
            channel->onMessage([this](auto data) {
                if (!std::holds_alternative<rtc::binary>(data)) return;
                const auto& bytes = std::get<rtc::binary>(data);
                if (bytes.size() < 9) return;
                const auto* p = reinterpret_cast<const uint8_t*>(bytes.data());
                const bool relative = p[0] != 0;
                int32_t x, y;
                std::memcpy(&x, p + 1, sizeof(x));
                std::memcpy(&y, p + 5, sizeof(y));
                input_.pointerMove(relative, x, y);
            });
        } else if (id == 1) {
            channel->onMessage([this](auto data) {
                if (!std::holds_alternative<rtc::binary>(data)) return;
                const auto& bytes = std::get<rtc::binary>(data);
                if (bytes.size() < 2) return;
                const auto* p = reinterpret_cast<const uint8_t*>(bytes.data());
                input_.pointerButton(p[0] != 0, p[1]);
            });
        } else if (id == 2) {
            channel->onMessage([this](auto data) {
                if (!std::holds_alternative<rtc::binary>(data)) return;
                const auto& bytes = std::get<rtc::binary>(data);
                if (bytes.size() < 13) return;
                const auto* p = reinterpret_cast<const uint8_t*>(bytes.data());
                uint8_t mode = p[0];
                float x, y, z;
                std::memcpy(&x, p + 1, 4);
                std::memcpy(&y, p + 5, 4);
                std::memcpy(&z, p + 9, 4);
                input_.scroll(mode, x, y, z);
            });
        } else if (id == 3) {
            channel->onMessage([this](auto data) {
                if (!std::holds_alternative<rtc::binary>(data)) return;
                const auto& bytes = std::get<rtc::binary>(data);
                if (bytes.size() < 2) return;
                const auto* p = reinterpret_cast<const uint8_t*>(bytes.data());
                input_.key(p[1], p[0] != 0);
            });
        } else if (id == 4) {
            channel->onMessage([this](auto data) {
                if (!std::holds_alternative<std::string>(data)) return;
                input_.setClipboard(std::get<std::string>(data));
            });
        }

        channels_.push_back(std::move(channel));
    }

    ix::WebSocket& ws_;
    bool authenticated_ = false;
    std::shared_ptr<rtc::PeerConnection> pc_;
    std::shared_ptr<rtc::Track> videoTrack_;
    std::shared_ptr<rtc::Track> audioTrack_;
    std::vector<std::shared_ptr<rtc::DataChannel>> channels_;
    RemoteInput input_;
    uint8_t videoPayloadType_ = VIDEO_PT;
    uint8_t audioPayloadType_ = AUDIO_PT;
};

std::mutex sessionsMutex;
std::unordered_map<std::string, std::shared_ptr<Session>> sessions;

} // namespace

int main() {
    rtc::InitLogger(rtc::LogLevel::Warning);
    ix::initNetSystem();

    ix::WebSocketServer server(SIGNALING_PORT, "0.0.0.0");
    server.disablePerMessageDeflate();

    server.setOnClientMessageCallback([](std::shared_ptr<ix::ConnectionState> state,
                                         ix::WebSocket& ws,
                                         const ix::WebSocketMessagePtr& message) {
        const auto id = state->getId();

        if (message->type == ix::WebSocketMessageType::Open) {
            auto session = std::make_shared<Session>(ws);
            std::lock_guard lock(sessionsMutex);
            sessions[id] = std::move(session);
            std::cerr << "WebSocket connected: " << id << '\n';
            return;
        }

        if (message->type == ix::WebSocketMessageType::Close) {
            std::lock_guard lock(sessionsMutex);
            sessions.erase(id);
            return;
        }

        if (message->type != ix::WebSocketMessageType::Message) return;

        std::shared_ptr<Session> session;
        {
            std::lock_guard lock(sessionsMutex);
            auto it = sessions.find(id);
            if (it != sessions.end()) session = it->second;
        }
        if (!session) return;

        try {
            session->message(json::parse(message->str));
        } catch (const std::exception& e) {
            std::cerr << "invalid signaling message: " << e.what() << '\n';
        }
    });

    auto result = server.listen();
    if (!result.first) {
        std::cerr << "WebSocket listen failed: " << result.second << '\n';
        ix::uninitNetSystem();
        return 1;
    }

    RtpSource videoSource(VIDEO_RTP_PORT, [](std::vector<uint8_t>&& packet) {
        std::lock_guard lock(sessionsMutex);
        for (auto& [_, session] : sessions) {
            if (session->authenticated()) session->sendVideo(std::vector<uint8_t>(packet));
        }
    });

    RtpSource audioSource(AUDIO_RTP_PORT, [](std::vector<uint8_t>&& packet) {
        std::lock_guard lock(sessionsMutex);
        for (auto& [_, session] : sessions) {
            if (session->authenticated()) session->sendAudio(std::vector<uint8_t>(packet));
        }
    });

    videoSource.start();
    audioSource.start();
    server.start();

    std::cerr << "minimal WebRTC host listening on ws://0.0.0.0:" << SIGNALING_PORT << '\n';
    std::cerr << "H264 RTP input: udp://127.0.0.1:" << VIDEO_RTP_PORT << '\n';
    std::cerr << "Opus RTP input: udp://127.0.0.1:" << AUDIO_RTP_PORT << '\n';
    if (!authToken().empty()) std::cerr << "WebSocket authentication enabled\n";

    server.wait();

    audioSource.stop();
    videoSource.stop();
    ix::uninitNetSystem();
    rtc::Cleanup();
    return 0;
}
