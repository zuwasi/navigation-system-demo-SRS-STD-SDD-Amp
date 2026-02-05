# Makefile for ARM Cortex-A7 Bare-Metal BLE/I2C Application
# MISRA C 2023 and CERT C compliant build configuration

# Toolchain
CROSS_COMPILE ?= arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size

# Project name
TARGET = arm_a7_ble_i2c

# Directories
SRC_DIR     = src
HAL_DIR     = hal
DRIVERS_DIR = drivers
INCLUDE_DIR = include
CONFIG_DIR  = config
BUILD_DIR   = build

# Source files
C_SOURCES = \
    $(SRC_DIR)/main.c \
    $(HAL_DIR)/hal_gic.c \
    $(DRIVERS_DIR)/i2c_driver.c \
    $(DRIVERS_DIR)/ble_driver.c

ASM_SOURCES = \
    $(SRC_DIR)/startup.s

# Include paths
INCLUDES = \
    -I$(INCLUDE_DIR) \
    -I$(HAL_DIR) \
    -I$(DRIVERS_DIR)

# Linker script
LDSCRIPT = $(CONFIG_DIR)/linker.ld

# Compiler flags for ARM Cortex-A7
CPU_FLAGS = -mcpu=cortex-a7 -marm -mfpu=neon-vfpv4 -mfloat-abi=hard

# MISRA C 2023 and CERT C compliant warning flags
WARNINGS = \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Werror \
    -Wstrict-prototypes \
    -Wmissing-prototypes \
    -Wconversion \
    -Wsign-conversion \
    -Wcast-align \
    -Wcast-qual \
    -Wdouble-promotion \
    -Wfloat-equal \
    -Wformat=2 \
    -Wformat-security \
    -Winit-self \
    -Wmissing-declarations \
    -Wpointer-arith \
    -Wredundant-decls \
    -Wshadow \
    -Wstrict-overflow=5 \
    -Wswitch-default \
    -Wswitch-enum \
    -Wundef \
    -Wuninitialized \
    -Wunused \
    -Wwrite-strings \
    -Wnested-externs \
    -Wold-style-definition \
    -Wbad-function-cast \
    -Wjump-misses-init \
    -Wlogical-op \
    -Wnull-dereference \
    -Wstack-protector \
    -Wvla

# C flags
CFLAGS = \
    $(CPU_FLAGS) \
    -std=c11 \
    -ffreestanding \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-stack-protector \
    -O2 \
    -g \
    $(WARNINGS) \
    $(INCLUDES)

# Assembler flags
ASFLAGS = $(CPU_FLAGS) -g

# Linker flags
LDFLAGS = \
    $(CPU_FLAGS) \
    -nostartfiles \
    -nostdlib \
    -T$(LDSCRIPT) \
    -Wl,--gc-sections \
    -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

# Object files
C_OBJECTS   = $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))
ASM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES:.s=.o))
OBJECTS     = $(C_OBJECTS) $(ASM_OBJECTS)

# Default target
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex
	@echo "Build complete!"
	@$(SIZE) $(BUILD_DIR)/$(TARGET).elf

# Create build directory (Windows compatible)
$(BUILD_DIR):
ifeq ($(OS),Windows_NT)
	if not exist $(BUILD_DIR)\$(SRC_DIR) mkdir $(BUILD_DIR)\$(SRC_DIR)
	if not exist $(BUILD_DIR)\$(HAL_DIR) mkdir $(BUILD_DIR)\$(HAL_DIR)
	if not exist $(BUILD_DIR)\$(DRIVERS_DIR) mkdir $(BUILD_DIR)\$(DRIVERS_DIR)
else
	mkdir -p $(BUILD_DIR)/$(SRC_DIR)
	mkdir -p $(BUILD_DIR)/$(HAL_DIR)
	mkdir -p $(BUILD_DIR)/$(DRIVERS_DIR)
endif

# Compile C sources
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble ASM sources
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ -lc -lgcc -lnosys

# Create binary
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Create hex file
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# Disassembly
disasm: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -d -S $< > $(BUILD_DIR)/$(TARGET).dis

# Clean (Windows compatible)
clean:
ifeq ($(OS),Windows_NT)
	if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
else
	rm -rf $(BUILD_DIR)
endif

# MISRA C static analysis (requires external tool like PC-lint, Parasoft, or cppcheck)
misra-check:
	@echo "Running MISRA C 2023 static analysis..."
	cppcheck --enable=all --std=c11 --addon=misra.json \
	    $(C_SOURCES) $(INCLUDES) 2>&1 | tee $(BUILD_DIR)/misra_report.txt

# CERT C static analysis
cert-check:
	@echo "Running CERT C static analysis..."
	cppcheck --enable=all --std=c11 --addon=cert.py \
	    $(C_SOURCES) $(INCLUDES) 2>&1 | tee $(BUILD_DIR)/cert_report.txt

# Format check
format-check:
	clang-format --dry-run --Werror $(C_SOURCES) $(wildcard $(INCLUDE_DIR)/*.h) \
	    $(wildcard $(HAL_DIR)/*.h) $(wildcard $(DRIVERS_DIR)/*.h)

.PHONY: all clean disasm misra-check cert-check format-check
