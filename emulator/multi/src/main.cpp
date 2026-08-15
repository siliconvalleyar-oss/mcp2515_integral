#include <iostream>
#include <memory>
#include <csignal>
#include <atomic>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

#include "core/can_manager.hpp"
#include "core/protocol_router.hpp"
#include "core/session_manager.hpp"
#include "core/security.hpp"
#include "manufacturers/chevrolet.hpp"
#include "manufacturers/ford.hpp"
#include "manufacturers/toyota.hpp"
#include "manufacturers/bmw.hpp"
#include "manufacturers/volkswagen.hpp"
#include "manufacturers/mercedes.hpp"
#include "manufacturers/honda.hpp"
#include "manufacturers/nissan.hpp"
#include "database/db_manager.hpp"
#include "database/seed_data.hpp"
#include "diagnostics/dtc_manager.hpp"
#include "diagnostics/freeze_frame.hpp"
#include "diagnostics/readiness.hpp"
#include "diagnostics/vin_decoder.hpp"
#include "simulation/driving_cycle.hpp"
#include "simulation/sensor_simulator.hpp"
#include "simulation/fault_injector.hpp"
#include "simulation/environment.hpp"
#include "simulation/vehicle_profile.hpp"
#include "api/rest_api.hpp"
#include "api/websocket.hpp"
#include "api/grpc_server.hpp"
#include "security/access_control.hpp"
#include "security/secure_comm.hpp"
#include "logging/can_logger.hpp"
#include "logging/metrics_exporter.hpp"
#include "logging/replay_analyzer.hpp"

using json = nlohmann::json;
using namespace ecumult;

static std::atomic<bool> g_running{true};

void signalHandler(int sig) {
    std::cout << "\n[Main] Signal " << sig << " received, shutting down..." << std::endl;
    g_running = false;
}

json loadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Main] Config file not found: " << path << std::endl;
        std::cerr << "[Main] Using default configuration" << std::endl;
        json default_config;
        default_config["can"]["interface"] = "can0";
        default_config["can"]["bitrate"] = 500000;
        default_config["active_manufacturer"] = "chevrolet";
        return default_config;
    }
    json config;
    file >> config;
    return config;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << R"(
╔══════════════════════════════════════════════════╗
║     ECU Multi-Emulator v1.0.0                    ║
║     Multi-Brand Vehicle Diagnostic Emulator      ║
║     Raspberry Pi + MCP2515                       ║
╚══════════════════════════════════════════════════╝
)" << std::endl;

    // Load config
    std::string config_path = (argc > 1) ? argv[1] : "config.json";
    json config = loadConfig(config_path);
    std::cout << "[Main] Loaded configuration from " << config_path << std::endl;

    // Initialize CAN Manager
    std::string can_iface = config.value("can", json()).value("interface", "can0");
    int bitrate = config.value("can", json()).value("bitrate", 500000);
    auto can_manager = std::make_shared<CANManager>(can_iface, bitrate);
    std::cout << "[Main] CAN interface: " << can_iface << " @ " << bitrate << " bps" << std::endl;

    // Initialize Database
    std::string db_path = config.value("database", json()).value("path", "/etc/ecu_emulator/emulator.db");
    auto db_manager = std::make_shared<DBManager>(db_path);
    if (db_manager->open()) {
        db_manager->runMigrations();
        std::cout << "[Main] Database initialized: " << db_path << std::endl;
    } else {
        std::cout << "[Main] Database not available, running without persistence" << std::endl;
    }

    // Initialize Protocol Router
    auto router = std::make_shared<ProtocolRouter>(can_manager);
    std::cout << "[Main] Protocol router initialized" << std::endl;

    // Register all manufacturers
    router->registerManufacturer(Manufacturer::CHEVROLET, std::make_unique<ChevroletGM>(can_manager));
    router->registerManufacturer(Manufacturer::FORD, std::make_unique<Ford>(can_manager));
    router->registerManufacturer(Manufacturer::TOYOTA, std::make_unique<Toyota>(can_manager));
    router->registerManufacturer(Manufacturer::BMW, std::make_unique<BMW>(can_manager));
    router->registerManufacturer(Manufacturer::VOLKSWAGEN, std::make_unique<Volkswagen>(can_manager));
    router->registerManufacturer(Manufacturer::MERCEDES, std::make_unique<Mercedes>(can_manager));
    router->registerManufacturer(Manufacturer::HONDA, std::make_unique<Honda>(can_manager));
    router->registerManufacturer(Manufacturer::NISSAN, std::make_unique<Nissan>(can_manager));
    std::cout << "[Main] Registered 8 manufacturers" << std::endl;

    // Select active manufacturer
    std::string active = config.value("active_manufacturer", "chevrolet");
    Manufacturer mfr = ProtocolRouter::stringToManufacturer(active);
    if (!router->selectManufacturer(mfr)) {
        std::cerr << "[Main] Could not select manufacturer: " << active << std::endl;
        router->selectManufacturer(Manufacturer::CHEVROLET);
    }
    auto cfg = ProtocolRouter::getConfigFor(router->getCurrentManufacturer());
    std::cout << "[Main] Active manufacturer: " << cfg.name << std::endl;

    // Initialize Security
    SecurityManager security;

    // Initialize Simulation components
    SensorSimulator sensor_sim;
    DrivingCycle driving_cycle(DrivingCycle::URBAN);
    FaultInjector fault_injector;
    EnvironmentSimulator env_sim;

    // Initialize Logging
    auto logger = std::make_shared<CANLogger>(
        config.value("logging", json()).value("file", "/var/log/ecu_emulator.log"),
        CANLogger::levelFromString(config.value("logging", json()).value("level", "info")),
        config.value("logging", json()).value("max_size_mb", 100),
        config.value("logging", json()).value("max_files", 5));
    std::cout << "[Main] Logger initialized at level: "
              << CANLogger::levelToString(logger->getLevel()) << std::endl;

    // Initialize Metrics
    MetricsExporter metrics;
    ReplayAnalyzer replay;

    // Initialize simulation profiles
    VehicleProfileManager profile_manager;
    std::string default_profile = config.value("simulation", json()).value("profile", "normal");
    if (!profile_manager.selectProfile(default_profile)) {
        profile_manager.selectProfile("normal");
    }
    std::cout << "[Main] Profile: " << profile_manager.getCurrentProfile()->name
              << " - " << profile_manager.getCurrentProfile()->description << std::endl;

    // Initialize odometer from config
    auto& sim_cfg = config["simulation"];
    if (sim_cfg.contains("odometer") && sim_cfg["odometer"].contains("initial_km")) {
        float init_odo = sim_cfg["odometer"]["initial_km"].get<float>();
        router->getCurrentImplementation()->setOdometer(init_odo);
    }

    // Initialize API servers
    auto rest_api = std::make_shared<RESTAPI>(router, db_manager, "0.0.0.0", 8080);
    auto ws_server = std::make_shared<WebSocketServer>("0.0.0.0", 8081, 100);

    // CAN frame routing callback
    can_manager->registerCallback(0x000, 0x000,
        [router](const CANFrame& cf) {
            router->routeCANFrame(cf);
        });

    // Start CAN interface
    if (can_manager->start()) {
        std::cout << "[Main] CAN interface started" << std::endl;
    } else {
        std::cout << "[Main] CAN interface not available (run as root or configure SocketCAN)" << std::endl;
        std::cout << "[Main] Running in simulation-only mode" << std::endl;
    }

    // Start API servers
    rest_api->start();
    ws_server->start();

    // Simulation state (must be before lambdas that capture by ref)
    float sim_time = 0;
    float odo_accum = 0;

    // REST API route handler for profile and odometer endpoints
    rest_api->setRouteHandler(
        [&](const std::string& path, const std::string& method,
            const std::string& body,
            const std::unordered_map<std::string, std::string>& params) -> std::string {

        try {
            // === Health / Status ===
            if (path == "/" || path == "/health") {
                json resp;
                resp["status"] = "ok";
                resp["service"] = "ECU Multi-Emulator";
                resp["version"] = "1.0.0";
                resp["profile"] = profile_manager.getCurrentProfile()
                    ? profile_manager.getCurrentProfile()->name : "none";
                resp["uptime_seconds"] = sim_time;
                return resp.dump();
            }

            // === GET /profile -> current profile info ===
            if (path == "/profile" && method == "GET") {
                auto* prof = profile_manager.getCurrentProfile();
                if (!prof) return std::string{};
                json resp;
                resp["name"] = prof->name;
                resp["description"] = prof->description;
                resp["unstable_idle"] = prof->unstable_idle;
                resp["rpm_idle"] = {prof->rpm_idle_min, prof->rpm_idle_max};
                resp["active_dtcs"] = json::array();
                for (auto code : prof->active_dtcs) {
                    resp["active_dtcs"].push_back(code);
                }
                return resp.dump();
            }

            // === GET /profiles -> list available profiles ===
            if (path == "/profiles" && method == "GET") {
                json resp = json::array();
                for (const auto& name : profile_manager.getAvailableProfiles()) {
                    resp.push_back(name);
                }
                return resp.dump();
            }

            // === POST /profile -> switch profile ===
            if (path == "/profile" && method == "POST") {
                json req = json::parse(body);
                std::string name = req.value("name", "normal");
                if (!profile_manager.selectProfile(name)) {
                    json err;
                    err["error"] = "Unknown profile: " + name;
                    return err.dump();
                }
                // Sync to manufacturer
                auto* impl = router->getCurrentImplementation();
                if (impl) {
                    impl->setProfile(name);
                }
                json resp;
                resp["status"] = "ok";
                resp["profile"] = name;
                return resp.dump();
            }

            // === GET /odometer ===
            if (path == "/odometer" && method == "GET") {
                auto* impl = router->getCurrentImplementation();
                if (!impl) return std::string{};
                json resp;
                resp["odometer_km"] = impl->getOdometer();
                resp["trip_km"] = impl->getTripOdometer();
                return resp.dump();
            }

            // === POST /odometer -> set odometer value ===
            if (path == "/odometer" && method == "POST") {
                auto* impl = router->getCurrentImplementation();
                if (!impl) return std::string{};
                json req = json::parse(body);
                if (req.contains("odometer_km")) {
                    impl->setOdometer(req["odometer_km"].get<float>());
                }
                if (req.contains("reset_trip") && req["reset_trip"].get<bool>()) {
                    impl->resetTrip();
                }
                json resp;
                resp["status"] = "ok";
                resp["odometer_km"] = impl->getOdometer();
                resp["trip_km"] = impl->getTripOdometer();
                return resp.dump();
            }

            // === GET /dtcs -> current DTCs ===
            if (path == "/dtcs" && method == "GET") {
                auto* impl = router->getCurrentImplementation();
                if (!impl) return std::string{};
                auto dtcs = impl->getDTCs(params.count("all") ? false : true);
                json resp = json::array();
                for (const auto& dtc : dtcs) {
                    json item;
                    item["code"] = dtc.code;
                    item["description"] = dtc.description;
                    item["pending"] = dtc.is_pending;
                    resp.push_back(item);
                }
                return resp.dump();
            }

            // === POST /dtcs/clear ===
            if (path == "/dtcs/clear" && method == "POST") {
                auto* impl = router->getCurrentImplementation();
                if (!impl) return std::string{};
                impl->clearDTCs();
                json resp;
                resp["status"] = "ok";
                return resp.dump();
            }

        } catch (const std::exception& e) {
            json err;
            err["error"] = e.what();
            return err.dump();
        }

        return std::string{}; // 404 sent by caller
    });

    // WebSocket update generator
    ws_server->setUpdateGenerator([&]() -> std::string {
        json update;
        update["rpm"] = sensor_sim.getEngineRPM();
        update["speed"] = sensor_sim.getVehicleSpeed();
        update["coolant"] = sensor_sim.getCoolantTemp();
        update["intake_temp"] = sensor_sim.getIntakeTemp();
        update["maf"] = sensor_sim.getMAF();
        update["throttle"] = sensor_sim.getThrottlePosition();
        update["fuel_level"] = sensor_sim.getFuelLevel();
        update["battery"] = sensor_sim.getBatteryVoltage();
        update["intake_pressure"] = sensor_sim.getIntakePressure();
        update["fuel_pressure"] = sensor_sim.getFuelPressure();
        update["profile"] = profile_manager.getCurrentProfile()
            ? profile_manager.getCurrentProfile()->name : "normal";
        update["odometer_km"] = router->getCurrentImplementation()
            ? router->getCurrentImplementation()->getOdometer() : 0;
        return update.dump();
    });

    // Main simulation loop
    std::cout << "\n[Main] System ready. Press Ctrl+C to stop.\n" << std::endl;

    while (g_running) {
        float dt = 0.05f; // 50ms simulation step
        sim_time += dt;

        // Update driving cycle
        DrivingCyclePoint cycle_pt = driving_cycle.getPoint(sim_time);

        // Apply profile to RPM (e.g., unstable idle)
        float profile_rpm = profile_manager.applyRPMProfile(cycle_pt.rpm, dt);
        sensor_sim.updateDriving(profile_rpm, cycle_pt.speed, dt);

        // Update environment
        env_sim.update(dt);

        // Fault injection
        uint16_t fault_code = 0;
        if (fault_injector.shouldTrigger(&fault_code)) {
            auto* impl = router->getCurrentImplementation();
            if (impl) {
                impl->setDTC(fault_code, "Simulated fault via injection", true);
                metrics.recordDTCSet();
                logger->warning("Fault injected: DTC 0x" + std::to_string(fault_code));
            }
        }

        // Sync sensor values to active manufacturer (with profile overrides)
        auto* impl = router->getCurrentImplementation();
        if (impl) {
            impl->setEngineRPM(sensor_sim.getEngineRPM());
            impl->setVehicleSpeed(sensor_sim.getVehicleSpeed());
            impl->setCoolantTemp(sensor_sim.getCoolantTemp());
            impl->setIntakeTemp(sensor_sim.getIntakeTemp());
            impl->setMAF(sensor_sim.getMAF());
            impl->setThrottlePosition(sensor_sim.getThrottlePosition());
            impl->setFuelLevel(sensor_sim.getFuelLevel());
            impl->setBatteryVoltage(sensor_sim.getBatteryVoltage());

            // Odometer: km = speed (km/h) * dt (hours)
            float speed_kmh = std::max(0.0f, sensor_sim.getVehicleSpeed());
            float km_per_sec = speed_kmh / 3600.0f;
            odo_accum += km_per_sec * dt;
            if (odo_accum >= 0.01f) {
                impl->incrementOdometer(odo_accum);
                odo_accum = 0;
            }
        }

        // Update metrics
        metrics.gauge("ecu_engine_rpm", sensor_sim.getEngineRPM());
        metrics.gauge("ecu_vehicle_speed", sensor_sim.getVehicleSpeed());
        metrics.gauge("ecu_battery_voltage", sensor_sim.getBatteryVoltage());
        metrics.gauge("ecu_uptime_seconds", sim_time);
        metrics.gauge("ecu_odometer_km", impl ? impl->getOdometer() : 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Clean shutdown
    std::cout << "\n[Main] Shutting down..." << std::endl;
    rest_api->stop();
    ws_server->stop();
    can_manager->stop();
    db_manager->close();
    std::cout << "[Main] Shutdown complete." << std::endl;

    return 0;
}
