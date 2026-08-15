#include "scanner/scanner.hpp"
#include "scanner/event_log.hpp"
#include "scanner/config.hpp"
#include "hardware/gpio.hpp"

namespace Scanner {

AutelScanner::AutelScanner()
    : initialized_(false) {
    Config::instance().load();
}

AutelScanner::~AutelScanner() {
    shutdown();
}

bool AutelScanner::initialize() {
    if (!setupHardware()) {
        logError("Error inicializando hardware");
        return false;
    }

    if (!setupCAN()) {
        logError("Error inicializando CAN bus");
        return false;
    }

    if (!setupDisplay()) {
        logError("Error inicializando display");
        return false;
    }

    // Initialize subsystems
    obd2_ = std::make_shared<OBD2>(mcp2515_);
    if (!obd2_->initialize()) {
        logError("Error inicializando OBD2");
        return false;
    }

    dtcManager_ = std::make_shared<DTCManager>();
    liveData_ = std::make_shared<LiveData>(obd2_);
    if (!liveData_->initialize()) {
        logError("Error inicializando LiveData");
        return false;
    }

    activeTest_ = std::make_shared<ActiveTest>(obd2_);
    if (!activeTest_->initialize()) {
        logError("Error inicializando ActiveTest");
        return false;
    }

    if (!setupMenu()) {
        logError("Error inicializando Menu");
        return false;
    }

    initialized_ = true;
    log("Sistema listo");
    return true;
}

void AutelScanner::run(const std::string& version) {
    if (!initialized_) {
        logError("Scanner no inicializado");
        return;
    }

    log("Iniciando AUTEL Scanner v" + (version.empty() ? "?" : version));
    if (ui_) {
        ui_->drawBootScreen(version);
    }

    menu_->run();
}

void AutelScanner::shutdown() {
    if (initialized_) {
        log("Apagando scanner...");
    }

    menu_.reset();
    liveData_.reset();
    activeTest_.reset();
    dtcManager_.reset();
    obd2_.reset();
    display_.reset();
    mcp2515_.reset();
    i2c_.reset();
    spi_.reset();

    initialized_ = false;
}

bool AutelScanner::setupHardware() {
    // GPIO via bcm2835 (CS del MCP2515, INT)
    if (!Hardware::GPIO::initialize()) {
        logError("No se pudo inicializar GPIO (bcm2835)");
        return false;
    }

    // SPI for MCP2515
    spi_ = std::make_shared<Hardware::SPI>(0, 500000);
    if (!spi_->initialize()) {
        logError("No se pudo inicializar SPI");
        return false;
    }

    // I2C for OLED
    i2c_ = std::make_shared<Hardware::I2C>(1, 0x3C);
    if (!i2c_->initialize()) {
        logError("No se pudo inicializar I2C");
        return false;
    }

    log("Hardware inicializado correctamente");
    return true;
}

bool AutelScanner::setupCAN() {
    mcp2515_ = std::make_shared<Hardware::MCP2515>(8, spi_);  // CS pin 8
    if (!mcp2515_->initialize(Hardware::MCP2515::Bitrate::BPS_500K)) {
        logError("No se pudo inicializar MCP2515");
        return false;
    }

    log("CAN bus inicializado (500 kbps)");
    return true;
}

bool AutelScanner::setupDisplay() {
    display_ = std::make_shared<Hardware::SSD1306>(0x3C, 128, 32);
    if (!display_->initialize()) {
        // Modo headless: sin OLED el scanner sigue funcionando por CAN.
        logError("SSD1306 no detectado - modo headless");
        display_.reset();
        return true;
    }

    ui_ = std::make_shared<Display>(display_);
    if (!ui_->initialize()) {
        logError("No se pudo inicializar UI");
        return false;
    }
    ui_->setBrightness(Config::instance().brightness());

    log("Display SSD1306 inicializado");
    return true;
}

bool AutelScanner::setupMenu() {
    menu_ = std::make_shared<Menu>(ui_);
    menu_->setDependencies(obd2_, dtcManager_, liveData_, activeTest_);
    if (!menu_->initialize()) {
        logError("No se pudo inicializar Menu");
        return false;
    }

    return true;
}

void AutelScanner::log(const std::string& message) {
    EventLog::instance().info(message);
}

void AutelScanner::logError(const std::string& error) {
    EventLog::instance().error(error);
    if (ui_) {
        ui_->drawError(error);
    }
}

} // namespace Scanner
