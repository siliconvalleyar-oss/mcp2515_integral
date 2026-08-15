// gm_commands.cpp — Comandos GM modo 22 UDS con PIDs corregidos
#include "gm_commands.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdint>
#include <unistd.h>

GMCommands::GMCommands(ELM327* elm327) : elm(elm327) {}
GMCommands::~GMCommands() { restoreHeader(); }

bool GMCommands::setupGMHeader() {
    elm->send("AT SH 7E0", 100);
    elm->send("AT CRA 7E8", 100);
    elm->send("AT FC SH 7E0", 60);
    elm->send("AT FC SD 30 00 00", 60);
    elm->send("AT FC SM 1", 60);
    usleep(100000);
    return true;
}
bool GMCommands::restoreHeader() {
    elm->send("AT SH 7DF", 100);
    elm->send("AT CRA", 100);
    elm->send("AT FC SM 0", 60);
    return true;
}

std::string GMCommands::sendCommand(const std::string& pidHex, bool useGMHeader) {
    if (!elm->isOnline()) return "Error: Sin conexion";
    if (useGMHeader) setupGMHeader();
    std::string response = elm->send("22 " + pidHex, 600);
    if (useGMHeader) restoreHeader();
    if (response.empty()) return "Sin respuesta";
    if (response.find("7F") != std::string::npos) {
        size_t pos = response.find("7F");
        if (pos!=std::string::npos && response.length()>=pos+6) {
            std::string nrc = response.substr(pos+4,2);
            if (nrc=="11") return "Error: Servicio no soportado";
            if (nrc=="22") return "Error: Condiciones incorrectas";
            if (nrc=="31") return "Error: PID fuera de rango";
            return "Error UDS NRC=" + nrc;
        }
        return "Error UDS: " + response;
    }
    return response;
}

// Helper: extraer bytes de datos tras prefijo
static std::string extractData(const std::string& resp, const std::string& pfx) {
    std::string u=resp, p=pfx;
    std::transform(u.begin(),u.end(),u.begin(),::toupper);
    std::transform(p.begin(),p.end(),p.begin(),::toupper);
    u.erase(std::remove(u.begin(),u.end(),' '),u.end());
    p.erase(std::remove(p.begin(),p.end(),' '),p.end());
    size_t pos=u.find(p);
    return pos==std::string::npos ? "" : u.substr(pos+p.length());
}
static uint32_t toU32(const std::string& h, size_t off, int n) {
    uint32_t r=0;
    for(int i=0;i<n;i++){size_t idx=off+i*2;if(idx+2>h.length())break;r=(r<<8)|(uint32_t)std::stoi(h.substr(idx,2),nullptr,16);}
    return r;
}

// ODÓMETRO — PID B100, 4 bytes BE, unidad 0.1 km
std::string GMCommands::decodeKilometers(const std::string& r) {
    std::string d = extractData(r,"62B100");
    if (d.length()<8) return "Formato invalido: "+r;
    double km = toU32(d,0,4)/10.0;
    std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<km<<" km";
    return ss.str();
}
std::string GMCommands::getKilometers() {
    std::string r = sendCommand("B1 00",true);
    if (r.find("Error")!=std::string::npos||r.find("Sin")!=std::string::npos) return r;
    return decodeKilometers(r);
}

// TEMP CATALIZADOR — PID 01B4, (raw16*0.1)-40 °C
std::string GMCommands::decodeCatalystTemp(const std::string& r) {
    std::string d=extractData(r,"6201B4"); bool b2=false;
    if(d.empty()){d=extractData(r,"6201B5");b2=true;}
    if(d.length()<4) return "Formato invalido: "+r;
    double t=toU32(d,0,2)*0.1-40.0;
    std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<t<<" C (B"<<(b2?2:1)<<")";
    return ss.str();
}
std::string GMCommands::getCatalystTemp() {
    std::string r=sendCommand("01 B4",true);
    if(r.find("Error")!=std::string::npos||r.find("Sin")!=std::string::npos) r=sendCommand("01 B5",true);
    if(r.find("Error")!=std::string::npos||r.find("Sin")!=std::string::npos) return r;
    return decodeCatalystTemp(r);
}

// PRESIÓN COMBUSTIBLE — PID 1180, raw16*4 kPa
std::string GMCommands::decodeFuelPressure(const std::string& r) {
    std::string d=extractData(r,"621180");
    if(d.empty()) d=extractData(r,"621181");
    if(d.length()<4) return "Formato invalido: "+r;
    double kPa=toU32(d,0,2)*4.0;
    std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<kPa<<" kPa ("<<kPa/100.0<<"bar)";
    return ss.str();
}
std::string GMCommands::getFuelPressure() {
    std::string r=sendCommand("11 80",true);
    if(r.find("Error")!=std::string::npos||r.find("Sin")!=std::string::npos) r=sendCommand("11 81",true);
    if(r.find("Error")!=std::string::npos||r.find("Sin")!=std::string::npos) return r;
    return decodeFuelPressure(r);
}

// TORQUE — PID 01A9, (raw16*0.5)-848 Nm
std::string GMCommands::decodeEngineTorque(const std::string& r) {
    std::string d=extractData(r,"6201A9");
    if(d.length()<4) return "Formato invalido: "+r;
    double nm=toU32(d,0,2)*0.5-848.0;
    std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<nm<<" Nm";
    return ss.str();
}
std::string GMCommands::getEngineTorque() {
    std::string r=sendCommand("01 A9",true);
    if(r.find("Error")!=std::string::npos||r.find("Sin")!=std::string::npos) return r;
    return decodeEngineTorque(r);
}

// VOLTAJE ECU — PID 01A1, raw16*0.001 V
std::string GMCommands::decodeECUVoltage(const std::string& r) {
    std::string d=extractData(r,"6201A1"); bool alt=false;
    if(d.empty()){d=extractData(r,"620280");alt=true;}
    if(d.length()<4) return "Formato invalido: "+r;
    double v=toU32(d,0,2)*(alt?0.1:0.001);
    std::stringstream ss; ss<<std::fixed<<std::setprecision(3)<<v<<" V";
    if(v<11.5) ss<<" BAJA!";
    return ss.str();
}
std::string GMCommands::getECUVoltage() {
    std::string r=sendCommand("01 A1",true);
    if(r.find("Error")!=std::string::npos||r.find("Sin")!=std::string::npos) r=sendCommand("02 80",true);
    if(r.find("Error")!=std::string::npos||r.find("Sin")!=std::string::npos) return r;
    return decodeECUVoltage(r);
}

// HISTORIAL — Servicio 19 02 FF
std::string GMCommands::decodeGMHistory(const std::string& r) {
    if(r.empty()) return "Sin datos";
    std::string d=extractData(r,"5902");
    if(d.empty()) return "Formato invalido";
    if(d.length()>2) d=d.substr(2); // saltar availability mask
    if(d.empty()) return "Sin codigos";
    const char* types[]={"P","C","B","U"};
    std::string result; int cnt=0;
    for(size_t i=0; i+8<=d.length(); i+=8) {
        uint8_t b1=(uint8_t)std::stoi(d.substr(i,2),nullptr,16);
        uint8_t b2=(uint8_t)std::stoi(d.substr(i+2,2),nullptr,16);
        uint8_t b3=(uint8_t)std::stoi(d.substr(i+4,2),nullptr,16);
        uint8_t st=(uint8_t)std::stoi(d.substr(i+6,2),nullptr,16);
        if(b1==0&&b2==0&&b3==0) continue;
        int ti=(b1>>6)&3; uint32_t n=((b1&0x3F)<<16)|(b2<<8)|b3;
        std::stringstream ss;
        ss<<types[ti]<<std::setw(4)<<std::setfill('0')<<std::hex<<std::uppercase<<n;
        if(st&0x08) ss<<" CONF"; if(st&0x01) ss<<" ACT";
        if(!result.empty()) result+=", ";
        result+=ss.str(); cnt++;
    }
    if(result.empty()) return "Sin codigos";
    return std::to_string(cnt)+" DTC: "+result;
}
std::string GMCommands::getGMHistory() {
    std::string r=sendCommand("19 02 FF",true);
    if(r.find("Error")!=std::string::npos||r.find("Sin")!=std::string::npos) return r;
    return decodeGMHistory(r);
}
std::string GMCommands::clearGMHistory() {
    std::string r=sendCommand("14 FF FF FF",true);
    if(r.find("54")!=std::string::npos) return "Historial borrado OK";
    return "Error: "+r;
}
std::string GMCommands::resetAdaptations() {
    std::string r=sendCommand("31 01 C1 0F",true);
    if(r.find("71")!=std::string::npos) return "Adaptativos reseteados OK";
    return "Error: "+r;
}

void GMCommands::scanPIDs(uint16_t start, uint16_t end) {
    std::cout<<"\n=== SCAN PIDs GM modo 22 ===\n";
    setupGMHeader();
    int found=0;
    for(uint32_t pid=start; pid<=end; pid++) {
        std::stringstream ss;
        ss<<std::hex<<std::uppercase<<std::setw(2)<<std::setfill('0')<<((pid>>8)&0xFF)
          <<" "<<std::setw(2)<<std::setfill('0')<<(pid&0xFF);
        std::string r=elm->send("22 "+ss.str(),400);
        if(!r.empty()&&r.find("7F")==std::string::npos&&r.find("62")!=std::string::npos) {
            found++;
            std::cout<<"✓ 0x"<<std::hex<<pid<<" -> "<<r<<"\n";
        }
        usleep(60000);
    }
    restoreHeader();
    std::cout<<"Encontrados: "<<std::dec<<found<<"\n";
}

std::string GMCommands::processResponse(const std::string& r, const std::string& pfx) {
    if(r.empty()||r.find("7F")!=std::string::npos) return "";
    if(!pfx.empty()&&r.find(pfx)==std::string::npos) return "";
    return r;
}
void GMCommands::showError(const std::string& cmd, const std::string& r) {
    std::cerr<<"[GM ERR] "<<cmd<<": "<<(r.empty()?"timeout":r)<<"\n";
}
