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

bool OBD2::sendRequestFrame(uint8_t mode, uint8_t pid) {
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

    return can_->sendMessage(msg);
}

bool OBD2::sendOBD2Request(uint8_t mode, uint8_t pid, uint8_t* response, size_t& length) {
    if (!sendRequestFrame(mode, pid)) {
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

bool OBD2::receiveISO15765(uint8_t* buffer, size_t bufferSize, size_t& outLen, uint32_t timeoutMs) {
    if (!initialized_ || !can_ || !buffer || bufferSize == 0) return false;

    auto start = std::chrono::steady_clock::now();
    bool haveFirst = false;
    size_t totalLen = 0;
    size_t received = 0;
    uint8_t expectedSeq = 1;

    auto elapsed = [&]() {
        auto now = std::chrono::steady_clock::now();
        return static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());
    };

    while (true) {
        Hardware::CANMessage msg{};
        if (can_->receiveMessage(msg)) {
            if (msg.id != responseId_ && msg.id != responseId_ + 1 && msg.id != responseId_ + 2) {
                continue;
            }
            if (msg.dlc == 0) continue;

            uint8_t pci = msg.data[0];
            uint8_t pciType = pci & 0xF0;

            if (!haveFirst) {
                if (pciType == 0x00) {
                    // Single Frame (PCI = length, 1-7 bytes)
                    size_t sfLen = std::min<size_t>(pci & 0x0F, msg.dlc - 1);
                    sfLen = std::min(sfLen, bufferSize);
                    memcpy(buffer, &msg.data[1], sfLen);
                    outLen = sfLen;
                    return true;
                }
                if (pciType == 0x10) {
                    // First Frame: PCI 0x10 | len high, byte[1] = len low, 6 bytes data
                    totalLen = (static_cast<size_t>(pci & 0x0F) << 8) | msg.data[1];
                    if (totalLen == 0 || totalLen > bufferSize) {
                        outLen = 0;
                        return false;
                    }
                    size_t ffLen = std::min<size_t>(msg.dlc - 2, 6);
                    ffLen = std::min(ffLen, totalLen);
                    memcpy(buffer, &msg.data[2], ffLen);
                    received = ffLen;
                    haveFirst = true;
                    expectedSeq = 1;

                    // Flow Control: indicar al ECU que envíe todos los CF sin pausa
                    sendFlowControl();

                    if (received >= totalLen) {
                        outLen = received;
                        return true;
                    }
                    continue;
                }
                // Ignorar tramas que no son SF/FF como primer frame
                continue;
            }

            // Modo CF: PCI 0x20 | secuencia, 7 bytes de datos
            if (pciType == 0x20) {
                uint8_t seq = pci & 0x0F;
                if (seq != expectedSeq) {
                    outLen = 0;  // Frame perdido
                    return false;
                }
                size_t cfLen = std::min<size_t>(msg.dlc - 1, 7);
                cfLen = std::min(cfLen, totalLen - received);
                memcpy(buffer + received, &msg.data[1], cfLen);
                received += cfLen;
                expectedSeq = static_cast<uint8_t>((expectedSeq + 1) & 0x0F);
                if (received >= totalLen) {
                    outLen = received;
                    return true;
                }
            }
        }

        if (elapsed() >= timeoutMs) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    outLen = received;
    return haveFirst && received == totalLen;
}

void OBD2::sendFlowControl(uint8_t bs, uint8_t stmin) {
    if (!can_) return;

    Hardware::CANMessage fc{};
    fc.id = requestId_ + 1;  // 0x7E0: ID físico del tester
    fc.extended = false;
    fc.dlc = 8;
    fc.data[0] = 0x30;  // PCI Flow Control
    fc.data[1] = bs;    // Block Size (0 = enviar todos)
    fc.data[2] = stmin; // Separation Time (0 = sin pausa)
    fc.data[3] = 0x00;
    fc.data[4] = 0x00;
    fc.data[5] = 0x00;
    fc.data[6] = 0x00;
    fc.data[7] = 0x00;

    can_->sendMessage(fc);
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

    uint8_t buffer[64] = {0};
    size_t outLen = 0;

    // Request VIN (Mode 09, PID 02). La respuesta es multi-frame ISO-TP:
    // FF + FC + CF. El payload es [49][02][count][VIN 17 bytes].
    if (!sendRequestFrame(0x09, 0x02)) {
        return false;
    }
    if (!receiveISO15765(buffer, sizeof(buffer), outLen)) {
        return false;
    }
    if (outLen < 3) return false;

    // Localizar el header "49 02" dentro del payload.
    size_t i = 0;
    while (i + 1 < outLen && !(buffer[i] == 0x49 && buffer[i + 1] == 0x02)) {
        ++i;
    }
    if (i + 1 >= outLen) return false;
    i += 2;

    // Algunos ECUs incluyen un byte de conteo (no imprimible) antes del VIN.
    if (i < outLen && buffer[i] < 0x20) {
        ++i;
    }

    while (i < outLen && vin.size() < 17) {
        char c = static_cast<char>(buffer[i]);
        if (c == 0) break;
        vin += c;
        ++i;
    }

    if (vin.size() < 11) {  // VIN válido mínimo
        vin.clear();
        return false;
    }
    return true;
}

bool OBD2::requestECUInfo(ECUInfo& info) {
    info = ECUInfo();

    // VIN
    requestVIN(info.vin);

    // Calibration ID (Mode 09 PID 04) - multi-frame ISO-TP
    uint8_t buffer[64] = {0};
    size_t outLen = 0;
    if (sendRequestFrame(0x09, 0x04) && receiveISO15765(buffer, sizeof(buffer), outLen)) {
        size_t i = 0;
        while (i + 1 < outLen && !(buffer[i] == 0x49 && buffer[i + 1] == 0x04)) {
            ++i;
        }
        if (i + 1 < outLen) {
            i += 2;
            if (i < outLen && buffer[i] < 0x20) {
                ++i;
            }
            for (; i < outLen && buffer[i] != 0; ++i) {
                info.calibrationId += static_cast<char>(buffer[i]);
            }
        }
    }

    // Software version (Mode 09 PID 00? No estándar. Placeholder.)
    info.softwareVersion = "";
    info.serialNumber = "";
    info.odometer = 0;

    return true;
}

} // namespace Scanner