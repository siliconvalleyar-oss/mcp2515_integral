#!/bin/bash
# AUTEL Scanner - Build Script
# Build via Makefile (binario: bin/scanner_autel_32|_64, sufijo por uname -m)

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

case "$(uname -m)" in
    aarch64) TARGET_SUFFIX="_64" ;;
    *)       TARGET_SUFFIX="_32" ;;
esac
BINARY="${PROJECT_DIR}/bin/scanner_autel${TARGET_SUFFIX}"

echo "=========================================="
echo "  AUTEL Scanner - Build"
echo "=========================================="
echo ""

echo "[1/3] Compilando (make)..."
make -C "${PROJECT_DIR}" -j$(nproc)

echo "[2/3] Ejecutando pruebas..."
if [ -x "${PROJECT_DIR}/tests/test_obd2" ]; then
    "${PROJECT_DIR}/tests/test_obd2"
else
    echo "  (sin binario de tests compilado)"
fi

echo "[3/3] Verificando binario..."
if [ -f "$BINARY" ]; then
    echo ""
    echo "=========================================="
    echo "  Build completado!"
    echo "=========================================="
    echo ""
    echo "Ejecutable: ${BINARY}"
    echo ""
    echo "Para ejecutar:"
    echo "  sudo ${BINARY}"
    echo ""
else
    echo ""
    echo "ERROR: No se genero el binario ${BINARY}"
    exit 1
fi
