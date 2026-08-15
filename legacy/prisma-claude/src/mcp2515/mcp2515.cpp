#include "mcp2515/mcp2515.h"
#include <bcm2835.h>
#include <cstdio>
#include <unistd.h>

MCP2515::MCP2515(uint8_t pinCS, uint8_t pinInt)
    : _pinCS(pinCS), _pinInt(pinInt) {}

MCP2515::~MCP2515() {}

bool MCP2515::begin(CanBitrate bitrate) {
    if (!bcm2835_init()) {
        fprintf(stderr, "[MCP2515] Error: no se pudo inicializar bcm2835 (ejecute con sudo/root)\n");
        return false;
    }

    if (!bcm2835_spi_begin()) {
        fprintf(stderr, "[MCP2515] Error: no se pudo iniciar el bus SPI\n");
        return false;
    }

    // MISO/MOSI/SCLK quedan gestionados automáticamente por bcm2835 al
    // habilitar el modo SPI (GPIO9/GPIO10/GPIO11). CE0 actúa como CS por hardware.
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256); // ~1 MHz aprox.
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                    // CE0 -> CS del MCP2515
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    // GPIO25 como entrada para la línea INT del MCP2515 (activa en bajo)
    bcm2835_gpio_fsel(_pinInt, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(_pinInt, BCM2835_GPIO_PUD_UP);

    reset();
    usleep(10000);

    setBitrate(bitrate);

    return true;
}

void MCP2515::reset() {
    char buf[1] = { (char)MCP2515_RESET };
    bcm2835_spi_transfern(buf, 1);
    usleep(10000);
}

uint8_t MCP2515::readRegister(uint8_t address) {
    char buf[3] = { (char)MCP2515_READ, (char)address, 0x00 };
    bcm2835_spi_transfern(buf, 3);
    return (uint8_t)buf[2];
}

void MCP2515::readRegisters(uint8_t address, uint8_t *buffer, uint8_t n) {
    if (n > 13) n = 13; // máximo necesario: SIDH..D7 (13 bytes)
    char buf[2 + 13];
    buf[0] = (char)MCP2515_READ;
    buf[1] = (char)address;
    for (int i = 0; i < n; i++) buf[2 + i] = 0x00;

    bcm2835_spi_transfern(buf, 2 + n);

    for (int i = 0; i < n; i++) buffer[i] = (uint8_t)buf[2 + i];
}

void MCP2515::writeRegister(uint8_t address, uint8_t value) {
    char buf[3] = { (char)MCP2515_WRITE, (char)address, (char)value };
    bcm2835_spi_transfern(buf, 3);
}

void MCP2515::modifyRegister(uint8_t address, uint8_t mask, uint8_t value) {
    char buf[4] = { (char)MCP2515_BIT_MODIFY, (char)address, (char)mask, (char)value };
    bcm2835_spi_transfern(buf, 4);
}

uint8_t MCP2515::readStatus() {
    char buf[2] = { (char)MCP2515_READ_STATUS, 0x00 };
    bcm2835_spi_transfern(buf, 2);
    return (uint8_t)buf[1];
}

void MCP2515::setBitrate(CanBitrate bitrate) {
    // La configuración de CNF1/2/3 requiere estar en modo configuración
    modifyRegister(REG_CANCTRL, MODE_MASK, MODE_CONFIG);
    usleep(1000);

    uint8_t cnf1, cnf2, cnf3;
    // Valores típicos para cristal de 8 MHz en el módulo MCP2515.
    // Si el módulo usa cristal de 16 MHz, ajustar esta tabla.
    switch (bitrate) {
        case CanBitrate::BPS_125K:  cnf1 = 0x01; cnf2 = 0xB1; cnf3 = 0x85; break;
        case CanBitrate::BPS_250K:  cnf1 = 0x00; cnf2 = 0xB1; cnf3 = 0x85; break;
        case CanBitrate::BPS_1000K: cnf1 = 0x00; cnf2 = 0x80; cnf3 = 0x80; break;
        case CanBitrate::BPS_500K:
        default:                    cnf1 = 0x00; cnf2 = 0x90; cnf3 = 0x82; break;
    }

    writeRegister(REG_CNF1, cnf1);
    writeRegister(REG_CNF2, cnf2);
    writeRegister(REG_CNF3, cnf3);

    // Interrupciones habilitadas en ambos buffers de recepción
    writeRegister(REG_CANINTE, CANINTF_RX0IF | CANINTF_RX1IF);

    // Sin filtros: acepta todas las tramas estándar en RXB0 y RXB1
    writeRegister(REG_RXB0CTRL, 0x60);
    writeRegister(REG_RXB1CTRL, 0x60);
}

bool MCP2515::setNormalMode() {
    modifyRegister(REG_CANCTRL, MODE_MASK, MODE_NORMAL);
    usleep(1000);
    return (readRegister(REG_CANSTAT) & MODE_MASK) == MODE_NORMAL;
}

bool MCP2515::setLoopbackMode() {
    modifyRegister(REG_CANCTRL, MODE_MASK, MODE_LOOPBACK);
    usleep(1000);
    return (readRegister(REG_CANSTAT) & MODE_MASK) == MODE_LOOPBACK;
}

bool MCP2515::setListenOnlyMode() {
    modifyRegister(REG_CANCTRL, MODE_MASK, MODE_LISTENONLY);
    usleep(1000);
    return (readRegister(REG_CANSTAT) & MODE_MASK) == MODE_LISTENONLY;
}

bool MCP2515::sendFrame(const CanFrame &frame) {
    uint8_t status = readStatus();
    if (status & 0x04) {
        // TXB0CTRL.TXREQ activo: el buffer de transmisión sigue ocupado
        return false;
    }

    uint8_t sidh = (uint8_t)(frame.id >> 3);
    uint8_t sidl = (uint8_t)((frame.id & 0x07) << 5);

    writeRegister(REG_TXB0SIDH, sidh);
    writeRegister(REG_TXB0SIDL, sidl);
    writeRegister(REG_TXB0EID8, 0x00);
    writeRegister(REG_TXB0EID0, 0x00);
    writeRegister(REG_TXB0DLC, frame.dlc & 0x0F);

    for (int i = 0; i < frame.dlc && i < 8; i++) {
        writeRegister(REG_TXB0D0 + i, frame.data[i]);
    }

    char rts[1] = { (char)MCP2515_RTS_TX0 };
    bcm2835_spi_transfern(rts, 1);

    return true;
}

bool MCP2515::hasMessage() {
    uint8_t intf = readRegister(REG_CANINTF);
    return (intf & (CANINTF_RX0IF | CANINTF_RX1IF)) != 0;
}

bool MCP2515::interruptPending() {
    // El pin INT del MCP2515 es activo en bajo
    return bcm2835_gpio_lev(_pinInt) == LOW;
}

void MCP2515::clearRxInterrupt(uint8_t flag) {
    modifyRegister(REG_CANINTF, flag, 0x00);
}

bool MCP2515::readFrame(CanFrame &frame) {
    uint8_t intf = readRegister(REG_CANINTF);
    uint8_t base;
    uint8_t clearFlag;

    if (intf & CANINTF_RX0IF) {
        base = REG_RXB0SIDH;
        clearFlag = CANINTF_RX0IF;
    } else if (intf & CANINTF_RX1IF) {
        base = REG_RXB1SIDH;
        clearFlag = CANINTF_RX1IF;
    } else {
        return false;
    }

    uint8_t buf[13];
    readRegisters(base, buf, 13);

    uint8_t sidh = buf[0];
    uint8_t sidl = buf[1];

    frame.id = ((uint32_t)sidh << 3) | (sidl >> 5);
    frame.extended = (sidl & 0x08) != 0;
    frame.dlc = buf[4] & 0x0F;
    if (frame.dlc > 8) frame.dlc = 8;
    frame.rtr = (buf[4] & 0x40) != 0;

    for (int i = 0; i < frame.dlc; i++) {
        frame.data[i] = buf[5 + i];
    }

    clearRxInterrupt(clearFlag);
    return true;
}
