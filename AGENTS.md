# AGENTS.md — Reglas de trabajo del workspace MCP2515 Integral

Reglas obligatorias para cualquier agente/desarrollador que trabaje en este repo.

## Flujo de trabajo remoto

1. **Editar/commitear localmente** (nada se edita directo en la máquina remota).
2. **Commit local** con conventional commits:
   `feat:`, `fix:`, `docs:`, `chore:`, `refactor:`, `test:`.
3. **Push** de la rama correspondiente (`main`, `emulator` o `scanner`).
4. **Actualizar la Pi** con `git pull` y compilar/probar SOLO ahí.
5. No dejar archivos generados (`build/`, `obj/`, `bin/`) en el remoto.

## Compilación y pruebas

- Los proyectos de hardware (MCP2515 SPI / GPIO / Bluetooth) se compilan y
  prueban **en la Raspberry Pi**, no en el dev host.
- Acceso a memoria/GPIO (bcm2835) y `/dev/spidev` requieren `sudo`/root.
- MCP2515: verificar cristal 8/16 MHz (`MCP2515_OSC_HZ`) antes de tocar bit
  timing; un cristal equivocado deja el bus a mitad de velocidad.
- `mcp2515_rpi` (multi): usa SocketCAN (`can0`/`vcan0`), no SPI directo.
- El driver kernel `mcp251x` y bcm2835 **compten por el módulo SPI**: usar uno
  a la vez.

## Versionado

- **Tag = VERSION:** el tag lleva `v` (`v1.0.5`) y el archivo `VERSION` (raíz
  de cada producto) el mismo número sin `v` (`1.0.5`). Siempre deben coincidir.
- **Ciclo patch 0-9:** no pasar de `v1.0.9` a `v1.1.1`; va a `v1.1.0`.
  Cada minor tiene exactamente 10 patches (0-9).
- **Todo push debe llevar su tag.** No se pushea sin tag.
- No eliminar tags publicados ni retroceder de versión.
- Bump:
  ```bash
  echo "1.1.0" > VERSION
  git add VERSION && git commit -m "chore: bump version to 1.1.0"
  git tag v1.1.0
  git push origin <rama> && git push origin v1.1.0
  ```

## Skills

Antes de tocar un producto, cargar su skill en `.opencode/skills/`:

- `mcp2515-raspberry-projects` — vista general del workspace.
- `ecu-multi-emulator` — `emulator/multi` (8 marcas, SocketCAN, APIs).
- `prisma-emulator-opencode` — `emulator/prisma` (MCP2515 SPI + ELM327 + ISO-TP).
- `autel-scanner-rpi` — `scanner/autel_scanner` (CAN + OLED SSD1306).
- `obd2-rpi-reader` — `scanner/reader` (Bluetooth ELM327 + OLED).
- `prisma-emulator-legacy` — referencia para `legacy/prisma-*`.

## Conceptos CAN/OBD2 compartidos

- IDs ISO 15765-4 (CAN 11-bit): funcional `0x7DF`, físico `0x7E0`,
  respuestas `0x7E8`/`0x7E9`.
- ELM327 `SP6` = ISO 15765-4 CAN 11-bit @ 500 kbps.
- ISO-TP: Single Frame (PCI `0x00|len`), First Frame (`0x10`), Flow Control
  (`0x30`), Consecutive Frames (`0x20|seq`, 7 bytes).
- Modos OBD2: `01` datos, `02` freeze frame, `03/07/0A` DTCs, `04` clear,
  `06` monitores, `08` control, `09` VIN/CALID/ECU name.
- Multi-frame sin Flow Control = el escáner no recibe VIN/CALID completos.
