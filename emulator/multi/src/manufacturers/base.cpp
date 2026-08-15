#include "base.hpp"
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace ecumult {

BaseManufacturer::BaseManufacturer(std::shared_ptr<CANManager> can_manager)
    : can_manager_(std::move(can_manager))
    , engine_rpm_(0)
    , vehicle_speed_(0)
    , coolant_temp_(95)
    , intake_temp_(35)
    , maf_(3.5f)
    , throttle_pos_(0)
    , fuel_pressure_(350)
    , fuel_level_(50)
    , timing_advance_(10)
    , intake_pressure_(101)
    , battery_voltage_(12.6f)
    , run_time_(0)
    , distance_with_mil_(0)
    , distance_since_clear_(0)
    , odometer_km_(0)
    , trip_km_(0)
    , ecu_serial_(0x12345678) {
    std::fill(o2_voltage_, o2_voltage_ + 8, 0.45f);
    setupDefaultPIDs();
    vin_ = "XXXXXXXXXXXXXXXXX";
    applyProfileDTCs();
}

void BaseManufacturer::onManufacturerSelected(const ManufacturerConfig& cfg) {
    config_ = cfg;
    std::cout << "[Base] Selected manufacturer: " << cfg.name << std::endl;
}

void BaseManufacturer::onManufacturerDeselected() {
    std::cout << "[Base] Deselected manufacturer: " << config_.name << std::endl;
}

void BaseManufacturer::reset() {
    dtcs_.clear();
    freeze_frames_.clear();
    session_manager_.lock();
    session_manager_.changeSession(UDSSession::DEFAULT);
    engine_rpm_ = 0;
    vehicle_speed_ = 0;
    coolant_temp_ = 95;
    battery_voltage_ = 12.6f;
}

// ===== OBD2 Standard Modes =====

std::vector<uint8_t> BaseManufacturer::handleMode01(uint8_t pid) {
    auto* entry = getPIDEntry(pid);
    if (!entry) {
        return {0x7F, 0x01, 0x12}; // NRC: subfunction not supported
    }

    float value;
    switch (pid) {
        case 0x0C: value = profile_manager_.applySensorOverride(0x0C, engine_rpm_, 0); break;
        case 0x0D: value = profile_manager_.applySensorOverride(0x0D, vehicle_speed_, 0); break;
        case 0x05: value = profile_manager_.applySensorOverride(0x05, coolant_temp_, 0); break;
        case 0x0F: value = profile_manager_.applySensorOverride(0x0F, intake_temp_, 0); break;
        case 0x10: value = profile_manager_.applySensorOverride(0x10, maf_, 0); break;
        case 0x11: value = profile_manager_.applySensorOverride(0x11, throttle_pos_, 0); break;
        case 0x14: value = profile_manager_.applySensorOverride(0x14, o2_voltage_[0], 0); break;
        case 0x15: value = profile_manager_.applySensorOverride(0x15, o2_voltage_[1], 0); break;
        case 0x22: value = profile_manager_.applySensorOverride(0x22, fuel_pressure_, 0); break;
        case 0x2F: value = profile_manager_.applySensorOverride(0x2F, fuel_level_, 0); break;
        case 0x04: value = profile_manager_.applySensorOverride(0x04, timing_advance_, 0); break;
        case 0x0B: value = profile_manager_.applySensorOverride(0x0B, intake_pressure_, 0); break;
        case 0x42: value = profile_manager_.applySensorOverride(0x42, battery_voltage_, 0); break;
        case 0x1F: value = run_time_; break;
        case 0x21: value = distance_with_mil_; break;
        case 0x31: value = distance_since_clear_; break;
        default: value = 0;
    }
    return encodePIDResponse(pid, value);
}

std::vector<uint8_t> BaseManufacturer::handleMode02(uint8_t pid) {
    // freeze frame data
    if (freeze_frames_.empty()) {
        return {0x7F, 0x02, 0x31}; // NRC: request out of range
    }
    return handleMode01(pid);
}

std::vector<uint8_t> BaseManufacturer::handleMode03() {
    std::vector<uint8_t> result;
    auto active = getDTCs(true);
    size_t count = std::min(active.size(), size_t(255));
    result.push_back(static_cast<uint8_t>(count));
    for (size_t i = 0; i < count; i++) {
        result.push_back((active[i].code >> 8) & 0xFF);
        result.push_back(active[i].code & 0xFF);
    }
    if (result.empty()) {
        result = {0x00}; // no DTCs
    }
    return result;
}

std::vector<uint8_t> BaseManufacturer::handleMode04() {
    clearDTCs();
    return {0x04, 0x00}; // success
}

std::vector<uint8_t> BaseManufacturer::handleMode05(uint8_t test_id) {
    (void)test_id;
    // O2 sensor monitoring
    return {0x05, 0x00};
}

std::vector<uint8_t> BaseManufacturer::handleMode06(uint8_t monitor_id) {
    (void)monitor_id;
    return {0x06, 0x00};
}

std::vector<uint8_t> BaseManufacturer::handleMode07() {
    // pending DTCs
    return handleMode03();
}

std::vector<uint8_t> BaseManufacturer::handleMode08(uint8_t test_id, const std::vector<uint8_t>& data) {
    (void)test_id;
    (void)data;
    return {0x08, 0x00};
}

std::vector<uint8_t> BaseManufacturer::handleMode09(uint8_t info_type) {
    switch (info_type) {
        case 0x00: // supported info types
            return {0x09, 0x00, 0x01, 0x02, 0x04, 0x05, 0x06, 0x08, 0x0A, 0x0B};
        case 0x01: // VIN
        {
            std::vector<uint8_t> resp = {0x09, 0x01};
            std::string vin = getVIN();
            for (char c : vin) resp.push_back(c);
            return resp;
        }
        case 0x02: // ECU name
            return {0x09, 0x02, 'E', 'C', 'U', ' ', 'E', 'm', 'u', 'l', 'a', 't', 'o', 'r', ' ', 'v', '1', '.', '0'};
        case 0x04: // calibration ID
        {
            auto cal = getCalibrationInfo();
            std::vector<uint8_t> resp = {0x09, 0x04};
            for (char c : cal.calibration_id) resp.push_back(c);
            return resp;
        }
        case 0x05: // CVN
        {
            auto cal = getCalibrationInfo();
            return {0x09, 0x05,
                    static_cast<uint8_t>((cal.cvn >> 24) & 0xFF),
                    static_cast<uint8_t>((cal.cvn >> 16) & 0xFF),
                    static_cast<uint8_t>((cal.cvn >> 8) & 0xFF),
                    static_cast<uint8_t>(cal.cvn & 0xFF)};
        }
        case 0x06: // in-use performance tracking
            return {0x09, 0x06};
        case 0x08: // ECU serial
        {
            uint32_t sn = getECUSerialNumber();
            return {0x09, 0x08,
                    static_cast<uint8_t>((sn >> 24) & 0xFF),
                    static_cast<uint8_t>((sn >> 16) & 0xFF),
                    static_cast<uint8_t>((sn >> 8) & 0xFF),
                    static_cast<uint8_t>(sn & 0xFF)};
        }
        case 0x0A: // software version
            return {0x09, 0x0A, '1', '.', '0', '.', '0'};
        case 0x0B: // extended VIN
            return {0x09, 0x0B};
        default:
            return {0x7F, 0x09, 0x12};
    }
}

std::vector<uint8_t> BaseManufacturer::handleMode0A() {
    // permanent DTCs
    return handleMode03();
}

// ===== UDS Services =====

std::vector<uint8_t> BaseManufacturer::handleDiagnosticSessionControl(uint8_t session_type) {
    auto session = static_cast<UDSSession>(session_type);
    if (session_manager_.changeSession(session)) {
        return {0x50, session_type, 0x00, 0x00, 0x00}; // session + timing
    }
    return {0x7F, 0x10, 0x12};
}

std::vector<uint8_t> BaseManufacturer::handleECUReset(uint8_t reset_type) {
    (void)reset_type;
    return {0x51, reset_type};
}

std::vector<uint8_t> BaseManufacturer::handleSecurityAccess(uint8_t level, const std::vector<uint8_t>& data) {
    if ((level & 0x01) == 0x01) {
        // Request seed (odd level)
        if (session_manager_.requestSeed(level)) {
            // seed would be stored in session manager
            return {0x67, level}; // + seed bytes (simplified)
        }
        return {0x7F, 0x27, 0x12};
    } else {
        // Send key (even level = level+1)
        if (session_manager_.sendKey(level | 0x01, data)) {
            return {0x67, level};
        }
        return {0x7F, 0x27, 0x35}; // invalid key
    }
}

std::vector<uint8_t> BaseManufacturer::handleCommunicationControl(uint8_t control_type) {
    (void)control_type;
    return {0x50, 0x28, 0x00};
}

std::vector<uint8_t> BaseManufacturer::handleTesterPresent() {
    session_manager_.setTesterPresent(true);
    session_manager_.resetTimers();
    return {0x7E, 0x3E, 0x00};
}

std::vector<uint8_t> BaseManufacturer::handleReadDataByIdentifier(uint16_t did, const std::vector<uint8_t>& data) {
    (void)data;
    switch (did) {
        case 0xF190: { // VIN
            std::vector<uint8_t> resp = {0x62};
            resp.push_back((did >> 8) & 0xFF);
            resp.push_back(did & 0xFF);
            std::string vin = getVIN();
            for (char c : vin) resp.push_back(c);
            return resp;
        }
        case 0xF187: // system name
            return {0x62, 0xF1, 0x87, 'M', 'u', 'l', 't', 'i', '-', 'E', 'C', 'U', ' ', 'E', 'm', 'u', 'l', 'a', 't', 'o', 'r'};
        case 0xF18C: // ECU serial
        {
            uint32_t sn = getECUSerialNumber();
            return {0x62, 0xF1, 0x8C,
                    static_cast<uint8_t>((sn >> 24) & 0xFF),
                    static_cast<uint8_t>((sn >> 16) & 0xFF),
                    static_cast<uint8_t>((sn >> 8) & 0xFF),
                    static_cast<uint8_t>(sn & 0xFF)};
        }
        case 0x0100: // engine RPM
        {
            uint16_t rpm = static_cast<uint16_t>(engine_rpm_ * 4);
            return {0x62, 0x01, 0x00,
                    static_cast<uint8_t>((rpm >> 8) & 0xFF),
                    static_cast<uint8_t>(rpm & 0xFF)};
        }
        case 0x0101: // vehicle speed
            return {0x62, 0x01, 0x01, static_cast<uint8_t>(vehicle_speed_ * 1.0f)};
        default:
            return {0x7F, 0x22, 0x12};
    }
}

std::vector<uint8_t> BaseManufacturer::handleWriteDataByIdentifier(uint16_t did, const std::vector<uint8_t>& data) {
    (void)did;
    (void)data;
    return {0x6F, 0x2E, (uint8_t)(did >> 8), (uint8_t)(did & 0xFF)};
}

std::vector<uint8_t> BaseManufacturer::handleReadMemoryByAddress(uint32_t address, uint16_t size) {
    (void)address;
    (void)size;
    return {0x7F, 0x23, 0x12};
}

std::vector<uint8_t> BaseManufacturer::handleWriteMemoryByAddress(uint32_t address, const std::vector<uint8_t>& data) {
    (void)address;
    (void)data;
    return {0x7F, 0x3D, 0x12};
}

std::vector<uint8_t> BaseManufacturer::handleRoutineControl(uint8_t routine_type, uint16_t routine_id, const std::vector<uint8_t>& data) {
    (void)data;
    return {0x70, (uint8_t)(routine_type + 0x30), (uint8_t)((routine_id >> 8) & 0xFF), (uint8_t)(routine_id & 0xFF), 0x00};
}

std::vector<uint8_t> BaseManufacturer::handleRequestDownload(uint8_t data_format, uint32_t address, uint32_t size) {
    (void)data_format;
    (void)address;
    (void)size;
    return {0x74, 0x20}; // max block size 32 bytes
}

std::vector<uint8_t> BaseManufacturer::handleRequestUpload(uint8_t data_format, uint32_t address, uint32_t size) {
    (void)data_format;
    (void)address;
    (void)size;
    return {0x75, 0x20};
}

std::vector<uint8_t> BaseManufacturer::handleTransferData(uint8_t block_seq_num, const std::vector<uint8_t>& data) {
    (void)data;
    return {0x76, block_seq_num};
}

std::vector<uint8_t> BaseManufacturer::handleRequestTransferExit() {
    return {0x77};
}

// ===== DTC Management =====

std::vector<DTCInfo> BaseManufacturer::getDTCs(bool only_active) {
    if (only_active) {
        std::vector<DTCInfo> active;
        for (const auto& dtc : dtcs_) {
            if (!dtc.is_pending) active.push_back(dtc);
        }
        return active;
    }
    return dtcs_;
}

bool BaseManufacturer::clearDTCs() {
    dtcs_.clear();
    freeze_frames_.clear();
    distance_since_clear_ = 0;
    return true;
}

bool BaseManufacturer::setDTC(uint16_t code, const std::string& description, bool pending) {
    // Check if already exists
    for (auto& dtc : dtcs_) {
        if (dtc.code == code) {
            dtc.is_pending = pending;
            return true;
        }
    }
    dtcs_.push_back({code, description, pending, false, "minor"});
    return true;
}

std::vector<FreezeFrame> BaseManufacturer::getFreezeFrames() {
    return freeze_frames_;
}

std::vector<uint8_t> BaseManufacturer::getReadinessStatus() {
    // OBD readiness: bitmask of monitor status
    return {0x00, 0x07, 0xFF, 0x00, 0x00}; // all supported, all complete
}

// ===== VIN & ID =====

std::string BaseManufacturer::getVIN() {
    return vin_;
}

void BaseManufacturer::setVIN(const std::string& vin) {
    vin_ = vin;
}

CalibrationInfo BaseManufacturer::getCalibrationInfo() {
    return calibration_;
}

uint32_t BaseManufacturer::getECUSerialNumber() {
    return ecu_serial_;
}

// ===== Simulation Getters =====

float BaseManufacturer::getEngineRPM() { return engine_rpm_; }
float BaseManufacturer::getVehicleSpeed() { return vehicle_speed_; }
float BaseManufacturer::getCoolantTemp() { return coolant_temp_; }
float BaseManufacturer::getIntakeTemp() { return intake_temp_; }
float BaseManufacturer::getMAF() { return maf_; }
float BaseManufacturer::getThrottlePosition() { return throttle_pos_; }
float BaseManufacturer::getO2SensorVoltage(uint8_t bank) {
    if (bank < 8) return o2_voltage_[bank];
    return 0;
}
float BaseManufacturer::getFuelPressure() { return fuel_pressure_; }
float BaseManufacturer::getFuelLevel() { return fuel_level_; }
float BaseManufacturer::getTimingAdvance() { return timing_advance_; }
float BaseManufacturer::getIntakePressure() { return intake_pressure_; }
float BaseManufacturer::getBatteryVoltage() { return battery_voltage_; }
float BaseManufacturer::getRunTime() { return run_time_; }
float BaseManufacturer::getDistanceWithMIL() { return distance_with_mil_; }
float BaseManufacturer::getDistanceSinceCodeClear() { return distance_since_clear_; }

// ===== Simulation Setters =====

void BaseManufacturer::setEngineRPM(float rpm) { engine_rpm_ = rpm; }
void BaseManufacturer::setVehicleSpeed(float speed) { vehicle_speed_ = speed; }
void BaseManufacturer::setCoolantTemp(float temp) { coolant_temp_ = temp; }
void BaseManufacturer::setIntakeTemp(float temp) { intake_temp_ = temp; }
void BaseManufacturer::setMAF(float maf) { maf_ = maf; }
void BaseManufacturer::setThrottlePosition(float pos) { throttle_pos_ = pos; }
void BaseManufacturer::setO2SensorVoltage(uint8_t bank, float voltage) {
    if (bank < 8) o2_voltage_[bank] = voltage;
}
void BaseManufacturer::setFuelPressure(float pressure) { fuel_pressure_ = pressure; }
void BaseManufacturer::setFuelLevel(float level) { fuel_level_ = level; }
void BaseManufacturer::setTimingAdvance(float advance) { timing_advance_ = advance; }
void BaseManufacturer::setIntakePressure(float pressure) { intake_pressure_ = pressure; }
void BaseManufacturer::setBatteryVoltage(float voltage) { battery_voltage_ = voltage; }

// ===== PID helpers =====

bool BaseManufacturer::isPIDSupported(uint8_t pid) const {
    return pid_table_.find(pid) != pid_table_.end();
}

std::vector<uint8_t> BaseManufacturer::getSupportedPIDs(uint8_t range) const {
    std::vector<uint8_t> result(4, 0);
    uint8_t start = range * 0x20;
    uint8_t end = start + 0x20;
    for (const auto& [pid, _] : pid_table_) {
        if (pid >= start && pid < end) {
            uint8_t bit = pid - start;
            result[bit / 8] |= (1 << (7 - (bit % 8)));
        }
    }
    return result;
}

const PIDEntry* BaseManufacturer::getPIDEntry(uint8_t pid) const {
    auto it = pid_table_.find(pid);
    if (it != pid_table_.end()) return &it->second;
    return nullptr;
}

std::shared_ptr<CANManager> BaseManufacturer::getCANManager() const {
    return can_manager_;
}

SessionManager& BaseManufacturer::getSessionManager() {
    return session_manager_;
}

const ManufacturerConfig& BaseManufacturer::getConfig() const {
    return config_;
}

void BaseManufacturer::sendResponse(uint32_t id, const std::vector<uint8_t>& data, bool is_extended) {
    can_manager_->sendFrame(id, data, is_extended);
}

void BaseManufacturer::sendPositiveResponse(uint8_t service_id, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> resp;
    resp.push_back(service_id + 0x40); // positive response
    resp.insert(resp.end(), data.begin(), data.end());
    sendResponse(config_.rx_id, resp);
}

void BaseManufacturer::sendNegativeResponse(uint8_t service_id, uint8_t nrc) {
    sendResponse(config_.rx_id, {0x7F, service_id, nrc});
}

void BaseManufacturer::initPIDs() {
    setupDefaultPIDs();
}

void BaseManufacturer::setupDefaultPIDs() {
    // PID 0x00 – supported PIDs 01-20
    addPID(0x00, "Supported PIDs 01-20", "", 0, 0, "bitmask");
    addPID(0x01, "Monitor status since DTCs cleared", "", 0, 0, "bitmask");
    addPID(0x03, "Fuel system status", "", 0, 0, "bitmask");
    addPID(0x04, "Calculated engine load", "%", 0, 100, "A*100/255");
    addPID(0x05, "Engine coolant temperature", "C", -40, 215, "A-40");
    addPID(0x06, "Fuel trim bank 1 short term", "%", -100, 99.2f, "(A-128)*100/128");
    addPID(0x07, "Fuel trim bank 1 long term", "%", -100, 99.2f, "(A-128)*100/128");
    addPID(0x08, "Fuel trim bank 2 short term", "%", -100, 99.2f, "(A-128)*100/128");
    addPID(0x09, "Fuel trim bank 2 long term", "%", -100, 99.2f, "(A-128)*100/128");
    addPID(0x0A, "Fuel pressure", "kPa", 0, 765, "A*3");
    addPID(0x0B, "Intake manifold pressure", "kPa", 0, 255, "A");
    addPID(0x0C, "Engine RPM", "rpm", 0, 16383.75f, "((A*256)+B)/4");
    addPID(0x0D, "Vehicle speed", "km/h", 0, 255, "A");
    addPID(0x0E, "Timing advance", "degrees", -64, 63.5f, "(A-128)/2");
    addPID(0x0F, "Intake air temperature", "C", -40, 215, "A-40");
    addPID(0x10, "MAF air flow rate", "g/s", 0, 655.35f, "((A*256)+B)/100");
    addPID(0x11, "Throttle position", "%", 0, 100, "A*100/255");
    addPID(0x12, "Commanded secondary air status", "", 0, 0, "bitmask");
    addPID(0x13, "O2 sensors present", "", 0, 0, "bitmask");
    addPID(0x14, "O2 sensor 1 voltage", "V", 0, 1.275f, "A*0.005");
    addPID(0x15, "O2 sensor 2 voltage", "V", 0, 1.275f, "A*0.005");
    addPID(0x16, "O2 sensor 3 voltage", "V", 0, 1.275f, "A*0.005");
    addPID(0x17, "O2 sensor 4 voltage", "V", 0, 1.275f, "A*0.005");
    addPID(0x1C, "OBD standards this ECU conforms to", "", 0, 0, "A");
    addPID(0x1F, "Run time since engine start", "s", 0, 65535, "((A*256)+B)");
    addPID(0x20, "Supported PIDs 21-40", "", 0, 0, "bitmask");

    // Extended PIDs
    addPID(0x21, "Distance traveled with MIL on", "km", 0, 65535, "((A*256)+B)");
    addPID(0x22, "Fuel rail pressure", "kPa", 0, 5177.265f, "((A*256)+B)*0.079");
    addPID(0x2F, "Fuel tank level input", "%", 0, 100, "A*100/255");
    addPID(0x31, "Distance since code cleared", "km", 0, 65535, "((A*256)+B)");
    addPID(0x33, "Barometric pressure", "kPa", 0, 255, "A");
    addPID(0x3C, "Catalyst temperature Bank 1", "C", -40, 6513.5f, "((A*256)+B)/10-40");
    addPID(0x42, "Control module voltage", "V", 0, 65.535f, "((A*256)+B)/1000");
    addPID(0x43, "Absolute load value", "%", 0, 25700, "((A*256)+B)*100/255");
    addPID(0x44, "Command equivalence ratio", "ratio", 0, 2, "((A*256)+B)/32768");
    addPID(0x45, "Relative throttle position", "%", 0, 100, "A*100/255");
    addPID(0x46, "Ambient air temperature", "C", -40, 215, "A-40");
    addPID(0x49, "Accelerator pedal position D", "%", 0, 100, "A*100/255");
    addPID(0x4A, "Accelerator pedal position E", "%", 0, 100, "A*100/255");
    addPID(0x4B, "Accelerator pedal position F", "%", 0, 100, "A*100/255");
    addPID(0x4C, "Commanded throttle actuator", "%", 0, 100, "A*100/255");
    addPID(0x50, "Max MAF", "g/s", 0, 2550, "A*10");
    addPID(0x5C, "Engine oil temperature", "C", -40, 210, "A-40");
    addPID(0x5D, "Fuel injection timing", "degrees", -210, 301.992f, "((A*256)+B)*0.0000305-210");
    addPID(0x5E, "Engine fuel rate", "L/h", 0, 3212.8f, "((A*256)+B)*0.05");
}

void BaseManufacturer::addPID(uint8_t pid, const std::string& name, const std::string& unit,
                        float min_val, float max_val, const std::string& formula) {
    PIDEntry entry;
    entry.pid = pid;
    entry.name = name;
    entry.unit = unit;
    entry.min_val = min_val;
    entry.max_val = max_val;
    entry.formula = formula;
    pid_table_[pid] = entry;
}

std::vector<uint8_t> BaseManufacturer::encodePIDResponse(uint8_t pid, float value) const {
    auto* entry = getPIDEntry(pid);
    if (!entry) return {0x7F, 0x01, 0x12};

    std::vector<uint8_t> result;
    result.push_back(0x41); // mode 01 response
    result.push_back(pid);

    switch (pid) {
        case 0x0C: { // RPM – 2 bytes
            uint16_t rpmEnc = static_cast<uint16_t>(value * 4);
            result.push_back((rpmEnc >> 8) & 0xFF);
            result.push_back(rpmEnc & 0xFF);
            break;
        }
        case 0x0D: // Speed – 1 byte
            result.push_back(static_cast<uint8_t>(value));
            break;
        case 0x04: // Engine load
        case 0x05: // Coolant temp
        case 0x0F: // Intake temp
        case 0x11: // Throttle position
        case 0x2F: // Fuel level
        case 0x46: // Ambient air
        case 0x5C: // Oil temp
            result.push_back(static_cast<uint8_t>(value + 40));
            break;
        case 0x10: // MAF – 2 bytes
        {
            uint16_t mafEnc = static_cast<uint16_t>(value * 100);
            result.push_back((mafEnc >> 8) & 0xFF);
            result.push_back(mafEnc & 0xFF);
            break;
        }
        case 0x14: case 0x15: case 0x16: case 0x17: // O2 – 6 bytes
        {
            // Simplified: just return voltage and trim
            result.push_back(static_cast<uint8_t>(value / 0.005));
            for (int i = 0; i < 5; i++) result.push_back(0x80);
            break;
        }
        case 0x0B: // Intake pressure
            result.push_back(static_cast<uint8_t>(value));
            break;
        case 0x0A: // Fuel pressure
            result.push_back(static_cast<uint8_t>(value / 3));
            break;
        case 0x1F: case 0x21: case 0x31: // Run time / distances – 2 bytes
        {
            uint16_t enc = static_cast<uint16_t>(value);
            result.push_back((enc >> 8) & 0xFF);
            result.push_back(enc & 0xFF);
            break;
        }
        case 0x42: // Battery voltage – 2 bytes
        {
            uint16_t vEnc = static_cast<uint16_t>(value * 1000);
            result.push_back((vEnc >> 8) & 0xFF);
            result.push_back(vEnc & 0xFF);
            break;
        }
        default:
            // Generic: return based on formula
            if (entry->formula.find("256") != std::string::npos) {
                uint16_t enc = static_cast<uint16_t>(value);
                result.push_back((enc >> 8) & 0xFF);
                result.push_back(enc & 0xFF);
            } else {
                result.push_back(static_cast<uint8_t>(value));
            }
            break;
    }
    return result;
}

float BaseManufacturer::decodePIDValue(uint8_t pid, const std::vector<uint8_t>& data) const {
    (void)pid;
    (void)data;
    return 0.0f;
}

// ===== Odometer =====

void BaseManufacturer::setOdometer(float km) {
    odometer_km_ = std::max(0.0f, km);
}

float BaseManufacturer::getOdometer() const {
    return odometer_km_;
}

float BaseManufacturer::getTripOdometer() const {
    return trip_km_;
}

void BaseManufacturer::incrementOdometer(float km) {
    if (km > 0) {
        odometer_km_ += km;
        trip_km_ += km;
    }
}

void BaseManufacturer::resetTrip() {
    trip_km_ = 0;
}

// ===== Vehicle Profile =====

VehicleProfileManager& BaseManufacturer::getProfileManager() {
    return profile_manager_;
}

bool BaseManufacturer::setProfile(const std::string& name) {
    if (profile_manager_.selectProfile(name)) {
        applyProfileDTCs();
        return true;
    }
    return false;
}

std::vector<std::string> BaseManufacturer::getAvailableProfiles() const {
    return profile_manager_.getAvailableProfiles();
}

bool BaseManufacturer::applyProfileDTCs() {
    auto* profile = profile_manager_.getCurrentProfile();
    if (!profile) return false;

    // Clear existing profile-based DTCs first (keep manually set ones)
    for (auto it = dtcs_.begin(); it != dtcs_.end();) {
        bool found = false;
        for (uint16_t code : profile->active_dtcs) {
            if (it->code == code) { found = true; break; }
        }
        if (found) { it = dtcs_.erase(it); }
        else { ++it; }
    }

    // Set profile DTCs
    for (size_t i = 0; i < profile->active_dtcs.size(); i++) {
        std::string desc = (i < profile->dtc_descriptions.size())
            ? profile->dtc_descriptions[i] : "Profile fault";
        setDTC(profile->active_dtcs[i], desc, false);
    }

    return true;
}

} // namespace ecumult
