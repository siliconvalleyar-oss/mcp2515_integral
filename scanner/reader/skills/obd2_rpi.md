---
name: obd2_rpi
description: OBD2 Raspberry Pi reader with SSD1306 OLED display
---

# OBD2 RPi Development Skill

This skill helps you work with the OBD2 RPi project - a C++17 application for
Raspberry Pi that reads OBD-II data via Bluetooth ELM327 and displays it on
a SSD1306 OLED display over SPI.

## Project Structure

```
obd2_rpi/
├── bin/                    # Compiled binary output
├── build/                  # CMake build directory
├── config/                 # Configuration files
│   └── obd2_rpi.conf       # Default config
├── docs/                   # Documentation
│   ├── architecture.md     # Architecture overview
│   ├── configuration.md    # Configuration reference
│   └── pid_reference.md    # OBD-II PID reference
├── include/
│   ├── obd2_rpi/
│   │   ├── config.hpp      # Configuration system
│   │   ├── display_iface.hpp # Abstract display interface
│   │   ├── pid.hpp         # PID registry
│   │   └── types.hpp       # Shared types
│   ├── elm327.hpp          # ELM327 Bluetooth driver
│   ├── gm_commands.hpp     # GM mode 22 commands
│   ├── logger.hpp          # CSV logger
│   ├── oled_display.hpp    # OLED page renderer
│   └── ssd1306.hpp         # SSD1306 SPI driver
├── scripts/
│   ├── build.sh            # Build script (local/remote)
│   ├── install_deps.sh     # Install dependencies
│   └── install_service.sh  # Install systemd service
├── src/
│   ├── main.cpp            # Entry point, keyboard, logging
│   ├── config.cpp          # Config file parser
│   ├── elm327.cpp          # ELM327 communication
│   ├── gm_commands.cpp     # GM UDS commands
│   ├── logger.cpp          # CSV logging
│   ├── oled_display.cpp    # OLED rendering (7 pages)
│   ├── pid.cpp             # PID registry
│   └── ssd1306.cpp         # SSD1306 SPI driver
├── skills/
│   └── obd2_rpi.md         # This skill file
├── CMakeLists.txt          # Build system
├── obd2_rpi.service        # systemd service unit
└── README.md               # Main documentation
```

## Architecture

### Threads
- **Main thread**: Keyboard input (stdin), CSV logging, console status (300ms)
- **OBD Poll thread**: PID reading, GM commands, DTCs (800ms cycle)
- **Display thread**: OLED rendering, auto-page rotation (400ms)

### Data Flow
```
ELM327 BT -> PID commands -> Parse response -> VehicleData (mutex) -> OLED render
```

### Pages (7 total, auto-rotate every 6s)
1. MAIN - RPM (big+bar), speed, coolant temp, load
2. ENGINE - MAP, throttle, timing, intake temp, MAF, baro
3. FUEL - STFT/LTFT B1/B2, fuel level
4. O2 - O2 voltage B1S1/B2S1, mixture status
5. GM - Odometer, battery voltage, torque, fuel pressure
6. DTC - Active trouble codes, MIL status
7. DEBUG - Last TX/RX line, protocol, connection status

## Development Commands

### Build
```bash
# Local build (x86 test)
cmake -S . -B build && make -C build

# Remote build on RPi
ssh joy@raspberry.local "cd /home/joy/src/elm_327_rpi_oled_v16/obd2_rpi && mkdir -p build && cmake -S . -B build && make -C build -j\$(nproc)"

# Or use script
./scripts/build.sh remote
```

### Run
```bash
# On Raspberry Pi
./bin/obd2_rpi 00:1D:A5:07:23:6E /dev/spidev0.0 25 17

# With config file
./bin/obd2_rpi 00:1D:A5:07:23:6E /dev/spidev0.0 25 17 config/obd2_rpi.conf
```

### Controls (stdin)
| Key | Action |
|-----|--------|
| n/p | Next/previous page |
| a   | Toggle auto-rotate |
| l   | Toggle CSV logging |
| g   | Force GM data read |
| d   | Read DTCs |
| c   | Clear DTCs |
| h   | Help |
| q   | Quit |

## Dependencies

```bash
sudo apt install -y \
    build-essential cmake \
    libbluetooth-dev \
    libgpiod-dev libgpiod2 gpiod \
    bluez bluetooth
```

## Wiring (SSD1306 SPI)

| Display | RPi Pin | GPIO |
|---------|---------|------|
| VCC | 1 (3.3V) | - |
| GND | 6 (GND) | - |
| DIN | 19 | GPIO10 (MOSI) |
| CLK | 23 | GPIO11 (SCLK) |
| CS | 24 | GPIO8 (CE0) |
| DC | 22 | GPIO25 |
| RES | 11 | GPIO17 |

## Typical Workflows

### Adding a new OBD-II PID
1. Add the formula to `src/elm327.cpp` as a new method
2. Add the declaration to `include/elm327.hpp`
3. Register it in `PIDRegistry::loadStandard()` in `src/pid.cpp`
4. Add display rendering in the appropriate page in `src/oled_display.cpp`
5. Add the field to `VehicleData` in `include/obd2_rpi/types.hpp`
6. Add the read call in the polling cycle in `src/main.cpp`

### Debug connection issues
1. Check Bluetooth: `bluetoothctl show`
2. Pair ELM327: `bluetoothctl pair 00:1D:A5:07:23:6E`
3. Check SPI: `ls -l /dev/spidev0.0`
4. Check GPIO: `gpioinfo`
5. Run with debug page (page 7) to see TX/RX

### Cross-compilation notes
- The CMakeLists.txt uses `CMAKE_SYSTEM_PROCESSOR` for arch detection
- Native ARM build produces optimized binaries
- On x86_64, it compiles with generic flags for testing

## Version

Current: 2.0.0
