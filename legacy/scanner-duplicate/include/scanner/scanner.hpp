#ifndef SCANNER_SCANNER_HPP
#define SCANNER_SCANNER_HPP

#include <memory>
#include <string>
#include "hardware/mcp2515.hpp"
#include "hardware/ssd1306.hpp"
#include "hardware/spi.hpp"
#include "hardware/i2c.hpp"
#include "scanner/display.hpp"
#include "scanner/menu.hpp"
#include "scanner/obd2.hpp"
#include "scanner/dtc.hpp"
#include "scanner/live_data.hpp"
#include "scanner/active_test.hpp"

namespace Scanner {

class AutelScanner {
public:
    AutelScanner();
    ~AutelScanner();

    bool initialize();
    void run();
    void shutdown();

    // Getters
    std::shared_ptr<Display> getDisplay() { return ui_; }
    std::shared_ptr<OBD2> getOBD2() { return obd2_; }
    std::shared_ptr<DTCManager> getDTCManager() { return dtcManager_; }
    std::shared_ptr<LiveData> getLiveData() { return liveData_; }
    std::shared_ptr<ActiveTest> getActiveTest() { return activeTest_; }

    // Logging
    void log(const std::string& message);
    void logError(const std::string& error);

private:
    bool setupHardware();
    bool setupCAN();
    bool setupDisplay();
    bool setupMenu();

    std::shared_ptr<Hardware::SPI> spi_;
    std::shared_ptr<Hardware::I2C> i2c_;
    std::shared_ptr<Hardware::MCP2515> mcp2515_;
    std::shared_ptr<Hardware::SSD1306> display_;
    std::shared_ptr<Display> ui_;
    std::shared_ptr<OBD2> obd2_;
    std::shared_ptr<DTCManager> dtcManager_;
    std::shared_ptr<LiveData> liveData_;
    std::shared_ptr<ActiveTest> activeTest_;
    std::shared_ptr<Menu> menu_;

    bool initialized_;
};

} // namespace Scanner

#endif // SCANNER_SCANNER_HPP
