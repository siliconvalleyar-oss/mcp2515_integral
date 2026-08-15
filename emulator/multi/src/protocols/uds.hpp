#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <functional>

namespace ecumult {

class UDSProtocol {
public:
    enum Service : uint8_t {
        SESSION_CONTROL      = 0x10,
        ECU_RESET            = 0x11,
        SECURITY_ACCESS      = 0x27,
        COMM_CONTROL         = 0x28,
        TESTER_PRESENT       = 0x3E,
        ACCESS_TIMING        = 0x83,
        SECURED_DATA_TRANS   = 0x84,
        CONTROL_DTC_SETTING  = 0x85,
        RESPONSE_ON_EVENT    = 0x86,
        LINK_CONTROL         = 0x87,
        READ_DATA_BY_ID      = 0x22,
        READ_MEM_BY_ADDR     = 0x23,
        READ_SCALING_DATA    = 0x24,
        READ_DATA_PERIODIC   = 0x2A,
        READ_DTC_INFO        = 0x19,
        WRITE_DATA_BY_ID     = 0x2E,
        WRITE_MEM_BY_ADDR    = 0x3D,
        ROUTINE_CONTROL      = 0x31,
        REQUEST_DOWNLOAD     = 0x34,
        REQUEST_UPLOAD       = 0x35,
        TRANSFER_DATA        = 0x36,
        REQUEST_TRANSFER_EXIT = 0x37,
        WRITE_DTC            = 0x14,
        IOCONTROL_BY_ID      = 0x2F,
        INPUT_OUTPUT_CONTROL = 0x30
    };

    enum NRCode : uint8_t {
        NRC_GENERAL_REJECT            = 0x10,
        NRC_SERVICE_NOT_SUPPORTED     = 0x11,
        NRC_SUBFUNC_NOT_SUPPORTED     = 0x12,
        NRC_INVALID_FORMAT            = 0x13,
        NRC_BUSY                      = 0x21,
        NRC_CONDITIONS_NOT_CORRECT    = 0x22,
        NRC_REQUEST_SEQUENCE_ERROR    = 0x24,
        NRC_NO_RESPONSE               = 0x25,
        NRC_SECURITY_ACCESS_DENIED    = 0x33,
        NRC_INVALID_KEY               = 0x35,
        NRC_EXCEEDED_NUM_ATTEMPTS     = 0x36,
        NRC_REQUIRED_TIME_DELAY       = 0x37,
        NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED = 0x70,
        NRC_TRANSFER_DATA_SUSPENDED   = 0x71,
        NRC_GENERAL_PROGRAMMING_FAILURE = 0x72,
        NRC_WRONG_BLOCK_SEQ_NUM       = 0x73,
        NRC_REQUEST_CORRECTLY_RECEIVED = 0x00,
        NRC_PENDING                   = 0x78
    };

    enum SessionType : uint8_t {
        DEFAULT_SESSION      = 0x01,
        PROGRAMMING_SESSION  = 0x02,
        EXTENDED_SESSION     = 0x03,
        SAFETY_SESSION       = 0x04
    };

    enum ResetType : uint8_t {
        HARD_RESET           = 0x01,
        KEY_OFF_ON_RESET     = 0x02,
        SOFT_RESET           = 0x03,
        ENABLE_RAPID_SHUTDOWN = 0x04,
        DISABLE_RAPID_SHUTDOWN = 0x05
    };

    struct UDSResponse {
        uint8_t service_id;
        std::vector<uint8_t> data;
        bool positive;
    };

    static std::vector<uint8_t> buildPositiveResponse(uint8_t service, const std::vector<uint8_t>& data = {});
    static std::vector<uint8_t> buildNegativeResponse(uint8_t service, uint8_t nrc);
    static bool isPositiveResponse(uint8_t response_byte);
    static bool isNegativeResponse(uint8_t response_byte);
    static uint8_t getServiceFromResponse(uint8_t response_byte);

    // DID ranges
    static bool isStandardDID(uint16_t did);
    static bool isManufacturerDID(uint16_t did);
    static bool isOBDDID(uint16_t did);

private:
    static const std::unordered_map<uint8_t, std::string> initServiceNames();
};

} // namespace ecumult
