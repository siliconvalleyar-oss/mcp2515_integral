#include "vehicle.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

static double clampd(double v, double lo, double hi) {
    return std::max(lo, std::min(hi, v));
}

static double randNoise(double amplitude) {
    return ((double)rand() / RAND_MAX - 0.5) * 2.0 * amplitude;
}

Vehicle::Vehicle()
    : _profile(DrivingProfile::RALENTI), _running(false), _simTime(0.0) {
    _params.speedKmh = 0;
    _params.rpm = 800;
    _params.engineTempC = 20;
    _params.engineLoadPct = 10;
    _params.fuelPressureKpa = 300;
    _params.batteryVoltage = 12.6;
    _params.throttlePct = 0;
    _params.intakeAirTempC = 25;
    _params.mafRateGs = 2.5;
    _params.fuelLevelPct = 75;
    _params.timingAdvanceDeg = 10;
    _params.ambientTempC = 22;
    _params.runTimeSec = 0;
    _params.odometerKm = 45230.0;
    _params.gear = GearState::PARK;
    _params.checkEngineOn = false;
    _params.engineRunning = false;
}

void Vehicle::setProfile(DrivingProfile profile) {
    std::lock_guard<std::mutex> lock(_mutex);
    _profile = profile;
}

DrivingProfile Vehicle::getProfile() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _profile;
}

void Vehicle::start() {
    std::lock_guard<std::mutex> lock(_mutex);
    _running = true;
    _params.engineRunning = true;
}

void Vehicle::stop() {
    std::lock_guard<std::mutex> lock(_mutex);
    _running = false;
    _params.engineRunning = false;
    _params.rpm = 0;
    _params.speedKmh = 0;
    _params.gear = GearState::PARK;
}

bool Vehicle::isRunning() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _running;
}

bool Vehicle::isFixed(const std::string &param) const {
    return _fixedValues.find(param) != _fixedValues.end();
}

double Vehicle::targetFor(const std::string &param) const {
    auto it = _fixedValues.find(param);
    return (it != _fixedValues.end()) ? it->second : 0.0;
}

void Vehicle::setFixedValue(const std::string &param, double value) {
    std::lock_guard<std::mutex> lock(_mutex);
    _fixedValues[param] = value;
}

void Vehicle::clearFixedValue(const std::string &param) {
    std::lock_guard<std::mutex> lock(_mutex);
    _fixedValues.erase(param);
}

void Vehicle::clearAllFixedValues() {
    std::lock_guard<std::mutex> lock(_mutex);
    _fixedValues.clear();
}

void Vehicle::update(double dt) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_running || dt <= 0.0) return;

    _simTime += dt;
    _params.runTimeSec = (uint32_t)_simTime;

    double targetSpeed = _params.speedKmh;
    double throttleTarget = _params.throttlePct;

    switch (_profile) {
        case DrivingProfile::RALENTI:
            targetSpeed = 0;
            throttleTarget = 2;
            break;
        case DrivingProfile::URBANO:
            targetSpeed = 45 + 15 * std::sin(_simTime / 20.0);
            throttleTarget = 30;
            break;
        case DrivingProfile::CARRETERA:
            targetSpeed = 110 + 10 * std::sin(_simTime / 30.0);
            throttleTarget = 45;
            break;
        case DrivingProfile::AGRESIVO:
            targetSpeed = 90 + 40 * std::fabs(std::sin(_simTime / 8.0));
            throttleTarget = 75;
            break;
        case DrivingProfile::PERSONALIZADO:
        default:
            // Mantiene el valor actual salvo que el usuario lo fije manualmente
            break;
    }

    // Velocidad
    if (!isFixed("speed")) {
        double diff = targetSpeed - _params.speedKmh;
        _params.speedKmh = clampd(_params.speedKmh + diff * 0.05 + randNoise(0.5), 0, 220);
    } else {
        _params.speedKmh = targetFor("speed");
    }

    // Marcha estimada según velocidad
    if (!isFixed("gear")) {
        if (_params.speedKmh < 1) _params.gear = GearState::NEUTRAL;
        else if (_params.speedKmh < 20) _params.gear = GearState::FIRST;
        else if (_params.speedKmh < 40) _params.gear = GearState::SECOND;
        else if (_params.speedKmh < 65) _params.gear = GearState::THIRD;
        else if (_params.speedKmh < 95) _params.gear = GearState::FOURTH;
        else _params.gear = GearState::FIFTH;
    }

    // RPM en función de velocidad y marcha (aproximación tipo Prisma 1.4/1.6)
    if (!isFixed("rpm")) {
        double baseRpm = 800.0;
        if (_params.gear != GearState::NEUTRAL && _params.gear != GearState::PARK) {
            double gearFactor = 45.0;
            baseRpm = 800 + _params.speedKmh * gearFactor / (1 + (int)_params.gear);
        }
        if (_profile == DrivingProfile::AGRESIVO) baseRpm += 800;
        _params.rpm = clampd(baseRpm + randNoise(50), 700, 6500);
    } else {
        _params.rpm = targetFor("rpm");
    }

    // Acelerador
    if (!isFixed("throttle")) {
        double diff = throttleTarget - _params.throttlePct;
        _params.throttlePct = clampd(_params.throttlePct + diff * 0.1 + randNoise(1.0), 0, 100);
    } else {
        _params.throttlePct = targetFor("throttle");
    }

    // Carga del motor
    if (!isFixed("load")) {
        _params.engineLoadPct = clampd(_params.throttlePct * 0.7 + randNoise(3.0), 0, 100);
    } else {
        _params.engineLoadPct = targetFor("load");
    }

    // Temperatura de motor: sube hasta ~90 °C y se estabiliza
    if (!isFixed("temp")) {
        if (_params.engineTempC < 90) {
            _params.engineTempC = clampd(_params.engineTempC + dt * 0.3, 0, 90);
        } else {
            _params.engineTempC = clampd(90 + randNoise(1.5), 85, 105);
        }
    } else {
        _params.engineTempC = targetFor("temp");
    }

    // Presión de combustible
    if (!isFixed("fuelPressure")) {
        _params.fuelPressureKpa = clampd(280 + _params.rpm * 0.02 + randNoise(5), 250, 400);
    } else {
        _params.fuelPressureKpa = targetFor("fuelPressure");
    }

    // Voltaje de batería (alternador cargando con motor en marcha)
    if (!isFixed("battery")) {
        double base = _params.engineRunning ? 14.2 : 12.6;
        _params.batteryVoltage = clampd(base + randNoise(0.15), 11.8, 14.7);
    } else {
        _params.batteryVoltage = targetFor("battery");
    }

    // Caudal másico de aire (MAF), aproximado según carga y rpm
    if (!isFixed("maf")) {
        _params.mafRateGs = clampd(
            2.0 + (_params.rpm / 1000.0) * (_params.engineLoadPct / 100.0) * 8.0 + randNoise(0.3),
            0.5, 60);
    } else {
        _params.mafRateGs = targetFor("maf");
    }

    // Temperatura de aire de admisión
    if (!isFixed("iat")) {
        _params.intakeAirTempC = clampd(_params.ambientTempC + _params.engineLoadPct * 0.1 + randNoise(0.5), 10, 60);
    } else {
        _params.intakeAirTempC = targetFor("iat");
    }

    // Avance de encendido
    if (!isFixed("timing")) {
        _params.timingAdvanceDeg = clampd(10 + (_params.rpm - 800) * 0.01 + randNoise(1.0), 0, 45);
    } else {
        _params.timingAdvanceDeg = targetFor("timing");
    }

    // Nivel de combustible: consumo lento proporcional a la carga
    if (!isFixed("fuelLevel")) {
        _params.fuelLevelPct = clampd(
            _params.fuelLevelPct - dt * 0.0005 * (1 + _params.engineLoadPct / 50.0), 0, 100);
    } else {
        _params.fuelLevelPct = targetFor("fuelLevel");
    }

    // Odómetro
    if (!isFixed("odometer")) {
        _params.odometerKm += (_params.speedKmh * dt) / 3600.0;
    } else {
        _params.odometerKm = targetFor("odometer");
    }
}

VehicleParameters Vehicle::getSnapshot() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _params;
}

std::string Vehicle::profileToString(DrivingProfile p) {
    switch (p) {
        case DrivingProfile::RALENTI:       return "Ralenti";
        case DrivingProfile::URBANO:        return "Urbano";
        case DrivingProfile::CARRETERA:     return "Carretera";
        case DrivingProfile::AGRESIVO:      return "Agresivo";
        case DrivingProfile::PERSONALIZADO: return "Personalizado";
    }
    return "Desconocido";
}

std::string Vehicle::gearToString(GearState g) {
    switch (g) {
        case GearState::PARK:    return "P";
        case GearState::REVERSE: return "R";
        case GearState::NEUTRAL: return "N";
        case GearState::DRIVE:   return "D";
        case GearState::FIRST:   return "1";
        case GearState::SECOND:  return "2";
        case GearState::THIRD:   return "3";
        case GearState::FOURTH:  return "4";
        case GearState::FIFTH:   return "5";
    }
    return "?";
}
