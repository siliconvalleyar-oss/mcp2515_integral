#pragma once

#include <cstdint>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
//  VehicleConfig: carga/guarda configuración de vehículos desde JSON.
//
//  Archivo: config/vehicles.json
//
//  Uso:
//    VehicleConfig cfg;
//    cfg.load();                         // carga config/vehicles.json
//    auto ids = cfg.listVehicles();      // ["chevrolet_prisma_2018", ...]
//    auto v = cfg.getVehicle("custom");  // VehicleInfo del custom
//    cfg.save();                         // guarda cambios (custom editado)
// ---------------------------------------------------------------------------
class VehicleConfig {
public:
    struct VehicleInfo {
        std::string id;
        std::string brand;
        std::string model;
        int year = 2024;
        std::string engine;
        std::string transmission;
        int displacement_cc = 1600;
        std::string fuel;

        // Parámetros del vehículo
        int rpm_idle = 800;
        int rpm_redline = 6500;
        int max_speed_kmh = 200;
        int torque_max_nm = 150;
        int power_max_hp = 120;
        int weight_kg = 1200;
        int fuel_tank_l = 50;
        std::vector<double> gear_ratios = {3.73, 2.14, 1.41, 1.03, 0.82, 3.17};
        double final_drive = 3.73;
        double temp_ambiente_default = 25.0;
        double voltaje_bateria_default = 12.5;
        double oil_life_default = 100.0;
        double distance_clear_km = 0;
        double odometro_km = 0;
        std::vector<std::string> dtcs;
        std::vector<std::string> dtcs_pending;
    };

    VehicleConfig();

    // Carga el archivo JSON. Devuelve true si ok.
    bool load();

    // Guarda el archivo JSON actual (con cambios de custom).
    bool save();

    // Lista de IDs de vehículos disponibles.
    std::vector<std::string> listVehicles() const;

    // Información de un vehículo por ID.
    VehicleInfo getVehicle(const std::string& id) const;

    // Información del vehículo actualmente seleccionado.
    VehicleInfo currentVehicle() const;

    // Establece el vehículo actual por ID.
    void setCurrentVehicle(const std::string& id);

    // Actualiza el vehículo custom con los valores dados.
    void updateCustom(const VehicleInfo& info);

    // Borra el vehículo custom.
    void clearCustom();

    // True si hay un custom configurado.
    bool hasCustom() const { return !custom_.brand.empty(); }

    // Ruta del archivo JSON.
    const std::string& filePath() const { return filePath_; }

private:
    std::string filePath_;
    std::string currentId_;
    VehicleInfo custom_;
    std::vector<VehicleInfo> vehicles_;
};
