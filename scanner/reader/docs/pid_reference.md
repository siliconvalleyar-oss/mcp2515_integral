# Referencia de PIDs OBD-II

## PIDs Estandar (Modo 01)

| Sensor | PID | Bytes | Formula | Min | Max |
|--------|-----|-------|---------|-----|-----|
| RPM | 010C | 2 | (A*256+B)/4 | 0 | 16383 |
| Velocidad | 010D | 1 | A km/h | 0 | 255 |
| Temp. Refrigerante | 0105 | 1 | A-40 °C | -40 | 215 |
| Carga Motor | 0104 | 1 | A*100/255 % | 0 | 100 |
| Pos. Acelerador | 0111 | 1 | A*100/255 % | 0 | 100 |
| Presion Admision | 010B | 1 | A kPa | 0 | 255 |
| Temp. Admision | 010F | 1 | A-40 °C | -40 | 215 |
| Avance Encendido | 010E | 1 | A/2-64 ° | -64 | 63.5 |
| MAF | 0110 | 2 | (A*256+B)/100 g/s | 0 | 655.35 |
| Nivel Combustible | 012F | 1 | A*100/255 % | 0 | 100 |
| Presion Combustible | 010A | 1 | A*3 kPa | 0 | 765 |
| Presion Barometrica | 0133 | 1 | A kPa | 0 | 255 |
| Temp. Ambiente | 0146 | 1 | A-40 °C | -40 | 215 |
| Temp. Aceite | 015C | 1 | A-40 °C | -40 | 215 |

## Fuel Trim

| Sensor | PID | Bytes | Formula |
|--------|-----|-------|---------|
| STFT B1 | 0106 | 1 | ((A-128)*100)/128 % |
| STFT B2 | 0108 | 1 | ((A-128)*100)/128 % |
| LTFT B1 | 0107 | 1 | ((A-128)*100)/128 % |
| LTFT B2 | 0109 | 1 | ((A-128)*100)/128 % |

## Sensores O2

| Sensor | PID | Bytes | Voltage | Trim |
|--------|-----|-------|---------|------|
| B1S1 | 0114 | 2 | A*0.005V | ((B-128)*100)/128 % |
| B1S2 | 0115 | 2 | A*0.005V | ((B-128)*100)/128 % |
| B1S3 | 0116 | 2 | A*0.005V | ((B-128)*100)/128 % |
| B1S4 | 0117 | 2 | A*0.005V | ((B-128)*100)/128 % |
| B2S1 | 0118 | 2 | A*0.005V | ((B-128)*100)/128 % |
| B2S2 | 0119 | 2 | A*0.005V | ((B-128)*100)/128 % |
| B2S3 | 011A | 2 | A*0.005V | ((B-128)*100)/128 % |
| B2S4 | 011B | 2 | A*0.005V | ((B-128)*100)/128 % |

## PIDs GM Modo 22 (UDS)

| Sensor | PID | Bytes | Formula | Unidad |
|--------|-----|-------|---------|--------|
| Odometro | B100 | 4 | raw32 / 10 | km |
| Temp Catalizador | 01B4 | 2 | raw16*0.1 - 40 | °C |
| Presion Combustible | 1180 | 2 | raw16 * 4 | kPa |
| Torque Motor | 01A9 | 2 | raw16*0.5 - 848 | Nm |
| Voltaje ECU | 01A1 | 2 | raw16 * 0.001 | V |

## Codigos DTC

Formato: `P0XXX` a `U3XXX` (4 digitos hex)

| Tipo | Rango | Significado |
|------|-------|-------------|
| P | P0000-P3FFF | Powertrain |
| C | C0000-C3FFF | Chassis |
| B | B0000-B3FFF | Body |
| U | U0000-U3FFF | Network |
