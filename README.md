# MCP2515 Integral — Raspberry Pi OBD2/CAN

Monorepo integral que unifica todos los proyectos de diagnóstico automotriz
sobre Raspberry Pi + MCP2515 que antes vivían en carpetas separadas.

## Productos

| Producto | Rama | Ubicación | Descripción |
|---|---|---|---|
| **ECU Emulator** | `emulator` | `emulator/` | Emula ECUs de vehículos: `prisma/` (Chevrolet Prisma, MCP2515 SPI) y `multi/` (8 marcas, SocketCAN + APIs) |
| **AUTEL Scanner** | `scanner` | `scanner/` | Scanner de diagnóstico: `autel_scanner/` (CAN + OLED SSD1306) y `reader/` (lector OBD2 Bluetooth ELM327) |
| **Hub** | `main` | raíz | README, `AGENTS.md` (reglas), `.opencode/skills/`, `docs/`, `legacy/` (referencia) |

## Ramas

- **`main`** — hub integral: documentación, skills centralizadas, reglas de
  trabajo y código `legacy/` de referencia.
- **`emulator`** — el producto emulador completo (`emulator/prisma` +
  `emulator/multi`) con sus skills y docs.
- **`scanner`** — el producto scanner completo (`scanner/autel_scanner` +
  `scanner/reader`) con sus skills y docs.

```bash
git branch emulator && git branch scanner   # ver ramas
git checkout emulator                        # trabajar en los emuladores
git checkout scanner                         # trabajar en el scanner
```

## Origen de los proyectos

| Carpeta nueva | Proyecto original | Repo GitHub |
|---|---|---|
| `emulator/prisma` | `mcp2515_emulator_obd2_opencode` | https://github.com/siliconvalleyar-oss/mcp2515_emulator_rpi |
| `emulator/multi` | `mcp2515_rpi` | https://github.com/siliconvalleyar-oss/mcp2515_rpi |
| `scanner/autel_scanner` | `raspberry_pi_scanner` | https://github.com/siliconvalleyar-oss/mcp2515_scanner_rpi |
| `scanner/reader` | `elm327_rpi2w` | https://github.com/siliconvalleyar-oss/obd2_oled_rpi2w |
| `legacy/prisma-openai` | `mcp2515_openai` | — |
| `legacy/prisma-clude` | `prisma-emulator_clude` | — |
| `legacy/scanner-duplicate` | `mcp2515_scanner_rpi` | https://github.com/siliconvalleyar-oss/mcp2515_scanner_rpi (copia idéntica del scanner, eliminada como duplicado) |

## Reglas de trabajo

Ver [`AGENTS.md`](AGENTS.md). Resumen:

- Compilar/probar **solo en la Raspberry Pi**, nunca en el dev host.
- Versionado: tag `vX.Y.Z` == archivo `VERSION`; ciclo patch 0-9 obligatorio
  (ej. `v1.0.9` → `v1.1.0`); todo push con su tag.
- Commits con conventional commits (`feat:`, `fix:`, `docs:`, `chore:`, ...).
- Usar las skills de `.opencode/skills/` antes de tocar cada producto.

## Estructura

```
mcp2515_integral/
├── AGENTS.md                  # Reglas de trabajo
├── README.md                  # Este archivo
├── docs/                      # Arquitectura integral
├── .opencode/skills/          # Skills centralizadas (6)
├── legacy/                    # Proyectos de referencia (históricos/duplicados)
│   ├── prisma-openai/
│   ├── prisma-clude/
│   └── scanner-duplicate/
├── emulator/                  # (rama emulator)
│   ├── prisma/                # Emulador ECU Chevrolet Prisma (MCP2515 SPI)
│   └── multi/                 # ECU Multi-Emulator 8 marcas (SocketCAN)
└── scanner/                   # (rama scanner)
    ├── autel_scanner/         # Scanner AUTEL (CAN + OLED)
    └── reader/                # Lector OBD2 Bluetooth + OLED
```

## Licencia

MIT
