#!/usr/bin/env bash
# ============================================================================
#  Pruebas de comunicación entre la Raspberry Pi y el módulo MCP2515.
#
#  Uso:
#    ./scripts/run_tests.sh                ejecuta las tres pruebas C++
#    ./scripts/run_tests.sh --spi          solo la prueba de enlace SPI
#    ./scripts/run_tests.sh --loopback     solo la prueba TX/RX loopback
#    ./scripts/run_tests.sh --bus          solo la prueba de bus (2 módulos)
#    ./scripts/run_tests.sh --socketcan    prueba alternativa con SocketCAN
#                                          (kernel mcp251x, ver can_kernel_test.sh)
#    ./scripts/run_tests.sh --build        compila las pruebas antes de ejecutar
#    ./scripts/run_tests.sh --help
#
#  Requiere sudo (acceso a /dev/mem). Desde el Makefile:  make test
#  Salida: 0 = todo OK, 1 = hubo fallos, 2 = uso incorrecto.
# ============================================================================
set -u

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SPI_BIN="$DIR/obj/test_spi"
LOOP_BIN="$DIR/obj/test_loopback"
BUS_BIN="$DIR/obj/test_bus"

RUN_SPI=1
RUN_LOOP=1
RUN_BUS=1
RUN_SOCKETCAN=0
BUILD=0

usage() {
    sed -n '5,16p' "$0"
}

for arg in "$@"; do
    case "$arg" in
        --spi)        RUN_LOOP=0; RUN_BUS=0 ;;
        --loopback)   RUN_SPI=0; RUN_BUS=0 ;;
        --bus)        RUN_SPI=0; RUN_LOOP=0 ;;
        --socketcan)  RUN_SPI=0; RUN_LOOP=0; RUN_BUS=0; RUN_SOCKETCAN=1 ;;
        --build)      BUILD=1 ;;
        --help|-h)    usage; exit 0 ;;
        *)            echo "Opción desconocida: $arg"; usage; exit 2 ;;
    esac
done

# bcm2835 necesita acceso a /dev/mem -> root.
if [ "$(id -u)" != "0" ]; then
    echo ">> Re-ejecutando con sudo (acceso a /dev/mem)..."
    exec sudo "$0" "$@"
fi

if [ "$BUILD" = "1" ]; then
    echo ">> Compilando las pruebas..."
    make -C "$DIR" test-build || exit 1
fi

fail=0

if [ "$RUN_SPI" = "1" ]; then
    if [ ! -x "$SPI_BIN" ]; then
        echo "ERROR: no existe $SPI_BIN. Ejecute: make test-build  (o --build)"
        fail=1
    else
        echo ""
        echo "================ Test SPI (enlace Pi <-> MCP2515) ================"
        "$SPI_BIN" || fail=1
    fi
fi

if [ "$RUN_LOOP" = "1" ]; then
    if [ ! -x "$LOOP_BIN" ]; then
        echo "ERROR: no existe $LOOP_BIN. Ejecute: make test-build  (o --build)"
        fail=1
    else
        echo ""
        echo "================ Test Loopback (TX/RX + INT) ====================="
        "$LOOP_BIN" || fail=1
    fi
fi

if [ "$RUN_BUS" = "1" ]; then
    if [ ! -x "$BUS_BIN" ]; then
        echo "ERROR: no existe $BUS_BIN. Ejecute: make test-build  (o --build)"
        fail=1
    else
        echo ""
        echo "=============== Test BUS (dos MCP2515 por CAN) ==================="
        "$BUS_BIN" || fail=1
    fi
fi

if [ "$RUN_SOCKETCAN" = "1" ]; then
    echo ""
    echo "============= Test SocketCAN (kernel mcp251x) ===================="
    echo "NOTA: este método usa el driver de kernel (can0) y es excluyente con"
    echo "las pruebas C++ (bcm2835) y con el emulador sobre el mismo módulo."
    "$DIR/scripts/can_kernel_test.sh" || fail=1
fi

echo ""
if [ "$fail" = "0" ]; then
    echo "RESULTADO GLOBAL: TODAS LAS PRUEBAS PASARON"
else
    echo "RESULTADO GLOBAL: HUBO FALLOS."
    echo "Revise el cableado (CE0->CS, CE1->CS, MISO->SO, MOSI->SI,"
    echo "SCLK->SCK, GPIO25->INT, GPIO24->INT, CANH/CANL con 120 ohmios),"
    echo "la alimentación de los módulos y el cristal (8/16 MHz)."
fi
exit "$fail"
