#ifndef SCANNER_CONFIG_HPP
#define SCANNER_CONFIG_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace Scanner {

struct VehicleInfo {
    std::string make;
    std::string model;
    std::string year;
    std::string ecu;      // p.ej. "ECM/PCM", "TCM"
    std::string diagAddr; // p.ej. "7E0"

    bool empty() const { return make.empty(); }
};

struct EcuOption {
    std::string name;
    std::string diagAddr;
};

struct VehicleModel {
    std::string model;
    std::vector<std::string> years;
    std::vector<EcuOption> ecus;
};

struct VehicleMake {
    std::string make;
    std::vector<VehicleModel> models;
};

// Catalogo de vehiculos soportados (config/vehicles.json).
class VehicleDB {
public:
    bool load(const std::string& path);
    const std::vector<VehicleMake>& makes() const { return makes_; }
    bool empty() const { return makes_.empty(); }

private:
    std::vector<VehicleMake> makes_;
};

struct Snapshot {
    std::string timestamp;
    std::string label;
    nlohmann::json data;

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["timestamp"] = timestamp;
        j["label"] = label;
        j["data"] = data;
        return j;
    }

    static Snapshot fromJson(const nlohmann::json& j) {
        Snapshot s;
        s.timestamp = j.value("timestamp", "");
        s.label = j.value("label", "");
        s.data = j.contains("data") ? j["data"] : nlohmann::json::object();
        return s;
    }
};

// Configuracion persistida del scanner (idioma, unidades, brillo, vehiculo,
// snapshots de datos). Se guarda como JSON en <dir>/config.json.
class Config {
public:
    static Config& instance();

    bool load();
    bool save();

    std::string configPath() const { return path_; }

    // Ajustes
    std::string language() const { return language_; }
    void setLanguage(const std::string& lang) { language_ = lang; }

    bool metricUnits() const { return units_ == "metric"; }
    std::string units() const { return units_; }
    void setUnits(const std::string& units) { units_ = units; }

    uint8_t brightness() const { return brightness_; }
    void setBrightness(uint8_t value) { brightness_ = value; }

    uint8_t contrast() const { return contrast_; }
    void setContrast(uint8_t value) { contrast_ = value; }

    bool beepEnabled() const { return beep_; }
    void setBeepEnabled(bool enabled) { beep_ = enabled; }

    bool autoScanOnBoot() const { return autoScanOnBoot_; }
    void setAutoScanOnBoot(bool enabled) { autoScanOnBoot_ = enabled; }

    // Vehiculo seleccionado
    VehicleInfo vehicle() const { return vehicle_; }
    void setVehicle(const VehicleInfo& v) { vehicle_ = v; }

    // Snapshots (Data Manager)
    const std::vector<Snapshot>& snapshots() const { return snapshots_; }
    void addSnapshot(const Snapshot& s);
    void clearSnapshots();

    // PIDs de la lista personalizada (hex)
    std::vector<uint8_t> customPIDs() const { return customPIDs_; }
    void setCustomPIDs(const std::vector<uint8_t>& pids) { customPIDs_ = pids; }

    void setConfigDir(const std::string& dir) { configDir_ = dir; }

private:
    Config() = default;
    void applyDefaults();
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

    std::string configDir_;
    std::string path_;
    std::string language_ = "es";
    std::string units_ = "metric";
    uint8_t brightness_ = 128;
    uint8_t contrast_ = 255;
    bool beep_ = false;
    bool autoScanOnBoot_ = false;
    VehicleInfo vehicle_;
    std::vector<Snapshot> snapshots_;
    std::vector<uint8_t> customPIDs_;
    static constexpr size_t MAX_SNAPSHOTS = 20;
};

} // namespace Scanner

#endif // SCANNER_CONFIG_HPP
