#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include "../core/can_manager.hpp"
#include "../core/protocol_router.hpp"
#include "../core/session_manager.hpp"
#include "../core/security.hpp"
#include "../manufacturers/base.hpp"
#include "../manufacturers/chevrolet.hpp"
#include "../manufacturers/ford.hpp"
#include "../manufacturers/toyota.hpp"
#include "../manufacturers/bmw.hpp"
#include "../manufacturers/volkswagen.hpp"
#include "../manufacturers/mercedes.hpp"
#include "../manufacturers/honda.hpp"
#include "../manufacturers/nissan.hpp"
#include "../diagnostics/dtc_manager.hpp"
#include "../diagnostics/vin_decoder.hpp"
#include "../diagnostics/readiness.hpp"
#include "../diagnostics/calibration.hpp"
#include "../diagnostics/freeze_frame.hpp"
#include "../protocols/obd2_standard.hpp"
#include "../protocols/uds.hpp"
#include "../protocols/gmlan.hpp"
#include "../protocols/kwp2000.hpp"
#include "../protocols/can_tp.hpp"
#include "../database/db_manager.hpp"
#include "../database/seed_data.hpp"
#include "../simulation/driving_cycle.hpp"
#include "../simulation/sensor_simulator.hpp"
#include "../simulation/fault_injector.hpp"
#include "../simulation/environment.hpp"
#include "../security/access_control.hpp"
#include "../security/secure_comm.hpp"
#include "../logging/can_logger.hpp"
#include "../logging/metrics_exporter.hpp"
#include "../logging/replay_analyzer.hpp"

using namespace ecumult;

// ===== Protocol Router Tests =====

TEST_CASE("ProtocolRouter_ManufacturerSelection", "[router]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    auto router = std::make_shared<ProtocolRouter>(can);

    REQUIRE(router->getCurrentManufacturer() == Manufacturer::UNKNOWN);

    router->registerManufacturer(Manufacturer::CHEVROLET, std::make_unique<ChevroletGM>(can));
    router->registerManufacturer(Manufacturer::TOYOTA, std::make_unique<Toyota>(can));

    REQUIRE(router->selectManufacturer(Manufacturer::CHEVROLET));
    REQUIRE(router->getCurrentManufacturer() == Manufacturer::CHEVROLET);

    REQUIRE(router->selectManufacturer(Manufacturer::TOYOTA));
    REQUIRE(router->getCurrentManufacturer() == Manufacturer::TOYOTA);

    REQUIRE_FALSE(router->selectManufacturer(Manufacturer::HYUNDAI)); // not registered
}

TEST_CASE("ProtocolRouter_StringConversion", "[router]") {
    REQUIRE(ProtocolRouter::stringToManufacturer("chevrolet") == Manufacturer::CHEVROLET);
    REQUIRE(ProtocolRouter::stringToManufacturer("ford") == Manufacturer::FORD);
    REQUIRE(ProtocolRouter::stringToManufacturer("toyota") == Manufacturer::TOYOTA);
    REQUIRE(ProtocolRouter::stringToManufacturer("bmw") == Manufacturer::BMW);
    REQUIRE(ProtocolRouter::stringToManufacturer("vw") == Manufacturer::VOLKSWAGEN);
    REQUIRE(ProtocolRouter::stringToManufacturer("mercedes") == Manufacturer::MERCEDES);
    REQUIRE(ProtocolRouter::stringToManufacturer("honda") == Manufacturer::HONDA);
    REQUIRE(ProtocolRouter::stringToManufacturer("nissan") == Manufacturer::NISSAN);
    REQUIRE(ProtocolRouter::stringToManufacturer("unknown") == Manufacturer::UNKNOWN);

    REQUIRE(ProtocolRouter::manufacturerToString(Manufacturer::CHEVROLET) == "chevrolet");
    REQUIRE(ProtocolRouter::manufacturerToString(Manufacturer::BMW) == "bmw");
}

TEST_CASE("ProtocolRouter_Config", "[router]") {
    auto cfg = ProtocolRouter::getConfigFor(Manufacturer::CHEVROLET);
    REQUIRE(cfg.name == "Chevrolet/GM");
    REQUIRE(cfg.bitrate == 500000);
    REQUIRE(cfg.protocol == ActiveProtocol::GMLAN);

    cfg = ProtocolRouter::getConfigFor(Manufacturer::BMW);
    REQUIRE(cfg.name == "BMW");
    REQUIRE(cfg.protocol == ActiveProtocol::UDS);
}

// ===== Session Manager Tests =====

TEST_CASE("SessionManager_BasicOperations", "[session]") {
    SessionManager sm;

    REQUIRE(sm.getCurrentSession() == UDSSession::DEFAULT);

    REQUIRE(sm.changeSession(UDSSession::EXTENDED));
    REQUIRE(sm.getCurrentSession() == UDSSession::EXTENDED);

    REQUIRE(sm.changeSession(UDSSession::PROGRAMMING));
    REQUIRE(sm.getCurrentSession() == UDSSession::PROGRAMMING);

    // Can't go to SAFETY from PROGRAMMING
    REQUIRE_FALSE(sm.changeSession(UDSSession::SAFETY));

    // Back to default
    REQUIRE(sm.changeSession(UDSSession::DEFAULT));
    REQUIRE(sm.getCurrentSession() == UDSSession::DEFAULT);
}

TEST_CASE("SessionManager_SecurityAccess", "[session]") {
    SessionManager sm;

    REQUIRE(sm.getCurrentSecurityLevel() == SecurityLevel::LOCKED);

    // Simple test with default XOR algorithm
    sm.setSeedKeyAlgorithm(1,
        [](const std::vector<uint8_t>& seed) -> std::vector<uint8_t> {
            return seed;
        },
        [](const std::vector<uint8_t>& seed, const std::vector<uint8_t>& key) -> bool {
            if (seed.size() != key.size()) return false;
            for (size_t i = 0; i < seed.size(); i++) {
                if (key[i] != (seed[i] ^ 0xFF)) return false;
            }
            return true;
        });

    REQUIRE(sm.requestSeed(1));
    REQUIRE_FALSE(sm.sendKey(1, {0x00, 0x00, 0x00, 0x00})); // wrong key
    REQUIRE(sm.getCurrentSecurityLevel() == SecurityLevel::LOCKED);
}

TEST_CASE("SessionManager_TesterPresent", "[session]") {
    SessionManager sm;
    REQUIRE_FALSE(sm.getTesterPresent());

    sm.setTesterPresent(true);
    REQUIRE(sm.getTesterPresent());
}

// ===== SecurityManager Tests =====

TEST_CASE("SecurityManager_GM28bit", "[security]") {
    SecurityManager sec;
    sec.registerAlgorithm("chevrolet", 1, SecurityManager::gmAlgorithm28bit());

    auto seed = sec.generateSeed("chevrolet", 1);
    REQUIRE(seed.size() == 4);

    // Compute expected key
    std::vector<uint8_t> key(4);
    for (int i = 0; i < 4; i++) {
        key[i] = static_cast<uint8_t>(~((seed[i] + 0x55 + i) & 0xFF));
    }
    REQUIRE(sec.validateKey("chevrolet", 1, seed, key));

    // Wrong key
    std::vector<uint8_t> wrong_key = {0x00, 0x00, 0x00, 0x00};
    REQUIRE_FALSE(sec.validateKey("chevrolet", 1, seed, wrong_key));
}

TEST_CASE("SecurityManager_Ford", "[security]") {
    SecurityManager sec;
    sec.registerAlgorithm("ford", 1, SecurityManager::fordAlgorithm());

    auto seed = sec.generateSeed("ford", 1);
    std::vector<uint8_t> key(4);
    for (int i = 0; i < 4; i++) {
        key[i] = static_cast<uint8_t>((seed[i] ^ 0x5A) + i);
    }
    REQUIRE(sec.validateKey("ford", 1, seed, key));
}

TEST_CASE("SecurityManager_Toyota", "[security]") {
    SecurityManager sec;
    sec.registerAlgorithm("toyota", 1, SecurityManager::toyotaAlgorithm());

    auto seed = sec.generateSeed("toyota", 1);
    std::vector<uint8_t> key(4);
    for (int i = 0; i < 4; i++) {
        key[i] = static_cast<uint8_t>((seed[i] + 0xAA) ^ 0x55);
    }
    REQUIRE(sec.validateKey("toyota", 1, seed, key));
}

// ===== DTC Manager Tests =====

TEST_CASE("DTCManager_Operations", "[dtc]") {
    DTCManager dtc;

    REQUIRE_FALSE(dtc.hasActiveDTCs());
    REQUIRE(dtc.count() == 0);

    dtc.addDTC(0x0101, "MAF circuit range/performance", DTC::Severity::MAJOR);
    REQUIRE(dtc.count() == 1);
    REQUIRE(dtc.hasActiveDTCs());
    REQUIRE(dtc.hasDTC(0x0101));

    dtc.addDTC(0x0300, "Random misfire", DTC::Severity::MAJOR, true); // pending
    REQUIRE(dtc.count() == 2);
    REQUIRE(dtc.hasPendingDTCs());

    auto active = dtc.getActiveDTCs();
    REQUIRE(active.size() == 1);
    REQUIRE(active[0].code == 0x0101);

    dtc.removeDTC(0x0101);
    REQUIRE(dtc.count() == 1);

    dtc.clearAll();
    REQUIRE(dtc.count() == 0);
}

TEST_CASE("DTCManager_CodeEncoding", "[dtc]") {
    REQUIRE(DTCManager::encodeCode("P0101") == 0x0101);
    REQUIRE(DTCManager::encodeCode("C1234") == 0x4234);
    REQUIRE(DTCManager::encodeCode("B2000") == 0x8200);
    REQUIRE(DTCManager::encodeCode("U1000") == 0xC100);

    REQUIRE(DTCManager::decodeCode(0x0101) == "P0101");
    REQUIRE(DTCManager::decodeCode(0x4234) == "C1234");
    REQUIRE(DTCManager::decodeCode(0xC100) == "U1000");
}

// ===== VIN Decoder Tests =====

TEST_CASE("VINDecoder_Decode", "[vin]") {
    auto info = VINDecoder::decode("1G1ZB5ST1GF100001");
    REQUIRE(info.wmi == "1G1");
    REQUIRE(info.manufacturer == "Chevrolet");
    REQUIRE(info.country == "USA");

    info = VINDecoder::decode("JT2BF22K1Y0123456");
    REQUIRE(info.wmi == "JT2");
    REQUIRE(info.manufacturer == "Toyota");

    info = VINDecoder::decode("WBA3A5C5XDF123456");
    REQUIRE(info.wmi == "WBA");
    REQUIRE(info.manufacturer == "BMW");
}

TEST_CASE("VINDecoder_Generate", "[vin]") {
    std::string vin = VINDecoder::generateVIN("Chevrolet", "Silverado", 2023);
    REQUIRE(vin.size() == 17);
    REQUIRE(vin.substr(0, 3) == "1G1");
}

// ===== OBD2 Standard Tests =====

TEST_CASE("OBD2Standard_PIDs", "[obd2]") {
    const auto& pids = OBD2Standard::getPIDs();
    REQUIRE(pids.size() > 0);
    REQUIRE(pids.find(0x0C) != pids.end()); // RPM
    REQUIRE(pids.find(0x0D) != pids.end()); // Speed
    REQUIRE(pids.find(0x05) != pids.end()); // Coolant

    REQUIRE(pids.at(0x0C).name == "Engine RPM");
    REQUIRE(pids.at(0x0D).unit == "km/h");
}

TEST_CASE("OBD2Standard_Decode", "[obd2]") {
    REQUIRE(OBD2Standard::decodePID(0x0D, {50}) == 50.0f); // speed A
    REQUIRE(OBD2Standard::decodePID(0x05, {135}) == 95.0f); // coolant A-40
    REQUIRE(OBD2Standard::decodePID(0x0C, {0, 0}) == 0.0f); // RPM
}

// ===== UDS Protocol Tests =====

TEST_CASE("UDSProtocol_Responses", "[uds]") {
    auto pos = UDSProtocol::buildPositiveResponse(0x22, {0x01, 0x02});
    REQUIRE(pos[0] == 0x62);
    REQUIRE(pos[1] == 0x01);

    auto neg = UDSProtocol::buildNegativeResponse(0x22, 0x12);
    REQUIRE(neg[0] == 0x7F);
    REQUIRE(neg[1] == 0x22);
    REQUIRE(neg[2] == 0x12);

    REQUIRE(UDSProtocol::isPositiveResponse(0x62));
    REQUIRE(UDSProtocol::isNegativeResponse(0x7F));
    REQUIRE_FALSE(UDSProtocol::isPositiveResponse(0x7F));
}

TEST_CASE("UDSProtocol_DIDs", "[uds]") {
    REQUIRE(UDSProtocol::isStandardDID(0xF100));
    REQUIRE(UDSProtocol::isOBDDID(0xF180));
    REQUIRE(UDSProtocol::isManufacturerDID(0x1000));
    REQUIRE_FALSE(UDSProtocol::isOBDDID(0x1234));
}

// ===== CAN TP Tests =====

TEST_CASE("CANTransportProtocol_SingleFrame", ["cantp"]) {
    CANTransportProtocol tp;

    auto frames = tp.prepareMessage({0x7E0, 0x7E8, false, {0x01, 0x0C}});
    REQUIRE(frames.size() == 1);
    REQUIRE(frames[0].second[0] == 0x02); // single frame, len=2
}

TEST_CASE("CANTransportProtocol_MultiFrame", ["cantp"]) {
    CANTransportProtocol tp;

    std::vector<uint8_t> large_msg(50, 0xAA);
    auto frames = tp.prepareMessage({0x7E0, 0x7E8, false, large_msg});
    REQUIRE(frames.size() > 1); // should be broken into multiple frames
    REQUIRE((frames[0].second[0] & 0xF0) == 0x10); // first frame
}

// ===== Driving Cycle Tests =====

TEST_CASE("DrivingCycle_Urban", "[simulation]") {
    DrivingCycle cycle(DrivingCycle::URBAN);
    REQUIRE(cycle.getType() == DrivingCycle::URBAN);
    REQUIRE(cycle.getTypeName() == "urban");

    auto pt = cycle.getPoint(0);
    REQUIRE(pt.rpm >= 0);

    auto full = cycle.getFullCycle();
    REQUIRE(full.size() > 0);
    REQUIRE(cycle.getTotalDuration() > 0);
}

TEST_CASE("DrivingCycle_Highway", "[simulation]") {
    DrivingCycle cycle(DrivingCycle::HIGHWAY);
    auto pt = cycle.getPoint(15);
    REQUIRE(pt.speed >= 0);
}

// ===== Sensor Simulator Tests =====

TEST_CASE("SensorSimulator_Basic", "[simulation]") {
    SensorSimulator sim;

    sim.setEngineRPM(2500);
    sim.setVehicleSpeed(80);
    REQUIRE(sim.getEngineRPM() == 2500);
    REQUIRE(sim.getVehicleSpeed() == 80);

    sim.updateDriving(3000, 100, 0.05f);
    REQUIRE(sim.getEngineRPM() > 0);
    REQUIRE(sim.getCoolantTemp() > 80);
    REQUIRE(sim.getBatteryVoltage() > 10);
}

// ===== Fault Injector Tests =====

TEST_CASE("FaultInjector_Basic", "[simulation]") {
    FaultInjector fi;
    REQUIRE_FALSE(fi.isEnabled());

    fi.enable(true);
    REQUIRE(fi.isEnabled());

    fi.setProbability(1.0f);
    REQUIRE(fi.getProbability() == 1.0f);

    fi.addDTC(0x0101, "Test fault");
    fi.addDTC(0x0300, "Another fault");

    uint16_t code;
    REQUIRE(fi.shouldTrigger(&code));
    REQUIRE(code != 0);
}

// ===== Environment Tests =====

TEST_CASE("EnvironmentSimulator_Basic", "[simulation]") {
    EnvironmentSimulator env;
    REQUIRE(env.getAmbientTemp() == 25.0f);
    REQUIRE(env.getBarometricPressure() > 0);

    env.setAltitude(1000);
    REQUIRE(env.getAltitude() == 1000);
    REQUIRE(env.getBarometricPressure() < 101.0f); // lower at altitude

    env.update(1.0f);
}

// ===== Access Control Tests =====

TEST_CASE("AccessControl_Authentication", "[security]") {
    AccessControl ac;

    REQUIRE(ac.authenticate("admin", "admin123"));
    REQUIRE_FALSE(ac.authenticate("admin", "wrong"));

    // Authorization levels
    REQUIRE(ac.authorize("admin", AccessControl::FULL));
    REQUIRE(ac.authorize("diag", AccessControl::DIAGNOSTIC));
    REQUIRE_FALSE(ac.authorize("diag", AccessControl::PROGRAMMING));
}

TEST_CASE("AccessControl_Lockout", "[security]") {
    AccessControl ac;
    ac.setMaxAttempts(3);

    REQUIRE(ac.authenticate("admin", "admin123")); // works
    REQUIRE_FALSE(ac.authenticate("admin", "wrong")); // fail 1
    REQUIRE_FALSE(ac.authenticate("admin", "wrong")); // fail 2
    REQUIRE_FALSE(ac.authenticate("admin", "wrong")); // fail 3 - lockout
    REQUIRE_FALSE(ac.authenticate("admin", "admin123")); // locked out

    ac.unlockUser("admin");
    REQUIRE(ac.authenticate("admin", "admin123")); // works again
}

// ===== Metrics Exporter Tests =====

TEST_CASE("MetricsExporter_Basic", "[logging]") {
    MetricsExporter m;

    m.increment("test_counter");
    REQUIRE(m.get("test_counter") == 1.0);

    m.increment("test_counter", 5);
    REQUIRE(m.get("test_counter") == 6.0);

    m.gauge("test_gauge", 42.5);
    REQUIRE(m.get("test_gauge") == 42.5);

    auto prom = m.getPrometheusText();
    REQUIRE(prom.find("test_gauge") != std::string::npos);
    REQUIRE(prom.find("42.5") != std::string::npos);
}

// ===== CAN Logger Tests =====

TEST_CASE("CANLogger_Levels", "[logging]") {
    CANLogger logger("/tmp/test_ecu.log", CANLogger::INFO);
    REQUIRE(logger.getLevel() == CANLogger::INFO);

    logger.setLevel(CANLogger::DEBUG);
    REQUIRE(logger.getLevel() == CANLogger::DEBUG);
}

// ===== Replay Analyzer Tests =====

TEST_CASE("ReplayAnalyzer_Basic", "[logging]") {
    ReplayAnalyzer ra;

    ra.recordFrame(0x7E0, {0x01, 0x0C}, true);
    ra.recordFrame(0x7E8, {0x41, 0x0C, 0x0E, 0x10}, false);

    REQUIRE(ra.getAllFrames().size() == 2);

    auto report = ra.analyze();
    REQUIRE(report.total_frames == 2);
    REQUIRE(report.tx_count == 1);
    REQUIRE(report.rx_count == 1);
}

// ===== Calibration Manager Tests =====

TEST_CASE("CalibrationManager_Basic", "[diagnostics]") {
    CalibrationManager cm;
    REQUIRE(cm.getCVN() != 0);
    REQUIRE_FALSE(cm.getCalibrationID().empty());
    REQUIRE_FALSE(cm.getSoftwareVersion().empty());
}

// ===== Freeze Frame Manager Tests =====

TEST_CASE("FreezeFrameManager_Basic", "[diagnostics]") {
    FreezeFrameManager ffm;
    REQUIRE(ffm.count() == 0);

    std::unordered_map<uint8_t, float> values = {{0x0C, 2500}, {0x0D, 80}};
    ffm.captureFrame(0x0101, values);
    REQUIRE(ffm.count() == 1);

    auto frames = ffm.getForDTC(0x0101);
    REQUIRE(frames.size() == 1);
    REQUIRE(frames[0].pid_values.at(0x0C) == 2500);

    ffm.clearAll();
    REQUIRE(ffm.count() == 0);
}

// ===== Readiness Monitor Tests =====

TEST_CASE("ReadinessMonitor_Basic", "[diagnostics]") {
    ReadinessMonitor rm;
    REQUIRE(rm.allComplete());

    rm.setComplete(0, false);
    REQUIRE_FALSE(rm.allComplete());

    auto monitors = rm.getAllMonitors();
    REQUIRE(monitors.size() > 0);

    auto status = rm.getOBDStatus();
    REQUIRE(status.size() == 5);
}

// ===== Base Manufacturer Tests =====

TEST_CASE("BaseManufacturer_PIDs", "[manufacturer]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    // Using Chevrolet as concrete implementation of base
    auto chevy = std::make_unique<ChevroletGM>(can);

    REQUIRE(chevy->getManufacturerId() == Manufacturer::CHEVROLET);
    REQUIRE_FALSE(chevy->getManufacturerName().empty());

    // PID support
    REQUIRE(chevy->isPIDSupported(0x0C));
    REQUIRE(chevy->isPIDSupported(0x0D));
    REQUIRE(chevy->isPIDSupported(0x05));
    REQUIRE_FALSE(chevy->isPIDSupported(0xFF));

    // Mode 01 response
    auto resp = chevy->handleMode01(0x0D);
    REQUIRE(resp.size() >= 2);
    REQUIRE(resp[0] == 0x41); // Mode 01 response prefix
}

TEST_CASE("BaseManufacturer_DTCManagement", "[manufacturer]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    auto chevy = std::make_unique<ChevroletGM>(can);

    chevy->setDTC(0x0101, "Test DTC", false);
    auto dtcs = chevy->getDTCs(true);
    REQUIRE(dtcs.size() == 1);
    REQUIRE(dtcs[0].code == 0x0101);

    REQUIRE(chevy->clearDTCs());
    REQUIRE(chevy->getDTCs(true).empty());
}

TEST_CASE("BaseManufacturer_VIN", "[manufacturer]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    auto chevy = std::make_unique<ChevroletGM>(can);

    chevy->setVIN("1G1ZB5ST1GF100001");
    REQUIRE(chevy->getVIN() == "1G1ZB5ST1GF100001");
}

TEST_CASE("BaseManufacturer_Mode09", "[manufacturer]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    auto chevy = std::make_unique<ChevroletGM>(can);

    auto vin_resp = chevy->handleMode09(0x01);
    REQUIRE(vin_resp.size() > 2);
    REQUIRE(vin_resp[0] == 0x09);
}

TEST_CASE("BaseManufacturer_SimulationSetters", "[manufacturer]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    auto chevy = std::make_unique<ChevroletGM>(can);

    chevy->setEngineRPM(3000);
    REQUIRE(chevy->getEngineRPM() == 3000);

    chevy->setVehicleSpeed(100);
    REQUIRE(chevy->getVehicleSpeed() == 100);

    chevy->setCoolantTemp(90);
    REQUIRE(chevy->getCoolantTemp() == 90);
}

// ===== All Manufacturers Basic Tests =====

TEST_CASE("AllManufacturers_Basic", "[manufacturer]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);

    std::vector<std::unique_ptr<BaseManufacturer>> manufacturers;
    manufacturers.push_back(std::make_unique<ChevroletGM>(can));
    manufacturers.push_back(std::make_unique<Ford>(can));
    manufacturers.push_back(std::make_unique<Toyota>(can));
    manufacturers.push_back(std::make_unique<BMW>(can));
    manufacturers.push_back(std::make_unique<Volkswagen>(can));
    manufacturers.push_back(std::make_unique<Mercedes>(can));
    manufacturers.push_back(std::make_unique<Honda>(can));
    manufacturers.push_back(std::make_unique<Nissan>(can));

    REQUIRE(manufacturers.size() == 8);

    for (auto& mfr : manufacturers) {
        REQUIRE_FALSE(mfr->getManufacturerName().empty());
        REQUIRE(mfr->getManufacturerId() != Manufacturer::UNKNOWN);

        mfr->setEngineRPM(1500);
        REQUIRE(mfr->getEngineRPM() == 1500);

        mfr->setVIN("TESTVIN1234567890");
        REQUIRE(mfr->getVIN() == "TESTVIN1234567890");

        auto dtcs = mfr->getDTCs();
        REQUIRE(dtcs.empty());

        REQUIRE(mfr->isPIDSupported(0x0C));
        REQUIRE(mfr->isPIDSupported(0x0D));
    }
}

// ===== GMLAN Protocol Tests =====

TEST_CASE("GMLAN_DTCEncoding", "[protocols]") {
    REQUIRE(GMLANProtocol::encodeGMDTC("P0101") == 0x0101);
    REQUIRE(GMLANProtocol::decodeGMDTC(0x0101) == "P0101");
    REQUIRE(GMLANProtocol::decodeGMDTC(0x4234) == "C1234");
}

// ===== KWP2000 Tests =====

TEST_CASE("KWP2000_Checksum", "[protocols]") {
    std::vector<uint8_t> data = {0x81, 0x10, 0xF1};
    uint8_t cs = KWP2000Protocol::calculateChecksum(data);
    REQUIRE(cs == (0x81 + 0x10 + 0xF1) & 0xFF);
}

// ===== CAN Logger Level Tests =====

TEST_CASE("CANLogger_LevelConversion", "[logging]") {
    REQUIRE(CANLogger::levelToString(CANLogger::INFO) == "INFO");
    REQUIRE(CANLogger::levelToString(CANLogger::ERROR) == "ERROR");
    REQUIRE(CANLogger::levelFromString("info") == CANLogger::INFO);
    REQUIRE(CANLogger::levelFromString("error") == CANLogger::ERROR);
    REQUIRE(CANLogger::levelFromString("unknown") == CANLogger::INFO); // default
}

// ===== DBManager Basic =====

TEST_CASE("DBManager_Basic", "[database]") {
    DBManager db(":memory:"); // in-memory database for testing
    REQUIRE(db.open());
    REQUIRE(db.runMigrations());
    REQUIRE(db.isOpen());
    db.close();
}

// ===== Access Level Conversion =====

TEST_CASE("AccessControl_LevelConversion", "[security]") {
    REQUIRE(AccessControl::levelFromString("full") == AccessControl::FULL);
    REQUIRE(AccessControl::levelFromString("diagnostic") == AccessControl::DIAGNOSTIC);
    REQUIRE(AccessControl::levelToString(AccessControl::ENGINEERING) == "engineering");
    REQUIRE(AccessControl::levelToString(AccessControl::NONE) == "none");
}
