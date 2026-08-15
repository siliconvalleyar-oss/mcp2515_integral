#!/usr/bin/env bash
# ============================================================================
#  can_kernel_test.sh
#  Verificación alternativa usando el driver de kernel SocketCAN (mcp251x),
#  en lugar del acceso directo por bcm2835 que usa el emulador.
#
#  Uso:
#    sudo ./scripts/can_kernel_test.sh            loopback del kernel en can0
#    sudo ./scripts/can_kernel_test.sh --bus      can0 <-> can1 (dos módulos)
#    sudo ./scripts/can_kernel_test.sh --setup    configura las overlays
#                                                 mcp2515 (edita /boot/config.txt
#                                                 y requiere reinicio)
#    sudo ./scripts/can_kernel_test.sh --osc=8000000   cristal del módulo
#    sudo ./scripts/can_kernel_test.sh --help
#
#  IMPORTANTE: el driver de kernel mcp251x y el emulador (bcm2835 directo)
#  compiten por el mismo módulo SPI/GPIO: use UNO a la vez.
#  Salida: 0 = OK, 1 = fallo, 2 = uso incorrecto.
# ============================================================================
set -u

BITRATE=500000
OSC=8000000
DO_SETUP=0
DO_BUS=0

usage() {
    sed -n '7,18p' "$0"
}

for arg in "$@"; do
    case "$arg" in
        --bus)      DO_BUS=1 ;;
        --setup)    DO_SETUP=1 ;;
        --osc=*)    OSC="${arg#--osc=}" ;;
        --help|-h)  usage; exit 0 ;;
        *)          echo "Opción desconocida: $arg"; usage; exit 2 ;;
    esac
done

if [ "$(id -u)" != "0" ]; then
    echo ">> Re-ejecutando con sudo (acceso a /sys/class/net)..."
    exec sudo "$0" "$@"
fi

# Raspberry Pi OS moderno usa /boot/firmware/config.txt (Bookworm+).
if [ -f /boot/firmware/config.txt ]; then
    CONFIG=/boot/firmware/config.txt
else
    CONFIG=/boot/config.txt
fi

need_tools() {
    local missing=""
    local t
    for t in ip cansend candump modprobe; do
        command -v "$t" >/dev/null 2>&1 || missing="$missing $t"
    done
    if [ -n "$missing" ]; then
        echo "ERROR: faltan herramientas:$missing"
        echo "Instale can-utils e iproute2:"
        echo "  sudo apt install -y can-utils iproute2"
        exit 1
    fi
}

# ----------------------------------------------------------------------------
#  --setup: activa las overlays mcp2515 (SPI + can0 [+ can1])
# ----------------------------------------------------------------------------
if [ "$DO_SETUP" = "1" ]; then
    need_tools
    echo ">> Configurando SocketCAN mcp2515 en $CONFIG"
    [ -f "$CONFIG" ] || touch "$CONFIG"
    cp -a "$CONFIG" "$CONFIG.bak" 2>/dev/null && \
        echo "   (copia de seguridad: $CONFIG.bak)"

    if grep -q '^dtparam=spi=on' "$CONFIG"; then
        echo "   dtparam=spi=on ya presente"
    elif grep -q '^#dtparam=spi=on' "$CONFIG"; then
        sed -i 's/^#dtparam=spi=on/dtparam=spi=on/' "$CONFIG"
        echo "   dtparam=spi=on habilitado (estaba comentado)"
    else
        echo 'dtparam=spi=on' >> "$CONFIG"
        echo "   dtparam=spi=on añadido"
    fi

    if ! grep -q 'dtoverlay=mcp2515-can0' "$CONFIG"; then
        echo "dtoverlay=mcp2515-can0,oscillator=$OSC,interrupt=25" >> "$CONFIG"
        echo "   overlay can0 (CE0, INT 25, osc $OSC) añadida"
    else
        echo "   overlay can0 ya presente"
    fi
    if [ "$DO_BUS" = "1" ]; then
        if ! grep -q 'dtoverlay=mcp2515-can1' "$CONFIG"; then
            echo "dtoverlay=mcp2515-can1,oscillator=$OSC,interrupt=24" >> "$CONFIG"
            echo "   overlay can1 (CE1, INT 24, osc $OSC) añadida"
        else
            echo "   overlay can1 ya presente"
        fi
    fi

    echo ""
    echo ">> Configuración lista. REINICIE la Raspberry Pi:  sudo reboot"
    echo "   Tras reiniciar, ejecute:  sudo ./scripts/can_kernel_test.sh"
    exit 0
fi

# ----------------------------------------------------------------------------
#  Pruebas
# ----------------------------------------------------------------------------
need_tools
modprobe can 2>/dev/null
modprobe can_raw 2>/dev/null
modprobe mcp251x 2>/dev/null

cleanup() {
    ip link set can0 down 2>/dev/null
    ip link set can1 down 2>/dev/null
}
trap cleanup EXIT

if ! ip link show can0 >/dev/null 2>&1; then
    echo "ERROR: la interfaz can0 no existe."
    echo "Active la overlay mcp2515 (una sola vez):"
    echo "  sudo ./scripts/can_kernel_test.sh --setup"
    echo "  sudo reboot"
    exit 1
fi

# ---------------- Loopback del kernel (sin bus ni segundo nodo) ------------
run_loopback() {
    echo ">> Prueba loopback del kernel en can0 (${BITRATE} bps, osc $OSC)..."
    echo "   No necesita bus ni segundo nodo."
    if ! ip link set can0 up type can bitrate "$BITRATE" loopback on; then
        echo " [XX] ERROR: no se pudo levantar can0 en modo loopback."
        return 1
    fi
    sleep 0.5

    local cap
    cap="$(mktemp)"
    local ok=1
    timeout 3 candump can0 -n 1 > "$cap" 2>/dev/null &
    local dumper=$!
    sleep 0.3
    cansend can0 123#DEADBEEF01020304 || ok=0
    wait "$dumper"

    if [ "$ok" = "1" ] && grep -qi "DE AD BE EF 01 02 03 04" "$cap"; then
        echo " [OK] Trama 123 recibida con datos correctos"
    else
        echo " [XX] No se recibió la trama esperada"
        sed 's/^/      /' "$cap" | head -3
        ok=0
    fi
    rm -f "$cap"
    ip link set can0 down 2>/dev/null
    if [ "$ok" = "1" ]; then return 0; else return 1; fi
}

# ---------------- Bus real entre dos interfaces ----------------------------
run_bus() {
    if ! ip link show can1 >/dev/null 2>&1; then
        echo "ERROR: la interfaz can1 no existe."
        echo "Se necesita un segundo módulo MCP2515 (CE1, GPIO24):"
        echo "  sudo ./scripts/can_kernel_test.sh --setup --bus"
        echo "  sudo reboot"
        return 1
    fi

    local iface
    for iface in can0 can1; do
        if ! ip link set "$iface" up type can bitrate "$BITRATE"; then
            echo " [XX] ERROR: no se pudo levantar $iface"
            return 1
        fi
    done
    sleep 0.5

    local ok=1 cap dumper i n

    # can0 -> can1
    cap="$(mktemp)"
    timeout 3 candump can1 -n 1 > "$cap" 2>/dev/null &
    dumper=$!
    sleep 0.3
    cansend can0 456#123456789ABCDEF0 || ok=0
    wait "$dumper"
    if [ "$ok" = "1" ] && grep -qi "456" "$cap" && \
       grep -qi "12 34 56 78 9A BC DE F0" "$cap"; then
        echo " [OK] can0 -> can1: trama 456 recibida con datos correctos"
    else
        echo " [XX] can0 -> can1: no se recibió la trama"
        sed 's/^/      /' "$cap" | head -3
        ok=0
    fi
    rm -f "$cap"

    # can1 -> can0
    cap="$(mktemp)"
    timeout 3 candump can0 -n 1 > "$cap" 2>/dev/null &
    dumper=$!
    sleep 0.3
    cansend can1 789#FEDCBA9876543210 || ok=0
    wait "$dumper"
    if [ "$ok" = "1" ] && grep -qi "789" "$cap" && \
       grep -qi "FE DC BA 98 76 54 32 10" "$cap"; then
        echo " [OK] can1 -> can0: trama 789 recibida con datos correctos"
    else
        echo " [XX] can1 -> can0: no se recibió la trama"
        sed 's/^/      /' "$cap" | head -3
        ok=0
    fi
    rm -f "$cap"

    # Ráfaga de 100 tramas can0 -> can1 con contador de secuencia
    cap="$(mktemp)"
    timeout 10 candump can1 -n 100 > "$cap" 2>/dev/null &
    dumper=$!
    sleep 0.3
    for i in $(seq 0 99); do
        cansend can0 "5A0#$(printf '%02X' "$i")A5A5A5A5A5A5" || ok=0
    done
    wait "$dumper"
    n="$(wc -l < "$cap")"
    if [ "$ok" = "1" ] && [ "$n" -eq 100 ]; then
        echo " [OK] Ráfaga: 100 tramas can0 -> can1 sin pérdidas"
    else
        echo " [XX] Ráfaga: se recibieron $n de 100 tramas"
        ok=0
    fi
    rm -f "$cap"

    ip link set can0 down 2>/dev/null
    ip link set can1 down 2>/dev/null
    if [ "$ok" = "1" ]; then return 0; else return 1; fi
}

fail=0
if [ "$DO_BUS" = "1" ]; then
    run_bus || fail=1
else
    run_loopback || fail=1
fi

echo ""
if [ "$fail" = "0" ]; then
    echo "RESULTADO: SOCKETCAN OK"
else
    echo "RESULTADO: HUBO FALLOS"
    echo "Revise el cableado CANH/CANL (y 120 ohmios por extremo), el cristal"
    echo "del módulo (--osc) y que no esté corriendo el emulador a la vez."
fi
exit "$fail"
