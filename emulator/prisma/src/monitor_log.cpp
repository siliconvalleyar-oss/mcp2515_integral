#include "monitor_log.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>

// ---------------------------------------------------------------------------
//  Constructor / destructor
// ---------------------------------------------------------------------------
MonitorLog::MonitorLog() {}

MonitorLog::~MonitorLog() {
    close();
}

// ---------------------------------------------------------------------------
//  Crear carpeta logs/ si no existe
// ---------------------------------------------------------------------------
void MonitorLog::ensureLogDir() {
    struct stat st;
    if (stat("logs", &st) != 0) {
        mkdir("logs", 0755);
    }
}

// ---------------------------------------------------------------------------
//  Abrir archivo de log
// ---------------------------------------------------------------------------
bool MonitorLog::open() {
    std::lock_guard<std::mutex> lk(mtx_);
    if (file_) return true;   // ya abierto

    ensureLogDir();

    // Obtener timestamp actual
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    struct tm tm_buf;
    localtime_r(&time, &tm_buf);

    char fname[128];
    std::snprintf(fname, sizeof(fname), "logs/log_monitor_%04d%02d%02d_%02d%02d.log",
                  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                  tm_buf.tm_hour, tm_buf.tm_min);

    file_ = std::fopen(fname, "a");
    if (!file_) return false;

    filename_ = fname;

    // Escribir cabecera
    std::fprintf(file_, "===========================================================\n");
    std::fprintf(file_, " ECU Emulator — Monitor Log\n");
    std::fprintf(file_, " Inicio: %04d-%02d-%02d %02d:%02d:%02d\n",
                 tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                 tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    std::fprintf(file_, " Archivo: %s\n", fname);
    std::fprintf(file_, "===========================================================\n");
    std::fprintf(file_, "\n");
    std::fflush(file_);

    return true;
}

// ---------------------------------------------------------------------------
//  Cerrar archivo
// ---------------------------------------------------------------------------
void MonitorLog::close() {
    std::lock_guard<std::mutex> lk(mtx_);
    if (file_) {
        // Escribir footer
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        struct tm tm_buf;
        localtime_r(&time, &tm_buf);

        std::fprintf(file_, "\n");
        std::fprintf(file_, "-----------------------------------------------------------\n");
        std::fprintf(file_, " Fin: %04d-%02d-%02d %02d:%02d:%02d\n",
                     tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                     tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
        std::fprintf(file_, "===========================================================\n");
        std::fflush(file_);
        std::fclose(file_);
        file_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
//  Timestamp "YYYY-MM-DD HH:MM:SS.mmm"
// ---------------------------------------------------------------------------
std::string MonitorLog::timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    struct tm tm_buf;
    localtime_r(&time, &tm_buf);

    char buf[48];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                  tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
                  static_cast<int>(ms.count()));
    return std::string(buf);
}

// ---------------------------------------------------------------------------
//  Escribir línea al archivo (con flush periódico)
// ---------------------------------------------------------------------------
void MonitorLog::writeLine(const std::string& line) {
    if (!file_) return;
    std::fprintf(file_, "%s\n", line.c_str());
    // Flush cada 10 líneas para no saturar disco en 100 Hz
    static int flushCounter = 0;
    if (++flushCounter >= 10) {
        std::fflush(file_);
        flushCounter = 0;
    }
}

// ---------------------------------------------------------------------------
//  Log de trama CAN cruda
// ---------------------------------------------------------------------------
void MonitorLog::logCanFrame(const char* direction, const CanFrame& f) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!file_) return;

    std::ostringstream os;
    os << "[" << timestamp() << "] " << direction << " "
       << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << f.id
       << std::dec << " " << static_cast<int>(f.dlc) << " ";

    for (int i = 0; i < static_cast<int>(f.dlc); ++i) {
        if (i > 0) os << ' ';
        char b[4];
        std::snprintf(b, sizeof(b), "%02X", f.data[i]);
        os << b;
    }

    writeLine(os.str());
}

// ---------------------------------------------------------------------------
//  Log de petición/respuesta OBD2
// ---------------------------------------------------------------------------
void MonitorLog::logObd2(const char* direction, uint16_t canId,
                         uint8_t mode, uint8_t pid,
                         const std::vector<uint8_t>& payload) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!file_) return;

    std::ostringstream os;
    os << "[" << timestamp() << "] OBD " << direction << " "
       << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << canId
       << " M=" << std::setw(2) << static_cast<int>(mode)
       << " P=" << std::setw(2) << static_cast<int>(pid) << " -> ";

    for (size_t i = 0; i < payload.size(); ++i) {
        if (i > 0) os << ' ';
        char b[4];
        std::snprintf(b, sizeof(b), "%02X", payload[i]);
        os << b;
    }

    writeLine(os.str());
}

// ---------------------------------------------------------------------------
//  Log de trama broadcast
// ---------------------------------------------------------------------------
void MonitorLog::logBroadcast(const CanFrame& f) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!file_) return;

    std::ostringstream os;
    os << "[" << timestamp() << "] BC  "
       << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << f.id
       << std::dec << " " << static_cast<int>(f.dlc) << " ";

    for (int i = 0; i < static_cast<int>(f.dlc); ++i) {
        if (i > 0) os << ' ';
        char b[4];
        std::snprintf(b, sizeof(b), "%02X", f.data[i]);
        os << b;
    }

    writeLine(os.str());
}

// ---------------------------------------------------------------------------
//  Log de mensaje genérico
// ---------------------------------------------------------------------------
void MonitorLog::logMessage(const char* tag, const std::string& msg) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!file_) return;

    std::ostringstream os;
    os << "[" << timestamp() << "] " << tag << " " << msg;
    writeLine(os.str());
}
