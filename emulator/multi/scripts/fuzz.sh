#!/bin/bash
# Envía tramas CAN aleatorias al emulador para fuzzing
# Uso: ./scripts/fuzz.sh [interfaz] [tramas] [intervalo_ms]

set -euo pipefail

IFACE="${1:-vcan0}"
COUNT="${2:-1000}"
INTERVAL_MS="${3:-10}"

echo "╔══════════════════════════════════════════╗"
echo "║  CAN Fuzzer                             ║"
echo "║  Interfaz: ${IFACE}                      ║"
echo "║  Tramas:   ${COUNT}                      ║"
echo "║  Intervalo: ${INTERVAL_MS} ms             ║"
echo "╚══════════════════════════════════════════╝"

# Verificar interfaz
if ! ip link show "$IFACE" &>/dev/null; then
    echo "[ERROR] Interfaz $IFACE no existe"
    echo "  Cree una virtual: sudo ip link add vcan0 type vcan && sudo ip link set vcan0 up"
    exit 1
fi

# Verificar cansend
if ! command -v cansend &>/dev/null; then
    echo "[ERROR] cansend no encontrado. Instale can-utils."
    exit 1
fi

# ── Generar tramas aleatorias ────────────────────────────────────
generate_id() {
    printf "0x%03X" $((RANDOM % 2048))
}

generate_data() {
    local len=$((RANDOM % 8 + 1))
    local data=""
    for ((i=0; i<len; i++)); do
        data+=$(printf "%02X" $((RANDOM % 256)))
    done
    echo "$data"
}

# ── Modos OBD2 comunes para fuzz dirigido ────────────────────────
OBD2_SERVICES=(01 02 03 04 05 06 07 08 09 0A 10 11 14 19 22 27 2E 2F 31 34 3E)
UDS_SERVICES=(10 11 14 19 22 23 24 27 28 2A 2C 2E 2F 30 31 34 35 36 37 3D 3E 83 84 85 86 87)

echo ""
echo "━━━ Fuzzing iniciado (Ctrl+C para detener) ─━━"

TX_COUNT=0
ERROR_COUNT=0
START_TIME=$(date +%s%N)

for ((i=1; i<=COUNT; i++)); do
    # Elegir modo aleatorio
    case $((RANDOM % 5)) in
        0)  # Trama estándar OBD2
            ID="7DF"
            SERVICE=${OBD2_SERVICES[$((RANDOM % ${#OBD2_SERVICES[@]}))]}
            PID=$(printf "%02X" $((RANDOM % 256)))
            DATA="02${SERVICE}${PID}00000000"
            ;;
        1)  # Trama UDS física
            ID="7E0"
            SERVICE=${UDS_SERVICES[$((RANDOM % ${#UDS_SERVICES[@]}))]}
            DATA="02${SERVICE}$(printf "%02X" $((RANDOM % 256)))00000000"
            ;;
        2)  # Trama completamente aleatoria
            ID=$(generate_id)
            DATA=$(generate_data)
            ;;
        3)  # Trama con ID extendido
            ID=$(printf "0x%08X" $((RANDOM % 0x1FFFFFFF)))
            DATA=$(generate_data)
            ;;
        4)  # Trama de broadcast funcional
            ID="7DF"
            DATA="01$(printf "%02X" $((RANDOM % 64)))0000000000"
            ;;
    esac

    # Enviar
    if cansend "$IFACE" "${ID}#${DATA}" 2>/dev/null; then
        ((TX_COUNT++))
    else
        ((ERROR_COUNT++))
    fi

    # Progreso cada 100 tramas
    if ((i % 100 == 0)); then
        ELAPSED=$((($(date +%s%N) - START_TIME) / 1000000))
        echo -ne "  Enviadas: ${TX_COUNT} | Errores: ${ERROR_COUNT} | Tiempo: ${ELAPSED}ms\r"
    fi

    sleep "0.${INTERVAL_MS}"
done

END_TIME=$(date +%s%N)
TOTAL_MS=$(( (END_TIME - START_TIME) / 1000000 ))

echo ""
echo ""
echo "━━━ Resumen ─━━"
echo "  Total tramas:  ${TX_COUNT}"
echo "  Errores:       ${ERROR_COUNT}"
echo "  Tiempo:        ${TOTAL_MS} ms"
echo "  Tasa:          $((TX_COUNT * 1000 / (TOTAL_MS + 1))) trama/s"
echo ""
echo "✓ Fuzzing completado en ${IFACE}"
