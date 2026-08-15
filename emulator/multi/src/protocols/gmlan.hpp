#pragma once

#include <cstdint>
#include <vector>

namespace ecumult {

class GMLANProtocol {
public:
    enum Service : uint8_t {
        DIAGNOSTIC_SESSION  = 0x10,
        ECU_RESET           = 0x11,
        READ_DID            = 0x1A,   // GM-specific
        READ_DID_EXTENDED   = 0x22,
        WRITE_DID           = 0x3B,   // GM-specific
        TESTER_PRESENT      = 0x3E,
        REPORT_PROGRAMMED   = 0xA2,
        READ_DATA_BY_ID     = 0x22,
        SECURITY_ACCESS     = 0x27
    };

    enum DID : uint16_t {
        VIN               = 0x1A00,
        MODEL_YEAR        = 0x1A01,
        ENGINE_TYPE       = 0x1A02,
        TRANSMISSION_TYPE = 0x1A0A,
        SOFTWARE_VERSION  = 0xF000,
        CALIBRATION_VER   = 0xF001,
        HARDWARE_VER      = 0xF002,
        VEHICLE_SPEED     = 0xF180,
        ENGINE_RPM        = 0xF181,
        COOLANT_TEMP      = 0xF182,
        BATTERY_VOLTAGE   = 0xF183,
        FUEL_LEVEL        = 0xF184,
        AMBIENT_TEMP      = 0xF185,
        VIN_ALT           = 0xF190
    };

    static std::vector<uint8_t> buildReadDIDResponse(uint16_t did, const std::vector<uint8_t>& data);
    static uint16_t getTXIDForFunction(uint8_t function);
    static uint8_t calculateChecksum(const std::vector<uint8_t>& data);

    // GM-specific DTC formats
    static uint16_t encodeGMDTC(const std::string& code);
    static std::string decodeGMDTC(uint16_t code);
};

} // namespace ecumult
