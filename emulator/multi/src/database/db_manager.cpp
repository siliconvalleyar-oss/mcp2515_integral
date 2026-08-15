#include "db_manager.hpp"
#include <iostream>
#include <sstream>

namespace ecumult {

DBManager::DBManager(const std::string& db_path)
    : db_path_(db_path)
    , db_(nullptr)
    , open_(false) {}

DBManager::~DBManager() {
    close();
}

bool DBManager::open() {
    if (open_) return true;
    if (sqlite3_open(db_path_.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "[DB] Error opening database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    open_ = true;
    return true;
}

bool DBManager::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    open_ = false;
    return true;
}

bool DBManager::execute(const std::string& sql) {
    if (!db_) return false;
    char* err_msg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "[DB] SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

bool DBManager::executeWithCallback(const std::string& sql,
    std::function<bool(int cols, char** values, char** col_names)> callback) {
    if (!db_) return false;
    auto cb_data = std::make_unique<decltype(callback)>(std::move(callback));
    char* err_msg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), callbackWrapper, cb_data.get(), &err_msg) != SQLITE_OK) {
        std::cerr << "[DB] SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

int DBManager::callbackWrapper(void* data, int cols, char** values, char** col_names) {
    auto* cb = static_cast<std::function<bool(int, char**, char**)>*>(data);
    return (*cb)(cols, values, col_names) ? 0 : 1;
}

bool DBManager::tableExists(const std::string& name) {
    std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='" + name + "'";
    bool exists = false;
    executeWithCallback(sql, [&exists](int cols, char** values, char**) -> bool {
        (void)cols;
        if (values[0]) exists = true;
        return true;
    });
    return exists;
}

bool DBManager::runMigrations() {
    if (!open_) return false;

    std::string schema = R"(
        CREATE TABLE IF NOT EXISTS vehicles (
            vin TEXT PRIMARY KEY,
            make TEXT NOT NULL,
            model TEXT NOT NULL,
            year INTEGER,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS dtcs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            vin TEXT NOT NULL,
            code INTEGER NOT NULL,
            description TEXT,
            severity TEXT DEFAULT 'minor',
            is_pending INTEGER DEFAULT 0,
            is_permanent INTEGER DEFAULT 0,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (vin) REFERENCES vehicles(vin)
        );

        CREATE TABLE IF NOT EXISTS parameters (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            value REAL NOT NULL DEFAULT 0,
            unit TEXT,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS freeze_frames (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            vin TEXT NOT NULL,
            dtc_code INTEGER NOT NULL,
            data BLOB,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (vin) REFERENCES vehicles(vin)
        );

        CREATE TABLE IF NOT EXISTS simulation_profiles (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            type TEXT NOT NULL,
            config BLOB,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS can_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            can_id INTEGER NOT NULL,
            data BLOB NOT NULL,
            direction TEXT NOT NULL,
            manufacturer TEXT
        );

        CREATE TABLE IF NOT EXISTS session_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            manufacturer TEXT,
            service TEXT,
            sub_func TEXT,
            response TEXT,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS metrics (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            value REAL NOT NULL,
            labels TEXT,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";

    return execute(schema);
}

bool DBManager::isOpen() const {
    return open_;
}

bool DBManager::insertVehicle(const std::string& vin, const std::string& make,
                               const std::string& model, int year) {
    std::string sql = "INSERT OR REPLACE INTO vehicles (vin, make, model, year) VALUES ('";
    sql += vin + "', '" + make + "', '" + model + "', " + std::to_string(year) + ")";
    return execute(sql);
}

bool DBManager::updateParameter(const std::string& name, double value) {
    std::string sql = "INSERT OR REPLACE INTO parameters (name, value, unit, updated_at) VALUES ('";
    sql += name + "', " + std::to_string(value) + ", '', CURRENT_TIMESTAMP)";
    return execute(sql);
}

double DBManager::getParameter(const std::string& name, double default_val) {
    std::string sql = "SELECT value FROM parameters WHERE name='" + name + "'";
    double result = default_val;
    executeWithCallback(sql, [&result](int cols, char** values, char**) -> bool {
        if (cols > 0 && values[0]) result = std::stod(values[0]);
        return true;
    });
    return result;
}

bool DBManager::saveDTCs(const std::string& vin, const std::vector<uint16_t>& codes) {
    execute("DELETE FROM dtcs WHERE vin='" + vin + "'");
    for (auto code : codes) {
        std::string sql = "INSERT INTO dtcs (vin, code) VALUES ('";
        sql += vin + "', " + std::to_string(code) + ")";
        if (!execute(sql)) return false;
    }
    return true;
}

std::vector<uint16_t> DBManager::loadDTCs(const std::string& vin) {
    std::vector<uint16_t> codes;
    executeWithCallback("SELECT code FROM dtcs WHERE vin='" + vin + "'",
        [&codes](int cols, char** values, char**) -> bool {
            if (cols > 0 && values[0]) codes.push_back(std::stoi(values[0]));
            return true;
        });
    return codes;
}

} // namespace ecumult
