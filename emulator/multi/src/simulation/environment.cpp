#include "environment.hpp"
#include <algorithm>

namespace ecumult {

EnvironmentSimulator::EnvironmentSimulator()
    : ambient_temp_(25.0f)
    , barometric_pressure_(101.325f)
    , altitude_(0)
    , humidity_(50.0f)
    , dynamic_conditions_(false)
    , rng_(std::random_device{}())
    , last_update_(std::chrono::steady_clock::now()) {}

void EnvironmentSimulator::setAmbientTemp(float temp) {
    ambient_temp_ = std::clamp(temp, -40.0f, 60.0f);
}

void EnvironmentSimulator::setBarometricPressure(float pressure) {
    barometric_pressure_ = std::max(60.0f, pressure);
}

void EnvironmentSimulator::setAltitude(float meters) {
    altitude_ = std::max(0.0f, meters);
    // Adjust pressure with altitude
    barometric_pressure_ = 101.325f * std::pow(1.0f - (0.0065f * altitude_ / 288.15f), 5.25577f);
}

void EnvironmentSimulator::setHumidity(float percent) {
    humidity_ = std::clamp(percent, 0.0f, 100.0f);
}

float EnvironmentSimulator::getAmbientTemp() const {
    return ambient_temp_;
}

float EnvironmentSimulator::getBarometricPressure() const {
    return barometric_pressure_;
}

float EnvironmentSimulator::getAltitude() const {
    return altitude_;
}

float EnvironmentSimulator::getHumidity() const {
    return humidity_;
}

void EnvironmentSimulator::update(float dt) {
    (void)dt;
    if (dynamic_conditions_) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update_).count();
        if (elapsed >= 30) {
            simulateWeatherChange();
            last_update_ = now;
        }
    }
}

void EnvironmentSimulator::setDynamicConditions(bool dynamic) {
    dynamic_conditions_ = dynamic;
    if (dynamic) {
        last_update_ = std::chrono::steady_clock::now();
    }
}

void EnvironmentSimulator::simulateWeatherChange() {
    std::uniform_real_distribution<float> dist(-2.0f, 2.0f);
    ambient_temp_ = std::clamp(ambient_temp_ + dist(rng_), -10.0f, 50.0f);
    humidity_ = std::clamp(humidity_ + dist(rng_) * 5.0f, 10.0f, 95.0f);
    barometric_pressure_ = std::max(95.0f, barometric_pressure_ + dist(rng_));
}

} // namespace ecumult
