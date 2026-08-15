#include "ford.hpp"

namespace ecumult {

void Ford::handleCanFrame(const CANFrame& cf) {
    uint32_t id = cf.frame.can_id & 0x7FF;
    const auto& can_data = cf.frame.data;
    uint8_t dlc = cf.frame.can_dlc;
    std::vector<uint8_t> data(can_data, can_data + dlc);

    if (id == config_.rx_id) return;
    if (id != config_.tx_id || dlc < 2) {
        // Listen for Ford-specific module IDs
        if ((id & 0x700) == 0x700) return;
        return;
    }

    uint8_t service = data[0];
    uint8_t sub_func = data[1];

    switch (service) {
        case 0x01: case 0x02: case 0x03: case 0x04:
        case 0x05: case 0x06: case 0x07: case 0x08:
        case 0x09: case 0x0A:
        {
            auto resp = (service == 0x01) ? handleMode01(sub_func) :
                        (service == 0x03) ? handleMode03() :
                        handleMode01(sub_func);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x22: { // Ford enhanced diagnostics
            uint16_t pid = (data[1] << 8) | data[2];
            auto resp = handleFordEnhancedPIDs(pid);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x27: { // Security (PATS)
            std::vector<uint8_t> extra;
            if (dlc > 2) extra.assign(data.begin() + 2, data.begin() + dlc);
            auto resp = handleSecurityAccess(sub_func, extra);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x2E: { // Write Data By Identifier
            uint16_t did = (data[1] << 8) | data[2];
            std::vector<uint8_t> writeData;
            if (dlc > 3) writeData.assign(data.begin() + 3, data.begin() + dlc);
            auto resp = handleWriteDataByIdentifier(did, writeData);
            sendResponse(config_.rx_id, resp);
            break;
        }
        default:
            sendNegativeResponse(service, 0x11);
            break;
    }
}

std::vector<uint8_t> Ford::handleFordPATS(const std::vector<uint8_t>& data) {
    (void)data;
    // PATS (Passive Anti-Theft System) response
    return {0x50, 0x01, 0x00}; // PATS protocol version
}

std::vector<uint8_t> Ford::handleFordModuleConfig() {
    return {0x62, 0x02, 0x02, 0xFF}; // all modules present
}

std::vector<uint8_t> Ford::handleFordEnhancedPIDs(uint16_t pid) {
    switch (pid) {
        case 0x1001: // Ford engine type
            return {0x62, 0x10, 0x01, 0x04}; // Ecoboost V6
        case 0x1002: // Transmission type
            return {0x62, 0x10, 0x02, 0x01}; // 10R80
        case 0x2001: // Turbo boost pressure
            return {0x62, 0x20, 0x01, 0x1E}; // 30 PSI
        case 0x2002: // Intercooler temp
            return {0x62, 0x20, 0x02, static_cast<uint8_t>(intake_temp_ + 40)};
        case 0x3001: // PATS status
            return {0x62, 0x30, 0x01, 0x01}; // enabled
        default:
            return {0x7F, 0x22, 0x12};
    }
}

void Ford::setMS_CAN_Enabled(bool enabled) {
    ms_can_enabled_ = enabled;
}

void Ford::setHS_CAN_Enabled(bool enabled) {
    hs_can_enabled_ = enabled;
}

void Ford::initFordPIDs() {
    setupDefaultPIDs();
    addPID(0x63, "Turbo Boost Pressure (Ford)", "psi", 0, 60, "A*0.5");
    addPID(0x64, "Fuel Rail Temp (Ford)", "C", -40, 215, "A-40");
}

} // namespace ecumult
