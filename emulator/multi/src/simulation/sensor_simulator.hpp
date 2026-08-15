#pragma once

#include <cstdint>
#include <random>
#include <functional>

namespace ecumult {

class SensorSimulator {
public:
    SensorSimulator();

    void setEngineRPM(float rpm);
    void setVehicleSpeed(float speed);
    void setCoolantTemp(float temp);
    void setIntakeTemp(float temp);
    void setMAF(float maf);
    void setThrottlePosition(float pos);
    void setFuelLevel(float level);
    void setBatteryVoltage(float voltage);
    void setTimingAdvance(float advance);
    void setIntakePressure(float pressure);
    void setFuelPressure(float pressure);

    float getEngineRPM() const;
    float getVehicleSpeed() const;
    float getCoolantTemp() const;
    float getIntakeTemp() const;
    float getMAF() const;
    float getThrottlePosition() const;
    float getFuelLevel() const;
    float getBatteryVoltage() const;
    float getTimingAdvance() const;
    float getIntakePressure() const;
    float getFuelPressure() const;

    void addNoise(float amplitude = 0.01f);
    void setNoiseLevel(float level);

    // Update RPM and speed together with realistic correlation
    void updateDriving(float rpm, float speed, float dt);

private:
    float engine_rpm_;
    float vehicle_speed_;
    float coolant_temp_;
    float intake_temp_;
    float maf_;
    float throttle_pos_;
    float fuel_level_;
    float battery_voltage_;
    float timing_advance_;
    float intake_pressure_;
    float fuel_pressure_;

    float noise_level_;
    mutable std::mt19937 rng_;
    mutable std::normal_distribution<float> noise_dist_;
};

} // namespace ecumult
