/**
 * @file main.c
 * @brief Main application for ARM Cortex-A7 bare-metal BLE/I2C system
 * @note MISRA C 2023 and CERT C compliant - No RTOS, uses while() and interrupts
 */

#include "types.h"
#include "arm_a7_regs.h"
#include "hal_gic.h"
#include "i2c_driver.h"
#include "ble_driver.h"
#include <string.h>

/* Application states */
typedef enum {
    APP_STATE_INIT = 0,
    APP_STATE_IDLE,
    APP_STATE_PROCESSING,
    APP_STATE_ERROR
} app_state_t;

/* I2C sensor addresses */
#define SENSOR_ADDR_TEMP        (0x48U)
#define SENSOR_ADDR_ACCEL       (0x1DU)

/* Application data structure */
typedef struct {
    app_state_t state;
    bool        ble_connected;
    bool        i2c_transfer_pending;
    u8_t        sensor_data[16];
    u16_t       sensor_data_len;
} app_context_t;

/* Global application context */
static app_context_t g_app_ctx = {
    .state = APP_STATE_INIT,
    .ble_connected = false,
    .i2c_transfer_pending = false,
    .sensor_data_len = 0U
};

/* Forward declarations */
static void app_init(void);
static void app_main_loop(void);
static void ble_event_handler(const ble_event_t *event);
static void i2c_complete_handler(i2c_instance_t instance, status_t result);
static status_t read_sensor_data(void);
static void process_ble_command(const u8_t *data, u16_t len);
void irq_handler(void);

/**
 * @brief BLE event callback
 */
static void ble_event_handler(const ble_event_t *event)
{
    /* CERT C EXP34-C: Validate pointer */
    if (IS_NULL_PTR(event))
    {
        return;
    }
    
    switch (event->type)
    {
        case BLE_EVT_CONNECTED:
            g_app_ctx.ble_connected = true;
            break;
            
        case BLE_EVT_DISCONNECTED:
            g_app_ctx.ble_connected = false;
            break;
            
        case BLE_EVT_DATA_RECEIVED:
            if (event->data_len > 0U)
            {
                process_ble_command(event->data, event->data_len);
            }
            break;
            
        case BLE_EVT_DATA_SENT:
            /* TX complete - ready for next transmission */
            break;
            
        case BLE_EVT_ERROR:
            g_app_ctx.state = APP_STATE_ERROR;
            break;
            
        case BLE_EVT_ADV_STARTED:
        case BLE_EVT_ADV_STOPPED:
        case BLE_EVT_SCAN_RESULT:
        case BLE_EVT_NONE:
        default:
            /* No action needed */
            break;
    }
}

/**
 * @brief I2C transfer complete callback
 */
static void i2c_complete_handler(i2c_instance_t instance, status_t result)
{
    (void)instance;
    
    g_app_ctx.i2c_transfer_pending = false;
    
    if (result != STATUS_OK)
    {
        g_app_ctx.state = APP_STATE_ERROR;
    }
}

/**
 * @brief Process BLE command from central device
 */
static void process_ble_command(const u8_t *data, u16_t len)
{
    /* CERT C EXP34-C: Validate pointer */
    if (IS_NULL_PTR(data) || (len == 0U))
    {
        return;
    }
    
    /* Command protocol: first byte is command ID */
    u8_t cmd = data[0];
    
    switch (cmd)
    {
        case 0x01U:  /* Read temperature sensor */
            (void)read_sensor_data();
            break;
            
        case 0x02U:  /* Read accelerometer */
            /* Implement accelerometer read */
            break;
            
        case 0xFFU:  /* Echo test */
            if (g_app_ctx.ble_connected)
            {
                (void)ble_send_data(data, len);
            }
            break;
            
        default:
            /* Unknown command */
            break;
    }
}

/**
 * @brief Read sensor data via I2C
 */
static status_t read_sensor_data(void)
{
    status_t result;
    
    if (g_app_ctx.i2c_transfer_pending)
    {
        result = STATUS_BUSY;
    }
    else
    {
        g_app_ctx.i2c_transfer_pending = true;
        
        /* Read 2 bytes from temperature sensor */
        result = i2c_read_async(I2C_INSTANCE_0, SENSOR_ADDR_TEMP,
                                g_app_ctx.sensor_data, 2U,
                                i2c_complete_handler);
        
        if (result == STATUS_OK)
        {
            g_app_ctx.sensor_data_len = 2U;
        }
        else
        {
            g_app_ctx.i2c_transfer_pending = false;
        }
    }
    
    return result;
}

/**
 * @brief Initialize application
 */
static void app_init(void)
{
    status_t result;
    
    /* Initialize GIC */
    result = gic_init();
    if (result != STATUS_OK)
    {
        g_app_ctx.state = APP_STATE_ERROR;
        return;
    }
    
    /* Initialize I2C */
    i2c_config_t i2c_cfg = {
        .clock_speed = 400000U,
        .own_address = 0U,
        .use_interrupts = true
    };
    
    result = i2c_init(I2C_INSTANCE_0, &i2c_cfg);
    if (result != STATUS_OK)
    {
        g_app_ctx.state = APP_STATE_ERROR;
        return;
    }
    
    /* Initialize BLE */
    ble_config_t ble_cfg = {
        .device_name = "ARM-A7-BLE",
        .adv_interval_ms = 100U,
        .conn_interval_min_ms = 20U,
        .conn_interval_max_ms = 40U,
        .tx_power_dbm = 0,
        .use_interrupts = true
    };
    
    result = ble_init(&ble_cfg, ble_event_handler);
    if (result != STATUS_OK)
    {
        g_app_ctx.state = APP_STATE_ERROR;
        return;
    }
    
    /* Start advertising */
    result = ble_start_advertising();
    if (result != STATUS_OK)
    {
        g_app_ctx.state = APP_STATE_ERROR;
        return;
    }
    
    /* Enable global interrupts */
    cpu_enable_irq();
    
    g_app_ctx.state = APP_STATE_IDLE;
}

/**
 * @brief Main application loop
 */
static void app_main_loop(void)
{
    /* Process BLE events */
    ble_process();
    
    /* Check if sensor data ready to send */
    if (!g_app_ctx.i2c_transfer_pending && 
        (g_app_ctx.sensor_data_len > 0U) &&
        g_app_ctx.ble_connected)
    {
        status_t result = ble_send_data(g_app_ctx.sensor_data, 
                                        g_app_ctx.sensor_data_len);
        if (result == STATUS_OK)
        {
            g_app_ctx.sensor_data_len = 0U;
        }
    }
    
    /* State machine */
    switch (g_app_ctx.state)
    {
        case APP_STATE_IDLE:
            /* Wait for events */
            break;
            
        case APP_STATE_PROCESSING:
            /* Processing state */
            break;
            
        case APP_STATE_ERROR:
        {
            /* Attempt recovery */
            (void)ble_stop_advertising();
            volatile u32_t delay = 1000000U;
            while (delay > 0U)
            {
                delay--;
            }
            (void)ble_start_advertising();
            g_app_ctx.state = APP_STATE_IDLE;
            break;
        }
            
        case APP_STATE_INIT:
        default:
            /* Should not reach here */
            break;
    }
}

/**
 * @brief IRQ handler - called by exception vector
 */
void irq_handler(void)
{
    u32_t irq_num = gic_acknowledge_irq();
    
    switch (irq_num)
    {
        case IRQ_I2C0:
            i2c_irq_handler(I2C_INSTANCE_0);
            break;
            
        case IRQ_I2C1:
            i2c_irq_handler(I2C_INSTANCE_1);
            break;
            
        case IRQ_BLE:
            ble_irq_handler();
            break;
            
        default:
            /* Spurious interrupt */
            break;
    }
    
    gic_end_of_irq(irq_num);
}

/**
 * @brief Main entry point
 */
int main(void)
{
    /* Initialize application */
    app_init();
    
    /* Main loop - MISRA C 2023: single exit point pattern with while(1) */
    while (g_app_ctx.state != APP_STATE_ERROR)
    {
        app_main_loop();
    }
    
    /* Cleanup on error exit */
    (void)ble_deinit();
    (void)i2c_deinit(I2C_INSTANCE_0);
    cpu_disable_irq();
    
    /* MISRA C 2023 Rule 21.8: Avoid non-returning main */
    return 0;
}
