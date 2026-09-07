#include "rtc/ice/Ice.h"

namespace tinyrtc {

void Ice::addServerReflexive(const Candidate& candidate) {
    candidates_.push_back(candidate);
}

void Ice::reset() {
    candidates_.clear();
    connected_ = false;
}

}