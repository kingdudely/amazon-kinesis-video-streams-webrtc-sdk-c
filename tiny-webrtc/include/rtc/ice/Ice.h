#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace tinyrtc {

struct Candidate {
    std::array<std::uint8_t, 16> address{};
    std::uint16_t port{};
    bool v6{};
};

class Ice {
public:
    static constexpr std::size_t MAX_CANDIDATES = 16;
    using CandidateHandler = std::function<void(const Candidate&)>;

    void setCandidateHandler(CandidateHandler handler);
    bool addRemoteCandidate(const Candidate& candidate) noexcept;
    void addServerReflexive(const Candidate& candidate) noexcept;

    const Candidate* remoteCandidate(std::size_t index) const noexcept;
    std::size_t remoteCandidateCount() const noexcept { return remoteCount_; }

    bool connected() const noexcept { return connected_; }
    void setConnected(bool value) noexcept { connected_ = value; }
    void reset() noexcept;

private:
    std::array<Candidate, MAX_CANDIDATES> candidates_{};
    std::size_t remoteCount_{};
    CandidateHandler candidateHandler_;
    bool connected_{};
};

}
