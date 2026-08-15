#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace ecumult {

class FreezeFrameManager {
public:
    struct FreezeFrame {
        uint16_t dtc_code;
        std::unordered_map<uint8_t, float> pid_values; // PID -> value
        std::chrono::system_clock::time_point timestamp;
    };

    FreezeFrameManager();

    void captureFrame(uint16_t dtc_code, const std::unordered_map<uint8_t, float>& values);
    void clearForDTC(uint16_t dtc_code);
    void clearAll();
    std::vector<FreezeFrame> getAll() const;
    std::vector<FreezeFrame> getForDTC(uint16_t dtc_code) const;
    size_t count() const;

private:
    std::vector<FreezeFrame> frames_;
    static constexpr size_t MAX_FRAMES = 50;
};

} // namespace ecumult
