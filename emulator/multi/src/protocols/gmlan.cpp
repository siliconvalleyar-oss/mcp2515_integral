#include "gmlan.hpp"
#include <algorithm>

namespace ecumult {

std::vector<uint8_t> GMLANProtocol::buildReadDIDResponse(uint16_t did, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> resp;
    resp.push_back(0x62); // positive response to 0x22
    resp.push_back((did >> 8) & 0xFF);
    resp.push_back(did & 0xFF);
    resp.insert(resp.end(), data.begin(), data.end());
    return resp;
}

uint16_t GMLANProtocol::getTXIDForFunction(uint8_t function) {
    (void)function;
    return 0x7E0;
}

uint8_t GMLANProtocol::calculateChecksum(const std::vector<uint8_t>& data) {
    uint8_t cs = 0;
    for (auto b : data) cs ^= b;
    return cs ^ 0xFF;
}

uint16_t GMLANProtocol::encodeGMDTC(const std::string& code) {
    if (code.size() < 5) return 0;
    uint16_t result = 0;
    switch (code[0]) {
        case 'P': result = 0x0000; break;
        case 'C': result = 0x4000; break;
        case 'B': result = 0x8000; break;
        case 'U': result = 0xC000; break;
        default: return 0;
    }
    result |= std::stoi(code.substr(1, 1)) << 12;
    result |= std::atoi(code.substr(2).c_str()) & 0x0FFF;
    return result;
}

std::string GMLANProtocol::decodeGMDTC(uint16_t code) {
    std::string result;
    switch (code & 0xC000) {
        case 0x0000: result = "P"; break;
        case 0x4000: result = "C"; break;
        case 0x8000: result = "B"; break;
        case 0xC000: result = "U"; break;
    }
    result += std::to_string((code >> 12) & 0x0F);
    result += std::to_string((code >> 8) & 0x0F);
    result += std::to_string((code >> 4) & 0x0F);
    result += std::to_string(code & 0x0F);
    return result;
}

} // namespace ecumult
