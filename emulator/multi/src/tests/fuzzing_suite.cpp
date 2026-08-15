// Fuzzing test suite for ECU emulator robustness
// Sends random CAN frames and verifies no crashes

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <random>
#include <vector>
#include "../core/can_manager.hpp"
#include "../core/protocol_router.hpp"
#include "../manufacturers/chevrolet.hpp"
#include "../manufacturers/ford.hpp"
#include "../manufacturers/toyota.hpp"
#include "../manufacturers/bmw.hpp"
#include "../manufacturers/volkswagen.hpp"
#include "../manufacturers/mercedes.hpp"
#include "../manufacturers/honda.hpp"
#include "../manufacturers/nissan.hpp"

using namespace ecumult;

static std::mt19937 fuzz_rng(12345);

// Generate random CAN frame data
static CANFrame randomFrame() {
    CANFrame cf;
    std::uniform_int_distribution<uint32_t> id_dist(0x000, 0x7FF);
    std::uniform_int_distribution<int> dlc_dist(1, 8);
    std::uniform_int_distribution<int> byte_dist(0, 255);

    cf.frame.can_id = id_dist(fuzz_rng);
    cf.frame.can_dlc = dlc_dist(fuzz_rng);
    cf.bus_id = 0;
    clock_gettime(CLOCK_MONOTONIC, &cf.timestamp);

    for (int i = 0; i < cf.frame.can_dlc; i++) {
        cf.frame.data[i] = byte_dist(fuzz_rng);
    }
    return cf;
}

// Run fuzzing against all manufacturers
static void fuzzManufacturer(BaseManufacturer* mfr, int iterations = 500) {
    for (int i = 0; i < iterations; i++) {
        auto frame = randomFrame();
        REQUIRE_NOTHROW(mfr->handleCanFrame(frame));
    }
}

TEST_CASE("Fuzzing_Chevrolet", "[fuzzing]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    ChevroletGM chevy(can);
    fuzzManufacturer(&chevy);
}

TEST_CASE("Fuzzing_Ford", "[fuzzing]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    Ford ford(can);
    fuzzManufacturer(&ford);
}

TEST_CASE("Fuzzing_Toyota", "[fuzzing]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    Toyota toyota(can);
    fuzzManufacturer(&toyota);
}

TEST_CASE("Fuzzing_BMW", "[fuzzing]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    BMW bmw(can);
    fuzzManufacturer(&bmw);
}

TEST_CASE("Fuzzing_Volkswagen", "[fuzzing]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    Volkswagen vw(can);
    fuzzManufacturer(&vw);
}

TEST_CASE("Fuzzing_Mercedes", "[fuzzing]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    Mercedes mb(can);
    fuzzManufacturer(&mb);
}

TEST_CASE("Fuzzing_Honda", "[fuzzing]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    Honda honda(can);
    fuzzManufacturer(&honda);
}

TEST_CASE("Fuzzing_Nissan", "[fuzzing]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);
    Nissan nissan(can);
    fuzzManufacturer(&nissan);
}

// Fuzz all manufacturers in sequence with more iterations
TEST_CASE("Fuzzing_AllManufacturers", "[fuzzing]") {
    auto can = std::make_shared<CANManager>("vcan0", 500000);

    std::vector<std::unique_ptr<BaseManufacturer>> mfrs;
    mfrs.push_back(std::make_unique<ChevroletGM>(can));
    mfrs.push_back(std::make_unique<Ford>(can));
    mfrs.push_back(std::make_unique<Toyota>(can));
    mfrs.push_back(std::make_unique<BMW>(can));
    mfrs.push_back(std::make_unique<Volkswagen>(can));
    mfrs.push_back(std::make_unique<Mercedes>(can));
    mfrs.push_back(std::make_unique<Honda>(can));
    mfrs.push_back(std::make_unique<Nissan>(can));

    for (auto& mfr : mfrs) {
        // Need to convert to BaseManufacturer* for the fuzzer
        auto* base = mfr.get();
        for (int i = 0; i < 1000; i++) {
            auto frame = randomFrame();
            REQUIRE_NOTHROW(base->handleCanFrame(frame));
        }
    }
}
