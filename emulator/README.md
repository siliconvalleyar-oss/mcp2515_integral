# ECU Emulator — Rama `emulator`

Producto emulador del monorepo `mcp2515_integral`. Emula ECUs de vehículos en
un bus CAN y responde a peticiones OBD2/ELM327 de escáneres de diagnóstico.

## Componentes

| Carpeta | Proyecto | Hardware | Detalle |
|---|---|---|---|
| `prisma/` | Emulador ECU **Chevrolet Prisma** | MCP2515 SPI0 (bcm2835) | ELM327 AT + OBD2 + ISO-TP multi-frame + autotests. Ver `prisma/README.md` |
| `multi/` | **ECU Multi-Emulator** (8 marcas) | SocketCAN (can0/vcan0) | OBD2 + UDS + GMLAN/KWP2000/CAN-TP + REST/WebSocket/gRPC + SQLite. Ver `multi/README.md` |

## Build rápido

```bash
# Prisma (requiere Raspberry Pi + libbcm2835)
cd prisma && make && sudo make run

# Multi (requiere SocketCAN)
cd multi && make              # o make CROSS_COMPILE= para nativo
sudo modprobe vcan && sudo ip link add vcan0 type vcan && sudo ip link set vcan0 up
./ecu_emulator
```

## Skills

- `.opencode/skills/prisma-emulator-opencode` — `prisma/`
- `.opencode/skills/ecu-multi-emulator` — `multi/`
- `.opencode/skills/prisma-emulator-legacy` — referencia `legacy/prisma-*`

## Reglas

- Compilar y probar **solo en la Raspberry Pi**.
- Versionado: tag `vX.Y.Z` == archivo `VERSION`; ciclo patch 0-9; todo push
  con tag. Ver `AGENTS.md` de la raíz.
- MCP2515: verificar cristal 8/16 MHz (`MCP2515_OSC_HZ` en `prisma`).
- El driver kernel `mcp251x` y bcm2835 compiten por el módulo SPI: usar uno a
  la vez.
