#include "mercedes.hpp"

namespace ecumult {

void Mercedes::handleCanFrame(const CANFrame& cf) {
    uint32_t id = cf.frame.can_id & 0x7FF;
    const auto& can_data = cf.frame.data;
    uint8_t dlc = cf.frame.can_dlc;
    std::vector<uint8_t> data(can_data, can_data + dlc);

    if (id == config_.rx_id) return;
    if (id != config_.tx_id || dlc < 2) return;

    uint8_t service = data[0];

    switch (service) {
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
        case 0x31: {
            if (dlc >= 4) {
                uint8_t rt = data[1];
                uint16_t rid = (data[2] << 8) | data[3];
                std::vector<uint8_t> rdata;
                if (dlc > 4) rdata.assign(data.begin() + 4, data.begin() + dlc);
                sendResponse(config_.rx_id, handleRoutineControl(rt, rid, rdata));
            }
            break;
        }
        case 0x3E: sendResponse(config_.rx_id, handleTesterPresent()); break;
        default: sendNegativeResponse(service, 0x11); break;
    }
}

std::vector<uint8_t> Mercedes::handleMercedesDAS(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x50, 0x01, 0x00, 0xFF}; // DAS (Drive Authorization System) OK
}

std::vector<uint8_t> Mercedes::handleMercedesSBC(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x50, 0x02, 0x00, 0x01}; // SBC (Sensotronic Brake Control) OK
}

std::vector<uint8_t> Mercedes::handleMercedesSAM(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x50, 0x03, 0x00, 0x00}; // SAM (Signal Acquisition Module) OK
}

void Mercedes::initMercedesPIDs() {
    setupDefaultPIDs();
    addPID(0x6A, "Airmatic Level (Mercedes)", "mm", -50, 50, "A-128");
    addPID(0x6B, "SBC Pressure (Mercedes)", "bar", 0, 200, "A*2");
}

} // namespace ecumult
