#pragma once
// =============================================================================
// SSD1306 SPI Driver — Raspberry Pi (Linux spidev + gpiod)
//
// Conexión GPIO (configurable):
//   DIN  (MOSI) → SPI0_MOSI  GPIO10  Pin19
//   CLK  (SCLK) → SPI0_SCLK  GPIO11  Pin23
//   CS          → SPI0_CE0   GPIO8   Pin24
//   DC          → GPIO25     Pin22
//   RES (Reset) → GPIO17     Pin11
//
// El driver usa /dev/spidev0.0 con SPI mode 0, 8 MHz.
// Para mayor compatibilidad con RPi Zero 2W se usa ioctl directo.
// =============================================================================

#include <cstdint>
#include <string>
#include <array>
#include <vector>

// Dimensiones de la pantalla
constexpr int OLED_WIDTH  = 128;
constexpr int OLED_HEIGHT = 64;
constexpr int OLED_PAGES  = OLED_HEIGHT / 8;  // 8 páginas de 8 bits

class SSD1306 {
public:
    // Pines GPIO (BCM numbering)
    struct Config {
        std::string spiDevice = "/dev/spidev0.0";
        int gpioChip  = 0;     // /dev/gpiochip0
        int pinDC     = 25;    // Data/Command
        int pinReset  = 17;    // Reset
        uint32_t spiSpeedHz = 8000000;  // 8 MHz
    };

    SSD1306();
    explicit SSD1306(const Config& cfg);
    ~SSD1306();

    bool begin();
    void end();

    // Control
    void clear();
    void display();          // Vuelca el framebuffer a la pantalla
    void setContrast(uint8_t contrast);
    void invertDisplay(bool invert);
    void setBrightness(uint8_t b) { setContrast(b); }

    // Framebuffer (1 bit por píxel, organizado en páginas de 8 filas)
    // pixel(x,y) donde x=0..127, y=0..63
    void drawPixel(int x, int y, bool on = true);
    bool getPixel(int x, int y) const;

    // Primitivas de dibujo
    void drawLine(int x0, int y0, int x1, int y1, bool on = true);
    void drawRect(int x, int y, int w, int h, bool on = true);
    void fillRect(int x, int y, int w, int h, bool on = true);
    void drawCircle(int cx, int cy, int r, bool on = true);

    // Texto — fuente 5x7 integrada
    void drawChar(int x, int y, char c, int scale = 1, bool on = true);
    void drawString(int x, int y, const std::string& s, int scale = 1, bool on = true);
    int  charWidth(int scale = 1)  const { return 6 * scale; }
    int  charHeight(int scale = 1) const { return 8 * scale; }

    // Helpers de layout
    void drawStringCentered(int y, const std::string& s, int scale = 1);
    void drawStringRight(int y, const std::string& s, int scale = 1);

    // Barra de progreso horizontal
    void drawProgressBar(int x, int y, int w, int h, float percent, bool on = true);

    // Icono de señal (3 barras)
    void drawSignalIcon(int x, int y, int strength); // strength 0-3

    bool isReady() const { return ready_; }

private:
    Config cfg_;
    int    spiFd_   = -1;
    int    gpioFd_  = -1;   // fd del chip GPIO
    void*  dcLine_  = nullptr;
    void*  rstLine_ = nullptr;
    bool   ready_   = false;

    // Framebuffer: 128 * 8 páginas = 1024 bytes
    std::array<uint8_t, OLED_WIDTH * OLED_PAGES> fb_{};

    // Comandos SSD1306
    void sendCmd(uint8_t cmd);
    void sendCmds(std::initializer_list<uint8_t> cmds);
    void sendData(const uint8_t* data, size_t len);
    void spiWrite(const uint8_t* data, size_t len);

    // GPIO helpers
    bool  gpioInit();
    void  gpioFree();
    void  setDC(bool high);
    void  setReset(bool high);

    // Fuente 5x7
    static const uint8_t FONT5x7[][5];
};
