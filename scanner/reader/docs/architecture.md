# Arquitectura de obd2_rpi

## Visión General

Aplicación C++17 para Raspberry Pi que lee datos OBD-II via Bluetooth RFCOMM
desde un escáner ELM327 y los muestra en una pantalla OLED SSD1306 (128x64) por SPI.

## Diagrama de Componentes

```
+------------------------------------------------------+
|                     main.cpp                          |
|  +----------+    +--------+    +------------------+   |
|  | Keyboard |    | Logger |    | Config (config)  |   |
|  | (stdin)  |    | (CSV)  |    | obd2_rpi.conf    |   |
|  +----------+    +--------+    +------------------+   |
+--------+---------------------------------------------+
         |
    +----+----+      +-------------------+
    | ELM327  |<---->| Bluetooth RFCOMM  |
    | (OBD-II)|      | (libbluetooth)    |
    +----+----+      +-------------------+
         |
    +----+----+
    |GMCommands|
    |(GM UDS)  |
    +---------+
                        +-------------------+
         +------------->| OBD Poll Thread   |
         |              | (800ms cycle)     |
         |              +--------+----------+
         |                       |
    +----+-----+          +-----+------+
    | Vehicle   |<---------| Datos      |
    | Data      |  mutex   | Compartidos|
    +-----------+          +------------+
         |
    +----+-----+
    |OLEDDisplay|------>| SSD1306 SPI Driver |
    |(7 pages)  |       | (libgpiod + spidev)|
    +-----------+       +--------------------+
```

## Hilos

| Hilo | Propósito | Ciclo |
|------|-----------|-------|
| **Main** | Entrada teclado, logging, status consola | 300ms |
| **OBD Poll** | Lectura PIDs OBD-II + GM + DTCs | 800ms |
| **Display** | Renderizado OLED + auto-rotación | 400ms |

## Pipeline de Datos OBD

```
PID Entrada -> Serializar comando AT -> Enviar BT -> Recibir -> Parsear
    -> Aplicar fórmula -> Actualizar VehicleData (mutex) -> Renderizar OLED
```

## Paginas OLED (7)

| Pag | Nombre | Datos |
|-----|--------|-------|
| 1 | MOTOR | RPM + barra, velocidad, temperatura, carga, MAF |
| 2 | ADMISION | MAP, throttle, timing, intake temp, baro |
| 3 | FUEL TRIM | STFT/LTFT B1/B2, nivel combustible |
| 4 | SENSOR O2 | Voltaje O2 B1S1/B2S1, barra, estado mezcla |
| 5 | DATOS GM | Odometro, bateria, torque, presion combustible, cat temp |
| 6 | CODIGOS DTC | Lista DTCs activos + MIL |
| 7 | DEBUG BT | TX/RX en tiempo real, protocolo |

## Flujo de Conexion

1. Abrir socket RFCOMM Bluetooth
2. Conectar a MAC del ELM327
3. Enviar secuencia de inicializacion AT:
   - ATZ (reset), ATE0 (echo off), ATL0 (linefeeds off)
   - ATS0 (spaces off), ATSP0 (auto protocol)
   - ATAT1 (adaptive timing), ATST20 (timeout)
4. Verificar protocolo detectado
5. Iniciar polling OBD

## Manejo de Errores

- Timeout BT: select() con timeout 600ms
- Desconexion: reconexion periodica cada 2s
- PID no disponible: retorna valor sentinela (-1, -99, -999)
- DTCs: reintento 3x con sleep 500ms en clearDTCs

## Dependencias

- libbluetooth-dev (RFCOMM)
- libgpiod-dev (GPIO DC/RESET)
- pthreads
- Linux spidev (/dev/spidev0.0)
- Linux gpiochip (/dev/gpiochip0)
