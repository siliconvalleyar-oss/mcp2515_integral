#pragma once

#include "mcp2515.h"

// ============================================================================
//  Autotest de comunicación MCP2515 <-> Raspberry Pi.
//
//  Lógica compartida entre los binarios de prueba (obj/test_spi,
//  obj/test_loopback, obj/test_bus) y el menú del emulador (opción 8).
//
//  ownsBcm2835: true  -> la prueba inicializa y cierra bcm2835 por sí misma
//                        (binarios independientes).
//                false -> bcm2835 ya está inicializado (emulador): no se
//                        inicializa ni se cierra al terminar.
// ============================================================================

// GPIO conectado a CLKOUT (pin 3 del MCP2515) para medir el cristal del
// módulo. OPCIONAL: haga un puente CLKOUT -> este GPIO. Si el módulo
// trabaja a 5 V, use un divisor resistivo (p. ej. 1 kOhm en serie + 2 kOhm
// a GND) o un level shifter: NUNCA aplique 5 V a un GPIO de la Pi.
// Sin el puente, la detección de cristal se omite con un aviso.
#ifndef AUTOTEST_CLKOUT_GPIO
#define AUTOTEST_CLKOUT_GPIO 26
#endif

// GPIO conectado al pin RESET (pin 11) del MCP2515, usado por el diagnóstico
// de fallo para distinguir "chip sin alimentación" de "chip en reset".
// OPCIONAL: haga un puente RESET -> este GPIO (con divisor 5V->3.3V si el
// módulo es de 5V). Sin el puente, el diagnóstico indica cómo medirlo a mano.
#ifndef AUTOTEST_RESET_GPIO
#define AUTOTEST_RESET_GPIO 27
#endif

// Mide la frecuencia de CLKOUT (Fosc/8, CLKPRE=11) en AUTOTEST_CLKOUT_GPIO
// durante ~200 ms. Requiere bcm2835 inicializado y el MCP2515 respondiendo
// por SPI. Devuelve Hz (0 si no hay señal en el pin).
double measureClkoutFreq();

// Traduce la frecuencia de CLKOUT medida al cristal del módulo:
//   0        -> no detectado (sin señal / CLKOUT no conectado / chip muerto)
//   8000000  -> cristal de 8 MHz
//   16000000 -> cristal de 16 MHz
uint32_t detectCrystalHz(double clkoutHz);

// Verifica el enlace SPI: reset, registros, BIT MODIFY, pin INT y
// detección del cristal (8 vs 16 MHz).
bool autotestSpi(bool ownsBcm2835);

// Verifica TX/RX en modo loopback interno sobre una instancia ya inicializada.
// Al terminar restaura el modo normal.
bool autotestLoopback(MCP2515& can);

// Verifica la comunicación CAN entre dos módulos (crea sus propias
// instancias: CE0 -> módulo A, CE1 -> módulo B).
// Si ownsBcm2835 es false (emulador), usa beginExisting()/endLight() para
// no cerrar bcm2835 globalmente y no romper las demás instancias.
bool autotestBus(bool ownsBcm2835 = true);

// Ejecuta las tres pruebas en orden; devuelve true si todas pasan.
bool autotestRun(MCP2515& can, bool ownsBcm2835);
