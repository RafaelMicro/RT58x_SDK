/** @file hosal_uart.h
 * @license
 * @description
 */

#ifndef __HOSAL_UART_H__
#define __HOSAL_UART_H__

#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
//                  Constant Definition
//=============================================================================
/* Notice: This define is not hardware register bit setting, so don't be confused with
 * real hardware bit setting.
 */
#define CMSPAR             (1<<2)    /*sticky, hidden defined, not original linux*/
#define PARENB             (1<<1)    /*partity enabled, hidden defined, not original linux */
#define PARODD             (1<<0)    /*partity ODD, hidden defined, not original linux */

#define  UART_BAUDRATE_Baud2400         (1667)
#define  UART_BAUDRATE_Baud4800          (833)
#define  UART_BAUDRATE_Baud9600          (417)
#define  UART_BAUDRATE_Baud14400         (278)
#define  UART_BAUDRATE_Baud19200         (208)
#define  UART_BAUDRATE_Baud28800         (139)
#define  UART_BAUDRATE_Baud38400         (104)
#define  UART_BAUDRATE_Baud57600          (69)
#define  UART_BAUDRATE_Baud76800          (52)
#define  UART_BAUDRATE_Baud115200         (35)
#define  UART_BAUDRATE_Baud500000          (8)
#define  UART_BAUDRATE_Baud1000000         (4)
#define  UART_BAUDRATE_Baud2000000         (2)

typedef int (*hosal_uart_callback_t)(void *p_arg);


/**
 * @brief UART PARITY Type Definitions
 *
 */
typedef enum
{
    UART_PARITY_NONE  =  0,               /*!< NONE PARITY  \hideinitializer */
    UART_PARITY_EVEN  = (PARENB),         /*!< EVEN PARITY  \hideinitializer */
    UART_PARITY_ODD   = (PARENB | PARODD), /*!< ODD PARITY  \hideinitializer */
    UART_PARITY_SPACE = (CMSPAR | PARENB),        /*!< PARITY SPACE  \hideinitializer */
    UART_PARITY_MARK  = (CMSPAR | PARENB | PARODD) /*!< PARITY MARK     \hideinitializer */
} hosal_uart_parity_t;

/**
 * @brief Types of UART flow control modes.
 *
 */
typedef enum
{
    UART_HWFC_DISABLED = 0,   /*!< HW flow control disabled.  \hideinitializer */
    UART_HWFC_ENABLED         /*!< HW flow control enabled.  \hideinitializer */
} hosal_uart_hwfc_t;


/**
 * @brief Types of UART stop bit modes.
 *
 */
typedef enum
{
    UART_STOPBIT_ONE  = 0,       /*!< 1 STOP BIT  \hideinitializer */
    UART_STOPBIT_TWO             /*!< 2 STOP BIT  \hideinitializer */
} hosal_uart_stopbit_t;

typedef enum
{
    UART_BAUDRATE_2400    = UART_BAUDRATE_Baud2400,    /*!< 2400 baud. \hideinitializer */
    UART_BAUDRATE_4800    = UART_BAUDRATE_Baud4800,    /*!< 4800 baud. \hideinitializer */
    UART_BAUDRATE_9600    = UART_BAUDRATE_Baud9600,    /*!< 9600 baud. \hideinitializer */
    UART_BAUDRATE_14400   = UART_BAUDRATE_Baud14400,   /*!< 14400 baud. \hideinitializer */
    UART_BAUDRATE_19200   = UART_BAUDRATE_Baud19200,   /*!< 19200 baud. \hideinitializer */
    UART_BAUDRATE_28800   = UART_BAUDRATE_Baud28800,   /*!< 28800 baud. \hideinitializer */
    UART_BAUDRATE_38400   = UART_BAUDRATE_Baud38400,   /*!< 38400 baud. \hideinitializer */
    UART_BAUDRATE_57600   = UART_BAUDRATE_Baud57600,   /*!< 57600 baud. \hideinitializer */
    UART_BAUDRATE_76800   = UART_BAUDRATE_Baud76800,   /*!< 76800 baud. \hideinitializer */
    UART_BAUDRATE_115200  = UART_BAUDRATE_Baud115200,  /*!< 115200 baud. \hideinitializer */
    UART_BAUDRATE_500000  = UART_BAUDRATE_Baud500000,  /*!< 500000 baud. \hideinitializer */
    UART_BAUDRATE_1000000 = UART_BAUDRATE_Baud1000000, /*!< 1000000 baud. \hideinitializer */
    UART_BAUDRATE_2000000 = UART_BAUDRATE_Baud2000000  /*!< 2000000 baud. \hideinitializer */
} hosal_uart_baudrate_t;


typedef enum
{
    UART_DATA_BITS_5 = 0,  /*!< 5-bits. \hideinitializer */
    UART_DATA_BITS_6,      /*!< 6-bits. \hideinitializer */
    UART_DATA_BITS_7,      /*!< 7-bits. \hideinitializer */
    UART_DATA_BITS_8       /*!< 8-bits. \hideinitializer */
} hosal_uart_databits_t;

typedef enum
{
    HOSAL_UART_MODE_POLL,            /**< @brief UART poll mode (default mode) */
    HOSAL_UART_MODE_INT_TX,          /**< @brief UART TX int mode */
    HOSAL_UART_MODE_INT_RX,          /**< @brief UART RX int mode */
    HOSAL_UART_MODE_INT,             /**< @brief UART TX and RX int mode */
} hosal_uart_mode_t;
//=============================================================================
//                  Macro Definition
//=============================================================================
/**
* @brief UART EVENT
*
* @details UART_EVENT_RX_TIMEOUT is not error, it just signal the event
*        "during receive when idle time is detected between consecutive characters."
*/

#define HOSAL_UART_TX_CALLBACK       1   /**< @brief UART tx idle interrupt callback */
#define HOSAL_UART_RX_CALLBACK       2   /**< @brief UART rx complete callback */
#define HOSAL_UART_TX_DMA_CALLBACK   3   /**< @brief UART tx DMA trans complete callback */
#define HOSAL_UART_RX_DMA_CALLBACK   4   /**< @brief UART rx DMA trans complete callback */
#define HOSAL_UART_BREAK_CALLBACK    5    /**< @brief UART break callback */

#define UART_EVENT_TX_DONE             (1UL << 0)  ///< Send completed; however UART may still transmit data
#define UART_EVENT_RX_DONE             (1UL << 1)  ///< Receive completed
#define UART_EVENT_RX_OVERFLOW         (1UL << 2)  ///< Receive data overflow
#define UART_EVENT_RX_TIMEOUT          (1UL << 3)  ///< Receive character timeout (optional)
#define UART_EVENT_RX_BREAK            (1UL << 4)  ///< Break detected on receive
#define UART_EVENT_RX_FRAMING_ERROR    (1UL << 5)  ///< Framing error detected on receive
#define UART_EVENT_RX_PARITY_ERROR     (1UL << 6)  ///< Parity error detected on receive
#define UART_EVENT_CTS                 (1UL << 7)  ///< CTS state changed (optional)

#define UART_EVENT_RX_RECV             (1UL << 8)  ///< Receive data 

#define UART_POWER_OFF                 (0)         ///< Power off the uart device
#define UART_POWER_ON                  (1)         ///< Power on the uart device

#define HOSAL_UART_BAUD_SET          1   /**< @brief UART baud set */
#define HOSAL_UART_BAUD_GET          2   /**< @brief UART baud get */
#define HOSAL_UART_DATA_WIDTH_SET    3   /**< @brief UART data width set */
#define HOSAL_UART_DATA_WIDTH_GET    4   /**< @brief UART data width get */
#define HOSAL_UART_STOP_BITS_SET     5   /**< @brief UART stop bits set */
#define HOSAL_UART_STOP_BITS_GET     6   /**< @brief UART stop bits get */
#define HOSAL_UART_FLOWMODE_SET      7   /**< @brief UART flow mode set */
#define HOSAL_UART_FLOWSTAT_GET      8   /**< @brief UART flow state get */
#define HOSAL_UART_PARITY_SET        9   /**< @brief UART flow mode set */
#define HOSAL_UART_PARITY_GET       10   /**< @brief UART flow state get */
#define HOSAL_UART_MODE_SET         11   /**< @brief UART mode set */
#define HOSAL_UART_MODE_GET         12   /**< @brief UART mode get */
#define HOSAL_UART_FREE_TXFIFO_GET  13   /**< @brief UART free tx fifo get */
#define HOSAL_UART_FREE_RXFIFO_GET  14   /**< @brief UART free rx fifo get */
#define HOSAL_UART_FLUSH            15   /**< @brief Wait for the send to complete */
#define HOSAL_UART_TX_TRIGGER_ON    16   /**< @brief UART TX trigger on */
#define HOSAL_UART_TX_TRIGGER_OFF   17   /**< @brief UART TX trigger off */
#define HOSAL_UART_DMA_TX_START     18   /**< @brief UART DMA TX start trans */
#define HOSAL_UART_DMA_RX_START     19   /**< @brief UART DMA RX start trans */
#define HOSAL_UART_ENABLE_INTERRUPT 20
#define HOSAL_UART_DISABLE_INTERRUPT 21
//=============================================================================
//                  Structure Definition
//=============================================================================
typedef struct
{
    uint8_t                   uart_id;      /**< @brief UART id */
    uint8_t                   tx_pin;       /**< @brief UART tx pin */
    uint8_t                   rx_pin;       /**< @brief UART rx pin */
    uint8_t                   cts_pin;      /**< @brief UART cts pin */
    uint8_t                   rts_pin;      /**< @brief UART rts pin */
    hosal_uart_baudrate_t     baud_rate;    /**< @brief UART baud rate */
    hosal_uart_databits_t     data_width;   /**< @brief UART data width */
    hosal_uart_parity_t       parity;       /**< @brief UART parity bit */
    hosal_uart_stopbit_t    stop_bits;  /**< @brief UART stop btis */
    hosal_uart_hwfc_t       flow_control;   /**< @brief UART flow control */
    hosal_uart_mode_t       mode;
} hosal_uart_config_t;

typedef struct
{
    uint8_t port;
    hosal_uart_config_t     config;
    hosal_uart_callback_t   tx_cb;
    void                    *p_tx_arg;
    hosal_uart_callback_t   rx_cb;
    void                    *p_rx_arg;
    hosal_uart_callback_t   txdma_cb;
    void                    *p_txdma_arg;
    hosal_uart_callback_t  rxdma_cb;
    void                    *p_rxdma_arg;
    hosal_uart_callback_t  break_cb;
    void                    *p_break_arg;
    void                    *priv;
} hosal_uart_dev_t;

typedef struct
{
    uint8_t *dma_buf;
    uint32_t dma_buf_size;
} hosal_uart_dma_cfg_t;

//=============================================================================
//                  Global Data Definition
//=============================================================================
#define HOSAL_UART_DEV_DECL(dev, id, tx, rx, baud) \
    hosal_uart_dev_t dev = {                       \
        .config =  {                               \
            .uart_id = id,                         \
            .tx_pin = tx,                          \
            .rx_pin = rx,                          \
            .cts_pin = 255,                        \
            .rts_pin = 255,                        \
            .baud_rate = baud,                     \
            .data_width = UART_DATA_BITS_8,   \
            .parity = UART_PARITY_NONE,             \
            .stop_bits = UART_STOPBIT_ONE,        \
        },  \
        .port  = id,   \
        .tx_cb = NULL, \
        .rx_cb = NULL, \
    };
//=============================================================================
//                  Private Function Definition
//=============================================================================

//=============================================================================
//                  Public Function Definition
//=============================================================================
int hosal_uart_init(hosal_uart_dev_t *uart_dev);
int hosal_uart_send(hosal_uart_dev_t *uart_dev, const void *data, uint32_t size);
int hosal_uart_receive(hosal_uart_dev_t *uart_dev, void *data, uint32_t expect_size);
int hosal_uart_ioctl (hosal_uart_dev_t *uart_dev, int ctl, void *p_arg);
int hosal_uart_callback_set (hosal_uart_dev_t *uart_dev,
                             int callback_type,
                             hosal_uart_callback_t pfn_callback,
                             void *arg);

int hosal_uart_finalize(hosal_uart_dev_t *uart_dev);
#ifdef __cplusplus
}
#endif

#endif