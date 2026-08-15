#pragma once
#include <string>
#include <chrono>

struct OBD2Config {
    // Bluetooth
    std::string btMac           = "00:1D:A5:07:23:6E";
    int         btChannel       = 1;
    int         btConnectTimeoutSec = 30;

    // SPI OLED
    std::string spiDevice       = "/dev/spidev0.0";
    int         spiSpeedHz      = 8000000;
    int         gpioChip        = 0;
    int         pinDC           = 25;
    int         pinReset        = 17;

    // Timing (ms)
    int         obdPollIntervalMs  = 800;
    int         displayRefreshMs   = 400;
    int         bluetoothTimeoutUs = 600000;

    // Display
    bool        autoRotate      = true;
    int         autoRotateIntervalSec = 6;
    int         initialPage     = 0;

    // GM
    bool        enableGM        = true;
    int         gmCycleInterval = 10;   // cada N ciclos OBD
    int         gmSlowInterval  = 30;

    // DTC
    int         dtcCycleInterval = 60;

    // Logging
    std::string logDir          = ".";
    bool        logOnStart      = false;

    static OBD2Config fromFile(const std::string& path);
    static OBD2Config fromArgs(int argc, char* argv[]);
    void save(const std::string& path) const;
    void print() const;
};
