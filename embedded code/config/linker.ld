/**
 * @file linker.ld
 * @brief Linker script for ARM Cortex-A7 bare-metal application
 */

/* Memory layout - adjust for target platform */
MEMORY
{
    ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 1M
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 256K
}

/* Entry point */
ENTRY(reset_handler)

SECTIONS
{
    /* Vector table at start of ROM */
    .vectors :
    {
        KEEP(*(.vectors))
    } > ROM

    /* Code section */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > ROM

    /* ARM exception handling */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > ROM

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > ROM

    /* Data section load address in ROM */
    __data_load = LOADADDR(.data);

    /* Initialized data in RAM */
    .data :
    {
        . = ALIGN(4);
        __data_start = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        __data_end = .;
    } > RAM AT > ROM

    /* Uninitialized data (BSS) */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

    /* Stack section */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        *(.stack)
        . = ALIGN(8);
    } > RAM

    /* Heap section */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __heap_start = .;
        . = . + 0x10000;  /* 64KB heap */
        __heap_end = .;
    } > RAM

    /* End of RAM */
    _end = .;
}
