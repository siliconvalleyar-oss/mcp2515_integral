#include "migrations.hpp"

namespace ecumult {

std::vector<Migrations::Migration> Migrations::getAll() {
    return initMigrations();
}

int Migrations::currentVersion() {
    return 1; // placeholder - would query DB
}

std::vector<Migrations::Migration> Migrations::initMigrations() {
    std::vector<Migration> migrations;
    migrations.push_back({1, "Initial schema", R"(
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
    )"});
    return migrations;
}

} // namespace ecumult
