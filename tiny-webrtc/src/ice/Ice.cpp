#include "rtc/ice/Ice.h"
#include <cstring>

namespace tinyrtc {

void Ice::setCandidateHandler(CandidateHandler handler)
{
    candidateHandler_ = std::move(handler);
}

bool Ice::addRemoteCandidate(const Candidate& candidate) noexcept
{
    if (candidate.port == 0 || remoteCount_ >= MAX_CANDIDATES)
        return false;

    for (std::size_t i = 0; i < remoteCount_; ++i) {
        if (candidates_[i].port == candidate.port && candidates_[i].v6 == candidate.v6 &&
            std::memcmp(candidates_[i].address.data(), candidate.address.data(), candidate.v6 ? 16 : 4) == 0)
            return true;
    }

    candidates_[remoteCount_++] = candidate;
    if (candidateHandler_)
        candidateHandler_(candidate);
    return true;
}

void Ice::addServerReflexive(const Candidate& candidate) noexcept
{
    if (candidate.port == 0)
        return;
    if (candidateHandler_)
        candidateHandler_(candidate);
}

const Candidate* Ice::remoteCandidate(std::size_t index) const noexcept
{
    return index < remoteCount_ ? &candidates_[index] : nullptr;
}

void Ice::reset() noexcept
{
    remoteCount_ = 0;
    connected_ = false;
    for (auto& c : candidates_)
        c = {};
}

}
