// ============================================================================
//  test_spi.cpp
//  Verifica el enlace SPI entre la Raspberry Pi y el MCP2515 (cableado,
//  alimentación y enlace de datos). La lógica está en test/autotest.cpp.
//
//  Compilación:   make test-build
//  Ejecución:     sudo ./obj/test_spi
//  Salida:        0 = OK, 1 = fallo
// ============================================================================
#include "autotest.h"

int main() {
    return autotestSpi(true) ? 0 : 1;
}
