#ifndef MCP2515_MCP2515_H
#define MCP2515_MCP2515_H

#include <cstdint>
#include <cstring>

// ==================== Instrucciones SPI del MCP2515 ====================
#define MCP2515_RESET       0xC0
#define MCP2515_READ        0x03
#define MCP2515_WRITE       0x02
#define MCP2515_READ_STATUS 0xA0
#define MCP2515_RX_STATUS   0xB0
#define MCP2515_BIT_MODIFY  0x05
#define MCP2515_RTS_TX0     0x81
#define MCP2515_RTS_TX1     0x82
#define MCP2515_RTS_TX2     0x84

// ==================== Registros utilizados ====================
#define REG_CANCTRL   0x0F
#define REG_CANSTAT   0x0E
#define REG_CNF1      0x2A
#define REG_CNF2      0x29
#define REG_CNF3      0x28
#define REG_CANINTE   0x2B
#define REG_CANINTF   0x2C
#define REG_EFLG      0x2D
#define REG_RXB0CTRL  0x60
#define REG_RXB1CTRL  0x70
#define REG_RXB0SIDH  0x61
#define REG_RXB1SIDH  0x71
#define REG_TXB0SIDH  0x31
#define REG_TXB0SIDL  0x32
#define REG_TXB0EID8  0x33
#define REG_TXB0EID0  0x34
#define REG_TXB0DLC   0x35
#define REG_TXB0D0    0x36

// ==================== Modos de operación (CANCTRL) ====================
#define MODE_NORMAL     0x00
#define MODE_SLEEP      0x20
#define MODE_LOOPBACK   0x40
#define MODE_LISTENONLY 0x60
#define MODE_CONFIG     0x80
#define MODE_MASK       0xE0

// ==================== Banderas de interrupción (CANINTF) ====================
#define CANINTF_RX0IF 0x01
#define CANINTF_RX1IF 0x02
#define CANINTF_TX0IF 0x04
#define CANINTF_ERRIF 0x20

// Representa una trama CAN estándar (11 bits) de hasta 8 bytes de datos
struct CanFrame {
    uint32_t id;        // Identificador (11 bits estándar)
    uint8_t  dlc;        // Longitud de datos (0-8)
    uint8_t  data[8];
    bool     extended;   // ID extendido (29 bits) - no usado en OBD2 estándar
    bool     rtr;         // Remote Transmission Request

    CanFrame() : id(0), dlc(0), extended(false), rtr(false) {
        memset(data, 0, sizeof(data));
    }
};

enum class CanBitrate {
    BPS_125K,
    BPS_250K,
    BPS_500K,   // Velocidad típica usada por OBD2 en buses CAN de 11 bits
    BPS_1000K
};

// Driver del controlador CAN MCP2515 sobre bus SPI (librería bcm2835)
//
// Conexionado esperado (Raspberry Pi -> MCP2515):
//   MISO   -> SO  (salida de datos del MCP2515)
//   MOSI   -> SI  (entrada de datos del MCP2515)
//   SCLK   -> SCK (reloj SPI)
//   CE0    -> CS  (Chip Select, gestionado por hardware SPI0)
//   GPIO25 -> INT (línea de interrupción, activa en bajo)
class MCP2515 {
public:
    MCP2515(uint8_t pinCS, uint8_t pinInt);
    ~MCP2515();

    // Inicializa bcm2835, el bus SPI y el MCP2515 en el bitrate indicado.
    // Debe ejecutarse con privilegios de root (acceso a /dev/mem).
    bool begin(CanBitrate bitrate = CanBitrate::BPS_500K);

    void reset();
    bool setNormalMode();
    bool setLoopbackMode();
    bool setListenOnlyMode();

    bool sendFrame(const CanFrame &frame);
    bool readFrame(CanFrame &frame);

    bool hasMessage();        // true si hay una trama pendiente (registro CANINTF)
    bool interruptPending();  // true si el pin INT físico está activo (bajo)

    uint8_t readRegister(uint8_t address);
    void writeRegister(uint8_t address, uint8_t value);
    void modifyRegister(uint8_t address, uint8_t mask, uint8_t value);
    void readRegisters(uint8_t address, uint8_t *buffer, uint8_t n);

private:
    uint8_t _pinCS;
    uint8_t _pinInt;

    void setBitrate(CanBitrate bitrate);
    uint8_t readStatus();
    void clearRxInterrupt(uint8_t flag);
};

#endif // MCP2515_MCP2515_H
