#include "elm327.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>

// ---------------------------------------------------------------------------
//  Utilidades de texto
// ---------------------------------------------------------------------------
std::string ELM327::toUpper(std::string s) {
    for (auto& c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

bool ELM327::isHex(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s)
        if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    return true;
}

std::vector<uint8_t> ELM327::hexToBytes(const std::string& s) {
    std::vector<uint8_t> out;
    if (s.size() % 2 != 0) return out;
    for (size_t i = 0; i < s.size(); i += 2)
        out.push_back(static_cast<uint8_t>(
            std::strtoul(s.substr(i, 2).c_str(), nullptr, 16)));
    return out;
}

// ---------------------------------------------------------------------------
//  Constructor / configuración
// ---------------------------------------------------------------------------
ELM327::ELM327(MCP2515* can, Vehicle* veh, Console* console)
    : can(can), veh(veh), console(console) {
    // DTCs de ejemplo activos (para que un escáner muestre fallos y freeze
    // frame al conectarse). Modo 04 los borra durante la sesión.
    dtcs = { 0x0301, 0x0420 };   // P0301 (fallo encendido cil 1), P0420 (catalizador)
    dtcsPending = { 0x0133 };    // P0133 (sonda O2 lenta) pendiente
    mil = !dtcs.empty();
    warmupsSinceClear = 3;
    distanceSinceClearKm = 25;
    setDefaults();
}

void ELM327::setDefaults() {
    echo = true;           // un ELM327 real trae el eco encendido (ATE1)
    linefeeds = false;
    headers = false;
    spaces = true;
    responsesOn = true;
    protocol = 6;          // ISO 15765-4 (CAN 11-bit / 500 kbps)
    protocolAuto = true;
    txId = 0x7DF;
    rxId = 0x7E8;
}

// ---------------------------------------------------------------------------
//  Formato de salida
// ---------------------------------------------------------------------------
std::string ELM327::terminator() const {
    return linefeeds ? "\r\n" : "\r";
}

std::string ELM327::formatPayload(const std::vector<uint8_t>& payload) const {
    std::string s;
    if (headers) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%03X ", rxId);
        s += buf;
    }
    for (size_t i = 0; i < payload.size(); ++i) {
        if (i && spaces) s += ' ';
        char b[4];
        std::snprintf(b, sizeof(b), "%02X", payload[i]);
        s += b;
    }
    return s;
}

// ---------------------------------------------------------------------------
//  Comandos AT
// ---------------------------------------------------------------------------
std::string ELM327::handleAt(const std::string& cmd) {
    if (cmd == "ATZ" || cmd == "ATRST") { setDefaults(); return "ELM327 v1.5"; }
    if (cmd == "ATI")  return "ELM327 v1.5";
    if (cmd == "AT@1") return "OBDII to RS232 Interpreter";
    if (cmd == "AT@2") return "Prisma ECU Emulator v1.0";
    if (cmd == "AT@3") return "Prisma ECU Emulator";
    if (cmd == "ATD")  { setDefaults(); return "OK"; }

    if (cmd == "ATE0") { echo = false; return "OK"; }
    if (cmd == "ATE1") { echo = true;  return "OK"; }
    if (cmd == "ATL0") { linefeeds = false; return "OK"; }
    if (cmd == "ATL1") { linefeeds = true;  return "OK"; }
    if (cmd == "ATH0") { headers = false; return "OK"; }
    if (cmd == "ATH1") { headers = true;  return "OK"; }
    if (cmd == "ATS0") { spaces = false; return "OK"; }
    if (cmd == "ATS1") { spaces = true;  return "OK"; }
    if (cmd == "ATR0") { responsesOn = false; return "OK"; }
    if (cmd == "ATR1") { responsesOn = true;  return "OK"; }

    if (cmd == "ATRV") {
        std::lock_guard<std::mutex> lk(veh->mtx);
        char b[16];
        std::snprintf(b, sizeof(b), "%.1fV", veh->value("voltaje_bateria"));
        return b;
    }

    if (cmd == "ATDPN") return protocolAuto ? "A6" : "6";
    if (cmd == "ATDP")  return protocolAuto
                             ? "AUTO, ISO 15765-4 (CAN 11/500)"
                             : "ISO 15765-4 (CAN 11/500)";

    // ATSP0 (auto) / ATSPn (protocolo específico; físicamente solo funciona el 6)
    if (cmd.rfind("ATSP", 0) == 0) {
        protocolAuto = true;
        if (cmd.size() > 4) {
            const char c = cmd[4];
            if (c != '0' && c != 'A') {
                protocolAuto = false;
                protocol = (c >= '0' && c <= '9') ? (c - '0') : (c - 'A' + 10);
            }
        }
        return "OK";
    }

    // ATSHxxxx: fijar ID de petición. Acepta 3 dígitos (11 bits) o 6 (29 bits).
    if (cmd.rfind("ATSH", 0) == 0) {
        const std::string hex = cmd.substr(4);
        if ((hex.size() == 3 || hex.size() == 6) && isHex(hex))
            txId = static_cast<uint16_t>(std::strtoul(hex.c_str(), nullptr, 16));
        return "OK";
    }
    // ATCRAxxxx: fijar ID de respuesta
    if (cmd.rfind("ATCRA", 0) == 0) {
        const std::string hex = cmd.substr(5);
        if ((hex.size() == 3 || hex.size() == 6) && isHex(hex))
            rxId = static_cast<uint16_t>(std::strtoul(hex.c_str(), nullptr, 16));
        return "OK";
    }

    // Comandos aceptados por compatibilidad (sin efecto en el emulador)
    if (cmd == "ATAL" || cmd == "ATAL0" || cmd == "ATAL1" || cmd == "ATAR" ||
        cmd == "ATBI" || cmd == "ATBD" || cmd == "ATBD0" || cmd == "ATBD1" ||
        cmd == "ATCAF0" || cmd == "ATCAF1" || cmd == "ATCFC0" ||
        cmd == "ATCFC1" || cmd == "ATCEA" || cmd == "ATCEA0" ||
        cmd == "ATCEA1" || cmd == "ATIGN" || cmd == "ATWM" || cmd == "ATWM0D" ||
        cmd == "ATBRT" || cmd == "ATBRT38" || cmd == "ATBRD" || cmd == "ATIFR0" ||
        cmd == "ATIFR1" || cmd == "ATDM1" || cmd == "ATKW" || cmd == "ATMT" ||
        cmd == "ATPPS" || cmd == "ATCSM0" || cmd == "ATCSM1" ||
        cmd == "ATAT1" || cmd == "ATAT2")
        return "OK";
    // Inicialización CAN típica de escáneres profesionales (AUTEL, Launch,
    // Autel MaxiCOM...): aceptar y aplicar cabeceras/datos de Flow Control.
    if (cmd.rfind("ATFCSH", 0) == 0) {   // cabecera CAN (3 o 6 dígitos hex)
        const std::string hex = cmd.substr(6);
        if ((hex.size() == 3 || hex.size() == 6) && isHex(hex))
            txId = static_cast<uint16_t>(std::strtoul(hex.c_str(), nullptr, 16));
        return "OK";
    }
    if (cmd.rfind("ATFCSM", 0) == 0 || cmd.rfind("ATFCSD", 0) == 0)
        return "OK";
    if (cmd == "ATMA") return "NO DATA";   // monitorizar todo el tráfico CAN
    if (cmd.rfind("ATST", 0) == 0 || cmd.rfind("ATCM", 0) == 0 ||
        cmd.rfind("ATTP", 0) == 0)
        return "OK";

    return "?";
}

// ---------------------------------------------------------------------------
//  Procesamiento de una línea (consola ELM327 del menú)
// ---------------------------------------------------------------------------
std::string ELM327::process(const std::string& raw) {
    std::string line = raw;
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' ||
                             line.back() == '>'))
        line.pop_back();
    const size_t b = line.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    line = line.substr(b);

    const std::string cmd = toUpper(line);
    std::string out;
    if (echo) out += line + "\r";

    if (cmd.rfind("AT", 0) == 0) {
        out += handleAt(cmd) + terminator();
        return out;
    }
    if (!isHex(cmd)) { out += "?" + terminator(); return out; }

    const std::vector<uint8_t> bytes = hexToBytes(cmd);
    if (bytes.empty() || bytes[0] > 0x0A) {
        out += "?" + terminator();
        return out;
    }

    const uint8_t mode = bytes[0];
    const bool modeOnly = (mode == 0x03 || mode == 0x04 || mode == 0x07 ||
                           mode == 0x0A);
    std::vector<uint8_t> pids(bytes.begin() + 1, bytes.end());
    if (pids.empty()) {
        if (modeOnly) pids.push_back(0x00);
        else { out += "?" + terminator(); return out; }
    }

    for (uint8_t pid : pids) {
        // 1) Publicar la petición en el bus (la verá un escáner conectado).
        CanFrame req;
        req.id = txId;
        req.dlc = modeOnly ? 2 : 3;
        req.data[0] = req.dlc - 1;            // PCI: nº de bytes siguientes
        req.data[1] = mode;
        req.data[2] = modeOnly ? 0x00 : pid;
        can->sendMessage(req, 20);            // puede fallar sin otro nodo (ACK)

        if (!responsesOn) continue;           // ATR0: no mostrar respuestas

        // 2) Responder localmente: somos la propia ECU.
        uint8_t payload[64];
        int plen = 0;
        if (!getObdResponse(mode, pid, payload, plen)) {
            out += "NO DATA" + terminator();
            continue;
        }
        const std::vector<uint8_t> p(payload, payload + plen);
        sendIsoTp(rxId, p);
        out += formatPayload(p) + terminator();
    }
    return out;
}

// ---------------------------------------------------------------------------
//  Peticiones recibidas por CAN (escáner externo)
// ---------------------------------------------------------------------------
void ELM327::handleCanRequest(const CanFrame& f) {
    if (f.dlc < 2) return;
    const uint8_t pci = f.data[0];
    const int len = pci & 0x0F;
    // Peticiones single-frame: modo [y 1..6 PIDs]. ISO 15765-4 permite pedir
    // de 2 a 6 PIDs en una sola trama (BUG-02); se responde cada uno.
    if ((pci & 0xF0) != 0x00 || len < 1 || len > 6) return;

    const uint8_t mode = f.data[1];
    const bool modeOnly = (mode == 0x03 || mode == 0x04 || mode == 0x07 ||
                           mode == 0x0A);
    // Direccionamiento físico (0x7E0) responde desde 0x7E9; funcional
    // (0x7DF) desde 0x7E8 (BUG-07).
    const uint16_t respId = (f.id == 0x7E0) ? 0x7E9 : rxId;

    const int nPids = modeOnly ? 1 : (len - 1);
    for (int i = 0; i < nPids; ++i) {
        const uint8_t pid = modeOnly ? 0x00 : f.data[2 + i];

        uint8_t payload[64];
        int plen = 0;
        if (!getObdResponse(mode, pid, payload, plen))
            continue;   // PID sin soporte: el escáner mostrará NO DATA

        const std::vector<uint8_t> p(payload, payload + plen);
        sendIsoTp(respId, p);

        if (console) {
            char hdr[32];
            std::snprintf(hdr, sizeof(hdr), "OBD> req %02X %02X -> ", mode, pid);
            console->println(std::string(hdr) + formatPayload(p));
        }
    }
}

// Procesa las tramas no-FC interceptadas durante respuestas multi-frame.
void ELM327::drainPending() {
    std::vector<CanFrame> tmp;
    tmp.swap(pending);
    for (const auto& f : tmp)
        handleCanRequest(f);
}

// Monitor: tramas 0x7E8 que circulan por el bus.
void ELM327::handleCanResponse(const CanFrame& f) {
    if (!console) return;
    std::string s = "OBD> RX 7E8";
    for (int i = 0; i < static_cast<int>(f.dlc); ++i) {
        char b[8];
        std::snprintf(b, sizeof(b), " %02X", f.data[i]);
        s += b;
    }
    console->println(s);
}

// ---------------------------------------------------------------------------
//  Generación de respuestas OBD2
// ---------------------------------------------------------------------------
// Codifica un DTC (0x0301 = P0301) en los 2 bytes del formato OBD-II.
static void encodeDtc(uint16_t code, uint8_t& hi, uint8_t& lo) {
    hi = static_cast<uint8_t>((code >> 8) & 0xFF);
    lo = static_cast<uint8_t>(code & 0xFF);
}

bool ELM327::getObdResponse(uint8_t mode, uint8_t pid, uint8_t* out, int& len) {
    std::lock_guard<std::mutex> lk(veh->mtx);   // protege dtcs/mil
    switch (mode) {
        case 0x01:
        case 0x02: return getMode01(mode, pid, out, len);
        case 0x03: {   // DTCs confirmados
            const size_t n = dtcs.size() > 3 ? 3 : dtcs.size();
            out[0] = 0x43;
            out[1] = static_cast<uint8_t>(n * 2);
            for (size_t i = 0; i < n; ++i)
                encodeDtc(dtcs[i], out[2 + i * 2], out[3 + i * 2]);
            len = 2 + static_cast<int>(n * 2);
            return true;
        }
        case 0x04: {   // borrar DTCs: limpia códigos y MIL
            dtcs.clear();
            dtcsPending.clear();
            mil = false;
            warmupsSinceClear = 0;
            distanceSinceClearKm = 0;
            out[0] = 0x44;
            len = 1;
            return true;
        }
        case 0x07: {   // DTCs pendientes
            const size_t n = dtcsPending.size() > 3 ? 3 : dtcsPending.size();
            out[0] = 0x47;
            out[1] = static_cast<uint8_t>(n * 2);
            for (size_t i = 0; i < n; ++i)
                encodeDtc(dtcsPending[i], out[2 + i * 2], out[3 + i * 2]);
            len = 2 + static_cast<int>(n * 2);
            return true;
        }
        case 0x06: return getMode06(pid, out, len);
        case 0x08: // control de sistemas a bordo: ninguno controlable por OBD
            out[0] = 0x48; out[1] = 0x00;
            out[2] = 0x00; out[3] = 0x00; out[4] = 0x00; out[5] = 0x00;
            len = 6; return true;
        case 0x0A: {   // DTCs permanentes
            const size_t n = dtcs.size() > 3 ? 3 : dtcs.size();
            out[0] = 0x4A;
            out[1] = static_cast<uint8_t>(n * 2);
            for (size_t i = 0; i < n; ++i)
                encodeDtc(dtcs[i], out[2 + i * 2], out[3 + i * 2]);
            len = 2 + static_cast<int>(n * 2);
            return true;
        }
        case 0x09: return getMode09(pid, out, len);
        default:   return false;
    }
}

// ---------------------------------------------------------------------------
//  Modo 06: monitores OBD en servicio ("readiness")
//  Formato: 46 <TID> <2B: datos/unidad> <2B: valor> <2B: max> <2B: min>
// ---------------------------------------------------------------------------
// NOTA: se llama bajo veh->mtx (el lock lo toma getObdResponse).
bool ELM327::getMode06(uint8_t tid, uint8_t* out, int& len) {
    out[0] = 0x46;
    out[1] = tid;
    const auto put = [&](uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
        out[2] = static_cast<uint8_t>(a >> 8); out[3] = static_cast<uint8_t>(a & 0xFF);
        out[4] = static_cast<uint8_t>(b >> 8); out[5] = static_cast<uint8_t>(b & 0xFF);
        out[6] = static_cast<uint8_t>(c >> 8); out[7] = static_cast<uint8_t>(c & 0xFF);
        out[8] = static_cast<uint8_t>(d >> 8); out[9] = static_cast<uint8_t>(d & 0xFF);
        len = 10;
    };
    switch (tid) {
        case 0x00:   // TIDs soportados: 01, 02, 41, 61, 91 (bits 7.. de 4 bytes)
            out[2] = 0x80; out[3] = 0x40;   // TID 01 (misfire), TID 02 (fuel)
            out[4] = 0x20; out[5] = 0x10;   // TID 41 (catalizador), TID 61 (EVAP)
            out[6] = 0x08; out[7] = 0x00;   // TID 91 (O2)
            len = 8; return true;
        case 0x01: put(0x0000, 0x0000, 0x00FF, 0x0000); return true;  // misfire: 0
        case 0x02: put(0x0000, 0x0000, 0x00FF, 0x0000); return true;  // fuel sys
        case 0x41: put(0x0000, 0x0064, 0x00FF, 0x0032); return true;  // catalizador: 100% (0x64) en rango
        case 0x61: put(0x0000, 0x0014, 0x00FF, 0x000A); return true;  // EVAP: 20% (0x14)
        case 0x91: put(0x0000, 0x0080, 0x00FF, 0x0000); return true;  // O2: 50% (0x80)
        default:   return false;
    }
}

// NOTA: se llama bajo veh->mtx (el lock lo toma getObdResponse).
bool ELM327::getMode01(uint8_t mode, uint8_t pid, uint8_t* out, int& len) {
    out[0] = (mode == 0x02) ? 0x42 : 0x41;   // datos actuales / freeze frame
    out[1] = pid;

    const auto u8 = [](double v) {
        return static_cast<uint8_t>(std::lround(std::max(0.0, v)));
    };
    const auto u8sat = [](double v) {   // satura a [0,255]
        return static_cast<uint8_t>(
            std::lround(std::max(0.0, std::min(255.0, v))));
    };
    const auto v = [&](const std::string& k) { return veh->value(k); };

    switch (pid) {
        case 0x00:   // PIDs 01-20 soportados
            out[2] = 0xFE; out[3] = 0xDC; out[4] = 0x05; out[5] = 0x48;
            len = 6; return true;
        case 0x20:   // PIDs 21-40 soportados
            out[2] = 0x01; out[3] = 0x60; out[4] = 0x01; out[5] = 0x00;
            len = 6; return true;
        case 0x40:   // PIDs 41-60 soportados
            out[2] = 0x32; out[3] = 0x29; out[4] = 0x00; out[5] = 0x08;
            len = 6; return true;
        case 0x60:   // PIDs 61-80 soportados: ninguno
            out[2] = 0x00; out[3] = 0x00; out[4] = 0x00; out[5] = 0x00;
            len = 6; return true;
        case 0x01: {  // estado de monitores + MIL (bit 7) y nº de DTCs
            const uint8_t n = static_cast<uint8_t>(
                std::min<size_t>(dtcs.size(), 127));
            out[2] = (mil ? 0x80 : 0x00) | n;
            out[3] = 0x00; out[4] = 0x00; out[5] = 0x00;
            len = 6; return true;
        }
        case 0x03:   // estado del sistema de combustible
            out[2] = veh->engineOn() ? 0x02 : 0x01;   // lazo cerrado / abierto
            out[3] = 0x00;
            len = 4; return true;
        case 0x04: out[2] = u8(v("carga_motor") * 255.0 / 100.0); len = 3; return true;
        case 0x05: out[2] = u8(v("temp_refrigerante") + 40.0);    len = 3; return true;
        // Fuel trims (calibración de mezcla): A = 128 + %*128/100
        case 0x06: out[2] = u8sat(128.0 + v("stft1") * 128.0 / 100.0);
                  len = 3; return true;
        case 0x07: out[2] = u8sat(128.0 + v("ltft1") * 128.0 / 100.0);
                  len = 3; return true;
        case 0x0B: out[2] = u8(v("map"));                         len = 3; return true;
        case 0x0C: {
            const uint16_t r = static_cast<uint16_t>(v("rpm") / 4.0);
            out[2] = static_cast<uint8_t>(r >> 8);
            out[3] = static_cast<uint8_t>(r & 0xFF);
            len = 4; return true;
        }
        case 0x0D: out[2] = u8(v("velocidad"));                   len = 3; return true;
        case 0x0E: {   // avance de encendido: A = ° + 64 (rango -64..63.5)
            out[2] = u8sat(64.0 + v("avance_encendido"));
            len = 3; return true;
        }
        case 0x0F: out[2] = u8(v("temp_admision") + 40.0);        len = 3; return true;
        case 0x10: {
            const uint16_t m = static_cast<uint16_t>(v("maf") * 100.0);
            out[2] = static_cast<uint8_t>(m >> 8);
            out[3] = static_cast<uint8_t>(m & 0xFF);
            len = 4; return true;
        }
        case 0x11: out[2] = u8(v("mariposa") * 255.0 / 100.0);    len = 3; return true;
        case 0x13: out[2] = u8(v("sonda_o2") * 200.0); out[3] = 0x80; len = 4; return true;
        case 0x1C: out[2] = 0x06;   // ISO 15765-4 CAN
                  len = 3; return true;
        case 0x1F: {
            const uint16_t rt = static_cast<uint16_t>(
                std::min(v("tiempo_motor"), 65535.0));
            out[2] = static_cast<uint8_t>(rt >> 8);
            out[3] = static_cast<uint8_t>(rt & 0xFF);
            len = 4; return true;
        }
        case 0x21:
        case 0x31: {
            const uint32_t km = static_cast<uint32_t>(v("distancia"));
            out[2] = static_cast<uint8_t>(km >> 24);
            out[3] = static_cast<uint8_t>(km >> 16);
            out[4] = static_cast<uint8_t>(km >> 8);
            out[5] = static_cast<uint8_t>(km & 0xFF);
            len = 6; return true;
        }
        case 0x2E: out[2] = u8(v("evap_purge") * 255.0 / 100.0);   len = 3; return true;
        case 0x2F: out[2] = u8(v("nivel_combustible") * 255.0 / 100.0); len = 3; return true;
        case 0x33: out[2] = u8(v("baro"));                         len = 3; return true;  // BARO kPa
        case 0x42: out[2] = u8(v("voltaje_bateria") / 0.8);            len = 3; return true;
        case 0x45: out[2] = u8(v("mariposa") * 255.0 / 100.0);         len = 3; return true;
        case 0x46: out[2] = u8(v("temp_ambiente") + 40.0);             len = 3; return true;
        case 0x49: out[2] = u8(std::min(v("mariposa") + 5.0, 100.0) * 255.0 / 100.0);
                  len = 3; return true;
        case 0x4C: out[2] = u8(v("mariposa") * 255.0 / 100.0);         len = 3; return true;
        // PID personalizado 0x4E: marcha (0=N, 1-5, 6=R)
        case 0x4E: out[2] = static_cast<uint8_t>(v("marcha"));         len = 3; return true;
        case 0x5C: out[2] = u8(v("temp_aceite") + 40.0);               len = 3; return true;
        default:   return false;
    }
}

bool ELM327::getMode09(uint8_t pid, uint8_t* out, int& len) {
    switch (pid) {
        case 0x00:   // PIDs de modo 09 soportados: 02 (VIN), 04 (CALID), 0A (ECU)
            out[0] = 0x49; out[1] = 0x00;
            out[2] = 0x03;                // nº de PIDs soportados
            out[3] = 0x0A; out[4] = 0x02;  // máscara: 02 y 0A (SAE J1979)
            out[5] = 0x00; out[6] = 0x00;
            len = 7; return true;
        case 0x02: { // VIN (17 caracteres) -> multi-frame ISO-TP
            static const char* vin = "9BGKL48T0HB130763";
            out[0] = 0x49; out[1] = 0x02; out[2] = 0x01;
            for (int i = 0; i < 17; ++i) out[3 + i] =
                static_cast<uint8_t>(vin[i]);
            len = 20; return true;
        }
        case 0x04: { // IDs de calibración (2 calibraciones, con conteo)
            static const char* cal1 = "1505708";
            static const char* cal2 = "52124404";
            out[0] = 0x49; out[1] = 0x04; out[2] = 0x02;   // conteo = 2
            for (int i = 0; i < 7; ++i) out[3 + i] =
                static_cast<uint8_t>(cal1[i]);
            for (int i = 0; i < 8; ++i) out[10 + i] =
                static_cast<uint8_t>(cal2[i]);
            len = 18; return true;
        }
        case 0x0A: { // Nombre de la ECU
            static const char* name = "GM PRISMA 1.4";
            out[0] = 0x49; out[1] = 0x0A; out[2] = 0x01;
            for (int i = 0; i < 13; ++i) out[3 + i] =
                static_cast<uint8_t>(name[i]);
            len = 16; return true;
        }
        default: return false;
    }
}

// ---------------------------------------------------------------------------
//  ISO-TP (ISO 15765-2): single frame o multi-frame (FF + FC + CF)
// ---------------------------------------------------------------------------
bool ELM327::sendIsoTp(uint16_t id, const std::vector<uint8_t>& payload,
                       int fcTimeoutMs) {
    // Single frame: PCI = longitud (1-7 bytes).
    if (payload.size() <= 7) {
        CanFrame f;
        f.id = id;
        f.dlc = static_cast<uint8_t>(1 + payload.size());
        f.data[0] = static_cast<uint8_t>(payload.size());
        for (size_t i = 0; i < payload.size(); ++i)
            f.data[1 + i] = payload[i];
        return can->sendMessage(f, 50);
    }

    // Primer frame (FF): PCI 0x10 | longitud alta, luego longitud baja y 6 bytes.
    CanFrame ff;
    ff.id = id;
    const size_t n0 = std::min<size_t>(6, payload.size());
    ff.dlc = static_cast<uint8_t>(2 + n0);
    ff.data[0] = 0x10 | static_cast<uint8_t>((payload.size() >> 8) & 0x0F);
    ff.data[1] = static_cast<uint8_t>(payload.size() & 0xFF);
    for (size_t i = 0; i < n0; ++i) ff.data[2 + i] = payload[i];
    if (!can->sendMessage(ff, 50)) return false;

    // Esperar el control de flujo (FC, PCI 0x30) del receptor. El FC lo
    // transmite el escáner con SU propio ID de transmisión (0x7DF/0x7E0),
    // nunca desde 0x7E8: se acepta venga de donde venga (BUG-01). Las tramas
    // que no son FC (peticiones nuevas) se guardan para no perderlas.
    const uint32_t t0 = nowMs();
    bool gotFc = false;
    while (nowMs() - t0 < static_cast<uint32_t>(fcTimeoutMs)) {
        CanFrame r;
        if (can->receiveMessage(r)) {
            if (r.dlc >= 1 && (r.data[0] & 0xF0) == 0x30) {
                gotFc = true;
                break;
            }
            // No es FC: retener la trama para procesarla después.
            pending.push_back(r);
        }
        bcm2835_delayMicroseconds(1000);
    }
    if (!gotFc) return false;

    // Marcos consecutivos (CF): PCI 0x20 | secuencia, 7 bytes cada uno.
    size_t offset = n0;
    uint8_t seq = 1;
    while (offset < payload.size()) {
        CanFrame cf;
        cf.id = id;
        const size_t m = std::min<size_t>(7, payload.size() - offset);
        cf.dlc = static_cast<uint8_t>(1 + m);
        cf.data[0] = 0x20 | (seq & 0x0F);
        for (size_t i = 0; i < m; ++i) cf.data[1 + i] = payload[offset + i];
        if (!can->sendMessage(cf, 50)) return false;
        offset += m;
        ++seq;
        bcm2835_delayMicroseconds(500);
    }
    return true;
}
