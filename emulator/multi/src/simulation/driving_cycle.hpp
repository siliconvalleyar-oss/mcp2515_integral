#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ecumult {

struct DrivingCyclePoint {
    float rpm;
    float speed;
    float throttle;
    float load;
    float duration_s;
};

class DrivingCycle {
public:
    enum CycleType {
        URBAN,
        HIGHWAY,
        MIXED,
        STEADY_STATE,
        CUSTOM
    };

    DrivingCycle(CycleType type = URBAN);

    void setType(CycleType type);
    CycleType getType() const;
    std::string getTypeName() const;

    DrivingCyclePoint getPoint(float time_seconds) const;
    std::vector<DrivingCyclePoint> getFullCycle() const;
    float getTotalDuration() const;

    void setCustomProfile(const std::vector<DrivingCyclePoint>& points);

    // Generate points between 0-1 normalized time
    static std::vector<DrivingCyclePoint> generateUrban();
    static std::vector<DrivingCyclePoint> generateHighway();
    static std::vector<DrivingCyclePoint> generateMixed();

private:
    CycleType type_;
    std::vector<DrivingCyclePoint> custom_points_;
    std::vector<DrivingCyclePoint> active_points_;
};

} // namespace ecumult
