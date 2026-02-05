/**
 * @file hal_gic.h
 * @brief Generic Interrupt Controller HAL for ARM Cortex-A7
 * @note MISRA C 2023 and CERT C compliant
 */

#ifndef HAL_GIC_H
#define HAL_GIC_H

#include "types.h"

/**
 * @brief Initialize the GIC distributor and CPU interface
 * @return STATUS_OK on success
 */
status_t gic_init(void);

/**
 * @brief Enable a specific interrupt
 * @param[in] irq_num Interrupt number (0-255)
 * @return STATUS_OK on success, STATUS_INVALID_PARAM on invalid IRQ
 */
status_t gic_enable_irq(u32_t irq_num);

/**
 * @brief Disable a specific interrupt
 * @param[in] irq_num Interrupt number (0-255)
 * @return STATUS_OK on success, STATUS_INVALID_PARAM on invalid IRQ
 */
status_t gic_disable_irq(u32_t irq_num);

/**
 * @brief Set interrupt priority
 * @param[in] irq_num Interrupt number
 * @param[in] priority Priority value (0-255, lower = higher priority)
 * @return STATUS_OK on success
 */
status_t gic_set_priority(u32_t irq_num, u8_t priority);

/**
 * @brief Acknowledge interrupt and get IRQ number
 * @return IRQ number of the pending interrupt
 */
u32_t gic_acknowledge_irq(void);

/**
 * @brief Signal end of interrupt handling
 * @param[in] irq_num Interrupt number to signal complete
 */
void gic_end_of_irq(u32_t irq_num);

/**
 * @brief Enable IRQ globally (CPSR)
 */
void cpu_enable_irq(void);

/**
 * @brief Disable IRQ globally (CPSR)
 */
void cpu_disable_irq(void);

#endif /* HAL_GIC_H */
