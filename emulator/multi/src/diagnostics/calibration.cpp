#include "calibration.hpp"
#include <random>

namespace ecumult {

CalibrationManager::CalibrationManager() {
    calibration_.cvn = 0x12345678;
    calibration_.calibration_id = "MULTI_ECU_V1.0";
    calibration_.sw_version = "1.0.0";
    calibration_.hw_version = "Rev C";
    calibration_.part_number = "A0000001";
    calibration_.supplier = "OpenCode Emulator";
    calibration_.ecu_serial = 0x87654321;
}

void CalibrationManager::setCalibration(const CalibrationData& cal) {
    calibration_ = cal;
}

CalibrationManager::CalibrationData CalibrationManager::getCalibration() const {
    return calibration_;
}

uint32_t CalibrationManager::getCVN() const {
    return calibration_.cvn;
}

std::string CalibrationManager::getCalibrationID() const {
    return calibration_.calibration_id;
}

std::string CalibrationManager::getSoftwareVersion() const {
    return calibration_.sw_version;
}

std::string CalibrationManager::getHardwareVersion() const {
    return calibration_.hw_version;
}

uint32_t CalibrationManager::getECUSerial() const {
    return calibration_.ecu_serial;
}

uint32_t CalibrationManager::calculateCVN(const std::vector<uint8_t>& data) {
    // Simplified CRC32-like CVN calculation
    uint32_t crc = 0xFFFFFFFF;
    for (auto b : data) {
        crc ^= b;
        for (int i = 0; i < 8; i++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return ~crc;
}

std::string CalibrationManager::generateCalibrationID(const std::string& manufacturer) {
    std::string id = manufacturer.substr(0, 4);
    std::transform(id.begin(), id.end(), id.begin(), ::toupper);
    id += "_ECU_";
    std::mt19937 rng(std::random_device{}());
    id += std::to_string(1000 + (rng() % 9000));
    return id;
}

} // namespace ecumult
