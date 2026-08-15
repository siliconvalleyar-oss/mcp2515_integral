#ifndef SCANNER_MENU_HPP
#define SCANNER_MENU_HPP

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "scanner/display.hpp"
#include "scanner/obd2.hpp"
#include "scanner/dtc.hpp"
#include "scanner/live_data.hpp"
#include "scanner/active_test.hpp"
#include "scanner/config.hpp"

namespace Scanner {

struct MenuItem {
    std::string id;
    std::string label;
    std::string icon;
    std::function<void()> action;
    std::vector<MenuItem> children;
};

class Menu {
public:
    Menu(std::shared_ptr<Display> display);
    ~Menu();

    bool initialize();
    void cleanup();

    // Navigation
    void showMainMenu();
    void showSubMenu(const std::string& parentId);
    void run();

    // Menu building
    void buildMenuTree();
    void addMenuItem(const std::string& parentId, const MenuItem& item);

    // Dependencies
    void setDependencies(std::shared_ptr<OBD2> obd2,
                         std::shared_ptr<DTCManager> dtcManager,
                         std::shared_ptr<LiveData> liveData,
                         std::shared_ptr<ActiveTest> activeTest);

private:
    enum class Key { UP, DOWN, SELECT, BACK, UNKNOWN };

    std::shared_ptr<Display> display_;
    std::shared_ptr<OBD2> obd2_;
    std::shared_ptr<DTCManager> dtcManager_;
    std::shared_ptr<LiveData> liveData_;
    std::shared_ptr<ActiveTest> activeTest_;
    bool running_;
    MenuItem root_;
    std::vector<MenuItem*> currentPath_;
    int selectedIndex_;

    VehicleDB vehicleDB_;
    int makeSel_ = -1;
    int modelSel_ = -1;
    int yearSel_ = -1;
    int ecuSel_ = -1;

    Key readKey();
    void handleKey(Key key);
    void navigateUp();
    void navigateDown();
    void selectCurrent();
    void back();
    void renderCurrentMenu();
    void renderConsoleMenu();
    void renderConsoleList(const std::string& title, const std::vector<std::string>& options,
                           int selected);
    void renderMonitor();
    MenuItem* findMenuItem(const std::string& id);
    std::string mainTitle() const;
    void updateSettingsLabels();
    void saveAndReload();

    // Seleccion de vehiculo (asistente marca/modelo/año/ECU)
    bool pickFromList(const std::string& title, const std::vector<std::string>& options,
                      int& selected);
    void vehicleSelect();

    // Acciones de menu (wiring a OBD2)
    void autoScan();
    void readCodes();
    void eraseCodes();
    void showLiveData();
    void runActiveTest();
    void showInfo();
    void showFreezeFrame();
    void captureFreezeFrame();
    void showSnapshots();
    void clearSnapshots();
    void logStub(const std::string& feature);

    // Ajustes
    void toggleLanguage();
    void cycleUnits();
    void cycleBrightness();
    void toggleBeep();
    void toggleAutoScan();
};

} // namespace Scanner

#endif // SCANNER_MENU_HPP
