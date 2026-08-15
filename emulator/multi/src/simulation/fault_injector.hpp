#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <random>

namespace ecumult {

class FaultInjector {
public:
    FaultInjector();

    void enable(bool on);
    bool isEnabled() const;

    void setProbability(float prob);
    float getProbability() const;

    void addDTC(uint16_t code, const std::string& description);
    void removeDTC(uint16_t code);
    void clearDTCs();

    struct FaultConfig {
        uint16_t code;
        std::string description;
        float probability_weight;
    };

    void setFaults(const std::vector<FaultConfig>& faults);
    std::vector<FaultConfig> getFaults() const;

    // Returns true if a fault should be triggered now
    bool shouldTrigger(uint16_t* out_code = nullptr);
    void triggerNow(uint16_t code);

private:
    bool enabled_;
    float probability_;
    std::vector<FaultConfig> faults_;
    mutable std::mt19937 rng_;
};

} // namespace ecumult
