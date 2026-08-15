---
name: prisma-emulator-legacy
description: |
  Use when working on the older Prisma emulator projects in legacy/ — legacy/prisma-openai (formerly mcp2515_openai) and legacy/prisma-claude (formerly prisma-emulator_clude, renamed from prisma-clude). Both emulate a Chevrolet Prisma ECU on Raspberry Pi + MCP2515 over SPI (libbcm2835), answering ELM327 AT commands and OBD2 requests. mcp2515_openai has SimulationMode {DYNAMIC, FIXED, RANDOM} and Profile {NORMAL, SPORT, ECONOMY, FAILSAFE}. prisma-emulator_clude has DrivingProfile {RALENTI, URBANO, CARRETERA, AGRESIVO, PERSONALIZADO} and a CAN polling loop. Neither has ISO-TP multi-frame. Use ONLY when the task targets these legacy emulators.
compatibility:
  - C++11
  - bcm2835 library
  - MCP2515 (SPI0)
  - Raspberry Pi
---

# Legacy Prisma Emulator Skill (mcp2515_openai + prisma-emulator_clude)

Historical projects — predecessors of `mcp2515_emulator_obd2_opencode`.
Same concept (Prisma ECU emulator) but simpler: single-frame OBD2 only, no
ISO-TP multi-frame, no Flow Control.

## Common
```bash
./scripts/install_dependencies.sh   # instala libbcm2835
make                                # -> bin/aplicacion
sudo ./bin/aplicacion               # o: make run (requiere root)
```
Both use CAN 11-bit 500 kbps; request 0x7DF/0x7E0, response 0x7E8.
Wiring: MOSI→SI, MISO→SO, SCLK→SCK, CE0→CS, GPIO25→INT.

## mcp2515_openai (v1, más temprana)
- `MCP2515` con `Bitrate {125K/250K/500K}`, `Mode {NORMAL/LOOPBACK/CONFIG/
  LISTEN_ONLY}`; CNF1/2/3 hardcodeados para 16 MHz (`0x00,0xD0,0x82`).
  Pins P1: CE0=`RPI_GPIO_P1_24`, INT=`RPI_GPIO_P1_22`.
- `Vehicle` con su **propio thread** de simulación; `SimulationMode
  {DYNAMIC, FIXED, RANDOM}`, `Profile {NORMAL, SPORT, ECONOMY, FAILSAFE}`;
  `setParameter(name)` con clamping; snapshot `Parameters`.
- `ELM327`: sin ISO-TP; el hilo CAN solo acepta frames modo 01 (`data[1]==0x01`),
  responde 0x7E8 DLC=8; PIDs 04/05/0A/0B/0C/0D/0F/10/11/1F/2F/42/A4(gear) +
  máscaras 00/20; respuestas con `\r\r>`.
- Build: `make` → `bin/aplicacion`.

## prisma-emulator_clude (v2, intermedia)
- `MCP2515` estilo C (`#define` registros), `CanFrame` con `rtr`,
  `CanBitrate {125K/250K/500K/1000K}`; `setNormalMode/LoopbackMode/
  ListenOnlyMode`, `hasMessage()` (CANINTF), `interruptPending()`.
- `Vehicle`: `update(dt)`, `DrivingProfile {RALENTI, URBANO, CARRETERA,
  AGRESIVO, PERSONALIZADO}`, `GearState {PARK..FIFTH}`; overrides FIJO
  (`setFixedValue/clearFixedValue`).
- `ELM327`: `pollCanRequests()` responde single-frame 0x7E8
  (`data[0]=2+len`) a peticiones 0x7DF/0x7E0; modos 01, 03 (0x00), 09/VIN.
- `main.cpp`: threads (simulación 10 Hz + CAN 20 ms) SOLO entre
  `startEmulation()`/`stopEmulation()`.
- Menú: 1 iniciar, 2 detener, 3 parámetros en tiempo real, 4 perfil,
  5 estado, 6 consola ELM327, 7 salir.

## Upgrade Path
When extending these, prefer porting to `mcp2515_emulator_obd2_opencode`
(ISO-TP multi-frame, FC handling, DTC modes, autotests) instead.
