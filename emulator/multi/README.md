# ECU Multi-Emulator v1.0.0

Multi-brand vehicle diagnostic ECU emulator for **Raspberry Pi + MCP2515**. Written in **C++17**, supports **8 manufacturers** with full **OBD2 + UDS + manufacturer-specific protocols**.

---

## Features

- **8 Manufacturers**: Chevrolet/GM, Ford, Toyota, BMW, Volkswagen/Audi, Mercedes-Benz, Honda, Nissan (+Hyundai/Kia)
- **Full OBD2**: Modes 01–0A with 50+ PIDs, DTCs, freeze frames, readiness monitors
- **UDS (ISO 14229)**: Diagnostic sessions, security access, DID read/write, ECU reset, routine control, upload/download
- **Manufacturer Protocols**: GMLAN Mode 22 (GM), Ford MS-CAN/PATS, Toyota Mode 21/22, BMW DCAN/EDIABAS, VW Adaptation/Long Coding, Mercedes Star/DAS, Honda HDS, Nissan Consult III
- **REST API**: Full HTTP API on port 8080
- **WebSocket**: Real-time sensor streaming on port 8081
- **CAN via SocketCAN**: Physical CAN or virtual (vcan)
- **Driving Simulation**: Urban/highway/mixed cycles with realistic sensor correlation
- **Fault Injection**: Programmatic DTC injection with configurable probability
- **Seed/Key Security**: Manufacturer-specific algorithms (GM 28/40-bit, Ford, Toyota, BMW, VW, Mercedes, Honda, Nissan)
- **SQLite3 Database**: Persistent storage for vehicles, DTCs, parameters, logs
- **Prometheus Metrics**: /metrics endpoint
- **Logging**: Rotating file logger with levels

---

## Directory Structure

```
├── core/               # CAN manager, protocol router, session manager, security
├── manufacturers/      # Base class + 8 manufacturer implementations
├── protocols/          # OBD2, UDS, GMLAN, KWP2000, CAN-TP
├── database/           # SQLite3 wrapper, migrations, schema, seed data
├── diagnostics/        # DTC manager, freeze frame, readiness, VIN, calibration
├── simulation/         # Driving cycles, sensor simulator, fault injector, environment
├── api/                # REST API, WebSocket server, gRPC server (optional)
├── security/           # Seed/key algorithms, access control, secure comm
├── logging/            # CAN logger, metrics exporter, replay analyzer
├── tests/              # Catch2 unit tests, manufacturer tests, fuzzing suite
├── docker/             # Dockerfile + entrypoint
├── docs/               # OpenAPI 3.0 specification
├── examples/           # Python client, shell test commands
├── CMakeLists.txt      # Build system
├── config.json         # Emulator configuration
└── main.cpp            # Entry point with threads for CAN, simulation, API
```

---

## Quick Start

### 1. Install Dependencies

```bash
# Debian/Raspberry Pi OS
sudo apt update
sudo apt install -y build-essential cmake git libsqlite3-dev \
    libnl-3-dev libnl-route-3-dev nlohmann-json3-dev \
    pkg-config can-utils iproute2

# Or use the install script
sudo ./install_multi_emulator.sh
```

### 2. Configure SocketCAN

```bash
# Real MCP2515 hardware
sudo ip link set can0 type can bitrate 500000 triple-sampling on
sudo ip link set can0 up

# OR virtual CAN (for testing without hardware)
sudo modprobe vcan
sudo ip link add vcan0 type vcan
sudo ip link set vcan0 up
# Then edit config.json: "interface": "vcan0"
```

### 3. Build

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### 4. Run

```bash
./ecu_emulator           # uses config.json from current directory
./ecu_emulator /path/to/config.json
```

### 5. Test

```bash
# API test
curl http://localhost:8080/vehicles

# CAN test (OBD2 Mode 01, RPM)
cansend can0 7DF#02010C0000000000

# Switch manufacturer via API
curl -X POST http://localhost:8080/select \
  -H "Content-Type: application/json" \
  -d '{"manufacturer": "toyota", "model": "Camry"}'
```

---

## API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | /vehicles | List all supported manufacturers/models |
| POST | /select | Select active manufacturer |
| GET | /current | Current state (sensors, DTCs, session) |
| POST | /inject_fault | Inject a DTC |
| POST | /clear_faults | Clear all DTCs |
| POST | /set_parameter | Set simulation parameter value |
| GET | /diagnostics | Full diagnostic report |
| POST | /start_recording | Record CAN traffic to file |
| POST | /replay | Replay recorded CAN traffic |
| GET | /metrics | Prometheus metrics |
| POST | /fuzz_start | Start CAN fuzzing |
| GET | /logs | Recent log entries |

See [docs/api.yaml](docs/api.yaml) for OpenAPI 3.0 specification.

---

## Manufacturer Protocols

| Manufacturer | Protocol | CAN IDs | Special Features |
|-------------|----------|---------|-----------------|
| Chevrolet/GM | GMLAN | 0x7E0/0x7E8 | Mode 22 DID, 28/40-bit seed/key |
| Ford | MS-CAN/HS-CAN | 0x7E0/0x7E8 | PATS, enhanced PIDs |
| Toyota | Mode 21/22 | 0x7E0/0x7E8 | Smart key, immobilizer |
| BMW | UDS/DCAN | 0x7E0/0x7E8 | EDIABAS jobs, FRM/CAS |
| VW/Audi | UDS | 0x7E0/0x7E8 | Adaptation, long coding |
| Mercedes | UDS | 0x7E0/0x7E8 | DAS, SBC, SAM modules |
| Honda | HDS | 0x7E0/0x7E8 | Immobilizer, i-VTEC |
| Nissan | Consult III | 0x7E0/0x7E8 | NATS, BCM |

---

## Test Commands

```bash
# Run the test script
./examples/test_commands.sh

# Or use the Python client
pip3 install requests
python3 examples/python_client.py

# Run unit tests (requires Catch2)
cd build && ctest
./ecu_tests
```

---

## Docker

```bash
# Build
docker build -t ecu-emulator -f docker/Dockerfile .

# Run with virtual CAN
docker run --rm --privileged \
  -e CREATE_VCAN=true \
  -p 8080:8080 -p 8081:8081 \
  ecu-emulator
```

---

## Configuration

Edit `config.json`:

- **can.interface**: SocketCAN interface (can0, vcan0)
- **can.bitrate**: CAN bus speed (default: 500000)
- **active_manufacturer**: Initial manufacturer
- **manufacturers.*.enabled**: Enable/disable manufacturers
- **simulation.driving_cycle**: urban/highway/mixed/steady/custom
- **api.rest.port**: REST API port (default: 8080)
- **api.websocket.port**: WebSocket port (default: 8081)
- **logging.level**: trace/debug/info/warning/error/critical
- **security.seed_key_enabled**: Enable seed/key security
- **diagnostics.default_dtcs**: Pre-populated DTCs

---

## Integration with FreeBUF/OpenCode

Two integration modes:

### 1. HTTP API (recommended)

Python tools in FreeBUF/OpenCode communicate with the C++ emulator via REST API + WebSocket.

```python
import requests
r = requests.post("http://localhost:8080/select",
                  json={"manufacturer": "bmw"})
```

### 2. Native C++ bindings (pybind11)

Build the emulator as a shared library and create Python bindings:

```bash
# In CMakeLists.txt, add:
add_library(ecu_emulator_core SHARED ${SOURCES})
# Then use pybind11 to generate Python bindings
```

See `examples/python_client.py` for a complete reference.

---

## License

MIT
