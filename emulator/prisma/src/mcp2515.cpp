#include "mcp2515.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
//  Registros del MCP2515
// ---------------------------------------------------------------------------
namespace reg {
constexpr uint8_t CANSTAT  = 0x0E;
constexpr uint8_t CANCTRL  = 0x0F;
constexpr uint8_t TEC      = 0x1C;
constexpr uint8_t REC      = 0x1D;
constexpr uint8_t CNF3     = 0x28;
constexpr uint8_t CNF2     = 0x29;
constexpr uint8_t CNF1     = 0x2A;
constexpr uint8_t CANINTE  = 0x2B;
constexpr uint8_t CANINTF  = 0x2C;
constexpr uint8_t EFLG     = 0x2D;
constexpr uint8_t TXB0CTRL = 0x30;
constexpr uint8_t TXB0SIDH = 0x31;
constexpr uint8_t TXB0SIDL = 0x32;
constexpr uint8_t TXB0EID8 = 0x33;
constexpr uint8_t TXB0EID0 = 0x34;
constexpr uint8_t TXB0DLC  = 0x35;
constexpr uint8_t TXB0DATA = 0x36;
constexpr uint8_t RXB0CTRL = 0x60;
constexpr uint8_t RXB0SIDH = 0x61;
constexpr uint8_t RXB0SIDL = 0x62;
constexpr uint8_t RXB0EID8 = 0x63;
constexpr uint8_t RXB0EID0 = 0x64;
constexpr uint8_t RXB0DLC  = 0x65;
constexpr uint8_t RXB0DATA = 0x66;
constexpr uint8_t RXB1CTRL = 0x70;
constexpr uint8_t RXB1SIDH = 0x71;
constexpr uint8_t RXB1DLC  = 0x75;
constexpr uint8_t RXB1DATA = 0x76;
} // namespace reg

// ---------------------------------------------------------------------------
//  Comandos SPI del MCP2515
// ---------------------------------------------------------------------------
namespace cmd {
constexpr uint8_t RESET       = 0xC0;
constexpr uint8_t WRITE       = 0x02;
constexpr uint8_t READ        = 0x03;
constexpr uint8_t BITMOD      = 0x05;
constexpr uint8_t RTS_TX0     = 0x81;
constexpr uint8_t READ_STATUS = 0xA0;
} // namespace cmd

// ---------------------------------------------------------------------------
//  Bits y máscaras
// ---------------------------------------------------------------------------
namespace bits {
constexpr uint8_t CANCTRL_REQOP   = 0xE0;
constexpr uint8_t REQOP_CONFIG    = 0x80;
constexpr uint8_t REQOP_NORMAL    = 0x00;
constexpr uint8_t REQOP_LOOPBACK  = 0x40;
constexpr uint8_t CANCTRL_OSM     = 0x08;   // one-shot: sin reintentos automáticos
constexpr uint8_t CANSTAT_OPMOD   = 0xE0;

constexpr uint8_t CANINTF_RX0IF   = 0x01;
constexpr uint8_t CANINTF_RX1IF   = 0x02;
constexpr uint8_t CANINTF_TX0IF   = 0x04;
constexpr uint8_t CANINTE_RX0IE   = 0x01;
constexpr uint8_t CANINTE_RX1IE   = 0x02;

constexpr uint8_t EFLG_RX0OVR     = 0x40;
constexpr uint8_t EFLG_RX1OVR     = 0x80;

constexpr uint8_t TXB_TXREQ       = 0x08;
constexpr uint8_t TXB_TXFAIL      = 0x70;   // ABTF | MLOA | TXERR

// RXM = 11 -> aceptar todos los mensajes (ignora máscaras/filtros).
constexpr uint8_t RXB0CTRL_RXM_ALL = 0x60;
// BUKT1 -> los mensajes de RXB0 rebotan a RXB1 cuando RXB0 está lleno.
constexpr uint8_t RXB0CTRL_BUKT    = 0x04;
} // namespace bits

// ---------------------------------------------------------------------------
//  Bit timing CAN (valores verificados; MCP2515 datasheet / librerías maduras)
//  Oscilador del módulo y velocidad -> registros CNF1/CNF2/CNF3.
// ---------------------------------------------------------------------------
namespace {
struct Timing {
    uint32_t osc;
    uint32_t baud;
    uint8_t  cnf1;
    uint8_t  cnf2;
    uint8_t  cnf3;
};

const Timing kTimings[] = {
    {  8000000UL, 1000000UL, 0x00, 0x80, 0x80 },
    {  8000000UL,  500000UL, 0x00, 0x90, 0x82 },
    {  8000000UL,  250000UL, 0x00, 0xB1, 0x85 },
    {  8000000UL,  125000UL, 0x01, 0xB1, 0x85 },
    { 16000000UL, 1000000UL, 0x00, 0xD0, 0x82 },
    { 16000000UL,  500000UL, 0x00, 0xF0, 0x86 },
    { 16000000UL,  250000UL, 0x41, 0xF1, 0x85 },
    { 16000000UL,  125000UL, 0x03, 0xF0, 0x86 },
    { 20000000UL, 1000000UL, 0x00, 0xD9, 0x82 },
    { 20000000UL,  500000UL, 0x00, 0xFA, 0x87 },
};
} // namespace

// ---------------------------------------------------------------------------
//  Utilidades
// ---------------------------------------------------------------------------
uint32_t nowMs() {
    static const auto start = std::chrono::steady_clock::now();
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start)
            .count());
}

// ---------------------------------------------------------------------------
//  Transacciones SPI
// ---------------------------------------------------------------------------
MCP2515::MCP2515(uint8_t cs, uint8_t intPin) : cs_(cs), intPin_(intPin) {}

void MCP2515::spiTransfer(uint8_t* buf, size_t len) const {
    // Selecciona el CS de esta instancia en cada transacción (permite usar
    // varios MCP2515 sobre el mismo bus SPI, p. ej. CE0 y CE1).
    bcm2835_spi_chipSelect(cs_);
    bcm2835_spi_transfern(reinterpret_cast<char*>(buf),
                          static_cast<uint32_t>(len));
}

uint8_t MCP2515::readRegister(uint8_t addr) const {
    uint8_t buf[3] = { cmd::READ, addr, 0x00 };
    spiTransfer(buf, 3);
    return buf[2];
}

void MCP2515::readRegisters(uint8_t addr, uint8_t* out, uint8_t n) const {
    uint8_t buf[2 + 8];
    buf[0] = cmd::READ;
    buf[1] = addr;
    for (uint8_t i = 0; i < n; ++i) buf[2 + i] = 0x00;
    spiTransfer(buf, static_cast<size_t>(2) + n);
    for (uint8_t i = 0; i < n; ++i) out[i] = buf[2 + i];
}

void MCP2515::writeRegister(uint8_t addr, uint8_t val) {
    uint8_t buf[3] = { cmd::WRITE, addr, val };
    spiTransfer(buf, 3);
}

void MCP2515::bitModify(uint8_t addr, uint8_t mask, uint8_t val) {
    uint8_t buf[4] = { cmd::BITMOD, addr, mask, val };
    spiTransfer(buf, 4);
}

// ---------------------------------------------------------------------------
//  Inicialización
// ---------------------------------------------------------------------------
void MCP2515::reset() {
    uint8_t b = cmd::RESET;
    spiTransfer(&b, 1);
}

bool MCP2515::setMode(uint8_t reqop) {
    // OPMOD solo refleja los bits REQOP (0xE0); OSM no interviene.
    const uint8_t op = reqop & bits::CANCTRL_REQOP;
    bitModify(reg::CANCTRL, bits::CANCTRL_REQOP, op);
    for (int i = 0; i < 200; ++i) {
        if ((readRegister(reg::CANSTAT) & bits::CANSTAT_OPMOD) == op)
            return true;
        bcm2835_delayMicroseconds(100);
    }
    return false;
}

bool MCP2515::setBitTiming(uint32_t oscHz, uint32_t baud) {
    for (const auto& t : kTimings) {
        if (t.osc == oscHz && t.baud == baud) {
            writeRegister(reg::CNF1, t.cnf1);
            writeRegister(reg::CNF2, t.cnf2);
            writeRegister(reg::CNF3, t.cnf3);
            return true;
        }
    }
    return false;
}

bool MCP2515::begin() {
    std::lock_guard<std::mutex> lk(spiMtx);

    if (!bcm2835_init())
        return false;
    if (!bcm2835_spi_begin()) {
        bcm2835_close();
        return false;
    }

    if (!configure()) {
        bcm2835_spi_end();
        bcm2835_close();
        return false;
    }

    initialized = true;
    return true;
}

// bcm2835 ya está inicializado (p. ej. por el emulador): configura esta
// instancia SIN llamar bcm2835_init/spi_begin/close. Un fallo aquí no
// desmapea la memoria que otras instancias (p. ej. el emulador) usan.
bool MCP2515::beginExisting() {
    std::lock_guard<std::mutex> lk(spiMtx);
    if (!configure())
        return false;
    initialized = true;
    return true;
}

// Configuración común: SPI para este CS, GPIO de INT, reset y registros.
// NO toca la inicialización global de bcm2835 (eso es responsabilidad de
// begin()/end() o del proceso que ya la haya hecho).
bool MCP2515::configure() {
    // SPI0 a ~7.8 MHz (divisor 32 del reloj de 250 MHz), modo 0, MSB primero.
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);
    bcm2835_spi_setChipSelectPolarity(cs_, LOW);   // CS activo en bajo

    // GPIO intPin -> INT del MCP2515 (drenaje abierto, activo en bajo).
    bcm2835_gpio_fsel(intPin_, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(intPin_, BCM2835_GPIO_PUD_UP);

    reset();
    bcm2835_delay(10);

    // 0xFF = el MCP2515 no responde por SPI (cableado/CS/voltaje).
    if (readRegister(reg::CANSTAT) == 0xFF)
        return false;

    if (!setMode(bits::REQOP_CONFIG))
        return false;
    if (!setBitTiming(MCP2515_OSC_HZ, CAN_BAUDRATE)) {
        std::fprintf(stderr,
                     "MCP2515: combinación oscilador/baudios no soportada "
                     "(osc=%lu, baud=%lu)\n",
                     (unsigned long)MCP2515_OSC_HZ, (unsigned long)CAN_BAUDRATE);
        return false;
    }

    // Recepción: aceptar todos los mensajes; si RXB0 se llena, rebota a RXB1.
    // Re-habilita interrupciones RX (el RESET del test SPI las deshabilita).
    writeRegister(reg::RXB0CTRL, bits::RXB0CTRL_RXM_ALL | bits::RXB0CTRL_BUKT);
    writeRegister(reg::RXB1CTRL, bits::RXB0CTRL_RXM_ALL);
    writeRegister(reg::CANINTE, bits::CANINTE_RX0IE | bits::CANINTE_RX1IE);
    writeRegister(reg::CANINTF, 0x00);
    bitModify(reg::EFLG, bits::EFLG_RX0OVR | bits::EFLG_RX1OVR, 0x00);

    // Modo normal con one-shot: evita reintentos infinitos si no hay otro nodo.
    if (!setMode(bits::REQOP_NORMAL | bits::CANCTRL_OSM))
        return false;

    return true;
}

void MCP2515::end() {
    std::lock_guard<std::mutex> lk(spiMtx);
    if (initialized) {
        bcm2835_spi_end();
        bcm2835_close();
        initialized = false;
    }
}

void MCP2515::endLight() {
    std::lock_guard<std::mutex> lk(spiMtx);
    initialized = false;
}

bool MCP2515::setLoopbackMode() {
    std::lock_guard<std::mutex> lk(spiMtx);
    if (!initialized) return false;
    return setMode(bits::REQOP_LOOPBACK);
}

bool MCP2515::setNormalMode() {
    std::lock_guard<std::mutex> lk(spiMtx);
    if (!initialized) return false;
    return setMode(bits::REQOP_NORMAL | bits::CANCTRL_OSM);
}

// ---------------------------------------------------------------------------
//  Transmisión (TXB0)
// ---------------------------------------------------------------------------
bool MCP2515::sendMessage(const CanFrame& f, int timeoutMs) {
    std::lock_guard<std::mutex> lk(spiMtx);
    if (!initialized) return false;

    // Esperar a que TXB0 quede libre.
    uint32_t t0 = nowMs();
    while ((readRegister(reg::TXB0CTRL) & bits::TXB_TXREQ) &&
           (nowMs() - t0 < static_cast<uint32_t>(timeoutMs)))
        bcm2835_delayMicroseconds(200);
    if (readRegister(reg::TXB0CTRL) & bits::TXB_TXREQ)
        return false;   // buffer ocupado

    if (!f.extended) {
        writeRegister(reg::TXB0SIDH, static_cast<uint8_t>(f.id >> 3));
        writeRegister(reg::TXB0SIDL, static_cast<uint8_t>((f.id & 0x07) << 5));
        writeRegister(reg::TXB0EID8, 0x00);
        writeRegister(reg::TXB0EID0, 0x00);
    } else {
        writeRegister(reg::TXB0SIDH, static_cast<uint8_t>((f.id >> 21) & 0xFF));
        writeRegister(reg::TXB0SIDL,
                      static_cast<uint8_t>((((f.id >> 18) & 0x07) << 5) | 0x08 |
                                           ((f.id >> 16) & 0x03)));
        writeRegister(reg::TXB0EID8, static_cast<uint8_t>((f.id >> 8) & 0xFF));
        writeRegister(reg::TXB0EID0, static_cast<uint8_t>(f.id & 0xFF));
    }

    writeRegister(reg::TXB0DLC, f.dlc & 0x0F);
    const uint8_t ndata = std::min<uint8_t>(f.dlc, 8);
    for (uint8_t i = 0; i < ndata; ++i)
        writeRegister(reg::TXB0DATA + i, f.data[i]);

    bitModify(reg::CANINTF, bits::CANINTF_TX0IF, 0x00);
    bitModify(reg::TXB0CTRL, bits::TXB_TXREQ, bits::TXB_TXREQ);   // transmitir

    t0 = nowMs();
    while (nowMs() - t0 < static_cast<uint32_t>(timeoutMs)) {
        if (readRegister(reg::CANINTF) & bits::CANINTF_TX0IF) {
            bitModify(reg::CANINTF, bits::CANINTF_TX0IF, 0x00);
            // one-shot: si hubo error (p. ej. sin ACK) queda marcado en TXB0CTRL
            if (readRegister(reg::TXB0CTRL) & bits::TXB_TXFAIL)
                return false;
            return true;
        }
        bcm2835_delayMicroseconds(200);
    }
    return false;
}

// ---------------------------------------------------------------------------
//  Recepción (RXB0 con rollover a RXB1)
// ---------------------------------------------------------------------------
bool MCP2515::receiveMessage(CanFrame& f) {
    std::lock_guard<std::mutex> lk(spiMtx);
    if (!initialized) return false;

    const uint8_t iflag = readRegister(reg::CANINTF);
    if (!(iflag & (bits::CANINTF_RX0IF | bits::CANINTF_RX1IF)))
        return false;

    const bool fromRx0 = (iflag & bits::CANINTF_RX0IF) != 0;
    const uint8_t sidh = fromRx0 ? reg::RXB0SIDH : reg::RXB1SIDH;

    uint8_t hdr[5];
    readRegisters(sidh, hdr, 5);

    f.extended = (hdr[1] & 0x08) != 0;
    if (!f.extended) {
        f.id = (static_cast<uint16_t>(hdr[0]) << 3) | (hdr[1] >> 5);
    } else {
        f.id = (static_cast<uint32_t>(hdr[0]) << 21) |
               (static_cast<uint32_t>(hdr[1] & 0xE0) << 13) |
               (static_cast<uint32_t>(hdr[1] & 0x03) << 16) |
               (static_cast<uint32_t>(hdr[2]) << 8) |
               hdr[3];
    }

    f.dlc = hdr[4] & 0x0F;
    if (f.dlc > 8) f.dlc = 8;
    if (f.dlc > 0) {
        uint8_t data[8];
        readRegisters(sidh + 5, data, f.dlc);
        std::memcpy(f.data, data, f.dlc);
    }

    bitModify(reg::CANINTF, fromRx0 ? bits::CANINTF_RX0IF
                                    : bits::CANINTF_RX1IF, 0x00);
    bitModify(reg::EFLG, bits::EFLG_RX0OVR | bits::EFLG_RX1OVR, 0x00);
    return true;
}

// ---------------------------------------------------------------------------
//  Estado y diagnóstico
// ---------------------------------------------------------------------------
bool MCP2515::isInterruptPending() const {
    return bcm2835_gpio_lev(intPin_) == LOW;
}

uint8_t MCP2515::readStatus() const {
    std::lock_guard<std::mutex> lk(spiMtx);
    uint8_t buf[2] = { cmd::READ_STATUS, 0x00 };
    spiTransfer(buf, 2);
    return buf[1];
}

uint8_t MCP2515::errorCountTx() const {
    std::lock_guard<std::mutex> lk(spiMtx);
    return readRegister(reg::TEC);
}

uint8_t MCP2515::errorCountRx() const {
    std::lock_guard<std::mutex> lk(spiMtx);
    return readRegister(reg::REC);
}

uint8_t MCP2515::errorFlags() const {
    std::lock_guard<std::mutex> lk(spiMtx);
    return readRegister(reg::EFLG);
}

void MCP2515::printInfo() const {
    std::lock_guard<std::mutex> lk(spiMtx);
    std::printf(" MCP2515: CANSTAT=0x%02X  CANINTF=0x%02X  EFLG=0x%02X  "
                "TEC=%u  REC=%u\n",
                readRegister(reg::CANSTAT), readRegister(reg::CANINTF),
                readRegister(reg::EFLG), readRegister(reg::TEC),
                readRegister(reg::REC));
}
