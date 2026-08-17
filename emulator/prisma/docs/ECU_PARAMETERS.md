# ECU_PARAMETERS.md — Mapa de respuestas de la ECU emulada (GM / Chevrolet Prisma)

> **Fuente de verdad para la AI que corrija o amplíe el código del emulador.**
> Este documento cataloga TODAS las solicitudes que un escáner de diagnóstico
> (p. ej. AUTEL MaxiSYS, con su Live Data) hace al vehículo y que la ECU emulada
> debe responder con valores. El listado original fue extraído del Live Data del
> escáner sobre el vehículo de prueba (Chevrolet Onix, VIN `9BGKL48T0HB130763`,
> ~403 parámetros diagnosticables — ver `SKILL_AUTEL.md`).
>
> La ECU emulada es un **emulador de VEHÍCULO**: no solo responde PIDs estándar,
> también debe responder a los DIDs GM modo 22 que piden los escáneres
> profesionales (torque, odómetro, inyectores, misfire, ATF, etc.).

---

## 1. Cómo usar este documento (para la AI)

- **Dónde se implementa:** `src/elm327.cpp` — `getObdResponse(mode, pid, out, len)`
  despacha por modo (01/02, 03, 04, 06, 07, 08, 0A, 09). Los valores salen del
  modelo `Vehicle` (`src/vehicle.cpp`, clave en español p. ej. `v("rpm")`),
  actualizado por el `Simulator` a 10 Hz o fijado manualmente (FIJO).
- **Modo 22 UDS:** implementado en `getMode22()` (src/elm327.cpp), despachado
  desde `handleCanRequest()` (CAN) y `process()` (consola). El escáner pide
  `22 <DID 2 bytes>` y la ECU responde `62 <DID> <datos>` (single-frame; DIDs
  de >7 bytes usarían ISO-TP `sendIsoTp` como VIN/CALID). DID no soportado →
  NRC `7F 22 <DID> 31`.
- **Convención de estado:**
  - ✅ **implementado** — ya responde en `elm327.cpp` (rama ECU actual).
  - 🔶 **parcial** — responde con una variante o falta algún byte/campo.
  - ⬜ **pendiente** — hay que implementarlo (checklist en §7).
  - **por confirmar** — DID GM candidato; verificar contra capturas del escáner
    real (los modelos GM usan DIDs distintos para el mismo parámetro, por eso el
    escáner muestra "var.1/var.2/...").

### Leyenda de mecanismos

| Mecanismo | Descripción |
|---|---|
| `01/0C` | OBD2 estándar modo 01 PID 0x0C (SAE J1979 / ISO 15031-5) |
| `06/TID` | Modo 06, monitores on-board (test ID) |
| `09/02` | Modo 09 (VIN / CALID / ECU name) |
| `22 AABB` | **UDS modo 22, DID GM 0xAABB** (requiere `getMode22`) |
| `31 ...` | Rutina UDS (servicio 0x31) |
| `14 ...` | Borrado UDS (servicio 0x14) |
| AT | Comando AT del dongle ELM327 (lo responde la ECU emulada) |

---

## 2. Estado y diagnóstico general

| Parámetro (listado del escáner) | Mecanismo | Fórmula / encoding | Valor emulado sugerido | Estado |
|---|---|---|---|---|
| Voltage (ELM327) — 11.8 V | AT `ATRV` | responde `11.8V` | 12.5–14.2 V (usa `v("voltaje_bateria")`) | ✅ |
| Luz de fallo motor (MIL ENCENDIDA) | `01/01` bit 7 | MIL = 1 si hay DTCs | `0x80` si `dtcs` no vacío | ✅ |
| Código de falla 1 (DTC) | `03` (y `07`, `0A`) | 2 bytes → P/C/B/U + 4 dígitos | p. ej. `P0301` (`0x0301`) | ✅ |
| Estado de monitores (readiness: encendido, combustible, componentes, catalizador, calentador cat., EVAP, aire secundario, refrigerante A/C, O2, calentador O2, EGR) | `01/01` bits 0-6 + `06` TIDs `01,02,41,61,91` | bit 7 = MIL; monitores = bits de disponibilidad/completado | "No-disponible/Completado" = `0x11` (disponible=1, completado=1) | ✅ (01/01 y 06 TIDs ya responden) |
| Tiempo actual — 02:14 | `01/1F` | tiempo de marcha, A×256+B segundos | 2 min 14 s = 134 s (`0x0086`) | ✅ (0x1F) · alt `22 11 A1` ⬜ |
| Estado del sistema de combustible (bucle cerrado) | `01/03` | 2 bytes; 0x02 = bucle cerrado usando O2 | `0x02 0x00` | ✅ |
| "Estado de los pulsos o parte del mensaje del módulo" | — | item de UI del escáner, no es un PID | — | nota (no implementable) |
| Restablecer distancia / combustible utilizado / velocidad gasolina / gasolina prevista | rutina UDS `31` o DID de viaje | DID GM por confirmar | — | ⬜ por confirmar |

---

## 3. Motor — PIDs OBD2 modo 01

| Parámetro (listado) | PID | Fórmula / encoding | Valor emulado sugerido | Estado |
|---|---|---|---|---|
| Valor calculado de la carga del motor — 25.1 % | `01/04` | A×100/255 | `v("carga_motor")` → 25.1 % | ✅ |
| Temperatura del líquido de enfriamiento — 54 °C | `01/05` | A−40 | `v("temp_refrigerante")` | ✅ |
| Ajuste de combustible a corto plazo B1 — −19.10 % | `01/06` | (A−128)×100/128 | `v("stft1")` | ✅ |
| Ajuste de combustible a largo plazo B1 | `01/07` | (A−128)×100/128 | `v("ltft1")` | ✅ |
| Ajuste de combustible a **corto plazo B2** | `01/08` | (A−128)×100/128 | `v("stft2")` | ✅ |
| Ajuste de combustible a **largo plazo B2** | `01/09` | (A−128)×100/128 | `v("ltft2")` | ✅ |
| Presión del combustible — kPa | `01/0A` | A×3 | `v("presion_combustible")` 250–400 kPa | ✅ (alt `22 11 80`) |
| Revoluciones del motor — rpm | `01/0C` | (A×256+B)/4 | `v("rpm")` | ✅ |
| Velocidad del vehículo — km/h | `01/0D` | A | `v("velocidad")` | ✅ |
| Avance del encendido | `01/0E` | A/2−64 | 5–35 ° | ✅ |
| Sensor de oxígeno 1 Banco 1 Voltaje — V | `01/13` | A×0.005 | `v("sonda_o2")` ≈ 0.45 V | ✅ |
| Sensor de oxígeno 2 Banco 1 (trim) — % | `01/14` | A×0.005 V, B=(B−128)×100/128 | trims O2 | ✅ |
| Sensor de oxígeno 1 Banco 2 Voltaje — V | `01/17` | A×0.005 | 0.45 V | ✅ |
| Sensores O2 restantes (B1S3/S4, B2S2/S3/S4) | `01/15,16,18,19,1A` | A×0.005 V, B trim | 0.45 V / trim | ✅ |

> **Nota de mapeo O2:** el emulador usa el mapeo **SAE J1979** (`0x13`=B1S1,
> `0x14`=B1S2, …, `0x17`=B2S1 … `0x1A`=B2S4). El lector `scanner/reader`
> mapea `0x14`=B1S1 (no estándar): si se usa ese lector contra este emulador,
> los valores O2 pueden aparecer en otra posición.
| Distancia recorrida — km (con MIL) | `01/21` | A×256+B km | `distanceSinceMIL` | ✅ |
| Distancia recorrida-(Total / Germania / Real) — 1.66 km | `01/31` (desde clear) + odómetro | A×256+B km | `distanceSinceClearKm` (25) | ✅ 0x31 · odómetro `22 B100` ✅ |
| Economizador de combustible — % | — | calculado por el escáner | — | nota (no es PID) |
| Revoluciones del motor ×1000 — 0.1 rpm | `01/0C` | mismo PID, escala del display | — | ✅ (display) |
| Velocidad media — km/h | — | calculado por el escáner | — | nota (no es PID) |
| A/C high pressure / Air Con High Side Pressure — kPa | `22` DID | DID GM por confirmar | 1400–2000 kPa | ⬜ por confirmar |
| Desired Idle Speed — rpm | DID GM `22` (por confirmar) | — | 750–900 rpm | ⬜ por confirmar (en `01/47` la ECU responde throttle absoluta B, SAE) |
| E85 (alcohol) content in fuel — % | `01/52` | A×100/255 | `v("etanol")` 10 % | ✅ |
| EGR Duty Cycle — % | `01/2C` | A×100/255 | `v("egr_duty")` (crear) | ✅ 0x2C · falta v("egr_duty") |
| EGR V — V | `22` DID | DID GM por confirmar | 0.5–4.5 V | ⬜ por confirmar |
| Elapsed Time Since Engine Start — sec. | `01/1F` | A×256+B s | 0–600 s | ✅ (0x1F) · alt `22 11 A1` ⬜ |
| Engine Oil Pressure var.1/2/3 — kPa | `22` DID | DIDs GM por confirmar (Duramax usa `22 18 94`) | 100–450 kPa | ⬜ por confirmar |
| Engine Oil Temp — °C | `01/5C` | A−40 | `v("temp_aceite")` | ✅ |
| Engine Torque — N·m | `22 01 A9` (reader) · alt `22 19 DE` (CANSF) | (A×256+B)×0.5−848 | `v("torque")` 0–320 N·m | ✅ (01A9) |
| EVAP — % | `01/2E` | A×100/255 | `v("evap_purge")` | ✅ |
| Fuel Tank Pressure — kPa | `01/53` | raw16 firmado (offset 0x8000), 4 Pa/bit | `v("presion_tanque")` −1..0 kPa | ✅ |
| HO2S Sensor — mV | `01/13`–`01/1A` | A×5 mV | 0–1000 mV | 🔶 (solo 0x13) |
| IAC Position | `22` DID | DID GM por confirmar | 0–100 % | ⬜ por confirmar |
| Ignition 1 Voltage — V | `01/42` | A/10 | `v("voltaje_bateria")` | ✅ |
| Injector Pulse Width Cyl 1-8 — ms | `22 11 93`…`22 11 9A` (ScanGauge) | raw16 = ms×128 (fórmula por confirmar) | `v("inyector_pw")` 1.5–6 ms | ✅ (fórmula por confirmar) |
| Intake Air Temp. 2 (IAT2) — °C | `22` DID | DID GM por confirmar (L5P: `2C FE 80 0A`) | 25–40 °C | ⬜ por confirmar |
| Knock Retard / (ALT) — ° | `22 11 A6` (ScanGauge) | raw16×0.5 | `v("knock_retard")` 0–8 ° | ✅ (getMode22) |
| Knock Sensor Active Counter | `22` DID | DID GM por confirmar | 0–255 | ⬜ por confirmar |
| Misfire Cyl 1-8 Current / History | `01/56`–`01/59` | nibble por cilindro (56/58 = actual, 57/59 = histórico); cil 1 = nibble alto de A | 0 | ✅ (01/56-59) |
| Odometer (engine units) — km | `22 B1 00` (reader) | 4 bytes BE /10 | `v("odometro")` ≈ 12345.6 km | ✅ |
| Outside Air Temp — °C | `01/46` | A−40 | `v("temp_ambiente")` | ✅ |
| PC Solenoid Actual/Reference Current — mA | `22` DIDs | DID GM por confirmar | 400–1000 mA | ⬜ por confirmar |
| Remaining Oil Life — % | `22 11 9F` (ScanGauge) | raw16 = %×255/100 (fórmula por confirmar) | `v("oil_life")` 85 % | ✅ (fórmula por confirmar) |
| Barometer V6 / V8 — kPa | `01/33` · alt `22 12 51` (V6) / `22 11 9D` (V8) | A · raw16×0.1 kPa (por confirmar) | `v("baro")` | ✅ |
| Air to Fuel Ratio / AFT Commanded | `01/44` (λ, raw16/32768) · alt `22 11 9E` (ScanGauge) | λ×14.7 = ratio | 14.0–14.7 | ✅ (0x44) |
| Balance Rate Cyl 1-8 | `22 16 2F`…`22 16 36` (ScanGauge) | raw16 = mm³×100 (por confirmar) | `v("balance_rate")` 4.4 mm³ | ✅ (fórmula por confirmar) |
| Current Gear / var.2 / var.3 | `01/4E` (PID custom prisma: 0=N,1-5,R=6) | A | `v("marcha")` | ✅ 0x4E · DID GM `22` ⬜ por confirmar |

---

## 4. Transmisión / ABS (otras ECU del bus)

El escáner pide estos datos a la **TCM (7E1)** o al **módulo ABS**, no al PCM.
La ECU emulada (PCM, responde desde 0x7E8/0x7E9) puede responderlos si se
configura como "vehículo completo", pero por defecto **marcar como no soportado**
(negativa 0x7F) o implementarlos opcionalmente en el mismo emulador:

| Parámetro (listado) | Mecanismo | Estado |
|---|---|---|
| Temperatura de ATF var.1/2/4/5/6/7/9 — °C | `22 19 40` (TFT, ScanGauge) y variantes por modelo | ⬜ por confirmar |
| 1-2 / 2-3 / 3-4 Shift Error — sec. | `22` DID TCM | ⬜ por confirmar |
| 1-2 / 2-3 / 3-4 Shift Time — sec. | `22` DID TCM | ⬜ por confirmar |
| Last Shift Time — sec. | `22` DID TCM | ⬜ por confirmar |
| TCC Slip Speed — rpm | `22` DID TCM | ⬜ por confirmar |
| Gear ratio | `22` DID TCM | ⬜ por confirmar |
| Output shaft speed — rpm | `22` DID TCM | ⬜ por confirmar |
| Transmission ISS — rpm | `22` DID TCM | ⬜ por confirmar |
| Transmission Fluid Temp (7E2) | `22 19 40` (CAN id 7E2) | ⬜ por confirmar |
| ABS Rear Right Wheel Speed — km/h | `22` DID ABS | ⬜ por confirmar |
| Service reminder reset | rutina UDS `31` (solo algunos modelos) | ⬜ por confirmar |

---

## 5. DIDs GM modo 22 — prioridad de implementación

La **fuente primaria** son los DIDs que ya usa el lector del mismo monorepo
(`scanner/reader/src/gm_commands.cpp` + `docs/pid_reference.md`): son los que el
escáner/lector pide sobre este mismo vehículo, así que el emulador debe
responderlos primero. Los DIDs CANSF provienen de la referencia ScanGauge GM
(https://www.scangauge.com/support/x-gauge-commands/gm/).

| Parámetro | DID `22` | Fórmula / encoding | Estado |
|---|---|---|---|
| Odómetro | `B1 00` | 4 bytes BE / 10 → km | ✅ (getMode22) |
| Temp. catalizador | `01 B4` (alt `01 B5`) | raw16×0.1 − 40 → °C | ✅ (getMode22) |
| Presión combustible | `11 80` (alt `11 81`) | raw16×4 → kPa | ✅ (getMode22) |
| Torque motor | `01 A9` | raw16×0.5 − 848 → N·m | ✅ (getMode22) |
| Voltaje ECU | `01 A1` (alt `02 80`) | raw16×0.001 (alt ×0.1) → V | ✅ (getMode22) |
| Oil life restante | `11 9F` | raw16 = %×255/100 (por confirmar) | ✅ (getMode22) |
| Inyector PW cyl 1-8 | `11 93`…`11 9A` | raw16 = ms×128 (por confirmar) | ✅ (getMode22) |
| Historial DTC | servicio `19 02 FF` → `59 02` | estado: bit 0x08 CONF, 0x01 ACT | ⬜ |
| Borrar historial | `14 FF FF FF` → `54` | — | ⬜ |
| Reset adaptativos | `31 01 C1 0F` → `71` | — | ⬜ |
| ATF temp (TFT) | `19 40` | °F / °C (verificar offset) | ⬜ |
| Torque (alt) | `19 DE` | Ft-Lbs (×1.3558 → N·m) | ⬜ |
| AFR | `11 9E` | ratio | ⬜ |
| Baro V6 / V8 | `12 51` / `11 9D` | kPa o inHg | ⬜ |
| Knock retard | `11 A6` | raw16×0.5 → ° | ✅ (getMode22) |
| Tiempo desde arranque | `11 A1` | raw16 = segundos | ✅ (getMode22) |
| Baro V6 / V8 | `12 51` / `11 9D` | raw16×0.1 → kPa (por confirmar) | ✅ (getMode22) |
| Balance rate cyl 1-8 | `16 2F`…`16 36` | raw16×0.01 → mm³ (por confirmar) | ✅ (getMode22) |
| Presión aceite | `18 94` (Duramax; Onix por confirmar) | kPa/PSI | ⬜ |
| EGR V / IAC / PC solenoids / knock counter | por confirmar | — | ⬜ |

> DID no soportado → la ECU responde NRC `7F 22 <DID> 31` (requestOutOfRange).

---

## 6. Respuesta negativa y formato

- PID modo 01 no soportado → **no listarlo** en las máscaras de soportados
  (`01/00`, `01/20`, `01/40`, `01/60`) y no responder (o `7F` según escáner).
- Modo 22 DID no soportado → responder NRC `0x7F 22 <DID> 31` (rango) — igual que
  el lector mapea `7F 11`/`7F 22`/`7F 31`.
- Single-frame cuando `DLC ≤ 7`; multi-frame ISO-TP con Flow Control para VIN,
  CALID y DIDs de 4+ bytes (ya implementado en `sendIsoTp`).

---

## 7. Checklist de implementación (para la AI que corrija el código)

- [x] **Modo 22 UDS:** `getMode22()` implementado y despachado desde
      `handleCanRequest()` (CAN) y `process()` (consola). Respuesta
      `62 <DID> <datos>`; DID no soportado → NRC `7F 22 <DID> 31`. El escáner
      envía `22` con CAN id `0x7E0` físico o `0x7DF` funcional; se responde
      desde `0x7E8`/`0x7E9`.
- [x] Responder DIDs prioridad 1: `B100`, `01B4`, `1180`, `01A9`, `01A1`
      (paridad con `scanner/reader`).
- [ ] Servicios UDS `19 02` (historial), `14` (borrar historial),
      `31 01 C1 0F` (reset adaptativos) — pendientes.
- [x] PIDs modo 01 `08` (STFT B2) y `09` (LTFT B2).
- [x] PIDs modo 01 `0A` (presión combustible), `14-1A` (sensores O2
      B1S2-B2S3), `44` (λ), `47` (throttle absoluta B), `52` (E85),
      `53` (presión de tanque).
- [x] PIDs modo 01 `56-59` (misfire por cilindro: nibble por cilindro,
      cil 1 = nibble alto de A; 56/58 = actual, 57/59 = histórico).
- [x] DIDs CANSF `119F` (oil life) y `1193-119A` (inyectores).
- [x] DIDs CANSF: `1251`/`119D` (baro), `11A6` (knock retard), `11A1`
      (tiempo desde arranque), `162F-1636` (balance rate).
- [ ] DIDs CANSF restantes: `1940` (ATF), `19DE`, `119E`.
- [x] Parámetros añadidos al modelo `Vehicle` + `Simulator::tick()`: `stft2`,
      `ltft2`, `odometro`, `torque`, `oil_life`, `inyector_pw`, `etanol`,
      `presion_tanque`, `misfire_actual`, `misfire_hist`, `knock_retard`,
      `balance_rate`.
- [ ] Parámetros pendientes: `egr_duty`, `presion_aceite`, `temp_atf`, etc.
- [x] Máscaras de PIDs soportados corregidas a SAE J1979 (bit7 = PID más
      bajo): `01/00` → `BF FF BF D2`, `01/20` → `80 06 80 00`,
      `01/40` → `5E 94 67 90` (PIDs realmente implementados, incl. 56-59).
- [ ] Marcar DIDs "por confirmar" en este documento cuando se verifiquen contra
      el escáner real (quitar la etiqueta y fijar fórmula/unidad).

---

## 8. Referencias

- `docs/SKILL_AUTEL.md` y `docs/ANALISIS_MENU_SCANNER_AUTEL.md` — menú/Live Data
  del escáner AUTEL (403 parámetros, vehículo de prueba Onix).
- `scanner/reader/docs/pid_reference.md` + `scanner/reader/src/gm_commands.cpp`
  (rama `scanner`) — DIDs GM modo 22 que pide el lector del monorepo.
- ScanGauge X-Gauges GM — DIDs GM CANSF de referencia:
  https://www.scangauge.com/support/x-gauge-commands/gm/
- SAE J1979 / ISO 15031-5 — PIDs modo 01 estándar.
