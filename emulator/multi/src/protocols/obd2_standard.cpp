#include "obd2_standard.hpp"
#include <cmath>

namespace ecumult {

const std::unordered_map<uint8_t, OBD2Standard::PIDInfo>& OBD2Standard::getPIDs() {
    static const auto pids = initPIDs();
    return pids;
}

std::unordered_map<uint8_t, OBD2Standard::PIDInfo> OBD2Standard::initPIDs() {
    std::unordered_map<uint8_t, PIDInfo> map;
    map[0x00] = {0x00, "Supported PIDs 01-20", "", 4, 0, 0, "bitmask"};
    map[0x01] = {0x01, "Monitor Status", "", 4, 0, 0, "bitmask"};
    map[0x03] = {0x03, "Fuel System Status", "", 2, 0, 0, "bitmask"};
    map[0x04] = {0x04, "Engine Load", "%", 1, 0, 100, "A*100/255"};
    map[0x05] = {0x05, "Coolant Temp", "°C", 1, -40, 215, "A-40"};
    map[0x06] = {0x06, "Fuel Trim ST B1", "%", 1, -100, 99.2f, "(A-128)*100/128"};
    map[0x07] = {0x07, "Fuel Trim LT B1", "%", 1, -100, 99.2f, "(A-128)*100/128"};
    map[0x08] = {0x08, "Fuel Trim ST B2", "%", 1, -100, 99.2f, "(A-128)*100/128"};
    map[0x09] = {0x09, "Fuel Trim LT B2", "%", 1, -100, 99.2f, "(A-128)*100/128"};
    map[0x0A] = {0x0A, "Fuel Pressure", "kPa", 1, 0, 765, "A*3"};
    map[0x0B] = {0x0B, "Intake MAP", "kPa", 1, 0, 255, "A"};
    map[0x0C] = {0x0C, "Engine RPM", "rpm", 2, 0, 16383.75f, "((A*256)+B)/4"};
    map[0x0D] = {0x0D, "Vehicle Speed", "km/h", 1, 0, 255, "A"};
    map[0x0E] = {0x0E, "Timing Advance", "°", 1, -64, 63.5f, "(A-128)/2"};
    map[0x0F] = {0x0F, "Intake Air Temp", "°C", 1, -40, 215, "A-40"};
    map[0x10] = {0x10, "MAF", "g/s", 2, 0, 655.35f, "((A*256)+B)/100"};
    map[0x11] = {0x11, "Throttle Position", "%", 1, 0, 100, "A*100/255"};
    map[0x13] = {0x13, "O2 Sensors Present", "", 1, 0, 0, "bitmask"};
    map[0x14] = {0x14, "O2 B1S1", "V", 2, 0, 1.275f, "A*0.005"};
    map[0x15] = {0x15, "O2 B1S2", "V", 2, 0, 1.275f, "A*0.005"};
    map[0x1C] = {0x1C, "OBD Standard", "", 1, 0, 0, "A"};
    map[0x1F] = {0x1F, "Run Time", "s", 2, 0, 65535, "((A*256)+B)"};
    map[0x20] = {0x20, "Supported PIDs 21-40", "", 4, 0, 0, "bitmask"};
    map[0x21] = {0x21, "Dist with MIL", "km", 2, 0, 65535, "((A*256)+B)"};
    map[0x22] = {0x22, "Fuel Rail Pressure", "kPa", 2, 0, 5177.265f, "((A*256)+B)*0.079"};
    map[0x2F] = {0x2F, "Fuel Level", "%", 1, 0, 100, "A*100/255"};
    map[0x31] = {0x31, "Dist since Clear", "km", 2, 0, 65535, "((A*256)+B)"};
    map[0x33] = {0x33, "Barometric Pressure", "kPa", 1, 0, 255, "A"};
    map[0x42] = {0x42, "Control Module Voltage", "V", 2, 0, 65.535f, "((A*256)+B)/1000"};
    map[0x43] = {0x43, "Absolute Load", "%", 2, 0, 25700, "((A*256)+B)*100/255"};
    map[0x44] = {0x44, "Command Equiv Ratio", "ratio", 2, 0, 2, "((A*256)+B)/32768"};
    map[0x45] = {0x45, "Relative Throttle", "%", 1, 0, 100, "A*100/255"};
    map[0x46] = {0x46, "Ambient Air Temp", "°C", 1, -40, 215, "A-40"};
    map[0x5C] = {0x5C, "Oil Temp", "°C", 1, -40, 210, "A-40"};
    return map;
}

std::vector<uint8_t> OBD2Standard::supportedPIDs(uint8_t range) {
    (void)range;
    return {0x00, 0x00, 0x00, 0x00};
}

std::vector<uint8_t> OBD2Standard::encodePID(uint8_t pid, float value) {
    (void)pid;
    (void)value;
    return {0x00};
}

float OBD2Standard::decodePID(uint8_t pid, const std::vector<uint8_t>& data) {
    auto& pids = getPIDs();
    auto it = pids.find(pid);
    if (it == pids.end() || data.empty()) return 0;

    float A = data.size() > 0 ? data[0] : 0;
    float B = data.size() > 1 ? data[1] : 0;

    if (it->second.formula == "A") return A;
    if (it->second.formula == "A-40") return A - 40;
    if (it->second.formula == "A*100/255") return A * 100.0f / 255.0f;
    if (it->second.formula == "A*3") return A * 3;
    if (it->second.formula == "(A-128)/2") return (A - 128) / 2;
    if (it->second.formula == "A*0.005") return A * 0.005f;
    if (it->second.formula == "(A-128)*100/128") return (A - 128) * 100.0f / 128.0f;
    if (it->second.formula == "((A*256)+B)/4") return ((A * 256) + B) / 4;
    if (it->second.formula == "((A*256)+B)/100") return ((A * 256) + B) / 100.0f;
    if (it->second.formula == "((A*256)+B)") return (A * 256) + B;
    if (it->second.formula == "((A*256)+B)*0.079") return ((A * 256) + B) * 0.079f;
    if (it->second.formula == "((A*256)+B)/1000") return ((A * 256) + B) / 1000.0f;
    if (it->second.formula == "((A*256)+B)*100/255") return ((A * 256) + B) * 100.0f / 255.0f;
    if (it->second.formula == "((A*256)+B)/32768") return ((A * 256) + B) / 32768.0f;

    return 0;
}

std::vector<uint8_t> OBD2Standard::encodeDTC(uint16_t code) {
    return {static_cast<uint8_t>((code >> 8) & 0xFF),
            static_cast<uint8_t>(code & 0xFF)};
}

uint16_t OBD2Standard::decodeDTC(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + 1 >= data.size()) return 0;
    return (data[offset] << 8) | data[offset + 1];
}

uint8_t OBD2Standard::calculateChecksum(const std::vector<uint8_t>& data) {
    uint8_t cs = 0;
    for (auto b : data) cs += b;
    return cs & 0xFF;
}

bool OBD2Standard::verifyChecksum(const std::vector<uint8_t>& data) {
    if (data.empty()) return false;
    uint8_t cs = data.back();
    uint8_t calc = 0;
    for (size_t i = 0; i < data.size() - 1; i++) calc += data[i];
    return (calc & 0xFF) == cs;
}

} // namespace ecumult
