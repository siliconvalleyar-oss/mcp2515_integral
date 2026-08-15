#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace ecumult {

struct DTC {
    uint16_t code;
    std::string display_code; // e.g. "P0101"
    std::string description;
    enum Severity { MINOR, MODERATE, MAJOR, CRITICAL };
    Severity severity;
    bool is_pending;
    bool is_permanent;
    uint32_t occurrence_count;
    std::chrono::system_clock::time_point first_occurrence;
    std::chrono::system_clock::time_point last_occurrence;

    std::string severityString() const;
    static Severity severityFromString(const std::string& s);
};

class DTCManager {
public:
    DTCManager();

    void addDTC(uint16_t code, const std::string& desc,
                DTC::Severity sev = DTC::Severity::MINOR,
                bool pending = false);
    void removeDTC(uint16_t code);
    void clearAll();
    void clearActive();
    void clearPending();
    void setPending(uint16_t code, bool pending);

    bool hasDTC(uint16_t code) const;
    bool hasActiveDTCs() const;
    bool hasPendingDTCs() const;

    std::vector<DTC> getActiveDTCs() const;
    std::vector<DTC> getPendingDTCs() const;
    std::vector<DTC> getAllDTCs() const;
    std::vector<DTC> getDTCsBySeverity(DTC::Severity sev) const;

    DTC* getDTC(uint16_t code);
    size_t count() const;

    static uint16_t encodeCode(const std::string& display);
    static std::string decodeCode(uint16_t code);

private:
    std::unordered_map<uint16_t, DTC> dtcs_;
};

} // namespace ecumult
