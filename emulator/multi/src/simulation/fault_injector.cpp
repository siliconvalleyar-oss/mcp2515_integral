#include "fault_injector.hpp"
#include <algorithm>

namespace ecumult {

FaultInjector::FaultInjector()
    : enabled_(false)
    , probability_(0.0f)
    , rng_(std::random_device{}()) {}

void FaultInjector::enable(bool on) {
    enabled_ = on;
}

bool FaultInjector::isEnabled() const {
    return enabled_;
}

void FaultInjector::setProbability(float prob) {
    probability_ = std::clamp(prob, 0.0f, 1.0f);
}

float FaultInjector::getProbability() const {
    return probability_;
}

void FaultInjector::addDTC(uint16_t code, const std::string& description) {
    faults_.push_back({code, description, 1.0f});
}

void FaultInjector::removeDTC(uint16_t code) {
    faults_.erase(
        std::remove_if(faults_.begin(), faults_.end(),
            [code](const FaultConfig& f) { return f.code == code; }),
        faults_.end());
}

void FaultInjector::clearDTCs() {
    faults_.clear();
}

void FaultInjector::setFaults(const std::vector<FaultConfig>& faults) {
    faults_ = faults;
}

std::vector<FaultInjector::FaultConfig> FaultInjector::getFaults() const {
    return faults_;
}

bool FaultInjector::shouldTrigger(uint16_t* out_code) {
    if (!enabled_ || faults_.empty()) return false;

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(rng_) > probability_) return false;

    std::uniform_int_distribution<size_t> idx_dist(0, faults_.size() - 1);
    auto& fault = faults_[idx_dist(rng_)];
    if (out_code) *out_code = fault.code;
    return true;
}

void FaultInjector::triggerNow(uint16_t code) {
    (void)code;
    // Immediate fault trigger - handled by the caller
}

} // namespace ecumult
