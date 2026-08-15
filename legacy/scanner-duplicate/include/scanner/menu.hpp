#ifndef SCANNER_MENU_HPP
#define SCANNER_MENU_HPP

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "scanner/display.hpp"

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
    void handleInput(char key);
    void run();

    // Menu building
    void buildMenuTree();
    void addMenuItem(const std::string& parentId, const MenuItem& item);

private:
    std::shared_ptr<Display> display_;
    bool running_;
    MenuItem root_;
    std::vector<MenuItem*> currentPath_;
    int selectedIndex_;

    void navigateUp();
    void navigateDown();
    void selectCurrent();
    void back();
    void renderCurrentMenu();
    MenuItem* findMenuItem(const std::string& id);
};

} // namespace Scanner

#endif // SCANNER_MENU_HPP
