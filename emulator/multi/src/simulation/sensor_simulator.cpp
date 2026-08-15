#include "sensor_simulator.hpp"
#include <algorithm>
#include <cmath>

namespace ecumult {

SensorSimulator::SensorSimulator()
    : engine_rpm_(0)
    , vehicle_speed_(0)
    , coolant_temp_(95)
    , intake_temp_(35)
    , maf_(3.5f)
    , throttle_pos_(0)
    , fuel_level_(50)
    , battery_voltage_(12.6f)
    , timing_advance_(10)
    , intake_pressure_(101.3f)
    , fuel_pressure_(350)
    , noise_level_(0.005f)
    , rng_(std::random_device{}())
    , noise_dist_(0.0f, 1.0f) {}

void SensorSimulator::setEngineRPM(float rpm) { engine_rpm_ = std::max(0.0f, rpm); }
void SensorSimulator::setVehicleSpeed(float speed) { vehicle_speed_ = std::max(0.0f, speed); }
void SensorSimulator::setCoolantTemp(float temp) { coolant_temp_ = std::clamp(temp, -40.0f, 215.0f); }
void SensorSimulator::setIntakeTemp(float temp) { intake_temp_ = std::clamp(temp, -40.0f, 215.0f); }
void SensorSimulator::setMAF(float maf) { maf_ = std::max(0.0f, maf); }
void SensorSimulator::setThrottlePosition(float pos) { throttle_pos_ = std::clamp(pos, 0.0f, 100.0f); }
void SensorSimulator::setFuelLevel(float level) { fuel_level_ = std::clamp(level, 0.0f, 100.0f); }
void SensorSimulator::setBatteryVoltage(float voltage) { battery_voltage_ = std::max(0.0f, voltage); }
void SensorSimulator::setTimingAdvance(float advance) { timing_advance_ = advance; }
void SensorSimulator::setIntakePressure(float pressure) { intake_pressure_ = std::max(0.0f, pressure); }
void SensorSimulator::setFuelPressure(float pressure) { fuel_pressure_ = std::max(0.0f, pressure); }

float SensorSimulator::getEngineRPM() const { return engine_rpm_; }
float SensorSimulator::getVehicleSpeed() const { return vehicle_speed_; }
float SensorSimulator::getCoolantTemp() const { return coolant_temp_; }
float SensorSimulator::getIntakeTemp() const { return intake_temp_; }
float SensorSimulator::getMAF() const { return maf_; }
float SensorSimulator::getThrottlePosition() const { return throttle_pos_; }
float SensorSimulator::getFuelLevel() const { return fuel_level_; }
float SensorSimulator::getBatteryVoltage() const { return battery_voltage_; }
float SensorSimulator::getTimingAdvance() const { return timing_advance_; }
float SensorSimulator::getIntakePressure() const { return intake_pressure_; }
float SensorSimulator::getFuelPressure() const { return fuel_pressure_; }

void SensorSimulator::addNoise(float amplitude) {
    auto noise = [&]() { return noise_dist_(rng_) * noise_level_ * amplitude; };
    engine_rpm_ += noise() * 20;
    vehicle_speed_ += noise();
    coolant_temp_ += noise();
    battery_voltage_ += noise() * 0.1f;
}

void SensorSimulator::setNoiseLevel(float level) {
    noise_level_ = std::max(0.0f, level);
}

void SensorSimulator::updateDriving(float rpm, float speed, float dt) {
    (void)dt;
    engine_rpm_ = rpm;
    vehicle_speed_ = speed;

    // Derive other sensors from RPM/Speed
    if (rpm > 0) {
        coolant_temp_ = std::max(85.0f, 90.0f + (rpm / 6000.0f) * 10.0f - noise_dist_(rng_) * 3.0f);
        intake_temp_ = std::max(25.0f, 30.0f + (rpm / 6000.0f) * 15.0f - noise_dist_(rng_) * 5.0f);
        maf_ = (rpm / 1000.0f) * (speed > 0 ? 1.0f + speed / 200.0f : 0.5f) + noise_dist_(rng_) * 0.2f;
        throttle_pos_ = std::min(100.0f, throttle_pos_ + (rpm > 2000 ? 0.5f : -0.3f));
        intake_pressure_ = 101.3f + (rpm / 6000.0f) * 30.0f + noise_dist_(rng_);
        fuel_pressure_ = 350.0f + noise_dist_(rng_) * 10.0f;
        timing_advance_ = 10.0f + (rpm / 6000.0f) * 20.0f - noise_dist_(rng_);
    }

    battery_voltage_ = 12.6f - (rpm > 0 ? 0.5f : 0.0f) + noise_dist_(rng_) * 0.2f;

    addNoise(0.01f);
}

} // namespace ecumult
