# legacy/ — Proyectos de referencia

Código de proyectos históricos y duplicados, conservado como referencia.
**No editar ni usar como base de desarrollo** — los productos activos viven en
`emulator/` y `scanner/`.

| Carpeta | Proyecto original | Estado | Por qué está aquí |
|---|---|---|---|
| `prisma-openai/` | `mcp2515_openai` | Histórico | Primera versión del emulador Prisma (modos DYNAMIC/FIXED/RANDOM, perfiles NORMAL/SPORT/ECONOMY/FAILSAFE, sin ISO-TP) |
| `prisma-claude/` | `prisma-emulator_clude` | Histórico | Versión intermedia del emulador (primer polling CAN, perfiles RALENTI/URBANO/CARRETERA/AGRESIVO) |
| `scanner-duplicate/` | `mcp2515_scanner_rpi` | Duplicado | Copia byte-idéntica de `scanner/autel_scanner` (ex `raspberry_pi_scanner`), eliminada como duplicado. Su repo GitHub sigue en https://github.com/siliconvalleyar-oss/mcp2515_scanner_rpi |

Los repos originales conservan su historial en GitHub:

- Emulador Prisma activo: https://github.com/siliconvalleyar-oss/mcp2515_emulator_rpi
- Multi-emulador: https://github.com/siliconvalleyar-oss/mcp2515_rpi
- Scanner: https://github.com/siliconvalleyar-oss/mcp2515_scanner_rpi
- Lector OBD2: https://github.com/siliconvalleyar-oss/obd2_oled_rpi2w

## Sobre la evolución del emulador Prisma

```
mcp2515_openai (v1, sin ISO-TP) ──▶ prisma-emulator_clude (v2, polling CAN)
                                        │
                                        ▼
                     mcp2515_emulator_obd2_opencode (v3, activo → emulator/prisma)
                     ISO-TP multi-frame, DTCs 03-0A, modo 06/09, autotests
```

La regla de oro del versionado (tag `vX.Y.Z` == `VERSION`, ciclo patch 0-9,
todo push con tag) aplica solo a los productos activos.
