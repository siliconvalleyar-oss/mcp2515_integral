#!/usr/bin/env bash
# can_test_ecu.sh - Modo ECU: escucha requests OBD2 en can0 y responde
# (echo del payload). Corre en la otra Raspberry, modo SCANNER envia.
#
# Uso:  ./can_test_ecu.sh [iface] [bitrate]
# Ej:   ./can_test_ecu.sh
#       ./can_test_ecu.sh can0 500000
set -u

IFACE="${1:-can0}"
BITRATE="${2:-500000}"

die() { echo "[ECU] ERROR: $*" >&2; exit 1; }

for c in cansend candump ip; do
    command -v "$c" >/dev/null 2>&1 || die "falta '$c' (sudo apt install can-utils)"
done

sudo ip link set "$IFACE" down 2>/dev/null
sudo ip link set "$IFACE" type can bitrate "$BITRATE" || die "no se pudo configurar $IFACE"
sudo ip link set "$IFACE" up || die "no se pudo levantar $IFACE"

if ! ip link show "$IFACE" 2>/dev/null | grep -q "state UP"; then
    die "$IFACE no quedo UP (revisa dtoverlay=mcp2515-can0 en /boot/firmware/config.txt)"
fi

echo "[ECU] $IFACE @${BITRATE} bps UP"
echo "[ECU] Escuchando...  request 7DF/7E0 -> respondo 7E8/7E9 (echo del payload)"
echo "[ECU] Ctrl-C para salir."

trap 'echo; echo "[ECU] deteniendo..."; sudo ip link set "$IFACE" down 2>/dev/null; exit 0' INT

sudo candump "$IFACE" | while read -r iface id len rest; do
    id="${id%#*}"
    case "$id" in
        7DF|7E0)
            reply="7E8"
            [ "$id" = "7E0" ] && reply="7E9"
            echo "[ECU] request ${id} -> ${reply}  data: ${rest}"
            sudo cansend "$IFACE" "${reply}#${rest}" || echo "[ECU] error al responder" >&2
            ;;
        7E8|7E9)
            echo "[ECU] (trama propia/ecus) $id $rest"
            ;;
        *)
            echo "[ECU] frame $id [$len] $rest"
            ;;
    esac
done
