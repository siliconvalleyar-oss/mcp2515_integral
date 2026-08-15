#!/bin/bash
# install_deps.sh — Instalar dependencias en Raspberry Pi 2W (Raspberry Pi OS)
echo "[INFO] Actualizando sistema..."
sudo apt update

echo "[INFO] Instalando dependencias..."
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libbluetooth-dev \
    libgpiod-dev \
    libgpiod2 \
    gpiod \
    bluez \
    bluetooth

echo "[INFO] Habilitando SPI..."
# Habilitar SPI en /boot/config.txt si no está habilitado
if ! grep -q "^dtparam=spi=on" /boot/config.txt; then
    echo "dtparam=spi=on" | sudo tee -a /boot/config.txt
    echo "[OK] SPI habilitado — se requiere reiniciar"
fi
# O usar raspi-config:
# sudo raspi-config nonint do_spi 0

echo "[INFO] Configurando Bluetooth..."
sudo systemctl enable bluetooth
sudo systemctl start bluetooth

echo ""
echo "╔══════════════════════════════════════════════╗"
echo "║  Para emparejar el ELM327:                   ║"
echo "║    bluetoothctl                              ║"
echo "║    power on                                  ║"
echo "║    scan on                                   ║"
echo "║    pair XX:XX:XX:XX:XX:XX                   ║"
echo "║    trust XX:XX:XX:XX:XX:XX                  ║"
echo "║    quit                                      ║"
echo "╚══════════════════════════════════════════════╝"
echo ""
echo "[IMPORTANTE] Si se habilitó SPI, reiniciar: sudo reboot"
