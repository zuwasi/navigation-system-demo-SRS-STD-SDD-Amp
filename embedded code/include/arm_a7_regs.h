/**
 * @file arm_a7_regs.h
 * @brief ARM Cortex-A7 register definitions
 * @note MISRA C 2023 and CERT C compliant
 */

#ifndef ARM_A7_REGS_H
#define ARM_A7_REGS_H

#include "types.h"

/* Base addresses - Platform specific (example for typical SoC) */
#define PERIPH_BASE             (0x40000000U)
#define I2C0_BASE               (PERIPH_BASE + 0x00005000U)
#define I2C1_BASE               (PERIPH_BASE + 0x00005400U)
#define UART0_BASE              (PERIPH_BASE + 0x0000C000U)
#define GPIO_BASE               (PERIPH_BASE + 0x00006000U)
#define GIC_BASE                (0x2C000000U)

/* Generic Interrupt Controller (GIC) registers */
#define GIC_DIST_BASE           (GIC_BASE + 0x1000U)
#define GIC_CPU_BASE            (GIC_BASE + 0x2000U)

#define GICD_CTLR               (*((vu32_t *)(GIC_DIST_BASE + 0x000U)))
#define GICD_ISENABLER(n)       (*((vu32_t *)(GIC_DIST_BASE + 0x100U + ((n) * 4U))))
#define GICD_ICENABLER(n)       (*((vu32_t *)(GIC_DIST_BASE + 0x180U + ((n) * 4U))))
#define GICD_IPRIORITYR(n)      (*((vu32_t *)(GIC_DIST_BASE + 0x400U + ((n) * 4U))))
#define GICD_ITARGETSR(n)       (*((vu32_t *)(GIC_DIST_BASE + 0x800U + ((n) * 4U))))
#define GICD_ICFGR(n)           (*((vu32_t *)(GIC_DIST_BASE + 0xC00U + ((n) * 4U))))

#define GICC_CTLR               (*((vu32_t *)(GIC_CPU_BASE + 0x000U)))
#define GICC_PMR                (*((vu32_t *)(GIC_CPU_BASE + 0x004U)))
#define GICC_IAR                (*((vu32_t *)(GIC_CPU_BASE + 0x00CU)))
#define GICC_EOIR               (*((vu32_t *)(GIC_CPU_BASE + 0x010U)))

/* I2C Register Structure */
typedef struct {
    vu32_t CR1;         /* Control register 1          */
    vu32_t CR2;         /* Control register 2          */
    vu32_t OAR1;        /* Own address register 1      */
    vu32_t OAR2;        /* Own address register 2      */
    vu32_t DR;          /* Data register               */
    vu32_t SR1;         /* Status register 1           */
    vu32_t SR2;         /* Status register 2           */
    vu32_t CCR;         /* Clock control register      */
    vu32_t TRISE;       /* TRISE register              */
} i2c_regs_t;

/* I2C register bit definitions */
#define I2C_CR1_PE              (0U)
#define I2C_CR1_START           (8U)
#define I2C_CR1_STOP            (9U)
#define I2C_CR1_ACK             (10U)
#define I2C_CR1_SWRST           (15U)

#define I2C_SR1_SB              (0U)
#define I2C_SR1_ADDR            (1U)
#define I2C_SR1_BTF             (2U)
#define I2C_SR1_RXNE            (6U)
#define I2C_SR1_TXE             (7U)
#define I2C_SR1_AF              (10U)

#define I2C_SR2_BUSY            (1U)
#define I2C_SR2_MSL             (0U)

/* GPIO Register Structure */
typedef struct {
    vu32_t MODER;       /* Mode register               */
    vu32_t OTYPER;      /* Output type register        */
    vu32_t OSPEEDR;     /* Output speed register       */
    vu32_t PUPDR;       /* Pull-up/pull-down register  */
    vu32_t IDR;         /* Input data register         */
    vu32_t ODR;         /* Output data register        */
    vu32_t BSRR;        /* Bit set/reset register      */
    vu32_t LCKR;        /* Lock register               */
    vu32_t AFRL;        /* Alternate function low      */
    vu32_t AFRH;        /* Alternate function high     */
} gpio_regs_t;

/* Interrupt numbers */
#define IRQ_I2C0                (23U)
#define IRQ_I2C1                (24U)
#define IRQ_BLE                 (48U)
#define IRQ_TIMER0              (29U)

/* Pointer macros for register access */
#define I2C0                    ((i2c_regs_t *)I2C0_BASE)
#define I2C1                    ((i2c_regs_t *)I2C1_BASE)
#define GPIO                    ((gpio_regs_t *)GPIO_BASE)

#endif /* ARM_A7_REGS_H */
