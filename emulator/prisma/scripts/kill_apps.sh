#!/usr/bin/env bash
# ============================================================================
#  Mata las instancias activas del emulador OBD2 (las lanzadas con make run o
#  ejecutando el binario a mano), y opcionalmente las pruebas de comunicación
#  que hayan quedado corriendo (test_spi / test_loopback / test_bus).
#
#  Uso:
#    ./scripts/kill_apps.sh            mata el emulador (emulator_prisma_32/64)
#    ./scripts/kill_apps.sh --all      mata además las pruebas de comunicación
#    ./scripts/kill_apps.sh --check    solo lista lo que está corriendo
#    ./scripts/kill_apps.sh --help
#
#  Desde el Makefile:  make kill
#  Requiere sudo si el emulador se lanzó con make run (root).
#  Salida: 0 = no había nada / todo terminado, 1 = quedaron procesos activos.
# ============================================================================
set -u

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Mismo binario que el Makefile: bin/emulator_prisma_<sufijo> según uname -m.
if [ "$(uname -m)" = "aarch64" ]; then
    EMU_PATTERN="emulator_prisma_64"
else
    EMU_PATTERN="emulator_prisma_32"
fi
TEST_PATTERNS="test_spi|test_loopback|test_bus"

KILL_TESTS=0
CHECK_ONLY=0

usage() {
    sed -n '5,14p' "$0"
}

for arg in "$@"; do
    case "$arg" in
        --all)    KILL_TESTS=1 ;;
        --check)  CHECK_ONLY=1 ;;
        --help|-h) usage; exit 0 ;;
        *) echo "Opción desconocida: $arg"; usage; exit 2 ;;
    esac
done

# El emulador puede correr como root (make run) -> re-ejecutar con sudo.
if [ "$(id -u)" != "0" ]; then
    echo ">> Re-ejecutando con sudo (procesos del emulador)..."
    exec sudo "$0" "$@"
fi

if [ "$CHECK_ONLY" = "1" ]; then
    echo ">> Procesos activos del emulador:"
    pgrep -af "$EMU_PATTERN" || echo "   (ninguno)"
    if [ "$KILL_TESTS" = "1" ]; then
        echo ">> Procesos activos de pruebas:"
        pgrep -af "$TEST_PATTERNS" || echo "   (ninguno)"
    fi
    exit 0
fi

# 1) Señal suave (SIGTERM) a todo lo que esté corriendo.
term=0
for pid in $(pgrep -f "$EMU_PATTERN"); do
    kill -TERM "$pid" 2>/dev/null && term=$((term + 1))
done
if [ "$KILL_TESTS" = "1" ]; then
    for pid in $(pgrep -f "$TEST_PATTERNS"); do
        kill -TERM "$pid" 2>/dev/null && term=$((term + 1))
    done
fi
[ "$term" -gt 0 ] && sleep 2

# 2) Reintento con SIGKILL para los que no terminaron.
force=0
for pid in $(pgrep -f "$EMU_PATTERN"); do
    if kill -0 "$pid" 2>/dev/null; then
        kill -KILL "$pid" 2>/dev/null && force=$((force + 1))
    fi
done
if [ "$KILL_TESTS" = "1" ]; then
    for pid in $(pgrep -f "$TEST_PATTERNS"); do
        if kill -0 "$pid" 2>/dev/null; then
            kill -KILL "$pid" 2>/dev/null && force=$((force + 1))
        fi
    done
fi

if [ "$term" -eq 0 ] && [ "$force" -eq 0 ]; then
    echo ">> No había instancias del emulador corriendo."
else
    echo ">> Terminados: $term (SIGTERM), forzados: $force (SIGKILL)."
fi

# Verificación final: si quedó algo, avisar y fallar.
remaining=$(pgrep -f "$EMU_PATTERN" | wc -l)
if [ "$remaining" -gt 0 ]; then
    echo ">> AVISO: aún quedan procesos: $(pgrep -af "$EMU_PATTERN" | tr '\n' ' ')"
    exit 1
fi
exit 0
