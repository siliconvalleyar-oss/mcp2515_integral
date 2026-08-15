#ifndef HARDWARE_SSD1306_HPP
#define HARDWARE_SSD1306_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>

namespace Hardware {

class I2C;

class SSD1306 {
public:
    enum class Color : uint8_t {
        BLACK = 0,
        WHITE = 1,
        INVERSE = 2
    };

    enum class Font {
        FONT_6x8,
        FONT_8x16
    };

    struct Point {
        int x;
        int y;
    };

    struct Rect {
        int x;
        int y;
        int width;
        int height;
    };

    SSD1306(uint8_t address = 0x3C, int width = 128, int height = 32);
    ~SSD1306();

    bool initialize();
    void cleanup();

    // Display control
    void clear();
    void display();
    void setBrightness(uint8_t brightness);
    void setContrast(uint8_t contrast);
    void invert(bool invert);

    // Drawing primitives
    void drawPixel(int x, int y, Color color = Color::WHITE);
    void drawLine(int x0, int y0, int x1, int y1, Color color = Color::WHITE);
    void drawRect(const Rect& rect, Color color = Color::WHITE);
    void fillRect(const Rect& rect, Color color = Color::WHITE);
    void drawCircle(int centerX, int centerY, int radius, Color color = Color::WHITE);
    void drawChar(char c, int x, int y, Font font = Font::FONT_6x8, Color color = Color::WHITE);
    void drawText(const std::string& text, int x, int y, Font font = Font::FONT_6x8, Color color = Color::WHITE);

    // Buffer operations
    const uint8_t* buffer() const { return buffer_.data(); }
    size_t bufferSize() const { return buffer_.size(); }
    int width() const { return width_; }
    int height() const { return height_; }

    // Scroll
    void startScrollLeft(uint8_t startPage, uint8_t endPage);
    void startScrollRight(uint8_t startPage, uint8_t endPage);
    void stopScroll();

private:
    bool writeCommand(uint8_t cmd);
    bool writeData(const uint8_t* data, size_t length);

    void setPageAddress(uint8_t startPage, uint8_t endPage);
    void setColumnAddress(uint8_t startColumn, uint8_t endColumn);

    uint8_t address_;
    int width_;
    int height_;
    std::vector<uint8_t> buffer_;
    std::unique_ptr<I2C> i2c_;
    bool initialized_;
};

} // namespace Hardware

#endif // HARDWARE_SSD1306_HPP
