# AUTEL Scanner - Raspberry Pi Edition

Scanner automotriz basado en Raspberry Pi con interfaz CAN bus (MCP2515) y display OLED SSD1306.

## Hardware

- **MCU:** Raspberry Pi (3B+, 4, 5)
- **CAN Bus:** MCP2515 (SPI)
- **Display:** SSD1306 OLED 128x32 (I2C)
- **Conexión OBD-II:** MCP2515 + transceptor CAN (SN65HVD230)

## Estructura del Proyecto

```
raspberry_pi_scanner/
├── CMakeLists.txt          # Build system
├── README.md               # Este archivo
├── VERSION                 # Versión del proyecto
├── include/
│   ├── scanner/            # Lógica del scanner
│   │   ├── scanner.hpp
│   │   ├── can_bus.hpp
│   │   ├── display.hpp
│   │   ├── menu.hpp
│   │   ├── obd2.hpp
│   │   ├── dtc.hpp
│   │   ├── live_data.hpp
│   │   └── active_test.hpp
│   └── hardware/           # Drivers de hardware
│       ├── mcp2515.hpp
│       ├── spi.hpp
│       ├── ssd1306.hpp
│       └── gpio.hpp
├── src/                    # Implementaciones
│   ├── main.cpp
│   ├── scanner/
│   └── hardware/
├── config/                 # Archivos de configuración
│   ├── default.cfg
│   └── logging.cfg
├── docs/                   # Documentación
│   ├── SKILL_AUTEL.md
│   └── HARDWARE.md
├── tests/                  # Pruebas unitarias
├── scripts/                # Scripts de utilidad
│   ├── setup.sh
│   └── build.sh
└── assets/                 # Recursos (fuentes, imágenes)
```

## Compilación

```bash
# Con CMake
mkdir build && cd build
cmake ..
make

# Con script
./scripts/build.sh
```

## Instalación de Dependencias

```bash
# Raspberry Pi OS
sudo apt update
sudo apt install -y build-essential cmake libbcm2835-dev i2c-tools

# Habilitar interfaces
sudo raspi-config
# Interface Options → SPI → Enable
# Interface Options → I2C → Enable
```

## Ejecución

```bash
sudo ./bin/scanner_autel_32   # sufijo _64 si uname -m = aarch64
```

> **Cristal del módulo:** la tabla de bit timing (`MCP2515::setBitrate` en
> `src/hardware/mcp2515.cpp`) está calculada para el cristal de **8 MHz** de
> los módulos CAN-BUS (misma tabla que `emulator/prisma`). OBD2 ISO 15765-4
> usa **500 kbps** (`BPS_500K`, `ATSP6`). Si su módulo tiene cristal de
> 16 MHz, ajuste los valores CNF1/CNF2/CNF3 a los de 16 MHz.

## Arquitectura

```
┌─────────────────────────────────────────────┐
│           Application Layer                  │
│  Scanner → Menu → OBD2 → DTC → LiveData    │
├─────────────────────────────────────────────┤
│           Hardware Abstraction Layer         │
│         MCP2515 ← SPI ← Raspberry Pi        │
│         SSD1306 ← I2C ← Raspberry Pi        │
└─────────────────────────────────────────────┘
```

## Licencia

MIT
