#pragma once
#include <cstdint>
#include <vector>

namespace tinyrtc {

struct Candidate {
    std::uint8_t address[16]{};
    std::uint16_t port{};
    bool v6{};
};

class Ice {
public:
    void addServerReflexive(const Candidate& candidate);
    bool connected() const noexcept { return connected_; }
    const std::vector<Candidate>& candidates() const noexcept { return candidates_; }
    void reset();
private:
    std::vector<Candidate> candidates_;
    bool connected_{};
};

}