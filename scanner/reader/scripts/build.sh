#!/bin/bash
# build.sh — Compilar obd2_rpi para Raspberry Pi
# Uso:
#   ./scripts/build.sh                     # Compilar local
#   ./scripts/build.sh remote              # Compilar via SSH
#   ./scripts/build.sh remote <HOST>       # Compilar via SSH a host custom
#   ./scripts/build.sh clean               # Limpiar

set -e

REMOTE_HOST="${2:-joy@raspberry.local}"
REMOTE_DIR="/home/joy/src/elm_327_rpi_oled_v16/obd2_rpi"
BUILD_DIR="build"

clean() {
    echo "[BUILD] Limpiando..."
    rm -rf ${BUILD_DIR} bin
    echo "[OK] Limpio"
}

build_local() {
    echo "[BUILD] Compilando localmente..."
    mkdir -p ${BUILD_DIR}
    cmake -S . -B ${BUILD_DIR} -DBUILD_TESTS=OFF
    make -C ${BUILD_DIR} -j$(nproc)
    echo "[OK] Binario en bin/obd2_rpi"
}

build_remote() {
    echo "[BUILD] Compilando en ${REMOTE_HOST}..."
    ssh "${REMOTE_HOST}" "cd ${REMOTE_DIR} && mkdir -p ${BUILD_DIR} && cmake -S . -B ${BUILD_DIR} -DBUILD_TESTS=OFF && make -C ${BUILD_DIR} -j\$(nproc)"
    echo "[OK] Compilacion remota completa"
}

case "${1:-local}" in
    clean)    clean ;;
    remote)   build_remote ;;
    local|*)  build_local ;;
esac
