#ifndef SCANNER_ACTIVE_TEST_HPP
#define SCANNER_ACTIVE_TEST_HPP

#include <string>
#include <vector>
#include <functional>
#include "scanner/obd2.hpp"

namespace Scanner {

struct TestItem {
    std::string id;
    std::string name;
    std::string category;
    std::function<bool(bool)> testFunction;  // Returns true if successful
    bool supported;
};

class ActiveTest {
public:
    ActiveTest(std::shared_ptr<OBD2> obdInterface);
    ~ActiveTest();

    bool initialize();
    void cleanup();

    // Test execution
    bool runTest(const std::string& testId, bool activate);
    bool runAllTests();

    // Test discovery
    std::vector<TestItem> getAvailableTests() const;
    std::vector<TestItem> getTestsByCategory(const std::string& category) const;

    // Predefined tests
    bool testEVAPValve(bool activate);
    bool testFuelPumpRelay(bool activate);
    bool testFanRelay(bool activate);
    bool testACClutch(bool activate);
    bool testThrottleBody(bool activate);
    bool testInjectors(bool activate);

private:
    std::shared_ptr<OBD2> obd_;
    bool initialized_;
    std::vector<TestItem> tests_;
    std::unordered_map<std::string, size_t> testIndex_;

    void loadTests();
    bool sendTestCommand(uint8_t mode, uint8_t pid, uint8_t* response, size_t& length);
};

} // namespace Scanner

#endif // SCANNER_ACTIVE_TEST_HPP
