# ARM Cortex-A7 Bare-Metal BLE/I2C Application

A MISRA C 2023 and CERT C compliant bare-metal embedded application for ARM Cortex-A7 processors with BLE and I2C interfaces.

## Features

- **Bare-metal** - No RTOS, uses `while()` loop and interrupt-driven architecture
- **BLE Interface** - Bluetooth Low Energy driver with advertising, scanning, and data transfer
- **I2C Interface** - I2C master driver with blocking and async (interrupt-driven) modes
- **MISRA C 2023 Compliant** - Follows MISRA C 2023 guidelines
- **CERT C Compliant** - Follows SEI CERT C Coding Standard

## Project Structure

```
embedded code/
├── .vscode/              # VSCode configuration
│   ├── c_cpp_properties.json
│   ├── settings.json
│   └── tasks.json
├── config/
│   └── linker.ld         # Linker script
├── drivers/
│   ├── ble_driver.c      # BLE driver implementation
│   ├── ble_driver.h
│   ├── i2c_driver.c      # I2C driver implementation
│   └── i2c_driver.h
├── hal/
│   ├── hal_gic.c         # GIC (Interrupt Controller) HAL
│   └── hal_gic.h
├── include/
│   ├── arm_a7_regs.h     # ARM Cortex-A7 register definitions
│   └── types.h           # MISRA-compliant type definitions
├── src/
│   ├── main.c            # Main application
│   └── startup.s         # Startup code and vector table
├── Makefile              # Build system
└── README.md
```

## Building

### Prerequisites

- ARM GNU Toolchain (`arm-none-eabi-gcc`)
- Make

### Build Commands

```bash
# Build the project
make all

# Clean build artifacts
make clean

# Generate disassembly
make disasm

# Run MISRA C static analysis (requires cppcheck with MISRA addon)
make misra-check

# Run CERT C static analysis
make cert-check
```

## MISRA C 2023 Compliance

Key compliance features:

- **Rule 4.6**: Fixed-width integer types used throughout
- **Rule 10.1**: Enums used for related constants
- **Rule 14.4**: Boolean expressions for control flow
- **Rule 15.5**: Single exit point pattern in functions
- **Rule 17.7**: Return values checked and handled

## CERT C Compliance

Key compliance features:

- **EXP34-C**: Null pointer validation before dereference
- **INT30-C**: Unsigned integer operations checked for wraparound
- **ARR30-C**: Array bounds validation
- **MEM30-C**: Proper memory handling

## Architecture

### Main Loop Pattern

```c
while (app_state != ERROR)
{
    ble_process();           // Process BLE events
    handle_sensor_data();    // Handle I2C sensor data
    state_machine();         // Application state machine
}
```

### Interrupt Handling

- GIC (Generic Interrupt Controller) manages all interrupts
- IRQ handler in startup.s saves context and calls C handler
- Drivers provide IRQ handler functions called from main IRQ handler

## Hardware Adaptation

Adjust the following for your specific platform:

1. **Base Addresses** in `include/arm_a7_regs.h`:
   - `PERIPH_BASE`, `GIC_BASE`, `I2C0_BASE`, etc.

2. **Memory Layout** in `config/linker.ld`:
   - ROM and RAM origins and lengths

3. **Clock Configuration** in drivers:
   - `SYSTEM_CLOCK_HZ` constant

## License

Proprietary - Elbit Systems
