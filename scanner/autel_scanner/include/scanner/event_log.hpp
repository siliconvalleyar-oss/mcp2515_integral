#ifndef SCANNER_EVENT_LOG_HPP
#define SCANNER_EVENT_LOG_HPP

#include <string>
#include <mutex>
#include <fstream>

namespace Scanner {

enum class LogLevel : int {
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4
};

// Logger de eventos thread-safe (singleton) del scanner.
//
// Configuracion via variables de entorno:
//   AUTEL_LOG_LEVEL  -> TRACE|DEBUG|INFO|WARN|ERROR (default: INFO)
//   AUTEL_LOG_FILE   -> ruta del archivo de log (default:
//                       /var/log/autel_scanner.log; si no es escribible,
//                       cae a ./autel_scanner.log)
class EventLog {
public:
    static EventLog& instance();

    EventLog(const EventLog&) = delete;
    EventLog& operator=(const EventLog&) = delete;

    void configure();
    void log(LogLevel level, const std::string& message);

    void trace(const std::string& msg) { log(LogLevel::TRACE, msg); }
    void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
    void info(const std::string& msg)  { log(LogLevel::INFO, msg); }
    void warn(const std::string& msg)  { log(LogLevel::WARN, msg); }
    void error(const std::string& msg) { log(LogLevel::ERROR, msg); }

    LogLevel level() const { return level_; }
    const std::string& filename() const { return filename_; }

private:
    EventLog();
    ~EventLog();

    bool openStream();
    static LogLevel levelFromString(const std::string& name);

    std::mutex mutex_;
    LogLevel level_;
    std::string filename_;
    std::ofstream stream_;
};

} // namespace Scanner

#endif // SCANNER_EVENT_LOG_HPP
