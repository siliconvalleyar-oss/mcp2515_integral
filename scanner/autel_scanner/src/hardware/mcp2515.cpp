#include "hardware/mcp2515.hpp"
#include "hardware/gpio.hpp"
#include <cstring>
#include <cstdlib>
#include <thread>
#include <unistd.h>

namespace Hardware {

MCP2515::MCP2515(uint8_t csPin, std::shared_ptr<SPI> spi)
    : csPin_(csPin), spi_(std::move(spi)), initialized_(false) {
    if (!spi_) {
        spi_ = std::make_shared<SPI>(0);
    }
}

MCP2515::~MCP2515() {
    cleanup();
}

bool MCP2515::initialize(Bitrate bitrate) {
    if (!spi_->initialize()) {
        return false;
    }

    // Initialize CS pin
    GPIO::setMode(csPin_, GPIO::PinMode::OUTPUT);
    GPIO::write(csPin_, true);

    // Reset MCP2515
    reset();

    // Small delay after reset
    usleep(10000);

    // Set configuration mode
    setControlMode(0x80);  // Configuration mode

    // Set bitrate
    if (!setBitrate(bitrate)) {
        return false;
    }

    // Set masks (accept all messages)
    setMask(0, 0x00000000, false);
    setMask(1, 0x00000000, false);

    // Set filters (accept all)
    setFilter(0, 0x00000000, false);
    setFilter(1, 0x00000000, false);
    setFilter(2, 0x00000000, false);
    setFilter(3, 0x00000000, false);
    setFilter(4, 0x00000000, false);
    setFilter(5, 0x00000000, false);

    // Set RX buffer control (rollover enabled)
    writeRegister(MCP2515Reg::RXB0CTRL, 0x04);  // Rollover to RXB1
    writeRegister(MCP2515Reg::RXB1CTRL, 0x00);

    // Set normal mode
    setControlMode(0x00);  // Normal mode

    usleep(1000);
    initialized_ = true;
    return true;
}

void MCP2515::cleanup() {
    if (initialized_) {
        reset();
        initialized_ = false;
    }
}

void MCP2515::reset() {
    GPIO::write(csPin_, false);
    spi_->transfer(MCP2515Cmd::RESET);
    GPIO::write(csPin_, true);
    usleep(100);
}

uint8_t MCP2515::readRegister(uint8_t reg) {
    uint8_t result = 0;
    GPIO::write(csPin_, false);
    spi_->transfer(MCP2515Cmd::READ);
    spi_->transfer(reg);
    result = spi_->transfer(0x00);
    GPIO::write(csPin_, true);
    return result;
}

void MCP2515::writeRegister(uint8_t reg, uint8_t value) {
    GPIO::write(csPin_, false);
    spi_->transfer(MCP2515Cmd::WRITE);
    spi_->transfer(reg);
    spi_->transfer(value);
    GPIO::write(csPin_, true);
}

void MCP2515::modifyRegister(uint8_t reg, uint8_t mask, uint8_t value) {
    GPIO::write(csPin_, false);
    spi_->transfer(MCP2515Cmd::BIT_MODIFY);
    spi_->transfer(reg);
    spi_->transfer(mask);
    spi_->transfer(value);
    GPIO::write(csPin_, true);
}

void MCP2515::setControlMode(uint8_t mode) {
    modifyRegister(MCP2515Reg::CANCTRL, 0xE0, mode);
}

bool MCP2515::setBitrate(Bitrate bitrate) {
    // Valores CNF1/CNF2/CNF3 para cristal de 8 MHz (tabla del driver
    // emulator/prisma, coincidente con MCP2515_OSC_HZ=8000000).
    switch (bitrate) {
        case Bitrate::BPS_125K:
            writeRegister(MCP2515Reg::CNF1, 0x01);
            writeRegister(MCP2515Reg::CNF2, 0xB1);
            writeRegister(MCP2515Reg::CNF3, 0x85);
            break;
        case Bitrate::BPS_250K:
            writeRegister(MCP2515Reg::CNF1, 0x00);
            writeRegister(MCP2515Reg::CNF2, 0xB1);
            writeRegister(MCP2515Reg::CNF3, 0x85);
            break;
        case Bitrate::BPS_500K:
            writeRegister(MCP2515Reg::CNF1, 0x00);
            writeRegister(MCP2515Reg::CNF2, 0x90);
            writeRegister(MCP2515Reg::CNF3, 0x82);
            break;
        case Bitrate::BPS_1M:
            writeRegister(MCP2515Reg::CNF1, 0x00);
            writeRegister(MCP2515Reg::CNF2, 0x80);
            writeRegister(MCP2515Reg::CNF3, 0x80);
            break;
    }
    return true;
}

bool MCP2515::setFilter(uint8_t filterId, uint32_t id, bool extended) {
    uint8_t regBase;
    switch (filterId) {
        case 0: regBase = MCP2515Reg::RXF0SIDH; break;
        case 1: regBase = MCP2515Reg::RXF1SIDH; break;
        case 2: regBase = MCP2515Reg::RXF2SIDH; break;
        case 3: regBase = MCP2515Reg::RXF3SIDH; break;
        case 4: regBase = MCP2515Reg::RXF4SIDH; break;
        case 5: regBase = MCP2515Reg::RXF5SIDH; break;
        default: return false;
    }

    uint8_t sidh = (id >> 21) & 0xFF;
    uint8_t sidl = ((id >> 18) & 0x03) << 5;
    uint8_t eid8 = (id >> 8) & 0xFF;
    uint8_t eid0 = id & 0xFF;

    if (extended) {
        sidl |= 0x08 | ((id >> 16) & 0x03);
    }

    writeRegister(regBase, sidh);
    writeRegister(regBase + 1, sidl);
    writeRegister(regBase + 2, eid8);
    writeRegister(regBase + 3, eid0);

    return true;
}

bool MCP2515::setMask(uint8_t maskId, uint32_t mask, bool extended) {
    uint8_t regBase = (maskId == 0) ? MCP2515Reg::RXM0SIDH : MCP2515Reg::RXM1SIDH;

    uint8_t sidh = (mask >> 21) & 0xFF;
    uint8_t sidl = ((mask >> 18) & 0x03) << 5;
    uint8_t eid8 = (mask >> 8) & 0xFF;
    uint8_t eid0 = mask & 0xFF;

    if (extended) {
        sidl |= 0x08;
    }

    writeRegister(regBase, sidh);
    writeRegister(regBase + 1, sidl);
    writeRegister(regBase + 2, eid8);
    writeRegister(regBase + 3, eid0);

    return true;
}

bool MCP2515::sendMessage(const CANMessage& msg) {
    return sendMessage(msg.id, msg.data, msg.dlc, msg.extended);
}

bool MCP2515::sendMessage(uint32_t id, const uint8_t* data, uint8_t dlc, bool extended) {
    if (dlc > 8) dlc = 8;

    uint8_t sidh = (id >> 21) & 0xFF;
    uint8_t sidl = ((id >> 18) & 0x03) << 5;
    uint8_t eid8 = (id >> 8) & 0xFF;
    uint8_t eid0 = id & 0xFF;

    if (extended) {
        sidl |= 0x08 | ((id >> 16) & 0x03);
    }

    GPIO::write(csPin_, false);
    spi_->transfer(0x40);  // Load TX buffer
    spi_->transfer(sidl);
    spi_->transfer(sidh);
    spi_->transfer(eid8);
    spi_->transfer(eid0);
    spi_->transfer(dlc);
    for (int i = 0; i < dlc; ++i) {
        spi_->transfer(data[i]);
    }
    GPIO::write(csPin_, true);

    // Request to send
    GPIO::write(csPin_, false);
    spi_->transfer(MCP2515Cmd::RTS_TX0);
    GPIO::write(csPin_, true);

    return true;
}

bool MCP2515::receiveMessage(CANMessage& msg) {
    if (!messageAvailable()) {
        return false;
    }

    uint8_t status = spi_->transfer(MCP2515Cmd::RX_STATUS);
    uint8_t rxBuf = (status & 0xC0) >> 6;

    uint8_t regBase = (rxBuf == 0) ? MCP2515Reg::RXB0SIDH : MCP2515Reg::RXB1SIDH;

    GPIO::write(csPin_, false);
    spi_->transfer(MCP2515Cmd::READ);
    spi_->transfer(regBase);
    uint8_t sidl = spi_->transfer(0x00);
    uint8_t sidh = spi_->transfer(0x00);
    uint8_t eid8 = spi_->transfer(0x00);
    uint8_t eid0 = spi_->transfer(0x00);
    uint8_t dlc = spi_->transfer(0x00);
    uint8_t data[8] = {0};
    for (int i = 0; i < 8; ++i) {
        data[i] = spi_->transfer(0x00);
    }
    GPIO::write(csPin_, true);

    msg.extended = (sidl & 0x08) != 0;
    msg.rtr = (dlc & 0x40) != 0;
    msg.dlc = dlc & 0x0F;
    memcpy(msg.data, data, msg.dlc);

    if (msg.extended) {
        msg.id = (static_cast<uint32_t>(sidh) << 21) |
                 (static_cast<uint32_t>(sidl & 0x03) << 16) |
                 (static_cast<uint32_t>(eid8) << 8) |
                 eid0;
    } else {
        msg.id = (static_cast<uint32_t>(sidh) << 3) | ((sidl >> 5) & 0x07);
    }

    return true;
}

bool MCP2515::messageAvailable() {
    GPIO::write(csPin_, false);
    uint8_t status = spi_->transfer(MCP2515Cmd::READ_STATUS);
    GPIO::write(csPin_, true);
    return (status & 0x80) || (status & 0x40);
}

uint8_t MCP2515::getErrorFlags() {
    return readRegister(MCP2515Reg::CANSTAT);
}

uint8_t MCP2515::getReceiveErrorCounter() {
    return readRegister(MCP2515Reg::REC);
}

uint8_t MCP2515::getTransmitErrorCounter() {
    return readRegister(MCP2515Reg::TEC);
}

} // namespace Hardware
