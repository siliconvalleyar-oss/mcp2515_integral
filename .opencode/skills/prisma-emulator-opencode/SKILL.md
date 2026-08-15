---
name: prisma-emulator-opencode
description: |
  Use when working on the Prisma ECU emulator project (./emulator/prisma, formerly mcp2515_emulator_obd2_opencode) — the newest Chevrolet Prisma ECU emulator for Raspberry Pi + MCP2515 CAN controller over SPI (libbcm2835). Covers the MCP2515 driver, Vehicle simulator (profiles Idle/City/Highway/Sport), ELM327 AT-command emulation, OBD2 modes 01-0A, ISO-TP single/multi-frame with Flow Control, CAN thread + 10 Hz simulation thread, and the SPI/loopback/2-module-bus autotests. Use ONLY when the task targets this specific emulator project.
compatibility:
  - C++11
  - bcm2835 library
  - MCP2515 (16 MHz default, MCP2515_OSC_HZ)
  - Raspberry Pi / SPI0 / GPIO25 INT
---

# Chevrolet Prisma ECU Emulator Skill (mcp2515_emulator_obd2_opencode)

## Quick Reference
```bash
make                            # -> ./prisma-obd-emulator
make install-bcm2835            # instala libbcm2835 (bcm2835-1.68) si falta
sudo make run                   # requiere root (/dev/mem)
make MCP2515_OSC_HZ=8000000     # compila para cristal de 8 MHz
make test-build && make test    # autotests SPI/loopback/bus con sudo
make test-socketcan             # prueba alternativa con driver kernel mcp251x
make CXX=arm-linux-gnueabihf-g++  # cross-compile
```

## Structure
```
include/: mcp2515.h (driver), vehicle.h (modelo+simulador+consola), elm327.h
src/: mcp2515.cpp, vehicle.cpp, elm327.cpp, main.cpp (hilos + menú)
test/: autotest.{h,cpp}, test_spi.cpp, test_loopback.cpp, test_bus.cpp
scripts/: run_tests.sh, can_kernel_test.sh, install_dependencies.sh
```

## MCP2515 Driver
- `MCP2515(cs=CE0, intPin=25)`, multi-instancia. `begin()/end()` (init global)
  y `beginExisting()/endLight()` (autotest tras reset). `sendMessage` (TXB0,
  one-shot), `receiveMessage` (RXB0/RXB1, rollover BUKT), `isInterruptPending`.
- Diag: `readStatus()`, `errorCountTx/Rx` (TEC/REC), `errorFlags()` (EFLG).
- Bit timing tabla `kTimings[]`; 16MHz/500k = `0x00,0xF0,0x86`. SPI con mutex.

## Vehicle Simulator
- `Vehicle`: 23 params, `value/setValue/isAuto/setAuto` NO bloquean (tomar
  `veh.mtx`). `Simulator` a 10 Hz, `Profile {Idle, City, Highway, Sport}`,
  fases `{Accel, Cruise, Decel, Stop}`; FIJO vs AUTO por parámetro.

## ELM327 / OBD2
- AT: ATZ/ATRST→"ELM327 v1.5", ATE/ATL/ATH/ATS/ATR, ATRV, ATDP/ATDPN→"A6",
  ATSPn (solo 6 físico), ATSH/ATCRA (IDs 3/6 hex), ATFCSH/FCSM/FCSD; resto
  por compatibilidad; desconocido→`?`.
- Modes: 01/02 (40+ PIDs, custom `0x4E`=gear 0=N 1-5 6=R), 03/07/0A DTCs
  (`0x0301`=P0301), 04 clear, 06 monitors (TID 01/02/41/61/91), 08 negativa,
  09 VIN `9BGKL48T0HB130763`, CALIDs, `GM PRISMA 1.4`.
- Multi-PID (2-6/trama). Respuesta física 0x7E9, funcional 0x7E8.
- ISO-TP: SF ≤7B; multi-frame espera FC (300ms, acepta de cualquier ID —
  fix BUG-01); frames no-FC bufferizados + `drainPending()`.

## Threads & Menu (main.cpp)
- CAN thread: poll INT + receive, despacha 0x7DF/0x7E0→handleCanRequest,
  0x7E8/0x7E9→handleCanResponse. Pausable (g_canPaused) durante autotest.
- Sim thread 10 Hz. Menu: 1 iniciar, 2 detener, 3 perfil, 4 FIJO/AUTO,
  5 estado, 6 consola ELM327, 7 info (CANSTAT/EFLG/TEC/REC), 8 autotest,
  9 monitor ANSI, 0 salir.

## Autotests
- `test_spi` (cableado, registro R/W, detección cristal via CLKOUT→GPIO26),
  `test_loopback` (TX/RX + INT), `test_bus` (2 módulos CE0/CE1, ráfaga 100).
- Runner: `sudo ./scripts/run_tests.sh [--spi|--loopback|--bus|--socketcan|--build]`.
- Fallos: `CANSTAT=0xFF`=sin respuesta; `0x00`+RESET bajo=chip en reset,
  +RESET alto=alimentación/cristal. Sondas: RESET→GPIO27, CLKOUT→GPIO26.
- SocketCAN alternativo: `scripts/can_kernel_test.sh --setup [--bus] [--osc=...]`
  (overlay mcp251x; kernel y bcm2835 compiten por el módulo).

## Remote Workflow (reglas del proyecto)
- Editar/commit local; compilar/probar SOLO en la Pi (`pi@raspi.local`,
  LINK=/home/pi/src/mcp2515_emulator_rpi), git pull + make + make run.
- Versionado: tag `vX.Y.Z` == `VERSION` (sin v); ciclo patch 0-9 (v1.0.9→v1.1.0);
  todo push con tag; conventional commits.

See `docs/SKILLS.md` for the full compendium and `docs/BUG_REPORT.md` (16
hallazgos P0-P3, auditoría pendiente) before touching code.
