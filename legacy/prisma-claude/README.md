# Emulador Chevrolet Prisma — OBD2 / ELM327 sobre MCP2515 (Raspberry Pi)

Emulador de la ECU de un Chevrolet Prisma que expone datos dinámicos de
velocidad, RPM, temperatura de motor, carga, presión de combustible,
voltaje de batería, marcha estimada y otros parámetros típicos, respondiendo
tanto a solicitudes OBD2 reales sobre el bus CAN (a través de un MCP2515)
como a comandos de texto estilo ELM327 desde una consola interactiva.

## 1. Conexionado (Raspberry Pi ↔ MCP2515)

| Raspberry Pi | MCP2515 | Función                                  |
|--------------|---------|-------------------------------------------|
| MISO (GPIO9) | SO      | Salida de datos del MCP2515                |
| MOSI (GPIO10)| SI      | Entrada de datos del MCP2515               |
| SCLK (GPIO11)| SCK     | Reloj SPI                                  |
| CE0 (GPIO8)  | CS      | Chip Select (SPI0, hardware)               |
| GPIO25       | INT     | Interrupción (activa en bajo)              |
| 3.3V / 5V*   | VCC     | Alimentación (según módulo, ver datasheet) |
| GND          | GND     | Tierra común                               |

\* La mayoría de los módulos MCP2515 llevan un regulador a bordo y aceptan
5V en VCC, pero el bus SPI de la Raspberry Pi trabaja a 3.3V. Verifique que
su módulo sea tolerante a 3.3V en las líneas SPI o utilice un traductor de
niveles si es necesario.

## 2. Estructura del proyecto

```
.
├── bin/                  Binario compilado (aplicacion)
├── include/
│   ├── elm327.h          Interprete de comandos AT / respuestas OBD2
│   ├── mcp2515.h          Header de conveniencia -> incluye mcp2515/mcp2515.h
│   ├── mcp2515/
│   │   └── mcp2515.h      Driver real del controlador CAN MCP2515
│   ├── oled/               Reservado para un futuro display OLED (no usado aun)
│   └── vehicle.h          Simulacion dinamica del vehiculo
├── obj/                   Objetos intermedios de compilacion
├── scripts/
│   └── install_dependencies.sh   Instala la libreria bcm2835
├── src/
│   ├── elm327.cpp
│   ├── main.cpp            Menu interactivo y arranque de hilos
│   ├── mcp2515.cpp          Placeholder de compatibilidad (no se compila)
│   ├── mcp2515/
│   │   └── mcp2515.cpp     Implementacion real del driver MCP2515
│   ├── oled/                Reservado para futuro soporte OLED
│   └── vehicle.cpp
├── Makefile
└── README.md
```

## 3. Compilación

```bash
# 1) Instalar dependencias (libreria bcm2835)
make install_deps

# 2) Compilar
make

# 3) Ejecutar (requiere privilegios de root por acceso a /dev/mem via bcm2835)
sudo ./bin/aplicacion
# o bien:
make run
```

## 4. Uso — Menú interactivo

```
1) Iniciar emulacion              -> Enciende el motor simulado y comienza
                                      a responder solicitudes OBD2 en el bus CAN
2) Detener emulacion              -> Detiene los hilos de simulacion y CAN
3) Cambiar parametros en tiempo real -> Fija manualmente velocidad, RPM,
                                      temperatura, carga, presion, voltaje,
                                      nivel de combustible o acelerador
4) Seleccionar perfil de conduccion  -> Ralenti / Urbano / Carretera /
                                      Agresivo / Personalizado
5) Ver estado actual              -> Muestra un snapshot de todos los parametros
6) Consola de comandos ELM327     -> Prueba comandos AT y PIDs manualmente
                                      sin necesidad de un escaner externo
7) Salir
```

### Ejemplo de consola ELM327 (opción 6)

```
>> ATZ
ELM327 v1.5
>> ATE0
OK
>> 0100
41 00 98 3B 80 11
>> 010C
41 0C 1A F8
>> 010D
41 0D 2D
>> ATRV
12.6V
>> salir
```

## 5. PIDs OBD2 soportados (modo 01)

| PID  | Parámetro                          |
|------|-------------------------------------|
| 00   | PIDs soportados [01-20]              |
| 04   | Carga calculada del motor            |
| 05   | Temperatura del refrigerante         |
| 0A   | Presión de combustible               |
| 0C   | RPM                                  |
| 0D   | Velocidad del vehículo               |
| 0E   | Avance de encendido                  |
| 0F   | Temperatura de aire de admisión      |
| 10   | Caudal másico de aire (MAF)          |
| 11   | Posición del acelerador              |
| 1F   | Tiempo desde el arranque del motor   |
| 2F   | Nivel de combustible                 |
| 42   | Voltaje del módulo de control (bat.) |
| 46   | Temperatura ambiente                 |
| A6   | Odómetro                             |

También responde modo 03 (códigos de falla, sin fallas por defecto) y
modo 09 PID 02 (VIN simulado).

## 6. Notas

- El "estado de marcha" (P/R/N/D/1-5) se calcula internamente a partir de
  la velocidad simulada; no corresponde a un PID estándar SAE J1979, por lo
  que se expone únicamente por consola (opción 5) y no vía CAN.
- Los valores por defecto usan un cristal de 8 MHz para el MCP2515 (CNF1/2/3
  en `mcp2515.cpp`). Si su módulo usa 16 MHz, ajuste esas constantes.
- Ejecutar el binario requiere privilegios de root porque la librería
  bcm2835 accede directamente a `/dev/mem`.
