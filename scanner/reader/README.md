# OBD2 RPi + SSD1306 SPI v2.0

> Lector OBD-II para Raspberry Pi con pantalla OLED SSD1306 de 128x64px.
> Conecta via Bluetooth RFCOMM a un escáner ELM327 y muestra datos en tiempo real.

![Arch](https://img.shields.io/badge/arch-armv6%20%7C%20armv7%20%7C%20aarch64-red)
![Lang](https://img.shields.io/badge/lang-C%2B%2B17-blue)
![BT](https://img.shields.io/badge/ble-Bluetooth%20RFCOMM-green)

## Hardware

### Conexiones OLED SSD1306 SPI

| SSD1306 | RPi Pin | GPIO BCM | Funcion |
|---------|---------|----------|---------|
| VCC | Pin 1 | 3.3V | Alimentacion |
| GND | Pin 6 | GND | Tierra |
| DIN | Pin 19 | GPIO10 | SPI0 MOSI |
| CLK | Pin 23 | GPIO11 | SPI0 SCLK |
| CS | Pin 24 | GPIO8 | SPI0 CE0 |
| DC | Pin 22 | GPIO25 | Data/Command |
| RES | Pin 11 | GPIO17 | Reset |

### ELM327 Bluetooth
- Escáner OBD-II compatible con ELM327 v1.5+
- Conexion RFCOMM canal 1
- MAC configurable via CLI o archivo de configuracion

## Compilacion

```bash
# Via SSH a la Raspberry Pi
ssh joy@raspberry.local
git clone <repo> ~/src/obd2_rpi
cd ~/src/obd2_rpi

# Compilar
mkdir -p build && cd build
cmake .. && make -j$(nproc)

# O usando el script
./scripts/build.sh
./scripts/build.sh remote          # Compila via SSH
./scripts/build.sh remote pi@pi    # Host custom
```

## Uso

```bash
./bin/obd2_rpi [MAC] [SPI_DEV] [PIN_DC] [PIN_RST] [CONFIG]

# Ejemplos
./bin/obd2_rpi 00:1D:A5:07:23:6E
./bin/obd2_rpi 00:1D:A5:07:23:6E /dev/spidev0.0 25 17
./bin/obd2_rpi 00:1D:A5:07:23:6E /dev/spidev0.0 25 17 config/obd2_rpi.conf
```

### Controles por Teclado

| Tecla | Accion |
|-------|--------|
| `n`/`p` | Siguiente/anterior pagina OLED |
| `a` | Toggle auto-rotacion |
| `l` | Iniciar/detener log CSV |
| `g` | Forzar lectura de datos GM |
| `d` | Leer DTCs |
| `c` | Borrar DTCs (con confirmacion) |
| `h` | Ayuda |
| `q` | Salir |

### Instalacion como Servicio

```bash
# 1. Editar MAC en obd2_rpi.service si es necesario
# 2. Instalar
sudo cp obd2_rpi.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable obd2_rpi
sudo systemctl start obd2_rpi
sudo systemctl status obd2_rpi
sudo journalctl -u obd2_rpi -f
```

## Paginas OLED

La pantalla muestra 7 paginas con rotacion automatica cada 6 segundos:

| # | Pagina | Contenido |
|---|--------|-----------|
| 1 | **MOTOR** | RPM (numero grande + barra), velocidad, temperatura refrigerante, carga motor, MAF |
| 2 | **ADMISION** | MAP, posicion acelerador, avance encendido, temperatura admision, MAF, presion barometrica |
| 3 | **FUEL TRIM** | STFT B1/B2, LTFT B1/B2, nivel combustible con barra |
| 4 | **SENSOR O2** | Voltaje O2 B1S1/B2S1 con barra, estado mezcla (POBRE/RICO/OK) |
| 5 | **DATOS GM** | Odometro, voltaje bateria, torque motor, presion combustible, temp catalizador |
| 6 | **DTC** | Lista codigos de error activos, estado MIL |
| 7 | **DEBUG BT** | Ultima linea TX/RX Bluetooth, protocolo, estado conexion |

## Arquitectura

```
src/
├── main.cpp          # Punto de entrada, teclado, logging
├── elm327.cpp        # Driver BT RFCOMM + PIDs OBD-II (modo 01)
├── gm_commands.cpp   # Comandos GM modo 22 (UDS)
├── ssd1306.cpp       # Driver SPI para OLED SSD1306
├── oled_display.cpp  # 7 paginas de informacion OBD2
├── logger.cpp        # Logging CSV
├── config.cpp        # Sistema de configuracion
└── pid.cpp           # Registro centralizado de PIDs

include/
├── obd2_rpi/
│   ├── config.hpp    # Configuracion
│   ├── types.hpp     # Tipos compartidos (VehicleData, etc)
│   ├── pid.hpp       # Registro de PIDs
│   └── display_iface.hpp  # Interfaz de display abstracta
├── elm327.hpp
├── gm_commands.hpp
├── ssd1306.hpp
├── oled_display.hpp
└── logger.hpp
```

## Configuracion

Ver `docs/configuration.md` para detalle de todas las opciones.

Archivo de configuracion por defecto: `config/obd2_rpi.conf`

## Dependencias

```bash
sudo apt install -y \
    build-essential cmake \
    libbluetooth-dev \
    libgpiod-dev libgpiod2 gpiod \
    bluez bluetooth
```

## Documentacion Adicional

- `docs/architecture.md` - Arquitectura y componentes
- `docs/pid_reference.md` - Referencia de PIDs OBD-II
- `docs/configuration.md` - Opciones de configuracion

## Licencia

MIT
