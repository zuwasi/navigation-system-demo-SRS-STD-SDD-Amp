/**
 * @file i2c_driver.h
 * @brief I2C driver interface for ARM Cortex-A7
 * @note MISRA C 2023 and CERT C compliant - bare-metal interrupt-driven
 */

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "types.h"

/* I2C instance enumeration */
typedef enum {
    I2C_INSTANCE_0 = 0,
    I2C_INSTANCE_1 = 1,
    I2C_INSTANCE_MAX
} i2c_instance_t;

/* I2C configuration structure */
typedef struct {
    u32_t clock_speed;      /* I2C clock speed in Hz (100000 or 400000) */
    u8_t  own_address;      /* Own address for slave mode (7-bit) */
    bool  use_interrupts;   /* Enable interrupt-driven mode */
} i2c_config_t;

/* I2C transfer direction */
typedef enum {
    I2C_DIR_WRITE = 0,
    I2C_DIR_READ  = 1
} i2c_direction_t;

/* I2C transfer state */
typedef enum {
    I2C_STATE_IDLE = 0,
    I2C_STATE_BUSY_TX,
    I2C_STATE_BUSY_RX,
    I2C_STATE_ERROR
} i2c_state_t;

/* I2C transfer callback type */
typedef void (*i2c_callback_t)(i2c_instance_t instance, status_t result);

/**
 * @brief Initialize I2C peripheral
 * @param[in] instance I2C instance (0 or 1)
 * @param[in] config Pointer to configuration structure
 * @return STATUS_OK on success
 */
status_t i2c_init(i2c_instance_t instance, const i2c_config_t *config);

/**
 * @brief Deinitialize I2C peripheral
 * @param[in] instance I2C instance
 * @return STATUS_OK on success
 */
status_t i2c_deinit(i2c_instance_t instance);

/**
 * @brief Write data to I2C device (blocking)
 * @param[in] instance I2C instance
 * @param[in] dev_addr Device address (7-bit)
 * @param[in] data Pointer to data buffer
 * @param[in] len Number of bytes to write
 * @param[in] timeout_ms Timeout in milliseconds
 * @return STATUS_OK on success
 */
status_t i2c_write_blocking(i2c_instance_t instance, u8_t dev_addr,
                            const u8_t *data, u32_t len, u32_t timeout_ms);

/**
 * @brief Read data from I2C device (blocking)
 * @param[in] instance I2C instance
 * @param[in] dev_addr Device address (7-bit)
 * @param[out] data Pointer to data buffer
 * @param[in] len Number of bytes to read
 * @param[in] timeout_ms Timeout in milliseconds
 * @return STATUS_OK on success
 */
status_t i2c_read_blocking(i2c_instance_t instance, u8_t dev_addr,
                           u8_t *data, u32_t len, u32_t timeout_ms);

/**
 * @brief Write data to I2C device (interrupt-driven)
 * @param[in] instance I2C instance
 * @param[in] dev_addr Device address (7-bit)
 * @param[in] data Pointer to data buffer
 * @param[in] len Number of bytes to write
 * @param[in] callback Completion callback
 * @return STATUS_OK if transfer started
 */
status_t i2c_write_async(i2c_instance_t instance, u8_t dev_addr,
                         const u8_t *data, u32_t len, i2c_callback_t callback);

/**
 * @brief Read data from I2C device (interrupt-driven)
 * @param[in] instance I2C instance
 * @param[in] dev_addr Device address (7-bit)
 * @param[out] data Pointer to data buffer
 * @param[in] len Number of bytes to read
 * @param[in] callback Completion callback
 * @return STATUS_OK if transfer started
 */
status_t i2c_read_async(i2c_instance_t instance, u8_t dev_addr,
                        u8_t *data, u32_t len, i2c_callback_t callback);

/**
 * @brief Get current I2C state
 * @param[in] instance I2C instance
 * @return Current state
 */
i2c_state_t i2c_get_state(i2c_instance_t instance);

/**
 * @brief I2C interrupt handler - call from IRQ handler
 * @param[in] instance I2C instance
 */
void i2c_irq_handler(i2c_instance_t instance);

#endif /* I2C_DRIVER_H */
