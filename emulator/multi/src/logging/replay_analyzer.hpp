#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <chrono>

namespace ecumult {

struct CANLogEntry {
    std::chrono::system_clock::time_point timestamp;
    uint32_t can_id;
    std::vector<uint8_t> data;
    bool is_tx;
};

class ReplayAnalyzer {
public:
    ReplayAnalyzer();

    bool loadFile(const std::string& path);
    bool startReplay(double speed_multiplier = 1.0);
    void stopReplay();
    bool isReplaying() const;

    std::vector<CANLogEntry> getNextFrames(size_t count = 1);
    std::vector<CANLogEntry> getAllFrames() const;

    bool startRecording(const std::string& path);
    void stopRecording();
    bool isRecording() const;

    void recordFrame(uint32_t can_id, const std::vector<uint8_t>& data, bool tx);

    // Analysis
    struct AnalysisReport {
        size_t total_frames;
        size_t rx_count;
        size_t tx_count;
        double duration_seconds;
        std::vector<uint32_t> unique_ids;
        std::vector<std::pair<uint32_t, size_t>> top_ids;
    };

    AnalysisReport analyze() const;

private:
    std::vector<CANLogEntry> frames_;
    size_t replay_position_;
    bool replaying_;
    bool recording_;
    std::ofstream recording_file_;
    std::string recording_path_;

    std::string formatCANEntry(const CANLogEntry& entry) const;
};

} // namespace ecumult
