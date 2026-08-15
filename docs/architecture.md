# Arquitectura Integral

Diagrama de alto nivel del workspace MCP2515 Integral.

```
                     ┌─────────────────────────────────────────────┐
                     │              mcp2515_integral               │
                     │      (rama main = hub + legacy)              │
                     │   README · AGENTS.md · .opencode/skills     │
                     └───────┬──────────────┬──────────────┬───────┘
                             │              │              │
                 (rama emulator)   (rama scanner)    (legacy/ referencias)
                             │              │              │
               ┌─────────────┴─────┐   ┌────┴─────────┐    ├── prisma-openai
               │  emulator/prisma  │   │  scanner/    │    ├── prisma-clude
               │  (ECU Prisma SPI) │   │  autel_scanner│   └── scanner-duplicate
               └───────────────────┘   │  reader/     │
               ┌───────────────────┐   └──────────────┘
               │  emulator/multi   │
               │  (8 marcas, CAN)  │
               └───────────────────┘
```

## Flujos de datos

### Emulador Prisma (`emulator/prisma`) — emula una ECU en el bus CAN
```
Escáner OBD2/ELM327 ── CAN 500k ──▶ MCP2515 (SPI0/bcm2835) ──▶ Hilo CAN
                        ◀── respuesta 0x7E8/0x7E9 ──          Hilo simulación (10 Hz)
                                                              Vehicle (mutex) + Simulator
```

### Multi-emulador (`emulator/multi`) — emula 8 marcas por SocketCAN
```
cansend/candump ── SocketCAN (can0/vcan0) ──▶ CANManager (rx/tx threads)
                                              → ProtocolRouter → BaseManufacturer
                                              → APIs REST (8080) / WS (8081) / gRPC
                                              → SQLite · simulación 50 ms · seed/key
```

### Scanner AUTEL (`scanner/autel_scanner`) — lee una ECU y muestra en OLED
```
ECU ── CAN 500k ──▶ MCP2515 (SPI) ──▶ Scanner::OBD2 (mode/pid)
                                        → decodePID → Menu/Display (SSD1306 I2C)
```

### Lector OBD2 (`scanner/reader`) — datos en vivo vía Bluetooth
```
ELM327 Bluetooth ── RFCOMM ──▶ ELM327 driver ──▶ VehicleData (mutex) ──▶ SSD1306 SPI
```

## Hilos por producto

| Producto | Threads |
|---|---|
| `emulator/prisma` | CAN (poll INT) + simulación 10 Hz + main (menú) |
| `emulator/multi` | CAN rx + CAN tx + REST + WS + gRPC + loop principal 50 ms |
| `scanner/autel_scanner` | Monohilo (llamadas OBD2 síncronas) |
| `scanner/reader` | main (teclado) + OBD poll 800 ms + display 400 ms |
