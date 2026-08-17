# SKILLS.md — Conocimiento y reglas del proyecto

Compendio de todo lo aprendido en `docs/` (LEARNINGS.md, WORKFLOW.md), README.md,
prompt.md y la estructura del código. Útil como referencia rápida para trabajar
en este repositorio.

> **Estado conocido del código:** antes de tocar código, consulte
> [docs/BUG_REPORT.md](BUG_REPORT.md) — auditoría de bugs (16 hallazgos, P0-P3),
> foco en el escenario "escáner OBD2 conectado", sugerencias y checklist de
> corrección pendiente. El índice de docs está en [docs/README.md](README.md).
>
> **Qué debe responder la ECU:** [docs/ECU_PARAMETERS.md](ECU_PARAMETERS.md) es
> el catálogo de TODAS las solicitudes del escáner (PIDs OBD2 y DIDs GM modo 22)
> que la ECU emulada debe responder con valores. Consultarlo antes de ampliar
> `src/elm327.cpp`. El modo 22 UDS ya responde: odómetro `B100`, torque `01A9`,
> temp. catalizador `01B4`, presión combustible `1180`, voltaje `01A1`, oil life
> `119F` e inyectores `1193-119A`; el resto de DIDs responde NRC `7F 22 ... 31`.

---

## 1. Qué es este proyecto

Emulador de la ECU de un **Chevrolet Prisma** para Raspberry Pi. Conectado a un
módulo **MCP2515** (controlador CAN + transceptor, p. ej. MCP2515+TJA1050) por el
bus **SPI0** usando la librería **bcm2835**. Responde a comandos **AT estándar
ELM327** y peticiones **OBD2** (ISO 15765-4, CAN 11-bit, 500 kbps → protocolo
ELM327 `SP6`) generando datos dinámicos (RPM, velocidad, temperatura, marcha, etc.).

Componentes:
- `include/mcp2515.h` + `src/mcp2515.cpp` — driver MCP2515 (SPI0/CE0, GPIO25 INT, bit timing).
- `include/vehicle.h` + `src/vehicle.cpp` — modelo del vehículo + simulador por perfiles + consola.
- `include/elm327.h` + `src/elm327.cpp` — emulador ELM327/OBD2 (AT + PIDs + ISO-TP).
- `src/main.cpp` — hilos (CAN + simulación a 10 Hz) y menú interactivo.
- `test/` — pruebas de comunicación (SPI, loopback, bus) compartidas en `test/autotest.{h,cpp}`.
- `scripts/` — `run_tests.sh`, `can_kernel_test.sh`, `install_dependencies.sh`.

El proyecto usa **C++11**, `-Wall -Wextra -O2`, enlaza `-lbcm2835 -lpthread`.

---

## 2. Workflow de desarrollo (remoto / local)

Regla de oro: **cambios solo en local, compilar y probar solo en la Pi**.

1. **Editar local** los archivos en `$PWD` (nada se edita directo en la máquina remota).
2. **Commit local** con mensajes semánticos (`feat:`, `fix:`, `docs:`, `chore:`, `refactor:`, `test:`).
3. **Push** a `origin/main`.
4. **Actualizar remoto** en la Pi: `git pull`.
5. **Compilar y probar remoto** en la Pi (no compilar en local).

Datos de conexión (de `prompt.md`):
- `USER="pi"`, `HOSTNAME="raspi.local"`, `LINK="/home/pi/src/mcp2515_emulator_rpi"`
- La contraseña de la Pi va en `$SSHPASS` (nunca mostrarla en pantalla).
- Ejemplo de comando:
  ```bash
  sshpass -e ssh -o StrictHostKeyChecking=no $USER@$HOSTNAME \
    "cd $LINK && git pull --ff-only && make -j4 && make run"
  ```

Recordatorios:
- No compilar ni probar en la máquina local.
- En la Pi solo `git pull` y build; no editar archivos dentro del repo remoto.
- No dejar archivos generados en el remoto.

---

## 3. Git / Versionado

### Reglas obligatorias

- **Todo push debe llevar su tag.** No se pushea sin tag.
- **Tag = VERSION:** el tag lleva `v` (`v1.0.5`) y el archivo `VERSION` (raíz)
  lleva el mismo número sin `v` (`1.0.5`). Siempre deben coincidir.
- **Siguiente número:** último tag + 1 en el último segmento, respetando el
  **ciclo patch 0-9**: no se pasa de `v1.0.9` a `v1.1.1`; debe ir a `v1.1.0`.
  Cada minor tiene exactamente 10 patches (0 a 9).
- **Cada commit significativo debe tener su tag.** No se salta ningún número.
- **No eliminar tags publicados** y **no retroceder de versión.** Si hay error,
  se crea un nuevo tag con el siguiente número de la secuencia.
- **Commit messages con conventional commits** (`feat:`, `fix:`, `docs:`, `chore:`, `refactor:`, `test:`).
- El versionado empezó en `1.0.0` (`v1.0.0`).

### Cómo hacer un bump

1. Último tag: `git tag --sort=-version:refname | head -1`.
2. Verificar que `VERSION` coincida con ese tag (sin `v`).
3. Calcular el siguiente según el ciclo patch 0-9.
4. Actualizar `VERSION`, commitear, taggear y pushear:
   ```bash
   echo "1.1.0" > VERSION
   git add VERSION && git commit -m "chore: bump version to 1.1.0"
   git tag v1.1.0
   git push origin main && git push origin v1.1.0
   ```

### Push con token (403 Permission denied)

Causa: el credential helper guarda credenciales sin permisos de escritura.
Solución: configurar temporalmente el remote con el token y **limpiar después**.

```bash
# 1. Remote temporal con token
git remote set-url origin https://USUARIO:<TOKEN>@github.com/USUARIO/REPOSITORIO.git
# 2. Push
git push origin main --tags
# 3. Limpiar (nunca dejar el token en la URL)
git remote set-url origin https://github.com/USUARIO/REPOSITORIO.git
```

### Dónde viven las credenciales

- `~/.git-credentials` — formato `https://USUARIO:TOKEN@github.com` (chmod 600).
- `~/.gitconfig` — `user.name`, `user.email`, `credential.helper`.
- Token también en config global: `git config --global --list | grep -i "user.password"`.
- macOS Keychain: `security find-internet-password -s "github.com" -a "USUARIO"`.

Verificación sin exponer el token:
```bash
ls -la ~/.git-credentials
git config --global credential.helper     # esperado: store
git ls-remote origin 2>&1 | head -3
git remote -v
```

### Errores comunes de Git

- **403 Permission denied** → token sin permisos de push; usar token válido.
- **SSH Permission denied** → falta `~/.ssh/id_ed25519.pub` en GitHub → Settings → SSH Keys.

---

## 4. Compilación, ejecución y pruebas

```bash
make                     # compila -> ./bin/emulator_prisma_32 (o _64 según uname -m)
make install-bcm2835     # instala libbcm2835 (sudo) si no está
sudo make run            # ejecuta (requiere root por /dev/mem)

make test-build          # compila obj/test_spi|test_loopback|test_bus
make test                # compila y ejecuta las 3 pruebas con sudo
make test-socketcan      # prueba alternativa con SocketCAN (kernel mcp251x)
make clean               # rm -rf obj bin
```

- `APP_VERSION` se inyecta en el build desde `VERSION` con `-DAPP_VERSION`.
- Si falta libbcm2835: `make install-bcm2835` (descarga bcm2835-1.68 de airspayce.com).
- Compilación cruzada: `make CXX=arm-linux-gnueabihf-g++`.
- En la Pi se compila con `g++` nativo, siempre vía SSH (ver sección 2).

### Runner de pruebas

`sudo ./scripts/run_tests.sh` (flags: `--spi`, `--loopback`, `--bus`,
`--socketcan`, `--build`). Salida: 0 = OK, 1 = fallos, 2 = uso incorrecto.

---

## 5. Hardware: cableado MCP2515 → Raspberry Pi

| Señal Pi (BCM) | Pin físico (header 40 pines) | Pin MCP2515 |
|----------------|------------------------------|-------------|
| MISO (GPIO9)  | 21 | SO |
| MOSI (GPIO10) | 19 | SI |
| SCLK (GPIO11) | 23 | SCK |
| CE0 (GPIO8)   | 24 | CS |
| GPIO25        | 22 | INT |
| 5V o 3V3      | 2/4 o 1 | VCC |
| GND           | 6, 9, 14, 20, 25 | GND |

**Advertencias de hardware:**
- Niveles de voltaje: SO sale a 5 V hacia un GPIO de 3,3 V. Módulos comerciales
  suelen tolerarlo, pero por seguridad se recomienda divisor o módulo con
  conversión de niveles en MISO.
- Cristal: registros de bit timing dependen del cristal (8 o 16 MHz). El código
  asume **8 MHz** (`MCP2515_OSC_HZ` en `include/mcp2515.h`); si el módulo es de
  16 MHz cambiar a `16000000UL`. El driver trae la tabla para ambos a 500 kbps.
- Pull-up de INT en Pi 4: `bcm2835_gpio_set_pud()` puede no aplicar en Pi 4.
  Si INT flota, añadir 10 kΩ de GPIO25 a 3,3 V (el emulador funciona igual
  porque sondea por SPI).

---

## 6. Comandos AT (ELM327) y PIDs OBD2 soportados

- **AT**: `ATZ`→`ELM327 v1.5`; `ATI/AT@1/AT@2/AT@3`; `ATE0/1`, `ATL0/1`, `ATH0/1`,
  `ATS0/1`, `ATSP0/ATSPn` (físicamente solo funciona el 6), `ATDP/ATDPN`→`A6`,
  `ATRV`, `ATSHxxxx`, `ATCRAxxxx`, `ATD`, `ATMM` (consulta el modo de máscara:
  `REAL`/`FULL`), `ATMM0` (máscara idéntica al Onix real) / `ATMM1`
  (superconjunto con todo lo implementado); por compatibilidad: `ATR0/1`,
  `ATST`, `ATAL`, `ATAR`, `ATBI`, `ATBD`, `ATCAF`, `ATCFC`, `ATCM`, `ATCEA`,
  `ATTP`, `ATIGN`, `ATWM`, `ATAT`. Desconocido → `?`.
- **Modos**: `01` (datos), `02` (freeze frame: `02 02` = DTC que lo provocó +
  máscara de PIDs con datos; el resto responde datos actuales con `42`),
  `03/07/0A` (sin DTCs), `04` (borrar DTCs), `06` (monitores OBD en servicio,
  formato ISO 15031-5:2006: 46 <TID> <TestValue:2> <MinLimit:2> <MaxLimit:2>
  <Unit:1> <TestID:1> <OTI:2>; TID `00` → máscara de 4 bytes `C0 00 00 00` =
  TIDs 01/02), `08` (control de sistemas: `48 <TID> <Data A..E>` — EVAP `01`,
  EVAP purge/vent `02`, fan relay `03`, fuel pump relay `04`, A/C clutch `05`;
  TID no soportado → NRC `7F 08 <TID> 12`; el Onix real responde NO DATA,
  aquí es superconjunto para el ActiveTest del AUTEL), `09` (VIN/calibración/
  nombre ECU, ISO-TP multi-frame; `0900` → `49 00 03 50 40 00 00`; CALID con
  cadenas terminadas en nulo; nombre ECU de 20 caracteres con relleno).
- **PIDs modo 01**: 00/20/40/60 (máscaras de soportados, SAE J1979, bit 7 =
  PID más bajo. **Modo real** (por defecto, según traza del Onix):
  `0100→41 00 BE 3F B8 13` (01,03,04,05,06,07, 0B-10, 11, 13, 14, 15, 1C,
  1F, 20), `0140→41 40 FE D2 80 00` (41,42,43,44,45,46,47, 49,4A,4C,4F, 51),
  `0120→41 20 80 06 80 00`, `0160→41 60 00 00 00 00`. **Modo full** (`ATMM1`):
  `0100→BF FF BF D2`, `0140→5E 94 67 90` — anuncia todo lo implementado),
  01, 03, 04, 05,
  **06 (STFT1), 07 (LTFT1)** (A = 128 + %·128/100), 0B, 0C (RPM = A/4, 2 bytes),
  0D (km/h), 0F (IAT = A-40), 10, 11, 13, 1C (0x06 ISO 15765-4), 1F, 21,
  **2E (purga EVAP = %·255/100)**, 2F, 31, 42 (batería), 45, 46, 49, 4C,
  **4E (marcha, PID personalizado: 0=N, 1-5=gears, 6=R)**, 5C.
  PIDs modo 01 añadidos: 08/09 (STFT/LTFT B2), 0A (presión combustible),
  14-1A (sensores O2, mapeo SAE), 44 (λ), 47 (throttle absoluta B), 52 (E85),
  53 (presión de tanque), **56-59 (misfire por cilindro: nibble por cilindro,
  cil 1 = nibble alto de A; 56/58 = actual, 57/59 = histórico)**. Ver el
  checklist en `docs/ECU_PARAMETERS.md`.
- **Modo 22 UDS (DIDs GM)**: implementado (`getMode22()` en `elm327.cpp`,
  despachado en `handleCanRequest()` y la consola). Responde `62 <DID> <datos>`
  para odómetro `B100`, torque `01A9`, temp. catalizador `01B4`, presión
  combustible `1180`, voltaje `01A1`, oil life `119F`, inyectores `1193-119A`,
  tiempo desde arranque `11A1`, knock retard `11A6`, baro `1251`/`119D`,
  balance rate `162F-1636`, temp. ATF `1940`, torque alt `19DE` (ft-lbs),
  AFR `119E`, sincronización de inyección `1564` (0x29) y estados `1201`/
  `2345` (0). Las fórmulas CANSF siguen el MTH de ScanGauge (verificado:
  valor = raw×A/B + C, C en complemento a 2 — ej. `00010001FFD8` = raw−40):
  OLF % = raw×200/51, KR ° = raw×45/50, BAR inHg = raw×3, BR mm³ =
  raw×5/32−20, PW ms = raw×200/131, ET s = raw — **confirmadas** (2026-08).
  Confirmados con traza del Onix real (`docs/SCANNER_TRACE_ONIX.md`):
  `1940` TFT = 1 byte, raw = °C+40 (`62 19 40 23` → −5 °C) y `11A1`
  (2 bytes = segundos). Pendientes: AFR `119E` (MTH ×1 vs ×0.01) y
  TRQ `19DE` (ft-lbs = raw×5 vs ft-lbs directo). DID no soportado →
  NRC `7F 22 <DID> 31`. Catálogo completo en `docs/ECU_PARAMETERS.md`.
- **TCM (segunda ECU del bus)**: peticiones físicas a `0x7E1` → respuesta desde
  `0x7E9`, y a `0x7E2` → `0x7EA` (donde AUTEL/ScanGauge leen la TFT `22 19 40`).
  `getTcmMode22()` responde los DIDs de transmisión: `1940` TFT (confirmado),
  `11E0` ISS, `11E1` OSS, `11E2` TCC slip, `11E3` gear ratio (raw16×0.01),
  `11E4` marcha, `11E5-11E7` shift times 1-2/2-3/3-4 (raw16 = ms), `11E8` last
  shift, `11E9-11EB` shift errors (DIDs candidatos, por confirmar). La TCM solo
  responde el servicio 22. En la consola: `ATSH 7E2` y luego `22 19 40`.
- **Direccionamiento ISO 15765-4**: `0x7E0` (ECM físico) → `0x7E8`; `0x7DF`
  (funcional) → `0x7E8`; `0x7E1`/`0x7E2` (TCM) → `0x7E9`/`0x7EA` (BUG-07
  corregido a la norma: antes la ECM respondía 0x7E9 a 0x7E0).
- **Modo 08 (control de sistemas)**: `getMode08()` — responde `48 <TID>
  <Data A..E>` (prueba completada, sin falla) para EVAP `01`, EVAP purge/vent
  `02`, fan relay `03`, fuel pump relay `04` y A/C clutch `05` (los tres
  últimos extensión del emulador); TID desconocido → NRC `7F 08 <TID> 12`
  (subFunctionNotSupported). Despachado en `handleCanRequest()` y la consola
  (`08 01` → `48 01 00 00 00 00 00`).
- **Servicios UDS 19/14/31**: `19 02 <máscara>` (historial DTCs →
  `59 02 01 FF <n> <DTC+estado>...`, multi-frame si n > 2; estado 0x09
  confirmado+activo, 0x04 pendiente), `14 FF FF FF` (borrar historial → `54`,
  limpia códigos/MIL/calentamientos/distancia) y `31 01 C1 0F` (reset de
  adaptativos → `71 01 C1 0F`, pone `ltft1`/`ltft2` en 0). Despachados en
  `handleCanRequest()` y la consola.
- **Modo 09**: `00` soportados, `02` VIN `9BGKL48T0HB130763`, `04` calibración
  `1505708/52124404`, `0A` nombre ECU `TCM-Engine Control` (el texto que
  responde el Onix real según la traza).
- Tráfico: peticiones en `0x7DF/0x7E0` (ECM) y `0x7E1/0x7E2` (TCM); respuestas
  desde `0x7E8` (ECM) y `0x7E9/0x7EA` (TCM).

---

## 7. Arquitectura / concurrencia

- **Hilo CAN:** sondea INT (GPIO25) y lee buffers RX del MCP2515; responde
  peticiones desde `0x7E8`.
- **Hilo de simulación (10 Hz):** actualiza parámetros según el perfil
  (Ralentí, Ciudad, Autopista, Deportivo). Parámetros en modo `FIJO` no se tocan.
- **Acceso a datos:** todos los accesos al modelo se serializan con un mutex
  (`Vehicle::mtx`); los métodos `value/setValue/isAuto/setAuto` NO bloquean.
- **SPI no es thread-safe:** el driver serializa el acceso con un mutex interno.
- Perfiles de conducción: `Profile::{Idle, City, Highway, Sport}`.

---

## 8. Solución de problemas

- **`ERROR: no se pudo inicializar el MCP2515`** → ejecutar con `sudo`; revisar
  CE0/CS, cableado, alimentación, cristal 8/16 MHz.
- **`libbcm2835 no está instalada`** → `make install-bcm2835`.
- **Escáner muestra `NO DATA`** → confirmar `ATSP6` (ISO 15765-4, CAN 11-bit,
  500 kbps), CANH/CANL bien conectados, terminación 120 Ω (2 nodos).
- **Sin ACK al transmitir** → modo one-shot evita reintentos infinitos; sin otro
  nodo en el bus las TX fallan (normal en banco solo).
- **`CANSTAT = 0xFF` / sin respuesta** → alimentación, cableado CE0→CS/MOSI→SI/
  SCLK→SCK, o otro dispositivo ocupando CE0.
- **Fallan escritura/lectura** → MISO/SO o MOSI/SI cruzados, o niveles de voltaje.
- **Solo falla comprobación de INT** → conexión GPIO25→INT (o pull-up Pi 4).
- **Falla loopback pero SPI pasa** → problema del MCP2515 (cristal, módulo) o bit timing.
- **TEC/REC/EFLG suben en bus de 2 nodos** → revisar terminación 120 Ω, CANH/CANL,
  cristal/bitrate coincidentes. En bus sano: TEC=REC=0 (error-active).

### Método alternativo: SocketCAN (kernel mcp251x)

**IMPORTANTE:** el driver de kernel y el emulador (bcm2835 directo) compiten por
el mismo módulo SPI/GPIO. Usar **uno a la vez** (si `can0` existe, el emulador no
funcionará sobre ese módulo).

```bash
sudo apt install -y can-utils iproute2
sudo ./scripts/can_kernel_test.sh --setup           # can0 (CE0, INT 25) — requiere reboot
sudo ./scripts/can_kernel_test.sh --setup --bus     # además can1 (CE1, INT 24)
sudo ./scripts/can_kernel_test.sh --osc=8000000     # si el cristal es 8 MHz
sudo reboot
```

```bash
sudo ip link set can0 up type can bitrate 500000
candump can0 &
cansend can0 123#DEADBEEF01020304
sudo ip link set can0 down
```

Equivalencias: `--loopback` ≈ `test_loopback`, `--bus` ≈ `test_bus`.

---

## 9. Menú de la app (binario)

```
[1] Iniciar simulación   [2] Detener simulación
[3] Perfil de conducción [4] Configurar parámetros
[5] Estado del vehículo  [6] Consola ELM327 / OBD2
[7] Información sistema  [8] Autotest de comunicación
[0] Salir
```
- **1/2:** motor encendido/apagado (datos dinámicos vs. enfriamiento, batería ~12,4 V).
- **4:** fijar parámetro (FIJO) o volver a simulado (AUTO).
- **6:** consola ELM327 sin escáner (p. ej. `ATZ`, `ATSP6`, `010C`, `0902`).
- **7:** CANSTAT, CANINTF, EFLG, TEC/REC.
- **8:** pausa el tráfico CAN, ejecuta las 3 pruebas, reinicializa el MCP2515 y reanuda.

---

## 10. Comandos útiles

```bash
# Variables de entorno
echo $HOME; echo $GIT_CONFIG_GLOBAL; echo $TMPDIR

# Estado git
git status; git log --oneline -5; git remote -v; git ls-remote origin

# Último tag publicado
git tag --sort=-version:refname | head -1
```

---

## 11. Ubicaciones de referencia

| Archivo | Ubicación | Contenido |
|---------|-----------|-----------|
| Credenciales git | `~/.git-credentials` | Token GitHub en URL |
| Config git global | `~/.gitconfig` | user, email, credential.helper |
| libbcm2835 | `/usr/local/lib/libbcm2835.a` + `/usr/local/include/bcm2835.h` | Dependencia de bajo nivel |
| Overlays del kernel | `/boot/config.txt` (copia de respaldo hecha por el script) | can0/can1 (SocketCAN) |
| Proyecto | `./` | Código fuente del emulador |
| VERSION | `./VERSION` | Número del último tag (sin `v`) |
