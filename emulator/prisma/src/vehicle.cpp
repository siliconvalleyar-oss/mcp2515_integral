#include "vehicle.h"

#include <algorithm>
#include <cmath>
#include <iostream>

// ---------------------------------------------------------------------------
//  Console
// ---------------------------------------------------------------------------
void Console::print(const std::string& s) {
    std::lock_guard<std::mutex> lk(mtx);
    std::cout << s << std::flush;
}

void Console::println(const std::string& s) {
    std::lock_guard<std::mutex> lk(mtx);
    std::cout << s << '\n' << std::flush;
}

// ---------------------------------------------------------------------------
//  Vehicle
// ---------------------------------------------------------------------------
Vehicle::Vehicle() {
    defs = {
        { "velocidad",           "Velocidad",              "km/h",   0, 240,       1 },
        { "rpm",                 "RPM del motor",          "rpm",    0, 8000,     50 },
        { "marcha",              "Marcha (0=N 1-5 6=R)",   "",       0, 6,         1 },
        { "carga_motor",         "Carga del motor",        "%",      0, 100,       1 },
        { "mariposa",            "Apertura de mariposa",   "%",      0, 100,       1 },
        { "temp_refrigerante",   "Temp. refrigerante",     "°C",   -40, 215,       1 },
        { "temp_admision",       "Temp. de admisión",      "°C",   -40, 215,       1 },
        { "temp_aceite",         "Temp. de aceite",        "°C",   -40, 215,       1 },
        { "temp_catalizador",    "Temp. catalizador",      "°C",   -40, 2000,      1 },
        { "temp_ambiente",       "Temp. ambiente",         "°C",   -40, 60,        1 },
        { "presion_combustible", "Presión combustible",    "kPa",    0, 1000,      5 },
        { "voltaje_bateria",     "Voltaje de batería",     "V",      0, 20,      0.1 },
        { "maf",                 "Flujo MAF",              "g/s",    0, 655,       1 },
        { "map",                 "Presión MAP",            "kPa",    0, 255,       1 },
        { "sonda_o2",            "Sonda O2",               "V",      0, 1.275,  0.01 },
        { "stft1",               "Fuel trim corto (B1)",   "%",   -100, 100,     0.1 },
        { "ltft1",               "Fuel trim largo (B1)",   "%",   -100, 100,     0.1 },
        { "evap_purge",          "Purga EVAP",             "%",      0, 100,       1 },
        { "avance_encendido",    "Avance de encendido",    "°",    -64,  64,     0.5 },
        { "baro",                "Presión barométrica",    "kPa",    50, 110,       1 },
        { "nivel_combustible",   "Nivel de combustible",   "%",      0, 100,       1 },
        { "distancia",           "Odómetro",               "km",     0, 1000000,   1 },
        { "tiempo_motor",        "Tiempo motor encendido", "s",      0, 1000000,   1 },
    };

    // Estado inicial: motor apagado, motor frío, batería en reposo.
    values["velocidad"] = 0.0;
    values["rpm"] = 0.0;
    values["marcha"] = 0.0;
    values["carga_motor"] = 0.0;
    values["mariposa"] = 0.0;
    values["temp_refrigerante"] = 25.0;
    values["temp_admision"] = 25.0;
    values["temp_aceite"] = 25.0;
    values["temp_catalizador"] = 25.0;
    values["temp_ambiente"] = 25.0;
    values["presion_combustible"] = 0.0;
    values["voltaje_bateria"] = 12.5;
    values["maf"] = 0.0;
    values["map"] = 101.3;
    values["sonda_o2"] = 0.45;
    values["stft1"] = 0.0;
    values["ltft1"] = 0.0;
    values["evap_purge"] = 0.0;
    values["avance_encendido"] = 8.0;
    values["baro"] = 102.0;
    values["nivel_combustible"] = 70.0;
    values["distancia"] = 12345.6;
    values["tiempo_motor"] = 0.0;

    for (const auto& p : defs)
        autoFlags[p.key] = (p.key != "temp_ambiente");   // ambiente es manual
    engineOn_ = false;
}

double Vehicle::value(const std::string& key) const {
    const auto it = values.find(key);
    return it != values.end() ? it->second : 0.0;
}

void Vehicle::setValue(const std::string& key, double v) {
    values[key] = v;
}

bool Vehicle::isAuto(const std::string& key) const {
    const auto it = autoFlags.find(key);
    return it != autoFlags.end() ? it->second : true;
}

void Vehicle::setAuto(const std::string& key, bool autoMode) {
    autoFlags[key] = autoMode;
}

// ---------------------------------------------------------------------------
//  Simulator
// ---------------------------------------------------------------------------
double Simulator::clamp(double v, double lo, double hi) {
    return std::max(lo, std::min(hi, v));
}

double Simulator::noise() {
    lcgState = lcgState * 6364136223846793005ULL + 1442695040888963407ULL;
    const double v =
        static_cast<double>((lcgState >> 33) & 0x1FFFFFFF) / 268435455.0;
    return v * 2.0 - 1.0;
}

void Simulator::setProfile(Profile p) {
    prof = p;
    phase = (p == Profile::Idle || p == Profile::City) ? Phase::Stop
                                                      : Phase::Accel;
    phaseT = 0.0;
    target = 112.0;
    cruiseDur = 25.0;
    goingUp = true;
}

const char* Simulator::profileName() const {
    switch (prof) {
        case Profile::Idle:    return "Ralentí";
        case Profile::City:    return "Ciudad";
        case Profile::Highway: return "Autopista";
        case Profile::Sport:   return "Deportivo";
    }
    return "?";
}

int Simulator::gearFor(double spd) const {
    if (spd < 2.0) return 0;
    double t1 = 15, t2 = 30, t3 = 50, t4 = 75;
    if (prof == Profile::Sport) { t1 = 20; t2 = 45; t3 = 78; t4 = 112; }
    if (spd < t1) return 1;
    if (spd < t2) return 2;
    if (spd < t3) return 3;
    if (spd < t4) return 4;
    return 5;
}

void Simulator::approach(const std::string& key, double target, double k) {
    const double cur = veh->value(key);
    veh->setValue(key, cur + (target - cur) * clamp(k, 0.0, 1.0));
}

void Simulator::tick(double dt) {
    std::lock_guard<std::mutex> lk(veh->mtx);
    tGlobal += dt;
    const double amb = veh->value("temp_ambiente");

    // ---------------- Motor apagado: decaimiento ----------------
    if (!run) {
        veh->setEngineOn(false);
        veh->setValue("rpm", std::max(0.0, veh->value("rpm") - 500.0 * dt));
        veh->setValue("velocidad",
                      std::max(0.0, veh->value("velocidad") - 12.0 * dt));
        veh->setValue("carga_motor",
                      std::max(0.0, veh->value("carga_motor") - 40.0 * dt));
        veh->setValue("mariposa",
                      std::max(0.0, veh->value("mariposa") - 40.0 * dt));
        veh->setValue("maf", std::max(0.0, veh->value("maf") - 20.0 * dt));
        approach("map", 101.3, dt / 1.0);
        approach("presion_combustible", 0.0, dt / 0.5);
        approach("voltaje_bateria", 12.4, dt / 1.0);
        approach("sonda_o2", 0.45, dt / 0.5);
        approach("temp_refrigerante", amb, dt / 240.0);
        approach("temp_admision", amb, dt / 60.0);
        approach("temp_aceite", amb, dt / 300.0);
        approach("temp_catalizador", amb, dt / 90.0);
        approach("marcha", 0.0, dt / 0.2);
        veh->setValue("evap_purge",
                      std::max(0.0, veh->value("evap_purge") - 5.0 * dt));
        approach("stft1", 0.0, dt / 0.5);
        approach("ltft1", 0.0, dt / 5.0);
        return;
    }

    // ---------------- Motor encendido: simulación por perfil ----------------
    veh->setEngineOn(true);
    double spd = veh->value("velocidad");
    double accel = 0.0;   // km/h por segundo
    phaseT += dt;

    switch (prof) {
        case Profile::Idle:
            phase = Phase::Stop;
            accel = 0.0;
            break;

        case Profile::City:   // ciclo: semáforo -> acelerar -> crucero -> freno
            switch (phase) {
                case Phase::Stop:   accel = 0.0;
                    if (phaseT >= 8.0)  { phase = Phase::Accel;  phaseT = 0.0; }
                    break;
                case Phase::Accel:  accel = 9.0;
                    if (spd >= 55)      { phase = Phase::Cruise; phaseT = 0.0; }
                    break;
                case Phase::Cruise: accel = 0.0;
                    if (phaseT >= 12.0) { phase = Phase::Decel;  phaseT = 0.0; }
                    break;
                case Phase::Decel:  accel = -10.0;
                    if (spd <= 0.0)     { phase = Phase::Stop;   phaseT = 0.0; }
                    break;
            }
            break;

        case Profile::Highway:  // 90 <-> 112 km/h con cruceros
            switch (phase) {
                case Phase::Accel:  accel = 6.0;
                    if (spd >= target) { phase = Phase::Cruise; phaseT = 0.0; }
                    break;
                case Phase::Cruise: accel = 0.0;
                    if (phaseT >= cruiseDur) {
                        if (goingUp) { goingUp = false; target = 90.0;
                                       cruiseDur = 15.0; phase = Phase::Decel; }
                        else         { goingUp = true;  target = 112.0;
                                       cruiseDur = 25.0; phase = Phase::Accel; }
                        phaseT = 0.0;
                    }
                    break;
                case Phase::Decel:  accel = -7.0;
                    if (spd <= target) { phase = Phase::Cruise; phaseT = 0.0; }
                    break;
                case Phase::Stop:   accel = 0.0;
                    break;
            }
            break;

        case Profile::Sport:    // aceleraciones y frenadas fuertes
            switch (phase) {
                case Phase::Accel:  accel = 17.0;
                    if (spd >= 150) { phase = Phase::Cruise; phaseT = 0.0; }
                    break;
                case Phase::Cruise: accel = 0.0;
                    if (phaseT >= 8.0) { phase = Phase::Decel; phaseT = 0.0; }
                    break;
                case Phase::Decel:  accel = -15.0;
                    if (spd <= 60)  { phase = Phase::Accel;  phaseT = 0.0; }
                    break;
                case Phase::Stop:   accel = 0.0;
                    break;
            }
            break;
    }

    // Velocidad
    if (veh->isAuto("velocidad")) {
        spd = (prof == Profile::Idle) ? 0.0 : std::max(0.0, spd + accel * dt);
        veh->setValue("velocidad", spd);
    } else {
        spd = veh->value("velocidad");
    }

    // Marcha
    int gear = 0;
    if (veh->isAuto("marcha")) {
        gear = gearFor(spd);
        veh->setValue("marcha", static_cast<double>(gear));
    } else {
        gear = static_cast<int>(veh->value("marcha"));
    }

    // Mariposa
    double throttle = 0.0;
    if (veh->isAuto("mariposa")) {
        if (accel > 0.5)                  throttle = 28.0 + accel * 3.2;
        else if (phase == Phase::Cruise)  throttle = 11.0 + (spd / 240.0) * 10.0;
        else if (phase == Phase::Decel)   throttle = 2.0;
        else                              throttle = 0.0;
        throttle = clamp(throttle + noise() * 2.0, 0.0, 100.0);
        veh->setValue("mariposa", throttle);
    } else {
        throttle = veh->value("mariposa");
    }

    // Carga del motor
    if (veh->isAuto("carga_motor")) {
        double load;
        if (accel > 0.5)                 load = 25.0 + accel * 3.5;
        else if (phase == Phase::Cruise) load = 18.0 + (spd / 220.0) * 22.0;
        else if (phase == Phase::Decel)  load = 8.0;
        else                             load = 14.0 + noise() * 3.0;
        load = clamp(load + noise() * 2.0, 5.0, 97.0);
        veh->setValue("carga_motor", load);
    }
    const double load = veh->value("carga_motor");

    // RPM
    if (veh->isAuto("rpm")) {
        double rpm;
        if (gear >= 1 && gear <= 5 && spd > 2.0) {
            static const double ratio[5] = { 140.0, 85.0, 60.0, 45.0, 31.0 };
            rpm = spd * ratio[gear - 1] + throttle * 18.0;
        } else {
            rpm = 800.0 + noise() * 15.0;   // ralentí / punto muerto
        }
        rpm = clamp(rpm, 750.0, 6500.0);
        veh->setValue("rpm", rpm);
    }

    // Parámetros derivados
    if (veh->isAuto("map"))
        veh->setValue("map", clamp(24.0 + load * 0.72 + noise() * 1.5, 20.0, 102.0));
    if (veh->isAuto("maf"))
        veh->setValue("maf", std::max(0.0, load * veh->value("rpm") / 3200.0));
    if (veh->isAuto("presion_combustible"))
        veh->setValue("presion_combustible",
                      390.0 + veh->value("rpm") / 150.0 + noise() * 3.0);
    if (veh->isAuto("voltaje_bateria"))
        veh->setValue("voltaje_bateria",
                      14.2 + 0.05 * std::sin(tGlobal * 3.0) + noise() * 0.05);
    if (veh->isAuto("sonda_o2"))
        veh->setValue("sonda_o2",
                      clamp(0.45 + 0.38 * std::sin(tGlobal * 7.0), 0.10, 0.88));

    // Fuel trims (calibración de mezcla): el corto corrige según la sonda O2,
    // el largo integra lentamente la corrección sostenida.
    if (veh->isAuto("stft1")) {
        const double o2 = veh->value("sonda_o2");
        veh->setValue("stft1",
                      clamp((o2 - 0.45) * 200.0 + noise() * 2.0, -10.0, 10.0));
    }
    if (veh->isAuto("ltft1")) {
        const double st = veh->value("stft1");
        const double lt = veh->value("ltft1");
        veh->setValue("ltft1",
                      clamp(lt + (st * 0.8 - lt) * (dt / 45.0), -15.0, 15.0));
    }

    // Válvula solenoide de purga EVAP: activa en crucero, mínima en
    // aceleración fuerte y 0 en ralentí / deceleración.
    if (veh->isAuto("evap_purge")) {
        double purge = 0.0;
        if (phase == Phase::Cruise) purge = 25.0 + (spd / 240.0) * 35.0;
        else if (accel > 0.5)       purge = 5.0;
        veh->setValue("evap_purge", clamp(purge + noise() * 3.0, 0.0, 100.0));
    }

    // Avance de encendido: avanza con el régimen y se retrasa con la carga.
    if (veh->isAuto("avance_encendido")) {
        const double rpm = veh->value("rpm");
        const double adv = 8.0 + (rpm / 800.0) * 6.0 - load * 0.10 + noise() * 1.0;
        veh->setValue("avance_encendido", clamp(adv, 0.0, 45.0));
    }
    if (veh->isAuto("baro"))
        veh->setValue("baro", 102.0 + noise() * 0.2);   // presión atmosférica
    if (veh->isAuto("temp_refrigerante")) {
        const double ect = veh->value("temp_refrigerante");
        veh->setValue("temp_refrigerante",
                      ect + (90.0 - ect) * (dt / (110.0 - load * 0.6)));
    }
    if (veh->isAuto("temp_admision"))
        veh->setValue("temp_admision", amb + 8.0 + noise() * 2.0);
    if (veh->isAuto("temp_aceite")) {
        const double oil = veh->value("temp_aceite");
        const double ect = veh->value("temp_refrigerante");
        veh->setValue("temp_aceite", oil + ((ect + 6.0) - oil) * (dt / 160.0));
    }
    if (veh->isAuto("temp_catalizador")) {
        const double cat = veh->value("temp_catalizador");
        veh->setValue("temp_catalizador",
                      cat + ((400.0 + load * 3.5) - cat) * (dt / 55.0));
    }
    if (veh->isAuto("nivel_combustible"))
        veh->setValue("nivel_combustible",
                      std::max(0.0, veh->value("nivel_combustible") - 0.00015 * dt));
    if (veh->isAuto("distancia"))
        veh->setValue("distancia", veh->value("distancia") + spd * dt / 3600.0);
    if (veh->isAuto("tiempo_motor"))
        veh->setValue("tiempo_motor", veh->value("tiempo_motor") + dt);
}
