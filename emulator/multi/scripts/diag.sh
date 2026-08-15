#!/bin/bash
# Cliente de diagnóstico CAN interactivo para probar el emulador
# Uso: ./scripts/diag.sh [interfaz]

set -euo pipefail

IFACE="${1:-vcan0}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

check_can() {
    if ! ip link show "$IFACE" &>/dev/null; then
        echo -e "${RED}[ERROR] Interfaz $IFACE no disponible${NC}"
        echo "  Configure: bash ${SCRIPT_DIR}/setup_can.sh"
        exit 1
    fi
}

send() {
    local id="$1"
    local data="$2"
    cansend "$IFACE" "${id}#${data}" 2>/dev/null || {
        echo -e "${RED}Error enviando trama${NC}"
        return 1
    }
    echo -e "${CYAN}TX${NC} ${id} ${data}"
}

# ── Menú interactivo ────────────────────────────────────────────
menu() {
    while true; do
        echo ""
        echo "╔══════════════════════════════════════════╗"
        echo "║  CAN Diagnostic Client                   ║"
        echo "║  Interfaz: ${IFACE}                       ║"
        echo "╚══════════════════════════════════════════╝"
        echo ""
        echo "  1)  OBD2 Mode 01 - RPM"
        echo "  2)  OBD2 Mode 01 - Velocidad"
        echo "  3)  OBD2 Mode 01 - Temperatura refrigerante"
        echo "  4)  OBD2 Mode 03 - Leer DTCs"
        echo "  5)  OBD2 Mode 04 - Limpiar DTCs"
        echo "  6)  OBD2 Mode 09 - VIN"
        echo "  7)  UDS Session Control - Extended"
        echo "  8)  UDS Tester Present"
        echo "  9)  UDS Security Access - Request Seed"
        echo " 10)  UDS Read DID - VIN (0xF190)"
        echo " 11)  GMLAN Mode 22 - Read DID Engine RPM"
        echo " 12)  UDS ECU Reset"
        echo " 13)  Trama personalizada"
        echo " 14)  Iniciar candump (monitoreo)"
        echo "  q)   Salir"
        echo ""
        read -rp "Seleccione [1-14]: " opt

        case "$opt" in
            1) send "7DF" "02010C0000000000" ;;
            2) send "7DF" "02010D0000000000" ;;
            3) send "7DF" "0201050000000000" ;;
            4) send "7DF" "0300000000000000" ;;
            5) send "7DF" "0400000000000000" ;;
            6) send "7DF" "0209020000000000" ;;
            7) send "7E0" "0210030000000000" ;;
            8) send "7E0" "013E000000000000" ;;
            9) send "7E0" "0227010000000000" ;;
            10) send "7E0" "0322F19000000000" ;;
            11) send "7E0" "0322F18100000000" ;;
            12) send "7E0" "0211010000000000" ;;
            13)
                read -rp "  ID (hex, ej: 7DF): " id
                read -rp "  DATA (hex, ej: 02010C0000000000): " data
                send "$id" "$data"
                ;;
            14)
                echo -e "${YELLOW}Monitoreando ${IFACE} (Ctrl+C para volver)...${NC}"
                timeout 5 candump "$IFACE" 2>/dev/null || true
                ;;
            q|Q) echo "Saliendo."; exit 0 ;;
            *) echo -e "${RED}Opción inválida${NC}" ;;
        esac
        sleep 0.3
    done
}

# ── Ejecutar ─────────────────────────────────────────────────────
check_can
echo -e "${GREEN}
   ______    __  __  _   __
  / ____/   / / / / / | / /
 / /   ___ / /_/ / /  |/ /
/ /___/ _ / __  / / /|  /
\\____/ // /_/ /_/ /_/ |_/
   /___/
   Multi-Brand CAN Diagnostic Client${NC}"
echo ""

# Si hay argumentos después del interfaz, ejecutar como comando único
if [ $# -gt 1 ]; then
    send "${@:2}"
    exit $?
fi

menu
