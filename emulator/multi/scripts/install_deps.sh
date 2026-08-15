#!/bin/bash
# Instala todas las dependencias necesarias para compilar el ECU Multi-Emulator
# Soporta: Debian/Ubuntu (apt), Fedora/RHEL (dnf), Arch (pacman)

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  ECU Multi-Emulator — Instalación de dependencias          ║"
echo "╚══════════════════════════════════════════════════════════════╝"

# ── Detectar distro ──────────────────────────────────────────────
install_apt() {
    sudo apt update
    sudo apt install -y \
        build-essential cmake git \
        libsqlite3-dev libsqlite3-dev:armhf \
        libnl-3-dev libnl-route-3-dev \
        nlohmann-json3-dev \
        pkg-config curl wget \
        can-utils iproute2 \
        g++-arm-linux-gnueabihf \
        binutils-arm-linux-gnueabihf \
        libc6-dev-armhf-cross
}

install_dnf() {
    sudo dnf install -y \
        gcc-c++ cmake git \
        sqlite-devel \
        libnl3-devel \
        nlohmann-json-devel \
        pkgconfig curl wget \
        can-utils \
        arm-linux-gnueabihf-gcc-c++ \
        arm-linux-gnueabihf-binutils
}

install_pacman() {
    sudo pacman -S --noconfirm \
        base-devel cmake git \
        sqlite libnl \
        nlohmann-json \
        pkgconf curl wget \
        can-utils \
        arm-linux-gnueabihf-gcc \
        arm-linux-gnueabihf-binutils
}

# ── Detectar e instalar ──────────────────────────────────────────
if command -v apt &>/dev/null; then
    install_apt
elif command -v dnf &>/dev/null; then
    install_dnf
elif command -v pacman &>/dev/null; then
    install_pacman
else
    echo "[ERROR] No se pudo detectar el gestor de paquetes."
    echo "  Instale manualmente: compilador cruzado arm-linux-gnueabihf-g++,"
    echo "  sqlite3-dev, libnl-3-dev, nlohmann-json, cmake, make, can-utils."
    exit 1
fi

# ── Verificar toolchain ──────────────────────────────────────────
echo ""
echo "━━━ Verificando toolchain ARM ━━━"
TOOLCHAIN="arm-linux-gnueabihf-g++"
if command -v $TOOLCHAIN &>/dev/null; then
    echo "  ✓ $TOOLCHAIN encontrado"
    $TOOLCHAIN --version | head -1
else
    echo "  ⚠ $TOOLCHAIN no encontrado en PATH"
    echo "    Puede compilar con: make CROSS_COMPILE="
fi

echo ""
echo "━━━ Verificando dependencias ━━━"
for dep in cmake make curl wget; do
    command -v $dep &>/dev/null && echo "  ✓ $dep" || echo "  ✗ $dep (opcional)"
done

for lib in sqlite3 nlohmann/json; do
    echo -n "  ? $lib ... "
    case $lib in
        sqlite3)
            echo '#include <sqlite3.h>' | g++ -E -x c++ - -o /dev/null 2>/dev/null \
                && echo "✓" || echo "⚠ no encontrado (buscar libsqlite3-dev)"
            ;;
        nlohmann/json)
            echo '#include <nlohmann/json.hpp>' | g++ -E -x c++ - -o /dev/null 2>/dev/null \
                && echo "✓" || echo "⚠ se descargará automáticamente al compilar"
            ;;
    esac
done

# ── Crear entorno virtual CAN (opcional) ─────────────────────────
echo ""
echo "━━━ Configurando vcan (opcional) ─━━"
if ! lsmod | grep -q vcan; then
    echo "  Cargando módulo vcan..."
    sudo modprobe vcan 2>/dev/null && echo "  ✓ vcan cargado" || echo "  ⚠ no se pudo cargar vcan"
fi
if ! ip link show vcan0 &>/dev/null; then
    sudo ip link add vcan0 type vcan 2>/dev/null && echo "  ✓ vcan0 creado" || true
fi
sudo ip link set vcan0 up 2>/dev/null && echo "  ✓ vcan0 activo" || true

# ── Resumen final ────────────────────────────────────────────────
echo ""
echo "━━━ Resumen ─━━"
echo "  Compilar:        cd ${SCRIPT_DIR} && make -j\$(nproc)"
echo "  Compilar nativo:  cd ${SCRIPT_DIR} && make CROSS_COMPILE= -j\$(nproc)"
echo "  Probar:           cd ${SCRIPT_DIR} && bash examples/test_commands.sh"
echo "  CAN virtual:      vcan0 listo en ${SCRIPT_DIR}/config.json"
echo ""
echo "  Dependencia faltante? Intente:"
echo "    sudo apt install nlohmann-json3-dev libsqlite3-dev libnl-3-dev"
echo ""
echo "✓ Instalación completada"
