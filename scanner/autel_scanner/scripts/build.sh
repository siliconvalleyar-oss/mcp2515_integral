#!/bin/bash
# AUTEL Scanner - Build Script

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

echo "=========================================="
echo "  AUTEL Scanner - Build"
echo "=========================================="
echo ""

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Run CMake
echo "[1/3] Ejecutando CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "[2/3] Compilando..."
make -j$(nproc)

# Run tests if available
echo "[3/3] Ejecutando pruebas..."
if [ -f "CTestTestfile.cmake" ]; then
    ctest --output-on-failure
fi

echo ""
echo "=========================================="
echo "  Build completado!"
echo "=========================================="
echo ""
echo "Ejecutable: ${BUILD_DIR}/autel_scanner"
echo ""
echo "Para ejecutar:"
echo "  sudo ${BUILD_DIR}/autel_scanner"
echo ""
