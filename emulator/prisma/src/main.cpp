#include "autotest.h"
#include "elm327.h"
#include "mcp2515.h"
#include "monitor_log.h"
#include "vehicle.h"
#include "vehicle_config.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif

static volatile bool g_stop = false;
static std::atomic<bool> g_canPaused{false};   // pausa durante el autotest

static Console console;
static MCP2515 can;
static Vehicle veh;
static Simulator sim(&veh);
static MonitorLog monitorLog;
static VehicleConfig vehConfig;
static ELM327* elm = nullptr;

// ---------------------------------------------------------------------------
//  Señales
// ---------------------------------------------------------------------------
static void onSignal(int) {
    g_stop = true;
}

// ---------------------------------------------------------------------------
//  Hilo CAN: recibe tramas del MCP2515 y las despacha
// ---------------------------------------------------------------------------
static void canThreadFunc() {
    // Tramas periódicas broadcast: el emulador no solo responde peticiones,
    // también publica RPM/velocidad (motor 0x320, transmisión 0x328).
    // IMPORTANTE: se envían DESPUÉS de procesar peticiones para no
    // bloquear el buffer TX (TXB0) ni el bus SPI cuando el escáner envía
    // una petición. Frecuencia: 10 Hz (100 ms) para no saturar el bus.
    auto lastBc = std::chrono::steady_clock::now();
    while (!g_stop) {
        // Durante el autotest el hilo CAN se pausa para no interferir con
        // las pruebas (ambos comparten el MCP2515/SPI).
        if (g_canPaused) {
            bcm2835_delay(5);
            continue;
        }
        const bool intAsserted = can.isInterruptPending();   // GPIO25 -> INT
        CanFrame f;
        while (can.receiveMessage(f)) {
            if (f.id == 0x7DF || f.id == 0x7E0 || f.id == 0x7E1 || f.id == 0x7E2)
                elm->handleCanRequest(f);
            else if (f.id == 0x7E8 || f.id == 0x7E9 || f.id == 0x7EA)
                elm->handleCanResponse(f);
        }
        // Tramas interceptadas durante respuestas multi-frame (no-FC).
        elm->drainPending();
        // Broadcast DESPUÉS de procesar peticiones (prioridad al escáner).
        const auto now = std::chrono::steady_clock::now();
        if (now - lastBc >= std::chrono::milliseconds(100)) {   // 10 Hz
            elm->sendBroadcastFrames();
            lastBc = now;
        }
        if (!intAsserted)
            bcm2835_delay(2);   // sin tráfico pendiente: pausa breve
    }
}

// ---------------------------------------------------------------------------
//  Hilo de simulación (10 Hz)
// ---------------------------------------------------------------------------
static void simThreadFunc() {
    while (!g_stop) {
        sim.tick(0.1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ---------------------------------------------------------------------------
//  Utilidades de menú
// ---------------------------------------------------------------------------
static std::string strip(const std::string& s) {
    const size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static std::string fmt(double v) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << v;
    return os.str();
}

static void selectVehicle();
static void configureCustomVehicle();

static void printMenu() {
    console.println("");
    console.println("-----------------------------------------------------------");
    console.println(" [1] Iniciar simulación       [2] Detener simulación");
    console.println(" [3] Perfil de conducción     [4] Configurar parámetros");
    console.println(" [5] Estado del vehículo      [6] Consola ELM327 / OBD2");
    console.println(" [7] Información del sistema  [8] Autotest de comunicación");
    console.println(" [9] Monitor en vivo (ECU)    [0] Salir");
    console.println(" [V] Seleccionar vehículo     [C] Configurar custom");
    console.print("Opción: ");
}

static void printState() {
    console.println("");
    console.println("--- Estado del vehículo ---");
    {
        std::lock_guard<std::mutex> lk(veh.mtx);
        console.println(" Motor: " +
            std::string(veh.engineOn() ? "ENCENDIDO" : "APAGADO") +
            "   Perfil: " + sim.profileName() +
            std::string(sim.running() ? "" : " (simulación detenida)"));
        for (const auto& p : veh.params()) {
            std::ostringstream os;
            os << " " << p.nombre << ": ";
            if (p.key == "marcha")
                os << static_cast<int>(veh.value(p.key));
            else
                os << std::fixed << std::setprecision(1) << veh.value(p.key);
            os << " " << p.unidad
               << "  [" << (veh.isAuto(p.key) ? "AUTO" : "FIJO") << "]";
            console.println(os.str());
        }
    }
}

static void selectProfile() {
    console.println("");
    console.println(" Perfiles de conducción:");
    console.println("  [1] Ralentí (motor al mínimo, detenido)");
    console.println("  [2] Ciudad (ciclo con semáforos)");
    console.println("  [3] Autopista (crucero 90-112 km/h)");
    console.println("  [4] Deportivo (aceleraciones fuertes)");
    console.println("  [0] Volver");
    console.print("Opción: ");
    std::string line;
    if (!std::getline(std::cin, line)) return;
    switch (std::atoi(line.c_str())) {
        case 1: sim.setProfile(Profile::Idle);    break;
        case 2: sim.setProfile(Profile::City);    break;
        case 3: sim.setProfile(Profile::Highway); break;
        case 4: sim.setProfile(Profile::Sport);   break;
        default: return;
    }
    console.println(std::string(" Perfil seleccionado: ") + sim.profileName());
}

static void handleParamMenu() {
    const auto& defs = veh.params();
    while (!g_stop) {
        console.println("");
        console.println("--- Parámetros del vehículo ---");
        for (size_t i = 0; i < defs.size(); ++i) {
            const auto& p = defs[i];
            double v;
            bool a;
            {
                std::lock_guard<std::mutex> lk(veh.mtx);
                v = veh.value(p.key);
                a = veh.isAuto(p.key);
            }
            std::ostringstream os;
            os << " [" << (i + 1) << "] " << p.nombre << " = "
               << std::fixed << std::setprecision(1) << v << " " << p.unidad
               << "  [" << (a ? "AUTO" : "FIJO") << "]";
            console.println(os.str());
        }
        console.println(" [0] Volver");
        console.print("Seleccione parámetro: ");
        std::string line;
        if (!std::getline(std::cin, line)) return;
        const int idx = std::atoi(line.c_str());
        if (idx <= 0 || idx > static_cast<int>(defs.size())) return;
        const auto& p = defs[static_cast<size_t>(idx - 1)];

        console.println("");
        console.println(std::string(" Parámetro: ") + p.nombre + "  (" +
                        p.unidad + ")");
        console.println("  [1] Fijar valor");
        console.println("  [2] Modo automático (simulado)");
        console.println("  [0] Volver");
        console.print("Opción: ");
        if (!std::getline(std::cin, line)) return;
        const int op = std::atoi(line.c_str());

        if (op == 1) {
            console.print("Valor (" + fmt(p.min) + " .. " + fmt(p.max) + "): ");
            if (!std::getline(std::cin, line)) return;
            double v = std::atof(line.c_str());
            {
                std::lock_guard<std::mutex> lk(veh.mtx);
                v = std::min(p.max, std::max(p.min, v));
                veh.setValue(p.key, v);
                veh.setAuto(p.key, false);
            }
            console.println(" OK: " + p.nombre + " fijado en " + fmt(v) + " " +
                            p.unidad + " (FIJO)");
        } else if (op == 2) {
            {
                std::lock_guard<std::mutex> lk(veh.mtx);
                veh.setAuto(p.key, true);
            }
            console.println(" OK: " + p.nombre + " en modo automático (AUTO)");
        }
    }
}

// ---------------------------------------------------------------------------
//  Monitor en vivo: redibuja el estado de la ECU en pantalla estilo CLI.
//  El marco (textos fijos) se dibuja una vez y solo se reescriben los
//  valores que cambian, usando códigos de escape ANSI (no requiere librería
//  externa). Termina con 'q' (o Ctrl+C).
// ---------------------------------------------------------------------------
static void monitorView() {
    // --- Configurar la terminal en modo crudo (leer teclas sin Enter) ---
    struct termios oldt, raw;
    bool rawOk = (tcgetattr(STDIN_FILENO, &oldt) == 0);
    if (rawOk) {
        raw = oldt;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;   // no bloquear en read()
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }

    // Ocultar cursor y limpiar pantalla una sola vez.
    console.print("\033[?25l\033[2J\033[H");

    [[maybe_unused]] const int nParams = static_cast<int>(veh.params().size());
    bool quit = false;
    while (!g_stop && !quit) {
        // Mover el cursor al origen y redibujar solo el bloque de estado.
        // Los textos fijos se reescriben idénticos (sin parpadeo en terminales
        // ANSI) y los valores cambian en su misma posición.
        std::ostringstream os;
        os << "\033[H";
        // \033[K al final de cada línea borra cualquier resto de un redibujo
        // anterior más largo (p. ej. el sufijo "(simulación detenida)").
        os << "===========================================================\033[K\n";
        os << " MONITOR EN VIVO DE LA ECU   (v" << APP_VERSION << ")\033[K\n";
        os << "===========================================================\033[K\n";
        {
            std::lock_guard<std::mutex> lk(veh.mtx);
            os << " Motor: "
               << (veh.engineOn() ? "ENCENDIDO" : "APAGADO")
               << "   Perfil: " << sim.profileName()
               << (sim.running() ? "" : " (simulación detenida)")
               << "\033[K\n";
            for (const auto& p : veh.params()) {
                const double v = veh.value(p.key);
                const bool a = veh.isAuto(p.key);
                os << " " << p.nombre << ": ";
                if (p.key == "marcha")
                    os << std::setw(10) << static_cast<int>(v);
                else
                    os << std::fixed << std::setprecision(1)
                       << std::setw(10) << v;
                os << " " << p.unidad
                   << "  [" << (a ? "AUTO" : "FIJO") << "]\033[K\n";
            }
        }
        os << "===========================================================\033[K\n";
        os << " Pulse 'q' para volver al menú    (los valores se actualizan\033[K\n";
        os << " en vivo cada ~250 ms, sin parpadear)\033[K\n";
        console.print(os.str());

        // Esperar entrada con timeout para redibujar periódicamente.
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 250000;   // 250 ms
        if (select(STDIN_FILENO + 1, &rfds, nullptr, nullptr, &tv) > 0) {
            char c = 0;
            if (read(STDIN_FILENO, &c, 1) == 1 && (c == 'q' || c == 'Q'))
                quit = true;
        }
    }

    // Restaurar terminal y limpiar para volver al menú.
    if (rawOk) tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    console.print("\033[?25h\033[2J\033[H");
    console.println("Monitor en vivo finalizado.");
}

static void handleElmMenu() {
    console.println("");
    console.println("--- Consola ELM327 ---");
    console.println(" Comandos AT u OBD2 (ej: ATZ, ATSP6, 010C, 010D, 0902).");
    console.println(" Escriba 'exit' para volver al menú.");
    std::string line;
    while (!g_stop) {
        console.print("> ");
        if (!std::getline(std::cin, line)) break;
        const std::string t = strip(line);
        if (t == "exit" || t == "quit" || t == "q") break;
        if (t.empty()) continue;
        const std::string resp = elm->process(line);
        if (!resp.empty()) console.print(resp);
    }
}

static void handleAutotest() {
    console.println("");
    console.println("--- Autotest de comunicación ---");
    console.println(" Pausando el tráfico CAN durante la prueba...");
    g_canPaused = true;

    // Ejecuta las tres pruebas (SPI, loopback y bus). El test SPI hace RESET
    // del módulo, así que se reconfigura el MCP2515 al terminar (sin tocar la
    // inicialización global de bcm2835, que sigue siendo del emulador).
    const bool ok = autotestRun(can, false);
    if (!can.beginExisting())
        console.println(" AVISO: falló la reinicialización del MCP2515 tras el autotest.");

    g_canPaused = false;
    console.println(std::string(" Autotest: ") +
                    (ok ? "TODAS LAS PRUEBAS PASARON" : "HUBO FALLOS (detalle arriba)"));
    console.println(" Tráfico CAN reanudado.");
}

static void printInfo() {
    console.println("");
    console.println("--- Información del sistema ---");
    console.println(std::string(" Versión: ") + APP_VERSION);
    console.println(" Bus SPI0: MISO=GPIO9(SO)  MOSI=GPIO10(SI)  "
                    "SCLK=GPIO11(SCK)  CE0=GPIO8(CS)");
    console.println(" Interrupción: GPIO25 -> INT del MCP2515");
    console.println(" Cristal MCP2515: " +
                    std::to_string(MCP2515_OSC_HZ / 1000000) +
                    " MHz   CAN: " + std::to_string(CAN_BAUDRATE / 1000) +
                    " kbps");
    console.println(" Protocolo OBD2: ISO 15765-4 (CAN 11-bit / 500 kbps) = "
                    "ELM327 SP6");
    console.println(" PID personalizado 0x4E: marcha (0=N, 1-5, 6=R)");
    console.println(" Tramas broadcast: 0x320 (motor) / 0x328 (transmisión) "
                    "a 10 Hz, deshabilitadas por defecto (ATBC1 para activar)");
    console.println(" Vehículos: config/vehicles.json (V = seleccionar, C = custom)");
    can.printInfo();
}

// ---------------------------------------------------------------------------
//  Selección de vehículo
// ---------------------------------------------------------------------------
static void selectVehicle() {
    const auto ids = vehConfig.listVehicles();
    if (ids.empty()) {
        console.println(" No hay vehículos configurados.");
        return;
    }

    console.println("");
    console.println("--- Seleccionar vehículo ---");
    for (size_t i = 0; i < ids.size(); ++i) {
        const auto v = vehConfig.getVehicle(ids[i]);
        std::ostringstream os;
        os << " [" << (i + 1) << "] " << v.brand << " " << v.model
           << " (" << v.year << ") — " << v.engine;
        if (ids[i] == vehConfig.currentVehicle().id)
            os << "  << ACTUAL";
        console.println(os.str());
    }
    if (vehConfig.hasCustom()) {
        const auto c = vehConfig.currentVehicle();
        std::ostringstream os;
        os << " [" << (ids.size() + 1) << "] Custom: " << c.brand
           << " " << c.model << " (" << c.year << ")";
        if (c.id == "custom") os << "  << ACTUAL";
        console.println(os.str());
    }
    console.println(" [0] Volver");
    console.print("Seleccione: ");

    std::string line;
    if (!std::getline(std::cin, line)) return;
    const int idx = std::atoi(line.c_str());
    if (idx <= 0) return;

    if (static_cast<size_t>(idx) <= ids.size()) {
        vehConfig.setCurrentVehicle(ids[idx - 1]);
        vehConfig.save();
        const auto v = vehConfig.currentVehicle();
        console.println(" Vehículo seleccionado: " + v.brand + " " + v.model);
    } else if (vehConfig.hasCustom() && static_cast<size_t>(idx) == ids.size() + 1) {
        vehConfig.setCurrentVehicle("custom");
        vehConfig.save();
        console.println(" Vehículo seleccionado: Custom");
    }
}

// ---------------------------------------------------------------------------
//  Editar datos del custom (asistente)
// ---------------------------------------------------------------------------
static void editCustomFields(VehicleConfig::VehicleInfo& custom) {
    console.println("");
    console.println(" Deje vacío para mantener el valor actual.");
    console.println("");

    auto askStr = [&](const std::string& label, std::string& val) {
        console.print(" " + label + " [" + val + "]: ");
        std::string line;
        if (std::getline(std::cin, line) && !line.empty()) val = line;
    };
    auto askInt = [&](const std::string& label, int& val) {
        console.print(" " + label + " [" + std::to_string(val) + "]: ");
        std::string line;
        if (std::getline(std::cin, line) && !line.empty()) val = std::atoi(line.c_str());
    };
    auto askDbl = [&](const std::string& label, double& val) {
        console.print(" " + label + " [" + fmt(val) + "]: ");
        std::string line;
        if (std::getline(std::cin, line) && !line.empty()) val = std::atof(line.c_str());
    };

    console.println(" == Datos básicos ==");
    askStr("Marca", custom.brand);
    askStr("Modelo", custom.model);
    askInt("Año", custom.year);
    askStr("Motor", custom.engine);
    askStr("Transmisión", custom.transmission);
    askInt("Cilindrada (cc)", custom.displacement_cc);
    askStr("Combustible", custom.fuel);

    console.println("");
    console.println(" == Parámetros del motor ==");
    askInt("RPM ralentí", custom.rpm_idle);
    askInt("RPM límite rojo", custom.rpm_redline);
    askInt("Velocidad máxima (km/h)", custom.max_speed_kmh);
    askInt("Torque máximo (Nm)", custom.torque_max_nm);
    askInt("Potencia máxima (HP)", custom.power_max_hp);
    askInt("Peso (kg)", custom.weight_kg);
    askInt("Tanque (litros)", custom.fuel_tank_l);
    askDbl("Relación final", custom.final_drive);

    console.println("");
    console.println(" == Valores iniciales ==");
    askDbl("Temp. ambiente (°C)", custom.temp_ambiente_default);
    askDbl("Voltaje batería (V)", custom.voltaje_bateria_default);
    askDbl("Oil life (%)", custom.oil_life_default);
    askDbl("Distancia desde clear (km)", custom.distance_clear_km);
    askDbl("Odómetro (km)", custom.odometro_km);
}

// ---------------------------------------------------------------------------
//  Menú de vehículo custom (editar / borrar / crear)
// ---------------------------------------------------------------------------
static void configureCustomVehicle() {
    while (!g_stop) {
        const bool exists = vehConfig.hasCustom();
        console.println("");
        console.println("--- Vehículo custom ---");

        if (exists) {
            const auto c = vehConfig.getVehicle("custom");
            console.println(" Actual: " + c.brand + " " + c.model +
                            " (" + std::to_string(c.year) + ") — " + c.engine);
            console.println("");
            console.println(" [1] Editar vehículo custom");
            console.println(" [2] Borrar vehículo custom");
        } else {
            console.println(" No hay vehículo custom configurado.");
            console.println("");
            console.println(" [1] Crear nuevo vehículo custom");
        }
        console.println(" [0] Volver");
        console.print("Opción: ");

        std::string line;
        if (!std::getline(std::cin, line)) return;
        const int op = std::atoi(line.c_str());

        if (op == 0) return;

        if (op == 1) {
            // Editar o crear
            VehicleConfig::VehicleInfo custom;
            if (exists) {
                custom = vehConfig.getVehicle("custom");
            } else {
                custom.id = "custom";
                custom.brand = "Custom";
                custom.model = "Mi Vehículo";
                custom.year = 2024;
                custom.engine = "Motor personalizado";
                custom.transmission = "Manual/Automática";
                custom.displacement_cc = 1600;
                custom.fuel = "Gasolina";
            }

            editCustomFields(custom);

            console.println("");
            console.print(" ¿Guardar? (s/n): ");
            if (std::getline(std::cin, line) && (line == "s" || line == "S")) {
                vehConfig.updateCustom(custom);
                vehConfig.setCurrentVehicle("custom");
                vehConfig.save();
                console.println(" Vehículo custom guardado.");
            } else {
                console.println(" Cancelado.");
            }
        } else if (op == 2 && exists) {
            // Borrar
            console.println("");
            console.print(" ¿Borrar vehículo custom? (s/n): ");
            if (std::getline(std::cin, line) && (line == "s" || line == "S")) {
                // Si el custom era el actual, volver al default
                if (vehConfig.currentVehicle().id == "custom") {
                    vehConfig.setCurrentVehicle("chevrolet_prisma_2018");
                }
                vehConfig.clearCustom();
                vehConfig.save();
                console.println(" Vehículo custom borrado.");
            } else {
                console.println(" Cancelado.");
            }
        }
    }
}

// ---------------------------------------------------------------------------
//  main
// ---------------------------------------------------------------------------
int main() {
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    console.println("===========================================================");
    console.println(std::string(" Emulador OBD2 ECU v") + APP_VERSION);
    console.println(" ELM327 + MCP2515 (SPI0) | ISO 15765-4 CAN 11-bit 500 kbps");
    console.println("===========================================================");

    if (!can.begin()) {
        console.println("");
        console.println("ERROR: no se pudo inicializar el MCP2515.");
        console.println(" - Ejecute con sudo (acceso a /dev/mem).");
        console.println(" - Verifique el cableado SPI: CE0->CS, MISO->SO, "
                        "MOSI->SI, SCLK->SCK, GPIO25->INT.");
        console.println(" - Verifique libbcm2835: make install-bcm2835");
        return 1;
    }

    // Cargar configuración de vehículos
    if (vehConfig.load()) {
        const auto v = vehConfig.currentVehicle();
        console.println("");
        console.println(" Vehículo: " + v.brand + " " + v.model + " (" +
                        std::to_string(v.year) + ") — " + v.engine);
    } else {
        console.println("");
        console.println(" AVISO: no se pudo cargar config/vehicles.json");
    }

    // Abrir log de monitoreo CAN
    if (monitorLog.open()) {
        console.println(" Log de monitoreo activo: logs/log_monitor_*.log");
        monitorLog.logMessage("INIT", "Emulador OBD2 v" + std::string(APP_VERSION) + " iniciado");
    } else {
        console.println(" AVISO: no se pudo crear el log de monitoreo (logs/).");
    }

    ELM327 elmObj(&can, &veh, &console, &monitorLog);
    elm = &elmObj;

    std::thread canThread(canThreadFunc);
    std::thread simThread(simThreadFunc);

    console.println("");
    console.println(" Listo. Conecte su escáner OBD2 (ELM327) al bus CAN y "
                    "use el menú.");
    console.println(" Sugerencia: pruebe la opción 6 y escriba '010C' para "
                    "leer RPM.");

    while (!g_stop) {
        printMenu();
        std::string line;
        if (!std::getline(std::cin, line)) {
            g_stop = true;
            break;
        }
        const std::string lineUpper = vehConfig.load() ? line : line;  // placeholder
        const int op = std::atoi(line.c_str());

        // Opciones de letra (V, C)
        if (line == "v" || line == "V") {
            selectVehicle();
            continue;
        }
        if (line == "c" || line == "C") {
            configureCustomVehicle();
            continue;
        }

        switch (op) {
            case 1: sim.start();
                    console.println(" Simulación INICIADA (motor encendido).");
                    break;
            case 2: sim.stop();
                    console.println(" Simulación DETENIDA (motor apagado).");
                    break;
            case 3: selectProfile();      break;
            case 4: handleParamMenu();    break;
            case 5: printState();         break;
            case 6: handleElmMenu();      break;
            case 7: printInfo();          break;
            case 8: handleAutotest();     break;
            case 9: monitorView();        break;
            case 0: g_stop = true;        break;
            default: console.println(" Opción inválida (0-9, V, C).");
        }
    }

    g_stop = true;
    canThread.join();
    simThread.join();
    can.end();

    // Cerrar log de monitoreo
    monitorLog.logMessage("EXIT", "Emulador detenido");
    monitorLog.close();

    console.println("");
    console.println(" Saliendo...");
    return 0;
}
