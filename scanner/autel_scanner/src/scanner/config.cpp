#include "scanner/config.hpp"
#include "scanner/event_log.hpp"

#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

namespace Scanner {

Config& Config::instance() {
    static Config cfg;
    return cfg;
}

bool VehicleDB::load(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        EventLog::instance().error("VehicleDB: no se pudo abrir " + path);
        return false;
    }

    try {
        nlohmann::json j;
        in >> j;
        makes_.clear();
        if (!j.contains("vehicles") || !j["vehicles"].is_array()) return false;
        for (const auto& mk : j["vehicles"]) {
            VehicleMake make;
            make.make = mk.value("make", "");
            for (const auto& md : mk.value("models", nlohmann::json::array())) {
                VehicleModel model;
                model.model = md.value("model", "");
                for (const auto& y : md.value("years", nlohmann::json::array())) {
                    model.years.push_back(y.get<std::string>());
                }
                for (const auto& e : md.value("ecus", nlohmann::json::array())) {
                    EcuOption ecu;
                    ecu.name = e.value("name", "");
                    ecu.diagAddr = e.value("diag_addr", "");
                    model.ecus.push_back(ecu);
                }
                make.models.push_back(model);
            }
            makes_.push_back(make);
        }
    } catch (const std::exception& e) {
        EventLog::instance().error(std::string("VehicleDB: JSON invalido (") + e.what() + ")");
        makes_.clear();
        return false;
    }

    EventLog::instance().info("VehicleDB: " + std::to_string(makes_.size()) + " marcas cargadas");
    return !makes_.empty();
}

void Config::applyDefaults() {
    language_ = "es";
    units_ = "metric";
    brightness_ = 128;
    contrast_ = 255;
    beep_ = false;
    autoScanOnBoot_ = false;
    vehicle_ = VehicleInfo{};
    snapshots_.clear();
    customPIDs_.clear();
}

bool Config::load() {
    // Ruta de config: dir configurado, o junto al ejecutable/../config, o cwd.
    if (configDir_.empty()) {
        // Preferir <ejecutable>/../config/config.json
        char self[4096];
        ssize_t n = readlink("/proc/self/exe", self, sizeof(self) - 1);
        if (n > 0) {
            self[n] = '\0';
            std::string exe(self);
            size_t slash = exe.find_last_of('/');
            std::string base = (slash == std::string::npos) ? "." : exe.substr(0, slash);
            configDir_ = base + "/../config";
        } else {
            configDir_ = "config";
        }
    }
    path_ = configDir_ + "/config.json";

    std::ifstream in(path_);
    if (!in.is_open()) {
        EventLog::instance().info("Config: no existe " + path_ + ", usando valores por defecto");
        applyDefaults();
        return false;
    }

    try {
        nlohmann::json j;
        in >> j;
        fromJson(j);
    } catch (const std::exception& e) {
        EventLog::instance().error(std::string("Config: JSON invalido (") + e.what() + "), usando defaults");
        applyDefaults();
        return false;
    }

    EventLog::instance().info("Config cargada desde " + path_);
    return true;
}

bool Config::save() {
    if (path_.empty()) {
        if (!load()) { /* path_ queda seteado por load() */ }
    }

    // Crear el directorio si falta.
    std::string dir = configDir_;
    if (!dir.empty() && dir != "config") {
        struct stat st{};
        if (stat(dir.c_str(), &st) != 0) {
            mkdir(dir.c_str(), 0755);
        }
    }

    std::ofstream out(path_, std::ios::trunc);
    if (!out.is_open()) {
        EventLog::instance().error("Config: no se pudo escribir " + path_);
        return false;
    }
    out << toJson().dump(2);
    out.close();

    EventLog::instance().info("Config guardada en " + path_);
    return true;
}

void Config::addSnapshot(const Snapshot& s) {
    snapshots_.push_back(s);
    if (snapshots_.size() > MAX_SNAPSHOTS) {
        snapshots_.erase(snapshots_.begin(),
                         snapshots_.begin() + (snapshots_.size() - MAX_SNAPSHOTS));
    }
    save();
}

void Config::clearSnapshots() {
    snapshots_.clear();
    save();
}

nlohmann::json Config::toJson() const {
    nlohmann::json j;
    j["language"] = language_;
    j["units"] = units_;
    j["brightness"] = brightness_;
    j["contrast"] = contrast_;
    j["beep"] = beep_;
    j["auto_scan_on_boot"] = autoScanOnBoot_;

    if (!vehicle_.empty()) {
        j["vehicle"] = {
            {"make", vehicle_.make},
            {"model", vehicle_.model},
            {"year", vehicle_.year},
            {"ecu", vehicle_.ecu},
            {"diag_addr", vehicle_.diagAddr}
        };
    }

    j["snapshots"] = nlohmann::json::array();
    for (const auto& s : snapshots_) {
        j["snapshots"].push_back(s.toJson());
    }

    j["custom_pids"] = nlohmann::json::array();
    for (uint8_t pid : customPIDs_) {
        j["custom_pids"].push_back(pid);
    }
    return j;
}

void Config::fromJson(const nlohmann::json& j) {
    language_ = j.value("language", "es");
    units_ = j.value("units", "metric");
    brightness_ = static_cast<uint8_t>(j.value("brightness", 128));
    contrast_ = static_cast<uint8_t>(j.value("contrast", 255));
    beep_ = j.value("beep", false);
    autoScanOnBoot_ = j.value("auto_scan_on_boot", false);

    if (j.contains("vehicle")) {
        const auto& v = j["vehicle"];
        vehicle_.make = v.value("make", "");
        vehicle_.model = v.value("model", "");
        vehicle_.year = v.value("year", "");
        vehicle_.ecu = v.value("ecu", "");
        vehicle_.diagAddr = v.value("diag_addr", "");
    }

    snapshots_.clear();
    if (j.contains("snapshots") && j["snapshots"].is_array()) {
        for (const auto& s : j["snapshots"]) {
            snapshots_.push_back(Snapshot::fromJson(s));
        }
    }

    customPIDs_.clear();
    if (j.contains("custom_pids") && j["custom_pids"].is_array()) {
        for (const auto& p : j["custom_pids"]) {
            if (p.is_number_unsigned()) {
                customPIDs_.push_back(static_cast<uint8_t>(p.get<unsigned int>()));
            }
        }
    }
}

} // namespace Scanner
