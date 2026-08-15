# Changelog

## [2.0.0] - 2026-06-10

### Added
- Sistema de configuracion externa (`config/obd2_rpi.conf`)
- Registro centralizado de PIDs (`pid.hpp`/`pid.cpp`)
- Interfaz de display abstracta (`display_iface.hpp`)
- Tipos compartidos unificados (`types.hpp`)
- Script de compilacion (`scripts/build.sh`) con soporte remoto
- Documentacion tecnica completa (`docs/`)
- OpenCode skill para desarrollo asistido
- Instalacion CMake con targets

### Changed
- `main.cpp` refactorizado para usar sistema de configuracion
- `CMakeLists.txt` mejorado con opciones y deteccion de plataforma
- Mejor manejo de errores en conexion BT
- Ciclos de polling configurables

### Fixed
- Deteccion de arquitectura ahora usa `CMAKE_SYSTEM_PROCESSOR`
- Timeout de BT usando constante configurable

## [1.0.0] - 2025

### Added
- Conexion Bluetooth RFCOMM con ELM327
- Lectura de PIDs OBD-II estandar (modo 01)
- Comandos GM modo 22 (UDS)
- Driver SPI para SSD1306 128x64
- 7 paginas OLED con auto-rotacion
- Logging CSV
- Servicio systemd
- Control por teclado (stdin)
