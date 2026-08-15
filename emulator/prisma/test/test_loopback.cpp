// ============================================================================
//  test_loopback.cpp
//  Verifica TX/RX del MCP2515 en modo loopback interno (sin bus ni segundo
//  nodo). La lógica está en test/autotest.cpp.
//
//  Compilación:   make test-build
//  Ejecución:     sudo ./obj/test_loopback
//  Salida:        0 = OK, 1 = fallo
// ============================================================================
#include "autotest.h"

#include <cstdio>

int main() {
    MCP2515 can;
    if (!can.begin()) {
        std::printf("\n ERROR: no se pudo inicializar el MCP2515.\n");
        std::printf(" Revise el cableado SPI (CE0->CS, MISO->SO, MOSI->SI,\n");
        std::printf(" SCLK->SCK, GPIO25->INT), la alimentacion y libbcm2835.\n");
        return 1;
    }
    const bool ok = autotestLoopback(can);
    can.end();
    return ok ? 0 : 1;
}
