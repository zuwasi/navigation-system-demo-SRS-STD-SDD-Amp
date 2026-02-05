# Navigation System Demo - Fixed-Wing Enhancements

A demonstration project featuring fixed-wing navigation and control enhancements for PX4-style architecture, including embedded ARM Cortex-A7 bare-metal code with BLE/I2C interfaces.

## Overview

This project implements three key navigation features for fixed-wing UAVs, derived from a commercialization process demo. The system integrates with navigator setpoints consumed by fixed-wing mode managers and lateral/longitudinal control modules.

## Features

### Feature A: Smart Landing Flare
Intelligent flare adjustment based on environmental conditions:
- Computes flare adjustment factor from wind estimate validity and terrain confidence
- Adjusts flare timing: earlier for strong headwind, delayed for low terrain confidence
- Bounded adjustment range: ±30% of nominal flare time
- Respects minimum flare altitude and time-to-touchdown thresholds
- Runtime enable/disable via parameter (disabled by default)

### Feature B: Mission Speed Limiter
Geofenced speed constraints for mission safety:
- Define speed limit zones using polygon lists
- Constrains groundspeed to configurable MAX_GS_LIMIT within zones
- Active in mission auto mode and loiter modes
- Automatically disabled during takeoff/landing phases
- Zone transition detection within 1 second

### Feature C: Energy-Aware Loiter
Battery-conscious loiter behavior:
- Detects low-energy state via battery/voltage thresholds
- Reduces loiter airspeed by configurable scale factor
- Reduces maximum bank angle for lateral acceleration limiting
- Smooth 2-second transition between modes
- Automatic reversion when energy state normalizes

## Project Structure

```
elbit-demo/
├── embedded code/         # ARM Cortex-A7 bare-metal implementation
│   ├── src/               # Main application and startup
│   ├── drivers/           # BLE and I2C drivers
│   ├── hal/               # Hardware Abstraction Layer (GIC)
│   ├── include/           # Type definitions and register maps
│   └── config/            # Linker script
├── SRS.md                 # Software Requirements Specification
├── SDD.md                 # Software Design Document
├── STD.md                 # Software Test Document
├── STR.md                 # Software Test Report
├── smart_landing_flare_sim.py  # Python simulation
└── README.md
```

## Embedded Code

The `embedded code/` directory contains MISRA C 2023 and CERT C compliant bare-metal code for ARM Cortex-A7:

- **No RTOS** - Uses while() loop and interrupt-driven architecture
- **BLE Interface** - Bluetooth Low Energy for wireless communication
- **I2C Interface** - Sensor integration (temperature, accelerometer)
- **2,059 lines** of C code across 9 files

### Building

```bash
cd "embedded code"
make all          # Build project
make misra-check  # Run MISRA C static analysis
make cert-check   # Run CERT C static analysis
```

## Requirements Summary

| ID | Requirement | Feature |
|----|-------------|---------|
| REQ-A-1 | Flare adjustment from wind/terrain | Smart Landing |
| REQ-A-4 | ±30% bounded adjustment | Smart Landing |
| REQ-B-1 | Geofenced speed zones | Speed Limiter |
| REQ-B-2 | Constrain to MAX_GS_LIMIT | Speed Limiter |
| REQ-C-1 | Low-energy detection | Energy Loiter |
| REQ-C-2 | Reduce loiter airspeed | Energy Loiter |

## Non-Functional Requirements

- Control loop overhead: <5% increase
- Speed limit evaluation: <2 ms per cycle
- Zone transition detection: <1 second
- Mode transitions: smoothed within 2 seconds

## Documentation

| Document | Description |
|----------|-------------|
| [SRS.md](SRS.md) | Software Requirements Specification |
| [SDD.md](SDD.md) | Software Design Document |
| [STD.md](STD.md) | Software Test Document |
| [STR.md](STR.md) | Software Test Report |

## License

Proprietary - Elbit Systems
