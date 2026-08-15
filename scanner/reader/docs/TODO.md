# TODO — OBD2 RPi + SSD1306 SPI

## Corto plazo

- [ ] Agregar reconexión automática Bluetooth cuando se pierde el enlace
- [ ] Manejar señal SIGHUP para reinicio limpio
- [ ] Verificar que `getDTCs()` no bloquee el ciclo de polling
- [ ] Agregar heartbeat LED (GPIO) cuando hay conexión activa

## Mediano plazo

- [ ] Soporte multi-idioma (español/inglés) en display
- [ ] Historial de DTCs con timestamp (guardar en archivo)
- [ ] Modo gráfico: mini sparklines de RPM y carga en dashboard
- [ ] Soporte de buzzer para alertas (sobrecalentamiento, DTC crítico)
- [ ] OBD2 Mode 06 (monitor de sensores) para tests onboard
- [ ] Cache de respuestas de PIDs lentos (GM, DTCs)

## Largo plazo

- [ ] Interfaz web vía WiFi integrada (ESP32 bridge o RPi zero W)
- [ ] Exportar datos a MQTT
- [ ] Soporte para múltiples protocolos OBD2 (CAN, KWP, PWM, VPW)
- [ ] App companion Flutter para display remoto
- [ ] Over-the-air (OTA) updates via rsync/git
- [ ] Dashboard configurable (elegir qué PIDs mostrar por página)
- [ ] Almacenamiento en SQLite de históricos de viaje
- [ ] Soporte de modo batch (lectura+logging sin display)

## Bugs conocidos

- [ ] `hci_read_remote_name` puede bloquearse hasta 2s por dispositivo no visible
- [ ] La página DEBUG BT muestra datos de la última TX/RX pero no se refresca en tiempo real
- [ ] El auto-rotate no respeta cambios de página manuales inmediatos

## Arquitectura

- [ ] Extraer estadísticas de comunicación a su propio módulo
- [ ] Agregar tests unitarios con Google Test
- [ ] CI/CD con GitHub Actions para compilación cruzada ARM64
- [ ] Separar `oled_display.cpp` en 7 archivos (uno por página)
