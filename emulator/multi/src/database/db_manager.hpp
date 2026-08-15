#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <sqlite3.h>

namespace ecumult {

class DBManager {
public:
    DBManager(const std::string& db_path = "/etc/ecu_emulator/emulator.db");
    ~DBManager();

    bool open();
    bool close();
    bool execute(const std::string& sql);
    bool executeWithCallback(const std::string& sql,
        std::function<bool(int cols, char** values, char** col_names)> callback);

    // Convenience methods
    bool tableExists(const std::string& name);
    bool runMigrations();
    bool isOpen() const;

    // Vehicle data
    bool insertVehicle(const std::string& vin, const std::string& make,
                       const std::string& model, int year);
    bool updateParameter(const std::string& name, double value);
    double getParameter(const std::string& name, double default_val = 0.0);

    // DTC persistence
    bool saveDTCs(const std::string& vin, const std::vector<uint16_t>& codes);
    std::vector<uint16_t> loadDTCs(const std::string& vin);

private:
    std::string db_path_;
    sqlite3* db_;
    bool open_;

    static int callbackWrapper(void* data, int cols, char** values, char** col_names);
};

} // namespace ecumult
