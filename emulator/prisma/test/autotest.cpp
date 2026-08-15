// ============================================================================
//  Autotest de comunicación MCP2515 <-> Raspberry Pi.
//
//  Tres pruebas reutilizables:
//    - autotestSpi:      enlace SPI (reset, registros, BIT MODIFY, INT)
//    - autotestLoopback: TX/RX interno del controlador + interrupción
//    - autotestBus:      comunicación CAN real entre DOS módulos MCP2515
//
//  Cableado esperado:
//    SPI: MISO->SO, MOSI->SI, SCLK->SCK, CE0->CS, GPIO25->INT
//    Bus (2 módulos): módulo B con CE1->CS y GPIO24->INT; CANH<->CANH,
//    CANL<->CANL y 120 ohmios de terminación en cada extremo.
// ============================================================================
#include "autotest.h"

#include <chrono>
#include <cstdio>
#include <cstring>

namespace {

// ---------------------------------------------------------------------------
//  Utilidades de salida / espera
// ---------------------------------------------------------------------------
bool result(int& fails, const char* name, bool ok, const char* extra) {
    std::printf(" [%s] %-42s %s\n", ok ? "OK" : "XX", name,
                extra ? extra : "");
    if (!ok) ++fails;
    return ok;
}

void warn(const char* msg) {
    std::printf(" [!] AVISO: %s\n", msg);
}

bool waitInterrupt(MCP2515& can, int timeoutMs) {
    const uint32_t t0 = nowMs();
    while (nowMs() - t0 < static_cast<uint32_t>(timeoutMs)) {
        if (can.isInterruptPending()) return true;
        bcm2835_delay(1);
    }
    return false;
}

bool waitRx(MCP2515& can, CanFrame& out, int timeoutMs) {
    const uint32_t t0 = nowMs();
    while (nowMs() - t0 < static_cast<uint32_t>(timeoutMs)) {
        if (can.receiveMessage(out)) return true;
        bcm2835_delay(1);
    }
    return false;
}

// Descarta las tramas pendientes (p. ej. el posible eco de la propia
// transmisión, que algunos controladores reciben en modo normal).
void drainRx(MCP2515& can) {
    CanFrame f;
    while (can.receiveMessage(f)) {
    }
}

// ---------------------------------------------------------------------------
//  Test loopback: envía una trama y comprueba recepción idéntica + INT
// ---------------------------------------------------------------------------
void loopTxRx(int& fails, MCP2515& can, uint16_t id, uint8_t dlc,
              const uint8_t* data, const char* label) {
    CanFrame tx;
    tx.id = id;
    tx.dlc = dlc;
    std::memcpy(tx.data, data, dlc);

    const bool txOk = can.sendMessage(tx, 100);
    result(fails, label, txOk, txOk ? "(TX enviada)" : "(TX fallo)");
    if (!txOk) return;

    const bool intOk = waitInterrupt(can, 300);
    result(fails, "  INT (GPIO25) se activo al recibir", intOk, "");

    // Leer la trama aunque INT no se haya activado (polling del buffer RX):
    // así el dato se verifica y el buffer no queda lleno (evita RX0OVR en la
    // siguiente trama cuando el pin INT no está cableado o configurado).
    CanFrame rx;
    const bool rxOk = waitRx(can, rx, 500);
    const bool dataOk = rxOk && rx.id == id && rx.dlc == dlc &&
                        std::memcmp(rx.data, data, dlc) == 0;
    char extra[64];
    std::snprintf(extra, sizeof(extra), "(RX id=0x%03X dlc=%u)", rx.id, rx.dlc);
    result(fails, "  Trama recibida identica", dataOk, dataOk ? extra : "");
}

// ---------------------------------------------------------------------------
//  SPI crudo (test_spi)
// ---------------------------------------------------------------------------
enum {
    REG_CANSTAT  = 0x0E,
    REG_CANCTRL  = 0x0F,
    REG_TEC      = 0x1C,
    REG_REC      = 0x1D,
    REG_EFLG     = 0x2D,
    REG_CNF3     = 0x28,
    REG_CNF2     = 0x29,
    REG_CNF1     = 0x2A,
    REG_CANINTF  = 0x2C,
    REG_TXB0SIDH = 0x31,
    REG_TXB0SIDL = 0x32,
    REG_TXB0EID8 = 0x33,
    REG_TXB0EID0 = 0x34,
    REG_TXB0DLC  = 0x35,
    REG_RXB0CTRL = 0x60,
};

enum { CMD_RESET = 0xC0, CMD_READ = 0x03, CMD_WRITE = 0x02, CMD_BITMOD = 0x05 };

uint8_t readReg(uint8_t addr) {
    uint8_t buf[3] = { CMD_READ, addr, 0x00 };
    bcm2835_spi_transfern(reinterpret_cast<char*>(buf), 3);
    return buf[2];
}

void writeReg(uint8_t addr, uint8_t val) {
    uint8_t buf[3] = { CMD_WRITE, addr, val };
    bcm2835_spi_transfern(reinterpret_cast<char*>(buf), 3);
}

void bitMod(uint8_t addr, uint8_t mask, uint8_t val) {
    uint8_t buf[4] = { CMD_BITMOD, addr, mask, val };
    bcm2835_spi_transfern(reinterpret_cast<char*>(buf), 4);
}

// Mide la velocidad real del enlace SPI realizando `iters` transacciones de
// `txnBytes` bytes y reporta el throughput efectivo. Es informativo.
// Devuelve los Mbps efectivos.
double spiBenchmark(const char* label, int txnBytes, int iters) {
    uint8_t buf[16];
    std::memset(buf, 0, sizeof(buf));
    buf[0] = CMD_READ;
    buf[1] = 0x00;

    // Calentamiento (evita el efecto de la primera transacción).
    for (int i = 0; i < 20; ++i)
        bcm2835_spi_transfern(reinterpret_cast<char*>(buf),
                              static_cast<uint32_t>(txnBytes));

    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < iters; ++i)
        bcm2835_spi_transfern(reinterpret_cast<char*>(buf),
                              static_cast<uint32_t>(txnBytes));
    const auto t1 = std::chrono::steady_clock::now();
    const double secs = std::chrono::duration<double>(t1 - t0).count();
    const double kBs =
        (static_cast<double>(iters) * txnBytes) / secs / 1000.0;
    const double mbps = kBs * 8.0 / 1000.0;
    const double usPer = secs / iters * 1e6;
    std::printf("  %-30s %7.1f kB/s  (%5.2f Mbps efectivos, %6.2f us/transaccion)\n",
                label, kBs, mbps, usPer);
    return mbps;
}

// Lee y muestra TEC/REC/EFLG de un nodo. No hace fallar la prueba: los
// checks de tramas definen el resultado; los contadores son diagnósticos.
void reportCanErrors(MCP2515& can, const char* nodeName) {
    const uint8_t tec = can.errorCountTx();
    const uint8_t rec = can.errorCountRx();
    const uint8_t eflg = can.errorFlags();
    char extra[48];
    std::snprintf(extra, sizeof(extra), "TEC=%u REC=%u EFLG=0x%02X",
                  tec, rec, eflg);
    const bool clean = (tec == 0 && rec == 0 && (eflg & 0xE0) == 0);
    std::printf(" [%s] %-42s %s\n", clean ? "OK" : "XX", nodeName, extra);
    if (!clean) {
        std::printf(" [!] %s: contadores de error elevados (TEC/REC>0 o EFLG).\n",
                    nodeName);
        std::printf("     Revise terminacion (120 ohmios por extremo), CANH/CANL\n");
        std::printf("     y que el bitrate/cristal coincidan en ambos nodos.\n");
    }
}

// ---------------------------------------------------------------------------
//  Sonda del pin RESET del MCP2515 (pin 11) en AUTOTEST_RESET_GPIO.
//  Devuelve:
//    1  -> RESET activamente en ALTO (el módulo lo mantiene en alto)
//    0  -> RESET activamente en BAJO  -> chip EN RESET
//   -1  -> flotando / no cableado (no se puede concluir)
//  La detección de flotado usa los pull-up/down de la Pi: si el pin no está
//  cableado o el módulo no lo maneja, el nivel cambia según el pull.
// ---------------------------------------------------------------------------
int probeResetPin() {
    bcm2835_gpio_fsel(AUTOTEST_RESET_GPIO, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(AUTOTEST_RESET_GPIO, BCM2835_GPIO_PUD_OFF);
    bcm2835_delay(2);

    // Con pull-down: si el módulo mantiene RESET en alto, sigue en alto.
    bcm2835_gpio_set_pud(AUTOTEST_RESET_GPIO, BCM2835_GPIO_PUD_DOWN);
    bcm2835_delay(2);
    const int withDown = bcm2835_gpio_lev(AUTOTEST_RESET_GPIO);

    // Con pull-up: si el módulo mantiene RESET en bajo, sigue en bajo.
    bcm2835_gpio_set_pud(AUTOTEST_RESET_GPIO, BCM2835_GPIO_PUD_UP);
    bcm2835_delay(2);
    const int withUp = bcm2835_gpio_lev(AUTOTEST_RESET_GPIO);

    bcm2835_gpio_set_pud(AUTOTEST_RESET_GPIO, BCM2835_GPIO_PUD_OFF);

    if (withDown == HIGH) return 1;    // lo gana el nivel alto del módulo
    if (withUp == LOW)    return 0;    // lo gana el nivel bajo del módulo
    return -1;                         // flotando / no cableado
}

// Imprime el diagnóstico del fallo del enlace SPI, distinguiendo:
//   - chip ausente / no responde por SPI (lectura 0xFF)
//   - chip sin alimentación o en reset (patrón 0x00, escrituras que no
//     persisten) — resuelto con la sonda opcional del pin RESET y CLKOUT.
// clkoutHz: frecuencia medida de CLKOUT; pasar -1 si no se midió.
void printFailureDiagnosis(uint8_t canstat, double clkoutHz) {
    std::printf("--------------------------------------------------------------\n");
    std::printf(" DIAGNOSTICO DEL FALLO (CANSTAT=0x%02X tras reset)\n", canstat);

    if (canstat == 0xFF) {
        std::printf(" El chip NO responde por SPI (lectura 0xFF).\n");
        std::printf(" Revise: CE0->CS, MOSI->SI, SCLK->SCK, alimentacion VCC/GND\n");
        std::printf(" y que no haya otro dispositivo ocupando CE0.\n");
    } else if (canstat == 0x80) {
        std::printf(" El reset SI funciona (CANSTAT=0x80) pero las escrituras no\n");
        std::printf(" persisten: revise los niveles de MISO (3.3V/5V), la soldadura\n");
        std::printf(" del modulo o pruebe con otro MCP2515.\n");
    } else {
        std::printf(" Patron 0x00: el chip no procesa (sin alimentacion, en reset\n");
        std::printf(" o sin oscilador). Causas en orden de probabilidad:\n");
        std::printf("   1) Modulo sin alimentacion (VCC 5V/GND) o MCP2515 danado.\n");
        std::printf("   2) Pin RESET del MCP2515 a bajo o flotando.\n");
        std::printf("   3) Cristal OSC1/OSC2 defectuoso o sin oscilar.\n");
    }

    // Sonda RESET (opcional): distingue "en reset" de "sin alimentacion".
    const int rst = probeResetPin();
    std::printf(" [%s] %-42s\n", rst == 1 ? "OK" : (rst == 0 ? "XX" : "--"),
                "Pin RESET (puente opcional)");
    if (rst == 1) {
        std::printf("     RESET en ALTO: correcto. Si el chip sigue sin responder,\n");
        std::printf("     mida VCC/GND (5V) con multimetro o pruebe otro modulo.\n");
    } else if (rst == 0) {
        std::printf("     RESET en BAJO: el chip esta EN RESET. Conecte el pin\n");
        std::printf("     RESET (11) a 3,3V o VCC (con pull-up de 10k) y repita.\n");
    } else {
        std::printf("     RESET flotando o sin puente: conecte RESET (pin 11) al\n");
        std::printf("     GPIO %d (con divisor 5V->3.3V si el modulo es de 5V)\n",
                    AUTOTEST_RESET_GPIO);
        std::printf("     para distinguir 'en reset' de 'sin alimentacion'.\n");
    }

    // Oscilador: solo si se midió CLKOUT (detección de cristal ya ejecutada).
    if (clkoutHz >= 0.0) {
        std::printf(" [%s] %-42s\n", clkoutHz > 0.0 ? "OK" : "--",
                    "Oscilador (CLKOUT)");
        if (clkoutHz <= 0.0) {
            std::printf("     CLKOUT a 0 Hz: el oscilador no arranca. Con RESET OK\n");
            std::printf("     y 5V presentes, el cristal o el MCP2515 estan mal.\n");
        }
    }
}

} // namespace

// ---------------------------------------------------------------------------
//  Detección del cristal del módulo (8 vs 16 MHz) vía CLKOUT
//
//  CLKOUT (pin 3 del MCP2515) emite Fosc dividido por un prescaler
//  configurable en CANCTRL (bits 1:0 CLKPRE: 00=/1, 01=/2, 10=/4, 11=/8;
//  bit 2 CLKEN habilita la salida). Tras el reset el chip queda con
//  CLKEN=1 y CLKPRE=11 (Fosc/8), por lo que:
//     cristal 16 MHz -> CLKOUT = 2 MHz
//     cristal  8 MHz -> CLKOUT = 1 MHz
//  Ambas frecuencias son fáciles de medir por polling en un GPIO sin
//  riesgo de aliasing. La detección es informativa: no hace fallar la
//  prueba, pero avisa si el cristal medido no coincide con el valor con
//  el que se compiló el driver (MCP2515_OSC_HZ), que es justo el fallo
//  que deja el bus CAN a la mitad del bitrate.
// ---------------------------------------------------------------------------
double measureClkoutFreq() {
    // GPIO como entrada, sin pull (CLKOUT es una salida del MCP2515).
    // PUD_DOWN: si el pin no está cableado, lee 0 estable -> "no detectado".
    bcm2835_gpio_fsel(AUTOTEST_CLKOUT_GPIO, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(AUTOTEST_CLKOUT_GPIO, BCM2835_GPIO_PUD_DOWN);

    // Asegurar CLKEN=1, CLKPRE=11 (Fosc/8). Es el valor de reset, pero lo
    // fijamos explícitamente por si un uso anterior lo dejó distinto.
    // Máscara 0x07 = bits 2:0 (CLKEN + CLKPRE); valor 0x07 = 11,1.
    bitMod(REG_CANCTRL, 0x07, 0x07);
    bcm2835_delay(10);   // estabilización del oscilador / salida

    const auto t0 = std::chrono::steady_clock::now();
    unsigned long transitions = 0;
    int prev = bcm2835_gpio_lev(AUTOTEST_CLKOUT_GPIO);
    double elapsed = 0.0;
    while (elapsed < 0.20) {        // ventana de medición de ~200 ms
        for (int i = 0; i < 256; ++i) {
            const int cur = bcm2835_gpio_lev(AUTOTEST_CLKOUT_GPIO);
            if (cur != prev) {
                ++transitions;
                prev = cur;
            }
        }
        elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t0).count();
    }

    // Cada ciclo completo = 2 transiciones -> f = transiciones / (2 * t).
    return elapsed > 0.0
               ? static_cast<double>(transitions) / (2.0 * elapsed)
               : 0.0;
}

uint32_t detectCrystalHz(double clkoutHz) {
    // CLKOUT = Fosc/8 (CLKPRE=11) -> Fosc estimado = 8 * CLKOUT.
    const double est = 8.0 * clkoutHz;
    if (est < 3.0e6)                // sin señal (no cableado / chip muerto)
        return 0;
    return est < 12.0e6 ? 8000000UL : 16000000UL;
}

// ---------------------------------------------------------------------------
//  autotestSpi
// ---------------------------------------------------------------------------
bool autotestSpi(bool ownsBcm2835) {
    std::printf("--------------------------------------------------------------\n");
    std::printf(" Test SPI: MCP2515 <-> Raspberry Pi\n");
    std::printf(" Pines: CE0->CS  MISO->SO  MOSI->SI  SCLK->SCK  GPIO25->INT\n");
    std::printf("--------------------------------------------------------------\n");

    if (ownsBcm2835) {
        if (!bcm2835_init()) {
            std::printf("ERROR: bcm2835_init() falló (¿ejecuta con sudo?).\n");
            return false;
        }
        if (!bcm2835_spi_begin()) {
            std::printf("ERROR: bcm2835_spi_begin() falló.\n");
            bcm2835_close();
            return false;
        }
    }

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    bcm2835_gpio_fsel(MCP2515_INT_PIN, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(MCP2515_INT_PIN, BCM2835_GPIO_PUD_UP);

    int fails = 0;

    // 1) Reset: tras reset el MCP2515 queda en modo config (CANSTAT=0x80)
    {
        uint8_t rst = CMD_RESET;
        bcm2835_spi_transfern(reinterpret_cast<char*>(&rst), 1);
        bcm2835_delay(10);
    }
    const uint8_t canstat = readReg(REG_CANSTAT);
    char extra[32];
    std::snprintf(extra, sizeof(extra), "(CANSTAT=0x%02X)", canstat);
    result(fails, "Reset: CANSTAT = 0x80 (modo config)", canstat == 0x80,
           extra);

    if (canstat == 0xFF) {
        // Chip ausente / no responde: diagnóstico con la sonda RESET antes
        // de cerrar bcm2835 (usa GPIO).
        printFailureDiagnosis(canstat, -1.0);
        if (ownsBcm2835) {
            bcm2835_spi_end();
            bcm2835_close();
        }
        return false;
    }

    // Tras el reset los contadores de error deben estar a cero.
    const uint8_t tec0 = readReg(REG_TEC);
    const uint8_t rec0 = readReg(REG_REC);
    const uint8_t eflg0 = readReg(REG_EFLG);
    char eflgExtra[40];
    std::snprintf(eflgExtra, sizeof(eflgExtra), "(TEC=%u REC=%u EFLG=0x%02X)",
                  tec0, rec0, eflg0);
    result(fails, "Contadores de error tras reset (TEC/REC/EFLG)",
           tec0 == 0 && rec0 == 0 && eflg0 == 0, eflgExtra);

    // 1b) Detección del cristal (8 vs 16 MHz). Informativo: mide CLKOUT
    //     (pin 3) en AUTOTEST_CLKOUT_GPIO y avisa si no coincide con
    //     MCP2515_OSC_HZ (include/mcp2515.h). El valor medido también lo
    //     usa el diagnóstico de fallo (¿oscila el cristal?).
    double clkoutHz = 0.0;
    {
        clkoutHz = measureClkoutFreq();
        const uint32_t xtal = detectCrystalHz(clkoutHz);
        if (xtal == 0) {
            std::printf(" [--] %-42s %s\n", "Cristal: CLKOUT no detectado",
                        "(0 Hz)");
            std::printf(" [!] Conecte CLKOUT (pin 3 del MCP2515) al GPIO %d para\n",
                        AUTOTEST_CLKOUT_GPIO);
            std::printf("     medir el cristal (con divisor 5V->3.3V si el modulo\n");
            std::printf("     es de 5V; nunca 5V directos a un GPIO de la Pi).\n");
            std::printf("     Sin senal tambien puede indicar cristal o alimentacion\n");
            std::printf("     defectuosos, o escrituras que no persisten (chip muerto).\n");
        } else {
            char xtra[96];
            std::snprintf(xtra, sizeof(xtra),
                          "(CLKOUT=%.2f MHz -> cristal %lu MHz)",
                          clkoutHz / 1e6, (unsigned long)(xtal / 1000000UL));
            std::printf(" [OK] %-42s %s\n", "Deteccion de cristal (8 vs 16 MHz)",
                        xtra);
            if (xtal != MCP2515_OSC_HZ) {
                std::printf(" [!] ATENCION: el modulo tiene cristal de %lu MHz pero el\n",
                            (unsigned long)(xtal / 1000000UL));
                std::printf("     codigo compila con MCP2515_OSC_HZ=%lu MHz\n",
                            (unsigned long)(MCP2515_OSC_HZ / 1000000UL));
                std::printf("     (include/mcp2515.h). Con ese desajuste el bus CAN\n");
                std::printf("     queda a la mitad del bitrate (250 kbps en vez de\n");
                std::printf("     500 kbps). Corrija el valor y recompile.\n");
            }
        }
    }

    // 2) Escritura / lectura de registros R/W (buffers TX)
    // Nota: TXBnSIDL tiene los bits 3 y 1 U-0 (unimplemented, leen '0');
    // por eso escribir 0xA5 se lee de vuelta como 0xA1 (ver datasheet
    // Register 3-4). El resto de registros del buffer TX son R/W completos.
    writeReg(REG_TXB0SIDH, 0x5A);
    writeReg(REG_TXB0SIDL, 0xA5);
    writeReg(REG_TXB0EID8, 0x3C);
    writeReg(REG_TXB0EID0, 0xC3);
    const bool rwOk = readReg(REG_TXB0SIDH) == 0x5A &&
                      readReg(REG_TXB0SIDL) == 0xA1 &&
                      readReg(REG_TXB0EID8) == 0x3C &&
                      readReg(REG_TXB0EID0) == 0xC3;
    result(fails, "Escritura/lectura de registros (TXB0)", rwOk,
           "(0x5A/A5->A1/3C/C3)");

    // 3) BIT MODIFY: se prueba en CANINTF (registro bit-modificable; el
    //    buffer TXBnDLC NO lo es: por datasheet, BIT MODIFY sobre un registro
    //    no bit-modificable fuerza la mascara a 0xFF, sobrescribiendo el
    //    registro completo con el byte de datos).
    writeReg(REG_CANINTF, 0xFF);
    bitMod(REG_CANINTF, 0x01, 0x00);
    const bool bmOk = readReg(REG_CANINTF) == 0xFE;
    writeReg(REG_CANINTF, 0x00);
    result(fails, "BIT MODIFY (limpiar bit 0 en CANINTF)", bmOk,
           "(0xFF -> 0xFE)");

    // 4) CNF1/2/3 escribibles (solo en modo config) y lectura coherente
    writeReg(REG_CNF1, 0x00);
    writeReg(REG_CNF2, 0xF0);
    writeReg(REG_CNF3, 0x86);
    const bool cnfOk = readReg(REG_CNF1) == 0x00 &&
                       readReg(REG_CNF2) == 0xF0 &&
                       readReg(REG_CNF3) == 0x86;
    result(fails, "Registros de bit timing CNF1/2/3", cnfOk, "(0x00/F0/86)");

    // 5) RXB0CTRL: aceptar todo + rollover a RXB1. El bit 1 (BUKT1) es una
    //    copia read-only del bit 2 (BUKT); con BUKT=1 la lectura devuelve
    //    0x66 y no 0x64 (datasheet Register 4-1).
    writeReg(REG_RXB0CTRL, 0x64);
    const bool rxbOk = readReg(REG_RXB0CTRL) == 0x66;
    result(fails, "RXB0CTRL (aceptar todo + rollover)", rxbOk, "(0x64 -> 0x66)");

    // 6) Pin INT en reposo (sin interrupciones -> nivel alto)
    const bool intOk = bcm2835_gpio_lev(MCP2515_INT_PIN) == HIGH;
    result(fails, "Pin INT (GPIO25) en alto sin trafico", intOk, "");

    // 7) Velocidad real del bus SPI (informativo)
    std::printf("\n Velocidad real del bus SPI (reloj configurado ~7.8 MHz, div 32):\n");
    const double spd3 = spiBenchmark("lectura registro (3 B)", 3, 2000);
    const double spd8 = spiBenchmark("lectura bloque (8 B)", 8, 2000);
    const double spd16 = spiBenchmark("lectura bloque (16 B)", 16, 2000);
    if (spd3 < 1.0 || spd8 < 1.0 || spd16 < 1.0)
        warn("Throughput efectivo muy bajo (<1 Mbps): revise cableado/velocidad SPI");

    std::printf("--------------------------------------------------------------\n");
    if (fails == 0) {
        if (ownsBcm2835) {
            bcm2835_spi_end();
            bcm2835_close();
        }
        std::printf(" RESULTADO: SPI OK - el enlace MCP2515 <-> Raspberry Pi funciona.\n");
        return true;
    }

    // Fallo a nivel de chip (reset/RW/bit timing): diagnóstico que distingue
    // "sin alimentación" de "en reset" (sonda RESET opcional + CLKOUT).
    // Se ejecuta antes de cerrar bcm2835 (usa GPIO). Un fallo solo de INT no
    // dispara este diagnóstico (el chip funciona; es el cableado de GPIO25).
    const bool chipLevelFails =
        canstat != 0x80 || !rwOk || !bmOk || !cnfOk || !rxbOk;
    if (chipLevelFails)
        printFailureDiagnosis(canstat, clkoutHz);

    if (ownsBcm2835) {
        bcm2835_spi_end();
        bcm2835_close();
    }

    std::printf("--------------------------------------------------------------\n");
    std::printf(" RESULTADO: %d comprobacion(es) FALLARON.\n", fails);
    std::printf(" Si solo fallan las de INT, revise la conexion GPIO25->INT.\n");
    return false;
}

// ---------------------------------------------------------------------------
//  autotestLoopback
// ---------------------------------------------------------------------------
bool autotestLoopback(MCP2515& can) {
    std::printf("--------------------------------------------------------------\n");
    std::printf(" Test Loopback: MCP2515 TX/RX interno + interrupcion GPIO25\n");
    std::printf("--------------------------------------------------------------\n");

    int fails = 0;
    result(fails, "Inicializacion SPI / MCP2515", true, "");

    const bool lbOk = can.setLoopbackMode();
    result(fails, "Modo loopback interno", lbOk, "");
    if (lbOk) {
        static const uint8_t d1[8] =
            { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04 };
        loopTxRx(fails, can, 0x123, 8, d1, "TX/RX trama 0x123 (8 bytes)");

        static const uint8_t d2[2] = { 0x02, 0x01 };
        loopTxRx(fails, can, 0x7DF, 2, d2, "TX/RX trama 0x7DF (2 bytes)");
    }

    reportCanErrors(can, "Errores CAN tras loopback (TEC/REC/EFLG)");

    can.setNormalMode();   // restaurar siempre antes de volver

    std::printf("--------------------------------------------------------------\n");
    if (fails == 0) {
        std::printf(" RESULTADO: LOOPBACK OK - el MCP2515 transmite y recibe.\n");
        return true;
    }
    std::printf(" RESULTADO: %d comprobacion(es) FALLARON.\n", fails);
    return false;
}

// ---------------------------------------------------------------------------
//  autotestBus
// ---------------------------------------------------------------------------
bool autotestBus(bool ownsBcm2835) {
    const uint8_t NODE_A_CS = BCM2835_SPI_CS0;
    const uint8_t NODE_A_INT = 25;
    const uint8_t NODE_B_CS = BCM2835_SPI_CS1;
    const uint8_t NODE_B_INT = 24;

    std::printf("--------------------------------------------------------------\n");
    std::printf(" Test BUS: dos MCP2515 comunicandose por CAN\n");
    std::printf(" SPI compartido: MISO->SO MOSI->SI SCLK->SCK (ambos modulos)\n");
    std::printf(" A: CE0->CS GPIO25->INT   |   B: CE1->CS GPIO24->INT\n");
    std::printf(" CANH_A<->CANH_B  CANL_A<->CANL_B  (120 ohmios por extremo)\n");
    std::printf("--------------------------------------------------------------\n");

    // Solo nodeA es dueño de la inicialización global de bcm2835: nodeA
    // usa begin()/end() y nodeB usa beginExisting()/endLight(). Si ambos
    // usaran begin()/end() el segundo end() volvería a llamar
    // bcm2835_spi_end() sobre un mapa ya desmapeado -> segfault.
    MCP2515 nodeA(NODE_A_CS, NODE_A_INT);
    MCP2515 nodeB(NODE_B_CS, NODE_B_INT);

    const bool aOk = ownsBcm2835 ? nodeA.begin() : nodeA.beginExisting();
    if (!aOk) {
        std::printf("\n ERROR: no se pudo inicializar el modulo A (CE0).\n");
        std::printf(" Revise CE0->CS, alimentacion y que CE0 no este compartido.\n");
        return false;
    }
    const bool bOk = nodeB.beginExisting();
    if (!bOk) {
        if (ownsBcm2835) nodeA.end(); else nodeA.endLight();
        std::printf("\n ERROR: no se pudo inicializar el modulo B (CE1).\n");
        std::printf(" Revise CE1->CS y la alimentacion del modulo B.\n");
        std::printf(" (La prueba de bus requiere un segundo modulo MCP2515.)\n");
        return false;
    }

    int fails = 0;
    result(fails, "Inicializacion de ambos MCP2515 (CE0 y CE1)", true, "");

    // 1) Nodo A -> Nodo B
    {
        static const uint8_t d[8] =
            { 0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x22, 0x33, 0x44 };
        CanFrame t;
        t.id = 0x321;
        t.dlc = 8;
        std::memcpy(t.data, d, 8);

        const bool ok = nodeA.sendMessage(t, 100);
        result(fails, "Nodo A transmite 0x321 (8 bytes)", ok,
               ok ? "" : "(TX fallo)");
        if (ok) {
            if (waitInterrupt(nodeB, 300))
                result(fails, "INT del Nodo B (GPIO24) se activo", true, "");
            else
                warn("INT del Nodo B no se activo: revise GPIO24->INT "
                     "(el bus puede funcionar igual)");

            drainRx(nodeA);
            CanFrame rx;
            const bool rxOk = waitRx(nodeB, rx, 500);
            const bool dataOk = rxOk && rx.id == 0x321 && rx.dlc == 8 &&
                                std::memcmp(rx.data, d, 8) == 0;
            char extra[64];
            std::snprintf(extra, sizeof(extra), "(RX id=0x%03X dlc=%u)",
                          rx.id, rx.dlc);
            result(fails, "Nodo B recibe la trama identica", dataOk,
                   dataOk ? extra : "");
        }
    }

    // 2) Nodo B -> Nodo A
    {
        drainRx(nodeA);
        drainRx(nodeB);
        static const uint8_t d[8] =
            { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 };
        CanFrame t;
        t.id = 0x456;
        t.dlc = 8;
        std::memcpy(t.data, d, 8);

        const bool ok = nodeB.sendMessage(t, 100);
        result(fails, "Nodo B transmite 0x456 (8 bytes)", ok,
               ok ? "" : "(TX fallo)");
        if (ok) {
            if (waitInterrupt(nodeA, 300))
                result(fails, "INT del Nodo A (GPIO25) se activo", true, "");
            else
                warn("INT del Nodo A no se activo: revise GPIO25->INT "
                     "(el bus puede funcionar igual)");

            drainRx(nodeB);
            CanFrame rx;
            const bool rxOk = waitRx(nodeA, rx, 500);
            const bool dataOk = rxOk && rx.id == 0x456 && rx.dlc == 8 &&
                                std::memcmp(rx.data, d, 8) == 0;
            char extra[64];
            std::snprintf(extra, sizeof(extra), "(RX id=0x%03X dlc=%u)",
                          rx.id, rx.dlc);
            result(fails, "Nodo A recibe la trama identica", dataOk,
                   dataOk ? extra : "");
        }
    }

    // 3) Intercambio estilo OBD2 (0x7DF -> 0x7E8)
    {
        drainRx(nodeA);
        drainRx(nodeB);
        CanFrame req;
        req.id = 0x7DF;
        req.dlc = 3;
        req.data[0] = 0x02;   // PCI: 2 bytes de datos
        req.data[1] = 0x01;   // modo 01
        req.data[2] = 0x0C;   // PID: RPM

        const bool ok = nodeA.sendMessage(req, 100);
        CanFrame r;
        const bool reqOk = ok && waitRx(nodeB, r, 500) &&
                           r.id == 0x7DF && r.dlc == 3 &&
                           r.data[0] == 0x02 && r.data[1] == 0x01 &&
                           r.data[2] == 0x0C;
        result(fails, "Peticion OBD2 A->B (0x7DF, modo 01 PID 0C)", reqOk, "");
        if (reqOk) {
            drainRx(nodeA);

            static const uint8_t d[5] = { 0x04, 0x41, 0x0C, 0x1A, 0xF0 };
            CanFrame resp;
            resp.id = 0x7E8;
            resp.dlc = 5;
            std::memcpy(resp.data, d, 5);
            nodeB.sendMessage(resp, 100);

            drainRx(nodeB);
            CanFrame a;
            const bool respOk = waitRx(nodeA, a, 500) &&
                                a.id == 0x7E8 && a.dlc == 5 &&
                                std::memcmp(a.data, d, 5) == 0;
            result(fails, "Respuesta OBD2 B->A (0x7E8)", respOk, "");
        }
    }

    // 4) Rafaga de 100 tramas A->B con contador
    {
        drainRx(nodeA);
        drainRx(nodeB);
        bool burstOk = true;
        for (int i = 0; i < 100 && burstOk; ++i) {
            CanFrame t;
            t.id = 0x500;
            t.dlc = 8;
            for (int j = 0; j < 8; ++j) t.data[j] = 0xA5;
            t.data[0] = static_cast<uint8_t>(i);

            burstOk = nodeA.sendMessage(t, 100);
            drainRx(nodeA);
            CanFrame r;
            burstOk = burstOk && waitRx(nodeB, r, 300) &&
                      r.id == 0x500 && r.data[0] == static_cast<uint8_t>(i);
        }
        result(fails, "Rafaga de 100 tramas A->B con contador", burstOk,
               burstOk ? "(secuencia completa)" : "(se perdieron tramas)");
    }

    // Estadísticas de errores CAN por nodo (TEC/REC/EFLG)
    reportCanErrors(nodeA, "Errores CAN nodo A (TEC/REC/EFLG)");
    reportCanErrors(nodeB, "Errores CAN nodo B (TEC/REC/EFLG)");

    if (ownsBcm2835) {
        nodeA.end();        // cierra bcm2835/SPI (dueño global)
        nodeB.endLight();   // solo marca no inicializado (no cerrar dos veces)
    } else {
        nodeA.endLight();
        nodeB.endLight();
    }

    std::printf("--------------------------------------------------------------\n");
    if (fails == 0) {
        std::printf(" RESULTADO: BUS OK - los dos MCP2515 se comunican por CAN.\n");
        return true;
    }
    std::printf(" RESULTADO: %d comprobacion(es) FALLARON.\n", fails);
    std::printf(" Revise CANH/CANL y la terminacion de 120 ohmios.\n");
    return false;
}

// ---------------------------------------------------------------------------
//  autotestRun
// ---------------------------------------------------------------------------
bool autotestRun(MCP2515& can, bool ownsBcm2835) {
    std::printf("\n========== AUTOTEST DE COMUNICACIÓN ==========\n");
    const bool spi = autotestSpi(ownsBcm2835);

    // El test SPI hace RESET del MCP2515, lo que deshabilita CANINTE y deja
    // el chip en modo config. Reconfigurar antes del loopback para que la
    // interrupción RX se vuelva a habilitar (sin esto el INT nunca se activa
    // y el buffer RX se llena -> RX0OVR).
    if (!can.beginExisting())
        std::printf(" AVISO: falló la reconfiguración del MCP2515 tras el test SPI.\n");

    const bool loop = autotestLoopback(can);
    const bool bus = autotestBus(ownsBcm2835);

    std::printf("\n========== RESUMEN AUTOTEST ==========\n");
    std::printf(" SPI      : %s\n", spi ? "OK" : "FALLO");
    std::printf(" Loopback : %s\n", loop ? "OK" : "FALLO");
    std::printf(" Bus      : %s\n", bus ? "OK" : "FALLO");
    return spi && loop && bus;
}
