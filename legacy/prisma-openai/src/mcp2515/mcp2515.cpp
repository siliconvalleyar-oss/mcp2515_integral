#include "mcp2515/mcp2515.h"

#include <bcm2835.h>
#include <cstring>
#include <iostream>

namespace {

constexpr uint8_t CANSTAT  = 0x0E;
constexpr uint8_t CANCTRL  = 0x0F;

constexpr uint8_t CNF1 = 0x2A;
constexpr uint8_t CNF2 = 0x29;
constexpr uint8_t CNF3 = 0x28;

constexpr uint8_t CANINTE = 0x2B;
constexpr uint8_t CANINTF = 0x2C;

constexpr uint8_t RXB0CTRL = 0x60;
constexpr uint8_t RXB1CTRL = 0x70;

constexpr uint8_t TXB0CTRL = 0x30;
constexpr uint8_t TXB0SIDH = 0x31;
constexpr uint8_t TXB0SIDL = 0x32;
constexpr uint8_t TXB0DLC  = 0x35;
constexpr uint8_t TXB0DATA = 0x36;

constexpr uint8_t RXB0SIDH = 0x61;
constexpr uint8_t RXB0SIDL = 0x62;
constexpr uint8_t RXB0DLC  = 0x65;
constexpr uint8_t RXB0DATA = 0x66;

constexpr uint8_t RXB1SIDH = 0x71;
constexpr uint8_t RXB1SIDL = 0x72;
constexpr uint8_t RXB1DLC  = 0x75;
constexpr uint8_t RXB1DATA = 0x76;

constexpr uint8_t SPI_RESET = 0xC0;
constexpr uint8_t SPI_READ  = 0x03;
constexpr uint8_t SPI_WRITE = 0x02;
constexpr uint8_t SPI_BITMOD = 0x05;
constexpr uint8_t SPI_RTS_TX0 = 0x81;
constexpr uint8_t SPI_READ_STATUS = 0xA0;

constexpr uint8_t INT_RX0IF = 0x01;
constexpr uint8_t INT_RX1IF = 0x02;

}

MCP2515::MCP2515(int csPin, int intPin)
    : csPin_(csPin),
      intPin_(intPin),
      initialized_(false) {
}

void MCP2515::select() {
    bcm2835_gpio_write(csPin_, LOW);
}

void MCP2515::deselect() {
    bcm2835_gpio_write(csPin_, HIGH);
}

void MCP2515::reset() {
    select();

    uint8_t cmd = SPI_RESET;
    bcm2835_spi_transfer(cmd);

    deselect();

    bcm2835_delay(10);
}

uint8_t MCP2515::readRegister(uint8_t address) {
    select();

    bcm2835_spi_transfer(SPI_READ);
    bcm2835_spi_transfer(address);

    uint8_t value = bcm2835_spi_transfer(0x00);

    deselect();

    return value;
}

void MCP2515::readRegisters(uint8_t address,
                            uint8_t* buffer,
                            size_t len) {
    select();

    bcm2835_spi_transfer(SPI_READ);
    bcm2835_spi_transfer(address);

    for (size_t i = 0; i < len; ++i) {
        buffer[i] = bcm2835_spi_transfer(0x00);
    }

    deselect();
}

void MCP2515::writeRegister(uint8_t address, uint8_t value) {
    select();

    bcm2835_spi_transfer(SPI_WRITE);
    bcm2835_spi_transfer(address);
    bcm2835_spi_transfer(value);

    deselect();
}

void MCP2515::writeRegisters(uint8_t address,
                             const uint8_t* buffer,
                             size_t len) {
    select();

    bcm2835_spi_transfer(SPI_WRITE);
    bcm2835_spi_transfer(address);

    for (size_t i = 0; i < len; ++i) {
        bcm2835_spi_transfer(buffer[i]);
    }

    deselect();
}

void MCP2515::bitModify(uint8_t address,
                        uint8_t mask,
                        uint8_t data) {
    select();

    bcm2835_spi_transfer(SPI_BITMOD);
    bcm2835_spi_transfer(address);
    bcm2835_spi_transfer(mask);
    bcm2835_spi_transfer(data);

    deselect();
}

uint8_t MCP2515::readStatus() {
    select();

    bcm2835_spi_transfer(SPI_READ_STATUS);
    uint8_t status = bcm2835_spi_transfer(0x00);

    deselect();

    return status;
}

bool MCP2515::configureBitrate(Bitrate bitrate) {
    /*
     * MCP2515 con cristal de 8 MHz.
     *
     * Estos valores son para:
     *
     * 125 kbit/s
     * 250 kbit/s
     * 500 kbit/s
     */
    switch (bitrate) {
        case Bitrate::KBPS125:
            writeRegister(CNF1, 0x03);
            writeRegister(CNF2, 0xF0);
            writeRegister(CNF3, 0x86);
            break;

        case Bitrate::KBPS250:
            writeRegister(CNF1, 0x01);
            writeRegister(CNF2, 0xF0);
            writeRegister(CNF3, 0x86);
            break;

        case Bitrate::KBPS500:
            writeRegister(CNF1, 0x00);
            writeRegister(CNF2, 0xD0);
            writeRegister(CNF3, 0x82);
            break;
    }

    return true;
}

bool MCP2515::begin(Bitrate bitrate) {
    if (!bcm2835_init()) {
        std::cerr << "Error: bcm2835_init() fallo\n";
        return false;
    }

    if (!bcm2835_spi_begin()) {
        std::cerr << "Error: bcm2835_spi_begin() fallo\n";
        bcm2835_close();
        return false;
    }

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);

    /*
     * 8 MHz es un valor conservador.
     */
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256);

    bcm2835_gpio_fsel(csPin_, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(csPin_, HIGH);

    bcm2835_gpio_fsel(intPin_, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(intPin_, BCM2835_GPIO_PUD_UP);

    reset();

    if (readRegister(CANSTAT) == 0xFF) {
        std::cerr << "No se detecta correctamente el MCP2515\n";
        return false;
    }

    if (!setMode(Mode::CONFIG)) {
        return false;
    }

    configureBitrate(bitrate);

    /*
     * RXB0/RXB1:
     * aceptar todos los mensajes para el simulador.
     */
    writeRegister(RXB0CTRL, 0x60);
    writeRegister(RXB1CTRL, 0x60);

    /*
     * Habilitar interrupciones RX0/RX1.
     */
    writeRegister(CANINTE, INT_RX0IF | INT_RX1IF);

    if (!setMode(Mode::NORMAL)) {
        return false;
    }

    initialized_ = true;
    return true;
}

void MCP2515::end() {
    if (!initialized_) {
        return;
    }

    bcm2835_spi_end();
    bcm2835_close();

    initialized_ = false;
}

bool MCP2515::setMode(Mode mode) {
    bitModify(CANCTRL, 0xE0,
             static_cast<uint8_t>(mode));

    for (int i = 0; i < 100; ++i) {
        uint8_t status =
            readRegister(CANSTAT) & 0xE0;

        if (status ==
            static_cast<uint8_t>(mode)) {
            return true;
        }

        bcm2835_delay(1);
    }

    return false;
}

bool MCP2515::writeTxBuffer(const CanFrame& frame) {
    if (frame.extended) {
        /*
         * Soporte básico de ID extendido.
         */
        uint8_t sidh =
            static_cast<uint8_t>((frame.id >> 21) & 0xFF);

        uint8_t sidl =
            static_cast<uint8_t>(
                ((frame.id >> 13) & 0xE0) |
                0x08 |
                ((frame.id >> 16) & 0x03)
            );

        uint8_t eid8 =
            static_cast<uint8_t>((frame.id >> 8) & 0xFF);

        uint8_t eid0 =
            static_cast<uint8_t>(frame.id & 0xFF);

        writeRegister(TXB0SIDH, sidh);
        writeRegister(TXB0SIDL, sidl);
        writeRegister(TXB0SIDL + 1, eid8);
        writeRegister(TXB0SIDL + 2, eid0);
    } else {
        writeRegister(
            TXB0SIDH,
            static_cast<uint8_t>((frame.id >> 3) & 0xFF)
        );

        writeRegister(
            TXB0SIDL,
            static_cast<uint8_t>((frame.id & 0x07) << 5)
        );
    }

    writeRegister(
        TXB0DLC,
        static_cast<uint8_t>(frame.dlc & 0x0F)
    );

    writeRegisters(TXB0DATA,
                   frame.data,
                   frame.dlc);

    return true;
}

bool MCP2515::requestToSend() {
    select();

    bcm2835_spi_transfer(SPI_RTS_TX0);

    deselect();

    return true;
}

bool MCP2515::send(const CanFrame& frame) {
    if (!initialized_ || frame.dlc > 8) {
        return false;
    }

    if (!writeTxBuffer(frame)) {
        return false;
    }

    requestToSend();

    for (int i = 0; i < 100; ++i) {
        uint8_t status = readRegister(TXB0CTRL);

        if ((status & 0x08) == 0) {
            return true;
        }

        bcm2835_delay(1);
    }

    return false;
}

bool MCP2515::available() const {
    if (!initialized_) {
        return false;
    }

    /*
     * INT del MCP2515 es activo en LOW.
     */
    return bcm2835_gpio_lev(intPin_) == LOW;
}

bool MCP2515::readRxBuffer(uint8_t bufferNumber,
                           CanFrame& frame) {
    uint8_t sidh;
    uint8_t sidl;
    uint8_t dlc;

    uint8_t address;

    if (bufferNumber == 0) {
        address = RXB0SIDH;
    } else {
        address = RXB1SIDH;
    }

    sidh = readRegister(address);
    sidl = readRegister(address + 1);

    dlc = readRegister(address + 4);

    frame.extended =
        (sidl & 0x08) != 0;

    if (frame.extended) {
        uint8_t eid8 =
            readRegister(address + 2);

        uint8_t eid0 =
            readRegister(address + 3);

        frame.id =
            (static_cast<uint32_t>(sidh) << 21) |
            (static_cast<uint32_t>(sidl & 0x03) << 16) |
            (static_cast<uint32_t>(eid8) << 8) |
            eid0;
    } else {
        frame.id =
            (static_cast<uint32_t>(sidh) << 3) |
            (sidl >> 5);
    }

    frame.dlc = dlc & 0x0F;

    if (frame.dlc > 8) {
        frame.dlc = 8;
    }

    if (bufferNumber == 0) {
        readRegisters(RXB0DATA,
                      frame.data,
                      frame.dlc);
    } else {
        readRegisters(RXB1DATA,
                      frame.data,
                      frame.dlc);
    }

    return true;
}

bool MCP2515::receive(CanFrame& frame) {
    if (!initialized_) {
        return false;
    }

    uint8_t flags = readRegister(CANINTF);

    if (flags & INT_RX0IF) {
        if (readRxBuffer(0, frame)) {
            bitModify(CANINTF, INT_RX0IF, 0);
            return true;
        }
    }

    if (flags & INT_RX1IF) {
        if (readRxBuffer(1, frame)) {
            bitModify(CANINTF, INT_RX1IF, 0);
            return true;
        }
    }

    return false;
}
