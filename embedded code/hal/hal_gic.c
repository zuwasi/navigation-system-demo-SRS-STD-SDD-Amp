/**
 * @file hal_gic.c
 * @brief Generic Interrupt Controller HAL implementation
 * @note MISRA C 2023 and CERT C compliant
 */

#include "hal_gic.h"
#include "arm_a7_regs.h"

/* Maximum IRQ number supported */
#define MAX_IRQ_NUM         (256U)
#define IRQS_PER_REGISTER   (32U)

/**
 * @brief Initialize the GIC distributor and CPU interface
 */
status_t gic_init(void)
{
    /* Disable distributor during setup */
    GICD_CTLR = 0U;
    
    /* Set all interrupts to lowest priority */
    for (u32_t i = 0U; i < (MAX_IRQ_NUM / 4U); i++)
    {
        GICD_IPRIORITYR(i) = 0xFFFFFFFFU;
    }
    
    /* Target all SPIs to CPU0 */
    for (u32_t i = 8U; i < (MAX_IRQ_NUM / 4U); i++)
    {
        GICD_ITARGETSR(i) = 0x01010101U;
    }
    
    /* Configure all as level-triggered */
    for (u32_t i = 2U; i < (MAX_IRQ_NUM / 16U); i++)
    {
        GICD_ICFGR(i) = 0U;
    }
    
    /* Enable distributor */
    GICD_CTLR = 1U;
    
    /* Configure CPU interface */
    GICC_PMR = 0xFFU;   /* Allow all priority levels */
    GICC_CTLR = 1U;     /* Enable CPU interface */
    
    DSB();
    ISB();
    
    return STATUS_OK;
}

/**
 * @brief Enable a specific interrupt
 */
status_t gic_enable_irq(u32_t irq_num)
{
    status_t result;
    
    if (irq_num >= MAX_IRQ_NUM)
    {
        result = STATUS_INVALID_PARAM;
    }
    else
    {
        u32_t reg_idx = irq_num / IRQS_PER_REGISTER;
        u32_t bit_pos = irq_num % IRQS_PER_REGISTER;
        
        GICD_ISENABLER(reg_idx) = (1U << bit_pos);
        DSB();
        
        result = STATUS_OK;
    }
    
    return result;
}

/**
 * @brief Disable a specific interrupt
 */
status_t gic_disable_irq(u32_t irq_num)
{
    status_t result;
    
    if (irq_num >= MAX_IRQ_NUM)
    {
        result = STATUS_INVALID_PARAM;
    }
    else
    {
        u32_t reg_idx = irq_num / IRQS_PER_REGISTER;
        u32_t bit_pos = irq_num % IRQS_PER_REGISTER;
        
        GICD_ICENABLER(reg_idx) = (1U << bit_pos);
        DSB();
        
        result = STATUS_OK;
    }
    
    return result;
}

/**
 * @brief Set interrupt priority
 */
status_t gic_set_priority(u32_t irq_num, u8_t priority)
{
    status_t result;
    
    if (irq_num >= MAX_IRQ_NUM)
    {
        result = STATUS_INVALID_PARAM;
    }
    else
    {
        u32_t reg_idx = irq_num / 4U;
        u32_t byte_offset = (irq_num % 4U) * 8U;
        u32_t reg_val = GICD_IPRIORITYR(reg_idx);
        
        reg_val &= ~(0xFFU << byte_offset);
        reg_val |= ((u32_t)priority << byte_offset);
        
        GICD_IPRIORITYR(reg_idx) = reg_val;
        
        result = STATUS_OK;
    }
    
    return result;
}

/**
 * @brief Acknowledge interrupt and get IRQ number
 */
u32_t gic_acknowledge_irq(void)
{
    return (GICC_IAR & 0x3FFU);
}

/**
 * @brief Signal end of interrupt handling
 */
void gic_end_of_irq(u32_t irq_num)
{
    GICC_EOIR = irq_num;
    DSB();
}

/**
 * @brief Enable IRQ globally (CPSR)
 */
void cpu_enable_irq(void)
{
    __asm__ volatile (
        "cpsie i"
        :
        :
        : "memory"
    );
}

/**
 * @brief Disable IRQ globally (CPSR)
 */
void cpu_disable_irq(void)
{
    __asm__ volatile (
        "cpsid i"
        :
        :
        : "memory"
    );
}
