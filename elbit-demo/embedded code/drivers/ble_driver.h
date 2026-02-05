/**
 * @file ble_driver.h
 * @brief BLE driver interface for ARM Cortex-A7
 * @note MISRA C 2023 and CERT C compliant - bare-metal interrupt-driven
 */

#ifndef BLE_DRIVER_H
#define BLE_DRIVER_H

#include "types.h"

/* BLE connection states */
typedef enum {
    BLE_STATE_OFF = 0,
    BLE_STATE_IDLE,
    BLE_STATE_ADVERTISING,
    BLE_STATE_SCANNING,
    BLE_STATE_CONNECTING,
    BLE_STATE_CONNECTED,
    BLE_STATE_DISCONNECTING,
    BLE_STATE_ERROR
} ble_state_t;

/* BLE event types */
typedef enum {
    BLE_EVT_NONE = 0,
    BLE_EVT_CONNECTED,
    BLE_EVT_DISCONNECTED,
    BLE_EVT_DATA_RECEIVED,
    BLE_EVT_DATA_SENT,
    BLE_EVT_ADV_STARTED,
    BLE_EVT_ADV_STOPPED,
    BLE_EVT_SCAN_RESULT,
    BLE_EVT_ERROR
} ble_event_type_t;

/* Maximum BLE data payload size */
#define BLE_MAX_PAYLOAD_SIZE    (244U)
#define BLE_MAX_DEVICE_NAME     (32U)
#define BLE_MAC_ADDR_LEN        (6U)

/* BLE MAC address type */
typedef struct {
    u8_t addr[BLE_MAC_ADDR_LEN];
} ble_mac_addr_t;

/* BLE event structure */
typedef struct {
    ble_event_type_t type;
    ble_mac_addr_t   peer_addr;
    u8_t             data[BLE_MAX_PAYLOAD_SIZE];
    u16_t            data_len;
    s8_t             rssi;
} ble_event_t;

/* BLE configuration */
typedef struct {
    char     device_name[BLE_MAX_DEVICE_NAME];
    u16_t    adv_interval_ms;
    u16_t    conn_interval_min_ms;
    u16_t    conn_interval_max_ms;
    s8_t     tx_power_dbm;
    bool     use_interrupts;
} ble_config_t;

/* BLE event callback type */
typedef void (*ble_event_callback_t)(const ble_event_t *event);

/**
 * @brief Initialize BLE subsystem
 * @param[in] config Pointer to configuration
 * @param[in] callback Event callback function
 * @return STATUS_OK on success
 */
status_t ble_init(const ble_config_t *config, ble_event_callback_t callback);

/**
 * @brief Deinitialize BLE subsystem
 * @return STATUS_OK on success
 */
status_t ble_deinit(void);

/**
 * @brief Start BLE advertising
 * @return STATUS_OK on success
 */
status_t ble_start_advertising(void);

/**
 * @brief Stop BLE advertising
 * @return STATUS_OK on success
 */
status_t ble_stop_advertising(void);

/**
 * @brief Start BLE scanning
 * @param[in] duration_ms Scan duration in milliseconds (0 = continuous)
 * @return STATUS_OK on success
 */
status_t ble_start_scan(u32_t duration_ms);

/**
 * @brief Stop BLE scanning
 * @return STATUS_OK on success
 */
status_t ble_stop_scan(void);

/**
 * @brief Connect to a BLE device
 * @param[in] addr MAC address of device to connect
 * @return STATUS_OK if connection initiated
 */
status_t ble_connect(const ble_mac_addr_t *addr);

/**
 * @brief Disconnect from current device
 * @return STATUS_OK on success
 */
status_t ble_disconnect(void);

/**
 * @brief Send data over BLE connection
 * @param[in] data Pointer to data buffer
 * @param[in] len Length of data (max BLE_MAX_PAYLOAD_SIZE)
 * @return STATUS_OK on success
 */
status_t ble_send_data(const u8_t *data, u16_t len);

/**
 * @brief Get current BLE state
 * @return Current state
 */
ble_state_t ble_get_state(void);

/**
 * @brief Get local BLE MAC address
 * @param[out] addr Pointer to store MAC address
 * @return STATUS_OK on success
 */
status_t ble_get_mac_address(ble_mac_addr_t *addr);

/**
 * @brief Process BLE events (call from main loop)
 */
void ble_process(void);

/**
 * @brief BLE interrupt handler - call from IRQ handler
 */
void ble_irq_handler(void);

#endif /* BLE_DRIVER_H */
