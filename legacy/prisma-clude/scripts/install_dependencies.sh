#!/bin/bash
# =========================================================
# Instala la libreria bcm2835 necesaria para compilar el
# emulador Chevrolet Prisma sobre Raspberry Pi.
# =========================================================
set -e

BCM2835_VERSION="1.75"
BCM2835_URL="http://www.airspayce.com/mikem/bcm2835/bcm2835-${BCM2835_VERSION}.tar.gz"

echo "=== Instalador de dependencias: libreria bcm2835 ==="

if ! command -v g++ >/dev/null 2>&1; then
    echo "Instalando herramientas de compilacion..."
    sudo apt-get update
    sudo apt-get install -y build-essential wget
fi

if ldconfig -p | grep -q libbcm2835; then
    echo "La libreria bcm2835 ya esta instalada. Nada que hacer."
    exit 0
fi

TMP_DIR=$(mktemp -d)
cd "$TMP_DIR"

echo "Descargando bcm2835 v${BCM2835_VERSION}..."
wget -q "$BCM2835_URL" -O bcm2835.tar.gz

echo "Extrayendo..."
tar xzf bcm2835.tar.gz
cd "bcm2835-${BCM2835_VERSION}"

echo "Compilando e instalando (requiere permisos de administrador)..."
./configure
make
sudo make install

cd /
rm -rf "$TMP_DIR"

echo ""
echo "=== Instalacion completa. Ejecute 'make' en la raiz del proyecto. ==="
