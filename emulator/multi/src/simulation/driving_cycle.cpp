#include "driving_cycle.hpp"
#include <cmath>

namespace ecumult {

DrivingCycle::DrivingCycle(CycleType type) : type_(type) {
    switch (type) {
        case URBAN: active_points_ = generateUrban(); break;
        case HIGHWAY: active_points_ = generateHighway(); break;
        case MIXED: active_points_ = generateMixed(); break;
        default: active_points_ = generateUrban(); break;
    }
}

void DrivingCycle::setType(CycleType type) {
    type_ = type;
    switch (type) {
        case URBAN: active_points_ = generateUrban(); break;
        case HIGHWAY: active_points_ = generateHighway(); break;
        case MIXED: active_points_ = generateMixed(); break;
        case STEADY_STATE:
            active_points_ = {{1500, 80, 25, 0.3f, 60.0f}};
            break;
        case CUSTOM:
            if (custom_points_.empty()) {
                active_points_ = generateUrban();
            } else {
                active_points_ = custom_points_;
            }
            break;
    }
}

DrivingCycle::CycleType DrivingCycle::getType() const {
    return type_;
}

std::string DrivingCycle::getTypeName() const {
    switch (type_) {
        case URBAN: return "urban";
        case HIGHWAY: return "highway";
        case MIXED: return "mixed";
        case STEADY_STATE: return "steady";
        case CUSTOM: return "custom";
        default: return "unknown";
    }
}

DrivingCyclePoint DrivingCycle::getPoint(float time_seconds) const {
    if (active_points_.empty()) return {0, 0, 0, 0, 0};

    float accumulated = 0;
    for (const auto& pt : active_points_) {
        accumulated += pt.duration_s;
        if (time_seconds <= accumulated) {
            // Linearly interpolate within this segment
            float prev_accumulated = accumulated - pt.duration_s;
            float t = (time_seconds - prev_accumulated) / pt.duration_s;
            return {pt.rpm, pt.speed, pt.throttle, pt.load, pt.duration_s};
        }
    }

    // After end of cycle, return last point
    return active_points_.back();
}

std::vector<DrivingCyclePoint> DrivingCycle::getFullCycle() const {
    return active_points_;
}

float DrivingCycle::getTotalDuration() const {
    float total = 0;
    for (const auto& pt : active_points_) {
        total += pt.duration_s;
    }
    return total;
}

void DrivingCycle::setCustomProfile(const std::vector<DrivingCyclePoint>& points) {
    custom_points_ = points;
    if (type_ == CUSTOM) {
        active_points_ = custom_points_;
    }
}

std::vector<DrivingCyclePoint> DrivingCycle::generateUrban() {
    return {
        {800, 0, 0, 0.1f, 10.0f},    // Idle at light
        {1200, 20, 15, 0.2f, 8.0f},   // Accelerate
        {1500, 35, 20, 0.3f, 10.0f},  // Cruise
        {2000, 50, 25, 0.35f, 12.0f}, // Medium speed
        {1200, 20, 10, 0.2f, 6.0f},   // Decelerate
        {800, 0, 0, 0.1f, 8.0f},      // Idle
        {1500, 40, 22, 0.3f, 10.0f},  // Accelerate
        {1800, 55, 28, 0.4f, 15.0f},  // Cruise
        {2500, 60, 35, 0.5f, 8.0f},   // Higher speed
        {800, 0, 0, 0.1f, 5.0f},      // Stop
        {1000, 15, 10, 0.15f, 6.0f},  // Slow
        {1500, 30, 20, 0.25f, 10.0f}, // Medium
        {800, 0, 0, 0.1f, 8.0f}       // Idle
    };
}

std::vector<DrivingCyclePoint> DrivingCycle::generateHighway() {
    return {
        {800, 0, 0, 0.1f, 5.0f},      // Start
        {2000, 40, 30, 0.3f, 10.0f},  // On-ramp
        {2500, 80, 35, 0.4f, 15.0f},  // Merge
        {2800, 100, 38, 0.45f, 30.0f}, // Cruise fast
        {3000, 120, 40, 0.5f, 40.0f},  // Highway speed
        {2500, 110, 35, 0.45f, 30.0f}, // Slight slow
        {2000, 90, 30, 0.4f, 20.0f},  // Moderate
        {1500, 60, 25, 0.3f, 10.0f},  // Decelerate
        {800, 0, 0, 0.1f, 5.0f}        // Off-ramp
    };
}

std::vector<DrivingCyclePoint> DrivingCycle::generateMixed() {
    return {
        {800, 0, 0, 0.1f, 5.0f},
        {1500, 30, 20, 0.25f, 8.0f},
        {2000, 50, 30, 0.35f, 10.0f},
        {2500, 80, 35, 0.4f, 20.0f},
        {2800, 110, 40, 0.5f, 25.0f},
        {1500, 60, 25, 0.3f, 12.0f},
        {1200, 35, 20, 0.25f, 8.0f},
        {800, 0, 0, 0.1f, 6.0f},
        {2000, 45, 28, 0.35f, 10.0f},
        {2500, 75, 35, 0.4f, 15.0f},
        {1500, 40, 22, 0.3f, 8.0f},
        {800, 0, 0, 0.1f, 5.0f}
    };
}

} // namespace ecumult
