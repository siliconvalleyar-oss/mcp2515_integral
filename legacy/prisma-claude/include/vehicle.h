#ifndef VEHICLE_H
#define VEHICLE_H

#include <cstdint>
#include <string>
#include <mutex>
#include <map>

// Perfiles de conducción disponibles para la simulación dinámica
enum class DrivingProfile {
    RALENTI,
    URBANO,
    CARRETERA,
    AGRESIVO,
    PERSONALIZADO   // Los valores se mantienen estables salvo fijación manual
};

enum class GearState : uint8_t {
    PARK = 0,
    REVERSE,
    NEUTRAL,
    DRIVE,
    FIRST,
    SECOND,
    THIRD,
    FOURTH,
    FIFTH
};

// Snapshot inmutable de todos los parámetros de la ECU emulada en un instante dado
struct VehicleParameters {
    double speedKmh;          // PID 0D - velocidad del vehículo
    double rpm;                // PID 0C - revoluciones del motor
    double engineTempC;        // PID 05 - temperatura del refrigerante
    double engineLoadPct;      // PID 04 - carga calculada del motor
    double fuelPressureKpa;    // PID 0A - presión de combustible
    double batteryVoltage;     // PID 42 - voltaje del módulo de control
    double throttlePct;        // PID 11 - posición del acelerador
    double intakeAirTempC;     // PID 0F - temperatura de aire de admisión
    double mafRateGs;          // PID 10 - caudal másico de aire
    double fuelLevelPct;       // PID 2F - nivel de combustible
    double timingAdvanceDeg;   // PID 0E - avance de encendido
    double ambientTempC;       // PID 46 - temperatura ambiente
    uint32_t runTimeSec;       // PID 1F - tiempo desde arranque del motor
    double odometerKm;         // PID A6 - distancia acumulada
    GearState gear;            // Estado de marcha (uso interno / diagnóstico extendido)
    bool checkEngineOn;        // Testigo de falla (MIL)
    bool engineRunning;
};

// Simulador del comportamiento dinámico de un Chevrolet Prisma.
// Es seguro llamar a sus métodos desde múltiples hilos (protegido por mutex interno).
class Vehicle {
public:
    Vehicle();

    // Avanza la simulación "dt" segundos según el perfil activo
    void update(double deltaSeconds);

    void setProfile(DrivingProfile profile);
    DrivingProfile getProfile() const;

    void start();  // Enciende el motor y habilita la actualización dinámica
    void stop();   // Detiene el motor (rpm/velocidad a 0)
    bool isRunning() const;

    VehicleParameters getSnapshot() const;

    // Fuerza un valor fijo para un parámetro (deja de simularse automáticamente)
    // Claves válidas: speed, rpm, temp, load, fuelPressure, battery,
    //                 fuelLevel, throttle, maf, iat, timing, gear, odometer
    void setFixedValue(const std::string &param, double value);
    void clearFixedValue(const std::string &param);
    void clearAllFixedValues();

    static std::string profileToString(DrivingProfile p);
    static std::string gearToString(GearState g);

private:
    mutable std::mutex _mutex;
    VehicleParameters _params;
    DrivingProfile _profile;
    bool _running;
    double _simTime;

    std::map<std::string, double> _fixedValues;

    bool isFixed(const std::string &param) const;
    double targetFor(const std::string &param) const;
};

#endif // VEHICLE_H
