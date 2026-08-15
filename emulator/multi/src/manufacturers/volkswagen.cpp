#include "volkswagen.hpp"

namespace ecumult {

void Volkswagen::handleCanFrame(const CANFrame& cf) {
    uint32_t id = cf.frame.can_id & 0x7FF;
    const auto& can_data = cf.frame.data;
    uint8_t dlc = cf.frame.can_dlc;
    std::vector<uint8_t> data(can_data, can_data + dlc);

    if (id == config_.rx_id) return;
    if (id != config_.tx_id || dlc < 2) return;

    uint8_t service = data[0];
    uint8_t sub_func = (dlc > 1) ? data[1] : 0;

    switch (service) {
        case 0x01: sendResponse(config_.rx_id, handleMode01(sub_func)); break;
        case 0x03: sendResponse(config_.rx_id, handleMode03()); break;
        case 0x04: sendResponse(config_.rx_id, handleMode04()); break;
        case 0x09: sendResponse(config_.rx_id, handleMode09(sub_func)); break;
        case 0x10: sendResponse(config_.rx_id, handleDiagnosticSessionControl(sub_func)); break;
        case 0x11: sendResponse(config_.rx_id, handleECUReset(sub_func)); break;
        case 0x22: {
            if (dlc >= 3) {
                uint16_t did = (data[1] << 8) | data[2];
                std::vector<uint8_t> extra;
                if (dlc > 3) extra.assign(data.begin() + 3, data.begin() + dlc);
                sendResponse(config_.rx_id, handleReadDataByIdentifier(did, extra));
            }
            break;
        }
        case 0x27: {
            std::vector<uint8_t> extra;
            if (dlc > 2) extra.assign(data.begin() + 2, data.begin() + dlc);
            sendResponse(config_.rx_id, handleSecurityAccess(sub_func, extra));
            break;
        }
        case 0x2E: {
            uint16_t did = (data[1] << 8) | data[2];
            std::vector<uint8_t> writeData;
            if (dlc > 3) writeData.assign(data.begin() + 3, data.begin() + dlc);
            sendResponse(config_.rx_id, handleWriteDataByIdentifier(did, writeData));
            break;
        }
        case 0x3E: sendResponse(config_.rx_id, handleTesterPresent()); break;
        default: sendNegativeResponse(service, 0x11); break;
    }
}

std::vector<uint8_t> Volkswagen::handleVWAdaptation(uint16_t channel, const std::vector<uint8_t>& data) {
    (void)data;
    switch (channel) {
        case 0x01: // Idle speed adaptation
            return {0x62, 0x01, 0x00, 0x58, 0x00}; // 880 RPM idle
        case 0x02: // Injection quantity
            return {0x62, 0x02, 0x00, 0x10, 0x00}; // 1.6 mg/stroke
        case 0x10: // Central locking
            return {0x62, 0x10, 0x00, 0x01}; // auto-lock enabled
        default:
            return {0x7F, 0x22, 0x12};
    }
}

std::vector<uint8_t> Volkswagen::handleVWLongCoding(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x6F, 0x2E, 0x00}; // long coding accepted
}

std::vector<uint8_t> Volkswagen::handleVWComponentProtection(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x50, 0x01, 0x00, 0x00}; // component protection: not active
}

void Volkswagen::initVWPIDs() {
    setupDefaultPIDs();
    addPID(0x68, "Boost Pressure (VW)", "mbar", 0, 4000, "((A*256)+B)*0.1");
    addPID(0x69, "Fuel Pressure Injector (VW)", "bar", 0, 200, "A*0.5");
}

} // namespace ecumult
