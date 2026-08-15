#include "scanner/menu.hpp"
#include "scanner/obd2.hpp"
#include "scanner/dtc.hpp"
#include "scanner/live_data.hpp"
#include "scanner/active_test.hpp"
#include <algorithm>
#include <cctype>
#include <thread>
#include <functional>

namespace Scanner {

Menu::Menu(std::shared_ptr<Display> display)
    : display_(std::move(display)), running_(false), selectedIndex_(0) {}

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
        {"auto_scan", "Auto Scan", "", nullptr, {}},
        {"control_units", "Control Units", "", nullptr, {}},
        {"read_codes", "Leer Codigos", "", nullptr, {}},
        {"erase_codes", "Borrar Codigos", "", nullptr, {}},
        {"live_data", "Datos en Vivo", "", nullptr, {}},
        {"active_test", "Pruebas Activas", "", nullptr, {}},
        {"special_functions", "Funciones Especiales", "", nullptr, {}},
        {"back", "Volver", "", nullptr, {}}
    };

    // Live Data submenu
    auto* liveData = &diag->children[4];
    liveData->children = {
        {"custom_list", "Lista Personalizada", "", nullptr, {}},
        {"all_params", "Todos los Parametros", "", nullptr, {}},
        {"back", "Volver", "", nullptr, {}}
    };

    // Service submenu
    auto* service = &root_.children[3];
    service->children = {
        {"oil_reset", "Reset Aceite", "", nullptr, {}},
        {"tpms_service", "TPMS", "", nullptr, {}},
        {"epb_service", "EPB", "", nullptr, {}},
        {"abs_srs", "ABS/SRS", "", nullptr, {}},
        {"sas_calib", "SAS", "", nullptr, {}},
        {"dpf_regen", "DPF", "", nullptr, {}},
        {"back", "Volver", "", nullptr, {}}
    };

    // Settings submenu
    auto* settings = &root_.children[2];
    settings->children = {
        {"language", "Idioma", "", nullptr, {}},
        {"units", "Unidades", "", nullptr, {}},
        {"display", "Pantalla", "", nullptr, {}},
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

    if (selected.id == "back") {
        back();
        return;
    }

    if (selected.id == "exit") {
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

    while (running_) {
        // In a real implementation, this would read from keyboard/UART
        // For now, we'll just loop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace Scanner
