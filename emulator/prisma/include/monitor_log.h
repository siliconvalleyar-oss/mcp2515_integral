#pragma once

#include <cstdint>
#include <ctime>
#include <mutex>
#include <string>
#include <vector>

#include "mcp2515.h"

// ---------------------------------------------------------------------------
//  MonitorLog: registra tramas CAN enviadas/recebidas en archivos de log.
//
//  Archivo: logs/log_monitor_YYYYMMDD_HHMM.log
//  Formato de línea:
//    [YYYY-MM-DD HH:MM:SS.mmm] TX|RX <ID_3hex> <DLC> <data hex separado por espacio>
//    [YYYY-MM-DD HH:MM:SS.mmm] OBD <dir> <mode> <pid> -> <respuesta hex>
//    [YYYY-MM-DD HH:MM:SS.mmm] BC  <ID_3hex> <data hex>
//
// -thread-safe: puede ser llamado desde el hilo CAN y desde la consola.
// ---------------------------------------------------------------------------
class MonitorLog {
public:
    MonitorLog();
    ~MonitorLog();

    // Abre un nuevo archivo de log en logs/log_monitor_YYYYMMDD_HHMM.log.
    // Si la carpeta logs/ no existe, la crea.
    // Devuelve true si se abrió correctamente.
    bool open();

    // Cierra el archivo de log actual.
    void close();

    // Registra una trama CAN cruda (TX o RX).
    void logCanFrame(const char* direction, const CanFrame& f);

    // Registra una petición/respuesta OBD2 procesada.
    // direction: "REQ" (petición recibida) o "RES" (respuesta enviada)
    void logObd2(const char* direction, uint16_t canId,
                 uint8_t mode, uint8_t pid,
                 const std::vector<uint8_t>& payload);

    // Registra una trama broadcast (0x320/0x328).
    void logBroadcast(const CanFrame& f);

    // Registra un mensaje genérico con timestamp.
    void logMessage(const char* tag, const std::string& msg);

    // True si el log está abierto y activo.
    bool isActive() const { return file_ != nullptr; }

private:
    std::string timestamp();      // "YYYY-MM-DD HH:MM:SS.mmm"
    void writeLine(const std::string& line);
    void ensureLogDir();          // crea logs/ si no existe

    FILE* file_ = nullptr;
    std::mutex mtx_;
    std::string filename_;
};
