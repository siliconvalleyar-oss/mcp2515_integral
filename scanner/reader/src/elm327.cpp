// elm327.cpp — Comunicación BT RFCOMM con ELM327
// Código corregido: PIDs fuel trim, O2 sensor, fórmulas
#include "elm327.hpp"

#include <fstream>
#include <functional>
#include <ctime>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

ELM327::ELM327(const std::string& macAddr, int ch)
    : sock(-1), mac(macAddr), channel(ch) {}

ELM327::~ELM327() { disconnect(); }

bool ELM327::connectBT()
{
    struct sockaddr_rc addr{};
    sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (sock < 0) { perror("[ERROR] socket"); return false; }

    addr.rc_family  = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t)channel;
    str2ba(mac.c_str(), &addr.rc_bdaddr);

    std::cout << "[INFO] Conectando a " << mac << " canal " << channel << "...\n";

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[ERROR] connect");
        close(sock); sock = -1;
        return false;
    }
    std::cout << "[OK] Conectado BT!\n";

    // Secuencia de inicialización ELM327
    send("ATZ",   1000);  // reset completo
    send("ATE0",   300);  // echo OFF
    send("ATL0",   200);  // linefeeds OFF
    send("ATS0",   200);  // spaces OFF
    send("ATSP0",  500);  // protocolo automático
    send("ATAT1",  200);  // adaptive timing
    send("ATST20", 200);  // timeout 200ms

    return true;
}

void ELM327::disconnect()
{
    if (sock >= 0) {
        std::cout << "[INFO] Cerrando socket BT\n";
        close(sock);
        sock = -1;
    }
}

std::string ELM327::readRaw()
{
    if (!isConnected()) return "";

    // Leer con timeout vía select
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    tv.tv_sec  = 0;
    tv.tv_usec = 600000; // 600ms timeout lectura

    if (select(sock + 1, &fds, nullptr, nullptr, &tv) <= 0) return "";

    char buffer[4096];
    int n = read(sock, buffer, sizeof(buffer) - 1);
    if (n <= 0) return "";

    buffer[n] = '\0';
    std::string result(buffer);
    // Limpiar: quitar CR, LF, espacios, prompt '>'
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), ' '),  result.end());
    result.erase(std::remove(result.begin(), result.end(), '>'),  result.end());
    // Convertir a mayúsculas
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string ELM327::send(const std::string& cmd, int delayMs)
{
    if (!isConnected()) return "";
    std::string full = cmd + "\r";
    std::cout << "[TX] " << cmd << std::endl;
    write(sock, full.c_str(), full.size());
    usleep(delayMs * 1000);
    std::string resp = readRaw();
    if (!resp.empty())
        std::cout << "[RX] " << resp.substr(0, 80) << std::endl;
    return resp;
}

std::vector<std::string> ELM327::splitResponse(const std::string& response)
{
    std::vector<std::string> results;
    // La respuesta ya viene limpia (sin espacios, en mayúsculas)
    std::string data = response;
    // Recortar hasta '>' si quedara alguno
    size_t arrowPos = data.find('>');
    if (arrowPos != std::string::npos) data = data.substr(0, arrowPos);

    for (size_t i = 0; i + 2 <= data.length(); i += 2)
        results.push_back(data.substr(i, 2));

    return results;
}

// ─── PARÁMETROS OBD-II ───────────────────────────────────────────────────────

int ELM327::getRPM() {
    auto b = splitResponse(send("010C"));
    if (b.size() >= 4 && b[0]=="41" && b[1]=="0C")
        return (std::stoi(b[2],nullptr,16)*256 + std::stoi(b[3],nullptr,16)) / 4;
    return -1;
}
int ELM327::getSpeed() {
    auto b = splitResponse(send("010D"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="0D")
        return std::stoi(b[2],nullptr,16);
    return -1;
}
int ELM327::getCoolantTemp() {
    auto b = splitResponse(send("0105"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="05")
        return std::stoi(b[2],nullptr,16) - 40;
    return -99;
}
int ELM327::getTemp() { return getCoolantTemp(); }
int ELM327::getEngineLoad() {
    auto b = splitResponse(send("0104"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="04")
        return (std::stoi(b[2],nullptr,16)*100)/255;
    return -1;
}
double ELM327::getThrottlePosition() {
    auto b = splitResponse(send("0111"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="11")
        return std::stoi(b[2],nullptr,16)*100.0/255.0;
    return -1.0;
}
double ELM327::getIntakePressure() {
    auto b = splitResponse(send("010B"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="0B")
        return std::stoi(b[2],nullptr,16);
    return -1.0;
}
int ELM327::getIntakeTemp() {
    auto b = splitResponse(send("010F"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="0F")
        return std::stoi(b[2],nullptr,16) - 40;
    return -99;
}
double ELM327::getTimingAdvance() {
    auto b = splitResponse(send("010E"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="0E")
        return std::stoi(b[2],nullptr,16)/2.0 - 64.0;
    return -99.0;
}
double ELM327::getFuelPressure() {
    auto b = splitResponse(send("010A"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="0A")
        return std::stoi(b[2],nullptr,16)*3.0;
    return -1.0;
}
double ELM327::getMAF() {
    auto b = splitResponse(send("0110"));
    if (b.size() >= 4 && b[0]=="41" && b[1]=="10")
        return (std::stoi(b[2],nullptr,16)*256 + std::stoi(b[3],nullptr,16))/100.0;
    return -1.0;
}
double ELM327::getFuelLevel() {
    auto b = splitResponse(send("012F"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="2F")
        return std::stoi(b[2],nullptr,16)*100.0/255.0;
    return -1.0;
}
double ELM327::getAmbientTemp() {
    auto b = splitResponse(send("0146"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="46")
        return std::stoi(b[2],nullptr,16) - 40.0;
    return -99.0;
}
double ELM327::getOilTemp() {
    auto b = splitResponse(send("015C"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="5C")
        return std::stoi(b[2],nullptr,16) - 40.0;
    return -99.0;
}
double ELM327::getCommandedEGR() {
    auto b = splitResponse(send("012C"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="2C")
        return std::stoi(b[2],nullptr,16)*100.0/255.0;
    return -1.0;
}
double ELM327::getEGRError() {
    auto b = splitResponse(send("012D"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="2D")
        return (std::stoi(b[2],nullptr,16)*100.0/128.0) - 100.0;
    return -1.0;
}
double ELM327::getEVAPPressure() {
    auto b = splitResponse(send("0132"));
    if (b.size() >= 4 && b[0]=="41" && b[1]=="32")
        return (std::stoi(b[2],nullptr,16)*256 + std::stoi(b[3],nullptr,16))/4.0;
    return -1.0;
}
double ELM327::getBarometricPressure() {
    auto b = splitResponse(send("0133"));
    if (b.size() >= 3 && b[0]=="41" && b[1]=="33")
        return std::stoi(b[2],nullptr,16);
    return -1.0;
}

// ─── FUEL TRIM — PIDs corregidos ─────────────────────────────────────────────
// Fórmula correcta OBD-II: ((A-128)*100)/128

double ELM327::getShortTermTrimBank1() {
    auto b = splitResponse(send("0106"));
    if (b.size()>=3 && b[0]=="41" && b[1]=="06")
        return ((std::stoi(b[2],nullptr,16)-128)*100.0)/128.0;
    return -999.0;
}
double ELM327::getShortTermTrimBank2() {
    auto b = splitResponse(send("0108"));
    if (b.size()>=3 && b[0]=="41" && b[1]=="08")
        return ((std::stoi(b[2],nullptr,16)-128)*100.0)/128.0;
    return -999.0;
}
double ELM327::getLongTermTrimBank1() {
    auto b = splitResponse(send("0107"));
    if (b.size()>=3 && b[0]=="41" && b[1]=="07")
        return ((std::stoi(b[2],nullptr,16)-128)*100.0)/128.0;
    return -999.0;
}
double ELM327::getLongTermTrimBank2() {
    auto b = splitResponse(send("0109"));
    if (b.size()>=3 && b[0]=="41" && b[1]=="09")
        return ((std::stoi(b[2],nullptr,16)-128)*100.0)/128.0;
    return -999.0;
}

FuelTrim ELM327::getAllFuelTrims() {
    FuelTrim ft;
    ft.shortTermBank1 = getShortTermTrimBank1();
    ft.shortTermBank2 = getShortTermTrimBank2();
    ft.longTermBank1  = getLongTermTrimBank1();
    ft.longTermBank2  = getLongTermTrimBank2();
    ft.available = (ft.shortTermBank1 != -999.0 || ft.longTermBank1 != -999.0);
    return ft;
}

// ─── SENSORES O2 — 1 sensor por PID (0x14-0x1B) ──────────────────────────────
// Voltaje: A*0.005 V    Trim: ((B-128)*100)/128 %

OxygenSensor ELM327::getO2Sensor(int bank, int sensor)
{
    OxygenSensor r;
    r.bank = bank; r.sensor = sensor;
    r.voltage = -1.0; r.shortTermTrim = -999.0;

    int pidBase;
    if (bank == 1 && sensor >= 1 && sensor <= 4) pidBase = 0x14 + (sensor-1);
    else if (bank == 2 && sensor >= 1 && sensor <= 4) pidBase = 0x18 + (sensor-1);
    else return r;

    std::stringstream ss;
    ss << "01" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << pidBase;
    auto bytes = splitResponse(send(ss.str()));

    std::stringstream pb;
    pb << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << pidBase;

    if (bytes.size()>=4 && bytes[0]=="41" && bytes[1]==pb.str()) {
        int vB = std::stoi(bytes[2],nullptr,16);
        int tB = std::stoi(bytes[3],nullptr,16);
        if (vB==0xFF && tB==0xFF) return r;
        r.voltage       = vB * 0.005;
        r.shortTermTrim = ((tB-128)*100.0)/128.0;
    }
    return r;
}

std::vector<OxygenSensor> ELM327::getOxygenSensors()
{
    std::vector<OxygenSensor> sensors;
    struct Map { int pid; int bank; int s; };
    std::vector<Map> map = {
        {0x14,1,1},{0x15,1,2},{0x16,1,3},{0x17,1,4},
        {0x18,2,1},{0x19,2,2},{0x1A,2,3},{0x1B,2,4}
    };
    for (auto& m : map) {
        auto s = getO2Sensor(m.bank, m.s);
        if (s.voltage >= 0) sensors.push_back(s);
    }
    return sensors;
}

// ─── DTCs ────────────────────────────────────────────────────────────────────

std::string ELM327::decodeDTCCode(const std::string& rawCode) {
    if (rawCode.length() < 4) return rawCode;
    // rawCode es 4 hex chars ej "0171"
    int hi = std::stoi(rawCode.substr(0,2),nullptr,16);
    int lo = std::stoi(rawCode.substr(2,2),nullptr,16);
    const char* types[] = {"P","C","B","U"};
    int typeIdx = (hi >> 6) & 0x03;
    int num = ((hi & 0x3F) << 8) | lo;
    std::stringstream ss;
    ss << types[typeIdx] << std::setw(4) << std::setfill('0') << std::hex << std::uppercase << num;
    return ss.str();
}

std::vector<DTCCode> ELM327::getDTCs()
{
    std::vector<DTCCode> dtcs;
    std::string r = send("03", 700);
    auto bytes = splitResponse(r);

    if (!bytes.empty() && bytes[0]=="43") {
        for (size_t i=1; i+1<bytes.size(); i+=2) {
            std::string code = bytes[i]+bytes[i+1];
            if (code!="0000" && code!="FFFF") {
                DTCCode dtc;
                dtc.code = decodeDTCCode(code);
                dtc.description = dtc.code;
                dtcs.push_back(dtc);
            }
        }
    }
    if (dtcs.empty()) { DTCCode n; n.code="NONE"; n.description="Sin codigos"; dtcs.push_back(n); }
    return dtcs;
}

bool ELM327::clearDTCs()
{
    send("ATZ",2000); send("ATE0",300); send("ATL0",200); send("ATS0",200); send("ATSP0",500);
    for (int i=0; i<3; i++) {
        std::string r = send("04",1500);
        if (r.find("44")!=std::string::npos || r.find("OK")!=std::string::npos) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return false;
}

// ─── DIAGNÓSTICO ─────────────────────────────────────────────────────────────

std::string ELM327::getProtocol() {
    std::string r = send("ATDP",200);
    return r;
}

bool ELM327::checkMIL() {
    auto b = splitResponse(send("0101"));
    if (b.size()>=3) return (std::stoi(b[2],nullptr,16) & 0x80) != 0;
    return false;
}

bool ELM327::setProtocol(int p) {
    return send("ATSP"+std::to_string(p),500).find("OK")!=std::string::npos;
}
bool ELM327::resetELM() {
    return send("ATZ",2000).find("ELM")!=std::string::npos;
}

std::string ELM327::getVIN()
{
    std::string fullCmd = "0902\r";
    write(sock, fullCmd.c_str(), fullCmd.size());
    usleep(700000);
    char buf[1024]; int n = read(sock,buf,sizeof(buf)-1);
    if (n<=0) return "N/A";
    buf[n]='\0';
    std::string resp(buf);
    resp.erase(std::remove(resp.begin(),resp.end(),'\r'),resp.end());
    resp.erase(std::remove(resp.begin(),resp.end(),' '),resp.end());
    std::transform(resp.begin(),resp.end(),resp.begin(),::toupper);

    std::string vin;
    size_t pos = resp.find("4902");
    if (pos!=std::string::npos) {
        std::string data = resp.substr(pos+4);
        for (size_t i=2; i+2<=data.length(); i+=2) {
            try {
                int v = std::stoi(data.substr(i,2),nullptr,16);
                if (v>=32 && v<=126) vin += (char)v;
            } catch(...) { break; }
        }
    }
    return vin.empty() ? "N/A" : vin;
}

std::string ELM327::getVehicleInfo() {
    std::string info = "Protocolo: " + getProtocol() + "\n";
    info += "VIN: " + getVIN() + "\n";
    info += checkMIL() ? "MIL: ACTIVA\n" : "MIL: OK\n";
    return info;
}

// ─── MODO 22 GM — sendCommand ────────────────────────────────────────────────

std::string ELM327::sendCommand(const std::string& pidHex)
{
    if (!isConnected()) return "";
    // Configurar header GM
    send("AT SH 7E0", 100);
    send("AT CRA 7E8", 100);
    send("AT FC SH 7E0", 60);
    send("AT FC SD 30 00 00", 60);
    send("AT FC SM 1", 60);
    usleep(100000);

    std::string cmd = "22 " + pidHex;
    std::string full = cmd + "\r";
    std::cout << "[TX GM] " << cmd << std::endl;
    write(sock, full.c_str(), full.size());

    // Leer con timeout 600ms
    char buffer[512]; std::string response;
    fd_set fds; struct timeval tv;
    FD_ZERO(&fds); FD_SET(sock,&fds);
    tv.tv_sec=0; tv.tv_usec=600000;
    while (true) {
        if (select(sock+1,&fds,nullptr,nullptr,&tv) <= 0) break;
        int n = recv(sock,buffer,sizeof(buffer)-1,0);
        if (n<=0) break;
        buffer[n]='\0'; response += buffer;
        if (response.find(">")!=std::string::npos) break;
    }

    // Restaurar
    send("AT SH 7DF",100);
    send("AT CRA",100);
    send("AT FC SM 0",60);

    // Limpiar
    response.erase(std::remove(response.begin(),response.end(),'\r'),response.end());
    response.erase(std::remove(response.begin(),response.end(),'\n'),response.end());
    response.erase(std::remove(response.begin(),response.end(),' '),response.end());
    response.erase(std::remove(response.begin(),response.end(),'>'),response.end());
    std::transform(response.begin(),response.end(),response.begin(),::toupper);

    std::cout << "[RX GM] " << response << std::endl;
    if (response.find("7F")!=std::string::npos) return "";
    return response;
}

// ─── LOGGING ─────────────────────────────────────────────────────────────────

void ELM327::logAllSensorsRaw(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    file << "timestamp,sensor,value\n";

    struct SI { std::string name; std::function<std::string()> fn; };
    std::vector<SI> sensors = {
        {"RPM",         [this](){ return std::to_string(getRPM()); }},
        {"Speed",       [this](){ return std::to_string(getSpeed()); }},
        {"CoolantTemp", [this](){ return std::to_string(getCoolantTemp()); }},
        {"EngineLoad",  [this](){ return std::to_string(getEngineLoad()); }},
        {"Throttle",    [this](){ return std::to_string(getThrottlePosition()); }},
        {"MAP",         [this](){ return std::to_string(getIntakePressure()); }},
        {"IntakeTemp",  [this](){ return std::to_string(getIntakeTemp()); }},
        {"TimingAdv",   [this](){ return std::to_string(getTimingAdvance()); }},
        {"MAF",         [this](){ return std::to_string(getMAF()); }},
        {"FuelLevel",   [this](){ return std::to_string(getFuelLevel()); }},
        {"BaroPress",   [this](){ return std::to_string(getBarometricPressure()); }},
        {"STFT_B1",     [this](){ return std::to_string(getShortTermTrimBank1()); }},
        {"STFT_B2",     [this](){ return std::to_string(getShortTermTrimBank2()); }},
        {"LTFT_B1",     [this](){ return std::to_string(getLongTermTrimBank1()); }},
        {"LTFT_B2",     [this](){ return std::to_string(getLongTermTrimBank2()); }},
        {"O2_B1S1_V",   [this](){ return std::to_string(getO2Sensor(1,1).voltage); }},
        {"O2_B2S1_V",   [this](){ return std::to_string(getO2Sensor(2,1).voltage); }},
    };
    time_t now = time(nullptr);
    for (auto& s : sensors) {
        file << now << ",\"" << s.name << "\",\"" << s.fn() << "\"\n";
        file.flush();
    }
    file.close();
}

void ELM327::logP0171Diagnostic(const std::string& filename, int durationSec) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    file << "timestamp,rpm,speed,stft_b1(%),stft_b2(%),ltft_b1(%),ltft_b2(%),o2_b1s1_V,o2_b2s1_V\n";

    time_t start = time(nullptr); int count=0;
    while (time(nullptr)-start < durationSec) {
        file << time(nullptr) << ","
             << getRPM() << ","
             << getSpeed() << ","
             << getShortTermTrimBank1() << ","
             << getShortTermTrimBank2() << ","
             << getLongTermTrimBank1() << ","
             << getLongTermTrimBank2() << ","
             << getO2Sensor(1,1).voltage << ","
             << getO2Sensor(2,1).voltage << "\n";
        file.flush();
        if (++count % 10 == 0)
            std::cout << "Registros: " << count << "\r" << std::flush;
        sleep(1);
    }
    file.close();
    std::cout << "\n[OK] Log P0171: " << count << " registros en " << filename << "\n";
}
