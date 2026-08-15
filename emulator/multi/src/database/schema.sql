-- ECU Multi-Emulator Master Database Schema
-- SQLite3

-- Vehicles table
CREATE TABLE IF NOT EXISTS vehicles (
    vin TEXT PRIMARY KEY,
    make TEXT NOT NULL,
    model TEXT NOT NULL,
    year INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Diagnostic Trouble Codes
CREATE TABLE IF NOT EXISTS dtcs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    vin TEXT NOT NULL,
    code INTEGER NOT NULL,
    description TEXT,
    severity TEXT DEFAULT 'minor' CHECK(severity IN ('minor', 'moderate', 'major', 'critical')),
    is_pending INTEGER DEFAULT 0,
    is_permanent INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (vin) REFERENCES vehicles(vin)
);

-- Simulation/ECU Parameters
CREATE TABLE IF NOT EXISTS parameters (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT UNIQUE NOT NULL,
    value REAL NOT NULL DEFAULT 0,
    unit TEXT,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Freeze frames (snapshots when DTC sets)
CREATE TABLE IF NOT EXISTS freeze_frames (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    vin TEXT NOT NULL,
    dtc_code INTEGER NOT NULL,
    data BLOB,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (vin) REFERENCES vehicles(vin)
);

-- Driving cycle simulation profiles
CREATE TABLE IF NOT EXISTS simulation_profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT UNIQUE NOT NULL,
    type TEXT NOT NULL CHECK(type IN ('urban', 'highway', 'mixed', 'custom', 'steady')),
    config BLOB,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- CAN traffic log
CREATE TABLE IF NOT EXISTS can_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,
    can_id INTEGER NOT NULL,
    data BLOB NOT NULL,
    direction TEXT NOT NULL CHECK(direction IN ('RX', 'TX')),
    manufacturer TEXT
);

-- Diagnostic session history
CREATE TABLE IF NOT EXISTS session_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    manufacturer TEXT,
    service TEXT,
    sub_func TEXT,
    response TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Prometheus-compatible metrics storage
CREATE TABLE IF NOT EXISTS metrics (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    value REAL NOT NULL,
    labels TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Indexes for performance
CREATE INDEX IF NOT EXISTS idx_dtcs_vin ON dtcs(vin);
CREATE INDEX IF NOT EXISTS idx_dtcs_code ON dtcs(code);
CREATE INDEX IF NOT EXISTS idx_dtcs_severity ON dtcs(severity);
CREATE INDEX IF NOT EXISTS idx_can_logs_ts ON can_logs(timestamp);
CREATE INDEX IF NOT EXISTS idx_can_logs_id ON can_logs(can_id);
CREATE INDEX IF NOT EXISTS idx_metrics_name ON metrics(name);
CREATE INDEX IF NOT EXISTS idx_session_log_ts ON session_log(timestamp);

-- Seed data for simulation profiles
INSERT OR IGNORE INTO simulation_profiles (name, type, config) VALUES
    ('urban', 'urban', '{"rpm_min": 800, "rpm_max": 3000, "speed_max": 60, "avg_speed": 30, "stops_per_km": 3}'),
    ('highway', 'highway', '{"rpm_min": 1500, "rpm_max": 3500, "speed_max": 130, "avg_speed": 110, "stops_per_km": 0}'),
    ('mixed', 'mixed', '{"rpm_min": 800, "rpm_max": 3500, "speed_max": 130, "avg_speed": 65, "stops_per_km": 1}'),
    ('steady', 'steady', '{"rpm_min": 1500, "rpm_max": 1500, "speed_max": 80, "avg_speed": 80, "stops_per_km": 0}'),
    ('custom', 'custom', '{"rpm_min": 1000, "rpm_max": 4000, "speed_max": 160, "avg_speed": 80, "stops_per_km": 2}');

-- Seed default parameters
INSERT OR IGNORE INTO parameters (name, value, unit) VALUES
    ('engine_rpm', 0, 'rpm'),
    ('vehicle_speed', 0, 'km/h'),
    ('coolant_temp', 95, '°C'),
    ('intake_temp', 35, '°C'),
    ('maf', 3.5, 'g/s'),
    ('throttle_pos', 0, '%'),
    ('fuel_pressure', 350, 'kPa'),
    ('fuel_level', 50, '%'),
    ('battery_voltage', 12.6, 'V'),
    ('timing_advance', 10, '°'),
    ('intake_pressure', 101, 'kPa'),
    ('o2_voltage_b1s1', 0.45, 'V'),
    ('o2_voltage_b1s2', 0.45, 'V');
