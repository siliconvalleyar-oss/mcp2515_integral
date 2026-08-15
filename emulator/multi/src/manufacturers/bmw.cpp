#include "bmw.hpp"
#include <sstream>

namespace ecumult {

void BMW::handleCanFrame(const CANFrame& cf) {
    uint32_t id = cf.frame.can_id & 0x7FF;
    const auto& can_data = cf.frame.data;
    uint8_t dlc = cf.frame.can_dlc;
    std::vector<uint8_t> data(can_data, can_data + dlc);

    if (id == config_.rx_id) return;
    if (id != config_.tx_id || dlc < 2) return;

    uint8_t service = data[0];
    uint8_t sub_func = (dlc > 1) ? data[1] : 0;

    switch (service) {
        case 0x10: {
            sendResponse(config_.rx_id, handleDiagnosticSessionControl(sub_func));
            break;
        }
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
        case 0x31: { // Routine control
            if (dlc >= 4) {
                uint16_t routine_id = (data[2] << 8) | data[3];
                std::vector<uint8_t> routine_data;
                if (dlc > 4) routine_data.assign(data.begin() + 4, data.begin() + dlc);
                sendResponse(config_.rx_id, handleRoutineControl(sub_func, routine_id, routine_data));
            }
            break;
        }
        case 0x3E: {
            sendResponse(config_.rx_id, handleTesterPresent());
            break;
        }
        default:
            sendNegativeResponse(service, 0x11);
            break;
    }
}

std::vector<uint8_t> BMW::handleBMWDCAN(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x50, 0x01, 0x00};
}

std::vector<uint8_t> BMW::handleBMWJob(const std::string& job_name, const std::vector<uint8_t>& params) {
    (void)params;
    last_bmw_job_ = job_name;
    if (job_name == "STEUERNR_LESEN") {
        // Read control unit number
        return {0x62, 0xF0, 0x00, 'B', 'M', 'W', '_', 'D', 'M', 'E', '_', 'V', '1', '2', '3'};
    }
    if (job_name == "CODIERDATEN_LESEN") {
        // Read coding data
        return {0x62, 0xF0, 0x01, 0x01, 0x02, 0x03, 0x04};
    }
    if (job_name == "STATUS_LESEN") {
        return {0x62, 0xF0, 0x02, 0x00}; // status OK
    }
    return {0x7F, 0x31, 0x12};
}

std::vector<uint8_t> BMW::handleBMWEncoding(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x6F, 0x2E, 0x00}; // encoding accepted
}

std::vector<uint8_t> BMW::handleFRM(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x50, 0x01, 0x00, 0xFF}; // FRM (Footwell Module) OK
}

std::vector<uint8_t> BMW::handleCAS(const std::vector<uint8_t>& data) {
    (void)data;
    return {0x50, 0x02, 0x00, 0x01}; // CAS (Car Access System) OK
}

void BMW::initBMWPIDs() {
    setupDefaultPIDs();
    addPID(0x65, "Vanos Intake Position", "deg", 0, 360, "((A*256)+B)*0.5");
    addPID(0x66, "Vanos Exhaust Position", "deg", 0, 360, "((A*256)+B)*0.5");
    addPID(0x67, "Turbo Wastegate Position (BMW)", "%", 0, 100, "A*100/255");
}

} // namespace ecumult
