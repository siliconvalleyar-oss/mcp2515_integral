#include "dtc_manager.hpp"
#include <algorithm>
#include <sstream>

namespace ecumult {

std::string DTC::severityString() const {
    switch (severity) {
        case MINOR: return "minor";
        case MODERATE: return "moderate";
        case MAJOR: return "major";
        case CRITICAL: return "critical";
        default: return "unknown";
    }
}

DTC::Severity DTC::severityFromString(const std::string& s) {
    if (s == "minor") return MINOR;
    if (s == "moderate") return MODERATE;
    if (s == "major") return MAJOR;
    if (s == "critical") return CRITICAL;
    return MINOR;
}

DTCManager::DTCManager() {}

void DTCManager::addDTC(uint16_t code, const std::string& desc,
                         DTC::Severity sev, bool pending) {
    auto it = dtcs_.find(code);
    if (it != dtcs_.end()) {
        it->second.occurrence_count++;
        it->second.last_occurrence = std::chrono::system_clock::now();
        it->second.is_pending = pending;
        return;
    }

    DTC dtc;
    dtc.code = code;
    dtc.display_code = decodeCode(code);
    dtc.description = desc;
    dtc.severity = sev;
    dtc.is_pending = pending;
    dtc.is_permanent = false;
    dtc.occurrence_count = 1;
    dtc.first_occurrence = std::chrono::system_clock::now();
    dtc.last_occurrence = dtc.first_occurrence;
    dtcs_[code] = dtc;
}

void DTCManager::removeDTC(uint16_t code) {
    dtcs_.erase(code);
}

void DTCManager::clearAll() {
    dtcs_.clear();
}

void DTCManager::clearActive() {
    for (auto it = dtcs_.begin(); it != dtcs_.end();) {
        if (!it->second.is_pending) {
            it = dtcs_.erase(it);
        } else {
            ++it;
        }
    }
}

void DTCManager::clearPending() {
    for (auto it = dtcs_.begin(); it != dtcs_.end();) {
        if (it->second.is_pending) {
            it->second.is_pending = false;
        }
        ++it;
    }
}

void DTCManager::setPending(uint16_t code, bool pending) {
    auto it = dtcs_.find(code);
    if (it != dtcs_.end()) {
        it->second.is_pending = pending;
    }
}

bool DTCManager::hasDTC(uint16_t code) const {
    return dtcs_.find(code) != dtcs_.end();
}

bool DTCManager::hasActiveDTCs() const {
    for (const auto& [code, dtc] : dtcs_) {
        if (!dtc.is_pending) return true;
    }
    return false;
}

bool DTCManager::hasPendingDTCs() const {
    for (const auto& [code, dtc] : dtcs_) {
        if (dtc.is_pending) return true;
    }
    return false;
}

std::vector<DTC> DTCManager::getActiveDTCs() const {
    std::vector<DTC> result;
    for (const auto& [code, dtc] : dtcs_) {
        if (!dtc.is_pending) result.push_back(dtc);
    }
    return result;
}

std::vector<DTC> DTCManager::getPendingDTCs() const {
    std::vector<DTC> result;
    for (const auto& [code, dtc] : dtcs_) {
        if (dtc.is_pending) result.push_back(dtc);
    }
    return result;
}

std::vector<DTC> DTCManager::getAllDTCs() const {
    std::vector<DTC> result;
    for (const auto& [code, dtc] : dtcs_) {
        result.push_back(dtc);
    }
    return result;
}

std::vector<DTC> DTCManager::getDTCsBySeverity(DTC::Severity sev) const {
    std::vector<DTC> result;
    for (const auto& [code, dtc] : dtcs_) {
        if (dtc.severity == sev) result.push_back(dtc);
    }
    return result;
}

DTC* DTCManager::getDTC(uint16_t code) {
    auto it = dtcs_.find(code);
    if (it != dtcs_.end()) return &it->second;
    return nullptr;
}

size_t DTCManager::count() const {
    return dtcs_.size();
}

uint16_t DTCManager::encodeCode(const std::string& display) {
    if (display.size() < 5) return 0;
    uint16_t result = 0;
    switch (display[0]) {
        case 'P': result = 0x0000; break;
        case 'C': result = 0x4000; break;
        case 'B': result = 0x8000; break;
        case 'U': result = 0xC000; break;
        default: return 0;
    }
    result |= std::stoi(display.substr(1, 1)) << 12;
    result |= std::stoi(display.substr(2, 3)) & 0x0FFF;
    return result;
}

std::string DTCManager::decodeCode(uint16_t code) {
    std::string result;
    switch (code & 0xC000) {
        case 0x0000: result = "P"; break;
        case 0x4000: result = "C"; break;
        case 0x8000: result = "B"; break;
        case 0xC000: result = "U"; break;
        default: result = "?";
    }
    result += std::to_string((code >> 12) & 0x0F);
    result += std::to_string((code >> 8) & 0x0F);
    result += std::to_string((code >> 4) & 0x0F);
    result += std::to_string(code & 0x0F);
    return result;
}

} // namespace ecumult
