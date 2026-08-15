# Emulador OBD2 Chevrolet Prisma (ELM327 + MCP2515)

Emulador de la ECU de un Chevrolet Prisma para Raspberry Pi. Se conecta a un
módulo **MCP2515** (controlador CAN + transceptor, p. ej. el módulo
MCP2515+TJA1050) por el **bus SPI0** usando la librería **bcm2835**, y responde
a comandos **AT estándar ELM327** y peticiones **OBD2** generando datos
dinámicos (velocidad, RPM, temperatura, carga, presión de combustible, voltaje
de batería, marcha, etc.).

Puede conectarse a un escáner OBD2/ELM327 real (o a un software de
diagnóstico) para simular una ECU Chevrolet Prisma en ISO 15765-4
(CAN 11-bit, 500 kbps — protocolo ELM327 `SP6`).

---

## 1. Cableado (SPI0 → MCP2515)

| Señal Pi (BCM) | Pin físico (header 40 pines) | Pin del MCP2515 |
|----------------|------------------------------|-----------------|
| **MISO** (GPIO9)  | 21 | **SO**  |
| **MOSI** (GPIO10) | 19 | **SI**  |
| **SCLK** (GPIO11) | 23 | **SCK** |
| **CE0** (GPIO8)   | 24 | **CS**  |
| **GPIO25**        | 22 | **INT** |
| **5V** (o 3V3)    | 2/4 (o 1) | **VCC** |
| **GND**           | 6, 9, 14, 20, 25 | **GND** |

Además, los pines **CANH / CANL** del módulo van al bus CAN (p. ej. a los
pines 6 y 14 del conector OBD2, o a un segundo nodo CAN para probar).

> **OJO con los niveles de voltaje:** muchos módulos MCP2515 funcionan a 5 V.
> La línea **SO → MISO** sale del MCP2515 a nivel de 5 V y entra directo a un
> GPIO de 3,3 V de la Pi. La mayoría de módulos comerciales toleran esto
> (los GPIO de la Pi suelen aguantar 5 V en entrada), pero por seguridad
> eléctrica se recomienda un divisor de tensión o un módulo con
> conversión de niveles en MISO.

> **Cristal del módulo:** los registros de bit timing dependen de la
> frecuencia del cristal del módulo (8 MHz o 16 MHz). Por defecto el código
> asume **16 MHz** (`MCP2515_OSC_HZ` en `include/mcp2515.h`). Si su módulo usa
> **8 MHz**, compile con `make MCP2515_OSC_HZ=8000000` (o cambie la constante
> a `8000000UL`); el driver ya incluye la tabla para 8 y 16 MHz a 500 kbps
> (y otras velocidades). La detección automática del cristal está en la
> sección 9.

> **Pull-up de INT (Pi 4):** `bcm2835_gpio_set_pud()` funciona de forma fiable
> en Pi 1-3; en Pi 4 puede no aplicar el pull-up. Si el INT flota, el emulador
> sigue funcionando (sondea por SPI), pero si quiere la interrupción bien
> definida añada una resistencia de 10 kΩ de GPIO25 a 3,3 V.

---

## 2. Instalación de libbcm2835

```bash
sudo apt update
sudo apt install -y build-essential wget
make install-bcm2835        # descarga, compila e instala libbcm2835 en /usr/local
```

## 3. Compilar y ejecutar

```bash
make                 # compila -> ./prisma-obd-emulator
sudo make run        # ejecuta (requiere root por /dev/mem)
```

> En la Pi se compila con `g++` nativo. Para compilación cruzada:
> `make CXX=arm-linux-gnueabihf-g++` (con las cadenas instaladas).

---

## 4. Uso

Al arrancar aparece un menú interactivo:

```
 [1] Iniciar simulación       [2] Detener simulación
 [3] Perfil de conducción     [4] Configurar parámetros
 [5] Estado del vehículo      [6] Consola ELM327 / OBD2
 [7] Información del sistema  [8] Autotest de comunicación
 [9] Monitor en vivo (ECU)    [0] Salir
```

- **1 / 2:** inicia (motor encendido, datos dinámicos) o detiene (motor
  apagado: RPM y velocidad caen, temperaturas se enfrían, batería a ~12,4 V).
- **3:** perfiles de conducción: *Ralentí*, *Ciudad* (semáforos), *Autopista*
  (90-112 km/h) y *Deportivo* (aceleraciones fuertes).
- **4:** configura cualquier parámetro en tiempo real: fijarlo a un valor
  constante (**FIJO**) o devolverlo al modo simulado (**AUTO**). Mientras un
  parámetro está en FIJO, el simulador no lo toca.
- **5:** estado completo del vehículo con indicación AUTO/FIJO.
- **6:** consola ELM327 para probar comandos sin escáner, p. ej.:

```
> ATZ
ELM327 v1.5
> ATSP6
OK
> 010C
41 0C 1A F0        <- RPM = 0x1AF0 / 4 = 1724 RPM (ver PID 0x0C)
> 010D
41 0D 2D           <- velocidad = 0x2D = 45 km/h
> 0902
49 02 01 39 42 47 ...   <- VIN
```

- **7:** información del sistema y estado de registros del MCP2515
  (CANSTAT, CANINTF, EFLG, contadores de error TEC/REC).
- **8:** autotest de comunicación: pausa el tráfico CAN y ejecuta las mismas
  pruebas de la sección 9 (SPI, loopback y bus de dos módulos) dentro del
  emulador. Al terminar reinicializa el MCP2515 y reanuda el tráfico.
- **9:** monitor en vivo de la ECU en pantalla completa estilo CLI: el marco
  (textos fijos) se dibuja una vez y solo se reescriben los valores que
  cambian (~250 ms), sin parpadear, usando códigos de escape ANSI (no
  requiere librería externa). Termina con `q` y vuelve al menú. Útil para
  ver la dinámica de los perfiles sin escáner.

---

## 5. Comandos AT soportados (ELM327)

| Comando | Respuesta / efecto |
|---|---|
| `ATZ` | Reset → `ELM327 v1.5` |
| `ATI`, `AT@1`, `AT@2`, `AT@3` | Identificación del dispositivo |
| `ATE0/1` | Eco off/on |
| `ATL0/1` | Saltos de línea off/on |
| `ATH0/1` | Cabeceras (IDs) off/on |
| `ATS0/1` | Espacios off/on |
| `ATSP0`/`ATSPn` | Protocolo automático / específico (solo funciona físicamente el 6) |
| `ATDP`, `ATDPN` | Describe el protocolo (`A6`) |
| `ATRV` | Voltaje de batería, p. ej. `14.2V` |
| `ATSHxxxx`, `ATCRAxxxx` | Fijar ID de petición / respuesta (3 o 6 dígitos hex, 11/29 bits) |
| `ATD` | Valores por defecto |
| `ATFCSH`, `ATFCSM`, `ATFCSD` | Cabecera / modo / datos de Flow Control CAN (init de escáneres) |
| `ATCSM0/1`, `ATMA`, `ATCEA0/1`, `ATAL0/1`, `ATBD0/1`, `ATBRT*`, `ATWM*`, `ATIFR0/1`, `ATDM1`, `ATKW`, `ATMT`, `ATPPS` | Aceptados por compatibilidad con escáneres profesionales |
| `ATR0/1`, `ATST`, `ATAL`, `ATAR`, `ATBI`, `ATBD`, `ATCAF`, `ATCFC`, `ATCM`, `ATCEA`, `ATTP`, `ATIGN`, `ATWM`, `ATAT` | Aceptados por compatibilidad |
| desconocido | `?` |

> El eco está encendido por defecto (`ATE1`), como un ELM327 real; la consola
> del menú (opción 6) puede apagarlo con `ATE0`.

## 6. Modos y PIDs OBD2 soportados

**Modos:** `01` (datos actuales), `02` (freeze frame, simplificado), `03`
(DTCs confirmados — por defecto `P0301`, `P0420`), `04` (borrar DTCs),
`06` (monitores OBD en servicio), `07` y `0A` (DTCs pendientes/permanentes),
`08` (control de sistemas: negativa), `09` (VIN, calibración, nombre de
ECU — con ISO-TP multi-frame).

**Modo 01 (y 02):**

| PID | Parámetro | Fórmula |
|---|---|---|
| 00, 20, 40, 60 | PIDs soportados | máscara de bits |
| 01 | Estado de monitores + MIL | bit 7 = MIL, bits 0-6 = nº DTCs |
| 03 | Sistema de combustible | lazo cerrado/abierto |
| 04 | Carga calculada | A = %×255/100 |
| 05 | Temp. refrigerante | A = °C + 40 |
| 06 | Fuel trim corto (STFT1) | A = 128 + %×128/100 |
| 07 | Fuel trim largo (LTFT1) | A = 128 + %×128/100 |
| 0B | Presión MAP | A = kPa |
| 0C | RPM | A = (RPM/4) en 2 bytes |
| 0D | Velocidad | A = km/h |
| 0E | Avance de encendido | A = ° + 64 |
| 0F | Temp. de admisión (IAT) | A = °C + 40 |
| 10 | Flujo MAF | A = g/s × 100 (2 bytes) |
| 11 | Posición mariposa | A = %×255/100 |
| 13 | Sonda O2 (B1S1) | A = V×200 |
| 1C | Norma OBD | 0x06 (ISO 15765-4 CAN) |
| 1F | Tiempo motor encendido | 2 bytes (s) |
| 21 | Distancia recorrida | 4 bytes (km) |
| 2E | Purga EVAP (solenoide) | A = %×255/100 |
| 2F | Nivel de combustible | A = %×255/100 |
| 31 | Dist. desde borrado DTC | 4 bytes (km) |
| 33 | Presión barométrica (BARO) | A = kPa |
| 42 | Voltaje de batería | A = V/0.8 |
| 45 | Mariposa relativa | % |
| 46 | Temp. ambiente | A = °C + 40 |
| 49 | Posición del pedal | % |
| 4C | Mariposa comandada | % |
| **4E** | **Marcha (PID personalizado)** | **0 = N, 1-5 = marchas, 6 = R** |
| 5C | Temp. de aceite | A = °C + 40 |

**Modo 06 (monitores):** `46 <TID> <2B> <2B valor> <2B máx> <2B mín>` con
TIDs 01 (misfire), 02 (sistema combustible), 41 (catalizador), 61 (EVAP) y
91 (sonda O2), con valores plausibles dentro de rango.

**Modo 09:** `00` (soportados, con conteo), `02` (VIN
`9BGKL48T0HB130763`), `04` (calibraciones `1505708` y `52124404`, con
conteo), `0A` (nombre de ECU `GM PRISMA 1.4`).

> El PID `0x4E` no es estándar OBD2: es un PID personalizado del emulador para
> leer la marcha. Las apps estándar lo ignorarán; se usa con herramientas que
> permiten PIDs personalizados.

> **Compatibilidad con escáneres de diagnóstico:** el emulador acepta
> peticiones multi-PID (2-6 PIDs por trama), responde desde `0x7E9` a
> peticiones físicas a `0x7E0` (y desde `0x7E8` a `0x7DF`), y espera el
> Flow Control del escáner venga de donde venga (p. ej. `0x7DF`), de modo que
> las respuestas multi-frame (VIN/CALID/nombre ECU) llegan completas. Al
> conectar un escáner OBD-II (ELM327 u otro), el arranque típico
> (`ATZ ATE0 ATSP6 ATFCSH... 0100 0902`) responde OK.

## 6. Modos y PIDs OBD2 soportados

**Modos:** `01` (datos actuales), `02` (freeze frame, simplificado), `03`
(sin DTCs), `04` (borrar DTCs), `07` y `0A` (sin DTCs), `08` (control de
sistemas: negativa), `09` (VIN, calibración, nombre de ECU — con ISO-TP
multi-frame).

**Modo 01 (y 02):**

| PID | Parámetro | Fórmula |
|---|---|---|
| 00, 20, 40, 60 | PIDs soportados | máscara de bits |
| 01 | Estado de monitores | — |
| 03 | Sistema de combustible | lazo cerrado/abierto |
| 04 | Carga calculada | A = %×255/100 |
| 05 | Temp. refrigerante | A = °C + 40 |
| 06 | Fuel trim corto (STFT1) | A = 128 + %×128/100 |
| 07 | Fuel trim largo (LTFT1) | A = 128 + %×128/100 |
| 0B | Presión MAP | A = kPa |
| 0C | RPM | A = (RPM/4) en 2 bytes |
| 0D | Velocidad | A = km/h |
| 0F | Temp. de admisión (IAT) | A = °C + 40 |
| 10 | Flujo MAF | A = g/s × 100 (2 bytes) |
| 11 | Posición mariposa | A = %×255/100 |
| 13 | Sonda O2 (B1S1) | A = V×200 |
| 1C | Norma OBD | 0x06 (ISO 15765-4 CAN) |
| 1F | Tiempo motor encendido | 2 bytes (s) |
| 21 | Distancia recorrida | 4 bytes (km) |
| 2E | Purga EVAP (solenoide) | A = %×255/100 |
| 2F | Nivel de combustible | A = %×255/100 |
| 31 | Dist. desde borrado DTC | 4 bytes (km) |
| 42 | Voltaje de batería | A = V/0.8 |
| 45 | Mariposa relativa | % |
| 46 | Temp. ambiente | A = °C + 40 |
| 49 | Posición del pedal | % |
| 4C | Mariposa comandada | % |
| **4E** | **Marcha (PID personalizado)** | **0 = N, 1-5 = marchas, 6 = R** |
| 5C | Temp. de aceite | A = °C + 40 |

**Modo 09:** `00` (soportados), `02` (VIN `9BGKS48D0XC000001`), `04`
(calibración `12647587`), `0A` (nombre de ECU `GM PRISMA 1.4`).

> El PID `0x4E` no es estándar OBD2: es un PID personalizado del emulador para
> leer la marcha. Las apps estándar lo ignorarán; se usa con herramientas que
> permiten PIDs personalizados.

---

## 7. Arquitectura del código

```
include/
  mcp2515.h   driver MCP2515 (SPI0/CE0, GPIO25 INT, bit timing)
  vehicle.h   modelo del vehículo + simulador por perfiles + consola
  elm327.h    emulador ELM327/OBD2 (AT + PIDs + ISO-TP)
src/
  mcp2515.cpp
  vehicle.cpp
  elm327.cpp
  main.cpp    hilos (CAN + simulación) y menú interactivo
```

- **Hilo CAN:** sondea el INT (GPIO25) y lee los buffers RX del MCP2515;
  las peticiones en `0x7DF/0x7E0` se responden desde `0x7E8`.
- **Hilo de simulación (10 Hz):** actualiza los parámetros del vehículo según
  el perfil seleccionado; los parámetros en modo `FIJO` no se tocan.
- **Acceso a datos:** todos los accesos al modelo se serializan con un mutex.

## 8. Solución de problemas

- **`ERROR: no se pudo inicializar el MCP2515`** → ejecute con `sudo`;
  revise CE0/CS y el cableado; compruebe que el módulo tenga alimentación y
  que el cristal sea de 8 o 16 MHz (si es 8 MHz, ajuste `MCP2515_OSC_HZ`).
- **`libbcm2835 no está instalada`** → `make install-bcm2835`.
- **El escáner muestra `NO DATA`** → confirme que el escáner use
  ISO 15765-4 CAN 11-bit 500 kbps (`ATSP6`) y que CANH/CANL estén bien
  conectados (y terminados con 120 Ω si es un banco de pruebas con 2 nodos).
- **Sin ACK al transmitir** → el modo one-shot evita reintentos infinitos;
  sin otro nodo en el bus las transmisiones fallan (normal en un banco solo).
  Para probar, conecte un segundo nodo o un escáner que conteste.
- **INT sin pull-up en Pi 4** → añada 10 kΩ de GPIO25 a 3,3 V (ver sección 1).

---

## 9. Pruebas de comunicación (MCP2515 ↔ Raspberry Pi)

Las pruebas (lógica compartida en `test/autotest.{h,cpp}`) verifican el enlace
entre la Pi y el módulo. Pueden ejecutarse como binarios independientes o
**desde el propio emulador con la opción 8 del menú** (pausa el tráfico CAN,
ejecuta las tres pruebas y reinicializa el MCP2515 al terminar):

```bash
make test-build        # compila las pruebas (obj/test_*)
make test              # compila y ejecuta las tres con sudo
```

O directamente el script:

```bash
sudo ./scripts/run_tests.sh            # las tres pruebas
sudo ./scripts/run_tests.sh --spi      # solo enlace SPI
sudo ./scripts/run_tests.sh --loopback # solo TX/RX loopback
sudo ./scripts/run_tests.sh --bus      # solo prueba de bus (2 módulos)
./scripts/run_tests.sh --build --spi   # compila y ejecuta solo SPI
```

### Qué comprueba cada prueba

**`obj/test_spi` — enlace SPI (cableado):**

| Comprobación | Qué verifica |
|---|---|
| Reset y CANSTAT | Tras reset el MCP2515 queda en modo config (`0x80`). `0xFF` = no responde por SPI (revisar alimentación y cableado) |
| Escritura/lectura de registros | MOSI→SI y SO→MISO funcionan en ambos sentidos |
| BIT MODIFY | Comando de modificación de bits del protocolo SPI |
| CNF1/2/3 y RXB0CTRL | Registros de configuración escribibles (modo config) |
| Pin INT (GPIO25) | Nivel alto en reposo (sin interrupciones pendientes) |
| Contadores de error tras reset | TEC/REC/EFLG deben ser 0 (sin errores) |
| Detección de cristal (8 vs 16 MHz) | Mide CLKOUT (pin 3 → GPIO26) y determina el cristal del módulo; avisa si no coincide con `MCP2515_OSC_HZ` (informativo, ver abajo) |
| Velocidad real del bus SPI | Throughput medido: kB/s, Mbps efectivos y µs/transacción (informativo; <1 Mbps efectivo sugiere problemas de cableado) |

**`obj/test_loopback` — TX/RX del controlador:**

| Comprobación | Qué verifica |
|---|---|
| Modo loopback interno | El MCP2515 pasa a loopback (TX interno, sin salir al bus) |
| TX trama 0x123 (8 bytes) | La trama se transmite y se recibe en los buffers RX |
| INT (GPIO25) se activa | La interrupción por recepción llega al GPIO25 |
| Trama recibida idéntica | ID, DLC y datos coinciden (0x123 + 8 bytes y 0x7DF + 2 bytes estilo OBD2) |

### Detección del cristal del módulo (8 vs 16 MHz)

El cristal del MCP2515 determina el bitrate CAN (los registros CNF1/2/3 se
calculan para el oscilador asumido). Si el módulo tiene cristal de 8 MHz
pero el código compila con 16 MHz (`MCP2515_OSC_HZ` en `include/mcp2515.h`),
el bus queda a **250 kbps en vez de 500 kbps** y la comunicación con la ECU
falla. El test SPI lo detecta automáticamente:

- **Cómo funciona:** el pin **CLKOUT (pin 3)** del MCP2515 emite `Fosc/8`
  (prescaler por defecto, `CLKPRE=11`): **16 MHz → 2 MHz**, **8 MHz → 1 MHz**.
  El test mide la frecuencia con un bucle de polling durante ~200 ms y
  clasifica el resultado.
- **Cableado (opcional, 1 puente):** `CLKOUT` (pin 3 del MCP2515) → **GPIO26**
  (constante `AUTOTEST_CLKOUT_GPIO` en `test/autotest.h`).
  > ⚠️ **Niveles:** en los módulos CAN-BUS el MCP2515 trabaja a 5 V, y
  > `CLKOUT` conmuta entre 0 y 5 V. **No aplique 5 V directos a un GPIO de
  > la Pi** (no toleran 5 V): use un divisor resistivo (p. ej. 1 kΩ en serie
  > con el GPIO y 2 kΩ a GND → ≈3,3 V) o un level shifter. En módulos a
  > 3,3 V se puede conectar directo.
- **Interpretación:** `CLKOUT ≈ 2 MHz` → cristal **16 MHz**; `≈ 1 MHz` →
  cristal **8 MHz**; `CLKOUT no detectado (0 Hz)` → el puente no está, o el
  chip no arranca el oscilador (alimentación/cristal defectuoso/escrituras
  que no persisten).
- **Si el cristal medido no coincide con `MCP2515_OSC_HZ`:** el test lo
  avisa en pantalla con la instrucción de corregir el valor en
  `include/mcp2515.h` y recompilar. Para compilar para el cristal detectado:
  `make MCP2515_OSC_HZ=8000000` (o `16000000`). La detección es informativa:
  no hace fallar la prueba, pero es la causa típica de "SPI OK pero el bus
  CAN no comunica".

### Interpretación de fallos

- **`CANSTAT = 0xFF` / sin respuesta** → alimentación (VCC/GND), cableado
  CE0→CS / MOSI→SI / SCLK→SCK, o hay otro dispositivo ocupando CE0.
- **Patrón `0x00`** (CANSTAT=0x00 y escrituras que no persisten) → el chip
  no procesa: **sin alimentación, en reset o sin oscilador**. El test imprime
  un diagnóstico (ver abajo) que ayuda a distinguir las causas.
- **Fallan escritura/lectura** → MISO/SO o MOSI/SI cruzados, o niveles de
  voltaje (módulos a 5 V, ver sección 1).
- **Solo falla la comprobación de INT** → la conexión GPIO25→INT (o su
  pull-up en Pi 4, ver sección 1). El resto del enlace sigue siendo válido
  (este fallo no dispara el diagnóstico de chip).
- **Falla el loopback pero el SPI pasa** → problema del MCP2515 (cristal,
  módulo defectuoso) o de la configuración de bit timing.

#### Diagnóstico: "chip sin alimentación" vs "chip en reset"

Cuando falla el enlace a nivel de chip (reset, registros o bit timing), el
test imprime un bloque `DIAGNOSTICO DEL FALLO` con la clasificación del
patrón y dos sondas:

- **Sonda RESET (opcional, 1 puente):** conecte el pin **RESET (pin 11) del
  MCP2515 → GPIO27** (constante `AUTOTEST_RESET_GPIO` en `test/autotest.h`;
  con divisor 5 V→3,3 V si el módulo es de 5 V). El test lee el pin con
  pull-down y pull-up para detectar si está flotando, y concluye:
  - **RESET en bajo** → `[XX]` **el chip está EN RESET**: conecte RESET a
    3,3 V o VCC (con pull-up de 10 kΩ) y repita.
  - **RESET en alto** → `[OK]` el reset está bien: si el chip sigue sin
    responder, mida **VCC/GND (5 V)** con multímetro o pruebe otro módulo.
  - **Flotando / sin puente** → `[--]` no se puede concluir: haga el puente
    o mida RESET a mano (un RESET flotante es un fallo típico de los
    módulos baratos).
- **Oscilador (CLKOUT):** si la detección de cristal dio `0 Hz` (oscilador
  sin arrancar) y RESET está OK, el problema es el cristal OSC1/OSC2 o el
  propio MCP2515 (módulo defectuoso o mala soldadura).

La regla general: **`0x00` + RESET bajo → problema de reset**;
**`0x00` + RESET alto → problema de alimentación/cristal/módulo**.

### Prueba de bus con dos módulos MCP2515 (`obj/test_bus`)

Verifica la comunicación **CAN real entre dos módulos** conectados por el
mismo bus SPI y por el bus CAN. Requiere un **segundo módulo MCP2515**:

| Señal | Módulo A | Módulo B |
|---|---|---|
| MISO / MOSI / SCLK | SO / SI / SCK | SO / SI / SCK (SPI compartido) |
| CE0 (GPIO8) | CS | — |
| CE1 (GPIO7) | — | CS |
| GPIO25 | INT | — |
| GPIO24 | — | INT |
| CANH | CANH_A ↔ CANH_B |
| CANL | CANL_A ↔ CANL_B |

> **Terminación:** para un banco de 2 nodos el bus necesita **120 Ω en cada
> extremo**. La mayoría de los módulos MCP2515 trae los resistores (o puentes
> de soldadura) para activarlos; si no, añada 120 Ω entre CANH y CANL en cada
> módulo.

Qué comprueba:

| Comprobación | Qué verifica |
|---|---|
| A → B (0x321, 8 bytes) | TX del módulo A y RX del B (ambos en modo normal) |
| B → A (0x456, 8 bytes) | Comunicación en sentido contrario |
| INT de cada nodo | GPIO24 (B) y GPIO25 (A) se activan al recibir (aviso si no) |
| Intercambio OBD2 | Petición 0x7DF (modo 01, PID 0C) y respuesta 0x7E8 |
| Ráfaga de 100 tramas | Sin pérdida de tramas (contador de secuencia) |
| Errores CAN por nodo | TEC/REC/EFLG tras la ráfaga: 0 = sin errores; valores altos indican problemas de terminación/cableado/bitrate |

Si falla solo la comprobación de INT de un nodo, el bus funciona igual:
revise el cableado de ese GPIO→INT. Si fallan las tramas, revise CANH/CANL,
la terminación de 120 Ω y que ambos módulos usen el mismo cristal/baudios.

> **Sin el segundo módulo (caso habitual en un banco con un solo MCP2515):**
> la prueba de bus termina con el mensaje `ERROR: no se pudo inicializar el
> módulo B (CE1)` y el autotest marca `Bus: FALLO`, pero **el emulador sigue
> funcionando** (el tráfico CAN se reanuda y los PIDs responden). Dentro del
> emulador las pruebas reutilizan la inicialización de bcm2835 existente
> (`beginExisting()`/`endLight()`), por lo que un módulo ausente en CE1 ya no
> puede cerrar la librería globalmente y provocar un segfault.

**TEC/REC (contadores de error del MCP2515):** `TEC` cuenta los errores de
*transmisión* (p. ej. tramas sin ACK por falta de terminación, otro nodo
apagado o bitrate incorrecto) y `REC` los de *recepción*. `EFLG` agrupa las
banderas de error (bus-off `TXBO`, error-passive `TXEP/RXEP`, warning
`TXWAR/RXWAR/EWARN`, overflow `RX0OVR/RX1OVR`). En un bus sano ambos nodos
quedan en **error-active con TEC=REC=0**; si suben durante la prueba, revise
la terminación de 120 Ω, CANH/CANL y que el cristal/bitrate coincidan.

### Método alternativo: SocketCAN (driver de kernel mcp251x)

En lugar del acceso directo por bcm2835 (el que usa el emulador), se puede
verificar el mismo módulo con el **driver de kernel mcp251x** y la pila
SocketCAN (`ip`, `cansend`, `candump` de `can-utils`).

> **IMPORTANTE:** el driver de kernel y el emulador (bcm2835 directo)
> compiten por el mismo módulo SPI/GPIO. Use **uno a la vez**: si `can0`
> existe, el emulador no funcionará sobre ese módulo hasta desactivar la
> overlay (o usar otro módulo).

**1. Configurar las overlays (una sola vez, requiere reinicio):**

```bash
sudo apt install -y can-utils iproute2
sudo ./scripts/can_kernel_test.sh --setup              # can0 (CE0, INT 25)
sudo ./scripts/can_kernel_test.sh --setup --bus        # además can1 (CE1, INT 24)
sudo reboot
```

Si el cristal del módulo es de 8 MHz, añada `--osc=8000000`. El script hace
una copia de `/boot/config.txt` antes de editarlo.

**2. Probar:**

```bash
sudo ./scripts/can_kernel_test.sh            # loopback del kernel en can0
sudo ./scripts/can_kernel_test.sh --bus      # can0 <-> can1 (dos módulos)
```

El loopback del kernel no necesita bus ni segundo nodo; `--bus` necesita las
dos interfaces (dos módulos con CANH/CANL unidos y 120 Ω por extremo).
También se puede probar a mano:

```bash
sudo ip link set can0 up type can bitrate 500000
candump can0 &                      # escuchar en segundo plano
cansend can0 123#DEADBEEF01020304   # enviar una trama
sudo ip link set can0 down
```

Equivalencias con las pruebas C++: `--loopback` ≈ `test_loopback`, `--bus` ≈
`test_bus`. Desde el runner: `sudo ./scripts/run_tests.sh --socketcan`, y
`make test-socketcan` compila nada y ejecuta el script con sudo.
