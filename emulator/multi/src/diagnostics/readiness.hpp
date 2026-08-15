#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace ecumult {

class ReadinessMonitor {
public:
    struct MonitorStatus {
        std::string name;
        bool supported;
        bool complete;
    };

    ReadinessMonitor();

    void setSupported(uint8_t monitor_id, bool supported);
    void setComplete(uint8_t monitor_id, bool complete);
    bool isSupported(uint8_t monitor_id) const;
    bool isComplete(uint8_t monitor_id) const;

    std::vector<MonitorStatus> getAllMonitors() const;
    std::vector<uint8_t> getOBDStatus() const;
    bool allComplete() const;

    void reset();

private:
    // OBD monitors: 0-7
    bool supported_[8];
    bool complete_[8];

    static std::string monitorName(uint8_t id);
};

} // namespace ecumult
