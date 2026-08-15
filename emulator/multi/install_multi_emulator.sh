#!/bin/bash
# Installation script for ECU Multi-Emulator
# Raspberry Pi + MCP2515

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}"
echo "╔══════════════════════════════════════════╗"
echo "║  ECU Multi-Emulator Installer           ║"
echo "║  Multi-Brand Vehicle Diagnostic System  ║"
echo "╚══════════════════════════════════════════╝"
echo -e "${NC}"

# Check root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Please run as root (sudo)${NC}"
    exit 1
fi

INSTALL_DIR="/opt/ecu_emulator"
CONFIG_DIR="/etc/ecu_emulator"
LOG_DIR="/var/log"
BUILD_DIR="/tmp/ecu_emulator_build"

echo ""
echo "━━━ Installing Dependencies ━━━"

# Detect package manager
if command -v apt &>/dev/null; then
    PKG_MANAGER="apt"
    apt update
    apt install -y \
        build-essential cmake git \
        libsqlite3-dev \
        libnl-3-dev libnl-route-3-dev \
        nlohmann-json3-dev \
        pkg-config \
        can-utils \
        iproute2
elif command -v dnf &>/dev/null; then
    PKG_MANAGER="dnf"
    dnf install -y gcc-c++ cmake git sqlite-devel libnl3-devel nlohmann-json-devel pkgconfig can-utils
elif command -v pacman &>/dev/null; then
    PKG_MANAGER="pacman"
    pacman -S --noconfirm base-devel cmake git sqlite libnl nlohmann-json can-utils
else
    echo -e "${RED}Unsupported package manager. Please install dependencies manually.${NC}"
    exit 1
fi

echo ""
echo "━━━ Building from Source ━━━"

mkdir -p ${BUILD_DIR}
cp -r $(dirname $0)/* ${BUILD_DIR}/
cd ${BUILD_DIR}

mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

echo ""
echo "━━━ Installing Files ━━━"

mkdir -p ${INSTALL_DIR}
mkdir -p ${CONFIG_DIR}
mkdir -p ${LOG_DIR}

cp ecu_emulator ${INSTALL_DIR}/
cp config.json ${CONFIG_DIR}/
cp database/schema.sql ${CONFIG_DIR}/

# Create symlink
ln -sf ${INSTALL_DIR}/ecu_emulator /usr/local/bin/ecu_emulator

echo ""
echo "━━━ Configuring SocketCAN ━━━"

# Add CAN interface configuration
if grep -q "can0" /etc/network/interfaces 2>/dev/null; then
    echo "CAN interface already configured"
else
    cat >> /etc/network/interfaces << EOF

# CAN interface for MCP2515
auto can0
iface can0 inet manual
    pre-up ip link set can0 type can bitrate 500000 triple-sampling on
    up ip link set can0 up
    down ip link set can0 down
EOF
    echo "CAN interface added to /etc/network/interfaces"
fi

echo ""
echo "━━━ Creating systemd Service ━━━"

cat > /etc/systemd/system/ecu_emulator.service << EOF
[Unit]
Description=ECU Multi-Emulator Diagnostic System
After=network.target can0.target
Wants=can0.target

[Service]
Type=simple
ExecStart=${INSTALL_DIR}/ecu_emulator ${CONFIG_DIR}/config.json
Restart=always
RestartSec=5
WorkingDirectory=${INSTALL_DIR}
StandardOutput=journal
StandardError=journal
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable ecu_emulator.service

echo ""
echo "━━━ Configuring Log Rotation ━━━"

cat > /etc/logrotate.d/ecu_emulator << EOF
${LOG_DIR}/ecu_emulator.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    copytruncate
}
EOF

echo ""
echo "━━━ Installation Complete ━━━"
echo -e "${GREEN}"
echo "  Installation directory: ${INSTALL_DIR}"
echo "  Configuration: ${CONFIG_DIR}/config.json"
echo "  Logs: ${LOG_DIR}/ecu_emulator.log"
echo ""
echo "  Commands:"
echo "    sudo systemctl start ecu_emulator   # Start emulator"
echo "    sudo systemctl stop ecu_emulator    # Stop emulator"
echo "    sudo systemctl status ecu_emulator  # Check status"
echo "    sudo journalctl -u ecu_emulator -f  # Follow logs"
echo ""
echo "  To configure CAN interface manually:"
echo "    sudo ip link set can0 type can bitrate 500000"
echo "    sudo ip link set can0 up"
echo ""
echo "  To enable debug mode, edit ${CONFIG_DIR}/config.json"
echo "  and set logging.level to 'debug' or 'trace'"
echo -e "${NC}"
