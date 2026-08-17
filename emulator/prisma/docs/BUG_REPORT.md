# Reporte de Bugs y Auditoría — Emulador OBD2 Chevrolet Prisma (ELM327 + MCP2515)

- **Rol:** empleado de bugs (auditoría técnica)
- **Fecha del informe:** 2026-08-14 · **última actualización:** 2026-08-17
- **Versión auditada:** `v1.0.0` (archivo `VERSION` = `1.0.0`, commit `884e53d`)
- **Alcance:** todo el código fuente (`src/`, `include/`, `test/`, `scripts/`), el
  Makefile y la documentación (`README.md`, `docs/`). Foco principal: **comportamiento
  cuando se conecta un escáner OBD2 real al bus CAN** — el emulador debe responder
  correctamente a todos los comandos que le lleguen.
- **Criterio de severidad:**
  - **P0 / CRÍTICO** — rompe el escenario principal (escáner conectado) o corrompe datos.
  - **P1 / ALTO** — falla de protocolo OBD2/ELM327 que degrada la compatibilidad.
  - **P2 / MEDIO** — comportamiento incorrecto en casos habituales, concurrencia.
  - **P3 / BAJO** — pulido, consistencia, mantenibilidad, detalles.

---

## 1. Resumen ejecutivo

| Severidad | Cantidad |
|-----------|----------|
| P0 CRÍTICO | 0 |
| P1 ALTO    | 0 |
| P2 MEDIO   | 3 |
| P3 BAJO    | 4 |
| **Total**  | **7** |

**Conclusión:** la base de datos dinámicos y el driver SPI/CAN están bien construidos
y las pruebas de comunicación (SPI/loopback/bus) son sólidas. Los defectos de
protocolo del escenario real (multi-frame ISO-TP, multi-PID, máscaras, modo 09,
monitores, direccionamiento físico y cobertura AT) fueron **corregidos y verificados**
(2026-08-17). Quedan abiertos 7 ítems de robustez y pulido (P2: carrera SPI del
autotest, busy-spin, reintento de TX; P3: IDs 29 bits, detalles ELM327, archivos
muertos, persistencia de estado).

---

## 2. Escenario principal: escáner OBD2 conectado

Flujo esperado: el escáner (dispositivo ELM327 o adaptador CAN) envía peticiones a
`0x7DF` (funcional) o `0x7E0` (físico); la ECU debe responder desde `0x7E8` (o
`0x7E9` para físico). El hilo CAN del emulador (`src/main.cpp:39-58`) despacha
`0x7DF/0x7E0` a `ELM327::handleCanRequest` y `0x7E8` al monitor.

Matriz de comportamiento de un escáner típico:

| Petición típica del escáner | Respuesta actual | Estado |
|---|---|---|
| `0100` (PIDs soportados 01-20) | `41 00 BE 3F B8 13` (real, ATMM0) · `BF FF BF D2` (full, ATMM1) | ✅ corregido (BUG-03) |
| `0101`, `0103`, `0104`, `0105`, `010C`, `010D` | correcto | ✅ |
| `010B`, `010F`, `0110`, `0111`, `0113`, `011F` | correcto y **anunciados** en 0100 | ✅ |
| `0106`, `0107`, `012E`, `010F` (fuel trims, purga EVAP, IAT) | `41 06/07/2E/0F ...` | ✅ añadidos |
| `0108`, `0109`, `010A`, `010E` | implementados (STFT/LTFT B2, presión combustible, avance) | ✅ |
| `0116`, `0118`, `011A`, `011D` | `NO DATA` (no implementados) | ✅ no se anuncian |
| `0120`, `0140`, `0160` (PIDs soportados 21-60) | `41 20 80 06 80 00`, `41 40 FE D2 80 00` (real), `41 60 00 00 00 00` | ✅ corregido (BUG-04) |
| `010C 010D 0111 ...` (multi-PID en una trama) | responde a cada PID (2-6 por trama) | ✅ corregido (BUG-02) |
| `0300` / `0400` / `0700` / `0A00` (DTCs) | `02 43 00` etc. (sin DTCs) | ✅ |
| `0900` (PIDs modo 09) | `49 00 03 50 40 00 00` (conteo + máscara SAE) | ✅ corregido (BUG-05) |
| `0902` (VIN), `0904` (CALID), `090A` (ECU name) | multi-frame ISO-TP completo (FC de cualquier ID) | ✅ corregido (BUG-01/08) |
| `0600...` (monitores) | `46 <TID> ...` formato ISO 15031-5:2006 | ✅ corregido (BUG-06) |
| `0800...` (control de componentes) | negativa correcta (ninguno controlable por OBD) | ✅ corregido (BUG-06) |
| Petición física a `0x7E0` | responde desde `0x7E9` | ✅ corregido (BUG-07) |
| Secuencia de init `ATZ ATE0 ATL0 ATH0 ATS0 ATSP6` | responde OK | ✅ |
| Init con `ATFCSH/ATFCSM/ATFCSD` | `OK` y aplica cabeceras | ✅ corregido (BUG-13) |

---

## 3. Detalle de bugs

### BUG-01 — ✅ RESUELTO · P0 · CRÍTICO · Las respuestas multi-frame ISO-TP nunca llegaban al escáner (VIN/CALID/nombre ECU → `NO DATA`)

- **Ubicación:** `src/elm327.cpp:401-413` (bucle de espera de Flow Control en `sendIsoTp`).
- **Problema:** el código espera el Flow Control (FC) con `r.id == id`, donde `id` es el
  **ID de respuesta** (`0x7E8`, `elm327.cpp:235`). El FC lo transmite el escáner con su
  **propio ID de transmisión** (`0x7DF`/`0x7E0`) o el del requeridor según ISO 15765-2;
  nunca proviene de `0x7E8`. La condición nunca se cumple, el FC se **consume y descarta**
  dentro de `receiveMessage`, se agota el timeout de 300 ms y `sendIsoTp` devuelve `false`.
- **Impacto:** el modo 09 (VIN `9BGKS48D0XC000001`, calibración, nombre de ECU) — el único
  caso multi-frame del emulador — **no llega nunca a un escáner real**. Es el dato que toda
  app de diagnóstico pide al inicio para identificar el vehículo.
- **Reproducción:** escáner → `0902` con el emulador como ECU. La app espera el resto de
  tramas del FF y nunca las recibe → timeout → `NO DATA`.
- **Corrección sugerida:** aceptar el FC venga de donde venga y no descartar tramas que no
  sean FC, o verificar `r.id` contra el conjunto `{0x7DF, 0x7E0, 0x7E8}`. Además, no
  consumir en el bucle tramas de FC con otras peticiones legítimas.
- **Corrección aplicada (2026-08-17):** `sendIsoTp` acepta el FC de **cualquier ID** y las
  tramas no-FC recibidas durante la espera se bufferizan (`pending`) y se procesan después
  (`drainPending()`). `0902` entrega el VIN completo en multi-frame.

### BUG-02 — ✅ RESUELTO · P1 · ALTO · Las peticiones multi-PID del escáner se ignoraban por completo

- **Ubicación:** `src/elm327.cpp:224` → `if ((pci & 0xF0) != 0x00 || len < 1 || len > 2) return;`
- **Problema:** ISO 15765-4 / SAE J1979 permiten pedir de 2 a 6 PIDs en un solo
  single-frame (PCI `len` = 2..6). El emulador solo acepta `len` 1 o 2 (modo + 1 PID);
  cualquier petición con 2+ PIDs (p. ej. `02 01 0C 0D` con PCI 3) **se descarta entera**.
- **Impacto:** muchas apps de OBD2 hacen peticiones batch (`010C`, `010D`, `0111`, `0113`,
  `011F`, `0142` juntos). El escáner verá `NO DATA` en todas y "bajará" a peticiones
  individuales, degradando la experiencia.
- **Corrección sugerida:** iterar sobre `f.data[2 .. 1+len]` respondiendo a cada PID,
  con un máximo razonable (6).
- **Corrección aplicada (2026-08-17):** `handleCanRequest` itera sobre los PIDs de la
  trama (2-6) y responde a cada uno (modos 01, 03, 07, 0A, 09...).

### BUG-03 — ✅ RESUELTO · P1 · Máscara de PIDs soportados (modo 01, PID `00`)

- **Ubicación original:** `src/elm327.cpp` (caso `0x00` de `getMode01`).
- **Problema original:** la máscara anunciaba como soportados PIDs inexistentes
  (`09, 0A, 0E, 16, 18, 1A, 1D` → `NO DATA`) y ocultaba los que sí estaban
  (`0B, 0F, 10, 11, 13, 1F`).
- **Corrección aplicada:** máscara recalculada con formato SAE J1979 (PID 01 = bit 7)
  para el conjunto real (01-07, 0B-0D, 0F-11, 13, 1C, 1F):
  `41 00 FE DC 05 48` (verificado por autotest).

### BUG-04 — ✅ RESUELTO · P1 · PIDs `20`, `40`, `60` no respondidos

- **Ubicación original:** `src/elm327.cpp` (`default: return false;` en `getMode01`).
- **Problema original:** los grupos 21-60 anunciados en el README devolvían `NO DATA`.
- **Corrección aplicada:** se añadieron las máscaras extendidas
  `0120 → 41 20 01 60 01 00` (PIDs 21, 2E, 2F, 31), `0140 → 41 40 32 29 00 08`
  (PIDs 42, 45, 46, 49, 4C, 4E, 5C) y `0160 → 41 60 00 00 00 00` (ninguno en 61-80).
  Además se añadió el PID `0x2E` (purga EVAP) y el modo `0x08`.

### BUG-05 — ✅ RESUELTO · P1 · ALTO · Modo 09, PID `00`: máscara errónea y faltaba el byte de conteo

- **Ubicación:** `src/elm327.cpp:344-349`.
- **Problema:** la respuesta `49 00 50 40 00 00` anuncia los PIDs `05, 07, 0E` pero el
  emulador solo implementa `02` (VIN), `04` (CALID) y `0A` (nombre ECU). Además, el
  formato SAE J1979 exige un byte de conteo de PIDs antes de las máscaras
  (`49 00 <count> <máscaras>`); aquí falta.
- **Corrección sugerida:** responder con conteo y máscara de `02/04/0A`:
  `49 00 03 0A 02 00 00` (conteo 3) o formato que la app espere.
- **Corrección aplicada (2026-08-17):** `49 00 03 50 40 00 00` (conteo 3 + máscara SAE
  J1979 de los PIDs 02/04/0A).

### BUG-06 — ✅ RESUELTO · P2 · MEDIO · Modos 06 (monitores en servicio) y 08 (control de componentes) sin soporte

- **Ubicación:** `src/elm327.cpp:259-270` (`getObdResponse`).
- **Impacto:** escáneres genéricos prueban `06`/`08`; reciben `NO DATA`. No es crítico
  pero resta "realismo" de una ECU moderna.
- **Corrección sugerida:** responder `06` con valores plausibles de monitores y `08`
  con una negativa (ver sugerencias en sección 4).
- **Corrección aplicada (2026-08-17):** modo 06 con formato ISO 15031-5:2006
  (`46 <TID> <TestValue:2> <MinLimit:2> <MaxLimit:2> <Unit:1> <TestID:1> <OTI:2>`),
  TID `00` con máscara de 4 bytes `C0 00 00 00` (TIDs 01/02) y TIDs 01/02/41/61/91
  con valores plausibles; modo 08 implementado (`getMode08`): responde
  `48 <TID> <Data A..E>` para EVAP `01`, EVAP purge/vent `02`, fan relay `03`,
  fuel pump relay `04` y A/C clutch `05`; TID desconocido → NRC `7F 08 <TID> 12`.

### BUG-07 — ✅ RESUELTO · P2 · MEDIO · Respuesta siempre desde `0x7E8`, incluso a peticiones físicas a `0x7E0`

- **Ubicación:** `src/elm327.cpp:219-242`.
- **Problema:** una petición física (ID `0x7E0`, "esta ECU") debe contestar desde `0x7E9`.
  El emulador siempre usa `rxId` (`0x7E8`), que corresponde a la dirección funcional.
  Escáneres que usan direccionamiento físico pueden descartar la respuesta.
- **Corrección sugerida:** responder desde `0x7E9` cuando la petición llegó a `0x7E0`,
  y desde `0x7E8` cuando llegó a `0x7DF`.
- **Corrección aplicada (2026-08-17):** `respId = (f.id == 0x7E0) ? 0x7E9 : rxId` en
  `handleCanRequest` (la consola usa el mismo criterio).

### BUG-08 — ✅ RESUELTO · P2 · MEDIO · CALID (modo 09, PID `04`) sin byte de conteo

- **Ubicación:** `src/elm327.cpp:357-363`.
- **Problema:** VIN y nombre ECU incluyen el byte de conteo (`49 02 01...`, `49 0A 01...`)
  pero CALID devuelve `49 04` + 8 bytes sin el conteo. Formato inconsistente → algunas
  apps no parsean la calibración.
- **Corrección sugerida:** añadir el byte `0x01` (1 calibración) tras `49 04`.
- **Corrección aplicada (2026-08-17):** CALID con byte de conteo y cadenas terminadas
  en nulo (ISO 15031-5); nombre ECU alineado a `TCM-Engine Control` (23 bytes como el
  Onix real).

### BUG-09 — P3 · BAJO · Codificación de IDs extendidos (29 bits) en TX del driver

- **Ubicación:** `src/mcp2515.cpp:296-301` (rama `extended` de `sendMessage`).
- **Problema:** los bits EID14-EID13 (bits 17-16 del ID) se escriben en `SIDL[1:0]`
  en vez de `SIDL[3:2]` (`((f.id >> 16) & 0x03)`). La decodificación RX
  (`mcp2515.cpp:347-351`) espera `SIDL[3:2]`, así que las tramas extendidas no hacen
  round-trip. El emulador solo usa IDs de 11 bits, por eso no se manifiesta hoy, pero es
  un defecto del driver.
- **Corrección sugerida:** `((f.id >> 14) & 0x03) << 2`.

### BUG-10 — P2 · MEDIO · Carrera SPI durante el autotest del menú (opción 8)

- **Ubicación:** `src/main.cpp:39-58` y `218-234`; `test/autotest.cpp:110-124, 179-511`.
- **Problema:** `g_canPaused` solo se revisa al inicio del bucle del hilo CAN. Si el hilo
  está dentro de `handleCanRequest`/`sendIsoTp` (bloqueado hasta 300 ms esperando FC),
  el autotest arranca y hace transacciones SPI **crudas** (`bcm2835_spi_transfern` sin
  tomar el mutex `spiMtx` que protege el driver) de forma concurrente → transacciones
  corruptas. Además, `autotestBus` cierra `bcm2835` (`.end()`) mientras el emulador
  sigue en marcha y luego `can.begin()` lo reinicializa.
- **Corrección sugerida:** coordinar con un mutex/cv compartido que garantice que el
  hilo CAN terminó antes de entrar al autotest, y que el autotest use las mismas
  primitivas con mutex (o que `begin()/end()` de las pruebas no toquen `bcm2835_close`
  cuando `ownsBcm2835 == false`).

### BUG-11 — P2 · MEDIO · El hilo CAN puede hacer busy-spin si INT queda bajo sin datos

- **Ubicación:** `src/main.cpp:47-56`.
- **Problema:** si INT (GPIO25) queda bajo por una condición sin RXIF (p. ej. error de TX
  o overflow), `intAsserted` es `true` y el bucle no duerme → lectura SPI continua a
  máxima velocidad. Consume CPU y puede interferir con la simulación.
- **Corrección sugerida:** añadir un pequeño `bcm2835_delay(1)` también en el camino
  "INT pendiente" cuando `receiveMessage` no devuelve tramas.

### BUG-12 — P2 · MEDIO · Respuestas perdidas si `sendMessage` falla (sin reintento)

- **Ubicación:** `src/elm327.cpp:231-235` (el retorno de `sendIsoTp` se ignora) y
  `src/mcp2515.cpp:278-324` (solo TXB0, timeout 50 ms).
- **Problema:** si el bus está ocupado o TXB0 está lleno, la respuesta se descarta en
  silencio → el escáner ve un `NO DATA` intermitente.
- **Corrección sugerida:** reintentar una vez, o registrar/contabilizar las respuestas
  perdidas (métricas útiles).

### BUG-13 — ✅ RESUELTO · P2 · MEDIO · Cobertura incompleta de comandos AT (verificación empírica)

- **Ubicación:** `src/elm327.cpp:81-151` (`handleAt`).
- **Verificación (2026-08-14):** arnés local que probó **84 comandos** contra
  `ELM327::process()` con estado reseteado (`ATZ`) antes de cada uno. Resultado:
  **65 responden OK/correcto; 19 comandos AT estándar devuelven `?`**.
- **Comandos AT estándar ELM327 que responden `?` (19):**

  | Comando | Uso real | Observación |
  |---|---|---|
  | `ATFCSH` / `ATFCSM` / `ATFCSD` | cabeceras/datos/modo de Flow Control CAN | **los envían muchas apps en el init** |
  | `ATCSM0` / `ATCSM1` | modo silencioso CAN | usado antes de `ATMA` |
  | `ATMA` | monitorizar todo el tráfico CAN | usado por apps "avanzadas" |
  | `ATCEA0` / `ATCEA1` | direccionamiento extendido CAN | 29 bits |
  | `ATAL0` | desactivar tramas largas | solo se acepta `ATAL` pelado |
  | `ATBD0` / `ATBD1` | divisor de baudios | solo se acepta `ATBD` pelado |
  | `ATBRT38` | fijar baudios | solo se acepta `ATBRT` pelado |
  | `ATWM0D` | mensaje de wake-up | solo se acepta `ATWM` pelado |
  | `ATIFR0` / `ATIFR1` | tiempo entre tramas | no implementado |
  | `ATDM1` | mostrar DTCs en texto | no implementado |
  | `ATKW` | palabras clave | no implementado |
  | `ATMT` | monitorizar por tiempo | no implementado |
  | `ATPPS` | parámetro programable | no implementado |

- **Silencioso (peor que `?`):** `ATSH18DAF1` y `ATCRA1E8` (IDs de 29 bits) devuelven
  `OK` pero **no aplican el valor** (`elm327.cpp:126-138` solo aceptan 3 dígitos hex);
  la app cree que configuró la cabecera y no fue así.
- **Corrección sugerida:** aceptar el prefijo `ATFCS*` respondiendo `OK`; aceptar
  `ATCSM0/1`, `ATMA`, `ATCEA0/1`, `ATAL0`, `ATDM1` como compatibles; permitir valores en
  `ATBD`, `ATBRT`, `ATWM`; aceptar IDs de 11 y 29 bits en `ATSH`/`ATCRA` (y aplicarlos).
- **Corrección aplicada (2026-08-17):** los 19 comandos que devolvían `?` ahora responden
  `OK` (lista de compatibilidad) o `NO DATA` (`ATMA`, intencional); `ATFCSH` aplica la
  cabecera (3 o 6 dígitos) y `ATSH`/`ATCRA` aceptan IDs de 3 o 6 dígitos. Además se
  añadió `ATMM`/`ATMM0`/`ATMM1` (modo de máscara real/full).

### BUG-14 — P3 · BAJO · Detalles de comportamiento ELM327 poco realistas

- **Ubicación:** `src/elm327.cpp:43-53`, `107-123`.
- **Detalles:** el eco está apagado por defecto (un ELM327 real lo trae encendido, `ATE1`);
  `ATDPN`/`ATDP` reportan protocolo `AUTO` por defecto aunque físicamente solo funcione
  SP6. No rompen el escenario, pero un escáner "peleón" los notará.
- **Corrección sugerida:** dejar `echo = true` por defecto y hacer que `ATDP` refleje el
  protocolo efectivo.

### BUG-15 — P3 · BAJO · Archivos muertos y directorios vacíos trackeados

- **Ubicación:** repositorio.
- **Detalles:** `include/mcp2515/mcp2515.h`, `src/mcp2515/mcp2515.cpp`,
  `scripts/install_dependencies.sh` (los tres de 0 bytes, trackeados) y
  `docs/SKILL.md` (vacío, duplicado de `docs/SKILLS.md`). `include/oled/`, `src/oled/`
  están vacíos.
- **Corrección sugerida:** `git rm` de los archivos vacíos y eliminar las carpetas vacías.

### BUG-16 — P3 · BAJO · El estado del vehículo no persiste entre reinicios

- **Ubicación:** `src/vehicle.cpp:23-68` (estado inicial fijo).
- **Detalle:** el odómetro (`distancia`) siempre arranca en `12345.6` km y el nivel de
  combustible en `70%`. Para un emulador de prueba es aceptable, pero no es realista si
  se usa como demo continuada.
- **Corrección sugerida:** persistir `distancia`/`nivel_combustible`/`tiempo_motor` en un
  archivo (ver sugerencias).

---

## 4. Sugerencias de nuevas implementaciones

Priorizadas por impacto en la experiencia con un escáner real:

1. **Soporte multi-PID completo** (máximo 6 PIDs por petición, `elm327.cpp:224`). ✅ implementado.
2. **Respuestas multi-frame robustas**: aceptar FC de cualquier ID y respetar `BS`/`STmin`
   del FC. ✅ FC de cualquier ID implementado (falta respetar `BS`/`STmin` del FC).
3. **Inyección de DTCs configurables** (modos 03/07/0A): menú para encender/apagar el MIL
   y fijar códigos (p. ej. `P0301`, `P0420`) con su estado. Muy útil para probar escáneres.
4. **Modo 06 (monitores OBD en servicio)**: devolver valores plausibles de catalizador,
   O2, EVAP y misfire para que apps de "readiness" pasen. ✅ implementado.
5. **Modo 08 (control de componentes)**: al menos responder negativa correcta
   (`7F 08 12` o similar) en vez de `NO DATA`, y opcionalmente activar relés (bomba de
   combustible) en el menú. ✅ negativa implementada; activar relés pendiente.
6. **PID `0x1C`/`0x44` y los grupos `0x20/0x40/0x60`** con máscaras coherentes para que
   apps modernas ("Dashboard") no marquen errores. ✅ implementado (PIDs + máscaras
   real/full con `ATMM0/1`).
7. **Direccionamiento físico**: respuesta desde `0x7E9` a peticiones a `0x7E0`; monitorizar
   `ATSH`/`ATCRA` para peticiones externas. ✅ implementado.
8. **Ralentí controlado y ciclos O2 más reales**: oscilación de la sonda con cruces lentos
   (simula mezcla rica/pobre) y control de flujo de combustible.
9. **Persistencia de estado**: guardar odómetro/nivel de combustible en un fichero
   (`/var/lib` o `~/`) para que sobrevivan reinicios.
10. **Consola de inyección CAN**: enviar tramas CAN crudas desde el menú (para debug).
11. **Logging**: registro de peticiones/respuestas con timestamp y métricas de
    respuestas perdidas (ver BUG-12).
12. **Soporte de IDs 29 bits** en `ATSH`/`ATCRA` y en el driver (ver BUG-09), con un PID
    de prueba en la consola.
13. **Configuración por archivo** (`config.ini`): perfiles, VIN, calibración, estado de
    arranque (motor encendido/apagado), cristal del MCP2515.
14. **Velocidad de respuesta (P2)**: introducir un pequeño retardo (50-100 ms) antes de
    responder para parecer una ECU real y no saturar el bus.

---

## 5. CHECKLIST DE CORRECCIÓN (completar en orden de prioridad)

> **REGLAS (obligatorias para quien lea este reporte):**
> 1. Complete los ítems **en orden de prioridad**: P0 → P1 → P2 → P3. No pase a un ítem
>    de menor prioridad dejando uno de mayor prioridad abierto sin justificación escrita.
> 2. Al completar un ítem: marque `[x]`, anote **fecha + autor** junto a la casilla y
>    registre el cambio en la tabla de la sección 6.
> 3. Tras cada corrección: recompile **solo en la Pi** (`git pull && make -j4`, ver
>    `docs/WORKFLOW.md`), ejecute `make test` y pruebe con un escáner OBD2 real (o la
>    consola del menú, opción 6).
> 4. No borre ítems; si necesita contexto extra, añádalo como comentario debajo del ítem.
> 5. Al terminar el checklist, actualice `README.md` y `docs/` si procede, y publique la
>    nueva versión siguiendo el flujo de `docs/LEARNINGS.md` (tag = `VERSION`, ciclo 0-9).

### P0 — Crítico (debe resolverse primero)

- [x] **BUG-01** — Arreglar la espera de Flow Control en `sendIsoTp` (aceptar FC de
      cualquier ID; no descartar tramas no-FC → `pending` + `drainPending()`).
      Hecho: __2026-08-17__
- [ ] **Validación P0** — Con un escáner (o adaptador CAN + `cansend`/`candump`),
      `0902` debe entregar el VIN completo. Fecha/autor: ______

### P1 — Alto (en orden)

- [x] **BUG-02** — Soportar peticiones multi-PID (2-6 PIDs) en `handleCanRequest`.
      Hecho: __2026-08-17__
- [x] **BUG-03** — Corregir máscara modo 01 PID `00` a `0xFE 0xDC 0x05 0x48`
      (incluye fuel trims `06/07`). Hecho: __2026-08-14__
- [x] **BUG-04** — Responder a PIDs `20/40/60` (máscaras extendidas) y añadir
      `0x2E` (purga EVAP) + modo `0x08`. Hecho: __2026-08-14__
- [x] **BUG-05** — Modo 09 PID `00`: byte de conteo (3) + máscara SAE J1979
      `50 40 00 00` (PIDs 02/04/0A). Hecho: __2026-08-17__

### P2 — Medio (en orden)

- [ ] **BUG-10** — Eliminar la carrera SPI del autotest (sincronizar hilo CAN; que el
      autotest respete `spiMtx`/no cierre bcm2835 en modo embebido). Fecha/autor: ______
- [x] **BUG-07** — Responder desde `0x7E9` a peticiones físicas `0x7E0`. Hecho: __2026-08-17__
- [ ] **BUG-12** — Reintento y contador de respuestas perdidas. Fecha/autor: ______
- [x] **BUG-08** — CALID (modo 09, PID `04`) con byte de conteo y cadenas
      terminadas en nulo (ISO 15031-5). Hecho: __2026-08-17__
- [ ] **BUG-11** — Evitar busy-spin del hilo CAN cuando INT queda bajo sin datos.
      Fecha/autor: ______
- [x] **BUG-06** — Modo 06 con valores plausibles y formato ISO 15031-5:2006
      (TestValue/MinLimit/MaxLimit/Unit/TestID/OTI), TID `00` con máscara de
      4 bytes `C0 00 00 00` (TIDs 01/02); modo 08 ya responde (negativa).
      Hecho: __2026-08-17__
- [x] **BUG-13** — Completar cobertura AT (los 19 comandos ya responden `OK`/`NO DATA`;
      `ATFCSH/M/SD` aplican cabeceras; `ATSH/ATCRA` aceptan 3 o 6 dígitos).
      Hecho: __2026-08-17__

### P3 — Bajo

- [ ] **BUG-09** — Corregir encoding de IDs extendidos en `sendMessage`. Fecha/autor: ______
- [ ] **BUG-14** — `echo = true` por defecto y `ATDP` con protocolo efectivo.
      Fecha/autor: ______
- [ ] **BUG-15** — Eliminar archivos vacíos/directorios muertos del repo. Fecha/autor: ______
- [ ] **BUG-16** — Persistir odómetro/nivel de combustible. Fecha/autor: ______

### Sugerencias (opcional, tras el checklist)

- [ ] **SUG-03** — Inyección de DTCs + MIL configurables. Fecha/autor: ______
- [ ] **SUG-10/11** — Consola de tramas CAN crudas y logging de peticiones/respuestas.
      Fecha/autor: ______
- [ ] **SUG-13** — Archivo de configuración (`config.ini`). Fecha/autor: ______

---

## 6. Registro de cumplimiento (checklists completados)

> Cada persona que trabaje este reporte debe **registrar aquí qué checklists cumplió**.
> Al marcar el último ítem de una prioridad, indíquelo como "completado".

| Fecha | Autor | ítems completados (IDs / prioridad) | Pruebas realizadas (make test, escáner, ...) | Notas |
|-------|-------|-------------------------------------|-----------------------------------------------|-------|
| 2026-08-14 | opencode | BUG-03, BUG-04 (P1) | Harness de PID (`0100/06/07/0F/2E/20/40/60`, modo 08) con mock bcm2835 | Máscaras verificadas bit a bit (SAE J1979); calibración fija `0106=7A`, `0107=83`, `012E=CC` |
| 2026-08-17 | codebuff | BUG-01, BUG-02, BUG-05, BUG-06, BUG-07, BUG-08, BUG-13 | Verificación estática del código (`sendIsoTp`, `handleCanRequest`, `getMode01/06/09`, `handleAt`) + traza del Onix real | Multi-frame con FC de cualquier ID; multi-PID 2-6; modo 09/06 corregidos; `0x7E9` físico; AT completo + `ATMM0/1` |

---

## 7. Anexo — Verificación rápida del escenario OBD2 (manual)

Sin escáner físico se puede validar con la consola del emulador (menú opción 6) y con
SocketCAN:

```bash
# Opción 6 (consola): debe responder
ATZ            -> ELM327 v1.5
ATSP6          -> OK
0100           -> 41 00 FE DC 05 48  (fuel trims 06/07 anunciados)
0106 / 0107    -> 41 06/07 <trim>     (STFT/LTFT, calibración)
010F           -> 41 0F <IAT>
012E           -> 41 2E <purga EVAP>
010C           -> 41 0C <rpm>
0902           -> 49 02 01 39 42 47 4B ...

# Con dos interfaces SocketCAN (can_utils), simular un escáner:
sudo ip link set can0 up type can bitrate 500000
cansend can0 7DF#02010C00         # petición 010C
candump can0                      # debe verse la respuesta 7E8
cansend can0 7DF#0309020000       # multi-PID 09 02 (VIN) - falla hoy (BUG-01)
```

Criterio de aceptación del P0/P1: `candump` debe mostrar la respuesta `7E8` para
peticiones single y multi-PID, y el VIN completo (tramas `10`/`30`/`21`..) para `0902`.
