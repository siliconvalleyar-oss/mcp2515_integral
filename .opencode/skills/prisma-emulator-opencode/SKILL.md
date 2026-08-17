---
name: prisma-emulator-opencode
description: |
  Use when working on the Chevrolet Prisma ECU emulator (emulator/prisma en el monorepo; nombre histórico mcp2515_emulator_obd2_opencode) — Raspberry Pi + MCP2515 CAN over SPI (libbcm2835). Covers the MCP2515 driver, Vehicle simulator (~37 params, profiles Idle/City/Highway/Sport), ELM327 AT-command emulation, OBD2 modes 01-0A + UDS services 19/14/22/31, ISO-TP single/multi-frame with Flow Control, CAN thread + 10 Hz simulation thread, autotests SPI/loopback/bus, and the real-vehicle trace alignment (docs/SCANNER_TRACE_ONIX.md). Use ONLY when the task targets this emulator.
compatibility:
  - C++11
  - bcm2835 library
  - MCP2515 (16 MHz default, MCP2515_OSC_HZ)
  - Raspberry Pi / SPI0 / GPIO25 INT
---

# Chevrolet Prisma ECU Emulator Skill (emulator/prisma)

## Quick Reference
```bash
make                            # -> bin/emulator_prisma_<32|64> (uname -m)
make install-bcm2835            # instala libbcm2835 (bcm2835-1.68) si falta
sudo make run                   # requiere root (/dev/mem)
make kill                       # mata instancias activas (scripts/kill_apps.sh)
make MCP2515_OSC_HZ=8000000     # compila para cristal de 8 MHz
make test-build && make test    # autotests SPI/loopback/bus con sudo
make test-socketcan             # prueba alternativa con SocketCAN (sudo)
make CXX=arm-linux-gnueabihf-g++  # cross-compile
```

## Structure
```
include/: mcp2515.h (driver), vehicle.h (modelo+simulador+consola), elm327.h
src/: mcp2515.cpp, vehicle.cpp, elm327.cpp, main.cpp (hilos + menú)
test/: autotest.{h,cpp}, test_spi.cpp, test_loopback.cpp, test_bus.cpp
scripts/: run_tests.sh, can_kernel_test.sh, kill_apps.sh (make kill),
          install_dependencies.sh
docs/: SKILLS.md, ECU_PARAMETERS.md (catálogo de respuestas, fuente de verdad),
       SCANNER_TRACE_ONIX.md (traza del Onix real), BUG_REPORT.md
```

## MCP2515 Driver
- `MCP2515(cs=CE0, intPin=25)`, multi-instancia. `begin()/end()` (init global)
  y `beginExisting()/endLight()` (autotest tras reset). `sendMessage` (TXB0,
  one-shot), `receiveMessage` (RXB0/RXB1, rollover BUKT), `isInterruptPending`.
- Diag: `readStatus()`, `errorCountTx/Rx` (TEC/REC), `errorFlags()` (EFLG).
- Bit timing tabla `kTimings[]`; 16MHz/500k = `0x00,0xF0,0x86`. SPI con mutex.

## Vehicle Simulator
- `Vehicle`: ~37 params, `value/setValue/isAuto/setAuto` NO bloquean (tomar
  `veh.mtx`). `Simulator` a 10 Hz, `Profile {Idle, City, Highway, Sport}`,
  fases `{Accel, Cruise, Decel, Stop}`; FIJO vs AUTO por parámetro. Params
  añadidos: stft2, ltft2, odometro, torque, oil_life, inyector_pw, etanol,
  presion_tanque, misfire_actual/hist, knock_retard, balance_rate, temp_atf, afr.

## ELM327 / OBD2 — modos 01-0A
- AT: ATZ/ATRST→"ELM327 v1.5", ATE/ATL/ATH/ATS/ATR, ATRV, ATDP/ATDPN→"A6",
  ATSPn (solo 6 físico), ATSH/ATCRA, ATFCSH/FCSM/FCSD, **ATMM (consulta
  máscara), ATMM0 (real) / ATMM1 (full)**; resto por compatibilidad; desconocido→`?`.
- **Encodings modo 01 (SAE J1979, corregidos 2026-08)**: el emulador codifica
  el valor al revés → la app mostraba valores absurdos (840 rpm → ~50 rpm).
  Regla: **raw = valor físico × escala inversa** — 0x0C RPM raw16 = rpm×4;
  0x0E avance raw = (°+64)×2; 0x42 batería raw = V×10; 0x10 MAF raw = g/s×100;
  0x44 λ raw16 = λ×32768; 0x53 presión tanque raw16 firmado (offset 0x8000,
  kPa×250); O2 (13-1A) A = V×200, B = 128 + %×128/100; trims 06-09 A =
  128 + %×128/100; temps A = °C+40; PIDs 56-59 misfire nibble por cilindro.
- **Máscaras real/full** (`ATMM0` default = idénticas al Onix real según la
  traza): real `0100→BE 3F B8 13`, `0140→FE D2 80 00`; full (ATMM1)
  `0100→BF FF BF D2`, `0140→5E 94 67 90`; `0120→80 06 80 00`, `0160→0`.
  PIDs `41/43/4A/4F` responden ceros como el real; `51` = 0x01 (gasolina).
  Los PIDs implementados responden aunque no se anuncien (el AUTEL sondea).
- PIDs implementados: 01, 03-10, 11, 13-1A, 1C, 1F, 21, 2E, 2F, 31, 33, 42,
  44, 45, 46, 47, 49, 4C, 4E (marcha custom 0=N,1-5,6=R), 51, 52, 53, 56-59,
  5C. Multi-PID (2-6/trama). Respuesta física 0x7E9, funcional 0x7E8.
- Modo 02: `02 02` → `42 02 <DTC> <máscara 4B>` (freeze frame); resto = datos
  actuales con prefijo 42.
- Modo 06: formato ISO 15031-5:2006+ `46 <TID> <TestValue:2> <MinLimit:2>
  <MaxLimit:2> <Unit:1> <TestID:1> <OTI:2>`; TID `00` → máscara 4B
  `C0 00 00 00` (TIDs 01/02); TIDs 01/02/41/61/91 con valores plausibles.
- Modo 08 (control de sistemas): `48 <TID> <Data A..E>` (prueba completada,
  sin falla) para EVAP `01`, EVAP purge/vent `02`, fan relay `03`, fuel pump
  relay `04`, A/C clutch `05` (03-05 extensión del emulador); TID desconocido
  → NRC `7F 08 <TID> 12`. El Onix real responde NO DATA (superconjunto).
- Modo 09: `0900→49 00 03 50 40 00 00` (PIDs 02/04/0A); 02 VIN
  `9BGKL48T0HB130763`; 04 CALID `1505708\0 52124404\0` (terminadores nulos);
  0A nombre ECU `TCM-Engine Control` (20 chars + relleno, 23 bytes — como el
  Onix real).

## Modo 22 UDS (DIDs GM) y servicios UDS
- `22 <DID>` → `62 <DID> <datos>`; DID no soportado → NRC `7F 22 <DID> 31`.
  Despachado en `handleCanRequest()` (CAN) y la consola. Catálogo completo en
  `docs/ECU_PARAMETERS.md` (fuente de verdad).
- DIDs implementados: `B100` odómetro (raw32/10 km), `01A9` torque
  (raw16×0.5−848 Nm), `01B4` temp. catalizador (raw16×0.1−40), `1180` presión
  combustible (raw16×4), `01A1` voltaje (raw16×0.001), `119F` oil life
  (% = raw×200/51), `1193-119A` inyector (ms = raw×200/131), `11A1` tiempo
  (raw16 = s), `11A6` knock retard (° = raw×45/50), `1251`/`119D` baro
  (inHg = raw×3 → kPa×3.386), `162F-1636` balance rate (mm³ = raw×5/32−20),
  `1940` TFT (**1 byte, raw = °C+40** — confirmado con traza real
  `62 19 40 23` → −5 °C), `19DE` torque alt (raw = ft-lbs, ×1.3558 → Nm),
  `119E` AFR (raw16×0.01, por confirmar), `1564` → 0x29, `1201` → 0000,
  `2345` → 00.
- **Fórmulas CANSF verificadas (2026-08)**: el MTH de X-Gauge se interpreta
  como `valor = raw×A/B + C` (C complemento a 2; ej. `00010001FFD8` = raw−40).
  Pendientes contra el escáner real: AFR 119E (×1 vs ×0.01), TRQ 19DE
  (raw×5 vs ft-lbs directo).
- Servicios UDS: `19 02 <máscara>` → `59 02 01 FF <n> <DTC+estado>...`
  (estado 0x09 confirmado+activo, 0x04 pendiente; multi-frame si n > 2);
  `14 FF FF FF` → `54` (limpia DTCs/MIL/calentamientos/distancia, igual que
  modo 04); `31 01 C1 0F` → `71 01 C1 0F` (reset adaptativos: pone
  `ltft1/ltft2` en 0). Despachados en `handleCanRequest()` y la consola.
- **TCM (segunda ECU del bus)**: peticiones a `0x7E1`/`0x7E2` → respuestas
  desde `0x7E9`/`0x7EA`, solo modo 22 (`getTcmMode22`): `1940` TFT (confirmado,
  también en ECM), `11E0` ISS, `11E1` OSS, `11E2` TCC slip, `11E3` gear ratio
  (raw16×0.01), `11E4` marcha, `11E5-11E7` shift times, `11E8` last shift,
  `11E9-11EB` shift errors (DIDs candidatos por confirmar). Direccionamiento
  ISO 15765-4: ECM física `0x7E0` → `0x7E8`; funcional `0x7DF` → `0x7E8`.
  Consola: `ATSH 7E2` + `22 19 40`.
- **Broadcast (100 Hz)**: `sendBroadcastFrames()` publica `0x320` (motor: RPM/
  TPS/carga/ECT/VSS) y `0x328` (transmisión: marcha/ISS/OSS/TCC slip) cada
  10 ms desde el hilo CAN — el emulador no solo responde peticiones, "hace
  ruido" en el bus como un vehículo real. `ATBC0`/`ATBC1` apagan/encienden
  (default ON), `ATBC` consulta. Layout propio, por confirmar.

## Threads & Menu (main.cpp)
- CAN thread: poll INT + receive, despacha 0x7DF/0x7E0→handleCanRequest,
  0x7E8/0x7E9→handleCanResponse. Pausable (g_canPaused) durante autotest.
- Sim thread 10 Hz. Menu: 1 iniciar, 2 detener, 3 perfil, 4 FIJO/AUTO,
  5 estado, 6 consola ELM327, 7 info (CANSTAT/EFLG/TEC/REC), 8 autotest,
  9 monitor ANSI, 0 salir.

## Alineación con el vehículo real (docs/SCANNER_TRACE_ONIX.md)
- La traza del Onix real (mismo VIN que el emulador) reveló: máscara real
  `0100→BE 3F B8 13` / `0140→FE D2 80 00`; nombre ECU `TCM-Engine Control`;
  TFT `1940` = 1 byte °C+40; solo 5 DIDs responden (`1564`, `1940`, `11A1`,
  `1201`, `2345`); los no soportados → NO DATA (el real NO responde NRC);
  `014F` responde ceros. El emulador es un superconjunto (implementa los
  DIDs CANSF de ScanGauge que el real no tiene).

## Autotests
- `test_spi` (cableado, registro R/W, detección cristal via CLKOUT→GPIO26),
  `test_loopback` (TX/RX + INT), `test_bus` (2 módulos CE0/CE1, ráfaga 100).
- Runner: `sudo ./scripts/run_tests.sh [--spi|--loopback|--bus|--socketcan|--build]`.
- Fallos: `CANSTAT=0xFF`=sin respuesta; `0x00`+RESET bajo=chip en reset,
  +RESET alto=alimentación/cristal. Sondas: RESET→GPIO27, CLKOUT→GPIO26.
- SocketCAN alternativo: `scripts/can_kernel_test.sh --setup [--bus] [--osc=...]`.

## kill_apps.sh
- `make kill` mata las instancias activas del emulador (mismo binario de
  `make run`): SIGTERM → SIGKILL → verificación. `--all` (también las
  pruebas test_spi/loopback/bus), `--check` (solo lista), `--help`.

## Remote Workflow (reglas del proyecto)
- Editar/commit local; compilar/probar SOLO en la Pi (`pi@raspi.local`),
  git pull + make + make run. No compilar en local.
- Versionado: tag `vX.Y.Z` == `VERSION` (sin v); ciclo patch 0-9 (v1.0.9→v1.1.0);
  todo push con tag; conventional commits.

See `docs/SKILLS.md` for the full compendium, `docs/ECU_PARAMETERS.md` (mapa
de respuestas, checklist de implementación) y `docs/BUG_REPORT.md` before
touching code.
