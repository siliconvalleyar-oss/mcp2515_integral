#include "can_logger.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace ecumult {

CANLogger::CANLogger(const std::string& log_path, LogLevel level,
                     size_t max_size_mb, int max_files)
    : log_path_(log_path)
    , level_(level)
    , max_size_(max_size_mb * 1024 * 1024)
    , max_files_(max_files) {
    openFile();
}

CANLogger::~CANLogger() {
    if (file_.is_open()) file_.close();
}

void CANLogger::log(LogLevel level, const std::string& message) {
    if (level < level_) return;
    std::string formatted = "[" + formatTimestamp() + "] [" + levelToString(level) + "] " + message;
    writeLog(formatted);
}

void CANLogger::trace(const std::string& msg) { log(TRACE, msg); }
void CANLogger::debug(const std::string& msg) { log(DEBUG, msg); }
void CANLogger::info(const std::string& msg) { log(INFO, msg); }
void CANLogger::warning(const std::string& msg) { log(WARNING, msg); }
void CANLogger::error(const std::string& msg) { log(ERROR, msg); }
void CANLogger::critical(const std::string& msg) { log(CRITICAL, msg); }

void CANLogger::logCANFrame(bool tx, uint32_t can_id, const std::vector<uint8_t>& data) {
    std::stringstream ss;
    ss << (tx ? "TX" : "RX") << " ID:0x" << std::hex << can_id << " [";
    for (size_t i = 0; i < data.size(); i++) {
        if (i > 0) ss << " ";
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    ss << "]";
    log(TRACE, ss.str());
}

void CANLogger::setLevel(LogLevel level) {
    level_ = level;
}

CANLogger::LogLevel CANLogger::getLevel() const {
    return level_;
}

std::vector<std::string> CANLogger::getRecentLogs(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (recent_logs_.size() <= count) return recent_logs_;
    return std::vector<std::string>(recent_logs_.end() - count, recent_logs_.end());
}

std::string CANLogger::levelToString(LogLevel level) {
    switch (level) {
        case TRACE: return "TRACE";
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        case CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

CANLogger::LogLevel CANLogger::levelFromString(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "trace") return TRACE;
    if (lower == "debug") return DEBUG;
    if (lower == "info") return INFO;
    if (lower == "warning" || lower == "warn") return WARNING;
    if (lower == "error") return ERROR;
    if (lower == "critical" || lower == "fatal") return CRITICAL;
    return INFO;
}

void CANLogger::rotateIfNeeded() {
    if (!file_.is_open()) return;
    auto pos = file_.tellp();
    if (pos < static_cast<std::streamoff>(max_size_)) return;
    file_.close();

    for (int i = max_files_ - 1; i > 0; i--) {
        std::string old_name = log_path_ + "." + std::to_string(i);
        std::string new_name = log_path_ + "." + std::to_string(i + 1);
        std::rename(old_name.c_str(), new_name.c_str());
    }
    std::rename(log_path_.c_str(), (log_path_ + ".1").c_str());
    openFile();
}

void CANLogger::writeLog(const std::string& formatted) {
    std::lock_guard<std::mutex> lock(mutex_);
    rotateIfNeeded();
    if (file_.is_open()) {
        file_ << formatted << std::endl;
    }
    std::cout << formatted << std::endl;

    recent_logs_.push_back(formatted);
    if (recent_logs_.size() > 1000) {
        recent_logs_.erase(recent_logs_.begin());
    }
}

std::string CANLogger::formatTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setw(3) << std::setfill('0') << ms.count();
    return ss.str();
}

void CANLogger::openFile() {
    file_.open(log_path_, std::ios::app);
    if (!file_.is_open()) {
        std::cerr << "[Logger] Could not open log file: " << log_path_ << std::endl;
    }
}

} // namespace ecumult
