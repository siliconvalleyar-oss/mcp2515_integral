# AUTEL Scanner & OBD2 Reader — Rama `scanner`

Producto de lectura/diagnóstico del monorepo `mcp2515_integral`. Lee datos de
una ECU (p. ej. la que emula la rama `emulator`) y los muestra en pantalla.

## Componentes

| Carpeta | Proyecto | Hardware | Detalle |
|---|---|---|---|
| `autel_scanner/` | **Scanner AUTEL** | MCP2515 SPI (bcm2835) + SSD1306 I2C | Menú tipo AUTEL MaxiSYS, LiveData, DTCs, ActiveTest. Ver `autel_scanner/README.md` |
| `reader/` | **Lector OBD2** | Bluetooth ELM327 (RFCOMM) + SSD1306 SPI | 7 páginas OLED, PIDs, GM mode 22, sistema systemd. Ver `reader/README.md` |

## Build rápido

```bash
# Scanner AUTEL (requiere root: GPIO/SPI/I2C)
cd autel_scanner && ./scripts/setup.sh && ./scripts/build.sh && sudo ./build/autel_scanner

# Lector OBD2
cd reader && cmake -S . -B build -DBUILD_TESTS=OFF && make -C build -j$(nproc)
./bin/obd2_rpi 00:1D:A5:07:23:6E /dev/spidev0.0 25 17
```

## Skills

- `.opencode/skills/autel-scanner-rpi` — `autel_scanner/`
- `.opencode/skills/obd2-rpi-reader` — `reader/`

## Notas

- `reader/` heredó la carpeta `skills/` del proyecto original (documento
  `obd2_rpi.md`); las skills activas viven en `.opencode/skills/` de la raíz.
- El scanner usa `bcm2835` (no wiringPi); numeración BCM GPIO (CS=GPIO8 CE0).
- Compilar/probar solo en la Raspberry Pi; `sudo` requerido.
- Versionado: tag `vX.Y.Z` == archivo `VERSION`; ciclo patch 0-9. Ver
  `AGENTS.md` de la raíz.
