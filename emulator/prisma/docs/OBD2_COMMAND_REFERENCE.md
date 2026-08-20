# Referencia de Comandos OBD2/UDS — Emulador ECU

> Generado desde traza real del escáner AUTEL conectado al emulador v1.8.0.
> Vehicle: Chevrolet Prisma 2018 (motor apagado, idle simulation).

---

## 1. Modo 01 — Datos actuales (SAE J1979)

| PID | Descripción | Comando | Respuesta | Decodificación |
|-----|-------------|---------|-----------|----------------|
| 00 | PIDs 01-20 soportados | `0100` | `41 00 BE 3F B8 13` | 01,03,04,05,06,07,0B-10,11,13,14,15,1C,1F,20 |
| 01 | Status MIL + DTCs | `0101` | `41 01 82 00 00 00` | MIL ON, 2 DTCs confirmados |
| 03 | Sistema combustible | `0103` | `41 03 02 00` | Lazo cerrado (closed loop) |
| 04 | Carga motor | `0104` | `41 04 2A` | 42/255 = **16.5%** |
| 05 | Temp. refrigerante | `0105` | `41 05 4E` | 78-40 = **38°C** |
| 06 | STFT Banco 1 | `0106` | `41 06 73` | (115-128)×100/128 = **-10.2%** |
| 07 | LTFT Banco 1 | `0107` | `41 07 80` | (128-128)×100/128 = **0%** |
| 0B | Presión MAP | `010B` | `41 0B 23` | **35 kPa** |
| 0C | RPM | `010C` | `41 0C 0C 4D` | 0x0C4D/4 = **787 RPM** |
| 0D | Velocidad | `010D` | `41 0D 00` | **0 km/h** (detenido) |
| 0E | Avance encendido | `010E` | `41 0E 9C` | (156/2)-64 = **14°** |
| 0F | Temp. admisión | `010F` | `41 0F 47` | 71-40 = **31°C** |
| 10 | Flujo MAF | `0110` | `41 10 01 C3` | 0x01C3/100 = **4.51 g/s** |
| 11 | Apertura mariposa | `0111` | `41 11 00` | **0%** |
| 13 | O2 Sensor B1S1 | `0114` | `41 14 82 8D` | Voltaje=0.41V, STFT=-10.2% |
| 1C | Tipo OBD | `011C` | `41 1C 06` | ISO 15765-4 CAN |
| 1F | Tiempo motor | `011F` | `41 1F 00 22` | **34 segundos** |
| 21 | Distancia DTC clear | `0121` | `41 21 00 00 30 39` | **12,345 km** |
| 2E | Purga EVAP | `012E` | `41 2E 12` | 18/255 = **7%** |
| 2F | Nivel combustible | `012F` | `41 2F B2` | 178/255 = **69.8%** |
| 31 | Distancia desde clear | `0131` | `41 31 00 00 30 39` | **12,345 km** |

---

## 2. Modo 09 — Información del vehículo

| PID | Descripción | Comando | Respuesta | Datos |
|-----|-------------|---------|-----------|-------|
| 00 | PIDs soportados | `0900` | `49 00 03 50 40 00 00` | 02, 04, 0A |
| 02 | VIN (17 chars) | `0902` | `49 02 01 39 42 47 ...` | **9BGKL48T0HB130763** |
| 04 | CALID (2 calibraciones) | `0904` | `49 04 02 31 35 30 ...` | **1505708** / **52124404** |
| 0A | Nombre ECU | `090A` | `49 0A 01 54 43 4D ...` | **TCM-Engine Control** |

---

## 3. Modo 22 UDS — DIDs GM (ECM, 0x7DF → 0x7E8)

### DIDs con respuesta válida (62)

| DID | Descripción | Comando | Respuesta | Decodificación |
|-----|-------------|---------|-----------|----------------|
| 1564 | Sincronización inyección | `22 1564` | `62 15 64 29` | **0x29 = 41** (valor fijo real) |
| 119E | AFR | `22 119E` | `62 11 9E 06 BD` | 0x06BD/100 = **17.25** |
| 19DE | Torque (ft-lbs) | `22 19DE` | `62 19 DE 00 37` | 0x0037 = **55 ft-lbs** |
| 11A1 | Tiempo desde arranque | `22 11A1` | `62 11 A1 00 36` | **54 segundos** |
| 2345 | Estado desconocido | `22 2345` | `62 23 45 00` | **0x00** (1 byte) |
| 162F | Balance rate cil 1 | `22 162F` | `62 16 2F 00 9C` | raw16=0x009C |
| 1630 | Balance rate cil 2 | `22 1630` | `62 16 30 00 9F` | raw16=0x009F |
| 1631 | Balance rate cil 3 | `22 1631` | `62 16 31 00 A0` | raw16=0x00A0 |
| 1193 | Ancho pulso inyector 1 | `22 1193` | `62 11 93 00 01` | raw16=0x0001 |
| 1194 | Ancho pulso inyector 2 | `22 1194` | `62 11 94 00 01` | raw16=0x0001 |
| 1195 | Ancho pulso inyector 3 | `22 1195` | `62 11 95 00 01` | raw16=0x0001 |
| 1196 | Ancho pulso inyector 4 | `22 1196` | `62 11 96 00 02` | raw16=0x0002 |
| 1197 | Ancho pulso inyector 5 | `22 1197` | `62 11 97 00 01` | raw16=0x0001 |
| 1198 | Ancho pulso inyector 6 | `22 1198` | `62 11 98 00 01` | raw16=0x0001 |
| 1199 | Ancho pulso inyector 7 | `22 1199` | `62 11 99 00 01` | raw16=0x0001 |

### DIDs sin soporte (NRC 7F 22 xx 31)

| DID | Descripción | Comando | Respuesta |
|-----|-------------|---------|-----------|
| F500 | — | `22 F500` | `7F 22 F5 00 31` |
| 1997 | — | `22 1997` | `7F 22 19 97 31` |
| 1993 | — | `22 1993` | `7F 22 19 93 31` |
| 1998 | — | `22 1998` | `7F 22 19 98 31` |
| 1994 | — | `22 1994` | `7F 22 19 94 31` |
| 1999 | — | `22 1999` | `7F 22 19 99 31` |
| 1995 | — | `22 1995` | `7F 22 19 95 31` |
| 199A | — | `22 199A` | `7F 22 19 9A 31` |
| 3201 | — | `22 3201` | `7F 22 32 01 31` |
| 1192 | — | `22 1192` | `7F 22 11 92 31` |
| 0052 | — | `22 0052` | `7F 22 00 52 31` |
| 1171 | — | `22 1171` | `7F 22 11 71 31` |
| 114B | — | `22 114B` | `7F 22 11 4B 31` |
| 1470 | — | `22 1470` | `7F 22 14 70 31` |
| 2344 | — | `22 2344` | `7F 22 23 44 31` |
| 1154 | — | `22 1154` | `7F 22 11 54 31` |
| 1170 | — | `22 1170` | `7F 22 11 70 31` |
| F432 | — | `22 F432` | `7F 22 F4 32 31` |
| 1145 | — | `22 1145` | `7F 22 11 45 31` |
| 1172 | — | `22 1172` | `7F 22 11 72 31` |
| 1141 | — | `22 1141` | `7F 22 11 41 31` |
| 129A | — | `22 129A` | `7F 22 12 9A 31` |

---

## 4. Modo 22 UDS — TCM (transmisión, 0x7E1 → 0x7E9)

| DID | Descripción | Comando | Respuesta | Decodificación |
|-----|-------------|---------|-----------|----------------|
| 1940 | Temp. ATF (TFT) | `22 1940` | `62 19 40 BE` | BE-40 = **94°C** |
| 199A | — | `22 199A` | `7F 22 19 9A 31` | No soportado |
| 280D | — | `22 280D` | `7F 22 28 0D 31` | No soportado |
| 210A | — | `22 210A` | `7F 22 21 0A 31` | No soportado |

---

## 5. Formato de respuesta UDS

### Respuesta positiva (62)
```
62 <DID hi> <DID lo> <datos...>
```

### Respuesta negativa (NRC)
```
7F <servicio> <DID hi> <DID lo> 31
                                   └─ requestOutOfRange (DID no soportado)
```

---

## 6. Fórmulas de decodificación

### Modo 01 (SAE J1979)
| PID | Fórmula | Unidad |
|-----|---------|--------|
| 0C | `(A×256+B)/4` | RPM |
| 0D | `A` | km/h |
| 04 | `A×100/255` | % carga |
| 05 | `A-40` | °C |
| 06/07 | `(A-128)×100/128` | % fuel trim |
| 0B | `A` | kPa |
| 0E | `A/2-64` | ° |
| 0F | `A-40` | °C |
| 10 | `(A×256+B)/100` | g/s |
| 11 | `A×100/255` | % |
| 1F | `A×256+B` | segundos |
| 21/31 | `A×2^24+B×2^16+C×256+D` | km |
| 2E | `A×100/255` | % |
| 2F | `A×100/255` | % |
| 42 | `A/10` | V |

### Modo 22 UDS
| DID | Fórmula | Unidad |
|-----|---------|--------|
| 119E | `raw16/100` | ratio AFR |
| 19DE | `raw16` | ft-lbs |
| 1564 | `raw8` | valor fijo |
| 11A1 | `raw16` | segundos |
| 162F-1636 | `(raw16×5/32)-20` | mm³ |
| 1193-1199 | `raw16×200/131` | ms |
| 1940 | `raw8-40` | °C (ATF) |

---

## 7. Secuencia típica del escáner AUTEL (Auto Scan)

```
1.  0100 → 41 00 BE 3F B8 13    (descubrir PIDs 01-20)
2.  0120 → 41 20 80 06 80 00    (descubrir PIDs 21-40)
3.  0902 → 49 02 01 ...          (VIN)
4.  0904 → 49 04 02 ...          (CALID)
5.  090A → 49 0A 01 ...          (ECU name)
6.  0101 → 41 01 82 ...          (MIL + DTC count)
7.  0103 → 41 03 02 00           (fuel system)
8.  0104-0114 → datos live       (cycling PIDs)
9.  22 1564 → 62 15 64 29        (GM DIDs)
10. 22 1940 → 62 19 40 XX        (ATF temp)
11. 22 1193-1199 → inyectores    (balance rates)
12. TCM 22 1940 → ATF desde TCM
```
