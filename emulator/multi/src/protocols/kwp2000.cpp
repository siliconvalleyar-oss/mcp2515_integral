#include "kwp2000.hpp"

namespace ecumult {

std::vector<uint8_t> KWP2000Protocol::buildKWPResponse(uint8_t service, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> msg;
    msg.push_back(0x80 | (service + 0x40)); // positive response
    msg.insert(msg.end(), data.begin(), data.end());
    msg.push_back(calculateChecksum(msg));
    return msg;
}

bool KWP2000Protocol::validateKWPFrame(const std::vector<uint8_t>& frame) {
    if (frame.size() < 3) return false;
    return calculateChecksum(frame) == 0;
}

uint8_t KWP2000Protocol::calculateChecksum(const std::vector<uint8_t>& data) {
    uint8_t cs = 0;
    for (auto b : data) cs += b;
    return cs & 0xFF;
}

std::vector<uint8_t> KWP2000Protocol::buildKWPMessage(uint8_t target, uint8_t source,
                                                         uint8_t service, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> msg;
    msg.push_back(target);   // target address
    msg.push_back(source);   // source address
    msg.push_back(service);  // service ID
    msg.insert(msg.end(), data.begin(), data.end());
    msg.push_back(calculateChecksum(msg));
    return msg;
}

} // namespace ecumult
