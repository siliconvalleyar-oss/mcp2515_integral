#include "honda.hpp"

namespace ecumult {

void Honda::handleCanFrame(const CANFrame& cf) {
    uint32_t id = cf.frame.can_id & 0x7FF;
    const auto& can_data = cf.frame.data;
    uint8_t dlc = cf.frame.can_dlc;
    std::vector<uint8_t> data(can_data, can_data + dlc);

    if (id == config_.rx_id) return;
    if (id != config_.tx_id || dlc < 2) return;

    uint8_t service = data[0];

    switch (service) {
        case 0x01: sendResponse(config_.rx_id, handleMode01((dlc > 1) ? data[1] : 0)); break;
        case 0x03: sendResponse(config_.rx_id, handleMode03()); break;
        case 0x04: sendResponse(config_.rx_id, handleMode04()); break;
        case 0x09: sendResponse(config_.rx_id, handleMode09((dlc > 1) ? data[1] : 0)); break;
        case 0x10: sendResponse(config_.rx_id, handleDiagnosticSessionControl((dlc > 1) ? data[1] : 0)); break;
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
            uint8_t level = (dlc > 1) ? data[1] : 0;
            std::vector<uint8_t> extra;
            if (dlc > 2) extra.assign(data.begin() + 2, data.begin() + dlc);
            sendResponse(config_.rx_id, handleSecurityAccess(level, extra));
            break;
        }
        case 0x3E: sendResponse(config_.rx_id, handleTesterPresent()); break;
        default: sendNegativeResponse(service, 0x11); break;
    }
}

std::vector<uint8_t> Honda::handleHondaHDS(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x50, 0x01, 0x01}; // HDS connected
}

std::vector<uint8_t> Honda::handleHondaImmobilizer(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x50, 0x02, 0x00}; // Immobilizer OK
}

std::vector<uint8_t> Honda::handleHondaVTEC(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x62, 0x01, 0x00, 0x01}; // VTEC active
}

void Honda::initHondaPIDs() {
    setupDefaultPIDs();
    addPID(0x6C, "VTEC Pressure (Honda)", "psi", 0, 100, "A*0.5");
    addPID(0x6D, "i-VTEC Phase (Honda)", "deg", 0, 50, "A*0.5");
}

} // namespace ecumult
