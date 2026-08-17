---
name: autel-scanner-rpi
description: |
  Use when working on the AUTEL scanner project (./scanner/autel_scanner, formerly raspberry_pi_scanner; the duplicate mcp2515_scanner_rpi was archived to legacy/scanner-duplicate on the main branch only). A C++17 automotive diagnostic scanner for Raspberry Pi that talks OBD2 over CAN (MCP2515 SPI, bcm2835) to an ECU and renders menus/live data on a SSD1306 128x32 OLED over I2C. Covers the Scanner namespace (AutelScanner, Menu, OBD2, DTC, LiveData, ActiveTest, Display), Hardware drivers (MCP2515, SPI, SSD1306, I2C, GPIO), PID decoding, DTC decoding, and the wiringPi→bcm2835 migration. Use ONLY when the task targets one of these two scanner directories.
compatibility:
  - C++17
  - bcm2835 (no wiringPi)
  - MCP2515 SPI (CE0=GPIO8, INT=GPIO25)
  - SSD1306 OLED I2C (bus 1, addr 0x3C)
  - Raspberry Pi 3B+/4/5
---

# AUTEL Scanner Skill (mcp2515_scanner_rpi / raspberry_pi_scanner)

> The two directories are byte-identical code. `raspberry_pi_scanner` is the
> upstream (git repo, tags v1.0.0-v1.0.3, docs). Fixes apply to both.

## Quick Reference
```bash
./scripts/setup.sh          # sudo: deps + SPI/I2C + grupos + /var/log/autel_scanner.log
./scripts/build.sh          # cmake Release + make -j$(nproc) + ctest
sudo ./build/autel_scanner  # requiere root
# Remoto:
ssh joy@raspberry.local "cd /home/joy/src/raspberry_pi_scanner && make clean && make -j4 && sudo make run"
```

## Structure
```
include/hardware/: gpio.hpp, i2c.hpp, spi.hpp, mcp2515.hpp, ssd1306.hpp
include/scanner/: scanner.hpp, obd2.hpp, menu.hpp, display.hpp, dtc.hpp,
                  live_data.hpp, active_test.hpp
src/: main.cpp + hardware/*.cpp + scanner/*.cpp
tests/test_obd2.cpp · scripts/{build.sh,setup.sh} · CMakeLists.txt · Makefile
```

## Hardware namespace (bcm2835, NOT wiringPi)
- `SPI`: Linux /dev/spidev, default 500 kHz.
- `MCP2515`: `initialize(Bitrate)`, `setFilter/setMask`, `sendMessage`,
  `receiveMessage`, `messageAvailable`, error counters; accept-all filters;
  RXB0→RXB1 rollover.
- `SSD1306`: I2C (bus 1, 0x3C), drawPixel/Line/Rect/Circle/Char/Text, scroll.
- `GPIO`: static fsel/pull/read/write + `setISR`. Use `bcm2835_init()`,
  `BCM2835_GPIO_FSEL_*`, `BCM2835_GPIO_PUD_*`, constants `HIGH`/`LOW`
  (no `BCM2835_GPIO_HIGH`).

## Scanner namespace
- `AutelScanner`: facade `initialize/run/shutdown`, owns subsystems.
- `Menu`: tree of `MenuItem {id,label,icon,action,children}`.
- `OBD2`: `sendOBD2Request(mode,pid)` → 8B CAN frame `id=0x7DF, data[0]=0x02,
  data[1]=mode, data[2]=pid`; waits `0x7E8/0x7E9/0x7EA` (1000ms). `decodePID()`
  switch: RPM `(A*256+B)/4`, speed `A`, MAF `(A*256+B)/100`, throttle/load
  `A*100/255`, fuel trim `(A-128)*100/128`, temp `A-40`. GM PIDs: 0x45 TPS2,
  0x49/4A APP1/2, 0x5C oil temp, 0x66 MAF voltage.
- `DTCManager`: DTC mode 03 decode (2-byte → letter+4 digits via bits
  `>>6>>4>>2`), severity by letter; clear mode 04.
- `LiveData`: 18 channels, displayMode 0-3, custom PID lists.
- `ActiveTest`: EVAP, fuel pump relay, fan relay, AC clutch, throttle body,
  injectors (placeholders pending real mapping).
- VIN mode 09/02 single-frame ASCII (no ISO-TP multi-frame support).
- Service stubs: `resetAdaptations`, `resetFuelTrim`,
  `programFuelComposition`, `resetImmobilizer`. Single-threaded (synchronous).

## Wiring (docs/HARDWARE.md)
- MCP2515: VCC=3.3V, CS=GPIO8(CE0), SCK=GPIO11, SI=GPIO10(MOSI),
  SO=GPIO9(MISO), INT=GPIO25. CANH→OBD-II pin 6, CANL→pin 14
  (via SN65HVD230 transceiver).
- SSD1306: SDA=GPIO2, SCL=GPIO3, 3.3V.

## Key Learnings (migration wiringPi→bcm2835)
- `pinMode`→`bcm2835_gpio_fsel`, `digitalWrite`→`bcm2835_gpio_write`,
  `wiringPiISR`→polling thread. bcm2835 uses BCM GPIO numbering (pin 8 =
  GPIO8 CE0, correct CS mapping).
- Contra el emulador Prisma: si un valor sale ~×4 menor (840 rpm → ~50 rpm),
  es el **encoding del emulador** el que está invertido — el decode J1979
  del scanner es correcto; el emulador debe enviar `raw = valor × escala
  inversa` (0x0C RPM = rpm×4, 0x0E avance = (°+64)×2, 0x42 batería = V×10).
- C++17: designated initializers are C++20 → use `memset(&tr,0,sizeof(tr))`.
- In `ISRData`, qualify `GPIO::Edge edge;`.
- PID/TODO tracking lives in `docs/TODO.md` (live checklist).

See `docs/SKILL_AUTEL.md` (OCR/menú AUTEL), `docs/ANALISIS_MENU_SCANNER_AUTEL.md`,
`docs/SKILLS.md`, `docs/LEARNINGS.md`.
