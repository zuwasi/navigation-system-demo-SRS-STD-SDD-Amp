/**
 * @file i2c_driver.c
 * @brief I2C driver implementation for ARM Cortex-A7
 * @note MISRA C 2023 and CERT C compliant - bare-metal interrupt-driven
 */

#include "i2c_driver.h"
#include "arm_a7_regs.h"
#include "hal_gic.h"

/* System clock frequency (example: 100MHz) */
#define SYSTEM_CLOCK_HZ     (100000000U)

/* Timeout counter base */
#define TIMEOUT_LOOP_COUNT  (10000U)

/* I2C handle structure */
typedef struct {
    i2c_regs_t      *regs;
    i2c_state_t      state;
    u8_t            *rx_buffer;
    const u8_t      *tx_buffer;
    u32_t            buffer_len;
    u32_t            buffer_idx;
    u8_t             dev_address;
    i2c_callback_t   callback;
    bool             initialized;
} i2c_handle_t;

/* Static handles for each I2C instance */
static i2c_handle_t g_i2c_handles[I2C_INSTANCE_MAX] = {
    { .regs = I2C0, .state = I2C_STATE_IDLE, .initialized = false },
    { .regs = I2C1, .state = I2C_STATE_IDLE, .initialized = false }
};

/* Forward declarations for internal functions */
static status_t i2c_wait_flag(const i2c_regs_t *regs, u32_t flag, 
                               bool expected, u32_t timeout);
static void i2c_generate_start(i2c_regs_t *regs);
static void i2c_generate_stop(i2c_regs_t *regs);
static status_t i2c_send_address(i2c_regs_t *regs, u8_t addr, 
                                  i2c_direction_t dir, u32_t timeout);

/**
 * @brief Initialize I2C peripheral
 */
status_t i2c_init(i2c_instance_t instance, const i2c_config_t *config)
{
    status_t result = STATUS_OK;
    
    /* CERT C EXP34-C: Validate pointer before dereference */
    if (IS_NULL_PTR(config))
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (instance >= I2C_INSTANCE_MAX)
    {
        result = STATUS_INVALID_PARAM;
    }
    else
    {
        i2c_handle_t *handle = &g_i2c_handles[instance];
        i2c_regs_t *regs = handle->regs;
        
        /* Reset I2C peripheral */
        BIT_SET(regs->CR1, I2C_CR1_SWRST);
        BIT_CLEAR(regs->CR1, I2C_CR1_SWRST);
        
        /* Configure clock - CR2 holds peripheral clock in MHz */
        u32_t pclk_mhz = SYSTEM_CLOCK_HZ / 1000000U;
        regs->CR2 = pclk_mhz & 0x3FU;
        
        /* Calculate CCR value for I2C clock speed */
        u32_t ccr_val;
        if (config->clock_speed <= 100000U)
        {
            /* Standard mode */
            ccr_val = SYSTEM_CLOCK_HZ / (config->clock_speed * 2U);
        }
        else
        {
            /* Fast mode (Duty cycle 2:1) */
            ccr_val = SYSTEM_CLOCK_HZ / (config->clock_speed * 3U);
            ccr_val |= 0x8000U; /* Fast mode bit */
        }
        regs->CCR = ccr_val;
        
        /* Configure TRISE */
        if (config->clock_speed <= 100000U)
        {
            regs->TRISE = pclk_mhz + 1U;
        }
        else
        {
            regs->TRISE = ((pclk_mhz * 300U) / 1000U) + 1U;
        }
        
        /* Set own address if specified */
        if (config->own_address != 0U)
        {
            regs->OAR1 = ((u32_t)config->own_address << 1U) | 0x4000U;
        }
        
        /* Enable I2C */
        BIT_SET(regs->CR1, I2C_CR1_PE);
        BIT_SET(regs->CR1, I2C_CR1_ACK);
        
        /* Enable interrupts if requested */
        if (config->use_interrupts)
        {
            u32_t irq_num = (instance == I2C_INSTANCE_0) ? IRQ_I2C0 : IRQ_I2C1;
            (void)gic_set_priority(irq_num, 0x80U);
            (void)gic_enable_irq(irq_num);
            
            /* Enable I2C event and buffer interrupts */
            regs->CR2 |= (1U << 9U) | (1U << 10U);  /* ITEVTEN | ITBUFEN */
        }
        
        handle->initialized = true;
        handle->state = I2C_STATE_IDLE;
    }
    
    return result;
}

/**
 * @brief Deinitialize I2C peripheral
 */
status_t i2c_deinit(i2c_instance_t instance)
{
    status_t result = STATUS_OK;
    
    if (instance >= I2C_INSTANCE_MAX)
    {
        result = STATUS_INVALID_PARAM;
    }
    else
    {
        i2c_handle_t *handle = &g_i2c_handles[instance];
        
        /* Disable I2C */
        BIT_CLEAR(handle->regs->CR1, I2C_CR1_PE);
        
        /* Disable interrupts */
        u32_t irq_num = (instance == I2C_INSTANCE_0) ? IRQ_I2C0 : IRQ_I2C1;
        (void)gic_disable_irq(irq_num);
        
        handle->initialized = false;
        handle->state = I2C_STATE_IDLE;
    }
    
    return result;
}

/**
 * @brief Wait for a flag with timeout
 */
static status_t i2c_wait_flag(const i2c_regs_t *regs, u32_t flag, 
                               bool expected, u32_t timeout)
{
    status_t result = STATUS_TIMEOUT;
    u32_t counter = timeout * TIMEOUT_LOOP_COUNT;
    
    while (counter > 0U)
    {
        bool flag_state = BIT_CHECK(regs->SR1, flag);
        if (flag_state == expected)
        {
            result = STATUS_OK;
            break;
        }
        counter--;
    }
    
    return result;
}

/**
 * @brief Generate START condition
 */
static void i2c_generate_start(i2c_regs_t *regs)
{
    BIT_SET(regs->CR1, I2C_CR1_START);
}

/**
 * @brief Generate STOP condition
 */
static void i2c_generate_stop(i2c_regs_t *regs)
{
    BIT_SET(regs->CR1, I2C_CR1_STOP);
}

/**
 * @brief Send address with direction
 */
static status_t i2c_send_address(i2c_regs_t *regs, u8_t addr, 
                                  i2c_direction_t dir, u32_t timeout)
{
    status_t result;
    
    /* Wait for START to be sent */
    result = i2c_wait_flag(regs, I2C_SR1_SB, true, timeout);
    
    if (result == STATUS_OK)
    {
        /* Send address with R/W bit */
        u8_t addr_byte = (u8_t)((addr << 1U) | (u8_t)dir);
        regs->DR = addr_byte;
        
        /* Wait for address to be acknowledged */
        result = i2c_wait_flag(regs, I2C_SR1_ADDR, true, timeout);
        
        if (result == STATUS_OK)
        {
            /* Clear ADDR flag by reading SR1 and SR2 */
            (void)regs->SR1;
            (void)regs->SR2;
        }
    }
    
    return result;
}

/**
 * @brief Write data to I2C device (blocking)
 */
status_t i2c_write_blocking(i2c_instance_t instance, u8_t dev_addr,
                            const u8_t *data, u32_t len, u32_t timeout_ms)
{
    status_t result = STATUS_OK;
    
    /* CERT C EXP34-C: Validate pointer */
    if (IS_NULL_PTR(data))
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (instance >= I2C_INSTANCE_MAX)
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (!g_i2c_handles[instance].initialized)
    {
        result = STATUS_NOT_READY;
    }
    else if (len == 0U)
    {
        result = STATUS_INVALID_PARAM;
    }
    else
    {
        i2c_handle_t *handle = &g_i2c_handles[instance];
        i2c_regs_t *regs = handle->regs;
        
        handle->state = I2C_STATE_BUSY_TX;
        
        /* Wait for bus to be free */
        u32_t wait_count = timeout_ms * TIMEOUT_LOOP_COUNT;
        while (BIT_CHECK(regs->SR2, I2C_SR2_BUSY) && (wait_count > 0U))
        {
            wait_count--;
        }
        
        if (wait_count == 0U)
        {
            result = STATUS_BUSY;
        }
        else
        {
            /* Generate START */
            i2c_generate_start(regs);
            
            /* Send address */
            result = i2c_send_address(regs, dev_addr, I2C_DIR_WRITE, timeout_ms);
            
            if (result == STATUS_OK)
            {
                /* Transmit data */
                for (u32_t i = 0U; (i < len) && (result == STATUS_OK); i++)
                {
                    result = i2c_wait_flag(regs, I2C_SR1_TXE, true, timeout_ms);
                    if (result == STATUS_OK)
                    {
                        regs->DR = data[i];
                    }
                }
                
                /* Wait for BTF (Byte Transfer Finished) */
                if (result == STATUS_OK)
                {
                    result = i2c_wait_flag(regs, I2C_SR1_BTF, true, timeout_ms);
                }
            }
            
            /* Generate STOP */
            i2c_generate_stop(regs);
        }
        
        handle->state = (result == STATUS_OK) ? I2C_STATE_IDLE : I2C_STATE_ERROR;
    }
    
    return result;
}

/**
 * @brief Read data from I2C device (blocking)
 */
status_t i2c_read_blocking(i2c_instance_t instance, u8_t dev_addr,
                           u8_t *data, u32_t len, u32_t timeout_ms)
{
    status_t result = STATUS_OK;
    
    /* CERT C EXP34-C: Validate pointer */
    if (IS_NULL_PTR(data))
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (instance >= I2C_INSTANCE_MAX)
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (!g_i2c_handles[instance].initialized)
    {
        result = STATUS_NOT_READY;
    }
    else if (len == 0U)
    {
        result = STATUS_INVALID_PARAM;
    }
    else
    {
        i2c_handle_t *handle = &g_i2c_handles[instance];
        i2c_regs_t *regs = handle->regs;
        
        handle->state = I2C_STATE_BUSY_RX;
        
        /* Enable ACK */
        BIT_SET(regs->CR1, I2C_CR1_ACK);
        
        /* Generate START */
        i2c_generate_start(regs);
        
        /* Send address with read direction */
        result = i2c_send_address(regs, dev_addr, I2C_DIR_READ, timeout_ms);
        
        if (result == STATUS_OK)
        {
            for (u32_t i = 0U; (i < len) && (result == STATUS_OK); i++)
            {
                /* For last byte, disable ACK before reading */
                if (i == (len - 1U))
                {
                    BIT_CLEAR(regs->CR1, I2C_CR1_ACK);
                    i2c_generate_stop(regs);
                }
                
                result = i2c_wait_flag(regs, I2C_SR1_RXNE, true, timeout_ms);
                if (result == STATUS_OK)
                {
                    data[i] = (u8_t)regs->DR;
                }
            }
        }
        else
        {
            /* Generate STOP on error */
            i2c_generate_stop(regs);
        }
        
        handle->state = (result == STATUS_OK) ? I2C_STATE_IDLE : I2C_STATE_ERROR;
    }
    
    return result;
}

/**
 * @brief Write data (interrupt-driven)
 */
status_t i2c_write_async(i2c_instance_t instance, u8_t dev_addr,
                         const u8_t *data, u32_t len, i2c_callback_t callback)
{
    status_t result = STATUS_OK;
    
    if (IS_NULL_PTR(data) || IS_NULL_PTR(callback))
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (instance >= I2C_INSTANCE_MAX)
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (!g_i2c_handles[instance].initialized)
    {
        result = STATUS_NOT_READY;
    }
    else if (g_i2c_handles[instance].state != I2C_STATE_IDLE)
    {
        result = STATUS_BUSY;
    }
    else
    {
        i2c_handle_t *handle = &g_i2c_handles[instance];
        
        handle->tx_buffer = data;
        handle->buffer_len = len;
        handle->buffer_idx = 0U;
        handle->dev_address = dev_addr;
        handle->callback = callback;
        handle->state = I2C_STATE_BUSY_TX;
        
        /* Generate START - rest handled by IRQ */
        i2c_generate_start(handle->regs);
    }
    
    return result;
}

/**
 * @brief Read data (interrupt-driven)
 */
status_t i2c_read_async(i2c_instance_t instance, u8_t dev_addr,
                        u8_t *data, u32_t len, i2c_callback_t callback)
{
    status_t result = STATUS_OK;
    
    if (IS_NULL_PTR(data) || IS_NULL_PTR(callback))
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (instance >= I2C_INSTANCE_MAX)
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (!g_i2c_handles[instance].initialized)
    {
        result = STATUS_NOT_READY;
    }
    else if (g_i2c_handles[instance].state != I2C_STATE_IDLE)
    {
        result = STATUS_BUSY;
    }
    else
    {
        i2c_handle_t *handle = &g_i2c_handles[instance];
        
        handle->rx_buffer = data;
        handle->buffer_len = len;
        handle->buffer_idx = 0U;
        handle->dev_address = dev_addr;
        handle->callback = callback;
        handle->state = I2C_STATE_BUSY_RX;
        
        BIT_SET(handle->regs->CR1, I2C_CR1_ACK);
        i2c_generate_start(handle->regs);
    }
    
    return result;
}

/**
 * @brief Get current I2C state
 */
i2c_state_t i2c_get_state(i2c_instance_t instance)
{
    i2c_state_t state = I2C_STATE_ERROR;
    
    if (instance < I2C_INSTANCE_MAX)
    {
        state = g_i2c_handles[instance].state;
    }
    
    return state;
}

/**
 * @brief I2C interrupt handler
 */
void i2c_irq_handler(i2c_instance_t instance)
{
    if (instance < I2C_INSTANCE_MAX)
    {
        i2c_handle_t *handle = &g_i2c_handles[instance];
        i2c_regs_t *regs = handle->regs;
        u32_t sr1 = regs->SR1;
        status_t result = STATUS_OK;
        
        /* START bit sent */
        if (BIT_CHECK(sr1, I2C_SR1_SB))
        {
            u8_t dir = (handle->state == I2C_STATE_BUSY_TX) ? (u8_t)0U : (u8_t)1U;
            regs->DR = (u8_t)((handle->dev_address << 1U) | dir);
        }
        /* Address sent */
        else if (BIT_CHECK(sr1, I2C_SR1_ADDR))
        {
            /* Clear ADDR by reading SR2 */
            (void)regs->SR2;
            
            if ((handle->state == I2C_STATE_BUSY_RX) && (handle->buffer_len == 1U))
            {
                BIT_CLEAR(regs->CR1, I2C_CR1_ACK);
            }
        }
        /* TX empty */
        else if (BIT_CHECK(sr1, I2C_SR1_TXE) && (handle->state == I2C_STATE_BUSY_TX))
        {
            if (handle->buffer_idx < handle->buffer_len)
            {
                regs->DR = handle->tx_buffer[handle->buffer_idx];
                handle->buffer_idx++;
            }
            else if (BIT_CHECK(sr1, I2C_SR1_BTF))
            {
                i2c_generate_stop(regs);
                handle->state = I2C_STATE_IDLE;
                
                if (IS_VALID_PTR(handle->callback))
                {
                    handle->callback(instance, STATUS_OK);
                }
            }
            else
            {
                /* Waiting for BTF */
            }
        }
        /* RX not empty */
        else if (BIT_CHECK(sr1, I2C_SR1_RXNE) && (handle->state == I2C_STATE_BUSY_RX))
        {
            handle->rx_buffer[handle->buffer_idx] = (u8_t)regs->DR;
            handle->buffer_idx++;
            
            if (handle->buffer_idx == (handle->buffer_len - 1U))
            {
                BIT_CLEAR(regs->CR1, I2C_CR1_ACK);
                i2c_generate_stop(regs);
            }
            
            if (handle->buffer_idx >= handle->buffer_len)
            {
                handle->state = I2C_STATE_IDLE;
                
                if (IS_VALID_PTR(handle->callback))
                {
                    handle->callback(instance, STATUS_OK);
                }
            }
        }
        /* ACK failure */
        else if (BIT_CHECK(sr1, I2C_SR1_AF))
        {
            regs->SR1 = ~(1U << I2C_SR1_AF);  /* Clear AF flag */
            i2c_generate_stop(regs);
            handle->state = I2C_STATE_ERROR;
            result = STATUS_ERROR;
            
            if (IS_VALID_PTR(handle->callback))
            {
                handle->callback(instance, result);
            }
        }
        else
        {
            /* No action needed */
        }
    }
}
