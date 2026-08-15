#include "freeze_frame.hpp"
#include <algorithm>

namespace ecumult {

FreezeFrameManager::FreezeFrameManager() {}

void FreezeFrameManager::captureFrame(uint16_t dtc_code,
    const std::unordered_map<uint8_t, float>& values) {
    if (frames_.size() >= MAX_FRAMES) {
        frames_.erase(frames_.begin());
    }
    FreezeFrame ff;
    ff.dtc_code = dtc_code;
    ff.pid_values = values;
    ff.timestamp = std::chrono::system_clock::now();
    frames_.push_back(ff);
}

void FreezeFrameManager::clearForDTC(uint16_t dtc_code) {
    frames_.erase(
        std::remove_if(frames_.begin(), frames_.end(),
            [dtc_code](const FreezeFrame& ff) { return ff.dtc_code == dtc_code; }),
        frames_.end());
}

void FreezeFrameManager::clearAll() {
    frames_.clear();
}

std::vector<FreezeFrameManager::FreezeFrame> FreezeFrameManager::getAll() const {
    return frames_;
}

std::vector<FreezeFrameManager::FreezeFrame> FreezeFrameManager::getForDTC(uint16_t dtc_code) const {
    std::vector<FreezeFrame> result;
    for (const auto& ff : frames_) {
        if (ff.dtc_code == dtc_code) result.push_back(ff);
    }
    return result;
}

size_t FreezeFrameManager::count() const {
    return frames_.size();
}

} // namespace ecumult
