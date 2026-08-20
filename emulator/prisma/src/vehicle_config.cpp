#include "vehicle_config.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
//  Constructor
// ---------------------------------------------------------------------------
VehicleConfig::VehicleConfig()
    : filePath_("config/vehicles.json") {}

// ---------------------------------------------------------------------------
//  Parsear un VehicleInfo desde un objeto JSON
// ---------------------------------------------------------------------------
static VehicleConfig::VehicleInfo parseVehicle(const json& j) {
    VehicleConfig::VehicleInfo v;
    v.id = j.value("id", "");
    v.brand = j.value("brand", "");
    v.model = j.value("model", "");
    v.year = j.value("year", 2024);
    v.engine = j.value("engine", "");
    v.transmission = j.value("transmission", "");
    v.displacement_cc = j.value("displacement_cc", 1600);
    v.fuel = j.value("fuel", "Gasolina");

    if (j.contains("parameters")) {
        const auto& p = j["parameters"];
        v.rpm_idle = p.value("rpm_idle", 800);
        v.rpm_redline = p.value("rpm_redline", 6500);
        v.max_speed_kmh = p.value("max_speed_kmh", 200);
        v.torque_max_nm = p.value("torque_max_nm", 150);
        v.power_max_hp = p.value("power_max_hp", 120);
        v.weight_kg = p.value("weight_kg", 1200);
        v.fuel_tank_l = p.value("fuel_tank_l", 50);
        v.final_drive = p.value("final_drive", 3.73);
        v.temp_ambiente_default = p.value("temp_ambiente_default", 25.0);
        v.voltaje_bateria_default = p.value("voltaje_bateria_default", 12.5);
        v.oil_life_default = p.value("oil_life_default", 100.0);
        v.distance_clear_km = p.value("distance_clear_km", 0);
        v.odometro_km = p.value("odometro_km", 0);

        if (p.contains("gear_ratios") && p["gear_ratios"].is_array()) {
            v.gear_ratios.clear();
            for (const auto& r : p["gear_ratios"])
                v.gear_ratios.push_back(r.get<double>());
        }
        if (p.contains("dtcs") && p["dtcs"].is_array()) {
            for (const auto& d : p["dtcs"])
                v.dtcs.push_back(d.get<std::string>());
        }
        if (p.contains("dtcs_pending") && p["dtcs_pending"].is_array()) {
            for (const auto& d : p["dtcs_pending"])
                v.dtcs_pending.push_back(d.get<std::string>());
        }
    }
    return v;
}

// ---------------------------------------------------------------------------
//  Serializar un VehicleInfo a JSON
// ---------------------------------------------------------------------------
static json serializeVehicle(const VehicleConfig::VehicleInfo& v) {
    json j;
    j["id"] = v.id;
    j["brand"] = v.brand;
    j["model"] = v.model;
    j["year"] = v.year;
    j["engine"] = v.engine;
    j["transmission"] = v.transmission;
    j["displacement_cc"] = v.displacement_cc;
    j["fuel"] = v.fuel;

    json p;
    p["rpm_idle"] = v.rpm_idle;
    p["rpm_redline"] = v.rpm_redline;
    p["max_speed_kmh"] = v.max_speed_kmh;
    p["torque_max_nm"] = v.torque_max_nm;
    p["power_max_hp"] = v.power_max_hp;
    p["weight_kg"] = v.weight_kg;
    p["fuel_tank_l"] = v.fuel_tank_l;
    p["gear_ratios"] = v.gear_ratios;
    p["final_drive"] = v.final_drive;
    p["temp_ambiente_default"] = v.temp_ambiente_default;
    p["voltaje_bateria_default"] = v.voltaje_bateria_default;
    p["oil_life_default"] = v.oil_life_default;
    p["distance_clear_km"] = v.distance_clear_km;
    p["odometro_km"] = v.odometro_km;
    p["dtcs"] = v.dtcs;
    p["dtcs_pending"] = v.dtcs_pending;
    j["parameters"] = p;

    return j;
}

// ---------------------------------------------------------------------------
//  Load
// ---------------------------------------------------------------------------
bool VehicleConfig::load() {
    std::ifstream file(filePath_);
    if (!file.is_open()) return false;

    json j;
    try {
        file >> j;
    } catch (...) {
        return false;
    }

    currentId_ = j.value("current_vehicle", "");

    if (j.contains("custom_vehicle") && !j["custom_vehicle"].is_null()) {
        custom_ = parseVehicle(j["custom_vehicle"]);
    }

    vehicles_.clear();
    if (j.contains("vehicles") && j["vehicles"].is_array()) {
        for (const auto& v : j["vehicles"]) {
            vehicles_.push_back(parseVehicle(v));
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
//  Save
// ---------------------------------------------------------------------------
bool VehicleConfig::save() {
    json j;
    j["version"] = 1;
    j["description"] = "Presets de vehículos para el emulador OBD2 ECU";
    j["current_vehicle"] = currentId_;

    if (custom_.brand.empty()) {
        j["custom_vehicle"] = nullptr;
    } else {
        j["custom_vehicle"] = serializeVehicle(custom_);
    }

    json arr = json::array();
    for (const auto& v : vehicles_) {
        arr.push_back(serializeVehicle(v));
    }
    j["vehicles"] = arr;

    std::ofstream file(filePath_);
    if (!file.is_open()) return false;
    file << j.dump(2) << "\n";
    return true;
}

// ---------------------------------------------------------------------------
//  List
// ---------------------------------------------------------------------------
std::vector<std::string> VehicleConfig::listVehicles() const {
    std::vector<std::string> ids;
    for (const auto& v : vehicles_)
        ids.push_back(v.id);
    return ids;
}

// ---------------------------------------------------------------------------
//  Get
// ---------------------------------------------------------------------------
VehicleConfig::VehicleInfo VehicleConfig::getVehicle(const std::string& id) const {
    for (const auto& v : vehicles_) {
        if (v.id == id) return v;
    }
    // Si es "custom" y existe, devolver el custom
    if (id == "custom" && !custom_.brand.empty()) return custom_;
    // Fallback: primer vehículo
    return vehicles_.empty() ? VehicleInfo{} : vehicles_.front();
}

// ---------------------------------------------------------------------------
//  Current
// ---------------------------------------------------------------------------
VehicleConfig::VehicleInfo VehicleConfig::currentVehicle() const {
    return getVehicle(currentId_);
}

// ---------------------------------------------------------------------------
//  Set current
// ---------------------------------------------------------------------------
void VehicleConfig::setCurrentVehicle(const std::string& id) {
    currentId_ = id;
}

// ---------------------------------------------------------------------------
//  Update custom
// ---------------------------------------------------------------------------
void VehicleConfig::updateCustom(const VehicleInfo& info) {
    custom_ = info;
    custom_.id = "custom";
}
