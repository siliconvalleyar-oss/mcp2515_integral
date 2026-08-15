#include "seed_data.hpp"

namespace ecumult {

std::vector<SeedVehicle> SeedData::getVehicles() {
    return {
        {"1G1ZB5ST1GF100001", "Chevrolet", "Silverado", 2023},
        {"1FTFW1ET3BFA10001", "Ford", "F-150", 2023},
        {"JT2BF22K1Y0123456", "Toyota", "Camry", 2023},
        {"WBA3A5C5XDF123456", "BMW", "3 Series", 2023},
        {"WVWZZZ1KZ8W123456", "Volkswagen", "Golf", 2023},
        {"WDB2030461F123456", "Mercedes-Benz", "C-Class", 2023},
        {"JHMFA36216S012345", "Honda", "Civic", 2023},
        {"JN1AZ4EH6BM123456", "Nissan", "Altima", 2023}
    };
}

std::vector<SeedDTC> SeedData::getDefaultDTCs() {
    return {
        {"P0101", "Mass Air Flow Circuit Range/Performance", "major"},
        {"P0102", "Mass Air Flow Circuit Low Input", "major"},
        {"P0103", "Mass Air Flow Circuit High Input", "major"},
        {"P0113", "Intake Air Temperature Circuit High Input", "minor"},
        {"P0118", "Engine Coolant Temperature Circuit High Input", "major"},
        {"P0121", "Throttle/Pedal Position Sensor Circuit", "major"},
        {"P0171", "System Too Lean Bank 1", "moderate"},
        {"P0172", "System Too Rich Bank 1", "moderate"},
        {"P0174", "System Too Lean Bank 2", "moderate"},
        {"P0300", "Random/Multiple Cylinder Misfire Detected", "major"},
        {"P0301", "Cylinder 1 Misfire Detected", "major"},
        {"P0302", "Cylinder 2 Misfire Detected", "major"},
        {"P0303", "Cylinder 3 Misfire Detected", "major"},
        {"P0304", "Cylinder 4 Misfire Detected", "major"},
        {"P0420", "Catalyst System Efficiency Below Threshold", "moderate"},
        {"P0430", "Catalyst System Efficiency Below Threshold Bank 2", "moderate"},
        {"P0440", "Evaporative Emission System Malfunction", "minor"},
        {"P0442", "Evaporative Emission System Leak Detected Small", "minor"},
        {"P0455", "Evaporative Emission System Leak Detected Large", "minor"},
        {"P0500", "Vehicle Speed Sensor Malfunction", "major"},
        {"P0505", "Idle Control System Malfunction", "moderate"},
        {"P0507", "Idle Control System RPM Higher Than Expected", "minor"},
        {"P0601", "Internal Control Module Memory Checksum Error", "critical"},
        {"P0606", "ECU/PCM Processor Fault", "critical"},
        {"P0620", "Generator Control Circuit Malfunction", "moderate"},
        {"P0700", "Transmission Control System Malfunction", "major"},
        {"P1135", "Pedal Position Sensor Circuit Malfunction", "major"},
        {"P2101", "Throttle Actuator Control Motor Circuit", "major"},
        {"P2119", "Throttle Actuator Control Throttle Body Range", "major"},
        {"P2122", "Throttle/Pedal Position Sensor Circuit Low", "major"},
        {"P2135", "Throttle/Pedal Position Sensor Correlation", "major"}
    };
}

std::vector<SeedParameter> SeedData::getDefaultParameters() {
    return {
        {"engine_rpm", 0, "rpm"},
        {"vehicle_speed", 0, "km/h"},
        {"coolant_temp", 95, "°C"},
        {"intake_temp", 35, "°C"},
        {"maf", 3.5, "g/s"},
        {"throttle_pos", 0, "%"},
        {"fuel_pressure", 350, "kPa"},
        {"fuel_level", 50, "%"},
        {"battery_voltage", 12.6, "V"},
        {"timing_advance", 10, "°"},
        {"intake_pressure", 101, "kPa"},
        {"o2_voltage_b1s1", 0.45, "V"},
        {"o2_voltage_b1s2", 0.45, "V"}
    };
}

std::vector<std::string> SeedData::getSeedSQL() {
    return {
        "INSERT OR IGNORE INTO parameters (name, value, unit) VALUES ('engine_rpm', 0, 'rpm');",
        "INSERT OR IGNORE INTO parameters (name, value, unit) VALUES ('vehicle_speed', 0, 'km/h');",
        "INSERT OR IGNORE INTO parameters (name, value, unit) VALUES ('coolant_temp', 95, '°C');",
        "INSERT OR IGNORE INTO parameters (name, value, unit) VALUES ('intake_temp', 35, '°C');",
        "INSERT OR IGNORE INTO parameters (name, value, unit) VALUES ('maf', 3.5, 'g/s');",
        "INSERT OR IGNORE INTO simulation_profiles (name, type, config) VALUES ('urban', 'urban', '{\"rpm_base\": 1500, \"speed_base\": 40}');",
        "INSERT OR IGNORE INTO simulation_profiles (name, type, config) VALUES ('highway', 'highway', '{\"rpm_base\": 2500, \"speed_base\": 110}');",
        "INSERT OR IGNORE INTO simulation_profiles (name, type, config) VALUES ('mixed', 'mixed', '{\"rpm_base\": 2000, \"speed_base\": 70}');"
    };
}

} // namespace ecumult
