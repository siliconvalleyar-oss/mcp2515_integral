#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "mcp2515.h"
#include "vehicle.h"

// ---------------------------------------------------------------------------
//  Emulador de ECU OBD2 estilo ELM327
//
//  - Interpreta comandos AT estándar (ATZ, ATE0, ATSP6, ATRV, ...).
//  - Responde a peticiones OBD2 (modos 01-0A) recibidas por CAN desde un
//    escáner en 0x7DF (funcional) o 0x7E0 (físico), respondiendo desde
//    0x7E8 / 0x7E9 respectivamente (ISO 15765-4, 11 bits).
//  - Soporta peticiones multi-PID (2-6 PIDs), respuestas multi-frame
//    (VIN/CALID/ECU name) aceptando el Flow Control venga de donde venga,
//    DTCs con freeze frame, monitores OBD (modo 06) y comandos AT ampliados
//    para compatibilidad con escáneres de diagnóstico profesionales.
//  - El menú puede usarla como "consola ELM327" para pruebas locales.
// ---------------------------------------------------------------------------
class ELM327 {
public:
    ELM327(MCP2515* can, Vehicle* veh, Console* console);

    void setDefaults();

    // Procesa una línea (AT u OBD2) escrita en la consola. Devuelve el texto
    // que debe mostrarse (incluye terminador CR/LF según configuración).
    std::string process(const std::string& line);

    // Petición recibida por CAN (llamado por el hilo CAN). Si el PID está
    // soportado, genera la respuesta y la envía por el bus.
    void handleCanRequest(const CanFrame& frame);

    // Procesa las tramas interceptadas (no-FC) durante la espera de Flow
    // Control en respuestas multi-frame. Lo llama el hilo CAN tras drenar
    // los buffers RX, para no perder peticiones legítimas del escáner.
    void drainPending();

    // Trama de respuesta (0x7E8/0x7E9) observada en el bus (monitor).
    void handleCanResponse(const CanFrame& frame);

private:
    std::string handleAt(const std::string& cmd);
    bool getObdResponse(uint8_t mode, uint8_t pid, uint8_t* out, int& len);
    bool getMode01(uint8_t mode, uint8_t pid, uint8_t* out, int& len);
    bool getMode06(uint8_t tid, uint8_t* out, int& len);
    bool getMode09(uint8_t pid, uint8_t* out, int& len);
    bool getMode22(uint16_t did, uint8_t* out, int& len);
    bool sendIsoTp(uint16_t id, const std::vector<uint8_t>& payload,
                   int fcTimeoutMs = 300);
    std::string formatPayload(const std::vector<uint8_t>& payload) const;
    std::string terminator() const;

    static std::string toUpper(std::string s);
    static bool isHex(const std::string& s);
    static std::vector<uint8_t> hexToBytes(const std::string& s);

    MCP2515* can;
    Vehicle* veh;
    Console* console;

    // Configuración estilo ELM327
    bool echo = true;         // ATE0/ATE1 (un ELM327 real trae el eco encendido)
    bool linefeeds = false;   // ATL0/ATL1
    bool headers = false;     // ATH0/ATH1
    bool spaces = true;       // ATS0/ATS1
    bool responsesOn = true;  // ATR0/ATR1
    int  protocol = 6;        // ISO 15765-4 (CAN 11/500)
    bool protocolAuto = true;
    uint16_t txId = 0x7DF;    // ID de petición (ATSH)
    uint16_t rxId = 0x7E8;    // ID de respuesta (ATCRA)

    // DTCs activos / pendientes (2 bytes por código: 0x0301 = P0301).
    // Protegidos por veh->mtx (getMode01/03/07/0A y modo 04 lo toman).
    std::vector<uint16_t> dtcs;
    std::vector<uint16_t> dtcsPending;
    bool mil = false;                    // MIL encendida si hay DTCs
    uint8_t warmupsSinceClear = 3;       // calentamientos desde borrado
    uint16_t distanceSinceClearKm = 25;  // distancia desde borrado

    // Tramas no-FC interceptadas durante la espera de Flow Control.
    std::vector<CanFrame> pending;
};
