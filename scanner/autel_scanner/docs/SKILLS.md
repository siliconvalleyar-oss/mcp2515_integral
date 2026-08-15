# SKILLS - Aprendizajes del Proyecto mcp2515_scanner_rpi

## 1. Migración de wiringPi a bcm2835

### Cambios realizados

Se migró toda la capa de GPIO desde `wiringPi` a `bcm2835` para compatibilidad
con Raspberry Pi OS 32 y 64 bits.

**Archivos modificados:**
- `src/hardware/gpio.cpp`
- `Makefile`
- `CMakeLists.txt`
- `README.md`
- `scripts/setup.sh`

### API reemplazada

| wiringPi | bcm2835 |
|----------|---------|
| `wiringPiSetup()` | `bcm2835_init()` |
| `pinMode(pin, mode)` | `bcm2835_gpio_fsel(pin, fsel)` |
| `digitalWrite(pin, val)` | `bcm2835_gpio_write(pin, HIGH/LOW)` |
| `digitalRead(pin)` | `bcm2835_gpio_lev(pin)` |
| `pullUpDnControl(pin, pud)` | `bcm2835_gpio_set_pud(pin, pud)` |
| `wiringPiISR(...)` | polling thread con `std::thread` |

### Constantes de bcm2835

- Modo pin: `BCM2835_GPIO_FSEL_INPT`, `BCM2835_GPIO_FSEL_OUTP`, `BCM2835_GPIO_FSEL_ALT0` a `ALT5`
- Pull-up/down: `BCM2835_GPIO_PUD_OFF`, `BCM2835_GPIO_PUD_DOWN`, `BCM2835_GPIO_PUD_UP`
- Niveles: `HIGH` (0x1), `LOW` (0x0)

### Nota sobre numeración de pines

`bcm2835` usa numeración **BCM GPIO** directamente. Antes, con `wiringPi`, el
pin 8 correspondía a GPIO2 (SDA1). Con `bcm2835`, el pin 8 es GPIO8 (CE0), lo
que corrige el mapeo del CS del MCP2515 para coincidir con `docs/HARDWARE.md`.

## 2. Problemas de compilación resueltos

### Designated initializers en C++17

`spi_ioc_transfer` se inicializaba con designated initializers (`.campo = valor`),
que son C++20. En C++17 generan warnings y fallan.

**Solución:** usar `memset(&tr, 0, sizeof(tr))` y asignar campos después.

### Variable 'Edge' no reconocida en gpio.cpp

Error: `'Edge' does not name a type` dentro de `ISRData`.

**Causa:** `Edge` está dentro del namespace `Hardware`. Dentro de `ISRData`
(que también está en `Hardware`), hay que calificar como `Hardware::Edge` o
bien declarar `Edge` como `GPIO::Edge`.

**Solución:** cambiar `Edge edge;` por `GPIO::Edge edge;` en `ISRData`.

### 'BCM2835_GPIO_HIGH' no declarado

Se usaron constantes inexistentes como `BCM2835_GPIO_HIGH` y `BCM2835_GPIO_LOW`.

**Solución:** `bcm2835.h` define `HIGH` y `LOW` directamente, no con prefijo
`BCM2835_GPIO_`. Usar `HIGH` y `LOW`.

### Variable sin usar en mcp2515.cpp

Warning: unused variable `ctrlReg` en `receiveMessage()`.

**Solución:** eliminar la variable `ctrlReg` que no se usa.

## 3. Compilación remota en Raspberry Pi

### Comando de compilación y ejecución

```bash
ssh joy@raspberry.local "cd /home/joy/src/raspberry_pi_scanner && make clean && make -j4 && sudo make run"
```

### Flujo documentado

El flujo completo está documentado en `docs/WORKFLOW.md`.

### Sincronización de código

Los cambios locales se suben a la Raspberry Pi con `scp` antes de compilar
remotamente, o se sincroniza el repo completo con git.

## 4. Versionado y Git

### Esquema de versionado

Reglas definidas en `docs/LEARNINGS.md`:

- Formato: `vMAJOR.MINOR.PATCH` → `v1.0.0`, `v1.0.1`, ..., `v1.0.9`, `v1.1.0`
- El archivo `VERSION` debe coincidir con el último tag (sin prefijo `v`)
- Ciclo patch 0-9 obligatorio antes de subir minor
- No se pueden eliminar tags publicados
- Todo push debe incluir tag

### Repositorio GitHub

- URL: `https://github.com/siliconvalleyar-oss/mcp2515_scanner_rpi`
- Usuario: `siliconvalleyar-oss`
- Token configurado en git credential helper

## 5. Dependencias del proyecto

### Raspberry Pi OS

```bash
sudo apt update
sudo apt install -y build-essential cmake libbcm2835-dev i2c-tools
```

### Interfaces requeridas

- SPI: para MCP2515 (CAN bus)
- I2C: para SSD1306 (OLED display)

### Configuración de interfaces

```bash
sudo raspi-config nonint do_spi 0
sudo raspi-config nonint do_i2c 0
```

## 6. Hardware conectado

### MCP2515 (CAN Bus)

- CS: GPIO8 (CE0)
- SCK: GPIO11 (SCLK)
- SI: GPIO10 (MOSI)
- SO: GPIO9 (MISO)
- INT: GPIO25 (opcional)
- Alimentación: 3.3V

### SSD1306 OLED 128x32

- SDA: GPIO2 (SDA1)
- SCL: GPIO3 (SCL1)
- Alimentación: 3.3V

## 7. Estructura del proyecto

```
raspberry_pi_scanner/
├── CMakeLists.txt
├── Makefile
├── VERSION
├── README.md
├── docs/
│   ├── HARDWARE.md
│   ├── LEARNINGS.md
│   ├── SKILLS.md
│   └── WORKFLOW.md
├── include/
│   ├── hardware/
│   │   ├── gpio.hpp
│   │   ├── i2c.hpp
│   │   ├── mcp2515.hpp
│   │   ├── spi.hpp
│   │   └── ssd1306.hpp
│   └── scanner/
│       ├── scanner.hpp
│       ├── obd2.hpp
│       ├── menu.hpp
│       ├── display.hpp
│       ├── dtc.hpp
│       ├── live_data.hpp
│       └── active_test.hpp
├── src/
│   ├── main.cpp
│   ├── hardware/
│   │   ├── gpio.cpp
│   │   ├── i2c.cpp
│   │   ├── spi.cpp
│   │   ├── mcp2515.cpp
│   │   └── ssd1306.cpp
│   └── scanner/
│       ├── scanner.cpp
│       ├── obd2.cpp
│       ├── menu.cpp
│       ├── display.cpp
│       ├── dtc.cpp
│       ├── live_data.cpp
│       └── active_test.cpp
├── config/
│   ├── default.cfg
│   ├── local.cfg.example
│   └── logging.cfg
├── scripts/
│   ├── build.sh
│   └── setup.sh
└── tests/
    └── test_obd2.cpp
```

## 8. Comandos útiles

### Compilación local

```bash
make clean
make -j4
sudo make run
```

### Compilación remota

```bash
ssh joy@raspberry.local "cd /home/joy/src/raspberry_pi_scanner && make clean && make -j4 && sudo make run"
```

### Versionado

```bash
# 1. Leer último tag
git describe --tags --abbrev=0

# 2. Actualizar VERSION
echo "1.0.1" > VERSION

# 3. Commit y push con tag
git add VERSION
git commit -m "chore: bump version to 1.0.1"
git tag v1.0.1
git push origin master --tags
```

### Verificación de build

```bash
# Ver warnings/errores
make clean && make -j4 2>&1 | grep -E "error:|warning:"

# Verificar binario
ls -la bin/scanner_autel_*
```

## 9. Notas importantes

- El programa requiere ejecutarse como root (`sudo`) para acceso a GPIO/SPI/I2C
- El archivo de log se escribe en `/var/log/autel_scanner.log`
- La compilación solo se hace remotamente en la Raspberry Pi
- Todos los documentos `.md` van en `docs/`
- No se usa `wiringPi` en ninguna parte del código
