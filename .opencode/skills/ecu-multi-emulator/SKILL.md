---
name: ecu-multi-emulator
description: |
  Use when working on the multi-brand ECU emulator project (./emulator/multi, formerly mcp2515_rpi) — a C++17 multi-brand vehicle diagnostic ECU emulator for Raspberry Pi + MCP2515 CAN controller via SocketCAN. Covers 8 manufacturers (Chevrolet/GM, Ford, Toyota, BMW, VW/Audi, Mercedes, Honda, Nissan+Hyundai), OBD2 modes 01-0A, UDS ISO 14229, GMLAN/KWP2000/CAN-TP protocols, seed/key security, simulation engine with vehicle profiles, REST API (8080), WebSocket (8081), gRPC, SQLite3, Prometheus metrics. Use ONLY when the task targets this specific multi-manufacturer emulator project.
compatibility:
  - C++17
  - arm-linux-gnueabihf-g++ (cross) / native g++
  - Linux SocketCAN (can0 / vcan0)
  - SQLite3, nlohmann/json, Catch2
---

# ECU Multi-Emulator Skill (mcp2515_rpi)

## Quick Reference
```bash
make                       # arm-linux-gnueabihf-g++ -std=c++17 (cross)
make CROSS_COMPILE=        # native g++
sudo ./install_multi_emulator.sh
sudo modprobe vcan && sudo ip link add vcan0 type vcan && sudo ip link set vcan0 up
./ecu_emulator [config.json]     # config.json: interface can0|vcan0
curl http://localhost:8080/vehicles
cansend can0 7DF#02010C0000000000   # OBD2 mode 01 RPM
curl -X POST localhost:8080/select -d '{"manufacturer":"toyota","model":"Camry"}'
cd build && ctest                  # Catch2 unit tests
```

## Structure
```
src/core/        can_manager, protocol_router, session_manager, security
src/manufacturers/ base.hpp/cpp + chevrolet, ford, toyota, bmw, volkswagen,
                   mercedes, honda, nissan
src/protocols/   obd2_standard, uds, gmlan, kwp2000, can_tp
src/database/    db_manager, migrations, seed_data
src/diagnostics/ dtc_manager, freeze_frame, readiness, vin_decoder, calibration
src/simulation/  driving_cycle, sensor_simulator, fault_injector, environment,
                 vehicle_profile
src/api/         rest_api, websocket, grpc_server
src/security/    access_control, secure_comm, seed_key
src/logging/     can_logger, metrics_exporter, replay_analyzer
src/tests/       test_all_modes, test_manufacturers, fuzzing_suite
docker/ · examples/ · docs/api.yaml · config.json · install_multi_emulator.sh
```

## Core
- `CANManager` (SocketCAN SOCK_RAW): `rxLoop()` (poll→recvfrom, timestamp +
  bus_id, dispatch callbacks por `(can_id & mask)`) + `txLoop()` (cola
  `tx_queue_` + `tx_mutex_`). Main registra callback catch-all `(0x000,0x000)`.
- `ProtocolRouter`: mapa Manufacturer→BaseManufacturer; `selectManufacturer()`
  llama `onManufacturerSelected(cfg)`; `routeCANFrame()`. Config:
  rx_id=0x7E8, tx_id=0x7E0, functional_id=0x7DF, bitrate=500000, vin_prefix.
- `SessionManager`: sesiones UDS {DEFAULT, PROGRAMMING, EXTENDED, SAFETY},
  SecurityLevel {LOCKED, LEVEL_1..3}, seed/key por nivel, tester-present,
  timeouts 5s/10s.

## Manufacturers
- `BaseManufacturer`: OBD2 01-0A completo, UDS (0x10,11,27,28,3E,22,23,2E,3D,
  31,34-37...), DTCs, VIN/calibraciones, sensores simulados, odómetro
  (`odometer_km_`/`trip_km_`), tabla PIDs (~50, `setupDefaultPIDs()` +
  `addPID()`), `sendResponse/sendPositiveResponse/sendNegativeResponse`.
- Marcas: ChevroletGM (GMLAN 22/2E, DIDs GM), Ford (PATS, MS-CAN), Toyota
  (modo 21/22, smart key, checksum XOR), BMW (DCAN/EDIABAS, Vanos), VW/Audi
  (adaptación, long coding), Mercedes (DAS/SBC), Honda (HDS, i-VTEC),
  Nissan (Consult III, NATS). VIN prefixes: 1G1/1FA/JT2/WBA/WVW/WDB/JHM/JN1/KMH.

## Protocols
- `CANTransportProtocol`: ISO 15765-2 state machine (SF/FF/CF/FC, seq 0x0F
  wrap, `prepareMessage` segmenta TX).
- `OBD2Standard`: tabla PIDs, encode/decode, DTC, checksum.
- `UDSProtocol`: ISO 14229 (SIDs 0x10-0x37, NRC 0x10-0x78).
- `KWP2000Protocol`: START/STOP comm, checksum. `GMLANProtocol`: 0x1A/0x3B/
  0xA2, DIDs 0x1A00 VIN, 0xF180-F185.

## API
- REST (8080, raw HTTP/1.1 + CORS): `GET /`|`/health`, `GET /profile`,
  `GET /profiles`, `POST /profile`, `GET/POST /odometer`, `GET /dtcs [?all]`,
  `POST /dtcs/clear`. 404 si desconocido.
- WebSocket (8081, 100 clientes, JSON: rpm, speed, coolant, MAF, throttle,
  fuel, battery, pressures, profile, odometer). gRPC opcional. `/metrics`.

## Main Loop (50 ms)
driving cycle → RPM profile → sensor sim → env sim → fault injection
(`impl->setDTC`) → sync sensores al fabricante → odómetro
(`speed/3600*dt`, flush 0.01 km) → metrics gauges. Threads: CAN rx/tx,
REST, WS, gRPC.

## Vehicle Profiles
10 perfiles: `normal`, `unstable_idle`, `bad_o2`, `maf_fault`,
`coolant_fault`, `misfire`, `low_battery`, `fuel_pressure`, `emission_fail`,
`custom`. Via `VehicleProfileManager` (`BaseManufacturer::getProfileManager()`):
`selectProfile("bad_o2")`, `applyRPMProfile(base_rpm, dt)`,
`applySensorOverride(0x14, raw, dt)` (stuck/noise/drift/clamp). Switch por
API `POST /profile`; `simulation.profile` en config.json.

## Scripts
`scripts/install_deps.sh`, `scripts/setup_can.sh`, `scripts/build.sh` (-n -d -t),
`scripts/deploy.sh` (SSH a la Pi), `scripts/run_tests.sh`, `scripts/fuzz.sh`
(fuzzing OBD2/UDS), `scripts/diag.sh` (cliente CAN interactivo).

## Key Rules
- CAN frame data: `cf.frame.data` es `__u8 data[8]` → convertir a vector.
- Manufacturers heredan `BaseManufacturer`; implementan `handleCanFrame()` y
  `getManufacturerId()`.
- Nuevo fabricante: crear `src/manufacturers/<name>.{hpp,cpp}`, registrar en
  main.cpp y protocol_router.cpp.
- Cross: nlohmann/json auto-descargado a build/include/; sqlite3 debe estar
  en el sysroot.

See `docs/api.yaml` (OpenAPI 3.0), `examples/python_client.py`,
`examples/test_commands.sh`.
