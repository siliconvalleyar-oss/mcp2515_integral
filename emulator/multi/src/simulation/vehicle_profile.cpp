#include "vehicle_profile.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace ecumult {

VehicleProfileManager::VehicleProfileManager()
    : current_profile_("normal")
    , rng_(std::random_device{}())
    , idle_phase_(0) {
    profiles_["normal"] = gmNormalIdle();
    profiles_["unstable_idle"] = gmUnstableIdle();
    profiles_["bad_o2"] = gmBadO2Sensor();
    profiles_["maf_fault"] = gmMAFFault();
    profiles_["coolant_fault"] = gmCoolantFault();
    profiles_["misfire"] = gmMisfire();
    profiles_["low_battery"] = gmLowBattery();
    profiles_["fuel_pressure"] = gmFuelPressureFault();
    profiles_["emission_fail"] = gmEmissionFail();
    profiles_["custom"] = gmCustom();
}

// ── Perfiles predefinidos GM ─────────────────────────────────────

VehicleProfile VehicleProfileManager::gmNormalIdle() {
    VehicleProfile p;
    p.name = "normal";
    p.description = "Ralentí normal, sensores dentro de especificación";
    p.rpm_idle_min = 650;
    p.rpm_idle_max = 750;
    p.rpm_drive_min = 1500;
    p.rpm_drive_max = 6000;
    p.unstable_idle = false;
    p.idle_fluctuation_hz = 0;
    p.idle_fluctuation_amp = 0;
    p.fuel_consumption_rate = 0.8f;
    p.oil_temp_base = 90;
    p.oil_pressure_base = 350;
    p.throttle_response_delay = 0.1f;
    p.max_speed = 200;
    p.acceleration_rate = 3.0f;
    return p;
}

VehicleProfile VehicleProfileManager::gmUnstableIdle() {
    VehicleProfile p;
    p.name = "unstable_idle";
    p.description = "Ralentí inestable: RPM fluctúa entre 500-1100, posible fallo de IAC o vacío";
    p.rpm_idle_min = 500;
    p.rpm_idle_max = 1100;
    p.rpm_drive_min = 1200;
    p.rpm_drive_max = 5000;
    p.unstable_idle = true;
    p.idle_fluctuation_hz = 2.0f;
    p.idle_fluctuation_amp = 250;
    p.active_dtcs = {0x0505, 0x0507, 0x0500};
    p.dtc_descriptions = {"Idle Control System Malfunction", "Idle Control System RPM Higher Than Expected", "Vehicle Speed Sensor Malfunction"};
    p.fuel_consumption_rate = 1.5f;
    p.oil_temp_base = 95;
    p.oil_pressure_base = 280;
    p.throttle_response_delay = 0.3f;
    p.max_speed = 170;
    p.acceleration_rate = 2.0f;

    p.sensor_overrides[0x0C] = {500, 1100, 80, 0, false, 0};   // RPM ruidoso
    p.sensor_overrides[0x0B] = {70, 120, 5, 0, false, 0};      // MAP errático
    p.sensor_overrides[0x11] = {0, 100, 3, 0, false, 0};       // Throttle ruidoso
    return p;
}

VehicleProfile VehicleProfileManager::gmBadO2Sensor() {
    VehicleProfile p;
    p.name = "bad_o2";
    p.description = "Sensor de oxígeno defectuoso: voltaje fijo en 0.45V o lecturas erráticas";
    p.rpm_idle_min = 700;
    p.rpm_idle_max = 800;
    p.rpm_drive_min = 1500;
    p.rpm_drive_max = 5500;
    p.unstable_idle = false;
    p.idle_fluctuation_hz = 0.5f;
    p.idle_fluctuation_amp = 30;
    p.active_dtcs = {0x0135, 0x0171, 0x0172};
    p.dtc_descriptions = {"O2 Sensor Heater Circuit Bank 1 Sensor 1", "System Too Lean Bank 1", "System Too Rich Bank 1"};
    p.fuel_consumption_rate = 1.2f;
    p.oil_temp_base = 92;
    p.oil_pressure_base = 330;
    p.throttle_response_delay = 0.15f;
    p.max_speed = 190;
    p.acceleration_rate = 2.5f;

    // O2 sensor stuck at 0.45V (dead sensor) o valores erráticos
    SensorOverride o2_stuck;
    o2_stuck.min_val = 0.44f;
    o2_stuck.max_val = 0.46f;
    o2_stuck.stuck_value = true;
    o2_stuck.stuck_at = 0.45f;
    p.sensor_overrides[0x14] = o2_stuck;  // O2 B1S1
    p.sensor_overrides[0x15] = o2_stuck;  // O2 B1S2

    // Fuel trims alterados por lectura incorrecta
    p.sensor_overrides[0x06] = {15, 25, 0, 0, false, 0};  // STFT muy positivo (cree que está pobre)
    p.sensor_overrides[0x07] = {10, 20, 0, 0, false, 0};  // LTFT alto
    return p;
}

VehicleProfile VehicleProfileManager::gmMAFFault() {
    VehicleProfile p;
    p.name = "maf_fault";
    p.description = "Sensor MAF defectuoso: lecturas incorrectas de flujo de aire";
    p.rpm_idle_min = 600;
    p.rpm_idle_max = 900;
    p.rpm_drive_min = 1300;
    p.rpm_drive_max = 5000;
    p.unstable_idle = true;
    p.idle_fluctuation_hz = 1.0f;
    p.idle_fluctuation_amp = 150;
    p.active_dtcs = {0x0101, 0x0102, 0x0103, 0x0171};
    p.dtc_descriptions = {"MAF Circuit Range/Performance", "MAF Circuit Low Input", "MAF Circuit High Input", "System Too Lean Bank 1"};
    p.fuel_consumption_rate = 1.3f;
    p.oil_temp_base = 93;
    p.oil_pressure_base = 310;

    p.sensor_overrides[0x10] = {0.5f, 1.5f, 0.3f, 0, false, 0};  // MAF muy bajo para RPM
    p.sensor_overrides[0x0C] = {600, 900, 50, 0, false, 0};       // RPM inestable
    p.sensor_overrides[0x06] = {20, 30, 0, 0, false, 0};          // STFT alto (inyecta más por MAF bajo)
    return p;
}

VehicleProfile VehicleProfileManager::gmCoolantFault() {
    VehicleProfile p;
    p.name = "coolant_fault";
    p.description = "Sensor de temperatura del refrigerante defectuoso: lectura fija en -40°C o valores erráticos";
    p.rpm_idle_min = 750;
    p.rpm_idle_max = 850;
    p.rpm_drive_min = 1500;
    p.rpm_drive_max = 5000;
    p.unstable_idle = false;
    p.active_dtcs = {0x0118, 0x0125, 0x0126};
    p.dtc_descriptions = {"ECT Sensor Circuit High Input", "ECT Sensor Circuit Low Input", "ECT Sensor Circuit Range/Performance"};
    p.fuel_consumption_rate = 2.0f;  // alto porque la ECU cree que el motor está frío
    p.oil_temp_base = 85;
    p.oil_pressure_base = 340;

    // Coolant temp fijo en -40°C (circuito abierto)
    p.sensor_overrides[0x05] = {-40, -40, 0, 0, true, -40};
    return p;
}

VehicleProfile VehicleProfileManager::gmMisfire() {
    VehicleProfile p;
    p.name = "misfire";
    p.description = "Fallo de encendido en cilindros múltiples, motor vibra";
    p.rpm_idle_min = 400;
    p.rpm_idle_max = 700;
    p.rpm_drive_min = 1000;
    p.rpm_drive_max = 4000;
    p.unstable_idle = true;
    p.idle_fluctuation_hz = 3.0f;
    p.idle_fluctuation_amp = 300;
    p.active_dtcs = {0x0300, 0x0301, 0x0303, 0x0304};
    p.dtc_descriptions = {"Random/Multiple Cylinder Misfire", "Cylinder 1 Misfire", "Cylinder 3 Misfire", "Cylinder 4 Misfire"};
    p.fuel_consumption_rate = 2.5f;
    p.oil_temp_base = 98;
    p.oil_pressure_base = 260;
    p.acceleration_rate = 1.0f;
    p.max_speed = 120;

    p.sensor_overrides[0x0C] = {400, 700, 120, 0, false, 0};
    p.sensor_overrides[0x04] = {60, 90, 5, 0, false, 0};  // engine load errático
    p.sensor_overrides[0x5C] = {100, 120, 0, 0, false, 0}; // oil temp alta
    return p;
}

VehicleProfile VehicleProfileManager::gmLowBattery() {
    VehicleProfile p;
    p.name = "low_battery";
    p.description = "Batería baja o alternador defectuoso: voltaje 10.5-11.5V, múltiples fallos eléctricos";
    p.rpm_idle_min = 700;
    p.rpm_idle_max = 800;
    p.rpm_drive_min = 1500;
    p.rpm_drive_max = 5000;
    p.unstable_idle = false;
    p.idle_fluctuation_hz = 0.3f;
    p.idle_fluctuation_amp = 50;
    p.active_dtcs = {0x0620, 0x0560, 0x0562, 0x0563};
    p.dtc_descriptions = {"Generator Control Circuit Malfunction", "System Voltage Low", "System Voltage Unstable", "System Voltage High"};
    p.fuel_consumption_rate = 1.0f;
    p.oil_temp_base = 90;
    p.oil_pressure_base = 320;

    p.sensor_overrides[0x42] = {10.2f, 11.8f, 0.5f, -0.01f, false, 0}; // voltaje bajo y cayendo
    return p;
}

VehicleProfile VehicleProfileManager::gmFuelPressureFault() {
    VehicleProfile p;
    p.name = "fuel_pressure";
    p.description = "Presión de combustible incorrecta: bomba de combustible o regulador defectuoso";
    p.rpm_idle_min = 600;
    p.rpm_idle_max = 750;
    p.rpm_drive_min = 1200;
    p.rpm_drive_max = 4500;
    p.unstable_idle = true;
    p.idle_fluctuation_hz = 1.5f;
    p.idle_fluctuation_amp = 120;
    p.active_dtcs = {0x0087, 0x0088, 0x0089, 0x0171};
    p.dtc_descriptions = {"Fuel Rail Pressure Too Low", "Fuel Rail Pressure Too High", "Fuel Pressure Regulator", "System Too Lean Bank 1"};
    p.fuel_consumption_rate = 1.8f;
    p.oil_pressure_base = 300;

    p.sensor_overrides[0x22] = {50, 150, 20, 0, false, 0};  // fuel rail pressure muy bajo (normal: 350-500)
    p.sensor_overrides[0x06] = {20, 35, 0, 0, false, 0};    // STFT alto por falta de combustible
    p.sensor_overrides[0x0A] = {10, 40, 5, 0, false, 0};    // fuel pressure (mode 01 PID 0x0A)
    return p;
}

VehicleProfile VehicleProfileManager::gmEmissionFail() {
    VehicleProfile p;
    p.name = "emission_fail";
    p.description = "Fallo de emisiones: convertidor catalítico ineficiente, sistema EVAP";
    p.rpm_idle_min = 700;
    p.rpm_idle_max = 800;
    p.rpm_drive_min = 1500;
    p.rpm_drive_max = 5500;
    p.unstable_idle = false;
    p.idle_fluctuation_hz = 0;
    p.idle_fluctuation_amp = 0;
    p.active_dtcs = {0x0420, 0x0430, 0x0442, 0x0455};
    p.dtc_descriptions = {
        "Catalyst System Efficiency Below Threshold Bank 1",
        "Catalyst System Efficiency Below Threshold Bank 2",
        "EVAP System Leak Detected Small",
        "EVAP System Leak Detected Large"
    };
    p.fuel_consumption_rate = 1.1f;
    p.oil_temp_base = 95;
    p.oil_pressure_base = 340;

    // O2 post-cat readings mimicking bad catalyst
    p.sensor_overrides[0x15] = {0.1f, 0.8f, 0.1f, 0, false, 0};  // O2 B1S2 activo (debería estar plano si el catalizador funciona)
    p.sensor_overrides[0x17] = {0.1f, 0.8f, 0.1f, 0, false, 0};  // O2 B2S2 activo
    return p;
}

VehicleProfile VehicleProfileManager::gmCustom() {
    VehicleProfile p;
    p.name = "custom";
    p.description = "Perfil personalizable vía API";
    p.rpm_idle_min = 650;
    p.rpm_idle_max = 750;
    p.rpm_drive_min = 1500;
    p.rpm_drive_max = 6000;
    p.unstable_idle = false;
    p.idle_fluctuation_hz = 0;
    p.idle_fluctuation_amp = 0;
    p.fuel_consumption_rate = 0.8f;
    p.oil_temp_base = 90;
    p.oil_pressure_base = 350;
    p.throttle_response_delay = 0.1f;
    p.max_speed = 200;
    p.acceleration_rate = 3.0f;
    return p;
}

// ── Gestión de perfiles ─────────────────────────────────────────

bool VehicleProfileManager::selectProfile(const std::string& name) {
    if (profiles_.find(name) == profiles_.end()) return false;
    current_profile_ = name;
    idle_phase_ = 0;
    std::cout << "[Profile] Selected: " << name << " - " << profiles_[name].description << std::endl;
    return true;
}

VehicleProfile* VehicleProfileManager::getCurrentProfile() {
    auto it = profiles_.find(current_profile_);
    if (it != profiles_.end()) return &it->second;
    return nullptr;
}

const VehicleProfile* VehicleProfileManager::getCurrentProfile() const {
    auto it = profiles_.find(current_profile_);
    if (it != profiles_.end()) return &it->second;
    return nullptr;
}

std::vector<std::string> VehicleProfileManager::getAvailableProfiles() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : profiles_) {
        names.push_back(name);
    }
    return names;
}

bool VehicleProfileManager::applyCustomProfile(const VehicleProfile& profile) {
    profiles_["custom"] = profile;
    return true;
}

float VehicleProfileManager::applyRPMProfile(float base_rpm, float dt) {
    auto* profile = getCurrentProfile();
    if (!profile) return base_rpm;

    if (!profile->unstable_idle) {
        return std::clamp(base_rpm, profile->rpm_idle_min, profile->rpm_drive_max);
    }

    idle_phase_ += dt * profile->idle_fluctuation_hz * 2 * M_PI;

    // Ralentí inestable: sinusoidal + ruido aleatorio
    std::normal_distribution<float> noise(0, 30);
    float fluctuation = std::sin(idle_phase_) * profile->idle_fluctuation_amp;
    fluctuation += noise(rng_);

    float result = base_rpm + fluctuation;
    return std::clamp(result, profile->rpm_idle_min, profile->rpm_drive_max);
}

float VehicleProfileManager::applySensorOverride(uint8_t pid, float base_value, float dt) {
    auto* profile = getCurrentProfile();
    if (!profile) return base_value;

    auto it = profile->sensor_overrides.find(pid);
    if (it == profile->sensor_overrides.end()) return base_value;

    const auto& ov = it->second;

    if (ov.stuck_value) {
        return ov.stuck_at;
    }

    std::normal_distribution<float> noise(0, ov.noise_amplitude);
    float result = base_value + noise(rng_);

    // Deriva
    result += ov.drift_per_sec * dt;

    return std::clamp(result, ov.min_val, ov.max_val);
}

bool VehicleProfileManager::isUnstableIdle() const {
    auto* profile = getCurrentProfile();
    return profile && profile->unstable_idle;
}

} // namespace ecumult
