#include "elm327.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

ELM327::ELM327(MCP2515& can, Vehicle& vehicle)
    : can_(can),
      vehicle_(vehicle),
      echo_(true),
      spaces_(true),
      headers_(false),
      linefeeds_(false) {
}

std::string ELM327::normalize(const std::string& command) {
    std::string result;

    for (char c : command) {
        if (c == '\r' || c == '\n' ||
            c == ' ' || c == '\t') {
            continue;
        }

        result +=
            static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(c)
                )
            );
    }

    return result;
}

std::string ELM327::byteToHex(uint8_t value) const {
    std::ostringstream ss;

    ss << std::uppercase
       << std::hex
       << std::setw(2)
       << std::setfill('0')
       << static_cast<int>(value);

    return ss.str();
}

std::string ELM327::processCommand(
    const std::string& command) {

    std::string cmd = normalize(command);

    if (cmd.empty()) {
        return "";
    }

    if (cmd.rfind("AT", 0) == 0) {
        return processAT(cmd);
    }

    return processOBD(cmd);
}

std::string ELM327::processAT(
    const std::string& command) {

    if (command == "ATZ") {
        echo_ = true;
        spaces_ = true;
        headers_ = false;
        linefeeds_ = false;

        return "ELM327 v1.5\r\r>";
    }

    if (command == "ATI") {
        return "ELM327 v1.5\r\r>";
    }

    if (command == "AT@1") {
        return "OBDII VEHICLE SIMULATOR\r\r>";
    }

    if (command == "ATE0") {
        echo_ = false;
        return "OK\r\r>";
    }

    if (command == "ATE1") {
        echo_ = true;
        return "OK\r\r>";
    }

    if (command == "ATS0") {
        spaces_ = false;
        return "OK\r\r>";
    }

    if (command == "ATS1") {
        spaces_ = true;
        return "OK\r\r>";
    }

    if (command == "ATH0") {
        headers_ = false;
        return "OK\r\r>";
    }

    if (command == "ATH1") {
        headers_ = true;
        return "OK\r\r>";
    }

    if (command == "ATL0") {
        linefeeds_ = false;
        return "OK\r\r>";
    }

    if (command == "ATL1") {
        linefeeds_ = true;
        return "OK\r\r>";
    }

    if (command == "ATSP0") {
        return "OK\r\r>";
    }

    if (command == "ATSP6") {
        return "OK\r\r>";
    }

    if (command == "ATDP") {
        return "ISO 15765-4 (CAN 11/500)\r\r>";
    }

    if (command == "ATDPN") {
        return "6\r\r>";
    }

    if (command == "ATCAF0") {
        return "OK\r\r>";
    }

    if (command == "ATCAF1") {
        return "OK\r\r>";
    }

    if (command == "ATCRA7E8") {
        return "OK\r\r>";
    }

    if (command == "ATWS") {
        return "ELM327 v1.5\r\r>";
    }

    if (command == "ATSH7E0") {
        return "OK\r\r>";
    }

    if (command == "ATSH7E8") {
        return "OK\r\r>";
    }

    if (command == "ATCM") {
        return "OK\r\r>";
    }

    if (command == "ATMA") {
        return "SEARCHING...\r\r>";
    }

    return "?\r\r>";
}

std::string ELM327::formatResponse(
    uint8_t mode,
    uint8_t pid,
    const uint8_t* data,
    uint8_t length) {

    std::ostringstream ss;

    if (headers_) {
        ss << "7E8";

        if (spaces_) {
            ss << " ";
        }
    }

    uint8_t payloadLength =
        static_cast<uint8_t>(length + 2);

    ss << byteToHex(payloadLength);

    if (spaces_) {
        ss << " ";
    }

    ss << byteToHex(
        static_cast<uint8_t>(mode + 0x40)
    );

    if (spaces_) {
        ss << " ";
    }

    ss << byteToHex(pid);

    for (uint8_t i = 0; i < length; ++i) {
        if (spaces_) {
            ss << " ";
        }

        ss << byteToHex(data[i]);
    }

    ss << "\r";

    if (linefeeds_) {
        ss << "\n";
    }

    ss << ">";

    return ss.str();
}

bool ELM327::sendObdResponse(
    uint8_t mode,
    uint8_t pid,
    const uint8_t* data,
    uint8_t length) {

    MCP2515::CanFrame frame;

    frame.id = 0x7E8;
    frame.extended = false;

    frame.dlc =
        static_cast<uint8_t>(length + 3);

    if (frame.dlc > 8) {
        return false;
    }

    frame.data[0] =
        static_cast<uint8_t>(length + 2);

    frame.data[1] =
        static_cast<uint8_t>(mode + 0x40);

    frame.data[2] = pid;

    for (uint8_t i = 0; i < length; ++i) {
        frame.data[3 + i] = data[i];
    }

    while (frame.dlc < 8) {
        frame.data[frame.dlc++] = 0x00;
    }

    return can_.send(frame);
}

std::string ELM327::supportedPids0100() {
    /*
     * PIDs soportados:
     *
     * 01-05
     * 0B-0D
     * 0F
     * 10-11
     * 1F
     * 2F
     * 42
     */
    uint32_t bits = 0;

    auto setBit = [&bits](int pid) {
        if (pid >= 1 && pid <= 32) {
            bits |= (1u << (32 - pid));
        }
    };

    for (int pid :
         {1, 4, 5, 11, 12, 13, 15, 16, 17,
          31, 32}) {
        setBit(pid);
    }

    uint8_t data[4] = {
        static_cast<uint8_t>((bits >> 24) & 0xFF),
        static_cast<uint8_t>((bits >> 16) & 0xFF),
        static_cast<uint8_t>((bits >> 8) & 0xFF),
        static_cast<uint8_t>(bits & 0xFF)
    };

    sendObdResponse(0x01, 0x00, data, 4);

    return formatResponse(0x01, 0x00, data, 4);
}

std::string ELM327::supportedPids0120() {
    uint32_t bits = 0;

    /*
     * 21-40.
     */
    for (int pid : {31, 32, 33, 34}) {
        int pos = 32 - (pid - 0x20);

        if (pos >= 0 && pos < 32) {
            bits |= (1u << pos);
        }
    }

    uint8_t data[4] = {
        static_cast<uint8_t>((bits >> 24) & 0xFF),
        static_cast<uint8_t>((bits >> 16) & 0xFF),
        static_cast<uint8_t>((bits >> 8) & 0xFF),
        static_cast<uint8_t>(bits & 0xFF)
    };

    sendObdResponse(0x01, 0x20, data, 4);

    return formatResponse(0x01, 0x20, data, 4);
}

std::string ELM327::processOBD(
    const std::string& command) {

    if (command.size() < 4) {
        return "?\r\r>";
    }

    /*
     * Solo soportamos servicios OBD estándar.
     */
    uint8_t mode =
        static_cast<uint8_t>(
            std::stoi(command.substr(0, 2), nullptr, 16)
        );

    uint8_t pid =
        static_cast<uint8_t>(
            std::stoi(command.substr(2, 2), nullptr, 16)
        );

    if (mode != 0x01) {
        /*
         * Respuesta genérica de servicio no soportado.
         */
        return "NO DATA\r\r>";
    }

    if (pid == 0x00) {
        return supportedPids0100();
    }

    if (pid == 0x20) {
        return supportedPids0120();
    }

    Vehicle::Parameters p =
        vehicle_.getParameters();

    uint8_t data[4] = {};
    uint8_t length = 0;

    switch (pid) {

        case 0x04:
            /*
             * Calculated engine load:
             * A = load * 255 / 100
             */
            data[0] =
                static_cast<uint8_t>(
                    std::clamp(
                        p.engineLoad * 2.55,
                        0.0,
                        255.0
                    )
                );

            length = 1;
            break;

        case 0x05:
            /*
             * Coolant:
             * A - 40
             */
            data[0] =
                static_cast<uint8_t>(
                    std::clamp(
                        p.coolantC + 40.0,
                        0.0,
                        255.0
                    )
                );

            length = 1;
            break;

        case 0x0B:
            /*
             * MAP kPa.
             */
            data[0] =
                static_cast<uint8_t>(
                    std::clamp(
                        p.mapKpa,
                        0.0,
                        255.0
                    )
                );

            length = 1;
            break;

        case 0x0C: {
            /*
             * RPM = ((A*256)+B)/4
             */
            uint16_t raw =
                static_cast<uint16_t>(
                    std::clamp(
                        p.rpm * 4.0,
                        0.0,
                        65535.0
                    )
                );

            data[0] =
                static_cast<uint8_t>(raw >> 8);

            data[1] =
                static_cast<uint8_t>(raw & 0xFF);

            length = 2;
            break;
        }

        case 0x0D:
            /*
             * Vehicle speed.
             */
            data[0] =
                static_cast<uint8_t>(
                    std::clamp(
                        p.speedKmh,
                        0.0,
                        255.0
                    )
                );

            length = 1;
            break;

        case 0x0F:
            /*
             * Intake air temperature.
             */
            data[0] =
                static_cast<uint8_t>(
                    std::clamp(
                        p.intakeTempC + 40.0,
                        0.0,
                        255.0
                    )
                );

            length = 1;
            break;

        case 0x10: {
            /*
             * MAF:
             * ((A*256)+B)/100
             */
            uint16_t raw =
                static_cast<uint16_t>(
                    std::clamp(
                        p.mafGps * 100.0,
                        0.0,
                        65535.0
                    )
                );

            data[0] =
                static_cast<uint8_t>(raw >> 8);

            data[1] =
                static_cast<uint8_t>(raw & 0xFF);

            length = 2;
            break;
        }

        case 0x11:
            /*
             * Throttle position.
             */
            data[0] =
                static_cast<uint8_t>(
                    std::clamp(
                        p.throttle * 2.55,
                        0.0,
                        255.0
                    )
                );

            length = 1;
            break;

        case 0x1F: {
            /*
             * Run time since engine start.
             */
            uint16_t seconds =
                static_cast<uint16_t>(
                    std::min<uint64_t>(
                        65535,
                        60 + (p.rpm > 0 ? 120 : 0)
                    )
                );

            data[0] =
                static_cast<uint8_t>(
                    seconds >> 8
                );

            data[1] =
                static_cast<uint8_t>(
                    seconds & 0xFF
                );

            length = 2;
            break;
        }

        case 0x2F:
            /*
             * Fuel tank level.
             */
            data[0] = 180;
            length = 1;
            break;

        case 0x42: {
            /*
             * Control module voltage:
             * ((A*256)+B)/1000
             */
            uint16_t raw =
                static_cast<uint16_t>(
                    std::clamp(
                        p.batteryVoltage * 1000.0,
                        0.0,
                        65535.0
                    )
                );

            data[0] =
                static_cast<uint8_t>(
                    raw >> 8
                );

            data[1] =
                static_cast<uint8_t>(
                    raw & 0xFF
                );

            length = 2;
            break;
        }

        /*
         * PID 0x0A:
         * Fuel pressure = A * 3 kPa.
         *
         * Es una aproximación de banco de pruebas.
         */
        case 0x0A:
            data[0] =
                static_cast<uint8_t>(
                    std::clamp(
                        p.fuelPressureKpa / 3.0,
                        0.0,
                        255.0
                    )
                );

            length = 1;
            break;

        /*
         * Algunos equipos de diagnóstico utilizan
         * PIDs propietarios para información de marcha.
         * No existe un PID universal que permita asumir
         * que "gear" funcionará igual en todos los vehículos.
         */
        case 0xA4:
            data[0] =
                static_cast<uint8_t>(
                    std::clamp(
                        p.gear,
                        0,
                        15
                    )
                );

            length = 1;
            break;

        default:
            return "NO DATA\r\r>";
    }

    sendObdResponse(
        mode,
        pid,
        data,
        length
    );

    return formatResponse(
        mode,
        pid,
        data,
        length
    );
}
