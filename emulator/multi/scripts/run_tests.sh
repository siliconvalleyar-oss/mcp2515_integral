#!/bin/bash
# Compila y ejecuta los tests del ECU Multi-Emulator
# Uso: ./scripts/run_tests.sh [opciones de Catch2]

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "╔══════════════════════════════════════════╗"
echo "║  ECU Multi-Emulator — Test Suite        ║"
echo "╚══════════════════════════════════════════╝"

# ── Compilar tests ──────────────────────────────────────────────
echo ""
echo "━━━ Compilando tests ━━━"
# Los tests necesitan Catch2. Si no está disponible, compilar con -DCATCH_CONFIG_MAIN
make -C "$SCRIPT_DIR" -j$(nproc) build/ecu_tests 2>/dev/null || {
    echo -e "${YELLOW}Catch2 no encontrado como librería. Compilando con header incluido...${NC}"
    # Compilar tests con Catch2 single-header (se descarga si no existe)
    CATCH_DIR="${BUILD_DIR}/include/catch2"
    mkdir -p "$CATCH_DIR"
    if [ ! -f "${CATCH_DIR}/catch.hpp" ]; then
        echo "  Descargando Catch2 single-header..."
        curl -sL https://github.com/catchorg/Catch2/releases/download/v2.13.10/catch.hpp \
            -o "${CATCH_DIR}/catch.hpp" 2>/dev/null || {
            echo -e "${RED}No se pudo descargar Catch2${NC}"
            echo "Instale Catch2: sudo apt install catch2"
            exit 1
        }
    fi

    CXX="${CROSS_COMPILE:-arm-linux-gnueabihf-}g++"
    CXXFLAGS="-std=c++17 -O2 -g -Isrc -I${BUILD_DIR}/include"
    CXXFLAGS+=" -Isrc/core -Isrc/manufacturers -Isrc/protocols"
    CXXFLAGS+=" -Isrc/database -Isrc/diagnostics -Isrc/simulation"
    CXXFLAGS+=" -Isrc/api -Isrc/security -Isrc/logging"

    SRCS=$(find src -name '*.cpp' -not -path 'src/main.cpp' -not -path 'src/tests/*' | sort)
    TEST_SRCS=$(find src/tests -name '*.cpp' | sort)

    mkdir -p "${BUILD_DIR}/test_objs"
    OBJS=""
    for src in $SRCS; do
        obj="${BUILD_DIR}/test_objs/$(echo $src | tr '/' '_' | sed 's/\.cpp$/.o/')"
        echo "  CXX $src"
        $CXX $CXXFLAGS -c "$src" -o "$obj" &
        OBJS="$OBJS $obj"
    done
    wait

    echo "  CXX tests (Catch2 main)"
    $CXX $CXXFLAGS -o "${BUILD_DIR}/ecu_tests" $OBJS $TEST_SRCS -lpthread -lsqlite3 -lrt
}

# ── Ejecutar tests ──────────────────────────────────────────────
echo ""
echo "━━━ Ejecutando tests ─━━"
TEST_BIN="${BUILD_DIR}/ecu_tests"
if [ ! -f "$TEST_BIN" ]; then
    echo -e "${RED}[ERROR] Test binary not found: ${TEST_BIN}${NC}"
    exit 1
fi

# Verificar si es un binario ARM (no se puede ejecutar en x86)
BIN_ARCH=$(file "$TEST_BIN" 2>/dev/null | grep -o "ARM\|ELF 64-bit\|ELF 32-bit" | head -1)
if echo "$TEST_BIN" | file -f - | grep -q "ARM"; then
    echo -e "${YELLOW}  ⚠ Binario ARM detectado, no se puede ejecutar en esta máquina${NC}"
    echo "  Ejecute en la Raspberry Pi:"
    echo "    scp ${TEST_BIN} pi@rpi:/tmp/"
    echo "    ssh pi@rpi /tmp/ecu_tests $@"
    exit 0
fi

echo "  Ejecutando: ${TEST_BIN} $@"
echo ""

"$TEST_BIN" "$@"

echo ""
echo "✓ Tests completados"
