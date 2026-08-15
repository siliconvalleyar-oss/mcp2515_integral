// Manufacturer-specific tests
// Each manufacturer's unique features validated

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include "../core/can_manager.hpp"
#include "../manufacturers/chevrolet.hpp"
#include "../manufacturers/ford.hpp"
#include "../manufacturers/toyota.hpp"
#include "../manufacturers/bmw.hpp"
#include "../manufacturers/volkswagen.hpp"
#include "../manufacturers/mercedes.hpp"
#include "../manufacturers/honda.hpp"
#include "../manufacturers/nissan.hpp"

using namespace ecumult;

// Shared CAN interface for tests
static auto test_can = std::make_shared<CANManager>("vcan0", 500000);

TEST_CASE("Chevrolet_GM_Mode22", "[manufacturer][gm]") {
    ChevroletGM chevy(test_can);

    // GMLAN Mode 22 Read DID
    auto resp = chevy.handleGMLANMode22(0xF190, {});
    REQUIRE_FALSE(resp.empty());
    REQUIRE(resp[0] == 0x62); // positive response to 0x22

    // Engine RPM DID
    chevy.setEngineRPM(3000);
    resp = chevy.handleGMLANMode22(0xF181, {});
    REQUIRE(resp.size() >= 4);
}

TEST_CASE("Chevrolet_GM_DTCEncoding", "[manufacturer][gm]") {
    uint16_t code = ChevroletGM::dtcStringToCode("P0101");
    REQUIRE(code == 0x0101);
    REQUIRE(ChevroletGM::dtcCodeToString(code) == "P0101");

    REQUIRE(ChevroletGM::dtcCodeToString(0x4234) == "C1234");
}

TEST_CASE("Ford_EnhancedPIDs", "[manufacturer][ford]") {
    Ford ford(test_can);

    auto resp = ford.handleFordEnhancedPIDs(0x1001);
    REQUIRE(resp.size() >= 3);
    REQUIRE(resp[0] == 0x62); // positive response

    resp = ford.handleFordEnhancedPIDs(0x2001);
    REQUIRE(resp.size() >= 3);
}

TEST_CASE("Ford_PATS", "[manufacturer][ford]") {
    Ford ford(test_can);
    auto resp = ford.handleFordPATS({0x01});
    REQUIRE_FALSE(resp.empty());
}

TEST_CASE("Toyota_Mode21", "[manufacturer][toyota]") {
    Toyota toyota(test_can);

    auto resp = toyota.handleMode21(0x01);
    REQUIRE_FALSE(resp.empty());
    REQUIRE(resp[0] == 0x61); // positive response to custom mode
}

TEST_CASE("Toyota_Mode22", "[manufacturer][toyota]") {
    Toyota toyota(test_can);
    auto resp = toyota.handleMode22(0x01);
    REQUIRE(resp.size() >= 2);
}

TEST_CASE("Toyota_Immobilizer", "[manufacturer][toyota]") {
    Toyota toyota(test_can);
    auto resp = toyota.handleImmobilizer({0x01});
    REQUIRE_FALSE(resp.empty());
}

TEST_CASE("BMW_DCAN", "[manufacturer][bmw]") {
    BMW bmw(test_can);
    auto resp = bmw.handleBMWDCAN({0x10, 0x03});
    REQUIRE_FALSE(resp.empty());
}

TEST_CASE("BMW_EDIABAS_Jobs", "[manufacturer][bmw]") {
    BMW bmw(test_can);

    auto resp = bmw.handleBMWJob("STEUERNR_LESEN", {});
    REQUIRE_FALSE(resp.empty());
    REQUIRE(resp[0] == 0x62);

    resp = bmw.handleBMWJob("CODIERDATEN_LESEN", {});
    REQUIRE_FALSE(resp.empty());
}

TEST_CASE("VW_Adaptation", "[manufacturer][vw]") {
    Volkswagen vw(test_can);
    auto resp = vw.handleVWAdaptation(0x01, {});
    REQUIRE_FALSE(resp.empty());
    REQUIRE(resp[0] == 0x62);
}

TEST_CASE("VW_ComponentProtection", "[manufacturer][vw]") {
    Volkswagen vw(test_can);
    auto resp = vw.handleVWComponentProtection({});
    REQUIRE_FALSE(resp.empty());
}

TEST_CASE("Mercedes_DAS", "[manufacturer][mercedes]") {
    Mercedes mb(test_can);
    auto resp = mb.handleMercedesDAS({});
    REQUIRE_FALSE(resp.empty());
}

TEST_CASE("Mercedes_SBC", "[manufacturer][mercedes]") {
    Mercedes mb(test_can);
    auto resp = mb.handleMercedesSBC({});
    REQUIRE_FALSE(resp.empty());
}

TEST_CASE("Honda_HDS", "[manufacturer][honda]") {
    Honda honda(test_can);
    auto resp = honda.handleHondaHDS({});
    REQUIRE_FALSE(resp.empty());
}

TEST_CASE("Honda_VTEC", "[manufacturer][honda]") {
    Honda honda(test_can);
    auto resp = honda.handleHondaVTEC({});
    REQUIRE_FALSE(resp.empty());
}

TEST_CASE("Nissan_ConsultIII", "[manufacturer][nissan]") {
    Nissan nissan(test_can);
    auto resp = nissan.handleConsultIII({});
    REQUIRE_FALSE(resp.empty());
}

TEST_CASE("Nissan_NATS", "[manufacturer][nissan]") {
    Nissan nissan(test_can);
    auto resp = nissan.handleNissanNATS({});
    REQUIRE_FALSE(resp.empty());
}
