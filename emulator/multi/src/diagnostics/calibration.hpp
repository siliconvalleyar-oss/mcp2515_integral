#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ecumult {

class CalibrationManager {
public:
    struct CalibrationData {
        uint32_t cvn;
        std::string calibration_id;
        std::string sw_version;
        std::string hw_version;
        std::string part_number;
        std::string supplier;
        uint32_t ecu_serial;
        std::vector<uint8_t> data_blob;
    };

    CalibrationManager();

    void setCalibration(const CalibrationData& cal);
    CalibrationData getCalibration() const;
    uint32_t getCVN() const;
    std::string getCalibrationID() const;
    std::string getSoftwareVersion() const;
    std::string getHardwareVersion() const;
    uint32_t getECUSerial() const;

    static uint32_t calculateCVN(const std::vector<uint8_t>& data);
    static std::string generateCalibrationID(const std::string& manufacturer);

private:
    CalibrationData calibration_;
};

} // namespace ecumult
