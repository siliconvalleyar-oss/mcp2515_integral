#include "chevrolet.hpp"
#include <cstring>
#include <sstream>

namespace ecumult {

void ChevroletGM::handleCanFrame(const CANFrame& cf) {
    uint32_t id = cf.frame.can_id & 0x7FF;
    const auto& can_data = cf.frame.data;
    uint8_t dlc = cf.frame.can_dlc;
    std::vector<uint8_t> data(can_data, can_data + dlc);

    // Ignore own messages
    if (id == config_.rx_id) return;

    // Functional broadcast
    if (id == config_.functional_id) {
        respondToFunctionalBroadcast();
        return;
    }

    // Physical request to our TX ID
    if (id != config_.tx_id || dlc < 2) {
        // Check for GMLAN specific IDs
        if (id >= 0x600 && id <= 0x6FF) {
            // GMLAN device control messages
        }
        return;
    }

    uint8_t service = data[0];
    uint8_t sub_func = (dlc > 1) ? data[1] : 0;

    switch (service) {
        case 0x01: // Mode 01 OBD2
        {
            auto resp = handleMode01(sub_func);
            if (resp.size() > 0 && resp[0] != 0x7F) {
                sendResponse(config_.rx_id, resp);
            } else {
                sendNegativeResponse(0x01, 0x12);
            }
            break;
        }
        case 0x02: { // Mode 02
            auto resp = handleMode02(sub_func);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x03: { // Mode 03
            auto resp = handleMode03();
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x04: { // Mode 04
            auto resp = handleMode04();
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x05: { // Mode 05
            auto resp = handleMode05(sub_func);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x06: { // Mode 06
            auto resp = handleMode06(sub_func);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x07: { // Mode 07
            auto resp = handleMode07();
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x08: { // Mode 08
            std::vector<uint8_t> extra;
            if (dlc > 2) extra.assign(data.begin() + 2, data.begin() + dlc);
            auto resp = handleMode08(sub_func, extra);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x09: { // Mode 09
            auto resp = handleMode09(sub_func);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x0A: { // Mode 0A
            auto resp = handleMode0A();
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x22: { // GMLAN Mode 22 – Read DID
            if (dlc >= 3) {
                uint16_t did = (data[1] << 8) | data[2];
                std::vector<uint8_t> extra;
                if (dlc > 3) extra.assign(data.begin() + 3, data.begin() + dlc);
                auto resp = handleGMLANMode22(did, extra);
                sendResponse(config_.rx_id, resp);
            }
            break;
        }
        case 0x10: // UDS Diagnostic Session Control
        {
            auto resp = handleDiagnosticSessionControl(sub_func);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x27: // Security Access
        {
            std::vector<uint8_t> extra;
            if (dlc > 2) extra.assign(data.begin() + 2, data.begin() + dlc);
            auto resp = handleSecurityAccess(sub_func, extra);
            sendResponse(config_.rx_id, resp);
            break;
        }
        case 0x3E: // Tester Present
        {
            auto resp = handleTesterPresent();
            sendResponse(config_.rx_id, resp);
            break;
        }
        default:
            sendNegativeResponse(service, 0x11); // service not supported
            break;
    }
}

std::vector<uint8_t> ChevroletGM::handleGMLANMode22(uint16_t did, const std::vector<uint8_t>& data) {
    (void)data;
    return readGMDID(did);
}

std::vector<uint8_t> ChevroletGM::handleGMLANTesterPresent() {
    return handleTesterPresent();
}

std::vector<uint8_t> ChevroletGM::handleGMLANReportProgrammedState() {
    return {0x72, 0x00}; // programmed state response
}

std::vector<uint8_t> ChevroletGM::readGMDID(uint16_t did) {
    switch (did) {
        case 0x1A00: // VIN
        {
            std::vector<uint8_t> resp = {0x62, 0x1A, 0x00};
            for (char c : vin_) resp.push_back(c);
            return resp;
        }
        case 0x1A01: // Model Year
            return {0x62, 0x1A, 0x01, 0x1F}; // 2023
        case 0x1A02: // Engine Type
            return {0x62, 0x1A, 0x02, 0x06}; // V8
        case 0x1A0A: // Transmission Type
            return {0x62, 0x1A, 0x0A, 0x02}; // Automatic
        case 0xF000: // Boot Software Version
            return {0x62, 0xF0, 0x00, 'G', 'M', '_', 'B', 'O', 'O', 'T', '_', 'V', '1', '.', '2'};
        case 0xF001: // Calibration Version
            return {0x62, 0xF0, 0x01, 'G', 'M', '_', 'C', 'A', 'L', '_', 'V', '2', '.', '5'};
        case 0xF002: // Hardware Version
            return {0x62, 0xF0, 0x02, 'E', '7', '8', '_', 'R', 'e', 'v', ' ', 'C'};
        case 0xF180: // Vehicle Speed
            return {0x62, 0xF1, 0x80, static_cast<uint8_t>(vehicle_speed_)};
        case 0xF181: // Engine RPM
        {
            uint16_t rpm = static_cast<uint16_t>(engine_rpm_ * 4);
            return {0x62, 0xF1, 0x81,
                    static_cast<uint8_t>((rpm >> 8) & 0xFF),
                    static_cast<uint8_t>(rpm & 0xFF)};
        }
        case 0xF182: // Coolant Temp
            return {0x62, 0xF1, 0x82, static_cast<uint8_t>(coolant_temp_ + 40)};
        case 0xF190: // VIN (alternate)
        {
            std::vector<uint8_t> resp = {0x62, 0xF1, 0x90};
            for (char c : vin_) resp.push_back(c);
            return resp;
        }
        case 0xF191: // Odometer (km)
        {
            uint32_t odo = static_cast<uint32_t>(odometer_km_);
            return {0x62, 0xF1, 0x91,
                    static_cast<uint8_t>((odo >> 24) & 0xFF),
                    static_cast<uint8_t>((odo >> 16) & 0xFF),
                    static_cast<uint8_t>((odo >> 8) & 0xFF),
                    static_cast<uint8_t>(odo & 0xFF)};
        }
        case 0xF192: // Trip odometer (km)
        {
            uint16_t trip = static_cast<uint16_t>(trip_km_);
            return {0x62, 0xF1, 0x92,
                    static_cast<uint8_t>((trip >> 8) & 0xFF),
                    static_cast<uint8_t>(trip & 0xFF)};
        }
        case 0xF193: // Active profile name
        {
            auto* prof = profile_manager_.getCurrentProfile();
            std::string pname = prof ? prof->name : "normal";
            std::vector<uint8_t> resp = {0x62, 0xF1, 0x93};
            for (char c : pname) resp.push_back(c);
            return resp;
        }
        default:
        {
            // Check if DID corresponds to a DTC code lookup
            if ((did & 0xFF00) == 0x1200) {
                uint16_t dtc_code = did & 0x00FF;
                auto dtcs = getDTCs(true);
                for (const auto& dtc : dtcs) {
                    if (dtc.code == dtc_code) {
                        return {0x62, static_cast<uint8_t>((did >> 8) & 0xFF),
                                static_cast<uint8_t>(did & 0xFF), 0x01}; // fault active
                    }
                }
                return {0x62, static_cast<uint8_t>((did >> 8) & 0xFF),
                        static_cast<uint8_t>(did & 0xFF), 0x00}; // no fault
            }
            return {0x7F, 0x22, 0x12};
        }
    }
}

bool ChevroletGM::writeGMDID(uint16_t did, const std::vector<uint8_t>& data) {
    (void)did;
    (void)data;
    return false;
}

uint16_t ChevroletGM::dtcStringToCode(const std::string& code) {
    if (code.size() < 5) return 0;
    uint16_t result = 0;
    char prefix = code[0];
    switch (prefix) {
        case 'P': result = 0x0000; break;
        case 'C': result = 0x4000; break;
        case 'B': result = 0x8000; break;
        case 'U': result = 0xC000; break;
        default: return 0;
    }
    result |= std::stoi(code.substr(1, 1)) << 12;
    result |= std::stoi(code.substr(2, 3));
    return result;
}

std::string ChevroletGM::dtcCodeToString(uint16_t code) {
    std::string result;
    switch (code & 0xC000) {
        case 0x0000: result = "P"; break;
        case 0x4000: result = "C"; break;
        case 0x8000: result = "B"; break;
        case 0xC000: result = "U"; break;
    }
    result += std::to_string((code >> 12) & 0x0F);
    result += std::to_string((code >> 8) & 0x0F);
    result += std::to_string((code >> 4) & 0x0F);
    result += std::to_string(code & 0x0F);
    return result;
}

void ChevroletGM::respondToFunctionalBroadcast() {
    // Send an OBD2 readiness response on functional ID
    sendResponse(config_.rx_id, {0x41, 0x01, 0x00, 0x07, 0xFF, 0x00, 0x00});
}

void ChevroletGM::initGMPIDs() {
    initPIDs();
    // Add GM-specific PIDs
    addPID(0x22, "Fuel Rail Pressure (GM)", "kPa", 0, 5177, "((A*256)+B)*0.079");
    addPID(0x5A, "Accelerator Pedal Position (GM)", "%", 0, 100, "A*100/255");
}

} // namespace ecumult
