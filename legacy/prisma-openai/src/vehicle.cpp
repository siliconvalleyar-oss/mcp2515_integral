#include "vehicle.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <iostream>
#include <random>

Vehicle::Vehicle()
    : running_(false),
      mode_(SimulationMode::DYNAMIC),
      profile_(Profile::NORMAL),
      tick_(0) {
    params_.speedKmh = 0.0;
    params_.rpm = 850.0;
    params_.coolantC = 88.0;
    params_.engineLoad = 18.0;
    params_.fuelPressureKpa = 400.0;
    params_.batteryVoltage = 13.8;
    params_.throttle = 4.0;
    params_.intakeTempC = 28.0;
    params_.mapKpa = 35.0;
    params_.mafGps = 2.5;
    params_.gear = 1;
}

Vehicle::~Vehicle() {
    stop();
}

void Vehicle::start() {
    if (running_) {
        return;
    }

    running_ = true;
    thread_ = std::thread(&Vehicle::simulationLoop, this);
}

void Vehicle::stop() {
    running_ = false;

    if (thread_.joinable()) {
        thread_.join();
    }
}

bool Vehicle::running() const {
    return running_;
}

void Vehicle::setMode(SimulationMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    mode_ = mode;
}

Vehicle::SimulationMode Vehicle::mode() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mode_;
}

void Vehicle::setProfile(Profile profile) {
    std::lock_guard<std::mutex> lock(mutex_);
    profile_ = profile;
}

Vehicle::Profile Vehicle::profile() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return profile_;
}

void Vehicle::setParameter(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (name == "speed") {
        params_.speedKmh = std::clamp(value, 0.0, 250.0);
    } else if (name == "rpm") {
        params_.rpm = std::clamp(value, 0.0, 8000.0);
    } else if (name == "coolant") {
        params_.coolantC = std::clamp(value, -40.0, 150.0);
    } else if (name == "load") {
        params_.engineLoad = std::clamp(value, 0.0, 100.0);
    } else if (name == "fuel") {
        params_.fuelPressureKpa = std::clamp(value, 0.0, 1000.0);
    } else if (name == "voltage") {
        params_.batteryVoltage = std::clamp(value, 8.0, 18.0);
    } else if (name == "throttle") {
        params_.throttle = std::clamp(value, 0.0, 100.0);
    } else if (name == "iat") {
        params_.intakeTempC = std::clamp(value, -40.0, 100.0);
    } else if (name == "map") {
        params_.mapKpa = std::clamp(value, 0.0, 255.0);
    } else if (name == "maf") {
        params_.mafGps = std::clamp(value, 0.0, 655.35);
    } else if (name == "gear") {
        params_.gear = static_cast<int>(
            std::clamp(value, 0.0, 6.0)
        );
    }
}

Vehicle::Parameters Vehicle::getParameters() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return params_;
}

std::string Vehicle::modeName(SimulationMode mode) {
    switch (mode) {
        case SimulationMode::DYNAMIC:
            return "DYNAMIC";
        case SimulationMode::FIXED:
            return "FIXED";
        case SimulationMode::RANDOM:
            return "RANDOM";
    }

    return "UNKNOWN";
}

std::string Vehicle::profileName(Profile profile) {
    switch (profile) {
        case Profile::NORMAL:
            return "NORMAL";
        case Profile::SPORT:
            return "SPORT";
        case Profile::ECONOMY:
            return "ECONOMY";
        case Profile::FAILSAFE:
            return "FAILSAFE";
    }

    return "UNKNOWN";
}

void Vehicle::simulationLoop() {
    std::default_random_engine rng(
        static_cast<unsigned>(
            std::chrono::steady_clock::now()
                .time_since_epoch()
                .count()
        )
    );

    std::uniform_real_distribution<double> noise(-1.0, 1.0);

    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            ++tick_;

            if (mode_ == SimulationMode::DYNAMIC) {
                double targetRpm = 850.0;

                if (params_.speedKmh > 0.0) {
                    targetRpm =
                        850.0 +
                        params_.speedKmh * 32.0;

                    if (profile_ == Profile::SPORT) {
                        targetRpm += 500.0;
                    }

                    if (profile_ == Profile::ECONOMY) {
                        targetRpm -= 150.0;
                    }
                }

                targetRpm = std::clamp(targetRpm, 850.0, 6500.0);

                params_.rpm +=
                    (targetRpm - params_.rpm) * 0.08;

                double targetLoad =
                    12.0 +
                    params_.speedKmh * 0.30 +
                    params_.throttle * 0.50;

                if (profile_ == Profile::SPORT) {
                    targetLoad += 10.0;
                }

                if (profile_ == Profile::ECONOMY) {
                    targetLoad -= 5.0;
                }

                params_.engineLoad +=
                    (targetLoad - params_.engineLoad) * 0.10;

                params_.engineLoad =
                    std::clamp(params_.engineLoad, 0.0, 100.0);

                double targetThrottle =
                    3.0 +
                    params_.speedKmh * 0.08;

                params_.throttle +=
                    (targetThrottle - params_.throttle) * 0.05;

                params_.throttle =
                    std::clamp(params_.throttle, 0.0, 100.0);

                double targetMap =
                    30.0 +
                    params_.engineLoad * 0.65;

                params_.mapKpa +=
                    (targetMap - params_.mapKpa) * 0.08;

                params_.mapKpa =
                    std::clamp(params_.mapKpa, 20.0, 100.0);

                double targetMaf =
                    2.0 +
                    params_.rpm *
                    params_.engineLoad *
                    0.000012;

                params_.mafGps +=
                    (targetMaf - params_.mafGps) * 0.10;

                double targetFuel =
                    400.0 +
                    params_.engineLoad * 0.4;

                params_.fuelPressureKpa +=
                    (targetFuel - params_.fuelPressureKpa) * 0.04;

                double targetCoolant =
                    88.0 +
                    params_.engineLoad * 0.08;

                params_.coolantC +=
                    (targetCoolant - params_.coolantC) * 0.01;

                params_.batteryVoltage =
                    13.7 +
                    std::sin(tick_ * 0.05) * 0.15;

                params_.intakeTempC =
                    25.0 +
                    params_.engineLoad * 0.05;

                if (params_.speedKmh < 1.0) {
                    params_.gear = 1;
                } else if (params_.speedKmh < 20.0) {
                    params_.gear = 1;
                } else if (params_.speedKmh < 40.0) {
                    params_.gear = 2;
                } else if (params_.speedKmh < 65.0) {
                    params_.gear = 3;
                } else if (params_.speedKmh < 90.0) {
                    params_.gear = 4;
                } else {
                    params_.gear = 5;
                }
            }

            if (mode_ == SimulationMode::RANDOM) {
                params_.speedKmh =
                    std::clamp(
                        params_.speedKmh + noise(rng) * 3.0,
                        0.0,
                        180.0
                    );

                params_.rpm =
                    std::clamp(
                        params_.rpm + noise(rng) * 120.0,
                        700.0,
                        6500.0
                    );

                params_.coolantC =
                    std::clamp(
                        params_.coolantC + noise(rng) * 0.5,
                        60.0,
                        110.0
                    );

                params_.batteryVoltage =
                    std::clamp(
                        params_.batteryVoltage + noise(rng) * 0.03,
                        12.0,
                        14.8
                    );
            }

            if (profile_ == Profile::FAILSAFE) {
                params_.rpm = 1200.0;
                params_.speedKmh = 0.0;
                params_.engineLoad = 20.0;
                params_.coolantC = 85.0;
                params_.throttle = 5.0;
                params_.gear = 1;
            }
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)
        );
    }
}

