#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
//  Consola: salida con exclusión mutua (hilos CAN + simulación + menú)
// ---------------------------------------------------------------------------
class Console {
public:
    void print(const std::string& s);    // sin salto de línea
    void println(const std::string& s);  // con salto de línea

private:
    std::mutex mtx;
};

// ---------------------------------------------------------------------------
//  Modelo del vehículo (parámetros OBD2)
//
//  Cada parámetro tiene un valor y un modo:
//    AUTO -> lo actualiza el simulador (datos dinámicos)
//    FIJO -> valor constante configurado por el usuario
//
//  NOTA: value()/setValue()/isAuto()/setAuto() NO bloquean. El llamador debe
//  mantener el mutex `mtx` (p. ej. std::lock_guard<std::mutex> lk(veh.mtx)).
// ---------------------------------------------------------------------------
class Vehicle {
public:
    struct Param {
        std::string key;       // clave interna
        std::string nombre;    // nombre mostrado (español)
        std::string unidad;
        double min, max, step;
    };

    Vehicle();

    const std::vector<Param>& params() const { return defs; }

    double value(const std::string& key) const;
    void   setValue(const std::string& key, double v);
    bool   isAuto(const std::string& key) const;
    void   setAuto(const std::string& key, bool autoMode);

    bool engineOn() const { return engineOn_; }
    void setEngineOn(bool on) { engineOn_ = on; }

    mutable std::mutex mtx;

private:
    std::vector<Param> defs;
    std::map<std::string, double> values;
    std::map<std::string, bool>   autoFlags;
    bool engineOn_ = false;
};

// ---------------------------------------------------------------------------
//  Simulador: genera los datos dinámicos de la ECU según un perfil
// ---------------------------------------------------------------------------
enum class Profile { Idle, City, Highway, Sport };

class Simulator {
public:
    explicit Simulator(Vehicle* v) : veh(v) {}

    void start() { run = true; }
    void stop()  { run = false; }
    bool running() const { return run; }

    void setProfile(Profile p);
    Profile profile() const { return prof; }
    const char* profileName() const;

    // Avanza la simulación un paso de dt segundos (hilo de simulación, 10 Hz).
    void tick(double dt);

private:
    enum class Phase { Accel, Cruise, Decel, Stop };

    Vehicle* veh;
    Profile prof = Profile::City;
    bool run = false;
    Phase phase = Phase::Stop;
    double phaseT = 0.0;
    double target = 112.0;    // velocidad objetivo (autopista)
    double cruiseDur = 25.0;  // duración del crucero (autopista)
    bool goingUp = true;      // subiendo/bajando (autopista)
    double tGlobal = 0.0;
    uint64_t lcgState = 0x12345678ULL;

    double noise();                                   // [-1, 1] pseudoaleatorio
    int gearFor(double spd) const;
    void approach(const std::string& key, double target, double k);
    static double clamp(double v, double lo, double hi);
};
