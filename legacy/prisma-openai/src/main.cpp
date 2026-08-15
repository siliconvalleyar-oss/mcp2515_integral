#include <bcm2835.h>
#include "elm327.h"
#include "mcp2515/mcp2515.h"
#include "vehicle.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

static void printParameters(const Vehicle& vehicle) {
    Vehicle::Parameters p =
        vehicle.getParameters();

    std::cout
        << "\n========== VEHICLE ==========\n"
        << std::fixed
        << std::setprecision(1)
        << "Velocidad       : "
        << p.speedKmh << " km/h\n"
        << "RPM             : "
        << p.rpm << "\n"
        << "Temperatura     : "
        << p.coolantC << " C\n"
        << "Carga motor     : "
        << p.engineLoad << " %\n"
        << "Presion fuel    : "
        << p.fuelPressureKpa << " kPa\n"
        << "Bateria         : "
        << p.batteryVoltage << " V\n"
        << "Acelerador      : "
        << p.throttle << " %\n"
        << "Temp. admision  : "
        << p.intakeTempC << " C\n"
        << "MAP             : "
        << p.mapKpa << " kPa\n"
        << "MAF             : "
        << p.mafGps << " g/s\n"
        << "Marcha          : "
        << p.gear << "\n"
        << "=============================\n";
}

static void printMenu(const Vehicle& vehicle) {
    std::cout
        << "\n"
        << "========================================\n"
        << "   CHEVROLET PRISMA OBD/CAN EMULATOR\n"
        << "========================================\n"
        << "Estado : "
        << (vehicle.running() ? "RUNNING" : "STOPPED")
        << "\n"
        << "Modo   : "
        << Vehicle::modeName(vehicle.mode())
        << "\n"
        << "Perfil : "
        << Vehicle::profileName(vehicle.profile())
        << "\n\n"

        << "1.  Iniciar simulacion\n"
        << "2.  Detener simulacion\n"
        << "3.  Ver parametros\n"
        << "4.  Cambiar parametro\n"
        << "5.  Seleccionar modo\n"
        << "6.  Seleccionar perfil\n"
        << "7.  Enviar comando ELM327/OBD\n"
        << "8.  Salir\n"
        << "----------------------------------------\n"
        << "Opcion: ";
}

static void changeParameter(Vehicle& vehicle) {
    std::string name;
    double value;

    std::cout
        << "Parametro "
        << "(speed/rpm/coolant/load/fuel/voltage/"
        << "throttle/iat/map/maf/gear): ";

    std::cin >> name;

    std::cout << "Valor: ";
    std::cin >> value;

    vehicle.setParameter(name, value);

    std::cout << "Parametro actualizado.\n";
}

static void selectMode(Vehicle& vehicle) {
    int option;

    std::cout
        << "\nModo:\n"
        << "1. DYNAMIC\n"
        << "2. FIXED\n"
        << "3. RANDOM\n"
        << "Seleccion: ";

    std::cin >> option;

    switch (option) {
        case 1:
            vehicle.setMode(
                Vehicle::SimulationMode::DYNAMIC
            );
            break;

        case 2:
            vehicle.setMode(
                Vehicle::SimulationMode::FIXED
            );
            break;

        case 3:
            vehicle.setMode(
                Vehicle::SimulationMode::RANDOM
            );
            break;

        default:
            std::cout << "Opcion invalida.\n";
            return;
    }

    std::cout << "Modo cambiado.\n";
}

static void selectProfile(Vehicle& vehicle) {
    int option;

    std::cout
        << "\nPerfil:\n"
        << "1. NORMAL\n"
        << "2. SPORT\n"
        << "3. ECONOMY\n"
        << "4. FAILSAFE\n"
        << "Seleccion: ";

    std::cin >> option;

    switch (option) {
        case 1:
            vehicle.setProfile(
                Vehicle::Profile::NORMAL
            );
            break;

        case 2:
            vehicle.setProfile(
                Vehicle::Profile::SPORT
            );
            break;

        case 3:
            vehicle.setProfile(
                Vehicle::Profile::ECONOMY
            );
            break;

        case 4:
            vehicle.setProfile(
                Vehicle::Profile::FAILSAFE
            );
            break;

        default:
            std::cout << "Opcion invalida.\n";
            return;
    }

    std::cout << "Perfil cambiado.\n";
}

int main() {
    constexpr int MCP2515_CS  = RPI_GPIO_P1_24; // CE0
    constexpr int MCP2515_INT = RPI_GPIO_P1_22; // GPIO25

    MCP2515 can(
        MCP2515_CS,
        MCP2515_INT
    );

    std::cout
        << "Inicializando MCP2515...\n";

    if (!can.begin(
            MCP2515::Bitrate::KBPS500)) {
        std::cerr
            << "No fue posible inicializar "
            << "el MCP2515.\n";

        return EXIT_FAILURE;
    }

    Vehicle vehicle;

    ELM327 elm(
        can,
        vehicle
    );

    std::atomic<bool> running(true);

    /*
     * Hilo que atiende solicitudes CAN.
     */
    std::thread canThread(
        [&]() {
            while (running) {
                if (can.available()) {
                    MCP2515::CanFrame frame;

                    if (can.receive(frame)) {
                        /*
                         * Solicitud OBD estándar:
                         *
                         * 7DF 02 01 0C ...
                         * 7E0 02 01 0C ...
                         */
                        if (frame.dlc >= 3 &&
                            frame.data[1] == 0x01) {

                            uint8_t mode =
                                frame.data[1];

                            uint8_t pid =
                                frame.data[2];

                            std::ostringstream command;

                            command
                                << std::uppercase
                                << std::hex
                                << std::setw(2)
                                << std::setfill('0')
                                << static_cast<int>(mode)
                                << std::setw(2)
                                << static_cast<int>(pid);

                            std::string response =
                                elm.processCommand(
                                    command.str()
                                );

                            (void)response;
                        }
                    }
                }

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(1)
                );
            }
        }
    );

    bool menuRunning = true;

    while (menuRunning) {
        printMenu(vehicle);

        int option;

        if (!(std::cin >> option)) {
            break;
        }

        switch (option) {

            case 1:
                vehicle.start();
                std::cout
                    << "Simulacion iniciada.\n";
                break;

            case 2:
                vehicle.stop();
                std::cout
                    << "Simulacion detenida.\n";
                break;

            case 3:
                printParameters(vehicle);
                break;

            case 4:
                changeParameter(vehicle);
                break;

            case 5:
                selectMode(vehicle);
                break;

            case 6:
                selectProfile(vehicle);
                break;

            case 7: {
                std::string command;

                std::cout
                    << "ELM327 > ";

                std::cin >> command;

                std::cout
                    << elm.processCommand(
                        command
                    )
                    << "\n";

                break;
            }

            case 8:
                menuRunning = false;
                break;

            default:
                std::cout
                    << "Opcion invalida.\n";
                break;
        }
    }

    running = false;

    vehicle.stop();

    if (canThread.joinable()) {
        canThread.join();
    }

    can.end();

    std::cout
        << "Emulador finalizado.\n";

    return EXIT_SUCCESS;
}

