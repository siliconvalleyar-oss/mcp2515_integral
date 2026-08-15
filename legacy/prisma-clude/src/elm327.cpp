#include "elm327.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstdlib>

ELM327::ELM327(Vehicle *vehicle, MCP2515 *can)
    : _vehicle(vehicle), _can(can), _echo(true), _headersOn(false),
      _linefeed(true), _spacesOn(true), _protocol(6) {}

void ELM327::setEcho(bool value)      { _echo = value; }
void ELM327::setHeadersOn(bool value) { _headersOn = value; }
void ELM327::setLinefeed(bool value)  { _linefeed = value; }

std::string ELM327::toUpper(const std::string &s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::toupper);
    return r;
}

std::string ELM327::trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string ELM327::processCommand(const std::string &raw) {
    std::string cmd = trim(raw);
    if (cmd.empty()) return "?";

    std::string upper = toUpper(cmd);
    // Los escáneres suelen enviar los PID sin espacios ("010C"), pero se
    // toleran variantes con espacios ("01 0C") eliminándolos aquí.
    std::string compact;
    for (char c : upper) if (!isspace((unsigned char)c)) compact += c;

    if (compact.rfind("AT", 0) == 0) {
        return handleAT(compact);
    }
    return handleOBD(compact);
}

std::string ELM327::handleAT(const std::string &cmd) {
    if (cmd == "ATZ")  { _headersOn = false; _echo = true; return "ELM327 v1.5"; }
    if (cmd == "ATE0") { _echo = false; return "OK"; }
    if (cmd == "ATE1") { _echo = true; return "OK"; }
    if (cmd == "ATL0") { _linefeed = false; return "OK"; }
    if (cmd == "ATL1") { _linefeed = true; return "OK"; }
    if (cmd == "ATH0") { _headersOn = false; return "OK"; }
    if (cmd == "ATH1") { _headersOn = true; return "OK"; }
    if (cmd == "ATS0") { _spacesOn = false; return "OK"; }
    if (cmd == "ATS1") { _spacesOn = true; return "OK"; }
    if (cmd == "ATI")  { return "ELM327 v1.5"; }
    if (cmd == "AT@1") { return "Emulador OBD2 Chevrolet Prisma"; }
    if (cmd == "ATWS") { return "ELM327 v1.5"; }
    if (cmd == "ATD")  { return "OK"; }
    if (cmd == "ATDP") { return "AUTO, ISO 15765-4 (CAN 11/500)"; }
    if (cmd == "ATDPN") { return "6"; }
    if (cmd == "ATRV") {
        VehicleParameters snap = _vehicle->getSnapshot();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << snap.batteryVoltage << "V";
        return oss.str();
    }
    if (cmd.rfind("ATSP", 0) == 0) {
        if (cmd.size() > 4 && isdigit((unsigned char)cmd[4])) _protocol = (uint8_t)(cmd[4] - '0');
        return "OK";
    }
    if (cmd.rfind("ATSH", 0) == 0)  return "OK"; // fijar cabecera - aceptado, sin efecto real
    if (cmd.rfind("ATCRA", 0) == 0) return "OK"; // set CAN receive address
    if (cmd.rfind("ATFC", 0) == 0)  return "OK"; // flow control
    if (cmd.rfind("ATAT", 0) == 0)  return "OK"; // adaptive timing

    return "OK";
}

static std::string byteToHex(uint8_t b) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return oss.str();
}

std::vector<uint8_t> ELM327::buildPidResponse(uint8_t mode, uint8_t pid, bool &known) {
    std::vector<uint8_t> out;
    known = true;
    VehicleParameters v = _vehicle->getSnapshot();

    if (mode == 0x01) {
        switch (pid) {
            case 0x00: // Bitmask de PIDs soportados 01-20 (incluye los implementados)
                out = { 0x98, 0x3B, 0x80, 0x11 };
                break;
            case 0x04: // Carga calculada del motor
                out = { (uint8_t)(v.engineLoadPct * 255.0 / 100.0) };
                break;
            case 0x05: // Temperatura del refrigerante
                out = { (uint8_t)(v.engineTempC + 40) };
                break;
            case 0x0A: // Presión de combustible
                out = { (uint8_t)(v.fuelPressureKpa / 3.0) };
                break;
            case 0x0C: { // RPM
                uint16_t raw = (uint16_t)(v.rpm * 4);
                out = { (uint8_t)(raw >> 8), (uint8_t)(raw & 0xFF) };
                break;
            }
            case 0x0D: // Velocidad del vehículo
                out = { (uint8_t)v.speedKmh };
                break;
            case 0x0E: // Avance de encendido
                out = { (uint8_t)((v.timingAdvanceDeg + 64) * 2) };
                break;
            case 0x0F: // Temperatura de aire de admisión
                out = { (uint8_t)(v.intakeAirTempC + 40) };
                break;
            case 0x10: { // Caudal másico de aire (MAF)
                uint16_t raw = (uint16_t)(v.mafRateGs * 100);
                out = { (uint8_t)(raw >> 8), (uint8_t)(raw & 0xFF) };
                break;
            }
            case 0x11: // Posición del acelerador
                out = { (uint8_t)(v.throttlePct * 255.0 / 100.0) };
                break;
            case 0x1F: { // Tiempo transcurrido desde el arranque del motor
                uint16_t raw = (uint16_t)v.runTimeSec;
                out = { (uint8_t)(raw >> 8), (uint8_t)(raw & 0xFF) };
                break;
            }
            case 0x2F: // Nivel de combustible
                out = { (uint8_t)(v.fuelLevelPct * 255.0 / 100.0) };
                break;
            case 0x42: { // Voltaje del módulo de control (batería)
                uint16_t raw = (uint16_t)(v.batteryVoltage * 1000);
                out = { (uint8_t)(raw >> 8), (uint8_t)(raw & 0xFF) };
                break;
            }
            case 0x46: // Temperatura ambiente
                out = { (uint8_t)(v.ambientTempC + 40) };
                break;
            case 0xA6: { // Distancia acumulada por el vehículo (odómetro, 0.1 km)
                uint32_t raw = (uint32_t)(v.odometerKm * 10);
                out = { (uint8_t)(raw >> 24), (uint8_t)(raw >> 16),
                        (uint8_t)(raw >> 8), (uint8_t)(raw & 0xFF) };
                break;
            }
            default:
                known = false;
                break;
        }
    } else if (mode == 0x03) {
        // Códigos de diagnóstico almacenados (DTC) - sin fallas por defecto
        out = { 0x00 };
    } else if (mode == 0x09) {
        switch (pid) {
            case 0x02: { // VIN simulado con formato Chevrolet Prisma (GM Argentina/Brasil)
                std::string vin = "9BGKS48V0FG123456";
                out.push_back(0x01); // 1 mensaje
                for (char c : vin) out.push_back((uint8_t)c);
                break;
            }
            default:
                known = false;
                break;
        }
    } else {
        known = false;
    }

    return out;
}

std::string ELM327::formatResponse(const std::vector<uint8_t> &bytes, uint8_t mode, uint8_t pid) {
    std::ostringstream oss;
    if (_headersOn) {
        oss << "7E8" << (_spacesOn ? " " : "")
            << byteToHex((uint8_t)(bytes.size() + 2)) << (_spacesOn ? " " : "");
    }
    oss << byteToHex((uint8_t)(mode + 0x40));
    if (_spacesOn) oss << " ";
    oss << byteToHex(pid);
    for (uint8_t b : bytes) {
        if (_spacesOn) oss << " ";
        oss << byteToHex(b);
    }
    return oss.str();
}

std::string ELM327::handleOBD(const std::string &cmd) {
    if (cmd.size() < 2) return "NO DATA";

    uint8_t mode = 0, pid = 0;
    try {
        mode = (uint8_t)std::stoi(cmd.substr(0, 2), nullptr, 16);
        if (cmd.size() >= 4) {
            pid = (uint8_t)std::stoi(cmd.substr(2, 2), nullptr, 16);
        }
    } catch (...) {
        return "?";
    }

    bool known = false;
    std::vector<uint8_t> data = buildPidResponse(mode, pid, known);
    if (!known) return "NO DATA";

    return formatResponse(data, mode, pid);
}

void ELM327::pollCanRequests() {
    if (!_can->hasMessage()) return;

    CanFrame req;
    if (!_can->readFrame(req)) return;

    // Solicitud OBD2 funcional (0x7DF) o física (0x7E0) dirigida a la ECU emulada
    if ((req.id == 0x7DF || req.id == 0x7E0) && req.dlc >= 3 && req.data[1] != 0) {
        uint8_t mode = req.data[1];
        uint8_t pid  = req.data[2];

        bool known = false;
        std::vector<uint8_t> data = buildPidResponse(mode, pid, known);
        if (!known) return;

        CanFrame resp;
        resp.id = 0x7E8;
        resp.extended = false;
        resp.dlc = 8;
        resp.data[0] = (uint8_t)(2 + data.size()); // longitud ISO-TP (single frame)
        resp.data[1] = (uint8_t)(mode + 0x40);
        resp.data[2] = pid;
        for (size_t i = 0; i < data.size() && i < 5; i++) {
            resp.data[3 + i] = data[i];
        }

        _can->sendFrame(resp);
    }
}
