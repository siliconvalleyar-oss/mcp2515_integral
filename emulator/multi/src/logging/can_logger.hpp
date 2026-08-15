#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <vector>

namespace ecumult {

class CANLogger {
public:
    enum LogLevel {
        TRACE,
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    CANLogger(const std::string& log_path = "/var/log/ecu_emulator.log",
              LogLevel level = INFO,
              size_t max_size_mb = 100,
              int max_files = 5);
    ~CANLogger();

    void log(LogLevel level, const std::string& message);
    void trace(const std::string& msg);
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);
    void critical(const std::string& msg);

    void logCANFrame(bool tx, uint32_t can_id, const std::vector<uint8_t>& data);
    void setLevel(LogLevel level);
    LogLevel getLevel() const;

    std::vector<std::string> getRecentLogs(size_t count = 100) const;

    static std::string levelToString(LogLevel level);
    static LogLevel levelFromString(const std::string& s);

private:
    std::string log_path_;
    LogLevel level_;
    size_t max_size_;
    int max_files_;
    mutable std::mutex mutex_;
    std::ofstream file_;
    std::vector<std::string> recent_logs_;

    void rotateIfNeeded();
    void writeLog(const std::string& formatted);
    std::string formatTimestamp() const;
    void openFile();
};

} // namespace ecumult
