#include "scanner/dtc.hpp"
#include <fstream>
#include <sstream>
#include <cctype>
#include <unordered_map>

namespace Scanner {

DTCManager::DTCManager() = default;

DTCManager::~DTCManager() = default;

bool DTCManager::loadDatabase(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        DTC dtc;
        std::stringstream ss(line);
        std::getline(ss, dtc.code, '|');
        std::getline(ss, dtc.description, '|');
        std::getline(ss, dtc.severity, '|');

        size_t index = dtcs_.size();
        dtcs_.push_back(dtc);
        dtcIndex_[dtc.code] = index;
    }

    return !dtcs_.empty();
}

const DTC* DTCManager::lookup(const std::string& code) const {
    auto it = dtcIndex_.find(code);
    if (it != dtcIndex_.end()) {
        return &dtcs_[it->second];
    }
    return nullptr;
}

void DTCManager::clear() {
    dtcs_.clear();
    dtcIndex_.clear();
}

std::string DTCManager::getStatusString(uint8_t statusByte) {
    std::string status;
    if (statusByte & 0x01) status += "Test Failed ";
    if (statusByte & 0x02) status += "Test Failed Since Last Clear ";
    if (statusByte & 0x04) status += "Pending ";
    if (statusByte & 0x08) status += "Confirmed ";
    if (statusByte & 0x10) status += "Test Not Completed Since Last Clear ";
    if (statusByte & 0x20) status += "Test Failed Since Last Clear ";
    if (statusByte & 0x40) status += "Warning Indicator ";
    if (statusByte & 0x80) status += "Test Not Completed ";
    return status.empty() ? "Unknown" : status;
}

std::string DTCManager::getSeverity(const std::string& code) {
    if (code.empty()) return "Unknown";

    char first = std::toupper(code[0]);
    switch (first) {
        case 'P': return "Major";
        case 'C': return "Critical";
        case 'B': return "Major";
        case 'U': return "Minor";
        default: return "Unknown";
    }
}

} // namespace Scanner
