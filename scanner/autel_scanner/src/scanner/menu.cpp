#include "scanner/menu.hpp"
#include "scanner/event_log.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <thread>
#include <functional>
#include <unistd.h>
#include <sys/select.h>

namespace Scanner {

Menu::Menu(std::shared_ptr<Display> display)
    : display_(std::move(display)), obd2_(nullptr), dtcManager_(nullptr),
      liveData_(nullptr), activeTest_(nullptr), running_(false), selectedIndex_(0) {}

Menu::~Menu() {
    cleanup();
}

bool Menu::initialize() {
    if (!display_) return false;
    buildMenuTree();
    return true;
}

void Menu::cleanup() {
    running_ = false;
}

void Menu::setDependencies(std::shared_ptr<OBD2> obd2,
                           std::shared_ptr<DTCManager> dtcManager,
                           std::shared_ptr<LiveData> liveData,
                           std::shared_ptr<ActiveTest> activeTest) {
    obd2_ = std::move(obd2);
    dtcManager_ = std::move(dtcManager);
    liveData_ = std::move(liveData);
    activeTest_ = std::move(activeTest);
}

void Menu::buildMenuTree() {
    // Main menu items
    root_.children = {
        {"diagnostics", "Diagnostics", "", nullptr, {}},
        {"data_manager", "Data Manager", "", nullptr, {}},
        {"settings", "Settings", "", nullptr, {}},
        {"service", "Service", "", nullptr, {}},
        {"exit", "Salir", "", nullptr, {}}
    };

    // Diagnostics submenu
    auto* diag = &root_.children[0];
    diag->children = {
        {"auto_scan", "Auto Scan", "", [this] { autoScan(); }, {}},
        {"control_units", "Control Units", "", [this] { showInfo(); }, {}},
        {"read_codes", "Leer Codigos", "", [this] { readCodes(); }, {}},
        {"erase_codes", "Borrar Codigos", "", [this] { eraseCodes(); }, {}},
        {"live_data", "Datos en Vivo", "", nullptr, {}},
        {"active_test", "Pruebas Activas", "", [this] { runActiveTest(); }, {}},
        {"special_functions", "Funciones Especiales", "", [this] { logStub("Funciones Especiales"); }, {}},
        {"back", "Volver", "", nullptr, {}}
    };

    // Live Data submenu
    auto* liveData = &diag->children[4];
    liveData->children = {
        {"custom_list", "Lista Personalizada", "", [this] { logStub("Lista Personalizada"); }, {}},
        {"all_params", "Todos los Parametros", "", [this] { showLiveData(); }, {}},
        {"back", "Volver", "", nullptr, {}}
    };

    // Service submenu
    auto* service = &root_.children[3];
    service->children = {
        {"oil_reset", "Reset Aceite", "", [this] { logStub("Reset Aceite"); }, {}},
        {"tpms_service", "TPMS", "", [this] { logStub("TPMS"); }, {}},
        {"epb_service", "EPB", "", [this] { logStub("EPB"); }, {}},
        {"abs_srs", "ABS/SRS", "", [this] { logStub("ABS/SRS"); }, {}},
        {"sas_calib", "SAS", "", [this] { logStub("SAS"); }, {}},
        {"dpf_regen", "DPF", "", [this] { logStub("DPF"); }, {}},
        {"back", "Volver", "", nullptr, {}}
    };

    // Settings submenu
    auto* settings = &root_.children[2];
    settings->children = {
        {"language", "Idioma", "", [this] { logStub("Idioma"); }, {}},
        {"units", "Unidades", "", [this] { logStub("Unidades"); }, {}},
        {"display", "Pantalla", "", [this] { logStub("Pantalla"); }, {}},
        {"back", "Volver", "", nullptr, {}}
    };
}

void Menu::addMenuItem(const std::string& parentId, const MenuItem& item) {
    MenuItem* parent = findMenuItem(parentId);
    if (parent) {
        parent->children.push_back(item);
    }
}

MenuItem* Menu::findMenuItem(const std::string& id) {
    // Simple recursive search
    std::function<MenuItem*(MenuItem*)> search = [&](MenuItem* item) -> MenuItem* {
        if (!item) return nullptr;
        if (item->id == id) return item;
        for (auto& child : item->children) {
            MenuItem* found = search(&child);
            if (found) return found;
        }
        return nullptr;
    };
    return search(&root_);
}

void Menu::showMainMenu() {
    currentPath_.clear();
    currentPath_.push_back(&root_);
    selectedIndex_ = 0;
    renderCurrentMenu();
}

void Menu::showSubMenu(const std::string& parentId) {
    MenuItem* parent = findMenuItem(parentId);
    if (parent && !parent->children.empty()) {
        EventLog::instance().info("Menu: " + parent->label);
        currentPath_.push_back(parent);
        selectedIndex_ = 0;
        renderCurrentMenu();
    }
}

void Menu::handleInput(char key) {
    switch (std::tolower(key)) {
        case 'w':
        case 'k':
            navigateUp();
            break;
        case 's':
        case 'j':
            navigateDown();
            break;
        case '\n':
        case ' ':
            selectCurrent();
            break;
        case 27:  // ESC
            back();
            break;
    }
}

void Menu::navigateUp() {
    if (!currentPath_.empty() && selectedIndex_ > 0) {
        selectedIndex_--;
        renderCurrentMenu();
    }
}

void Menu::navigateDown() {
    if (currentPath_.empty()) return;
    MenuItem* current = currentPath_.back();
    if (selectedIndex_ < static_cast<int>(current->children.size()) - 1) {
        selectedIndex_++;
        renderCurrentMenu();
    }
}

void Menu::selectCurrent() {
    if (currentPath_.empty()) return;

    MenuItem* current = currentPath_.back();
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(current->children.size())) {
        return;
    }

    MenuItem& selected = current->children[selectedIndex_];

    EventLog::instance().debug("Seleccion: " + selected.label);

    if (selected.id == "back") {
        back();
        return;
    }

    if (selected.id == "exit") {
        EventLog::instance().info("Saliendo del menu");
        running_ = false;
        return;
    }

    if (selected.action) {
        selected.action();
    } else if (!selected.children.empty()) {
        showSubMenu(selected.id);
    }
}

void Menu::back() {
    if (currentPath_.size() > 1) {
        currentPath_.pop_back();
        selectedIndex_ = 0;
        renderCurrentMenu();
    }
}

void Menu::renderCurrentMenu() {
    if (currentPath_.empty() || !display_) return;

    MenuItem* current = currentPath_.back();
    std::vector<std::string> items;
    for (const auto& child : current->children) {
        items.push_back(child.label);
    }

    display_->drawMenu(items, selectedIndex_);
}

void Menu::run() {
    running_ = true;
    showMainMenu();
    EventLog::instance().info("Menu iniciado (w/s navegar, Enter seleccionar, ESC volver)");

    while (running_) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        struct timeval tv = {0, 100000};  // 100 ms

        int ret = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(STDIN_FILENO, &fds)) {
            char c = '\0';
            ssize_t n = read(STDIN_FILENO, &c, 1);
            if (n == 1) {
                handleInput(c);
            } else if (n == 0) {
                // stdin en EOF (sin tty): evitar bucle ocupado
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}

// ---------------------------------------------------------------------------
//  Acciones de menú (wiring a OBD2)
// ---------------------------------------------------------------------------

void Menu::autoScan() {
    if (!obd2_) {
        logStub("Auto Scan (sin OBD2)");
        return;
    }
    EventLog::instance().info("Auto Scan: probando comunicacion con la ECU");

    if (display_) display_->print("Auto Scan", "Comprobando ECU...");

    uint8_t raw[8] = {0};
    size_t len = 0;
    // PID 0x00: PIDs soportados -> handshake OBD2
    bool ok = obd2_->requestPIDEx(0x00, raw, sizeof(raw), len);

    if (ok) {
        EventLog::instance().info("Auto Scan: ECU detectada (PIDs soportados = 0x"
                                  + std::to_string(raw[0]) + ")");
        if (display_) {
            display_->print("Auto Scan", "ECU DETECTADA", "", "OK");
        }
    } else {
        EventLog::instance().warn("Auto Scan: sin respuesta de la ECU (mode 01 pid 00)");
        if (display_) {
            display_->print("Auto Scan", "SIN ECU", "", "verifique CAN");
        }
    }
}

void Menu::readCodes() {
    if (!obd2_) {
        logStub("Leer Codigos (sin OBD2)");
        return;
    }
    EventLog::instance().info("Leyendo DTCs (mode 03)");

    if (display_) display_->print("Leer Codigos", "Consultando...");

    std::vector<std::string> dtcs;
    bool ok = obd2_->requestDTCs(dtcs);

    if (!ok) {
        if (display_) display_->print("Leer Codigos", "ERROR de comunicacion");
        return;
    }

    if (dtcs.empty()) {
        EventLog::instance().info("DTCs: sin codigos almacenados");
        if (display_) display_->print("Leer Codigos", "SIN CODIGOS", "DTC", "0");
        return;
    }

    std::string summary = std::to_string(dtcs.size()) + " DTCs: " + dtcs[0];
    if (dtcs.size() > 1) summary += " +" + std::to_string(dtcs.size() - 1);
    EventLog::instance().info("DTCs encontrados: " + summary);

    if (display_) {
        std::string line2 = dtcs[0];
        std::string line3;
        if (dtcs.size() > 1) line3 = dtcs[1];
        display_->print("DTCs: " + std::to_string(dtcs.size()), line2, line3,
                        "Pulse ESC");
    }
}

void Menu::eraseCodes() {
    if (!obd2_) {
        logStub("Borrar Codigos (sin OBD2)");
        return;
    }
    EventLog::instance().info("Borrando DTCs (mode 04)");

    if (display_) display_->print("Borrar Codigos", "Borrando...");

    bool ok = obd2_->clearDTCs();

    if (ok) {
        if (display_) display_->print("Borrar Codigos", "OK", "", "DTCs limpios");
    } else {
        if (display_) display_->print("Borrar Codigos", "ERROR", "", "no respondio");
    }
}

void Menu::showLiveData() {
    if (!obd2_) {
        logStub("Datos en Vivo (sin OBD2)");
        return;
    }
    EventLog::instance().info("Leyendo datos en vivo");

    if (display_) display_->print("Datos en Vivo", "Leyendo...");

    std::unordered_map<std::string, PIDData> data;
    if (!obd2_->requestLiveData(data)) {
        EventLog::instance().warn("Datos en vivo: sin respuesta");
        if (display_) display_->print("Datos en Vivo", "ERROR", "", "sin ECU");
        return;
    }

    char rpm[16], speed[16], tps[16], ect[16];
    snprintf(rpm, sizeof(rpm), "%.0f", data.count("rpm") ? data["rpm"].value : 0.0f);
    snprintf(speed, sizeof(speed), "%.0f", data.count("speed") ? data["speed"].value : 0.0f);
    snprintf(tps, sizeof(tps), "%.0f%%", data.count("throttle") ? data["throttle"].value : 0.0f);
    snprintf(ect, sizeof(ect), "%.0fC", data.count("coolant_temp") ? data["coolant_temp"].value : 0.0f);

    EventLog::instance().info(std::string("Live: RPM=") + rpm + " SPEED=" + speed
                              + " TPS=" + tps + " ECT=" + ect);

    if (display_) {
        display_->print(std::string("RPM ") + rpm + "  SPEED " + speed,
                        std::string("TPS ") + tps + "  ECT " + ect,
                        "", "Pulse ESC");
    }
}

void Menu::runActiveTest() {
    if (!activeTest_) {
        logStub("Pruebas Activas (sin ActiveTest)");
        return;
    }
    EventLog::instance().info("Prueba activa: EVAP valve (placeholders pendientes de mapeo real)");

    if (display_) display_->print("Pruebas Activas", "EVAP", "", "ver docs/TODO.md");

    if (activeTest_->runTest("evap", true)) {
        EventLog::instance().info("EVAP: comando enviado");
    } else {
        EventLog::instance().warn("EVAP: no se pudo enviar el comando");
    }
}

void Menu::showInfo() {
    EventLog::instance().info("Info de la unidad seleccionada");
    if (display_) {
        display_->print("Control Units", "ECU detectada", "", "Pulse ESC");
    }
}

void Menu::logStub(const std::string& feature) {
    EventLog::instance().warn("Funcion no implementada: " + feature);
    if (display_) {
        display_->print(feature, "NO IMPLEMENTADO", "", "Pulse ESC");
    }
}

} // namespace Scanner
