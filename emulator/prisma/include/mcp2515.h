#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>

#include <bcm2835.h>

// ---------------------------------------------------------------------------
//  Configuración de hardware
//
//  Bus SPI0 de la Raspberry Pi conectado al MCP2515:
//    MISO (GPIO9)  -> SO   del MCP2515
//    MOSI (GPIO10) -> SI   del MCP2515
//    SCLK (GPIO11) -> SCK  del MCP2515
//    CE0  (GPIO8)  -> CS   del MCP2515 (chip select, activo en bajo)
//    GPIO25        -> INT  del MCP2515 (interrupción, activo en bajo)
// ---------------------------------------------------------------------------
#define MCP2515_INT_PIN 25   // GPIO25 -> INT

// Frecuencia del cristal del módulo MCP2515 (8 MHz o 16 MHz según el módulo).
// El Makefile/README indican cómo verificar la del módulo que se tenga.
#ifndef MCP2515_OSC_HZ
#define MCP2515_OSC_HZ 16000000UL
#endif

// Velocidad del bus CAN. OBD2 sobre ISO 15765-4 usa 500 kbps.
#ifndef CAN_BAUDRATE
#define CAN_BAUDRATE 500000UL
#endif

// ---------------------------------------------------------------------------
//  Trama CAN
// ---------------------------------------------------------------------------
struct CanFrame {
    uint16_t id = 0;        // ID estándar de 11 bits (o 29 bits si extended)
    bool extended = false;
    uint8_t dlc = 0;        // nº de bytes de datos (0-8)
    uint8_t data[8] = {0};
};

// Milisegundos transcurridos desde el arranque del proceso (para timeouts).
uint32_t nowMs();

// ---------------------------------------------------------------------------
//  Driver del MCP2515 sobre SPI (bcm2835)
// ---------------------------------------------------------------------------
class MCP2515 {
public:
    // cs: chip select del bus SPI (BCM2835_SPI_CS0 o BCM2835_SPI_CS1).
    // intPin: GPIO conectado a INT del módulo (activo en bajo).
    // Permite varias instancias (varios módulos) sobre el mismo bus SPI.
    explicit MCP2515(uint8_t cs = BCM2835_SPI_CS0, uint8_t intPin = MCP2515_INT_PIN);

    // Inicializa bcm2835, el SPI y el MCP2515 (modo normal, acepta todo).
    bool begin();
    // Cierra SPI y bcm2835 (solo si esta instancia los inicializó).
    void end();

    // Configura esta instancia sobre un bcm2835 ya inicializado (p. ej.
    // dentro del emulador): NO llama bcm2835_init/spi_begin/close, por lo
    // que un fallo aquí no rompe las demás instancias. Tras usarlo, liberar
    // con endLight() (no con end()).
    bool beginExisting();
    // Marca la instancia como no inicializada sin cerrar bcm2835 (global).
    void endLight();

    // Cambia el modo de operación (útil para autotests del módulo).
    bool setLoopbackMode();   // TX/RX interno, sin salir al bus
    bool setNormalMode();     // modo normal (one-shot)

    // Envía una trama usando TXB0. Bloqueante hasta timeoutMs.
    bool sendMessage(const CanFrame& frame, int timeoutMs = 50);

    // Lee una trama recibida. No bloqueante: devuelve false si no hay nada.
    bool receiveMessage(CanFrame& frame);

    // True si el pin INT (GPIO25) está activo (bajo) = hay interrupción pendiente.
    bool isInterruptPending() const;

    // Registro de estado (comando READ_STATUS 0xA0).
    uint8_t readStatus() const;

    // Estadísticas de errores CAN (diagnóstico).
    uint8_t errorCountTx() const;   // TEC (0x1C): errores de transmisión
    uint8_t errorCountRx() const;   // REC (0x1D): errores de recepción
    uint8_t errorFlags() const;     // EFLG (0x2D): banderas de error

    // Imprime estado de registros (diagnóstico).
    void printInfo() const;

private:
    uint8_t  readRegister(uint8_t addr) const;
    void     readRegisters(uint8_t addr, uint8_t* out, uint8_t n) const;
    void     writeRegister(uint8_t addr, uint8_t val);
    void     bitModify(uint8_t addr, uint8_t mask, uint8_t val);
    void     reset();
    bool     setMode(uint8_t reqop);
    bool     setBitTiming(uint32_t oscHz, uint32_t baud);
    void     spiTransfer(uint8_t* buf, size_t len) const;
    // Configuración común del chip (SPI, GPIO INT, reset, registros).
    // No toca la inicialización global de bcm2835.
    bool     configure();

    uint8_t cs_;        // chip select de esta instancia
    uint8_t intPin_;    // GPIO de INT de esta instancia

    mutable std::mutex spiMtx;   // SPI no es thread-safe: serializa el acceso
    bool initialized = false;
};
