/**
 * @file startup.s
 * @brief ARM Cortex-A7 startup code and exception vectors
 * @note Bare-metal startup for interrupt-driven application
 */

    .syntax unified
    .arch armv7-a
    .arm

/* Stack sizes */
    .equ    SVC_STACK_SIZE,     0x1000
    .equ    IRQ_STACK_SIZE,     0x1000
    .equ    FIQ_STACK_SIZE,     0x400
    .equ    ABT_STACK_SIZE,     0x400
    .equ    UND_STACK_SIZE,     0x400
    .equ    SYS_STACK_SIZE,     0x2000

/* Processor modes */
    .equ    MODE_USR,   0x10
    .equ    MODE_FIQ,   0x11
    .equ    MODE_IRQ,   0x12
    .equ    MODE_SVC,   0x13
    .equ    MODE_ABT,   0x17
    .equ    MODE_UND,   0x1B
    .equ    MODE_SYS,   0x1F
    .equ    I_BIT,      0x80
    .equ    F_BIT,      0x40

/* Vector table - placed at 0x00000000 or 0xFFFF0000 */
    .section .vectors, "ax"
    .global _vector_table
_vector_table:
    ldr     pc, =reset_handler          /* Reset */
    ldr     pc, =undefined_handler      /* Undefined Instruction */
    ldr     pc, =svc_handler            /* Supervisor Call */
    ldr     pc, =prefetch_abort_handler /* Prefetch Abort */
    ldr     pc, =data_abort_handler     /* Data Abort */
    .word   0                           /* Reserved */
    ldr     pc, =irq_handler_asm        /* IRQ */
    ldr     pc, =fiq_handler            /* FIQ */

/* Startup code */
    .section .text
    .global reset_handler
    .type reset_handler, %function
reset_handler:
    /* Disable interrupts */
    cpsid   if

    /* Initialize stack pointers for all modes */
    ldr     r0, =_stack_top

    /* FIQ mode stack */
    msr     cpsr_c, #(MODE_FIQ | I_BIT | F_BIT)
    mov     sp, r0
    sub     r0, r0, #FIQ_STACK_SIZE

    /* IRQ mode stack */
    msr     cpsr_c, #(MODE_IRQ | I_BIT | F_BIT)
    mov     sp, r0
    sub     r0, r0, #IRQ_STACK_SIZE

    /* Abort mode stack */
    msr     cpsr_c, #(MODE_ABT | I_BIT | F_BIT)
    mov     sp, r0
    sub     r0, r0, #ABT_STACK_SIZE

    /* Undefined mode stack */
    msr     cpsr_c, #(MODE_UND | I_BIT | F_BIT)
    mov     sp, r0
    sub     r0, r0, #UND_STACK_SIZE

    /* SVC mode stack */
    msr     cpsr_c, #(MODE_SVC | I_BIT | F_BIT)
    mov     sp, r0
    sub     r0, r0, #SVC_STACK_SIZE

    /* System mode stack (used by main) */
    msr     cpsr_c, #(MODE_SYS | I_BIT | F_BIT)
    mov     sp, r0

    /* Clear BSS section */
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
    mov     r2, #0
bss_clear_loop:
    cmp     r0, r1
    strlt   r2, [r0], #4
    blt     bss_clear_loop

    /* Copy initialized data from ROM to RAM */
    ldr     r0, =__data_load
    ldr     r1, =__data_start
    ldr     r2, =__data_end
data_copy_loop:
    cmp     r1, r2
    ldrlt   r3, [r0], #4
    strlt   r3, [r1], #4
    blt     data_copy_loop

    /* Enable VFP/NEON if present */
    mrc     p15, 0, r0, c1, c0, 2       /* Read CPACR */
    orr     r0, r0, #(0xF << 20)        /* Enable CP10 and CP11 */
    mcr     p15, 0, r0, c1, c0, 2
    isb
    mov     r0, #(1 << 30)              /* Enable VFP */
    vmsr    fpexc, r0

    /* Stay in SVC mode for main() */
    msr     cpsr_c, #(MODE_SVC | I_BIT | F_BIT)

    /* Call main */
    bl      main

    /* Infinite loop if main returns */
halt_loop:
    wfi
    b       halt_loop

/* IRQ handler wrapper */
    .global irq_handler_asm
    .type irq_handler_asm, %function
irq_handler_asm:
    /* Save context */
    sub     lr, lr, #4              /* Adjust return address */
    push    {r0-r12, lr}            /* Save registers */
    mrs     r0, spsr                /* Save SPSR */
    push    {r0}

    /* Call C IRQ handler */
    bl      irq_handler

    /* Restore context */
    pop     {r0}
    msr     spsr_cxsf, r0           /* Restore SPSR */
    ldmfd   sp!, {r0-r12, pc}^      /* Restore and return */

/* Exception handlers */
    .type undefined_handler, %function
undefined_handler:
    b       undefined_handler

    .type svc_handler, %function
svc_handler:
    b       svc_handler

    .type prefetch_abort_handler, %function
prefetch_abort_handler:
    b       prefetch_abort_handler

    .type data_abort_handler, %function
data_abort_handler:
    b       data_abort_handler

    .type fiq_handler, %function
fiq_handler:
    b       fiq_handler

/* Stack section */
    .section .stack, "aw", %nobits
    .align 4
    .space  SVC_STACK_SIZE + IRQ_STACK_SIZE + FIQ_STACK_SIZE + ABT_STACK_SIZE + UND_STACK_SIZE + SYS_STACK_SIZE
    .global _stack_top
_stack_top:

    .end
