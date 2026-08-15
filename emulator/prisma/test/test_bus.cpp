// ============================================================================
//  test_bus.cpp
//  Verifica la comunicación CAN real entre DOS módulos MCP2515 conectados por
//  el mismo bus SPI (CE0/CE1) y por CAN (CANH/CANL). La lógica está en
//  test/autotest.cpp.
//
//  Compilación:   make test-build
//  Ejecución:     sudo ./obj/test_bus
//  Salida:        0 = OK, 1 = fallo
// ============================================================================
#include "autotest.h"

int main() {
    return autotestBus() ? 0 : 1;
}
