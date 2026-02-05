/**
 * @file ble_driver.c
 * @brief BLE driver implementation for ARM Cortex-A7
 * @note MISRA C 2023 and CERT C compliant - bare-metal interrupt-driven
 */

#include "ble_driver.h"
#include "arm_a7_regs.h"
#include "hal_gic.h"
#include <string.h>

/* BLE Controller register base (example address) */
#define BLE_BASE                (PERIPH_BASE + 0x00010000U)

/* BLE Controller Registers */
typedef struct {
    vu32_t CTRL;        /* Control register */
    vu32_t STATUS;      /* Status register */
    vu32_t INT_EN;      /* Interrupt enable */
    vu32_t INT_FLAG;    /* Interrupt flags */
    vu32_t TX_DATA;     /* TX data register */
    vu32_t RX_DATA;     /* RX data register */
    vu32_t TX_LEN;      /* TX length */
    vu32_t RX_LEN;      /* RX length */
    vu32_t ADV_CTRL;    /* Advertising control */
    vu32_t CONN_CTRL;   /* Connection control */
    vu32_t SCAN_CTRL;   /* Scan control */
    vu32_t TX_POWER;    /* TX power control */
    vu32_t MAC_L;       /* MAC address low */
    vu32_t MAC_H;       /* MAC address high */
} ble_regs_t;

#define BLE_REGS    ((ble_regs_t *)BLE_BASE)

/* Control register bits */
#define BLE_CTRL_ENABLE         (0U)
#define BLE_CTRL_RESET          (1U)
#define BLE_CTRL_ADV_START      (4U)
#define BLE_CTRL_SCAN_START     (5U)
#define BLE_CTRL_CONN_INIT      (6U)
#define BLE_CTRL_TX_START       (8U)

/* Status register bits */
#define BLE_STATUS_READY        (0U)
#define BLE_STATUS_CONNECTED    (1U)
#define BLE_STATUS_ADV_ACTIVE   (2U)
#define BLE_STATUS_SCAN_ACTIVE  (3U)
#define BLE_STATUS_TX_BUSY      (4U)
#define BLE_STATUS_RX_READY     (5U)

/* Interrupt flag bits */
#define BLE_INT_CONNECTED       (0U)
#define BLE_INT_DISCONNECTED    (1U)
#define BLE_INT_RX_DONE         (2U)
#define BLE_INT_TX_DONE         (3U)
#define BLE_INT_ADV_DONE        (4U)
#define BLE_INT_SCAN_RESULT     (5U)
#define BLE_INT_ERROR           (7U)

/* Event queue size */
#define BLE_EVENT_QUEUE_SIZE    (8U)

/* BLE handle structure */
typedef struct {
    ble_state_t           state;
    ble_event_callback_t  callback;
    ble_config_t          config;
    ble_mac_addr_t        local_addr;
    ble_mac_addr_t        peer_addr;
    ble_event_t           event_queue[BLE_EVENT_QUEUE_SIZE];
    u8_t                  evt_queue_head;
    u8_t                  evt_queue_tail;
    bool                  initialized;
    u8_t                  rx_buffer[BLE_MAX_PAYLOAD_SIZE];
    u16_t                 rx_len;
    volatile bool         rx_pending;
    volatile bool         tx_complete;
} ble_handle_t;

/* Static BLE handle */
static ble_handle_t g_ble_handle = {
    .state = BLE_STATE_OFF,
    .initialized = false,
    .evt_queue_head = 0U,
    .evt_queue_tail = 0U,
    .rx_pending = false,
    .tx_complete = false
};

/* Forward declarations */
static status_t ble_enqueue_event(const ble_event_t *event);
static bool ble_dequeue_event(ble_event_t *event);
static void ble_read_rx_data(void);

/**
 * @brief Initialize BLE subsystem
 */
status_t ble_init(const ble_config_t *config, ble_event_callback_t callback)
{
    status_t result = STATUS_OK;
    
    /* CERT C EXP34-C: Validate pointers */
    if (IS_NULL_PTR(config) || IS_NULL_PTR(callback))
    {
        result = STATUS_INVALID_PARAM;
    }
    else
    {
        ble_regs_t *regs = BLE_REGS;
        
        /* Reset BLE controller */
        BIT_SET(regs->CTRL, BLE_CTRL_RESET);
        
        /* Wait for reset complete */
        volatile u32_t delay = 10000U;
        while (delay > 0U)
        {
            delay--;
        }
        
        BIT_CLEAR(regs->CTRL, BLE_CTRL_RESET);
        
        /* Wait for ready */
        u32_t timeout = 100000U;
        while (!BIT_CHECK(regs->STATUS, BLE_STATUS_READY) && (timeout > 0U))
        {
            timeout--;
        }
        
        if (timeout == 0U)
        {
            result = STATUS_TIMEOUT;
        }
        else
        {
            /* Store configuration */
            (void)memcpy(&g_ble_handle.config, config, sizeof(ble_config_t));
            g_ble_handle.callback = callback;
            
            /* Configure TX power */
            regs->TX_POWER = (u32_t)((s32_t)config->tx_power_dbm + 20);
            
            /* Configure advertising interval */
            regs->ADV_CTRL = (u32_t)(config->adv_interval_ms & 0xFFFFU);
            
            /* Read local MAC address */
            u32_t mac_low = regs->MAC_L;
            u32_t mac_high = regs->MAC_H;
            
            g_ble_handle.local_addr.addr[0] = (u8_t)(mac_low & 0xFFU);
            g_ble_handle.local_addr.addr[1] = (u8_t)((mac_low >> 8U) & 0xFFU);
            g_ble_handle.local_addr.addr[2] = (u8_t)((mac_low >> 16U) & 0xFFU);
            g_ble_handle.local_addr.addr[3] = (u8_t)((mac_low >> 24U) & 0xFFU);
            g_ble_handle.local_addr.addr[4] = (u8_t)(mac_high & 0xFFU);
            g_ble_handle.local_addr.addr[5] = (u8_t)((mac_high >> 8U) & 0xFFU);
            
            /* Enable interrupts if requested */
            if (config->use_interrupts)
            {
                regs->INT_EN = (1U << BLE_INT_CONNECTED) |
                               (1U << BLE_INT_DISCONNECTED) |
                               (1U << BLE_INT_RX_DONE) |
                               (1U << BLE_INT_TX_DONE) |
                               (1U << BLE_INT_ERROR);
                
                (void)gic_set_priority(IRQ_BLE, 0x80U);
                (void)gic_enable_irq(IRQ_BLE);
            }
            
            /* Enable BLE controller */
            BIT_SET(regs->CTRL, BLE_CTRL_ENABLE);
            
            g_ble_handle.state = BLE_STATE_IDLE;
            g_ble_handle.initialized = true;
            g_ble_handle.evt_queue_head = 0U;
            g_ble_handle.evt_queue_tail = 0U;
        }
    }
    
    return result;
}

/**
 * @brief Deinitialize BLE subsystem
 */
status_t ble_deinit(void)
{
    status_t result = STATUS_OK;
    
    if (!g_ble_handle.initialized)
    {
        result = STATUS_NOT_READY;
    }
    else
    {
        ble_regs_t *regs = BLE_REGS;
        
        /* Disable interrupts */
        regs->INT_EN = 0U;
        (void)gic_disable_irq(IRQ_BLE);
        
        /* Disable BLE controller */
        BIT_CLEAR(regs->CTRL, BLE_CTRL_ENABLE);
        
        g_ble_handle.state = BLE_STATE_OFF;
        g_ble_handle.initialized = false;
    }
    
    return result;
}

/**
 * @brief Start BLE advertising
 */
status_t ble_start_advertising(void)
{
    status_t result = STATUS_OK;
    
    if (!g_ble_handle.initialized)
    {
        result = STATUS_NOT_READY;
    }
    else if ((g_ble_handle.state != BLE_STATE_IDLE) && 
             (g_ble_handle.state != BLE_STATE_CONNECTED))
    {
        result = STATUS_BUSY;
    }
    else
    {
        ble_regs_t *regs = BLE_REGS;
        
        BIT_SET(regs->CTRL, BLE_CTRL_ADV_START);
        g_ble_handle.state = BLE_STATE_ADVERTISING;
        
        /* Queue event */
        ble_event_t evt = { .type = BLE_EVT_ADV_STARTED, .data_len = 0U };
        (void)ble_enqueue_event(&evt);
    }
    
    return result;
}

/**
 * @brief Stop BLE advertising
 */
status_t ble_stop_advertising(void)
{
    status_t result = STATUS_OK;
    
    if (!g_ble_handle.initialized)
    {
        result = STATUS_NOT_READY;
    }
    else if (g_ble_handle.state != BLE_STATE_ADVERTISING)
    {
        result = STATUS_INVALID_PARAM;
    }
    else
    {
        ble_regs_t *regs = BLE_REGS;
        
        BIT_CLEAR(regs->CTRL, BLE_CTRL_ADV_START);
        g_ble_handle.state = BLE_STATE_IDLE;
        
        ble_event_t evt = { .type = BLE_EVT_ADV_STOPPED, .data_len = 0U };
        (void)ble_enqueue_event(&evt);
    }
    
    return result;
}

/**
 * @brief Start BLE scanning
 */
status_t ble_start_scan(u32_t duration_ms)
{
    status_t result = STATUS_OK;
    
    (void)duration_ms;  /* TODO: Implement scan duration */
    
    if (!g_ble_handle.initialized)
    {
        result = STATUS_NOT_READY;
    }
    else if (g_ble_handle.state != BLE_STATE_IDLE)
    {
        result = STATUS_BUSY;
    }
    else
    {
        ble_regs_t *regs = BLE_REGS;
        
        BIT_SET(regs->CTRL, BLE_CTRL_SCAN_START);
        g_ble_handle.state = BLE_STATE_SCANNING;
    }
    
    return result;
}

/**
 * @brief Stop BLE scanning
 */
status_t ble_stop_scan(void)
{
    status_t result = STATUS_OK;
    
    if (!g_ble_handle.initialized)
    {
        result = STATUS_NOT_READY;
    }
    else if (g_ble_handle.state != BLE_STATE_SCANNING)
    {
        result = STATUS_INVALID_PARAM;
    }
    else
    {
        ble_regs_t *regs = BLE_REGS;
        
        BIT_CLEAR(regs->CTRL, BLE_CTRL_SCAN_START);
        g_ble_handle.state = BLE_STATE_IDLE;
    }
    
    return result;
}

/**
 * @brief Connect to a BLE device
 */
status_t ble_connect(const ble_mac_addr_t *addr)
{
    status_t result = STATUS_OK;
    
    if (IS_NULL_PTR(addr))
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (!g_ble_handle.initialized)
    {
        result = STATUS_NOT_READY;
    }
    else if ((g_ble_handle.state != BLE_STATE_IDLE) && 
             (g_ble_handle.state != BLE_STATE_SCANNING))
    {
        result = STATUS_BUSY;
    }
    else
    {
        ble_regs_t *regs = BLE_REGS;
        
        /* Store peer address */
        (void)memcpy(&g_ble_handle.peer_addr, addr, sizeof(ble_mac_addr_t));
        
        /* Write peer address to hardware */
        u32_t addr_low = ((u32_t)addr->addr[0]) |
                         ((u32_t)addr->addr[1] << 8U) |
                         ((u32_t)addr->addr[2] << 16U) |
                         ((u32_t)addr->addr[3] << 24U);
        /* Peer address registers would be written here */
        (void)addr_low;
        
        BIT_SET(regs->CTRL, BLE_CTRL_CONN_INIT);
        g_ble_handle.state = BLE_STATE_CONNECTING;
    }
    
    return result;
}

/**
 * @brief Disconnect from current device
 */
status_t ble_disconnect(void)
{
    status_t result = STATUS_OK;
    
    if (!g_ble_handle.initialized)
    {
        result = STATUS_NOT_READY;
    }
    else if (g_ble_handle.state != BLE_STATE_CONNECTED)
    {
        result = STATUS_INVALID_PARAM;
    }
    else
    {
        ble_regs_t *regs = BLE_REGS;
        
        BIT_CLEAR(regs->CTRL, BLE_CTRL_CONN_INIT);
        g_ble_handle.state = BLE_STATE_DISCONNECTING;
    }
    
    return result;
}

/**
 * @brief Send data over BLE connection
 */
status_t ble_send_data(const u8_t *data, u16_t len)
{
    status_t result = STATUS_OK;
    
    if (IS_NULL_PTR(data))
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (len > BLE_MAX_PAYLOAD_SIZE)
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (!g_ble_handle.initialized)
    {
        result = STATUS_NOT_READY;
    }
    else if (g_ble_handle.state != BLE_STATE_CONNECTED)
    {
        result = STATUS_NOT_READY;
    }
    else
    {
        ble_regs_t *regs = BLE_REGS;
        
        /* Wait for TX ready */
        u32_t timeout = 100000U;
        while (BIT_CHECK(regs->STATUS, BLE_STATUS_TX_BUSY) && (timeout > 0U))
        {
            timeout--;
        }
        
        if (timeout == 0U)
        {
            result = STATUS_TIMEOUT;
        }
        else
        {
            /* Write data length */
            regs->TX_LEN = (u32_t)len;
            
            /* Write data (assuming FIFO access) */
            for (u16_t i = 0U; i < len; i++)
            {
                regs->TX_DATA = (u32_t)data[i];
            }
            
            /* Start transmission */
            g_ble_handle.tx_complete = false;
            BIT_SET(regs->CTRL, BLE_CTRL_TX_START);
        }
    }
    
    return result;
}

/**
 * @brief Get current BLE state
 */
ble_state_t ble_get_state(void)
{
    return g_ble_handle.state;
}

/**
 * @brief Get local BLE MAC address
 */
status_t ble_get_mac_address(ble_mac_addr_t *addr)
{
    status_t result = STATUS_OK;
    
    if (IS_NULL_PTR(addr))
    {
        result = STATUS_INVALID_PARAM;
    }
    else if (!g_ble_handle.initialized)
    {
        result = STATUS_NOT_READY;
    }
    else
    {
        (void)memcpy(addr, &g_ble_handle.local_addr, sizeof(ble_mac_addr_t));
    }
    
    return result;
}

/**
 * @brief Enqueue BLE event
 */
static status_t ble_enqueue_event(const ble_event_t *event)
{
    status_t result = STATUS_OK;
    u8_t next_head = (g_ble_handle.evt_queue_head + 1U) % BLE_EVENT_QUEUE_SIZE;
    
    if (next_head == g_ble_handle.evt_queue_tail)
    {
        result = STATUS_ERROR;  /* Queue full */
    }
    else
    {
        (void)memcpy(&g_ble_handle.event_queue[g_ble_handle.evt_queue_head],
                     event, sizeof(ble_event_t));
        g_ble_handle.evt_queue_head = next_head;
    }
    
    return result;
}

/**
 * @brief Dequeue BLE event
 */
static bool ble_dequeue_event(ble_event_t *event)
{
    bool has_event = false;
    
    if (g_ble_handle.evt_queue_tail != g_ble_handle.evt_queue_head)
    {
        (void)memcpy(event, &g_ble_handle.event_queue[g_ble_handle.evt_queue_tail],
                     sizeof(ble_event_t));
        g_ble_handle.evt_queue_tail = 
            (g_ble_handle.evt_queue_tail + 1U) % BLE_EVENT_QUEUE_SIZE;
        has_event = true;
    }
    
    return has_event;
}

/**
 * @brief Read RX data from hardware
 */
static void ble_read_rx_data(void)
{
    ble_regs_t *regs = BLE_REGS;
    
    u32_t rx_len = regs->RX_LEN;
    if (rx_len > BLE_MAX_PAYLOAD_SIZE)
    {
        rx_len = BLE_MAX_PAYLOAD_SIZE;
    }
    
    g_ble_handle.rx_len = (u16_t)rx_len;
    
    for (u16_t i = 0U; i < g_ble_handle.rx_len; i++)
    {
        g_ble_handle.rx_buffer[i] = (u8_t)(regs->RX_DATA & 0xFFU);
    }
    
    g_ble_handle.rx_pending = true;
}

/**
 * @brief Process BLE events (call from main loop)
 */
void ble_process(void)
{
    ble_event_t evt;
    
    /* Process any pending RX data */
    if (g_ble_handle.rx_pending)
    {
        g_ble_handle.rx_pending = false;
        
        evt.type = BLE_EVT_DATA_RECEIVED;
        evt.data_len = g_ble_handle.rx_len;
        (void)memcpy(evt.data, g_ble_handle.rx_buffer, g_ble_handle.rx_len);
        (void)memcpy(&evt.peer_addr, &g_ble_handle.peer_addr, sizeof(ble_mac_addr_t));
        
        if (IS_VALID_PTR(g_ble_handle.callback))
        {
            g_ble_handle.callback(&evt);
        }
    }
    
    /* Process queued events */
    while (ble_dequeue_event(&evt))
    {
        if (IS_VALID_PTR(g_ble_handle.callback))
        {
            g_ble_handle.callback(&evt);
        }
    }
}

/**
 * @brief BLE interrupt handler
 */
void ble_irq_handler(void)
{
    ble_regs_t *regs = BLE_REGS;
    u32_t int_flags = regs->INT_FLAG;
    ble_event_t evt = { .type = BLE_EVT_NONE, .data_len = 0U };
    
    /* Connected event */
    if (BIT_CHECK(int_flags, BLE_INT_CONNECTED))
    {
        regs->INT_FLAG = (1U << BLE_INT_CONNECTED);  /* Clear flag */
        g_ble_handle.state = BLE_STATE_CONNECTED;
        
        evt.type = BLE_EVT_CONNECTED;
        (void)memcpy(&evt.peer_addr, &g_ble_handle.peer_addr, sizeof(ble_mac_addr_t));
        (void)ble_enqueue_event(&evt);
    }
    
    /* Disconnected event */
    if (BIT_CHECK(int_flags, BLE_INT_DISCONNECTED))
    {
        regs->INT_FLAG = (1U << BLE_INT_DISCONNECTED);
        g_ble_handle.state = BLE_STATE_IDLE;
        
        evt.type = BLE_EVT_DISCONNECTED;
        (void)ble_enqueue_event(&evt);
    }
    
    /* RX complete */
    if (BIT_CHECK(int_flags, BLE_INT_RX_DONE))
    {
        regs->INT_FLAG = (1U << BLE_INT_RX_DONE);
        ble_read_rx_data();
    }
    
    /* TX complete */
    if (BIT_CHECK(int_flags, BLE_INT_TX_DONE))
    {
        regs->INT_FLAG = (1U << BLE_INT_TX_DONE);
        g_ble_handle.tx_complete = true;
        
        evt.type = BLE_EVT_DATA_SENT;
        (void)ble_enqueue_event(&evt);
    }
    
    /* Error */
    if (BIT_CHECK(int_flags, BLE_INT_ERROR))
    {
        regs->INT_FLAG = (1U << BLE_INT_ERROR);
        g_ble_handle.state = BLE_STATE_ERROR;
        
        evt.type = BLE_EVT_ERROR;
        (void)ble_enqueue_event(&evt);
    }
}
