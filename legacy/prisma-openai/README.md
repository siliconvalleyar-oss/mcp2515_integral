# Chevrolet Prisma OBD/CAN Emulator

Emulador de ECU/OBD-II para Raspberry Pi utilizando:

- Raspberry Pi
- MCP2515
- SPI
- librería bcm2835
- CAN 11-bit
- 500 kbit/s
- ELM327/OBD-II

## ADVERTENCIA

Este proyecto está destinado a pruebas en banco y desarrollo.

No conectar directamente a un vehículo en circulación.

La transmisión de mensajes CAN incorrectos puede afectar módulos electrónicos
del vehículo.

## Conexiones

Raspberry Pi -> MCP2515

| Raspberry Pi | MCP2515 |
|--------------|---------|
| MOSI         | SI      |
| MISO         | SO      |
| SCLK         | SCK     |
| CE0          | CS      |
| GPIO25       | INT     |
| GND          | GND     |

SPI:

    SPI0
    CE0

GPIO25 corresponde al pin físico 22 de la Raspberry Pi.

## CAN

Configuración inicial:

    500 kbit/s
    CAN 11-bit
    respuesta ECU: 0x7E8
    solicitud ECU: 0x7E0
    broadcast OBD: 0x7DF

El cristal del MCP2515 debe ser compatible con los valores de CNF utilizados
en el código. La implementación incluida está configurada para un MCP2515
con cristal de 8 MHz.

## Instalación

Ejecutar:

    ./scripts/install_dependencies.sh

Después habilitar SPI:

    sudo raspi-config

Seleccionar:

    Interface Options
    SPI
    Enable

## Compilación

    make

Resultado:

    bin/aplicacion

Para ejecutar:

    sudo ./bin/aplicacion

o:

    make run

## Menú

El programa permite:

1. iniciar simulación
2. detener simulación
3. visualizar parámetros
4. modificar parámetros
5. seleccionar modo
6. seleccionar perfil
7. ejecutar comandos ELM327/OBD
8. salir

## Modos

### DYNAMIC

Simula la evolución de:

- velocidad
- RPM
- carga
- acelerador
- MAP
- MAF
- temperatura
- presión de combustible
- tensión

### FIXED

Los parámetros se mantienen en los valores configurados mediante el menú.

Ejemplo:

    speed = 60
    rpm = 2200
    coolant = 90

### RANDOM

Introduce variaciones controladas para pruebas de software de diagnóstico.

## Perfiles

NORMAL
: comportamiento normal.

SPORT
: RPM y carga superiores.

ECONOMY
: RPM/carga reducidas.

FAILSAFE
: simulación de un estado de funcionamiento limitado.

## Parámetros modificables

    speed
    rpm
    coolant
    load
    fuel
    voltage
    throttle
    iat
    map
    maf
    gear

## Comandos ELM327

Implementados, entre otros:

    ATZ
    ATI
    AT@1
    ATE0
    ATE1
    ATS0
    ATS1
    ATH0
    ATH1
    ATL0
    ATL1
    ATSP0
    ATSP6
    ATDP
    ATDPN
    ATCAF0
    ATCAF1
    ATSH7E0
    ATSH7E8
    ATMA

## OBD-II

Ejemplos:

    0100
    0104
    0105
    010A
    010B
    010C
    010D
    010F
    0110
    0111
    011F
    012F
    0142

Respuestas:

    7E8

con formato ISO-TP de una sola trama para los PIDs implementados.

## Ejemplos

RPM:

    010C

Velocidad:

    010D

Temperatura:

    0105

Carga:

    0104

Tensión:

    0142

Presión de combustible:

    010A

## Notas

El proyecto implementa una ECU virtual genérica compatible con el modelo de
datos OBD-II.

Los PIDs propietarios o específicos del Chevrolet Prisma pueden variar según
año, motor, ECU y mercado.

La información de "marcha" no debe asumirse como un PID universal de todos los
Prisma/ECU. Se incluye como dato experimental para pruebas.

Antes de conectar hardware real se recomienda probar:

    MCP2515 -> Raspberry Pi -> CAN transceiver -> CAN analyzer

en una red CAN aislada.