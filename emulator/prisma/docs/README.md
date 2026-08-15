# Documentación del proyecto

Índice de la documentación del **Emulador OBD2 Chevrolet Prisma (ELM327 + MCP2515)**.

## Documentos

| Archivo | Contenido |
|---|---|
| [BUG_REPORT.md](BUG_REPORT.md) | **Reporte de bugs y auditoría técnica** (2026-08-14, v1.0.0). Errores y fallas detectados (16 bugs, P0-P3), foco en el escenario "escáner OBD2 conectado", sugerencias de nuevas implementaciones, checklist de corrección por prioridad y reglas para quien lo ejecute. |
| [SKILLS.md](SKILLS.md) | Compendio de todo lo aprendido del proyecto: workflow local/remoto, reglas de Git/versionado, build y pruebas, cableado MCP2515, comandos AT/PIDs OBD2, arquitectura, solución de problemas y SocketCAN. |
| [WORKFLOW.md](WORKFLOW.md) | Flujo de desarrollo: cambios locales, compilación y pruebas solo en la Pi, bump de versión con tag. |
| [LEARNINGS.md](LEARNINGS.md) | Reglas y aprendizajes: versionado estricto (tag = `VERSION`, ciclo patch 0-9), push con token y credenciales. |

## Documentación complementaria

- [README.md](../README.md) — manual principal del proyecto (raíz): cableado, instalación,
  uso, comandos AT/PIDs, arquitectura, solución de problemas y pruebas de comunicación.
- `test/` y `scripts/` — pruebas de comunicación (SPI, loopback, bus, SocketCAN).

## Orden recomendado de lectura

1. `../README.md` — qué es y cómo usarlo.
2. `WORKFLOW.md` — cómo trabajar (local/remoto).
3. `LEARNINGS.md` — reglas de versionado y git.
4. `SKILLS.md` — resumen de conocimiento del proyecto.
5. `BUG_REPORT.md` — estado conocido de bugs y checklist pendiente.
