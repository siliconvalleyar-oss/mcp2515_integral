#!/bin/bash
# Configura interfaces CAN (real MCP2515 o virtual vcan)
# Uso: ./scripts/setup_can.sh [vcan|can0|mcp2515]

set -euo pipefail

MODE="${1:-auto}"

echo "╔══════════════════════════════════════════╗"
echo "║  Configuración de interfaz CAN          ║"
echo "╚══════════════════════════════════════════╝"

setup_vcan() {
    echo "→ Configurando vcan0 (virtual)..."
    sudo modprobe vcan 2>/dev/null || echo "  vcan ya cargado"
    sudo ip link add vcan0 type vcan 2>/dev/null || echo "  vcan0 ya existe"
    sudo ip link set vcan0 up
    echo "  ✓ vcan0 activo (500 kbps por defecto)"
    echo "  Prueba: candump vcan0 & cansend vcan0 123#DEADBEEF"
}

setup_mcp2515() {
    local SPI_CS="${1:-0}"
    echo "→ Configurando can0 (MCP2515, CS=${SPI_CS})..."

    # Cargar módulo SPI
    sudo modprobe spi_bcm2835 2>/dev/null || true

    # Cargar MCP2515 con parámetros
    sudo modprobe mcp251x 2>/dev/null || true

    # Configurar mediante Device Tree overlay (RPi)
    if [ -f /proc/device-tree/model ] && grep -qi "raspberry" /proc/device-tree/model; then
        echo "  → Usando overlays de Raspberry Pi..."
        sudo dtoverlay mcp2515-can0 oscillator=16000000 interrupt=25 2>/dev/null || {
            echo "  ⚠ overlay no disponible, configurando manualmente..."
            sudo ip link set can0 type can bitrate 500000 triple-sampling on
        }
    else
        sudo ip link set can0 type can bitrate 500000 triple-sampling on
    fi

    sudo ip link set can0 up
    echo "  ✓ can0 activo (500 kbps, triple-sampling)"
}

setup_can0() {
    echo "→ Configurando can0 (hardware existente)..."
    sudo ip link set can0 type can bitrate 500000 2>/dev/null || true
    sudo ip link set can0 up 2>/dev/null || {
        echo "  ✗ No se pudo activar can0"
        echo "  Soluciones:"
        echo "    - sudo modprobe mcp251x"
        echo "    - Verificar conexión MCP2515"
        echo "    - Usar vcan: $0 vcan"
        exit 1
    }
    echo "  ✓ can0 activo"
}

show_status() {
    echo ""
    echo "━━━ Estado de interfaces CAN ━━━"
    for iface in can0 can1 vcan0 vcan1; do
        if ip link show "$iface" &>/dev/null; then
            state=$(ip -s -d link show "$iface" | grep -o "state [A-Z]*" | head -1 || echo "state UP")
            echo "  $iface: $(ip link show "$iface" | grep -o 'state [A-Z]*' | head -1 || echo "presente")"
            ip -s -d link show "$iface" | grep -A2 "RX:" | head -3
        fi
    done
}

# ── Ejecutar según modo ─────────────────────────────────────────
case "$MODE" in
    vcan|virtual)
        setup_vcan
        ;;
    can0|real|mcp2515)
        setup_can0
        ;;
    auto)
        if ip link show can0 &>/dev/null; then
            setup_can0
        else
            setup_vcan
        fi
        ;;
    status)
        show_status
        exit 0
        ;;
    *)
        echo "Uso: $0 [vcan|can0|mcp2515|auto|status]"
        echo ""
        echo "  vcan     - Interfaz CAN virtual (sin hardware)"
        echo "  can0     - Interfaz CAN física existente"
        echo "  mcp2515  - Configurar MCP2515 en Raspberry Pi"
        echo "  auto     - Detectar y configurar (por defecto)"
        echo "  status   - Mostrar estado de interfaces CAN"
        exit 1
        ;;
esac

show_status
echo ""
echo "✓ Configuración completada"
echo "  Para monitorear: candump $([ "$MODE" = "vcan" ] && echo "vcan0" || echo "can0")"
