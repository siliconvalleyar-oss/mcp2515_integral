#include "replay_analyzer.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>

namespace ecumult {

ReplayAnalyzer::ReplayAnalyzer()
    : replay_position_(0)
    , replaying_(false)
    , recording_(false) {}

bool ReplayAnalyzer::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    frames_.clear();
    std::string line;
    while (std::getline(file, line)) {
        CANLogEntry entry;
        // Parse: TIMESTAMP ID DIR DATA (simplified)
        // Format: 2024-01-01 00:00:00.000 0x7E8 RX 01 02 03
        std::stringstream ss(line);
        std::string ts_str, id_str, dir_str, data_str;
        ss >> ts_str >> id_str >> dir_str;

        entry.is_tx = (dir_str == "TX");
        entry.can_id = std::stoul(id_str, nullptr, 16);

        std::vector<uint8_t> data;
        uint32_t byte;
        while (ss >> std::hex >> byte) {
            data.push_back(static_cast<uint8_t>(byte & 0xFF));
        }
        entry.data = data;
        frames_.push_back(entry);
    }
    file.close();
    return true;
}

bool ReplayAnalyzer::startReplay(double speed_multiplier) {
    (void)speed_multiplier;
    replay_position_ = 0;
    replaying_ = true;
    return true;
}

void ReplayAnalyzer::stopReplay() {
    replaying_ = false;
}

bool ReplayAnalyzer::isReplaying() const {
    return replaying_;
}

std::vector<CANLogEntry> ReplayAnalyzer::getNextFrames(size_t count) {
    std::vector<CANLogEntry> result;
    if (!replaying_) return result;

    size_t end = std::min(replay_position_ + count, frames_.size());
    for (size_t i = replay_position_; i < end; i++) {
        result.push_back(frames_[i]);
    }
    replay_position_ = end;

    if (replay_position_ >= frames_.size()) {
        replaying_ = false;
    }
    return result;
}

std::vector<CANLogEntry> ReplayAnalyzer::getAllFrames() const {
    return frames_;
}

bool ReplayAnalyzer::startRecording(const std::string& path) {
    recording_path_ = path;
    recording_file_.open(path);
    if (!recording_file_.is_open()) return false;
    recording_ = true;
    return true;
}

void ReplayAnalyzer::stopRecording() {
    recording_ = false;
    if (recording_file_.is_open()) recording_file_.close();
}

bool ReplayAnalyzer::isRecording() const {
    return recording_;
}

void ReplayAnalyzer::recordFrame(uint32_t can_id, const std::vector<uint8_t>& data, bool tx) {
    CANLogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.can_id = can_id;
    entry.data = data;
    entry.is_tx = tx;

    if (recording_ && recording_file_.is_open()) {
        recording_file_ << formatCANEntry(entry) << std::endl;
    }
    frames_.push_back(entry);
}

ReplayAnalyzer::AnalysisReport ReplayAnalyzer::analyze() const {
    AnalysisReport report;
    report.total_frames = frames_.size();
    report.rx_count = 0;
    report.tx_count = 0;

    std::map<uint32_t, size_t> id_count;
    for (const auto& f : frames_) {
        if (f.is_tx) report.tx_count++;
        else report.rx_count++;
        id_count[f.can_id]++;
    }

    if (!frames_.empty()) {
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(
            frames_.back().timestamp - frames_.front().timestamp);
        report.duration_seconds = duration.count();
    }

    for (const auto& [id, count] : id_count) {
        report.unique_ids.push_back(id);
        report.top_ids.emplace_back(id, count);
    }

    std::sort(report.top_ids.begin(), report.top_ids.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    if (report.top_ids.size() > 10) {
        report.top_ids.resize(10);
    }

    return report;
}

std::string ReplayAnalyzer::formatCANEntry(const CANLogEntry& entry) const {
    std::stringstream ss;
    auto tt = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp.time_since_epoch()) % 1000;
    ss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setw(3) << std::setfill('0') << ms.count();
    ss << " 0x" << std::hex << entry.can_id;
    ss << (entry.is_tx ? " TX" : " RX");
    for (auto b : entry.data) {
        ss << " " << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    }
    return ss.str();
}

} // namespace ecumult
