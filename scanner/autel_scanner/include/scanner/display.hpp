#ifndef SCANNER_DISPLAY_HPP
#define SCANNER_DISPLAY_HPP

#include <string>
#include <vector>
#include <memory>
#include "hardware/ssd1306.hpp"

namespace Scanner {

class Display {
public:
    Display(std::shared_ptr<Hardware::SSD1306> screen);
    ~Display();

    bool initialize();
    void cleanup();

    // Screen management
    void clear();
    void show();
    void setBrightness(uint8_t level);
    void setContrast(uint8_t level);

    // Text rendering
    void print(int line, const std::string& text, bool highlight = false);
    void print(const std::string& line1, const std::string& line2 = "",
               const std::string& line3 = "", const std::string& line4 = "");

    // Menu rendering
    void drawMenu(const std::vector<std::string>& items, int selectedIndex);
    void drawMenu(const std::string& title, const std::vector<std::string>& items,
                  const std::vector<std::string>& icons, int selectedIndex);
    void drawProgress(const std::string& title, int percent);
    void drawStatus(const std::string& status, bool error = false);
    void drawData(const std::string& label, const std::string& value,
                  const std::string& unit = "", int line = 0);

    // Special screens
    void drawBootScreen();
    void drawError(const std::string& error);
    void drawWelcome();
    void drawHeader(const std::string& title);

private:
    void drawScrollBar(int totalItems, int selectedIndex);
    std::string truncate(const std::string& text, size_t maxLength);

    std::shared_ptr<Hardware::SSD1306> screen_;
    bool initialized_;
    static constexpr int SCREEN_WIDTH = 128;
    static constexpr int SCREEN_HEIGHT = 32;
    static constexpr int LINE_HEIGHT = 8;
    static constexpr int MAX_LINES = 4;
    static constexpr int CHAR_WIDTH = 6;
};

} // namespace Scanner

#endif // SCANNER_DISPLAY_HPP
