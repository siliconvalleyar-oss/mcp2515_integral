// ssd1306.cpp — Driver SPI para SSD1306 128x64 en Raspberry Pi
// Usa Linux spidev (/dev/spidev0.0) + libgpiod para DC y RES
//
// Referencia: SSD1306 Datasheet Rev 1.1, Solomon Systech, 2008

#include "ssd1306.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <gpiod.h>

// ─── Fuente 5x7 (ASCII 32-127) ───────────────────────────────────────────────
// Cada carácter ocupa 5 bytes (columnas de 8 bits)
const uint8_t SSD1306::FONT5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // 32 espacio
    {0x00,0x00,0x5F,0x00,0x00}, // 33 !
    {0x00,0x07,0x00,0x07,0x00}, // 34 "
    {0x14,0x7F,0x14,0x7F,0x14}, // 35 #
    {0x24,0x2A,0x7F,0x2A,0x12}, // 36 $
    {0x23,0x13,0x08,0x64,0x62}, // 37 %
    {0x36,0x49,0x55,0x22,0x50}, // 38 &
    {0x00,0x05,0x03,0x00,0x00}, // 39 '
    {0x00,0x1C,0x22,0x41,0x00}, // 40 (
    {0x00,0x41,0x22,0x1C,0x00}, // 41 )
    {0x14,0x08,0x3E,0x08,0x14}, // 42 *
    {0x08,0x08,0x3E,0x08,0x08}, // 43 +
    {0x00,0x50,0x30,0x00,0x00}, // 44 ,
    {0x08,0x08,0x08,0x08,0x08}, // 45 -
    {0x00,0x60,0x60,0x00,0x00}, // 46 .
    {0x20,0x10,0x08,0x04,0x02}, // 47 /
    {0x3E,0x51,0x49,0x45,0x3E}, // 48 0
    {0x00,0x42,0x7F,0x40,0x00}, // 49 1
    {0x42,0x61,0x51,0x49,0x46}, // 50 2
    {0x21,0x41,0x45,0x4B,0x31}, // 51 3
    {0x18,0x14,0x12,0x7F,0x10}, // 52 4
    {0x27,0x45,0x45,0x45,0x39}, // 53 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 54 6
    {0x01,0x71,0x09,0x05,0x03}, // 55 7
    {0x36,0x49,0x49,0x49,0x36}, // 56 8
    {0x06,0x49,0x49,0x29,0x1E}, // 57 9
    {0x00,0x36,0x36,0x00,0x00}, // 58 :
    {0x00,0x56,0x36,0x00,0x00}, // 59 ;
    {0x08,0x14,0x22,0x41,0x00}, // 60 <
    {0x14,0x14,0x14,0x14,0x14}, // 61 =
    {0x00,0x41,0x22,0x14,0x08}, // 62 >
    {0x02,0x01,0x51,0x09,0x06}, // 63 ?
    {0x32,0x49,0x79,0x41,0x3E}, // 64 @
    {0x7E,0x11,0x11,0x11,0x7E}, // 65 A
    {0x7F,0x49,0x49,0x49,0x36}, // 66 B
    {0x3E,0x41,0x41,0x41,0x22}, // 67 C
    {0x7F,0x41,0x41,0x22,0x1C}, // 68 D
    {0x7F,0x49,0x49,0x49,0x41}, // 69 E
    {0x7F,0x09,0x09,0x09,0x01}, // 70 F
    {0x3E,0x41,0x49,0x49,0x7A}, // 71 G
    {0x7F,0x08,0x08,0x08,0x7F}, // 72 H
    {0x00,0x41,0x7F,0x41,0x00}, // 73 I
    {0x20,0x40,0x41,0x3F,0x01}, // 74 J
    {0x7F,0x08,0x14,0x22,0x41}, // 75 K
    {0x7F,0x40,0x40,0x40,0x40}, // 76 L
    {0x7F,0x02,0x0C,0x02,0x7F}, // 77 M
    {0x7F,0x04,0x08,0x10,0x7F}, // 78 N
    {0x3E,0x41,0x41,0x41,0x3E}, // 79 O
    {0x7F,0x09,0x09,0x09,0x06}, // 80 P
    {0x3E,0x41,0x51,0x21,0x5E}, // 81 Q
    {0x7F,0x09,0x19,0x29,0x46}, // 82 R
    {0x46,0x49,0x49,0x49,0x31}, // 83 S
    {0x01,0x01,0x7F,0x01,0x01}, // 84 T
    {0x3F,0x40,0x40,0x40,0x3F}, // 85 U
    {0x1F,0x20,0x40,0x20,0x1F}, // 86 V
    {0x3F,0x40,0x38,0x40,0x3F}, // 87 W
    {0x63,0x14,0x08,0x14,0x63}, // 88 X
    {0x07,0x08,0x70,0x08,0x07}, // 89 Y
    {0x61,0x51,0x49,0x45,0x43}, // 90 Z
    {0x00,0x7F,0x41,0x41,0x00}, // 91 [
    {0x02,0x04,0x08,0x10,0x20}, // 92 backslash
    {0x00,0x41,0x41,0x7F,0x00}, // 93 ]
    {0x04,0x02,0x01,0x02,0x04}, // 94 ^
    {0x40,0x40,0x40,0x40,0x40}, // 95 _
    {0x00,0x01,0x02,0x04,0x00}, // 96 `
    {0x20,0x54,0x54,0x54,0x78}, // 97 a
    {0x7F,0x48,0x44,0x44,0x38}, // 98 b
    {0x38,0x44,0x44,0x44,0x20}, // 99 c
    {0x38,0x44,0x44,0x48,0x7F}, // 100 d
    {0x38,0x54,0x54,0x54,0x18}, // 101 e
    {0x08,0x7E,0x09,0x01,0x02}, // 102 f
    {0x0C,0x52,0x52,0x52,0x3E}, // 103 g
    {0x7F,0x08,0x04,0x04,0x78}, // 104 h
    {0x00,0x44,0x7D,0x40,0x00}, // 105 i
    {0x20,0x40,0x44,0x3D,0x00}, // 106 j
    {0x7F,0x10,0x28,0x44,0x00}, // 107 k
    {0x00,0x41,0x7F,0x40,0x00}, // 108 l
    {0x7C,0x04,0x18,0x04,0x78}, // 109 m
    {0x7C,0x08,0x04,0x04,0x78}, // 110 n
    {0x38,0x44,0x44,0x44,0x38}, // 111 o
    {0x7C,0x14,0x14,0x14,0x08}, // 112 p
    {0x08,0x14,0x14,0x18,0x7C}, // 113 q
    {0x7C,0x08,0x04,0x04,0x08}, // 114 r
    {0x48,0x54,0x54,0x54,0x20}, // 115 s
    {0x04,0x3F,0x44,0x40,0x20}, // 116 t
    {0x3C,0x40,0x40,0x40,0x3C}, // 117 u
    {0x1C,0x20,0x40,0x20,0x1C}, // 118 v
    {0x3C,0x40,0x30,0x40,0x3C}, // 119 w
    {0x44,0x28,0x10,0x28,0x44}, // 120 x
    {0x0C,0x50,0x50,0x50,0x3C}, // 121 y
    {0x44,0x64,0x54,0x4C,0x44}, // 122 z
    {0x00,0x08,0x36,0x41,0x00}, // 123 {
    {0x00,0x00,0x7F,0x00,0x00}, // 124 |
    {0x00,0x41,0x36,0x08,0x00}, // 125 }
    {0x10,0x08,0x08,0x10,0x08}, // 126 ~
    {0x00,0x00,0x00,0x00,0x00}, // 127 DEL
};

// ─── Comandos de inicialización SSD1306 ──────────────────────────────────────
static const uint8_t INIT_CMDS[] = {
    0xAE,       // display OFF
    0xD5, 0x80, // Set Display Clock Divide Ratio / Oscillator Frequency
    0xA8, 0x3F, // Set Multiplex Ratio (63 = 64 líneas)
    0xD3, 0x00, // Set Display Offset = 0
    0x40,       // Set Display Start Line = 0
    0x8D, 0x14, // Charge Pump: enable (0x14=ON, 0x10=OFF)
    0x20, 0x00, // Memory Addressing Mode: Horizontal
    0xA1,       // Segment Re-map: col 127→SEG0 (espejo horizontal)
    0xC8,       // COM Output Scan Direction: remapped (espejo vertical)
    0xDA, 0x12, // Set COM Pins Hardware Configuration
    0x81, 0xCF, // Set Contrast Control
    0xD9, 0xF1, // Set Pre-charge Period
    0xDB, 0x40, // Set VCOMH Deselect Level
    0xA4,       // Entire Display ON (follow RAM)
    0xA6,       // Set Normal Display (no invertir)
    0x2E,       // Deactivate scroll
    0xAF        // display ON
};

// ─── Constructor / Destructor ────────────────────────────────────────────────

SSD1306::SSD1306() : cfg_(Config{}) {}
SSD1306::SSD1306(const Config& cfg) : cfg_(cfg) {}

SSD1306::~SSD1306() { end(); }

// ─── Inicialización ──────────────────────────────────────────────────────────

bool SSD1306::begin()
{
    // Abrir SPI
    spiFd_ = open(cfg_.spiDevice.c_str(), O_RDWR);
    if (spiFd_ < 0) {
        std::cerr << "[SSD1306] No se pudo abrir " << cfg_.spiDevice
                  << " — ¿SPI habilitado con raspi-config?\n";
        return false;
    }

    // Configurar SPI: Mode 0, 8 bits, velocidad
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = cfg_.spiSpeedHz;
    ioctl(spiFd_, SPI_IOC_WR_MODE, &mode);
    ioctl(spiFd_, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spiFd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    // GPIO
    if (!gpioInit()) return false;

    // Reset hardware: RES=LOW 10ms → HIGH
    setReset(false); usleep(10000);
    setReset(true);  usleep(10000);

    // Enviar secuencia de inicialización
    for (uint8_t cmd : INIT_CMDS) sendCmd(cmd);

    clear();
    display();

    ready_ = true;
    std::cout << "[SSD1306] OLED inicializado OK\n";
    return true;
}

void SSD1306::end()
{
    if (ready_) {
        // Apagar display antes de cerrar
        sendCmd(0xAE);
        ready_ = false;
    }
    gpioFree();
    if (spiFd_ >= 0) { close(spiFd_); spiFd_ = -1; }
}

// ─── GPIO con libgpiod ───────────────────────────────────────────────────────

bool SSD1306::gpioInit()
{
    std::string chipPath = "/dev/gpiochip" + std::to_string(cfg_.gpioChip);
    gpioFd_ = open(chipPath.c_str(), O_RDWR);
    if (gpioFd_ < 0) {
        std::cerr << "[SSD1306] No se pudo abrir " << chipPath << "\n";
        return false;
    }

    struct gpiochip_info info;
    ioctl(gpioFd_, GPIO_GET_CHIPINFO_IOCTL, &info);

    // Configurar DC y RES como salida via ioctl
    // Usamos gpiod_chip / gpiod_line para compatibilidad
    struct gpiod_chip* chip = gpiod_chip_open(chipPath.c_str());
    if (!chip) {
        std::cerr << "[SSD1306] gpiod_chip_open falló\n";
        return false;
    }

    struct gpiod_line* dc  = gpiod_chip_get_line(chip, cfg_.pinDC);
    struct gpiod_line* rst = gpiod_chip_get_line(chip, cfg_.pinReset);

    if (!dc || !rst) {
        std::cerr << "[SSD1306] No se pudieron obtener líneas GPIO DC=" 
                  << cfg_.pinDC << " RST=" << cfg_.pinReset << "\n";
        gpiod_chip_close(chip);
        return false;
    }

    gpiod_line_request_output(dc,  "ssd1306_dc",  0);
    gpiod_line_request_output(rst, "ssd1306_rst", 1);

    dcLine_  = dc;
    rstLine_ = rst;

    // Guardar chip handle en gpioFd_ area — usamos puntero a chip
    close(gpioFd_);
    gpioFd_ = -1;

    // Guardamos el chip como void* usando un miembro adicional
    // (reutilizamos gpioFd_ como flag, chip se guarda en dcLine_ parent)
    return true;
}

void SSD1306::gpioFree()
{
    if (dcLine_)  { gpiod_line_release((struct gpiod_line*)dcLine_);  dcLine_=nullptr;  }
    if (rstLine_) { gpiod_line_release((struct gpiod_line*)rstLine_); rstLine_=nullptr; }
}

void SSD1306::setDC(bool high) {
    if (dcLine_) gpiod_line_set_value((struct gpiod_line*)dcLine_, high ? 1 : 0);
}
void SSD1306::setReset(bool high) {
    if (rstLine_) gpiod_line_set_value((struct gpiod_line*)rstLine_, high ? 1 : 0);
}

// ─── SPI ─────────────────────────────────────────────────────────────────────

void SSD1306::spiWrite(const uint8_t* data, size_t len)
{
    if (spiFd_ < 0 || !data || len == 0) return;
    struct spi_ioc_transfer tr{};
    tr.tx_buf       = (unsigned long)data;
    tr.rx_buf       = 0;
    tr.len          = len;
    tr.speed_hz     = cfg_.spiSpeedHz;
    tr.bits_per_word= 8;
    tr.delay_usecs  = 0;
    ioctl(spiFd_, SPI_IOC_MESSAGE(1), &tr);
}

void SSD1306::sendCmd(uint8_t cmd) {
    setDC(false);  // DC=0 → comando
    spiWrite(&cmd, 1);
}

void SSD1306::sendCmds(std::initializer_list<uint8_t> cmds) {
    setDC(false);
    for (uint8_t c : cmds) spiWrite(&c, 1);
}

void SSD1306::sendData(const uint8_t* data, size_t len) {
    setDC(true);   // DC=1 → datos
    spiWrite(data, len);
}

// ─── Framebuffer ─────────────────────────────────────────────────────────────

void SSD1306::clear() {
    fb_.fill(0x00);
}

void SSD1306::display()
{
    if (!ready_) return;
    // Configurar ventana completa
    sendCmds({0x21, 0, 127}); // column start=0, end=127
    sendCmds({0x22, 0, 7});   // page start=0, end=7
    sendData(fb_.data(), fb_.size());
}

void SSD1306::setContrast(uint8_t contrast) {
    sendCmds({0x81, contrast});
}
void SSD1306::invertDisplay(bool invert) {
    sendCmd(invert ? 0xA7 : 0xA6);
}

// ─── Primitivas ──────────────────────────────────────────────────────────────

void SSD1306::drawPixel(int x, int y, bool on)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    int page = y / 8;
    int bit  = y % 8;
    int idx  = page * OLED_WIDTH + x;
    if (on) fb_[idx] |=  (1 << bit);
    else    fb_[idx] &= ~(1 << bit);
}

bool SSD1306::getPixel(int x, int y) const {
    if (x<0||x>=OLED_WIDTH||y<0||y>=OLED_HEIGHT) return false;
    return (fb_[(y/8)*OLED_WIDTH+x] >> (y%8)) & 1;
}

void SSD1306::drawLine(int x0, int y0, int x1, int y1, bool on)
{
    int dx = std::abs(x1-x0), dy = std::abs(y1-y0);
    int sx = x0<x1?1:-1, sy = y0<y1?1:-1;
    int err = dx-dy;
    while (true) {
        drawPixel(x0,y0,on);
        if (x0==x1 && y0==y1) break;
        int e2=2*err;
        if (e2>-dy){err-=dy; x0+=sx;}
        if (e2< dx){err+=dx; y0+=sy;}
    }
}

void SSD1306::drawRect(int x, int y, int w, int h, bool on) {
    drawLine(x,   y,   x+w-1, y,     on);
    drawLine(x,   y+h-1,x+w-1,y+h-1, on);
    drawLine(x,   y,   x,     y+h-1, on);
    drawLine(x+w-1,y,  x+w-1, y+h-1, on);
}

void SSD1306::fillRect(int x, int y, int w, int h, bool on) {
    for (int j=y; j<y+h; j++)
        for (int i=x; i<x+w; i++)
            drawPixel(i,j,on);
}

void SSD1306::drawCircle(int cx, int cy, int r, bool on) {
    int x=r, y=0, err=0;
    while (x>=y) {
        drawPixel(cx+x,cy+y,on); drawPixel(cx+y,cy+x,on);
        drawPixel(cx-y,cy+x,on); drawPixel(cx-x,cy+y,on);
        drawPixel(cx-x,cy-y,on); drawPixel(cx-y,cy-x,on);
        drawPixel(cx+y,cy-x,on); drawPixel(cx+x,cy-y,on);
        y++;
        if (err<=0) err+=2*y+1;
        else { x--; err+=2*(y-x)+1; }
    }
}

// ─── Texto ───────────────────────────────────────────────────────────────────

void SSD1306::drawChar(int x, int y, char c, int scale, bool on)
{
    int idx = (int)c - 32;
    if (idx < 0 || idx >= 96) idx = 0;
    const uint8_t* glyph = FONT5x7[idx];

    for (int col=0; col<5; col++) {
        uint8_t column = glyph[col];
        for (int row=0; row<8; row++) {
            if ((column >> row) & 1) {
                if (scale == 1) {
                    drawPixel(x+col, y+row, on);
                } else {
                    fillRect(x+col*scale, y+row*scale, scale, scale, on);
                }
            }
        }
    }
}

void SSD1306::drawString(int x, int y, const std::string& s, int scale, bool on)
{
    int cx = x;
    for (char c : s) {
        if (c == '\n') { cx=x; y += 8*scale+1; continue; }
        drawChar(cx, y, c, scale, on);
        cx += 6*scale;
        if (cx >= OLED_WIDTH) break;
    }
}

void SSD1306::drawStringCentered(int y, const std::string& s, int scale)
{
    int len = (int)s.length() * 6 * scale - scale; // último char sin gap
    int x = (OLED_WIDTH - len) / 2;
    if (x < 0) x = 0;
    drawString(x, y, s, scale);
}

void SSD1306::drawStringRight(int y, const std::string& s, int scale)
{
    int len = (int)s.length() * 6 * scale - scale;
    int x = OLED_WIDTH - len - 1;
    if (x < 0) x = 0;
    drawString(x, y, s, scale);
}

void SSD1306::drawProgressBar(int x, int y, int w, int h, float percent, bool on)
{
    drawRect(x, y, w, h, on);
    int fill = (int)((w-2) * std::max(0.0f, std::min(1.0f, percent/100.0f)));
    if (fill > 0) fillRect(x+1, y+1, fill, h-2, on);
}

void SSD1306::drawSignalIcon(int x, int y, int strength)
{
    // 3 barras verticales de altura creciente
    for (int i=0; i<3; i++) {
        int bh = 3 + i*3;
        int bx = x + i*4;
        int by = y + (9-bh);
        if (i < strength) fillRect(bx, by, 3, bh, true);
        else              drawRect(bx, by, 3, bh, true);
    }
}
