# TODO - Pasos siguientes para manejo de respuestas OBD2

## Regla de uso

Este archivo funciona como **checklist viva**.
Cada item tiene una casilla `- [ ]` o `- [x]`.
Cuando un item se cumple, **tildarlo** con `- [x]`.
Si un item necesita sub-items, cada sub-item también se tilda individualmente.
No borrar items completados; conservarlos como historial.

---

## 1. Parsing robusto de respuestas OBD2

- [x] Leer `docs/ANALISIS_MENU_SCANNER_AUTEL.md`
- [x] Definir estructura `OBD2Response` / `ECUInfo` en `include/scanner/obd2.hpp`
  - [x] Campos: `mode`, `pid`, `raw`, `parsedValue`, `unit`, `status`
- [x] Implementar parser genérico `decodePID` en `src/scanner/obd2.cpp`
  - [x] Validar modo y PID esperados
  - [ ] Manejar respuestas multipaquete completas (>8 bytes)
  - [ ] Detectar NEGATIVE RESPONSE (0x7F)
- [x] Añadir `decodePID(uint8_t pid, const uint8_t* data, size_t len) -> PIDData`
  - [x] MAP (0x0B)
  - [x] MAF (0x10)
  - [x] MAF voltage (0x66)
  - [x] BARO (0x33)
  - [x] TPS1 (0x11)
  - [x] TPS2 (0x45)
  - [x] IAT (0x0F)
  - [x] ECT (0x05)
  - [x] Oil temp (0x5C)
  - [x] Fuel level (0x2F)
  - [x] APP1 / APP2 (0x49 / 0x4A)
  - [x] STFT / LFT (0x06 / 0x07)
  - [x] Ignition timing (0x0E)
  - [ ] Misfire counts (0x56-0x59)
  - [ ] Injector circuit tests (0x1C-0x1F)
  - [ ] HO2S heaters (0x1D-0x1E)
  - [ ] A/C clutch relay (0x24-0x26)

## 2. Implementar Freeze Frame completo

- [x] Completar `requestFreezeFrame` en `src/scanner/obd2.cpp`
  - [x] Enviar Mode 02 con PID 01/02
  - [x] Parsear DTC almacenado
  - [x] Parsear parámetros básicos del freeze frame (IAT, MAF, MAP, fuel level)
  - [ ] Parsear parámetros específicos GM (purge cmd, heating cycles, etc.)
  - [x] Retornar `std::unordered_map<std::string, PIDData>`
- [ ] Integrar Freeze Frame en UI (`src/scanner/display.cpp` / `menu.cpp`)

## 3. Custom List (Listado Personalizado)

- [x] Añadir método `requestCustomList(const std::vector<uint8_t>& pids)` en `obd2.hpp`
- [x] Implementar en `obd2.cpp`
  - [x] Iterar lista de PIDs
  - [ ] Paralelizar requests si la ECU lo permite
  - [ ] Agrupar resultados por bloques según `ANALISIS_MENU_SCANNER_AUTEL.md`
- [ ] Integrar Custom List en menú (`src/scanner/menu.cpp`)

## 4. Active Tests - Implementación real

- [ ] Revisar placeholders en `src/scanner/active_test.cpp`
  - [ ] `testEVAPValve(bool activate)`
  - [ ] `testFuelPumpRelay(bool activate)`
  - [ ] `testFanRelay(bool activate)`
  - [ ] `testACClutch(bool activate)`
  - [ ] `testThrottleBody(bool activate)`
  - [ ] `testInjectors(bool activate)`
- [ ] Mapear cada test a PIDs OBD2 reales del Chevrolet Onix
- [ ] Añadir confirmación visual en display

## 5. ECU Information completa

- [x] Implementar `requestECUInfo()` en `obd2.hpp`
  - [x] VIN (Mode 09 PID 02) - parcialmente hecho
  - [x] Calibration IDs
  - [ ] Serial number
  - [ ] Odómetro
  - [ ] Software version
- [ ] Mostrar ECU Info en `display.cpp`

## 6. Service / Mantenimiento

- [x] Añadir `resetAdaptations()` (reinicio compensaciones)
- [x] Añadir `resetFuelTrim()` 
- [x] Añadir `programFuelComposition(uint8_t ethanolPercent)`
- [x] Añadir `resetImmobilizer()`

## 7. Parsing de DTCs mejorado

- [ ] Decodificar DTCs completos (P, C, B, U)
- [ ] Añadir descripción textual si es posible
- [ ] Soportar Mode 07 (pending DTCs)
- [ ] Soportar Mode 0A (permanent DTCs)

## 8. Validación y pruebas

- [x] Probar compilación en Raspberry Pi 32-bit y 64-bit
- [x] Suprimir warnings de parámetros sin usar con `[[maybe_unused]]`
- [ ] Validar parsing contra valores conocidos del Chevrolet Onix
- [ ] Añadir modo simulación para pruebas sin hardware
- [ ] Validar manejo de timeouts y errores CAN

---

*Documento generado a partir del análisis de `docs/ANALISIS_MENU_SCANNER_AUTEL.md`.*
*Fecha: 2026-08-15.*
