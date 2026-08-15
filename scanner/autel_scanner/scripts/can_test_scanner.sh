#!/usr/bin/env bash
# can_test_scanner.sh - Modo SCANNER: envia un request OBD2 por can0 y
# espera la respuesta de la ECU (la otra Raspberry en modo ECU).
#
# Uso:  ./can_test_scanner.sh [iface] [bitrate] [req_id] [payload] [timeout_ms]
# Ej:   ./can_test_scanner.sh
#       ./can_test_scanner.sh can0 500000 7DF "02 01 00 00 00 00 00 00" 5000
set -u

IFACE="${1:-can0}"
BITRATE="${2:-500000}"
REQ_ID="${3:-7DF}"
PAYLOAD="${4:-02 01 00 00 00 00 00 00}"
TIMEOUT_MS="${5:-5000}"

die() { echo "[SCANNER] ERROR: $*" >&2; exit 1; }

for c in cansend candump ip; do
    command -v "$c" >/dev/null 2>&1 || die "falta '$c' (sudo apt install can-utils)"
done

sudo ip link set "$IFACE" down 2>/dev/null
sudo ip link set "$IFACE" type can bitrate "$BITRATE" || die "no se pudo configurar $IFACE"
sudo ip link set "$IFACE" up || die "no se pudo levantar $IFACE"

if ! ip link show "$IFACE" 2>/dev/null | grep -q "state UP"; then
    die "$IFACE no quedo UP (revisa dtoverlay=mcp2515-can0 en /boot/firmware/config.txt)"
fi

echo "[SCANNER] $IFACE @${BITRATE} bps UP"
echo "[SCANNER] Enviando request  ${REQ_ID}  # ${PAYLOAD}"

# candump DEBE abrir el socket ANTES de transmitir: si se abre despues de
# cansend, la respuesta puede llegar antes de que exista el socket y se pierde.
tmp="$(mktemp)"
sudo candump "$IFACE" >"$tmp" 2>&1 &
candump_pid=$!
sleep 1

sudo cansend "$IFACE" "${REQ_ID}#${PAYLOAD// /}" || die "fallo cansend"

echo "[SCANNER] Esperando respuesta (${TIMEOUT_MS} ms)..."
rc=0
for _ in $(seq 1 $((TIMEOUT_MS / 100))); do
    if [ -s "$tmp" ]; then
        sleep 0.2
        break
    fi
    sleep 0.1
done
if ! [ -s "$tmp" ]; then
    rc=1
fi
sudo kill "$candump_pid" 2>/dev/null
sudo wait "$candump_pid" 2>/dev/null
out="$(cat "$tmp" 2>/dev/null)"
rm -f "$tmp"

if [ -n "$out" ]; then
    echo "[SCANNER] RESPUESTA RECIBIDA:"
    echo "$out"
    echo "[SCANNER] OK: comunicacion CAN funcionando."
else
    echo "[SCANNER] SIN RESPUESTA en ${TIMEOUT_MS} ms (revisa cableado y dtoverlay)."
fi

sudo ip link set "$IFACE" down
exit $rc
