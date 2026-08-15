#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <limits>
#include <iomanip>
#include <map>
#include <bcm2835.h>

#include "vehicle.h"
#include "mcp2515/mcp2515.h"
#include "elm327.h"

// ==================== Conexionado (Raspberry Pi -> MCP2515) ====================
//   MISO   -> SO  : gestionado por hardware SPI0 (GPIO9)
//   MOSI   -> SI  : gestionado por hardware SPI0 (GPIO10)
//   SCLK   -> SCK : gestionado por hardware SPI0 (GPIO11)
//   CE0    -> CS  : BCM2835_SPI_CS0 (GPIO8)
//   GPIO25 -> INT : línea de interrupción (activa en bajo)
static const uint8_t PIN_CS  = RPI_GPIO_P1_24; // CE0 (pin físico 24)
static const uint8_t PIN_INT = RPI_GPIO_P1_22; // GPIO25 (pin físico 22)

Vehicle vehicle;
MCP2515 can(PIN_CS, PIN_INT);
ELM327 elm(&vehicle, &can);

std::atomic<bool> g_emulating{false};
std::thread g_simThread;
std::thread g_canThread;

// ---------------------------------------------------------------------------
// Hilos de trabajo
// ---------------------------------------------------------------------------
void simulationLoop() {
    using namespace std::chrono;
    auto last = steady_clock::now();
    while (g_emulating) {
        auto now = steady_clock::now();
        double dt = duration_cast<duration<double>>(now - last).count();
        last = now;
        vehicle.update(dt);
        std::this_thread::sleep_for(milliseconds(100));
    }
}

void canLoop() {
    while (g_emulating) {
        elm.pollCanRequests();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

// ---------------------------------------------------------------------------
// Acciones del menú
// ---------------------------------------------------------------------------
void startEmulation() {
    if (g_emulating) {
        std::cout << "La emulación ya está en ejecución.\n";
        return;
    }
    vehicle.start();
    g_emulating = true;
    g_simThread = std::thread(simulationLoop);
    g_canThread = std::thread(canLoop);
    std::cout << "Emulación iniciada. Respondiendo solicitudes OBD2 en el bus CAN.\n";
}

void stopEmulation() {
    if (!g_emulating) {
        std::cout << "La emulación no está en ejecución.\n";
        return;
    }
    g_emulating = false;
    if (g_simThread.joinable()) g_simThread.join();
    if (g_canThread.joinable()) g_canThread.join();
    vehicle.stop();
    std::cout << "Emulación detenida.\n";
}

void printStatus() {
    VehicleParameters v = vehicle.getSnapshot();
    std::cout << "\n===== Estado actual del vehículo (Chevrolet Prisma) =====\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Perfil:            " << Vehicle::profileToString(vehicle.getProfile()) << "\n";
    std::cout << "Motor:             " << (v.engineRunning ? "Encendido" : "Apagado") << "\n";
    std::cout << "Velocidad:         " << v.speedKmh << " km/h\n";
    std::cout << "RPM:               " << v.rpm << "\n";
    std::cout << "Marcha:            " << Vehicle::gearToString(v.gear) << "\n";
    std::cout << "Temp. motor:       " << v.engineTempC << " C\n";
    std::cout << "Carga del motor:   " << v.engineLoadPct << " %\n";
    std::cout << "Acelerador:        " << v.throttlePct << " %\n";
    std::cout << "Presion combust.:  " << v.fuelPressureKpa << " kPa\n";
    std::cout << "Nivel combust.:    " << v.fuelLevelPct << " %\n";
    std::cout << "Voltaje bateria:   " << v.batteryVoltage << " V\n";
    std::cout << "MAF:               " << v.mafRateGs << " g/s\n";
    std::cout << "Temp. aire adm.:   " << v.intakeAirTempC << " C\n";
    std::cout << "Avance encendido:  " << v.timingAdvanceDeg << " grados\n";
    std::cout << "Tiempo en marcha:  " << v.runTimeSec << " s\n";
    std::cout << "Odometro:          " << (long)v.odometerKm << " km\n";
    std::cout << "===========================================================\n\n";
}

void selectProfileMenu() {
    std::cout << "\nSeleccione perfil de conduccion:\n";
    std::cout << " 1) Ralenti\n 2) Urbano\n 3) Carretera\n 4) Agresivo\n 5) Personalizado\n";
    std::cout << "Opcion: ";
    int op;
    if (!(std::cin >> op)) { std::cin.clear(); std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); return; }

    switch (op) {
        case 1: vehicle.setProfile(DrivingProfile::RALENTI); break;
        case 2: vehicle.setProfile(DrivingProfile::URBANO); break;
        case 3: vehicle.setProfile(DrivingProfile::CARRETERA); break;
        case 4: vehicle.setProfile(DrivingProfile::AGRESIVO); break;
        case 5: vehicle.setProfile(DrivingProfile::PERSONALIZADO); break;
        default: std::cout << "Opcion invalida.\n"; return;
    }
    std::cout << "Perfil actualizado a: " << Vehicle::profileToString(vehicle.getProfile()) << "\n";
}

void changeParametersMenu() {
    std::cout << "\n--- Cambiar parametros en tiempo real ---\n";
    std::cout << " 1) Velocidad (km/h)\n";
    std::cout << " 2) RPM\n";
    std::cout << " 3) Temperatura de motor (C)\n";
    std::cout << " 4) Carga del motor (%)\n";
    std::cout << " 5) Presion de combustible (kPa)\n";
    std::cout << " 6) Voltaje de bateria (V)\n";
    std::cout << " 7) Nivel de combustible (%)\n";
    std::cout << " 8) Posicion del acelerador (%)\n";
    std::cout << " 9) Liberar todos los valores fijos (volver a simulacion automatica)\n";
    std::cout << " 0) Volver\n";
    std::cout << "Opcion: ";
    int op;
    if (!(std::cin >> op)) { std::cin.clear(); std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); return; }

    static const std::map<int, std::string> keys = {
        {1, "speed"}, {2, "rpm"}, {3, "temp"}, {4, "load"},
        {5, "fuelPressure"}, {6, "battery"}, {7, "fuelLevel"}, {8, "throttle"}
    };

    if (op == 0) return;
    if (op == 9) { vehicle.clearAllFixedValues(); std::cout << "Valores liberados.\n"; return; }

    auto it = keys.find(op);
    if (it == keys.end()) { std::cout << "Opcion invalida.\n"; return; }

    std::cout << "Ingrese el valor fijo: ";
    double value;
    if (!(std::cin >> value)) { std::cin.clear(); std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); return; }

    vehicle.setFixedValue(it->second, value);
    std::cout << "Parametro actualizado.\n";
}

void elm327Console() {
    std::cout << "\n--- Consola ELM327 (escriba 'salir' para volver) ---\n";
    std::cout << "Ejemplos: ATZ, ATE0, ATSP0, 0100, 010C, 010D, 0105, ATRV\n";
    std::string line;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    while (true) {
        std::cout << ">> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "salir") break;
        std::string resp = elm.processCommand(line);
        std::cout << resp << "\n";
    }
}

void printMenu() {
    std::cout << "\n============ Emulador Chevrolet Prisma (OBD2 / ELM327) ============\n";
    std::cout << " 1) Iniciar emulacion\n";
    std::cout << " 2) Detener emulacion\n";
    std::cout << " 3) Cambiar parametros en tiempo real\n";
    std::cout << " 4) Seleccionar perfil de conduccion\n";
    std::cout << " 5) Ver estado actual\n";
    std::cout << " 6) Consola de comandos ELM327 (pruebas manuales)\n";
    std::cout << " 7) Salir\n";
    std::cout << "=====================================================================\n";
    std::cout << "Opcion: ";
}

int main() {
    std::cout << "Inicializando interfaz SPI / MCP2515 ...\n";
    if (!can.begin(CanBitrate::BPS_500K)) {
        std::cerr << "Error al inicializar bcm2835/SPI. Verifique permisos (ejecutar con sudo) y conexiones.\n";
        return 1;
    }
    if (!can.setNormalMode()) {
        std::cerr << "Advertencia: no se pudo confirmar el modo normal del MCP2515. Continuando de todas formas.\n";
    }
    std::cout << "MCP2515 inicializado correctamente en modo CAN normal (500 kbps).\n";

    bool exitProgram = false;
    while (!exitProgram) {
        printMenu();
        int option;
        if (!(std::cin >> option)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        switch (option) {
            case 1: startEmulation(); break;
            case 2: stopEmulation(); break;
            case 3: changeParametersMenu(); break;
            case 4: selectProfileMenu(); break;
            case 5: printStatus(); break;
            case 6: elm327Console(); break;
            case 7:
                stopEmulation();
                exitProgram = true;
                break;
            default:
                std::cout << "Opcion invalida.\n";
        }
    }

    bcm2835_spi_end();
    bcm2835_close();
    std::cout << "Emulador finalizado.\n";
    return 0;
}
