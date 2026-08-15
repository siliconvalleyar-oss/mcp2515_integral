---
name: obd2-rpi-reader
description: |
  Use when working on the OBD2 reader project (./scanner/reader, formerly elm327_rpi2w) — a C++17 OBD2 reader for Raspberry Pi 2W that connects over Bluetooth RFCOMM to an ELM327 scanner and renders live data on a SSD1306 128x64 OLED display over SPI (libgpiod). Covers build with cmake, 3-thread architecture (main/OBD poll/display), 7 OLED pages, PID registry, GM mode 22 commands, systemd service. Use ONLY when the task targets this specific OBD2 reader.
compatibility:
  - C++17
  - Raspberry Pi 2W
  - Bluetooth RFCOMM / ELM327
  - SSD1306 SPI (DC=GPIO25, RES=GPIO17)
---

# OBD2 RPi Reader Skill (elm327_rpi2w)

## Quick Reference
```bash
cmake -S . -B build -DBUILD_TESTS=OFF && make -C build -j$(nproc)
./bin/obd2_rpi 00:1D:A5:07:23:6E /dev/spidev0.0 25 17 [config/obd2_rpi.conf]
./scripts/build.sh remote            # SSH a joy@raspberry.local
./scripts/install_service.sh         # instala obd2_rpi.service (systemd)
```

## Structure
```
src/: main.cpp, elm327.cpp, gm_commands.cpp, ssd1306.cpp, oled_display.cpp,
      logger.cpp, config.cpp, pid.cpp
include/: obd2_rpi/{config,types,pid,display_iface}.hpp, elm327.hpp,
          gm_commands.hpp, ssd1306.hpp, oled_display.hpp, logger.hpp
scripts/: build.sh (local|remote|clean), install_deps.sh, install_service.sh
```

## Architecture
- 3 threads: main (keyboard+CSV log, 300ms), OBD poll (800ms: PIDs, GM
  commands, DTCs), display (OLED render, 400ms, auto-rotate every 6s).
- `VehicleData` guarded by mutex; `PIDRegistry::loadStandard()` centralizes
  OBD2 PIDs; `OBD2Config` loads from file or CLI.
- 7 OLED pages: MAIN, ENGINE (ADMISION), FUEL TRIM, O2, GM (odometer,
  battery, torque, fuel pressure), DTC, DEBUG BT.
- SSD1306 SPI via libgpiod: DC=GPIO25, RES=GPIO17.

## Keyboard Controls
`n`/`p` page, `a` auto-rotate, `l` CSV logging, `g` force GM data,
`d` read DTCs, `c` clear DTCs (confirm), `h` help, `q` quit.

## Adding a PID
1. Method in `ELM327` (elm327.hpp/.cpp). 2. Register in
`PIDRegistry::loadStandard()` (pid.cpp). 3. Field in `VehicleData`
(types.hpp). 4. Read in `obdPollThread()` (main.cpp). 5. Render in
`oled_display.cpp` page function.

## Dependencies
`sudo apt install -y build-essential cmake libbluetooth-dev libgpiod-dev libgpiod2 gpiod bluez bluetooth`

## Debugging
- Bluetooth: `bluetoothctl show`, `bluetoothctl pair <MAC>`
- SPI: `ls -l /dev/spidev0.0`; GPIO: `gpioinfo`
- Use DEBUG BT page (7) to inspect TX/RX lines.

See `skills/obd2_rpi.md` and `docs/*.md` for the full reference.
