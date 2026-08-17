---
name: mcp2515-raspberry-projects
description: |
  Use when working on any product in the mcp2515_integral monorepo — Raspberry Pi CAN/OBD2 automotive projects in C++17. Covers: scanner/reader (OBD2 reader + SSD1306 OLED over Bluetooth ELM327), emulator/prisma (newest Chevrolet Prisma ECU emulator, MCP2515 SPI + ELM327 AT + OBD2 + ISO-TP), scanner/autel_scanner (AUTEL-style diagnostic scanner with SSD1306), emulator/multi (multi-brand ECU emulator via SocketCAN with REST/WebSocket/gRPC APIs). Legacy projects (legacy/prisma-openai, legacy/prisma-claude, legacy/scanner-duplicate) live ONLY on the main branch. Branches: main (hub+legacy), emulator (emulators), scanner (scanner+reader). Use ONLY when the task targets one of these Raspberry Pi MCP2515/OBD2 projects.
compatibility:
  - C++11/C++17
  - Raspberry Pi (1-5, 2W)
  - MCP2515 CAN controller (SPI)
  - bcm2835 library
  - SocketCAN (Linux)
  - SSD1306 OLED (SPI/I2C)
---

# MCP2515 Raspberry Pi OBD2/CAN Projects — Workspace Skill

Guía de referencia de los 7 proyectos del workspace `MCP2515/`. Cada proyecto
es un sistema automotriz sobre Raspberry Pi que habla **OBD2/ELM327/CAN**.

## Mapa del Workspace

| Proyecto | Rol | Hardware | Estado |
|---|---|---|---|
| `elm327_rpi2w/` | **Lector OBD2** + display OLED SSD1306 (SPI) | ELM327 Bluetooth (RFCOMM) + Pi 2W | Activo, v2.0.0 |
| `mcp2515_emulator_obd2_opencode/` | **Emulador ECU Chevrolet Prisma** (el más nuevo) | MCP2515 SPI0 (bcm2835) | Activo, con autotests |
| `mcp2515_openai/` | Emulador Prisma (versión temprana) | MCP2515 SPI0 (bcm2835) | Histórico |
| `prisma-emulator_clude/` | Emulador Prisma (versión intermedia) | MCP2515 SPI0 (bcm2835) | Histórico |
| `mcp2515_scanner_rpi/` | **Scanner AUTEL** (lado lector) | MCP2515 + SSD1306 I2C | Activo |
| `raspberry_pi_scanner/` | Scanner AUTEL (misma copia, con repo git) | MCP2515 + SSD1306 I2C | Duplicado |
| `mcp2515_rpi/` | **ECU Multi-Emulator** (8 marcas, APIs) | SocketCAN (can0/vcan0) | Activo, v1.0.0 |

> Los dos scanners (`mcp2515_scanner_rpi/` y `raspberry_pi_scanner/`) son
> código byte-idéntico. `raspberry_pi_scanner` es el "upstream" con repo git.

## Conceptos CAN/OBD2 comunes (todos los proyectos)

- **IDs ISO 15765-4 (CAN 11-bit):** request funcional `0x7DF`, request físico
  `0x7E0`, respuestas `0x7E8`/`0x7E9` (físico desde 0x7E0).
- **Protocolo ELM327:** `SP6` = ISO 15765-4 CAN 11-bit @ 500 kbps.
- **ISO-TP:** Single Frame (PCI `0x00|len`, ≤7 bytes), First Frame
  (`0x10|hi`, lo, +6), Flow Control (`0x30`), Consecutive Frames
  (`0x20|seq`, 7 bytes).
- **OBD2 modes:** `01` current data, `02` freeze frame, `03`/`07`/`0A` DTCs,
  `04` clear DTCs, `06` monitors, `08` systems control, `09` VIN/CALID/ECU name.
- **MCP2515 wiring:** MISO=GPIO9 (pin 21)→SO, MOSI=GPIO10 (19)→SI,
  SCLK=GPIO11 (23)→SCK, CE0=GPIO8 (24)→CS, GPIO25 (22)→INT. 5V modules:
  cuidado con niveles en SO→MISO (recomendado divisor o módulo level-shifted).
- **Cristal MCP2515:** 8 o 16 MHz define el bit timing (CNF1/2/3). Cristal
  equivocado = bus a mitad de velocidad.

---

## 1. elm327_rpi2w — Lector OBD2 + SSD1306

Lector OBD-II para Raspberry Pi (incl. Pi 2W) que conecta vía **Bluetooth
RFCOMM** a un escáner ELM327 y muestra datos en un **OLED SSD1306 128x64 por
SPI**.

### Estructura
```
src/: main.cpp (entry, keyboard, logging), elm327.cpp (driver BT + PIDs),
      gm_commands.cpp (modo 22 UDS GM), ssd1306.cpp (SPI driver),
      oled_display.cpp (7 páginas), logger.cpp (CSV), config.cpp, pid.cpp
include/: obd2_rpi/{config,types,pid,display_iface}.hpp, elm327.hpp,
          gm_commands.hpp, ssd1306.hpp, oled_display.hpp, logger.hpp
scripts/: build.sh, install_deps.sh, install_service.sh
```

### Arquitectura
- **3 threads:** main (teclado + log CSV + consola 300ms), OBD poll (ciclo
  800ms: PIDs, comandos GM, DTCs), display (render OLED 400ms + auto-rotación).
- **VehicleData** protegido con mutex; **PIDRegistry** centraliza los PIDs
  OBD2; **OBD2Config** carga de archivo o CLI.
- **7 páginas OLED** (rotación 6s): MOTOR, ADMISIÓN, FUEL TRIM, O2, GM
  (odómetro/batería/torque/presión combustible), DTC, DEBUG BT.
- SSD1306 SPI via libgpiod: DC=GPIO25, RES=GPIO17.

### Build / Run / Deploy
```bash
cmake -S . -B build -DBUILD_TESTS=OFF && make -C build -j$(nproc)
./bin/obd2_rpi 00:1D:A5:07:23:6E /dev/spidev0.0 25 17 [config/obd2_rpi.conf]
./scripts/build.sh remote            # SSH a joy@raspberry.local
./scripts/install_service.sh         # systemd: obd2_rpi.service
```

### Dependencias
`build-essential cmake libbluetooth-dev libgpiod-dev libgpiod2 gpiod bluez bluetooth`

### Teclas
`n/p` página, `a` auto-rotación, `l` log CSV, `g` datos GM, `d` DTCs,
`c` borrar DTCs, `h` ayuda, `q` salir.

### Cómo añadir un PID
1. Método en `ELM327` (elm327.hpp/.cpp). 2. Registrar en
`PIDRegistry::loadStandard()` (pid.cpp). 3. Campo en `VehicleData` (types.hpp).
4. Lectura en `obdPollThread()` (main.cpp). 5. Render en página de
`oled_display.cpp`.

---

## 2. mcp2515_emulator_obd2_opencode — Emulador ECU Prisma (nuevo)

Emula la ECU de un **Chevrolet Prisma** conectado a un **MCP2515 por SPI0**
(librería **bcm2835**). Responde a comandos **AT ELM327** y peticiones
**OBD2** de un escáner real o la consola integrada. Es el proyecto de
emulación más completo.

### Estructura
```
include/: mcp2515.h (driver), vehicle.h (modelo + simulador + consola),
          elm327.h (AT + OBD2 + ISO-TP)
src/: mcp2515.cpp, vehicle.cpp, elm327.cpp, main.cpp (hilos + menú)
test/: autotest.{h,cpp}, test_spi.cpp, test_loopback.cpp, test_bus.cpp
scripts/: run_tests.sh, can_kernel_test.sh, kill_apps.sh (make kill),
          install_dependencies.sh
docs/: SKILLS.md, ECU_PARAMETERS.md (catálogo de respuestas, fuente de verdad),
       SCANNER_TRACE_ONIX.md (traza del Onix real), BUG_REPORT.md
Makefile: make, make run, make kill, make test, make test-build,
          make test-socketcan, make install-bcm2835, make clean,
          make MCP2515_OSC_HZ=8000000
```

### Driver MCP2515 (mcp2515.cpp)
- Clase `MCP2515(uint8_t cs=CE0, uint8_t intPin=25)`. Multi-instancia OK.
- `begin()/end()` (init global bcm2835) y `beginExisting()/endLight()`
  (reutilizan init, para el autotest tras reset).
- `sendMessage(frame, timeout=50ms)` vía TXB0 (one-shot OSM); `receiveMessage()`
  no bloqueante RXB0/RXB1 (rollover BUKT); `isInterruptPending()` sondea GPIO25.
- Diagnóstico: `readStatus()`, `errorCountTx()/Rx()` (TEC/REC), `errorFlags()`
  (EFLG). Bit timing por tabla `kTimings[]` {oscHz,baud} → CNF1/2/3
  (16 MHz/500k = `0x00,0xF0,0x86`). SPI serializado con mutex interno.

### Modelo Vehicle + Simulador (vehicle.cpp)
- `Vehicle`: ~37 parámetros (velocidad, rpm, marcha, temperaturas, MAF, MAP,
  O2, fuel trims, EVAP, timing, baro, combustible, odómetro, run time +
  stft2, ltft2, torque, oil_life, inyector_pw, etanol, presion_tanque,
  misfire_actual/hist, knock_retard, balance_rate, temp_atf, afr).
  Métodos `value/setValue/isAuto/setAuto` **no bloquean** — llamar con
  `veh.mtx` tomado.
- `Simulator` a **10 Hz**; `Profile {Idle, City, Highway, Sport}`;
  fases `{Accel, Cruise, Decel, Stop}`; ruido LCG; `gearFor()`;
  parámetros en **FIJO** no se tocan (AUTO/FIJO por parámetro).

### ELM327 / OBD2 (elm327.cpp)
- **AT:** ATZ/ATRST→`ELM327 v1.5`, ATI/AT@1-3, ATE0/1, ATL, ATH, ATS, ATR,
  ATRV (batería), ATDP/ATDPN→`A6`, ATSPn (físico solo 6), ATSH/ATCRA
  (IDs TX/RX, 3 o 6 dígitos), ATFCSH/FCSM/FCSD (init CAN escáneres pro),
  **ATMM** (consulta máscara), **ATMM0** (real, default) / **ATMM1** (full);
  desconocido → `?`.
- **Encodings modo 01 (SAE J1979, corregidos 2026-08):** el emulador codifica
  `raw = valor físico × escala inversa` — 0x0C RPM = rpm×4, 0x0E avance =
  (°+64)×2, 0x42 batería = V×10. Antes invertido → la app mostraba ~50 rpm
  con 840 reales.
- **Máscaras real/full** (`ATMM0` = idénticas al Onix real según traza):
  real `0100→BE 3F B8 13`, `0140→FE D2 80 00`; full `0100→BF FF BF D2`,
  `0140→5E 94 67 90`. Los PIDs implementados responden aunque no se anuncien
  (el AUTEL sondea más allá de la máscara).
- **Modos:** 01/02 (40+ PIDs, incl. PID custom `0x4E` = marcha: 0=N, 1-5,
  6=R), 03/07/0A (DTCs, format `0x0301`=P0301), 04 (clear), 06 (monitores
  ISO 15031-5:2006+, TID 01/02/41/61/91), 08 (negativa), 09 (VIN
  `9BGKL48T0HB130763`, CALIDs con terminadores nulos, nombre ECU
  `TCM-Engine Control` como el real), **22 UDS** (DIDs GM: B100, 01A9,
  01B4, 1180, 01A1, 119F, 1193-119A, 11A1, 11A6, 1251/119D, 162F-1636,
  1940, 19DE, 119E, 1564, 1201, 2345) + servicios UDS `19 02` (historial
  DTCs), `14` (borrar), `31 01 C1 0F` (reset adaptativos).
- **Multi-PID:** 2-6 PIDs por trama. Respuesta física `0x7E9` a 0x7E0,
  funcional `0x7E8` a 0x7DF.
- **Referencia:** catálogo completo de respuestas (PIDs + DIDs + fórmulas,
  checklist) en `docs/ECU_PARAMETERS.md`; alineación con el auto real en
  `docs/SCANNER_TRACE_ONIX.md`.
- **ISO-TP:** ≤7B single frame; multi-frame con **espera de Flow Control**
  (300ms) aceptando FC de *cualquier* ID (fix BUG-01); tramas no-FC recibidas
  se bufferizan (`drainPending()`) para no perder peticiones legítimas.

### Threads y menú (main.cpp)
- **Hilo CAN:** sondea INT + `receiveMessage`, despacha por ID
  (0x7DF/0x7E0→`handleCanRequest`, 0x7E8/0x7E9→`handleCanResponse`).
  Pausable (`g_canPaused`) durante autotest.
- **Hilo simulación** (10 Hz). Menú: 1 inicia, 2 detiene, 3 perfil,
  4 configurar (FIJO/AUTO), 5 estado, 6 consola ELM327, 7 info sistema
  (CANSTAT/CANINTF/EFLG/TEC/REC), 8 autotest, 9 monitor en vivo (ANSI), 0 salir.

### Autotest (test/autotest.cpp)
- `test_spi` (cableado, registro R/W, BIT MODIFY, detección cristal 8 vs 16
  MHz vía CLKOUT→GPIO26, throughput SPI), `test_loopback` (TX/RX interno,
  INT), `test_bus` (2 módulos CE0/CE1, ráfaga 100 tramas, TEC/REC/EFLG).
- Runner: `sudo ./scripts/run_tests.sh [--spi|--loopback|--bus|--socketcan|--build]`.
- **Diagnóstico de fallo:** `CANSTAT=0xFF`=sin respuesta (alimentación/
  cableado); patrón `0x00`+RESET bajo=chip en reset, +RESET alto=alimentación/
  cristal. Sondas opcionales RESET→GPIO27 (`AUTOTEST_RESET_GPIO`),
  CLKOUT→GPIO26 (`AUTOTEST_CLKOUT_GPIO`).
- **SocketCAN alternativo:** `scripts/can_kernel_test.sh --setup [--bus]
  [--osc=8000000]` (overlays mcp251x, requiere reboot). El driver kernel y
  bcm2835 **compiten por el módulo**: usar uno a la vez.

### Workflow remoto (reglas del proyecto)
- Editar/commit local, `git pull` en la Pi (`USER=pi HOSTNAME=raspi.local
  LINK=/home/pi/src/mcp2515_emulator_rpi`), compilar/probar SOLO en la Pi.
- Versionado: tag `vX.Y.Z` debe coincidir con `VERSION` (sin `v`); ciclo
  patch 0-9 (no saltar de `v1.0.9` a `v1.1.1`; va a `v1.1.0`). Todo push con
  tag. Commits conventional (`feat:` `fix:` `docs:` `chore:` `refactor:`
  `test:`).

---

## 3. mcp2515_openai — Emulador (versión temprana)

Antecesor del emulador nuevo. Mismo concepto pero más simple, sin namespaces.

### Diferencias clave vs proyecto 2
- `MCP2515` con `Bitrate {125K/250K/500K}` y `Mode {NORMAL/LOOPBACK/CONFIG/
  LISTEN_ONLY}`; CNF1/2/3 hardcodeados para **16 MHz** (`0x00,0xD0,0x82`).
  Pins por numeración P1 (`RPI_GPIO_P1_24` CE0, `RPI_GPIO_P1_22` GPIO25).
- `Vehicle` tiene su **propio thread** de simulación + `SimulationMode
  {DYNAMIC, FIXED, RANDOM}` y `Profile {NORMAL, SPORT, ECONOMY, FAILSAFE}`;
  `setParameter()` por nombre; snapshot `Parameters` (speed, rpm, coolant,
  load, fuel, voltage, throttle, iat, map, maf, gear).
- `ELM327` **sin ISO-TP**: el hilo CAN solo acepta frames con `data[1]==0x01`
  (modo 01), responde desde `0x7E8` con DLC=8 rellenado; single-frame únicamente.
  PIDs modo 01 (04,05,0A,0B,0C,0D,0F,10,11,1F,2F,42,A4 gear) + máscaras 00/20.
  Respuestas con `\r\r>`.

---

## 4. prisma-emulator_clude — Emulador (versión intermedia)

Puente entre la v1 y la v2; primera con bucle real de polling CAN.

### Diferencias clave
- `MCP2515` con constantes estilo C (`#define`), `CanFrame` con flag `rtr`,
  `CanBitrate {125K/250K/500K/1000K}`; `setNormalMode/LoopbackMode/
  ListenOnlyMode`, `hasMessage()` (poll CANINTF), `interruptPending()`.
- `Vehicle`: mutex + `update(dt)`, `DrivingProfile {RALENTI, URBANO,
  CARRETERA, AGRESIVO, PERSONALIZADO}` y `GearState {PARK..FIFTH}`; snapshot
  `VehicleParameters` (añade timing advance, ambiente, odómetro, MIL,
  engineRunning); overrides FIJO (`setFixedValue/clearFixedValue`).
- `ELM327` con `pollCanRequests()`: si `id==0x7DF||0x7E0` y `data[1]!=0`,
  responde single-frame 0x7E8 (`data[0]=2+len`). Modos 01, 03 (0x00 sin
  fallas), 09/VIN. Sin modo 02/04/06/07/0A, sin ISO-TP multi-frame.
- `main.cpp`: threads SOLO al iniciar emulación (`startEmulation`):
  `simulationLoop()` 10Hz + `canLoop()` 20ms; `stopEmulation` los une.

---

## 5 y 6. mcp2515_scanner_rpi / raspberry_pi_scanner — Scanner AUTEL

Herramienta de diagnóstico tipo **AUTEL MaxiSYS** (lado lector): pide datos a
una ECU (p. ej. el emulador del proyecto 2) y los muestra en un **OLED
SSD1306 128x32 por I2C**. Misma capa hardware `bcm2835`.

### Estructura
```
include/hardware/: gpio.hpp, i2c.hpp, spi.hpp, mcp2515.hpp, ssd1306.hpp
include/scanner/: scanner.hpp, obd2.hpp, menu.hpp, display.hpp, dtc.hpp,
                  live_data.hpp, active_test.hpp
src/: main.cpp + hardware/*.cpp + scanner/*.cpp
tests/test_obd2.cpp · scripts/{build.sh,setup.sh} · CMakeLists.txt · Makefile
```

### Namespace `Hardware`
- `SPI`: Linux `/dev/spidev`, default 500 kHz.
- `MCP2515`: `initialize(Bitrate)`, `setFilter/setMask`, `sendMessage`,
  `receiveMessage`, `messageAvailable`, contadores de error; filtros
  aceptan todo; RXB0→RXB1 rollover.
- `SSD1306`: I2C (bus 1, addr 0x3C), drawPixel/Line/Rect/Circle/Char/Text,
  scroll, buffer. `I2C`: bus 1.
- `GPIO`: pin-mode/pull/read/write estáticos + `setISR`. **Usa bcm2835**
  (no wiringPi): `bcm2835_init()`, `BCM2835_GPIO_FSEL_*`,
  `BCM2835_GPIO_PUD_*`, `HIGH`/`LOW` (sin prefijo `BCM2835_GPIO_`).

### Namespace `Scanner`
- `AutelScanner` (facade: `initialize/run/shutdown`, dueño de subsistemas),
  `Display` (UI en 128x32, fuente 6x8), `Menu` (árbol `MenuItem
  {id,label,icon,action,children}`), `OBD2`, `DTCManager` (tabla
  `code|description|severity`, severidad por letra P=C=Major…),
  `LiveData` (18 canales, displayMode 0-3, listas custom),
  `ActiveTest` (EVAP, fuel pump, fan relay, AC clutch, throttle body,
  injectors).
- **OBD2** (`obd2.cpp`): `sendOBD2Request(mode,pid)` → frame 8B CAN
  `id=0x7DF`, `data[0]=0x02`, `data[1]=mode`, `data[2]=pid`; espera
  `0x7E8/0x7E9/0x7EA` (timeout 1000ms). `decodePID()` con switch grande y
  helpers: `(A*256+B)/4` RPM, `A` speed, `(A*256+B)/100` MAF, `A*100/255`
  throttle/load, `(A-128)*100/128` fuel trim, `A-40` temp. PIDs GM: 0x45
  TPS2, 0x49/4A APP1/2, 0x5C oil temp, 0x66 MAF voltage.
- **DTCs:** modo 03 → decodifica par de 2 bytes a letra+4 dígitos
  (bits `>>6`,`>>4`,`>>2`,`&0x03`); clear modo 04.
- VIN modo 09/02 (ASCII, single-frame). Stubs service:
  `resetAdaptations`, `resetFuelTrim`, `programFuelComposition`,
  `resetImmobilizer`. Monohilo (sin thread CAN; llamadas síncronas).

### Build / Run
```bash
./scripts/setup.sh          # instala deps + habilita SPI/I2C + log + grupos
./scripts/build.sh          # cmake Release + make -j$(nproc) + ctest
sudo ./build/autel_scanner  # requiere root (GPIO/SPI/I2C)
# Remoto:
ssh joy@raspberry.local "cd /home/joy/src/raspberry_pi_scanner && make clean && make -j4 && sudo make run"
```

### Lecciones aprendidas (SKILLS.md)
- Migración wiringPi→bcm2835: `pinMode`→`bcm2835_gpio_fsel`,
  `digitalWrite`→`bcm2835_gpio_write`, ISR→thread de polling.
- Designated initializers `.campo=` son C++20 → usar `memset(&tr,0,sizeof)`
  en C++17.
- `Edge` en `ISRData` debe calificarse `GPIO::Edge edge;`.
- Documentación AUTEL (OCR/Tesseract `--psm 3 -l spa`) en
  `docs/SKILL_AUTEL.md` y `docs/ANALISIS_MENU_SCANNER_AUTEL.md`.

---

## 7. mcp2515_rpi — ECU Multi-Emulator (8 marcas)

El más grande: emula **8 marcas** (Chevrolet/GM, Ford, Toyota, BMW, VW/Audi,
Mercedes, Honda, Nissan +Hyundai/Kia) sobre **SocketCAN** (no SPI directo),
con REST/WebSocket/gRPC, SQLite, simulación, seguridad seed/key y diagnóstico.
Namespace `ecumult`.

### Estructura
```
src/core/        can_manager, protocol_router, session_manager, security
src/manufacturers/ base.hpp/cpp + 8 marcas (chevrolet, ford, toyota, bmw,
                   volkswagen, mercedes, honda, nissan)
src/protocols/   obd2_standard, uds, gmlan, kwp2000, can_tp
src/database/    db_manager (SQLite3), migrations, seed_data
src/diagnostics/ dtc_manager, freeze_frame, readiness, vin_decoder, calibration
src/simulation/  driving_cycle, sensor_simulator, fault_injector, environment,
                 vehicle_profile
src/api/         rest_api, websocket, grpc_server
src/security/    access_control, secure_comm, seed_key
src/logging/     can_logger, metrics_exporter, replay_analyzer
src/tests/       test_all_modes, test_manufacturers, fuzzing_suite (Catch2)
scripts/         install_deps, setup_can, build (-n -d -t), deploy, run_tests,
                 fuzz, diag
docker/          Dockerfile + entrypoint.sh · examples/test_commands.sh
Makefile · config.json · install_multi_emulator.sh
```

### Core
- `CANManager` (SocketCAN `SOCK_RAW`): `rxLoop()` (poll→recvfrom, timestamp +
  `bus_id`, dispatch a callbacks por `(can_id & mask)`) + `txLoop()` (cola
  `tx_queue_` con `tx_mutex_`). Main registra callback catch-all `(0x000,0x000)`.
- `ProtocolRouter`: mapa `Manufacturer → BaseManufacturer`;
  `selectManufacturer()` llama `onManufacturerSelected(cfg)`;
  `routeCANFrame()` al activo. `ManufacturerConfig {rx_id=0x7E8, tx_id=0x7E0,
  functional_id=0x7DF, bitrate=500000, vin_prefix}`.
- `SessionManager`: sesiones UDS (DEFAULT/PROGRAMMING/EXTENDED/SAFETY),
  `SecurityLevel {LOCKED, LEVEL_1..3}`, seed/key por nivel (`std::function`),
  tester-present, timeouts 5s/10s.

### BaseManufacturer + marcas
- `BaseManufacturer`: implementa OBD2 01-0A completo, servicios UDS
  (0x10,11,27,28,3E,22,23,2E,3D,31,34-37...), DTCs (`getDTCs/clearDTCs/
  setDTC/getFreezeFrames/getReadinessStatus`), VIN/calibraciones, sensores
  simulados, **odómetro** (`odometer_km_`/`trip_km_`, incremento por
  velocidad), tabla de PIDs (`setupDefaultPIDs()` ~50 + `addPID()`),
  `sendResponse/sendPositiveResponse/sendNegativeResponse` (0x7F+NRC).
- Subclases: `ChevroletGM` (GMLAN modo 22/2E, DIDs GM), `Ford` (PATS,
  MS-CAN), `Toyota` (modo 21/22, smart key, checksum XOR), `BMW` (DCAN/
  EDIABAS, Vanos/wastegate), `Volkswagen` (adaptación/long coding), `Mercedes`
  (DAS/SBC), `Honda` (HDS), `Nissan` (Consult III, turbo/CVT).

### Protocolos
- `CANTransportProtocol`: máquina de estados ISO 15765-2 completa
  (SF/FF/CF/FC, seq 0x0F wrap, `prepareMessage` segmenta TX).
- `OBD2Standard`: tabla PIDs estática, encode/decode, DTC, checksum.
- `UDSProtocol`: ISO 14229 (SIDs 0x10-0x37, NRC 0x10-0x78, sesiones/reset).
- `KWP2000Protocol`: framing START/STOP comm, checksum.
- `GMLANProtocol`: servicios 0x1A/0x3B/0xA2, DIDs 0x1A00 VIN, 0xF180-F185.

### API
- **REST** (puerto 8080, HTTP/1.1 raw socket + CORS): `GET /`|`/health`,
  `GET /profile`, `GET /profiles`, `POST /profile`, `GET/POST /odometer`,
  `GET /dtcs [?all]`, `POST /dtcs/clear`. 404 para desconocido.
- **WebSocket** (8081, 100 clientes, generador JSON: rpm, speed, coolant,
  MAF, throttle, fuel, battery, pressures, profile, odometer).
- **gRPC** (opcional). **Prometheus** `/metrics`.

### main.cpp
Loop principal a **50 ms**: driving cycle → RPM profile → sensor sim → env
sim → fault injection → sync sensores al fabricante activo → odómetro
(`speed/3600*dt`, flush 0.01 km) → gauges de métricas. Threads: CAN rx/tx,
REST, WS, gRPC.

### Build / Run / Test
```bash
make                       # arm-linux-gnueabihf-g++ -std=c++17 (cross)
make CROSS_COMPILE=        # g++ nativo
sudo ./install_multi_emulator.sh
sudo modprobe vcan && sudo ip link add vcan0 type vcan && sudo ip link set vcan0 up
./ecu_emulator [config.json]   # interface can0|vcan0 en config.json
curl http://localhost:8080/vehicles
cansend can0 7DF#02010C0000000000      # RPM
curl -X POST localhost:8080/select -d '{"manufacturer":"toyota","model":"Camry"}'
cd build && ctest                        # Catch2
```

### Perfiles de vehículo (`vehicle_profile.hpp`)
10 perfiles: `normal`, `unstable_idle`, `bad_o2`, `maf_fault`, `coolant_fault`,
`misfire`, `low_battery`, `fuel_pressure`, `emission_fail`, `custom`.
`VehicleProfileManager` (accesible vía `BaseManufacturer::getProfileManager()`):
`selectProfile("bad_o2")`, `applyRPMProfile(base_rpm, dt)`,
`applySensorOverride(0x14, raw, dt)` (stuck/noise/drift/clamp). Cambiar en
runtime por API `POST /profile`.

---

## Trabajando con estos proyectos (reglas generales)

1. **Hardware MCP2515:** verificar cristal 8/16 MHz antes de tocar bit timing;
   nivel de voltaje MISO (5V→3.3V) en módulos de 5V.
2. **SPI no es thread-safe:** serializar con mutex (los drivers lo hacen).
3. **IDs CAN:** respetar 0x7DF/0x7E0/0x7E8/0x7E9; escáneres pro hacen
   `ATSP6 ATFCSH...` antes de `0100 0902`.
4. **ISO-TP multi-frame:** sin Flow Control el escáner no recibe VIN/CALID
   completos; aceptar FC desde cualquier ID y bufferizar tramas intermedias.
5. **root/sudo:** bcm2835 (emuladores) y acceso GPIO/SPI/I2C (scanners)
   requieren root. SocketCAN requiere `ip link set ... up`.
6. **Remoto:** los proyectos compilan/protejen en la Raspberry Pi
   (`ssh joy@raspberry.local` o `pi@raspi.local`), no en el dev host.
7. **Versionado:** tag `vX.Y.Z` == `VERSION`; ciclo patch 0-9; todo push con
   tag; conventional commits.

## Documentación por proyecto (fuentes de referencia)

| Proyecto | Docs clave |
|---|---|
| `elm327_rpi2w/` | `docs/architecture.md`, `docs/configuration.md`, `docs/pid_reference.md`, `skills/obd2_rpi.md` |
| `mcp2515_emulator_obd2_opencode/` | `docs/SKILLS.md` (compendio), `docs/LEARNINGS.md`, `docs/WORKFLOW.md`, `docs/BUG_REPORT.md`, `docs/README.md` (índice) |
| `mcp2515_openai/` | `doc/LEARNINGS.md`, `doc/WORKFLOW.md` |
| `prisma-emulator_clude/` | `docs/LEARNINGS.md`, `docs/WORKFLOW.md` |
| `mcp2515_scanner_rpi/` | `docs/SKILL_AUTEL.md` (OCR/menú), `docs/HARDWARE.md`, `docs/WORKFLOW.md`, `docs/TODO.md` |
| `raspberry_pi_scanner/` | `docs/SKILLS.md` (bcm2835), `docs/SKILL_AUTEL.md`, `docs/ANALISIS_MENU_SCANNER_AUTEL.md`, `docs/LEARNINGS.md` |
| `mcp2515_rpi/` | `docs/api.yaml` (OpenAPI 3.0), `examples/python_client.py`, `examples/test_commands.sh` |
