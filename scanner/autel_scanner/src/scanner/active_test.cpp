#include "scanner/active_test.hpp"
#include <algorithm>
#include <thread>

namespace Scanner {

ActiveTest::ActiveTest(std::shared_ptr<OBD2> obdInterface)
    : obd_(std::move(obdInterface)), initialized_(false) {}

ActiveTest::~ActiveTest() = default;

bool ActiveTest::initialize() {
    loadTests();
    initialized_ = true;
    return true;
}

void ActiveTest::cleanup() {
    tests_.clear();
    testIndex_.clear();
    initialized_ = false;
}

bool ActiveTest::runTest(const std::string& testId, bool activate) {
    auto it = testIndex_.find(testId);
    if (it == testIndex_.end()) {
        return false;
    }

    const TestItem& test = tests_[it->second];
    if (!test.supported || !test.testFunction) {
        return false;
    }

    return test.testFunction(activate);
}

bool ActiveTest::runAllTests() {
    bool allPassed = true;
    for (auto& test : tests_) {
        if (test.supported) {
            bool result = test.testFunction(true);
            if (!result) {
                allPassed = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            test.testFunction(false);
        }
    }
    return allPassed;
}

std::vector<TestItem> ActiveTest::getAvailableTests() const {
    std::vector<TestItem> available;
    for (const auto& test : tests_) {
        if (test.supported) {
            available.push_back(test);
        }
    }
    return available;
}

std::vector<TestItem> ActiveTest::getTestsByCategory(const std::string& category) const {
    std::vector<TestItem> filtered;
    for (const auto& test : tests_) {
        if (test.supported && test.category == category) {
            filtered.push_back(test);
        }
    }
    return filtered;
}

bool ActiveTest::testEVAPValve([[maybe_unused]] bool activate) {
    if (!obd_) return false;
    uint8_t response[8] = {0};
    size_t length = 0;
    return obd_->sendOBD2Request(0x02, 0x0A, response, length);
}

bool ActiveTest::testFuelPumpRelay([[maybe_unused]] bool activate) {
    if (!obd_) return false;
    uint8_t response[8] = {0};
    size_t length = 0;
    return obd_->sendOBD2Request(0x02, 0x0B, response, length);
}

bool ActiveTest::testFanRelay([[maybe_unused]] bool activate) {
    if (!obd_) return false;
    uint8_t response[8] = {0};
    size_t length = 0;
    return obd_->sendOBD2Request(0x02, 0x0C, response, length);
}

bool ActiveTest::testACClutch([[maybe_unused]] bool activate) {
    if (!obd_) return false;
    uint8_t response[8] = {0};
    size_t length = 0;
    return obd_->sendOBD2Request(0x02, 0x0D, response, length);
}

bool ActiveTest::testThrottleBody([[maybe_unused]] bool activate) {
    if (!obd_) return false;
    uint8_t response[8] = {0};
    size_t length = 0;
    return obd_->sendOBD2Request(0x02, 0x0E, response, length);
}

bool ActiveTest::testInjectors([[maybe_unused]] bool activate) {
    if (!obd_) return false;
    uint8_t response[8] = {0};
    size_t length = 0;
    return obd_->sendOBD2Request(0x02, 0x19, response, length);
}

void ActiveTest::loadTests() {
    tests_ = {
        {"evap_valve", "Valvula Solenoide EVAP", "Emisiones", [this](bool on) { return testEVAPValve(on); }, true},
        {"fuel_pump", "Bomba de Combustible", "Combustible", [this](bool on) { return testFuelPumpRelay(on); }, true},
        {"fan_relay", "Ventilador Refrigeracion", "Refrigeracion", [this](bool on) { return testFanRelay(on); }, true},
        {"ac_clutch", "Compresor A/A", "Aire Acondicionado", [this](bool on) { return testACClutch(on); }, true},
        {"throttle", "Cuerpo Mariposa", "Admision", [this](bool on) { return testThrottleBody(on); }, true},
        {"injectors", "Inyectores", "Combustible", [this](bool on) { return testInjectors(on); }, true}
    };

    for (size_t i = 0; i < tests_.size(); ++i) {
        testIndex_[tests_[i].id] = i;
    }
}

bool ActiveTest::sendTestCommand(uint8_t mode, uint8_t pid, uint8_t* response, size_t& length) {
    if (!obd_) return false;
    return obd_->sendOBD2Request(mode, pid, response, length);
}

} // namespace Scanner
