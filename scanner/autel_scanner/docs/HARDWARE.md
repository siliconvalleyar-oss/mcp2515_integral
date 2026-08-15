# Hardware Documentation

## Esquematico de Conexion

### MCP2515 (CAN Bus)
```
MCP2515 Module    Raspberry Pi    Descripcion
--------------------------------------------------
VCC              3.3V            Alimentacion 3.3V
GND              GND             Tierra
CS               GPIO8 (CE0)     Chip Select SPI
SCK              GPIO11 (SCLK)   SPI Clock
SI               GPIO10 (MOSI)   SPI Master Out
SO               GPIO9 (MISO)    SPI Master In
INT              GPIO25          Interrupcion (opcional)
--------------------------------------------------
CANH             CANH            Conector OBD-II pin 6
CANL             CANL            Conector OBD-II pin 14
```

### SSD1306 OLED 128x32
```
OLED Module      Raspberry Pi    Descripcion
--------------------------------------------------
VCC              3.3V            Alimentacion 3.3V
GND              GND             Tierra
SDA              GPIO2 (SDA1)    I2C Data
SCL              GPIO3 (SCL1)    I2C Clock
--------------------------------------------------
```

### OBD-II Connector
```
Pin 6 (CAN High)  → CANH del transceptor
Pin 14 (CAN Low)  → CANL del transceptor
Pin 4 (Chassis)   → GND
Pin 16 (Battery)  → +12V (para alimentar el modulo)
```

## Conexion Completa

```
Raspberry Pi
     │
     ├─── SPI (CE0) ─── MCP2515 ── SN65HVD230 ── OBD-II (Pin 6/14)
     │
     └─── I2C (SDA/SCL) ── SSD1306 OLED
```

## Lista de Componentes

1. Raspberry Pi (3B+ o superior recomendado)
2. MCP2515 CAN Bus Module
3. SN65HVD230 CAN Transceiver
4. SSD1306 OLED 128x32 Display
5. OBD-II Connector (macho)
6. Cables jumper
7. Protoboard (opcional)

## Notas

- El MCP2515 requiere un transceptor CAN (SN65HVD230) para conversión de niveles
- La alimentacion del modulo MCP2515 debe ser 3.3V para compatibilidad con Raspberry Pi
- El display OLED debe ser de 0.96" con chip SSD1306
- Se recomienda usar una fuente de alimentacion externa de 12V para el transceptor CAN
