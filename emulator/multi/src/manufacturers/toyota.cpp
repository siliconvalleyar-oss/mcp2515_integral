#include "toyota.hpp"

namespace ecumult {

void Toyota::handleCanFrame(const CANFrame& cf) {
    uint32_t id = cf.frame.can_id & 0x7FF;
    const auto& can_data = cf.frame.data;
    uint8_t dlc = cf.frame.can_dlc;
    std::vector<uint8_t> data(can_data, can_data + dlc);

    if (id == config_.rx_id) return;
    if (id != config_.tx_id || dlc < 2) {
        // Toyota BCM/ABS/SRS IDs
        if (id >= 0x700 && id <= 0x7FF && id != config_.tx_id && id != config_.functional_id) {
            // Other ECU broadcasts
        }
        return;
    }

    uint8_t service = data[0];
    uint8_t sub_func = data[1];

    switch (service) {
        case 0x01: {
            sendResponse(config_.rx_id, handleMode01(sub_func));
            break;
        }
        case 0x03: {
            sendResponse(config_.rx_id, handleMode03());
            break;
        }
        case 0x04: {
            sendResponse(config_.rx_id, handleMode04());
            break;
        }
        case 0x07: {
            sendResponse(config_.rx_id, handleMode07());
            break;
        }
        case 0x09: {
            sendResponse(config_.rx_id, handleMode09(sub_func));
            break;
        }
        case 0x15: { // Toyota Mode 21
            auto resp = handleMode21(sub_func);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x16: { // Toyota Mode 22
            auto resp = handleMode22(sub_func);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x27: {
            std::vector<uint8_t> extra;
            if (dlc > 2) extra.assign(data.begin() + 2, data.begin() + dlc);
            sendResponse(config_.rx_id, handleSecurityAccess(sub_func, extra));
            break;
        }
        default:
            sendNegativeResponse(service, 0x11);
            break;
    }
}

std::vector<uint8_t> Toyota::handleMode21(uint8_t pid) {
    switch (pid) {
        case 0x01: // Engine type
            return {0x61, 0x21, 0x01, 0x03}; // 2GR-FE V6
        case 0x02: // Transmission type
            return {0x61, 0x21, 0x02, 0x01}; // 6-speed auto
        case 0x03: // Drive type
            return {0x61, 0x21, 0x03, 0x02}; // FWD
        case 0x04: // Fuel system
            return {0x61, 0x21, 0x04, 0x01}; // Port injection
        default:
            return {0x7F, 0x15, 0x12};
    }
}

std::vector<uint8_t> Toyota::handleMode22(uint8_t pid) {
    (void)pid;
    return {0x62, 0x22, pid, 0x00};
}

std::vector<uint8_t> Toyota::handleSmartKey(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x50, 0x01, 0x00, 0x01}; // Smart key system OK
}

std::vector<uint8_t> Toyota::handleImmobilizer(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x50, 0x02, 0x00}; // Immobilizer OK
}

uint8_t Toyota::toyota_checksum(const std::vector<uint8_t>& data) {
    uint8_t cs = 0;
    for (auto b : data) cs ^= b;
    return cs;
}

void Toyota::initToyotaPIDs() {
    setupDefaultPIDs();
    addPID(0x60, "Toyota Engine Load", "%", 0, 100, "A*100/255");
    addPID(0x61, "Hybrid Battery SOC", "%", 0, 100, "A*100/255");
}

} // namespace ecumult
