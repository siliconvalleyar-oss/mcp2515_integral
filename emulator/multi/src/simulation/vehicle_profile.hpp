#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <random>

namespace ecumult {

struct SensorOverride {
    float min_val;
    float max_val;
    float noise_amplitude;  // 0 = sin ruido, >0 = ruido adicional
    float drift_per_sec;    // deriva por segundo
    bool stuck_value;       // true = el valor no cambia
    float stuck_at;         // valor fijo si stuck_value=true
};

struct VehicleProfile {
    std::string name;
    std::string description;

    // Rango de RPM
    float rpm_idle_min;
    float rpm_idle_max;
    float rpm_drive_min;
    float rpm_drive_max;

    // Comportamiento del ralentí
    bool unstable_idle;       // true = ralentí inestable
    float idle_fluctuation_hz; // frecuencia de fluctuación
    float idle_fluctuation_amp; // amplitud de fluctuación en RPM

    // Fallos simulados
    std::vector<uint16_t> active_dtcs;
    std::vector<std::string> dtc_descriptions;

    // Override de sensores por PID
    std::unordered_map<uint8_t, SensorOverride> sensor_overrides;

    // Consumo y desgaste
    float fuel_consumption_rate;  // L/h en ralentí
    float oil_temp_base;          // °C
    float oil_pressure_base;      // kPa

    // Respuesta del vehículo
    float throttle_response_delay; // segundos
    float max_speed;               // km/h
    float acceleration_rate;       // m/s²
};

class VehicleProfileManager {
public:
    VehicleProfileManager();

    // Perfiles predefinidos GM
    static VehicleProfile gmNormalIdle();
    static VehicleProfile gmUnstableIdle();
    static VehicleProfile gmBadO2Sensor();
    static VehicleProfile gmMAFFault();
    static VehicleProfile gmCoolantFault();
    static VehicleProfile gmMisfire();
    static VehicleProfile gmLowBattery();
    static VehicleProfile gmFuelPressureFault();
    static VehicleProfile gmEmissionFail();
    static VehicleProfile gmCustom();

    bool selectProfile(const std::string& name);
    VehicleProfile* getCurrentProfile();
    const VehicleProfile* getCurrentProfile() const;
    std::vector<std::string> getAvailableProfiles() const;
    bool applyCustomProfile(const VehicleProfile& profile);

    // Aplicar perfil a valores de sensor
    float applyRPMProfile(float base_rpm, float dt);
    float applySensorOverride(uint8_t pid, float base_value, float dt);

    bool isUnstableIdle() const;

private:
    std::unordered_map<std::string, VehicleProfile> profiles_;
    std::string current_profile_;
    mutable std::mt19937 rng_;
    float idle_phase_; // para fluctuación sinusoidal
};

} // namespace ecumult
