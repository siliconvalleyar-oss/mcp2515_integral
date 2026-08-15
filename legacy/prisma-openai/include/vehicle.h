#ifndef VEHICLE_H
#define VEHICLE_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

class Vehicle {
public:
    enum class SimulationMode {
        DYNAMIC,
        FIXED,
        RANDOM
    };

    enum class Profile {
        NORMAL,
        SPORT,
        ECONOMY,
        FAILSAFE
    };

    struct Parameters {
        double speedKmh;
        double rpm;
        double coolantC;
        double engineLoad;
        double fuelPressureKpa;
        double batteryVoltage;
        double throttle;
        double intakeTempC;
        double mapKpa;
        double mafGps;
        int gear;
    };

    Vehicle();
    ~Vehicle();

    void start();
    void stop();

    bool running() const;

    void setMode(SimulationMode mode);
    SimulationMode mode() const;

    void setProfile(Profile profile);
    Profile profile() const;

    void setParameter(const std::string& name, double value);

    Parameters getParameters() const;

    static std::string modeName(SimulationMode mode);
    static std::string profileName(Profile profile);

private:
    void simulationLoop();

    mutable std::mutex mutex_;
    Parameters params_;

    std::atomic<bool> running_;
    std::thread thread_;

    SimulationMode mode_;
    Profile profile_;

    uint64_t tick_;
};

#endif
