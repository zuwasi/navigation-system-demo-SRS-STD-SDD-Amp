# Navigation System Demo Repository

This repository contains two separate demo projects for fixed-wing UAV navigation systems.

## Repository Structure

```
├── elbit-demo/              # Elbit Demo - ARM Cortex-A7 Embedded Code
│   ├── embedded code/       # MISRA C 2023 / CERT C compliant bare-metal code
│   ├── SRS.md               # Software Requirements Specification
│   ├── SDD.md               # Software Design Document
│   ├── STD.md               # Software Test Document
│   ├── STR.md               # Software Test Report
│   └── smart_landing_flare_sim.py
│
└── PX4-navigation-demo/     # PX4 Navigation Demo - Full Autopilot Stack
    ├── src/                 # PX4 source modules
    ├── boards/              # Board configurations
    ├── platforms/           # Platform support
    └── ...
```

## Projects

### 1. Elbit Demo (`elbit-demo/`)

ARM Cortex-A7 bare-metal embedded code with BLE and I2C interfaces. Features:

- **Smart Landing Flare** - Flare adjustment from wind/terrain conditions
- **Mission Speed Limiter** - Geofenced speed constraints
- **Energy-Aware Loiter** - Battery-conscious loiter behavior
- MISRA C 2023 and CERT C compliant
- 2,059 lines of C code

### 2. PX4 Navigation Demo (`PX4-navigation-demo/`)

Full PX4 autopilot stack for fixed-wing navigation enhancements. Based on PX4-Autopilot with custom navigation modules.

## Documentation

- See `elbit-demo/SRS.md` for requirements specification
- See `elbit-demo/SDD.md` for design documentation
- See `elbit-demo/STD.md` for test documentation

## License

Proprietary - Elbit Systems
