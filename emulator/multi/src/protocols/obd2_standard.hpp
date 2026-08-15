#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>

namespace ecumult {

class OBD2Standard {
public:
    struct PIDInfo {
        uint8_t pid;
        std::string name;
        std::string unit;
        uint8_t data_bytes;
        float min_val;
        float max_val;
        std::string formula;
    };

    static const std::unordered_map<uint8_t, PIDInfo>& getPIDs();
    static std::vector<uint8_t> supportedPIDs(uint8_t range);
    static std::vector<uint8_t> encodePID(uint8_t pid, float value);
    static float decodePID(uint8_t pid, const std::vector<uint8_t>& data);
    static std::vector<uint8_t> encodeDTC(uint16_t code);
    static uint16_t decodeDTC(const std::vector<uint8_t>& data, size_t offset = 0);
    static uint8_t calculateChecksum(const std::vector<uint8_t>& data);
    static bool verifyChecksum(const std::vector<uint8_t>& data);

private:
    static std::unordered_map<uint8_t, PIDInfo> initPIDs();
};

} // namespace ecumult
