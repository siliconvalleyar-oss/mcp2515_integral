#include "scanner/obd2.hpp"
#include <cstring>
#include <cstdio>
#include <chrono>
#include <thread>
#include <algorithm>

namespace Scanner {

OBD2::OBD2(std::shared_ptr<Hardware::MCP2515> canInterface)
    : can_(std::move(canInterface)), initialized_(false),
      requestId_(0x7DF), responseId_(0x7E8) {}

OBD2::~OBD2() {
    cleanup();
}

bool OBD2::initialize() {
    if (!can_) return false;
    initialized_ = true;
    return true;
}

void OBD2::cleanup() {
    initialized_ = false;
}

static std::string toHex(uint8_t value) {
    char buf[5];
    snprintf(buf, sizeof(buf), "%02X", value);
    return buf;
}

bool OBD2::sendOBD2Request(uint8_t mode, uint8_t pid, uint8_t* response, size_t& length) {
    if (!initialized_ || !can_) return false;

    Hardware::CANMessage msg{};
    msg.id = requestId_;
    msg.extended = false;
    msg.dlc = 8;
    msg.data[0] = 0x02;  // Number of additional bytes
    msg.data[1] = mode;
    msg.data[2] = pid;
    msg.data[3] = 0x00;
    msg.data[4] = 0x00;
    msg.data[5] = 0x00;
    msg.data[6] = 0x00;
    msg.data[7] = 0x00;

    if (!can_->sendMessage(msg)) {
        return false;
    }

    return waitForResponse(response, length);
}

bool OBD2::waitForResponse(uint8_t* response, size_t& length, uint32_t timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    Hardware::CANMessage msg{};

    while (true) {
        if (can_->receiveMessage(msg)) {
            if (msg.id == responseId_ || msg.id == responseId_ + 1 || msg.id == responseId_ + 2) {
                memcpy(response, msg.data, msg.dlc);
                length = msg.dlc;
                return true;
            }
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed >= timeoutMs) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return false;
}

bool OBD2::requestPID(uint8_t pid, uint8_t* data, size_t length) {
    uint8_t response[8] = {0};
    size_t respLen = 0;

    if (!sendOBD2Request(0x01, pid, response, respLen)) {
        return false;
    }

    if (respLen < 3 || response[1] != 0x41 || response[2] != pid) {
        return false;
    }

    size_t available = respLen - 3;
    size_t copyLen = std::min(length, available);
    memcpy(data, &response[3], copyLen);
    return true;
}

bool OBD2::requestPIDEx(uint8_t pid, uint8_t* buffer, size_t bufferSize, size_t& outLen) {
    uint8_t response[8] = {0};
    size_t respLen = 0;

    if (!sendOBD2Request(0x01, pid, response, respLen)) {
        outLen = 0;
        return false;
    }

    if (respLen < 3 || response[1] != 0x41 || response[2] != pid) {
        outLen = 0;
        return false;
    }

    outLen = respLen - 3;
    if (outLen > bufferSize) {
        outLen = bufferSize;
    }
    memcpy(buffer, &response[3], outLen);
    return true;
}

bool OBD2::decodePID(uint8_t pid, const uint8_t* data, size_t len, PIDData& out) {
    if (len == 0) return false;

    out = PIDData();
    out.value = 0;
    out.min = 0;
    out.max = 0;

    switch (pid) {
        case 0x04: { // Engine Load
            if (len < 1) return false;
            out.name = "Engine Load";
            out.value = calculateLoad(data[0]);
            out.unit = "%";
            out.min = 0;
            out.max = 100;
            break;
        }
        case 0x05: { // ECT
            if (len < 1) return false;
            out.name = "ECT";
            out.value = calculateTemp(data[0]);
            out.unit = "°C";
            out.min = -40;
            out.max = 215;
            break;
        }
        case 0x06: { // STFT
            if (len < 1) return false;
            out.name = "STFT";
            out.value = calculateFuelTrim(data[0]);
            out.unit = "%";
            out.min = -100;
            out.max = 99.2;
            break;
        }
        case 0x07: { // LFT
            if (len < 1) return false;
            out.name = "LFT";
            out.value = calculateFuelTrim(data[0]);
            out.unit = "%";
            out.min = -100;
            out.max = 99.2;
            break;
        }
        case 0x0B: { // MAP
            if (len < 1) return false;
            out.name = "MAP";
            out.value = calculateMAP(data[0], len > 1 ? data[1] : 0);
            out.unit = "kPa";
            out.min = 0;
            out.max = 255;
            break;
        }
        case 0x0C: { // RPM
            if (len < 2) return false;
            out.name = "RPM";
            out.value = calculateRPM(data[0], data[1]);
            out.unit = "rpm";
            out.min = 0;
            out.max = 8000;
            break;
        }
        case 0x0D: { // Speed
            if (len < 1) return false;
            out.name = "Speed";
            out.value = calculateSpeed(data[0]);
            out.unit = "km/h";
            out.min = 0;
            out.max = 250;
            break;
        }
        case 0x0E: { // Ignition Timing
            if (len < 1) return false;
            out.name = "Ignition Timing";
            out.value = (data[0] / 2.0f) - 64.0f;
            out.unit = "°";
            out.min = -64;
            out.max = 63.5;
            break;
        }
        case 0x0F: { // IAT
            if (len < 1) return false;
            out.name = "IAT";
            out.value = calculateTemp(data[0]);
            out.unit = "°C";
            out.min = -40;
            out.max = 215;
            break;
        }
        case 0x10: { // MAF
            if (len < 2) return false;
            out.name = "MAF";
            out.value = calculateMAF(data[0], data[1]);
            out.unit = "g/s";
            out.min = 0;
            out.max = 1000;
            break;
        }
        case 0x11: { // TPS1
            if (len < 1) return false;
            out.name = "TPS1";
            out.value = calculateThrottle(data[0]);
            out.unit = "%";
            out.min = 0;
            out.max = 100;
            break;
        }
        case 0x2F: { // Fuel Level
            if (len < 1) return false;
            out.name = "Fuel Level";
            out.value = (data[0] * 100.0f) / 255.0f;
            out.unit = "%";
            out.min = 0;
            out.max = 100;
            break;
        }
        case 0x33: { // BARO
            if (len < 1) return false;
            out.name = "BARO";
            out.value = data[0];
            out.unit = "kPa";
            out.min = 0;
            out.max = 255;
            break;
        }
        case 0x45: { // TPS2 (GM specific)
            if (len < 1) return false;
            out.name = "TPS2";
            out.value = (data[0] * 100.0f) / 255.0f;
            out.unit = "%";
            out.min = 0;
            out.max = 100;
            break;
        }
        case 0x49: { // APP1
            if (len < 1) return false;
            out.name = "APP1";
            out.value = (data[0] * 100.0f) / 255.0f;
            out.unit = "%";
            out.min = 0;
            out.max = 100;
            break;
        }
        case 0x4A: { // APP2
            if (len < 1) return false;
            out.name = "APP2";
            out.value = (data[0] * 100.0f) / 255.0f;
            out.unit = "%";
            out.min = 0;
            out.max = 100;
            break;
        }
        case 0x5C: { // Oil temp
            if (len < 1) return false;
            out.name = "Oil Temp";
            out.value = data[0] - 40.0f;
            out.unit = "°C";
            out.min = -40;
            out.max = 215;
            break;
        }
        case 0x66: { // MAF voltage (GM specific)
            if (len < 1) return false;
            out.name = "MAF Voltage";
            out.value = (data[0] * 5.0f) / 255.0f;
            out.unit = "V";
            out.min = 0;
            out.max = 5;
            break;
        }
        default:
            out.name = "Unknown 0x" + toHex(pid);
            out.value = len > 0 ? data[0] : 0;
            out.unit = "";
            out.min = 0;
            out.max = 255;
            break;
    }
    return true;
}

float OBD2::calculateMAF(uint8_t a, uint8_t b) {
    return ((a * 256.0f) + b) / 100.0f;  // g/s
}

float OBD2::calculateMAP(uint8_t a, uint8_t b) {
    (void)b;
    return a;  // kPa
}

float OBD2::calculateRPM(uint8_t a, uint8_t b) {
    return ((a * 256.0f) + b) / 4.0f;
}

float OBD2::calculateSpeed(uint8_t a) {
    return static_cast<float>(a);  // km/h
}

float OBD2::calculateThrottle(uint8_t a) {
    return (a * 100.0f) / 255.0f;  // %
}

float OBD2::calculateFuelTrim(uint8_t a) {
    return (a - 128.0f) * 100.0f / 128.0f;  // %
}

float OBD2::calculateLoad(uint8_t a) {
    return (a * 100.0f) / 255.0f;  // %
}

float OBD2::calculateTemp(uint8_t a) {
    return a - 40.0f;  // °C
}

bool OBD2::requestDTCs(std::vector<std::string>& dtcs) {
    dtcs.clear();

    uint8_t response[8] = {0};
    size_t respLen = 0;

    if (!sendOBD2Request(0x03, 0x00, response, respLen)) {
        return false;
    }

    if (respLen < 3) return false;

    uint8_t count = response[2];
    for (size_t i = 0; i < count && (3 + i * 2) < respLen; ++i) {
        char dtcStr[5] = {0};
        snprintf(dtcStr, sizeof(dtcStr), "%c%c%c%c",
                 ((response[3 + i * 2] >> 6) & 0x03) + 'P',
                 ((response[3 + i * 2] >> 4) & 0x03) + '0',
                 ((response[3 + i * 2] >> 2) & 0x03) + '0',
                 (response[3 + i * 2] & 0x03) + '0');
        dtcs.emplace_back(dtcStr);
    }

    return true;
}

bool OBD2::clearDTCs() {
    uint8_t response[8] = {0};
    size_t respLen = 0;
    return sendOBD2Request(0x04, 0x00, response, respLen);
}

bool OBD2::requestLiveData(std::unordered_map<std::string, PIDData>& data) {
    data.clear();

    uint8_t raw[8] = {0};
    size_t len = 0;

    std::vector<std::pair<uint8_t, std::string>> pids = {
        {0x0C, "rpm"},
        {0x0D, "speed"},
        {0x11, "throttle"},
        {0x04, "load"},
        {0x05, "coolant_temp"},
        {0x10, "maf"},
        {0x0B, "map"},
        {0x33, "baro"},
        {0x0F, "iat"},
        {0x2F, "fuel_level"},
        {0x49, "app1"},
        {0x4A, "app2"},
        {0x06, "stft"},
        {0x07, "lft"},
        {0x0E, "ignition_timing"},
        {0x45, "tps2"},
        {0x5C, "oil_temp"},
        {0x66, "maf_voltage"},
    };

    for (const auto& [pid, key] : pids) {
        if (requestPIDEx(pid, raw, sizeof(raw), len)) {
            PIDData pidData;
            if (decodePID(pid, raw, len, pidData)) {
                data[key] = pidData;
            }
        }
    }

    return !data.empty();
}

bool OBD2::requestCustomList(const std::vector<uint8_t>& pids, std::unordered_map<std::string, PIDData>& data) {
    data.clear();

    uint8_t raw[8] = {0};
    size_t len = 0;

    for (uint8_t pid : pids) {
        if (requestPIDEx(pid, raw, sizeof(raw), len)) {
            PIDData pidData;
            if (decodePID(pid, raw, len, pidData)) {
                data[pidData.name] = pidData;
            }
        }
    }

    return !data.empty();
}

bool OBD2::requestFreezeFrame(uint32_t dtc, std::unordered_map<std::string, PIDData>& data) {
    data.clear();
    (void)dtc;

    uint8_t response[8] = {0};
    size_t respLen = 0;

    // Request freeze frame DTC (Mode 02 PID 01)
    if (!sendOBD2Request(0x02, 0x01, response, respLen) || respLen < 3) {
        return false;
    }

    uint8_t storedDTC[5] = {0};
    if (respLen >= 4) {
        snprintf(reinterpret_cast<char*>(storedDTC), sizeof(storedDTC), "%c%c%c%c",
                 ((response[2] >> 6) & 0x03) + 'P',
                 ((response[2] >> 4) & 0x03) + '0',
                 ((response[2] >> 2) & 0x03) + '0',
                 (response[2] & 0x03) + '0');
    }
    data["dtc"] = {"DTC", "", 0, 0, 0};

    // Request freeze frame parameters
    uint8_t freezePIDs[] = {0x0F, 0x10, 0x0B, 0x2F};
    for (uint8_t pid : freezePIDs) {
        uint8_t raw[8] = {0};
        size_t len = 0;
        if (requestPIDEx(pid, raw, sizeof(raw), len)) {
            PIDData pidData;
            if (decodePID(pid, raw, len, pidData)) {
                data[pidData.name + "_freeze"] = pidData;
            }
        }
    }

    return !data.empty();
}

bool OBD2::requestVIN(std::string& vin) {
    vin.clear();

    uint8_t response[8] = {0};
    size_t respLen = 0;

    // Request VIN (Mode 09, PID 02)
    if (!sendOBD2Request(0x09, 0x02, response, respLen)) {
        return false;
    }

    if (respLen < 4) return false;

    // VIN is ASCII in response[3..]
    for (size_t i = 3; i < respLen && response[i] != 0; ++i) {
        vin += static_cast<char>(response[i]);
    }

    return !vin.empty();
}

bool OBD2::requestECUInfo(ECUInfo& info) {
    info = ECUInfo();

    // VIN
    requestVIN(info.vin);

    // Calibration ID (Mode 09 PID 04)
    uint8_t raw[8] = {0};
    size_t len = 0;
    if (requestPIDEx(0x04, raw, sizeof(raw), len)) {
        for (size_t i = 3; i < len && raw[i] != 0; ++i) {
            info.calibrationId += static_cast<char>(raw[i]);
        }
    }

    // Software version (Mode 09 PID 00? No estándar. Placeholder.)
    info.softwareVersion = "";
    info.serialNumber = "";
    info.odometer = 0;

    return true;
}

} // namespace Scanner