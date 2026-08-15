#include "scanner/event_log.hpp"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Scanner {

namespace {
const char* kDefaultFile   = "/var/log/autel_scanner.log";
const char* kFallbackFile  = "./autel_scanner.log";
const char* kLevelNames[]  = {"TRACE", "DEBUG", "INFO ", "WARN ", "ERROR"};
}

EventLog& EventLog::instance() {
    static EventLog inst;
    return inst;
}

EventLog::EventLog()
    : level_(LogLevel::INFO), filename_(kDefaultFile) {
    configure();
}

EventLog::~EventLog() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stream_.is_open()) {
        stream_.flush();
        stream_.close();
    }
}

LogLevel EventLog::levelFromString(const std::string& name) {
    if (name == "TRACE") return LogLevel::TRACE;
    if (name == "DEBUG") return LogLevel::DEBUG;
    if (name == "WARN")  return LogLevel::WARN;
    if (name == "ERROR") return LogLevel::ERROR;
    if (name == "INFO")  return LogLevel::INFO;
    return LogLevel::INFO;
}

void EventLog::configure() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (const char* lvl = std::getenv("AUTEL_LOG_LEVEL")) {
        level_ = levelFromString(lvl);
    }
    if (const char* file = std::getenv("AUTEL_LOG_FILE")) {
        if (*file) {
            filename_ = file;
        }
    }

    if (!openStream()) {
        // Fallback: si la ruta configurada no es abrible, usar local.
        filename_ = kFallbackFile;
        openStream();
    }
}

bool EventLog::openStream() {
    if (stream_.is_open()) {
        stream_.close();
    }
    stream_.open(filename_, std::ios::app);
    return stream_.is_open();
}

void EventLog::log(LogLevel level, const std::string& message) {
    if (level < level_) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() % 1000);

    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << "." << std::setw(3) << std::setfill('0') << ms << "] "
       << kLevelNames[static_cast<int>(level)] << " " << message;

    std::lock_guard<std::mutex> lock(mutex_);
    if (stream_.is_open()) {
        stream_ << ss.str() << std::endl;
        stream_.flush();
    }
    printf("%s\n", ss.str().c_str());
}

} // namespace Scanner
