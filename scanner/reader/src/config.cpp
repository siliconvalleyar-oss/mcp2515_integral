#include "obd2_rpi/config.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

OBD2Config OBD2Config::fromFile(const std::string& path) {
    OBD2Config cfg;
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[CONFIG] No se encontro " << path << ", usando valores por defecto\n";
        return cfg;
    }
    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        if (key == "bt_mac")               cfg.btMac = val;
        else if (key == "bt_channel")      cfg.btChannel = std::stoi(val);
        else if (key == "bt_timeout")      cfg.btConnectTimeoutSec = std::stoi(val);
        else if (key == "spi_device")      cfg.spiDevice = val;
        else if (key == "spi_speed")       cfg.spiSpeedHz = std::stoi(val);
        else if (key == "gpio_chip")       cfg.gpioChip = std::stoi(val);
        else if (key == "pin_dc")          cfg.pinDC = std::stoi(val);
        else if (key == "pin_rst")         cfg.pinReset = std::stoi(val);
        else if (key == "obd_poll_ms")     cfg.obdPollIntervalMs = std::stoi(val);
        else if (key == "display_ms")      cfg.displayRefreshMs = std::stoi(val);
        else if (key == "bt_timeout_us")   cfg.bluetoothTimeoutUs = std::stoi(val);
        else if (key == "auto_rotate")     cfg.autoRotate = (val == "true" || val == "1");
        else if (key == "rotate_interval") cfg.autoRotateIntervalSec = std::stoi(val);
        else if (key == "enable_gm")       cfg.enableGM = (val == "true" || val == "1");
        else if (key == "log_dir")         cfg.logDir = val;
        else if (key == "log_on_start")    cfg.logOnStart = (val == "true" || val == "1");
    }
    f.close();
    std::cout << "[CONFIG] Cargado: " << path << "\n";
    return cfg;
}

OBD2Config OBD2Config::fromArgs(int argc, char* argv[]) {
    OBD2Config cfg;
    if (argc > 1) cfg.btMac = argv[1];
    if (argc > 2) cfg.spiDevice = argv[2];
    if (argc > 3) cfg.pinDC = std::stoi(argv[3]);
    if (argc > 4) cfg.pinReset = std::stoi(argv[4]);
    if (argc > 5) { std::string p = argv[5]; if (p != "-") cfg = fromFile(p); }
    return cfg;
}

void OBD2Config::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return;
    f << "# OBD2 RPi Configuration\n";
    f << "# Created by obd2_rpi\n\n";
    f << "# Bluetooth\n";
    f << "bt_mac = " << btMac << "\n";
    f << "bt_channel = " << btChannel << "\n";
    f << "bt_timeout = " << btConnectTimeoutSec << "\n\n";
    f << "# SPI OLED\n";
    f << "spi_device = " << spiDevice << "\n";
    f << "spi_speed = " << spiSpeedHz << "\n";
    f << "gpio_chip = " << gpioChip << "\n";
    f << "pin_dc = " << pinDC << "\n";
    f << "pin_rst = " << pinReset << "\n\n";
    f << "# Timing\n";
    f << "obd_poll_ms = " << obdPollIntervalMs << "\n";
    f << "display_ms = " << displayRefreshMs << "\n";
    f << "bt_timeout_us = " << bluetoothTimeoutUs << "\n\n";
    f << "# Display\n";
    f << "auto_rotate = " << (autoRotate ? "true" : "false") << "\n";
    f << "rotate_interval = " << autoRotateIntervalSec << "\n\n";
    f << "# Features\n";
    f << "enable_gm = " << (enableGM ? "true" : "false") << "\n";
    f << "log_dir = " << logDir << "\n";
    f << "log_on_start = " << (logOnStart ? "true" : "false") << "\n";
    f.close();
}

void OBD2Config::print() const {
    std::cout << "MAC: " << btMac << "\n";
    std::cout << "SPI: " << spiDevice << " @" << spiSpeedHz/1000000 << "MHz\n";
    std::cout << "GPIO DC:" << pinDC << " RST:" << pinReset << "\n";
    std::cout << "Poll:" << obdPollIntervalMs << "ms  Disp:" << displayRefreshMs << "ms\n";
    std::cout << "AutoRotate:" << (autoRotate?"ON":"OFF") << " every " << autoRotateIntervalSec << "s\n";
    std::cout << "GM:" << (enableGM?"ON":"OFF") << "\n";
}
