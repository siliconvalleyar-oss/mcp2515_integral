#!/bin/bash
# Compila el ECU Multi-Emulator con opciones flexibles
# Uso: ./scripts/build.sh [opciones]

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

usage() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "Opciones:"
    echo "  -j N        Número de hilos paralelos (por defecto: nproc)"
    echo "  -n, --native   Compilar con g++ nativo (sin cross)"
    echo "  -c, --cross    Compilar con arm-linux-gnueabihf-g++ (por defecto)"
    echo "  -d, --debug    Modo debug (sin optimización, -g)"
    echo "  -r, --release  Modo release (-O2) (por defecto)"
    echo "  -t, --test     Compilar tests (requiere Catch2)"
    echo "  --clean        Limpiar y recompilar"
    echo "  -v, --verbose  Verbose (muestra comandos completos)"
    echo "  -h, --help     Muestra esta ayuda"
    echo ""
    echo "Ejemplos:"
    echo "  $0                  # compilación cruzada release"
    echo "  $0 -n -j4           # nativa, 4 hilos"
    echo "  $0 -d --clean       # debug, rebuild limpio"
    echo "  $0 -t               # compilar tests"
}

JOBS=$(nproc)
MODE="cross"
BUILD_TYPE="release"
BUILD_TESTS=false
CLEAN=false
VERBOSE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -j) JOBS="$2"; shift 2 ;;
        -n|--native) MODE="native"; shift ;;
        -c|--cross) MODE="cross"; shift ;;
        -d|--debug) BUILD_TYPE="debug"; shift ;;
        -r|--release) BUILD_TYPE="release"; shift ;;
        -t|--test) BUILD_TESTS=true; shift ;;
        --clean) CLEAN=true; shift ;;
        -v|--verbose) VERBOSE="V=1"; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "[ERROR] Opción desconocida: $1"; usage; exit 1 ;;
    esac
done

# ── Flags ────────────────────────────────────────────────────────
CROSS_FLAG=""
MAKEFLAGS=""
if [ "$MODE" = "native" ]; then
    CROSS_FLAG="CROSS_COMPILE="
    echo -e "${CYAN}Modo: nativo (g++)${NC}"
else
    echo -e "${CYAN}Modo: cross-compilación (arm-linux-gnueabihf-g++)${NC}"
fi

if [ "$BUILD_TYPE" = "debug" ]; then
    MAKEFLAGS="CXXFLAGS_EXTRA=-g"
    echo -e "${YELLOW}Debug: -g habilitado${NC}"
fi

if $CLEAN; then
    echo -e "${YELLOW}Limpiando build anterior...${NC}"
    make -C "$SCRIPT_DIR" clean 2>/dev/null || true
fi

# ── Verificar toolchain ──────────────────────────────────────────
if [ "$MODE" = "cross" ]; then
    CXX="arm-linux-gnueabihf-g++"
    if ! command -v "$CXX" &>/dev/null; then
        echo -e "${RED}[ERROR] $CXX no encontrado${NC}"
        echo "  Instálelo o use -n para compilar con g++ nativo."
        echo "  Debian: sudo apt install g++-arm-linux-gnueabihf"
        exit 1
    fi
fi

# ── Compilar ─────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}━━━ Compilando ECU Multi-Emulator ━━━${NC}"
echo "  Directorio: ${SCRIPT_DIR}"
echo "  Hilos:      ${JOBS}"
echo "  Modo:       ${MODE}"
echo "  Tipo:       ${BUILD_TYPE}"
echo ""

BUILD_CMD="make -C ${SCRIPT_DIR} -j${JOBS} ${CROSS_FLAG} ${MAKEFLAGS} ${VERBOSE}"

if $BUILD_TESTS; then
    echo -e "${YELLOW}Incluyendo tests...${NC}"
    BUILD_CMD="${BUILD_CMD} test"
fi

echo -e "${CYAN}\$ ${BUILD_CMD}${NC}"
echo ""
eval "$BUILD_CMD"

# ── Resultado ────────────────────────────────────────────────────
echo ""
if [ -f "${BUILD_DIR}/ecu_emulator" ]; then
    FILE_SIZE=$(stat --printf="%s" "${BUILD_DIR}/ecu_emulator" 2>/dev/null || stat -f%z "${BUILD_DIR}/ecu_emulator" 2>/dev/null)
    echo -e "${GREEN}✓ Compilación exitosa${NC}"
    echo "  Binario: ${BUILD_DIR}/ecu_emulator ($(numfmt --to=iec $FILE_SIZE 2>/dev/null || echo "${FILE_SIZE} bytes"))"
    file "${BUILD_DIR}/ecu_emulator" | sed 's/^/  Tipo: /'
else
    echo -e "${RED}✗ El binario no se generó${NC}"
    exit 1
fi
