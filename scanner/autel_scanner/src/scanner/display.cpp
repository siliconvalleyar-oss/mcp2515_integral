#include "scanner/display.hpp"
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace Scanner {

Display::Display(std::shared_ptr<Hardware::SSD1306> screen)
    : screen_(std::move(screen)), initialized_(false) {}

Display::~Display() {
    cleanup();
}

bool Display::initialize() {
    if (!screen_) return false;
    if (!screen_->initialize()) return false;
    initialized_ = true;
    return true;
}

void Display::cleanup() {
    if (initialized_) {
        clear();
        show();
        initialized_ = false;
    }
}

void Display::clear() {
    if (screen_) {
        screen_->clear();
    }
}

void Display::show() {
    if (screen_) {
        screen_->display();
    }
}

void Display::setBrightness(uint8_t level) {
    if (screen_) {
        screen_->setBrightness(level);
    }
}

void Display::setContrast(uint8_t level) {
    if (screen_) {
        screen_->setContrast(level);
    }
}

void Display::print(int line, const std::string& text, bool highlight) {
    if (!screen_ || line >= MAX_LINES) return;

    int y = line * LINE_HEIGHT;
    if (highlight) {
        Hardware::SSD1306::Rect rect{0, y, SCREEN_WIDTH, LINE_HEIGHT};
        screen_->fillRect(rect, Hardware::SSD1306::Color::WHITE);
        screen_->drawText(truncate(text, SCREEN_WIDTH / CHAR_WIDTH - 1), 2, y + 1,
                         Hardware::SSD1306::Font::FONT_6x8, Hardware::SSD1306::Color::BLACK);
    } else {
        screen_->drawText(truncate(text, SCREEN_WIDTH / CHAR_WIDTH - 1), 2, y,
                         Hardware::SSD1306::Font::FONT_6x8, Hardware::SSD1306::Color::WHITE);
    }
}

void Display::print(const std::string& line1, const std::string& line2,
                    const std::string& line3, const std::string& line4) {
    clear();
    if (!line1.empty()) print(0, line1);
    if (!line2.empty()) print(1, line2);
    if (!line3.empty()) print(2, line3);
    if (!line4.empty()) print(3, line4);
    show();
}

void Display::drawMenu(const std::vector<std::string>& items, int selectedIndex) {
    clear();
    if (items.empty()) return;

    int totalItems = static_cast<int>(items.size());
    int startIdx = 0;
    if (selectedIndex >= MAX_LINES) {
        startIdx = selectedIndex - MAX_LINES + 1;
    }

    for (int i = 0; i < MAX_LINES && (i + startIdx) < totalItems; ++i) {
        int itemIdx = i + startIdx;
        print(i, items[itemIdx], itemIdx == selectedIndex);
    }

    drawScrollBar(totalItems, selectedIndex);
    show();
}

void Display::drawProgress(const std::string& title, int percent) {
    clear();
    print(0, title);
    print(1, "Procesando...");

    int barWidth = SCREEN_WIDTH - 10;
    int filledWidth = (barWidth * percent) / 100;
    Hardware::SSD1306::Rect barBg{5, 16, barWidth, 8};
    Hardware::SSD1306::Rect barFill{5, 16, filledWidth, 8};
    screen_->drawRect(barBg, Hardware::SSD1306::Color::WHITE);
    screen_->fillRect(barFill, Hardware::SSD1306::Color::WHITE);

    char percentStr[8];
    snprintf(percentStr, sizeof(percentStr), "%d%%", percent);
    print(3, percentStr);
    show();
}

void Display::drawStatus(const std::string& status, bool error) {
    Hardware::SSD1306::Rect statusBar{0, SCREEN_HEIGHT - 8, SCREEN_WIDTH, 8};
    screen_->fillRect(statusBar, error ? Hardware::SSD1306::Color::BLACK : Hardware::SSD1306::Color::WHITE);
    screen_->drawText(truncate(status, SCREEN_WIDTH / CHAR_WIDTH - 1), 2, SCREEN_HEIGHT - 8,
                     Hardware::SSD1306::Font::FONT_6x8,
                     error ? Hardware::SSD1306::Color::WHITE : Hardware::SSD1306::Color::BLACK);
    show();
}

void Display::drawData(const std::string& label, const std::string& value,
                       const std::string& unit, int line) {
    if (line >= MAX_LINES) return;

    std::string text = label + ": " + value + " " + unit;
    print(line, truncate(text, SCREEN_WIDTH / CHAR_WIDTH - 1));
}

void Display::drawBootScreen() {
    clear();
    print(0, "AUTEL Scanner");
    print(1, "Raspberry Pi");
    print(2, "Version 1.0.0");
    print(3, "Iniciando...");
    show();
}

void Display::drawError(const std::string& error) {
    clear();
    print(0, "ERROR", true);
    print(1, truncate(error, SCREEN_WIDTH / CHAR_WIDTH - 1));
    print(2, "");
    print(3, "Presione ESC");
    show();
}

void Display::drawWelcome() {
    clear();
    print(0, "AUTEL MaxiSYS");
    print(1, "Raspberry Pi");
    print(2, "MS906 Pro");
    print(3, "v1.0.0");
    show();
}

void Display::drawScrollBar(int totalItems, int selectedIndex) {
    if (totalItems <= MAX_LINES) return;

    int scrollHeight = (MAX_LINES * SCREEN_HEIGHT) / totalItems;
    int scrollPos = (selectedIndex * SCREEN_HEIGHT) / totalItems;

    Hardware::SSD1306::Rect scrollBar{SCREEN_WIDTH - 3, 0, 3, SCREEN_HEIGHT};
    screen_->fillRect(scrollBar, Hardware::SSD1306::Color::BLACK);
    Hardware::SSD1306::Rect scrollThumb{SCREEN_WIDTH - 3, scrollPos, 3, scrollHeight};
    screen_->fillRect(scrollThumb, Hardware::SSD1306::Color::WHITE);
}

std::string Display::truncate(const std::string& text, size_t maxLength) {
    if (text.length() <= maxLength) {
        return text;
    }
    return text.substr(0, maxLength - 3) + "...";
}

} // namespace Scanner
