#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include "../core/protocol_router.hpp"
#include "../core/session_manager.hpp"
#include "../core/can_manager.hpp"
#include "../simulation/vehicle_profile.hpp"

namespace ecumult {

struct PIDEntry {
    uint8_t pid;
    std::string name;
    std::string unit;
    float min_val;
    float max_val;
    std::string formula; // e.g., "A*0.0625" for RPM
    std::function<std::vector<uint8_t>(float)> encode;
    std::function<float(const std::vector<uint8_t>&)> decode;
};

struct DTCInfo {
    uint16_t code;
    std::string description;
    bool is_pending;
    bool is_permanent;
    std::string severity;
};

struct FreezeFrame {
    uint16_t dtc_code;
    std::unordered_map<uint8_t, float> pid_values;
    std::chrono::system_clock::time_point timestamp;
};

struct CalibrationInfo {
    uint32_t cvn; // Calibration Verification Number
    std::string calibration_id;
    std::string sw_version;
    std::string hw_version;
};

class BaseManufacturer {
public:
    BaseManufacturer(std::shared_ptr<CANManager> can_manager);
    virtual ~BaseManufacturer() = default;

    // Pure virtual – each manufacturer must implement
    virtual void handleCanFrame(const CANFrame& frame) = 0;
    virtual Manufacturer getManufacturerId() const = 0;
    virtual std::string getManufacturerName() const = 0;

    // Lifecycle
    virtual void onManufacturerSelected(const ManufacturerConfig& cfg);
    virtual void onManufacturerDeselected();
    virtual void reset();

    // OBD2 Standard Modes (01-0A) – common implementation
    virtual std::vector<uint8_t> handleMode01(uint8_t pid);
    virtual std::vector<uint8_t> handleMode02(uint8_t pid);
    virtual std::vector<uint8_t> handleMode03();
    virtual std::vector<uint8_t> handleMode04();
    virtual std::vector<uint8_t> handleMode05(uint8_t test_id);
    virtual std::vector<uint8_t> handleMode06(uint8_t monitor_id);
    virtual std::vector<uint8_t> handleMode07();
    virtual std::vector<uint8_t> handleMode08(uint8_t test_id, const std::vector<uint8_t>& data);
    virtual std::vector<uint8_t> handleMode09(uint8_t info_type);
    virtual std::vector<uint8_t> handleMode0A();

    // UDS Services (ISO 14229)
    virtual std::vector<uint8_t> handleDiagnosticSessionControl(uint8_t session_type);
    virtual std::vector<uint8_t> handleECUReset(uint8_t reset_type);
    virtual std::vector<uint8_t> handleSecurityAccess(uint8_t level, const std::vector<uint8_t>& data);
    virtual std::vector<uint8_t> handleCommunicationControl(uint8_t control_type);
    virtual std::vector<uint8_t> handleTesterPresent();
    virtual std::vector<uint8_t> handleReadDataByIdentifier(uint16_t did, const std::vector<uint8_t>& data);
    virtual std::vector<uint8_t> handleWriteDataByIdentifier(uint16_t did, const std::vector<uint8_t>& data);
    virtual std::vector<uint8_t> handleReadMemoryByAddress(uint32_t address, uint16_t size);
    virtual std::vector<uint8_t> handleWriteMemoryByAddress(uint32_t address, const std::vector<uint8_t>& data);
    virtual std::vector<uint8_t> handleRoutineControl(uint8_t routine_type, uint16_t routine_id, const std::vector<uint8_t>& data);
    virtual std::vector<uint8_t> handleRequestDownload(uint8_t data_format, uint32_t address, uint32_t size);
    virtual std::vector<uint8_t> handleRequestUpload(uint8_t data_format, uint32_t address, uint32_t size);
    virtual std::vector<uint8_t> handleTransferData(uint8_t block_seq_num, const std::vector<uint8_t>& data);
    virtual std::vector<uint8_t> handleRequestTransferExit();

    // DTC management
    virtual std::vector<DTCInfo> getDTCs(bool only_active = false);
    virtual bool clearDTCs();
    virtual bool setDTC(uint16_t code, const std::string& description, bool pending = false);
    virtual std::vector<FreezeFrame> getFreezeFrames();
    virtual std::vector<uint8_t> getReadinessStatus();

    // VIN and identification
    virtual std::string getVIN();
    virtual void setVIN(const std::string& vin);
    virtual CalibrationInfo getCalibrationInfo();
    virtual uint32_t getECUSerialNumber();

    // Simulation values
    virtual float getEngineRPM();
    virtual float getVehicleSpeed();
    virtual float getCoolantTemp();
    virtual float getIntakeTemp();
    virtual float getMAF();
    virtual float getThrottlePosition();
    virtual float getO2SensorVoltage(uint8_t bank = 0);
    virtual float getFuelPressure();
    virtual float getFuelLevel();
    virtual float getTimingAdvance();
    virtual float getIntakePressure();
    virtual float getBatteryVoltage();
    virtual float getRunTime();
    virtual float getDistanceWithMIL();
    virtual float getDistanceSinceCodeClear();

    // Odometer
    virtual void setOdometer(float km);
    virtual float getOdometer() const;
    virtual float getTripOdometer() const;
    virtual void incrementOdometer(float km);
    virtual void resetTrip();

    // Vehicle profile
    virtual VehicleProfileManager& getProfileManager();
    virtual bool setProfile(const std::string& name);
    virtual std::vector<std::string> getAvailableProfiles() const;
    virtual bool applyProfileDTCs();

    // Setters for simulation
    virtual void setEngineRPM(float rpm);
    virtual void setVehicleSpeed(float speed);
    virtual void setCoolantTemp(float temp);
    virtual void setIntakeTemp(float temp);
    virtual void setMAF(float maf);
    virtual void setThrottlePosition(float pos);
    virtual void setO2SensorVoltage(uint8_t bank, float voltage);
    virtual void setFuelPressure(float pressure);
    virtual void setFuelLevel(float level);
    virtual void setTimingAdvance(float advance);
    virtual void setIntakePressure(float pressure);
    virtual void setBatteryVoltage(float voltage);

    // PID tables
    virtual bool isPIDSupported(uint8_t pid) const;
    virtual std::vector<uint8_t> getSupportedPIDs(uint8_t range) const;
    virtual const PIDEntry* getPIDEntry(uint8_t pid) const;

    // Accessors
    std::shared_ptr<CANManager> getCANManager() const;
    SessionManager& getSessionManager();
    const ManufacturerConfig& getConfig() const;
    void sendResponse(uint32_t id, const std::vector<uint8_t>& data, bool is_extended = false);
    void sendPositiveResponse(uint8_t service_id, const std::vector<uint8_t>& data);
    void sendNegativeResponse(uint8_t service_id, uint8_t nrc);

protected:
    std::shared_ptr<CANManager> can_manager_;
    ManufacturerConfig config_;
    SessionManager session_manager_;

    // simulated values
    float engine_rpm_;
    float vehicle_speed_;
    float coolant_temp_;
    float intake_temp_;
    float maf_;
    float throttle_pos_;
    float o2_voltage_[8];
    float fuel_pressure_;
    float fuel_level_;
    float timing_advance_;
    float intake_pressure_;
    float battery_voltage_;
    float run_time_;
    float distance_with_mil_;
    float distance_since_clear_;

    // Odometer
    float odometer_km_;
    float trip_km_;

    // Vehicle profile
    VehicleProfileManager profile_manager_;

    std::string vin_;
    CalibrationInfo calibration_;
    uint32_t ecu_serial_;

    std::vector<DTCInfo> dtcs_;
    std::vector<FreezeFrame> freeze_frames_;
    std::unordered_map<uint8_t, PIDEntry> pid_table_;

    virtual void initPIDs();
    void setupDefaultPIDs();
    void addPID(uint8_t pid, const std::string& name, const std::string& unit,
                float min_val, float max_val, const std::string& formula);
    std::vector<uint8_t> encodePIDResponse(uint8_t pid, float value) const;
    float decodePIDValue(uint8_t pid, const std::vector<uint8_t>& data) const;
};

} // namespace ecumult
