#pragma once

#include <cstdint>
#include <random>
#include <chrono>

namespace ecumult {

class EnvironmentSimulator {
public:
    EnvironmentSimulator();

    void setAmbientTemp(float temp);
    void setBarometricPressure(float pressure);
    void setAltitude(float meters);
    void setHumidity(float percent);

    float getAmbientTemp() const;
    float getBarometricPressure() const;
    float getAltitude() const;
    float getHumidity() const;

    void update(float dt);
    void setDynamicConditions(bool dynamic);

private:
    float ambient_temp_;
    float barometric_pressure_;
    float altitude_;
    float humidity_;

    bool dynamic_conditions_;
    mutable std::mt19937 rng_;
    std::chrono::steady_clock::time_point last_update_;

    void simulateWeatherChange();
};

} // namespace ecumult
