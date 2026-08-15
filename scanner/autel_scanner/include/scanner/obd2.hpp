#ifndef SCANNER_OBD2_HPP
#define SCANNER_OBD2_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include "hardware/mcp2515.hpp"

namespace Scanner {

struct PIDData {
    std::string name;
    std::string unit;
    float value;
    float min;
    float max;
};

struct ECUInfo {
    std::string vin;
    std::string calibrationId;
    std::string serialNumber;
    float odometer;
    std::string softwareVersion;
};

class OBD2 {
public:
    OBD2(std::shared_ptr<Hardware::MCP2515> canInterface);
    ~OBD2();

    bool initialize();
    void cleanup();

    // OBD-II Modes
    bool requestDTCs(std::vector<std::string>& dtcs);
    bool clearDTCs();
    bool requestLiveData(std::unordered_map<std::string, PIDData>& data);
    bool requestFreezeFrame(uint32_t dtc, std::unordered_map<std::string, PIDData>& data);

    // PIDs
    bool requestPID(uint8_t pid, uint8_t* data, size_t length);
    bool requestPIDEx(uint8_t pid, uint8_t* buffer, size_t bufferSize, size_t& outLen);
    bool requestCustomList(const std::vector<uint8_t>& pids, std::unordered_map<std::string, PIDData>& data);
    bool decodePID(uint8_t pid, const uint8_t* data, size_t len, PIDData& out);
    float calculateMAF(uint8_t a, uint8_t b);
    float calculateMAP(uint8_t a, uint8_t b);
    float calculateRPM(uint8_t a, uint8_t b);
    float calculateSpeed(uint8_t a);
    float calculateThrottle(uint8_t a);
    float calculateFuelTrim(uint8_t a);
    float calculateLoad(uint8_t a);
    float calculateTemp(uint8_t a);

    // VIN
    bool requestVIN(std::string& vin);

    // ECU Information
    bool requestECUInfo(ECUInfo& info);

    // Service / Maintenance
    bool resetAdaptations();
    bool resetFuelTrim();
    bool programFuelComposition(uint8_t ethanolPercent);
    bool resetImmobilizer();

public:
    bool sendOBD2Request(uint8_t mode, uint8_t pid, uint8_t* response, size_t& length);
    bool waitForResponse(uint8_t* response, size_t& length, uint32_t timeoutMs = 1000);

private:
    bool sendRequestFrame(uint8_t mode, uint8_t pid);
    bool receiveISO15765(uint8_t* buffer, size_t bufferSize, size_t& outLen,
                         uint32_t timeoutMs = 2000);
    void sendFlowControl(uint8_t bs = 0, uint8_t stmin = 0);

    std::shared_ptr<Hardware::MCP2515> can_;
    bool initialized_;
    uint32_t requestId_;
    uint32_t responseId_;
};

} // namespace Scanner

#endif // SCANNER_OBD2_HPP
