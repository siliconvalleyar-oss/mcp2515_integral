#include "scanner/menu.hpp"
#include "scanner/event_log.hpp"
#include "scanner/config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>
#include <thread>
#include <unistd.h>
#include <sys/select.h>

namespace Scanner {

Menu::Menu(std::shared_ptr<Display> display)
    : display_(std::move(display)), running_(false), selectedIndex_(0) {}

Menu::~Menu() {
    cleanup();
}

bool Menu::initialize() {
    buildMenuTree();
    updateSettingsLabels();
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

// ---------------------------------------------------------------------------
//  Arbol de menu estilo AUTEL MaxiCOM
// ---------------------------------------------------------------------------

void Menu::buildMenuTree() {
    // Menu principal
    root_.children = {
        {"vehicle", "Vehículo", "V", [this] { vehicleSelect(); }, {}},
        {"diagnostics", "Diagnóstico", "D", nullptr, {}},
        {"live_data", "Datos en Vivo", "L", nullptr, {}},
        {"active_test", "Pruebas Activas", "A", [this] { runActiveTest(); }, {}},
        {"service", "Servicio", "S", nullptr, {}},
        {"data_manager", "Data Manager", "M", nullptr, {}},
        {"settings", "Ajustes", "C", nullptr, {}},
        {"exit", "Salir", "X", nullptr, {}}
    };

    // Diagnostico
    auto* diag = &root_.children[1];
    diag->children = {
        {"auto_scan", "Auto Scan", "R", [this] { autoScan(); }, {}},
        {"read_codes", "Leer Códigos", "C", [this] { readCodes(); }, {}},
        {"erase_codes", "Borrar Códigos", "E", [this] { eraseCodes(); }, {}},
        {"freeze_frame", "Freeze Frame", "F", [this] { showFreezeFrame(); }, {}},
        {"control_units", "Info ECU", "I", [this] { showInfo(); }, {}},
        {"back", "Volver", "", nullptr, {}}
    };

    // Datos en Vivo
    auto* liveData = &root_.children[2];
    liveData->children = {
        {"custom_list", "Lista Personalizada", "L", [this] { logStub("Lista Personalizada"); }, {}},
        {"all_params", "Todos los Parámetros", "A", [this] { showLiveData(); }, {}},
        {"back", "Volver", "", nullptr, {}}
    };

    // Servicio / Mantenimiento
    auto* service = &root_.children[4];
    service->children = {
        {"oil_reset", "Reset Aceite", "O", [this] { logStub("Reset Aceite"); }, {}},
        {"tpms_service", "TPMS", "T", [this] { logStub("TPMS"); }, {}},
        {"epb_service", "EPB", "P", [this] { logStub("EPB"); }, {}},
        {"abs_srs", "ABS/SRS", "B", [this] { logStub("ABS/SRS"); }, {}},
        {"sas_calib", "SAS", "S", [this] { logStub("SAS"); }, {}},
        {"dpf_regen", "DPF", "D", [this] { logStub("DPF"); }, {}},
        {"back", "Volver", "", nullptr, {}}
    };

    // Data Manager
    auto* dataMgr = &root_.children[5];
    dataMgr->children = {
        {"capture_frame", "Capturar Freeze Frame", "F", [this] { captureFreezeFrame(); }, {}},
        {"view_snapshots", "Ver Snapshots", "V", [this] { showSnapshots(); }, {}},
        {"clear_snapshots", "Borrar Snapshots", "B", [this] { clearSnapshots(); }, {}},
        {"back", "Volver", "", nullptr, {}}
    };

    // Ajustes
    auto* settings = &root_.children[6];
    settings->children = {
        {"language", "", "", [this] { toggleLanguage(); }, {}},
        {"units", "", "", [this] { cycleUnits(); }, {}},
        {"brightness", "", "", [this] { cycleBrightness(); }, {}},
        {"beep", "", "", [this] { toggleBeep(); }, {}},
        {"auto_scan_boot", "", "", [this] { toggleAutoScan(); }, {}},
        {"back", "Volver", "", nullptr, {}}
    };

    updateSettingsLabels();
}

void Menu::updateSettingsLabels() {
    auto* settings = findMenuItem("settings");
    if (!settings) return;
    auto& cfg = Config::instance();

    auto setLabel = [&](const std::string& id, const std::string& label) {
        for (auto& c : settings->children) {
            if (c.id == id) {
                c.label = label;
                c.icon = "";
                return;
            }
        }
    };

    std::string lang = (cfg.language() == "es") ? "Español" : "English";
    setLabel("language", "Idioma: " + lang);
    setLabel("units", cfg.metricUnits() ? "Unidades: Métrico" : "Unidades: Imperial");
    setLabel("brightness", "Brillo: " + std::to_string(cfg.brightness()));
    setLabel("beep", cfg.beepEnabled() ? "Bip: On" : "Bip: Off");
    setLabel("auto_scan_boot", cfg.autoScanOnBoot() ? "Auto Scan: On" : "Auto Scan: Off");
}

void Menu::saveAndReload() {
    Config::instance().save();
    updateSettingsLabels();
    renderCurrentMenu();
}

void Menu::addMenuItem(const std::string& parentId, const MenuItem& item) {
    MenuItem* parent = findMenuItem(parentId);
    if (parent) {
        parent->children.push_back(item);
    }
}

MenuItem* Menu::findMenuItem(const std::string& id) {
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

Menu::Key Menu::readKey() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = {0, 100000};  // 100 ms
    int ret = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
    if (ret <= 0 || !FD_ISSET(STDIN_FILENO, &fds)) return Key::UNKNOWN;

    char c = '\0';
    if (read(STDIN_FILENO, &c, 1) != 1) return Key::UNKNOWN;

    switch (c) {
        case 'w': case 'k': case 'W': case 'K':
            return Key::UP;
        case 's': case 'j': case 'S': case 'J':
            return Key::DOWN;
        case '\r': case '\n': case ' ':
            return Key::SELECT;
        case '\x1b': {
            // Secuencia de escape: flechas llegan como ESC [ A / B / C / D
            // (o ESC O A en modo aplicacion). Esperar los bytes siguientes.
            char b1 = '\0';
            struct timeval tv2 = {0, 50000};  // 50 ms
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            ret = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv2);
            if (ret > 0 && FD_ISSET(STDIN_FILENO, &fds) && read(STDIN_FILENO, &b1, 1) == 1) {
                if (b1 == '[' || b1 == 'O') {
                    char b2 = '\0';
                    FD_ZERO(&fds);
                    FD_SET(STDIN_FILENO, &fds);
                    ret = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv2);
                    if (ret > 0 && FD_ISSET(STDIN_FILENO, &fds)
                        && read(STDIN_FILENO, &b2, 1) == 1) {
                        switch (b2) {
                            case 'A': return Key::UP;
                            case 'B': return Key::DOWN;
                            case 'C': return Key::SELECT;
                            case 'D': return Key::BACK;
                        }
                    }
                }
            }
            return Key::BACK;  // ESC suelto
        }
    }
    return Key::UNKNOWN;
}

void Menu::handleKey(Key key) {
    switch (key) {
        case Key::UP:
            navigateUp();
            break;
        case Key::DOWN:
            navigateDown();
            break;
        case Key::SELECT:
            selectCurrent();
            break;
        case Key::BACK:
            back();
            break;
        case Key::UNKNOWN:
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

std::string Menu::mainTitle() const {
    auto& cfg = Config::instance();
    if (!cfg.vehicle().empty()) {
        return cfg.vehicle().make + " " + cfg.vehicle().model;
    }
    return "AUTEL Scanner";
}

void Menu::renderCurrentMenu() {
    if (currentPath_.empty()) return;
    renderConsoleMenu();
    renderMonitor();
}

void Menu::renderConsoleMenu() {
    MenuItem* current = currentPath_.back();
    std::vector<std::string> items;
    for (const auto& child : current->children) {
        items.push_back(child.label);
    }
    renderConsoleList((current == &root_) ? mainTitle() : current->label, items, selectedIndex_);
}

void Menu::renderConsoleList(const std::string& title,
                             const std::vector<std::string>& options,
                             int selected) {
    const bool tty = isatty(STDOUT_FILENO) != 0;
    std::string out;
    if (tty) {
        out += "\x1b[2J\x1b[H";  // limpiar pantalla + home
    }

    out += "=== " + title + " ===\n";
    for (size_t i = 0; i < options.size(); ++i) {
        std::string prefix = (static_cast<int>(i) == selected) ? "> " : "  ";
        if (tty && static_cast<int>(i) == selected) {
            out += "\x1b[1;36m" + prefix + options[i] + "\x1b[0m\n";
        } else {
            out += prefix + options[i] + "\n";
        }
    }

    if (tty) {
        out += "\x1b[2m[w/s] navegar  [Enter] seleccionar  [ESC] volver/salir\x1b[0m\n";
    }
    fputs(out.c_str(), stdout);
    fflush(stdout);
}

void Menu::renderMonitor() {
    if (!display_ || currentPath_.empty()) return;

    MenuItem* current = currentPath_.back();
    std::string title = (current == &root_) ? mainTitle() : current->label;
    std::string item;
    if (selectedIndex_ >= 0 && static_cast<size_t>(selectedIndex_) < current->children.size()) {
        const auto& sel = current->children[selectedIndex_];
        item = sel.icon.empty() ? sel.label : sel.icon + " " + sel.label;
    }

    char status[32];
    snprintf(status, sizeof(status), "%d/%d", selectedIndex_ + 1,
             static_cast<int>(current->children.size()));
    display_->drawMonitor(title, item, status);
}

void Menu::run() {
    running_ = true;

    auto& cfg = Config::instance();
    if (!cfg.vehicle().empty()) {
        showMainMenu();
    } else {
        EventLog::instance().info("Sin vehiculo seleccionado: iniciando asistente");
        showMainMenu();
        vehicleSelect();
    }

    // Auto Scan al inicio si esta configurado
    if (cfg.autoScanOnBoot() && obd2_) {
        EventLog::instance().info("Auto Scan al inicio activado");
        autoScan();
        renderCurrentMenu();
    }

    EventLog::instance().info("Menu iniciado (w/s navegar, Enter seleccionar, ESC volver)");

    while (running_) {
        Key key = readKey();
        if (key == Key::UNKNOWN) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        handleKey(key);
    }
}

// ---------------------------------------------------------------------------
//  Asistente de seleccion de vehiculo
// ---------------------------------------------------------------------------

bool Menu::pickFromList(const std::string& title, const std::vector<std::string>& options,
                        int& selected) {
    int idx = (selected >= 0 && selected < static_cast<int>(options.size())) ? selected : 0;

    while (true) {
        renderConsoleList(title, options, idx);

        if (display_) {
            char status[32];
            snprintf(status, sizeof(status), "%d/%d", idx + 1,
                     static_cast<int>(options.size()));
            std::string item = (idx >= 0 && static_cast<size_t>(idx) < options.size())
                                   ? options[idx] : "";
            display_->drawMonitor(title, item, status);
        }

        Key key = readKey();
        switch (key) {
            case Key::UP:
                if (idx > 0) idx--;
                break;
            case Key::DOWN:
                if (idx < static_cast<int>(options.size()) - 1) idx++;
                break;
            case Key::SELECT:
                selected = idx;
                return true;
            case Key::BACK:
                return false;
            case Key::UNKNOWN:
                break;
        }
    }
}

void Menu::vehicleSelect() {
    EventLog::instance().info("Asistente de seleccion de vehiculo");

    std::string dbPath;
    {
        std::string cfgDir = Config::instance().configPath();
        size_t slash = cfgDir.find_last_of('/');
        dbPath = (slash == std::string::npos) ? "config/vehicles.json"
                                              : cfgDir.substr(0, slash) + "/vehicles.json";
    }

    if (!vehicleDB_.load(dbPath)) {
        if (display_) {
            display_->print("Vehículo", "Base de datos", "no encontrada", "config/vehicles.json");
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return;
    }

    const auto& makes = vehicleDB_.makes();

    // Paso 1: marca
    std::vector<std::string> makeNames;
    for (const auto& m : makes) makeNames.push_back(m.make);
    if (!pickFromList("Marca", makeNames, makeSel_)) return;
    const VehicleMake& make = makes[makeSel_];

    // Paso 2: modelo
    std::vector<std::string> modelNames;
    for (const auto& m : make.models) modelNames.push_back(m.model);
    modelSel_ = 0;
    if (!pickFromList(make.make, modelNames, modelSel_)) return;
    const VehicleModel& model = make.models[modelSel_];

    // Paso 3: anio
    int yearChoice = 0;
    if (!model.years.empty() && !pickFromList(model.model, model.years, yearChoice)) return;
    yearSel_ = yearChoice;

    // Paso 4: ECU
    std::vector<std::string> ecuNames;
    for (const auto& e : model.ecus) ecuNames.push_back(e.name);
    ecuSel_ = 0;
    if (!model.ecus.empty() && !pickFromList(model.model, ecuNames, ecuSel_)) return;
    const EcuOption& ecu = model.ecus.empty() ? EcuOption{"ECM/PCM", "7E0"} : model.ecus[ecuSel_];

    VehicleInfo v;
    v.make = make.make;
    v.model = model.model;
    v.year = model.years.empty() ? "" : model.years[yearSel_];
    v.ecu = ecu.name;
    v.diagAddr = ecu.diagAddr;
    Config::instance().setVehicle(v);
    Config::instance().save();

    EventLog::instance().info("Vehiculo seleccionado: " + v.make + " " + v.model + " (" + v.ecu + ")");
    renderCurrentMenu();
}

// ---------------------------------------------------------------------------
//  Acciones de menu (wiring a OBD2)
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

    if (display_) display_->print("Leer Códigos", "Consultando...");

    std::vector<std::string> dtcs;
    bool ok = obd2_->requestDTCs(dtcs);

    if (!ok) {
        if (display_) display_->print("Leer Códigos", "ERROR de comunicacion");
        return;
    }

    if (dtcs.empty()) {
        EventLog::instance().info("DTCs: sin codigos almacenados");
        if (display_) display_->print("Leer Códigos", "SIN CODIGOS", "DTC", "0");
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

    if (display_) display_->print("Borrar Códigos", "Borrando...");

    bool ok = obd2_->clearDTCs();

    if (ok) {
        if (display_) display_->print("Borrar Códigos", "OK", "", "DTCs limpios");
    } else {
        if (display_) display_->print("Borrar Códigos", "ERROR", "", "no respondio");
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

    auto& cfg = Config::instance();
    VehicleInfo v = cfg.vehicle();

    if (display_) {
        if (!v.empty()) {
            display_->print(v.make + " " + v.model,
                            "ECU: " + v.ecu,
                            "Dir: " + v.diagAddr,
                            "Pulse ESC");
        } else {
            display_->print("Control Units", "ECU detectada", "", "Pulse ESC");
        }
    }
}

void Menu::showFreezeFrame() {
    if (!obd2_) {
        logStub("Freeze Frame (sin OBD2)");
        return;
    }
    EventLog::instance().info("Leyendo freeze frame (mode 02)");

    if (display_) display_->print("Freeze Frame", "Consultando...");

    std::unordered_map<std::string, PIDData> data;
    if (!obd2_->requestFreezeFrame(0, data)) {
        EventLog::instance().warn("Freeze frame: sin respuesta");
        if (display_) display_->print("Freeze Frame", "SIN DATOS", "", "sin ECU");
        return;
    }

    for (const auto& kv : data) {
        EventLog::instance().info("FF " + kv.first + "=" + std::to_string(kv.second.value));
    }

    if (display_) {
        display_->print("Freeze Frame", "Captura OK", "", "use Data Manager");
    }
}

void Menu::captureFreezeFrame() {
    if (!obd2_) {
        logStub("Capturar Freeze Frame (sin OBD2)");
        return;
    }

    if (display_) display_->print("Data Manager", "Capturando...");

    std::unordered_map<std::string, PIDData> data;
    if (!obd2_->requestFreezeFrame(0, data)) {
        EventLog::instance().warn("Freeze frame: sin respuesta");
        if (display_) display_->print("Data Manager", "SIN DATOS", "", "sin ECU");
        return;
    }

    Snapshot snap;
    char buf[32];
    time_t now = time(nullptr);
    struct tm tm{};
    localtime_r(&now, &tm);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    snap.timestamp = buf;

    auto& cfg = Config::instance();
    VehicleInfo v = cfg.vehicle();
    snap.label = v.empty() ? "Freeze Frame" : v.make + " " + v.model;

    nlohmann::json j = nlohmann::json::object();
    for (const auto& kv : data) {
        j[kv.first] = kv.second.value;
    }
    snap.data = j;

    cfg.addSnapshot(snap);

    EventLog::instance().info("Snapshot guardado: " + snap.label + " (" + snap.timestamp + ")");
    if (display_) display_->print("Data Manager", "SNAPSHOT OK", snap.label, snap.timestamp);
}

void Menu::showSnapshots() {
    auto& cfg = Config::instance();
    const auto& snaps = cfg.snapshots();

    if (snaps.empty()) {
        if (display_) display_->print("Snapshots", "VACIOS", "", "use Capturar FF");
        return;
    }

    std::vector<std::string> labels;
    for (const auto& s : snaps) {
        labels.push_back(s.label + " " + s.timestamp.substr(11));
    }

    int idx = 0;
    if (!pickFromList("Snapshots (" + std::to_string(labels.size()) + ")", labels, idx)) return;

    const Snapshot& sel = snaps[idx];
    if (display_) {
        std::string line1 = sel.label;
        std::string line2, line3;
        int count = 0;
        for (auto it = sel.data.begin(); it != sel.data.end() && count < 2; ++it, ++count) {
            std::string v = it.value().is_number() ?
                std::to_string(it.value().get<double>()) : "n/a";
            if (count == 0) line2 = it.key() + "=" + v;
            else line3 = it.key() + "=" + v;
        }
        display_->print(line1, line2, line3, "Pulse ESC");
    }
}

void Menu::clearSnapshots() {
    auto& cfg = Config::instance();
    cfg.clearSnapshots();
    EventLog::instance().info("Snapshots borrados");
    if (display_) display_->print("Data Manager", "Snapshots", "BORRADOS", "");
}

void Menu::logStub(const std::string& feature) {
    EventLog::instance().warn("Funcion no implementada: " + feature);
    if (display_) {
        display_->print(feature, "NO IMPLEMENTADO", "", "Pulse ESC");
    }
}

// ---------------------------------------------------------------------------
//  Ajustes (persistidos en config.json)
// ---------------------------------------------------------------------------

void Menu::toggleLanguage() {
    auto& cfg = Config::instance();
    cfg.setLanguage(cfg.language() == "es" ? "en" : "es");
    saveAndReload();
    EventLog::instance().info("Idioma: " + cfg.language());
}

void Menu::cycleUnits() {
    auto& cfg = Config::instance();
    cfg.setUnits(cfg.metricUnits() ? "imperial" : "metric");
    saveAndReload();
    EventLog::instance().info("Unidades: " + cfg.units());
}

void Menu::cycleBrightness() {
    auto& cfg = Config::instance();
    uint8_t b = cfg.brightness();
    b = (b >= 255) ? 32 : b + 32;
    cfg.setBrightness(b);
    if (display_) display_->setBrightness(b);
    saveAndReload();
    EventLog::instance().info("Brillo: " + std::to_string(b));
}

void Menu::toggleBeep() {
    auto& cfg = Config::instance();
    cfg.setBeepEnabled(!cfg.beepEnabled());
    saveAndReload();
    EventLog::instance().info("Bip: " + std::string(cfg.beepEnabled() ? "on" : "off"));
}

void Menu::toggleAutoScan() {
    auto& cfg = Config::instance();
    cfg.setAutoScanOnBoot(!cfg.autoScanOnBoot());
    saveAndReload();
    EventLog::instance().info("Auto Scan al inicio: "
                              + std::string(cfg.autoScanOnBoot() ? "on" : "off"));
}

} // namespace Scanner
