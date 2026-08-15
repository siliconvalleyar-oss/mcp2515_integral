#include "readiness.hpp"

namespace ecumult {

ReadinessMonitor::ReadinessMonitor() {
    reset();
}

void ReadinessMonitor::setSupported(uint8_t monitor_id, bool supported) {
    if (monitor_id < 8) supported_[monitor_id] = supported;
}

void ReadinessMonitor::setComplete(uint8_t monitor_id, bool complete) {
    if (monitor_id < 8) complete_[monitor_id] = complete;
}

bool ReadinessMonitor::isSupported(uint8_t monitor_id) const {
    if (monitor_id >= 8) return false;
    return supported_[monitor_id];
}

bool ReadinessMonitor::isComplete(uint8_t monitor_id) const {
    if (monitor_id >= 8) return false;
    return complete_[monitor_id];
}

std::vector<ReadinessMonitor::MonitorStatus> ReadinessMonitor::getAllMonitors() const {
    std::vector<MonitorStatus> result;
    for (uint8_t i = 0; i < 8; i++) {
        if (supported_[i]) {
            result.push_back({monitorName(i), true, complete_[i]});
        }
    }
    return result;
}

std::vector<uint8_t> ReadinessMonitor::getOBDStatus() const {
    // Byte 1: MIL status + DTC count
    uint8_t b1 = 0x00;
    for (int i = 0; i < 8; i++) {
        if (supported_[i] && complete_[i]) b1 |= (1 << (7 - i));
    }
    return {b1, 0x07, 0xFF, 0x00, 0x00};
}

bool ReadinessMonitor::allComplete() const {
    for (uint8_t i = 0; i < 8; i++) {
        if (supported_[i] && !complete_[i]) return false;
    }
    return true;
}

void ReadinessMonitor::reset() {
    for (int i = 0; i < 8; i++) {
        supported_[i] = true;
        complete_[i] = true;
    }
}

std::string ReadinessMonitor::monitorName(uint8_t id) {
    static const char* names[] = {
        "Misfire", "Fuel System", "Components",
        "Catalyst", "Heated Catalyst", "EVAP System",
        "Secondary Air", "AC System"
    };
    if (id < 8) return names[id];
    return "Unknown";
}

} // namespace ecumult
