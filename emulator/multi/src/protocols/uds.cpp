#include "uds.hpp"

namespace ecumult {

std::vector<uint8_t> UDSProtocol::buildPositiveResponse(uint8_t service, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> resp;
    resp.push_back(service + 0x40); // positive response SID
    resp.insert(resp.end(), data.begin(), data.end());
    return resp;
}

std::vector<uint8_t> UDSProtocol::buildNegativeResponse(uint8_t service, uint8_t nrc) {
    return {0x7F, service, nrc};
}

bool UDSProtocol::isPositiveResponse(uint8_t response_byte) {
    return (response_byte & 0xC0) == 0x40;
}

bool UDSProtocol::isNegativeResponse(uint8_t response_byte) {
    return response_byte == 0x7F;
}

uint8_t UDSProtocol::getServiceFromResponse(uint8_t response_byte) {
    if (isPositiveResponse(response_byte)) return response_byte - 0x40;
    return 0xFF;
}

bool UDSProtocol::isStandardDID(uint16_t did) {
    return (did >= 0xF000 && did <= 0xF1FF);
}

bool UDSProtocol::isManufacturerDID(uint16_t did) {
    return ((did >= 0x1000 && did <= 0x1FFF) ||
            (did >= 0x2000 && did <= 0x2FFF));
}

bool UDSProtocol::isOBDDID(uint16_t did) {
    return (did >= 0xF180 && did <= 0xF1BF);
}

} // namespace ecumult
