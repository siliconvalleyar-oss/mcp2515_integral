#include "hardware/ssd1306.hpp"
#include "hardware/i2c.hpp"
#include <cstring>
#include <cstdio>

namespace Hardware {

// SSD1306 Command constants
namespace SSD1306_CMD {
    constexpr uint8_t SET_CONTRAST = 0x81;
    constexpr uint8_t DISPLAY_ALL_ON = 0xA4;
    constexpr uint8_t DISPLAY_ALL_ON_RESUME = 0xA4;
    constexpr uint8_t DISPLAY_ALL_ON_FOLLOW_RAM = 0xA5;
    constexpr uint8_t NORMAL_DISPLAY = 0xA6;
    constexpr uint8_t INVERT_DISPLAY = 0xA7;
    constexpr uint8_t DISPLAY_DIM = 0xAE;
    constexpr uint8_t DISPLAY_ON = 0xAF;
    constexpr uint8_t SET_DISPLAY_OFFSET = 0xD3;
    constexpr uint8_t SET_COM_PINS = 0xDA;
    constexpr uint8_t SET_VCOM_DETECT = 0xDB;
    constexpr uint8_t SET_DISPLAY_CLOCK_DIV = 0xD5;
    constexpr uint8_t SET_PRECHARGE = 0xD9;
    constexpr uint8_t SET_MULTIPLEX = 0xA8;
    constexpr uint8_t SET_LOW_COLUMN = 0x00;
    constexpr uint8_t SET_HIGH_COLUMN = 0x10;
    constexpr uint8_t SET_START_LINE = 0x40;
    constexpr uint8_t MEMORY_MODE = 0x20;
    constexpr uint8_t COLUMN_ADDR = 0x21;
    constexpr uint8_t PAGE_ADDR = 0x22;
    constexpr uint8_t COM_SCAN_INC = 0xC0;
    constexpr uint8_t COM_SCAN_DEC = 0xC8;
    constexpr uint8_t SEG_REMAP = 0xA0;
    constexpr uint8_t CHARGE_PUMP = 0x8D;
    constexpr uint8_t EXTERNAL_VCC = 0x01;
    constexpr uint8_t SWITCH_CAP_VCC = 0x02;
    constexpr uint8_t ACTIVATE_SCROLL = 0x2F;
    constexpr uint8_t DEACTIVATE_SCROLL = 0x2E;
    constexpr uint8_t SET_VERTICAL_SCROLL_AREA = 0xA3;
}

SSD1306::SSD1306(uint8_t address, int width, int height)
    : address_(address), width_(width), height_(height), initialized_(false) {
    buffer_.resize((width * height) / 8);
}

SSD1306::~SSD1306() {
    cleanup();
}

bool SSD1306::initialize() {
    i2c_ = std::make_unique<I2C>(1, address_);
    if (!i2c_->initialize()) {
        return false;
    }

    // Initialization sequence
    uint8_t initCmds[] = {
        SSD1306_CMD::DISPLAY_DIM,
        SSD1306_CMD::SET_DISPLAY_CLOCK_DIV, 0x80,
        SSD1306_CMD::SET_MULTIPLEX, static_cast<uint8_t>(height_ - 1),
        SSD1306_CMD::SET_DISPLAY_OFFSET, 0x00,
        SSD1306_CMD::SET_START_LINE | 0x00,
        SSD1306_CMD::CHARGE_PUMP, 0x14,
        SSD1306_CMD::MEMORY_MODE, 0x00,
        SSD1306_CMD::SEG_REMAP | 0x01,
        SSD1306_CMD::COM_SCAN_DEC,
        SSD1306_CMD::SET_COM_PINS, 0x02,
        SSD1306_CMD::SET_CONTRAST, 0x8F,
        SSD1306_CMD::SET_PRECHARGE, 0xF1,
        SSD1306_CMD::SET_VCOM_DETECT, 0x40,
        SSD1306_CMD::DISPLAY_ALL_ON_RESUME,
        SSD1306_CMD::NORMAL_DISPLAY,
        SSD1306_CMD::DISPLAY_ON
    };

    for (uint8_t cmd : initCmds) {
        if (!writeCommand(cmd)) {
            return false;
        }
    }

    clear();
    display();
    initialized_ = true;
    return true;
}

void SSD1306::cleanup() {
    if (initialized_) {
        writeCommand(SSD1306_CMD::DISPLAY_DIM);
        i2c_->cleanup();
        initialized_ = false;
    }
}

bool SSD1306::writeCommand(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};  // 0x00 = command mode
    return i2c_->write(buf, 2);
}

bool SSD1306::writeData(const uint8_t* data, size_t length) {
    uint8_t buf[2] = {0x40, 0x00};  // 0x40 = data mode
    bool result = true;
    for (size_t i = 0; i < length; ++i) {
        buf[1] = data[i];
        if (!i2c_->write(buf, 2)) {
            result = false;
            break;
        }
    }
    return result;
}

void SSD1306::clear() {
    memset(buffer_.data(), 0, buffer_.size());
}

void SSD1306::display() {
    setPageAddress(0, (height_ / 8) - 1);
    setColumnAddress(0, width_ - 1);
    writeData(buffer_.data(), buffer_.size());
}

void SSD1306::setBrightness(uint8_t brightness) {
    writeCommand(SSD1306_CMD::SET_CONTRAST);
    writeCommand(brightness);
}

void SSD1306::setContrast(uint8_t contrast) {
    writeCommand(SSD1306_CMD::SET_CONTRAST);
    writeCommand(contrast);
}

void SSD1306::invert(bool invert) {
    writeCommand(invert ? SSD1306_CMD::INVERT_DISPLAY : SSD1306_CMD::NORMAL_DISPLAY);
}

void SSD1306::drawPixel(int x, int y, Color color) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;

    size_t index = x + (y / 8) * width_;
    uint8_t mask = 1 << (y % 8);

    switch (color) {
        case Color::WHITE:
            buffer_[index] |= mask;
            break;
        case Color::BLACK:
            buffer_[index] &= ~mask;
            break;
        case Color::INVERSE:
            buffer_[index] ^= mask;
            break;
    }
}

void SSD1306::drawLine(int x0, int y0, int x1, int y1, Color color) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void SSD1306::drawRect(const Rect& rect, Color color) {
    drawLine(rect.x, rect.y, rect.x + rect.width - 1, rect.y, color);
    drawLine(rect.x, rect.y + rect.height - 1, rect.x + rect.width - 1, rect.y + rect.height - 1, color);
    drawLine(rect.x, rect.y, rect.x, rect.y + rect.height - 1, color);
    drawLine(rect.x + rect.width - 1, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1, color);
}

void SSD1306::fillRect(const Rect& rect, Color color) {
    for (int y = rect.y; y < rect.y + rect.height; ++y) {
        for (int x = rect.x; x < rect.x + rect.width; ++x) {
            drawPixel(x, y, color);
        }
    }
}

void SSD1306::drawCircle(int centerX, int centerY, int radius, Color color) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        drawPixel(centerX + x, centerY + y, color);
        drawPixel(centerX + y, centerX + x, color);
        drawPixel(centerX - y, centerX + x, color);
        drawPixel(centerX - x, centerX + y, color);
        drawPixel(centerX - x, centerY - y, color);
        drawPixel(centerX - y, centerY - x, color);
        drawPixel(centerX + y, centerY - x, color);
        drawPixel(centerX + x, centerY - y, color);

        y += 1;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void SSD1306::drawChar(char c, int x, int y, [[maybe_unused]] Font font, Color color) {
    // Simple 6x8 font implementation
    static const uint8_t font6x8[][6] = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // space
        {0x00, 0x00, 0x5F, 0x00, 0x00, 0x00}, // !
        // ... complete font table would go here
    };

    if (c < 32 || c > 126) c = ' ';
    int index = c - 32;

    for (int i = 0; i < 6; ++i) {
        uint8_t line = font6x8[index][i];
        for (int j = 0; j < 8; ++j) {
            if (line & (1 << j)) {
                drawPixel(x + i, y + j, color);
            }
        }
    }
}

void SSD1306::drawText(const std::string& text, int x, int y, Font font, Color color) {
    int cursorX = x;
    for (char c : text) {
        if (c == '\n') {
            cursorX = x;
            y += 8;
        } else {
            drawChar(c, cursorX, y, font, color);
            cursorX += 6;
        }
    }
}

void SSD1306::startScrollLeft([[maybe_unused]] uint8_t startPage, [[maybe_unused]] uint8_t endPage) {
    writeCommand(SSD1306_CMD::DEACTIVATE_SCROLL);
    writeCommand(0x2A);  // Horizontal scroll
    writeCommand(0x00);  // Dummy byte
    writeCommand(0x00);  // Start page
    writeCommand(0x00);  // Time interval
    writeCommand(0x07);  // End page
    writeCommand(0x00);  // Dummy byte
    writeCommand(0xFF);  // Dummy byte
    writeCommand(SSD1306_CMD::ACTIVATE_SCROLL);
}

void SSD1306::startScrollRight([[maybe_unused]] uint8_t startPage, [[maybe_unused]] uint8_t endPage) {
    writeCommand(SSD1306_CMD::DEACTIVATE_SCROLL);
    writeCommand(0x2A);  // Horizontal scroll
    writeCommand(0x00);  // Dummy byte
    writeCommand(0xFF);  // Start page (right scroll)
    writeCommand(0x00);  // Time interval
    writeCommand(0x07);  // End page
    writeCommand(0x00);  // Dummy byte
    writeCommand(0xFF);  // Dummy byte
    writeCommand(SSD1306_CMD::ACTIVATE_SCROLL);
}

void SSD1306::stopScroll() {
    writeCommand(SSD1306_CMD::DEACTIVATE_SCROLL);
}

void SSD1306::setPageAddress(uint8_t startPage, uint8_t endPage) {
    writeCommand(SSD1306_CMD::PAGE_ADDR);
    writeCommand(startPage);
    writeCommand(endPage);
}

void SSD1306::setColumnAddress(uint8_t startColumn, uint8_t endColumn) {
    writeCommand(SSD1306_CMD::COLUMN_ADDR);
    writeCommand(startColumn);
    writeCommand(endColumn);
}

} // namespace Hardware
