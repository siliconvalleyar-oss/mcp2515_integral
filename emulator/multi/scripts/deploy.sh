#!/bin/bash
# Despliega el ECU Multi-Emulator en una Raspberry Pi remota vía SSH/SCP
# Uso: ./scripts/deploy.sh [usuario@]host [opciones]

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

usage() {
    echo "Uso: $0 [usuario@]host [-k clave_ssh] [-p puerto] [-b|--build] [-c config.json]"
    echo ""
    echo "Argumentos:"
    echo "  host             IP o hostname de la Raspberry Pi"
    echo ""
    echo "Opciones:"
    echo "  -u usuario       Usuario SSH (o usar usuario@host)"
    echo "  -k KEY           Ruta a clave privada SSH"
    echo "  -p PORT          Puerto SSH (por defecto: 22)"
    echo "  -b, --build      Compilar antes de desplegar"
    echo "  --native         Compilar con g++ nativo (no cross)"
    echo "  -c FILE          Enviar archivo de configuración"
    echo "  -d DIR           Directorio destino en la RPi (por defecto: /opt/ecu_emulator)"
    echo "  -s, --setup      Ejecutar install_multi_emulator.sh en la RPi"
    echo "  -h, --help       Muestra esta ayuda"
    echo ""
    echo "Ejemplos:"
    echo "  $0 pi@192.168.1.100                     # desplegar binario"
    echo "  $0 192.168.1.100 -u pi -b               # compilar + desplegar"
    echo "  $0 pi@192.168.1.100 -c config.json -s   # config + setup remoto"
}

# ── Parsear argumentos ──────────────────────────────────────────
HOST=""
SSH_USER=""
SSH_KEY=""
SSH_PORT=22
DO_BUILD=false
NATIVE=false
CONFIG_FILE=""
REMOTE_DIR="/opt/ecu_emulator"
DO_SETUP=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) usage; exit 0 ;;
        -b|--build) DO_BUILD=true; shift ;;
        --native) NATIVE=true; shift ;;
        -k) SSH_KEY="$2"; shift 2 ;;
        -p) SSH_PORT="$2"; shift 2 ;;
        -u) SSH_USER="$2"; shift 2 ;;
        -c) CONFIG_FILE="$2"; shift 2 ;;
        -d) REMOTE_DIR="$2"; shift 2 ;;
        -s|--setup) DO_SETUP=true; shift ;;
        *)
            if [[ "$1" =~ ^[a-zA-Z0-9._-]+@[a-zA-Z0-9._-]+$ ]]; then
                HOST="$1"
            elif [[ "$1" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
                HOST="${SSH_USER:+${SSH_USER}@}${1}"
            else
                echo "[ERROR] Argumento desconocido: $1"
                usage; exit 1
            fi
            shift ;;
    esac
done

# ── Validar ──────────────────────────────────────────────────────
if [ -z "$HOST" ]; then
    echo -e "${RED}[ERROR] Especificar host: $0 usuario@host${NC}"
    usage; exit 1
fi

SSH_OPTS="-p ${SSH_PORT} -o StrictHostKeyChecking=no -o ConnectTimeout=10"
[ -n "$SSH_KEY" ] && SSH_OPTS="${SSH_OPTS} -i ${SSH_KEY}"

echo "╔══════════════════════════════════════════╗"
echo "║  Deploy ECU Multi-Emulator              ║"
echo "║  Target: ${HOST}${NC}"
echo "╚══════════════════════════════════════════╝"

# ── Compilar si se pidió ────────────────────────────────────────
if $DO_BUILD; then
    echo ""
    echo "━━━ Compilando ━━━"
    BUILD_ARGS="-j$(nproc)"
    $NATIVE && BUILD_ARGS="${BUILD_ARGS} -n"
    bash "${SCRIPT_DIR}/scripts/build.sh" ${BUILD_ARGS}
fi

# ── Verificar binario ───────────────────────────────────────────
BINARY="${SCRIPT_DIR}/build/ecu_emulator"
if [ ! -f "$BINARY" ]; then
    echo -e "${RED}[ERROR] Binario no encontrado: ${BINARY}${NC}"
    echo "  Compile primero: bash scripts/build.sh"
    exit 1
fi

# ── Probar conexión SSH ─────────────────────────────────────────
echo ""
echo "━━━ Probando conexión SSH ━━━"
if ! ssh ${SSH_OPTS} "$HOST" "echo OK" 2>/dev/null; then
    echo -e "${RED}[ERROR] No se pudo conectar a ${HOST}${NC}"
    echo "  Verifique:"
    echo "    - ssh-copy-id ${HOST}"
    echo "    - Puerto: ${SSH_PORT}"
    echo "    - Clave: ${SSH_KEY:-"(por defecto)"}"
    exit 1
fi
echo -e "  ${GREEN}✓ Conexión exitosa${NC}"

# ── Transferir archivos ─────────────────────────────────────────
echo ""
echo "━━━ Transfiriendo archivos ━━━"

ssh ${SSH_OPTS} "$HOST" "sudo mkdir -p ${REMOTE_DIR} && sudo chown ${SSH_USER:-\$USER} ${REMOTE_DIR}"

echo "  → Binario..."
scp ${SSH_OPTS} "$BINARY" "${HOST}:${REMOTE_DIR}/ecu_emulator"

echo "  → Configuración..."
scp ${SSH_OPTS} "${SCRIPT_DIR}/config.json" "${HOST}:${REMOTE_DIR}/config.json"

# Config personalizada si se especificó
if [ -n "$CONFIG_FILE" ] && [ -f "$CONFIG_FILE" ]; then
    scp ${SSH_OPTS} "$CONFIG_FILE" "${HOST}:${REMOTE_DIR}/config.json"
    echo "  → Config personalizada: $CONFIG_FILE"
fi

ssh ${SSH_OPTS} "$HOST" "chmod +x ${REMOTE_DIR}/ecu_emulator"

echo -e "  ${GREEN}✓ Archivos transferidos${NC}"

# ── Setup remoto ─────────────────────────────────────────────────
if $DO_SETUP; then
    echo ""
    echo "━━━ Setup remoto ━━━"
    ssh ${SSH_OPTS} "$HOST" "bash -s" < "${SCRIPT_DIR}/install_multi_emulator.sh" || {
        echo -e "${YELLOW}  ⚠ install_multi_emulator.sh falló, continuando...${NC}"
    }
fi

# ── Probar ejecución remota ─────────────────────────────────────
echo ""
echo "━━━ Verificando ─━━"
ssh ${SSH_OPTS} "$HOST" "cd ${REMOTE_DIR} && timeout 2 ./ecu_emulator --help 2>&1 || true"
echo ""

echo -e "${GREEN}✓ Deploy completado${NC}"
echo ""
echo "Comandos remotos:"
echo "  ssh ${HOST} 'sudo ${REMOTE_DIR}/ecu_emulator ${REMOTE_DIR}/config.json'"
echo "  ssh ${HOST} 'journalctl -u ecu_emulator -f'"
echo "  ssh ${HOST} 'candump can0'"
