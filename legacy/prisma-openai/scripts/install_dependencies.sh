#!/bin/bash

set -e

echo "======================================"
echo " Instalacion de dependencias"
echo "======================================"

sudo apt-get update

sudo apt-get install -y \
    g++ \
    make \
    git \
    libbcm2835-dev

echo
echo "Dependencias instaladas."

echo
echo "Verificando bcm2835..."

if ldconfig -p | grep -q bcm2835; then
    echo "bcm2835 encontrada."
else
    echo "ADVERTENCIA: no se encontro bcm2835 mediante ldconfig."
fi

echo
echo "Habilita SPI con:"
echo
echo "    sudo raspi-config"
echo
echo "Interface Options -> SPI -> Enable"
echo
