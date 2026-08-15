#pragma once

#include <cstdint>
#include <vector>

namespace ecumult {

class KWP2000Protocol {
public:
    static constexpr uint8_t START_COMM = 0x81;
    static constexpr uint8_t STOP_COMM = 0x82;
    static constexpr uint8_t TIMING_PACKET = 0x83;

    static std::vector<uint8_t> buildKWPResponse(uint8_t service, const std::vector<uint8_t>& data);
    static bool validateKWPFrame(const std::vector<uint8_t>& frame);
    static uint8_t calculateChecksum(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> buildKWPMessage(uint8_t target, uint8_t source,
                                                  uint8_t service, const std::vector<uint8_t>& data);
};

} // namespace ecumult
