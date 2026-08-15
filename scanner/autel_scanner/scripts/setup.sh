#!/bin/bash
# AUTEL Scanner - Setup Script for Raspberry Pi
# Installs all dependencies for 32-bit and 64-bit Raspberry Pi OS

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "=========================================="
echo "  AUTEL Scanner - Setup"
echo "=========================================="
echo ""

# Check if running on Raspberry Pi
if ! grep -q "Raspberry Pi" /proc/cpuinfo 2>/dev/null; then
    echo "Warning: No se detecto Raspberry Pi"
    read -p "Continuar de todas formas? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Detect architecture
ARCH=$(uname -m)
if [ "$ARCH" = "aarch64" ]; then
    echo "Arquitectura detectada: 64-bit (aarch64)"
else
    echo "Arquitectura detectada: 32-bit (armv7l)"
fi

# Check for sudo/root
if [ "$EUID" -ne 0 ]; then
    echo "Error: Este script debe ejecutarse con sudo"
    echo "Uso: sudo ./scripts/setup.sh"
    exit 1
fi

# Update system
echo ""
echo "[1/6] Actualizando sistema..."
apt update

# Install dependencies
echo ""
echo "[2/6] Instalando dependencias..."
apt install -y \
    build-essential \
    cmake \
    git \
    i2c-tools \
    libbcm2835-dev \
    python3-pip \
    python3-smbus \
    pyserial

# Enable I2C and SPI
echo ""
echo "[3/6] Habilitando interfaces..."
raspi-config nonint do_i2c 0
raspi-config nonint do_spi 0

# Add user to required groups
echo ""
echo "[4/6] Configurando permisos..."
usermod -aG i2c,spi "$SUDO_USER"

# Create log directory
echo ""
echo "[5/6] Creando directorio de logs..."
mkdir -p /var/log
touch /var/log/autel_scanner.log
chmod 666 /var/log/autel_scanner.log

# Create project directory if needed
echo ""
echo "[6/6] Configurando proyecto..."
mkdir -p "$SCRIPT_DIR"/{build,obj,bin}

echo ""
echo "=========================================="
echo "  Setup completado!"
echo "=========================================="
echo ""
echo "IMPORTANTE: Reinicia la Raspberry Pi para aplicar cambios"
echo "  sudo reboot"
echo ""
echo "Despues de reiniciar, compila el proyecto:"
echo "  cd $SCRIPT_DIR"
echo "  make clean"
echo "  make -j4"
echo ""
