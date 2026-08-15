#!/bin/bash
# Docker entrypoint for ECU Multi-Emulator

set -e

echo "╔══════════════════════════════════════════╗"
echo "║  ECU Multi-Emulator v1.0.0              ║"
echo "║  Starting in Docker container...         ║"
echo "╚══════════════════════════════════════════╝"

# Create virtual CAN interface if needed
if [ "$CREATE_VCAN" = "true" ]; then
    echo "→ Creating virtual CAN interface vcan0..."
    modprobe vcan 2>/dev/null || true
    ip link add vcan0 type vcan 2>/dev/null || true
    ip link set vcan0 up
    echo "→ vcan0 is up"
fi

# Check if real CAN interfaces exist
if ip link show can0 &>/dev/null; then
    echo "→ CAN interface can0 found"
    ip link set can0 up type can bitrate 500000 2>/dev/null || true
fi

echo "→ Starting ECU Emulator..."
echo ""

exec "$@"
