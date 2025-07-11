#include "main.h"
#include "sys_arch.h"
#include "app_uart.h"
#include "thread_queue.h"
#include "hosal_uart.h"
//=============================================================================
//                Private Definitions of const value
//=============================================================================
#define UART1_NOTIFY_ISR(ebit)                 (g_uart1_evt_var |= ebit); __uart1_signal()
#define UART1_NOTIFY(ebit)                     enter_critical_section(); g_uart1_evt_var |= ebit; leave_critical_section(); __cpc_uart_signal()
#define UART1_GET_NOTIFY(ebit)                 enter_critical_section(); ebit = g_uart1_evt_var; g_uart1_evt_var = UART1_EVENT_NONE; ; leave_critical_section()

#define RX_BUFF_SIZE                    484
#define MAX_UART_BUFFER_SIZE                256
//=============================================================================
//                Private ENUM
//=============================================================================

//=============================================================================
//                Private Struct
//=============================================================================
typedef struct uart_io
{
    uint16_t start;
    uint16_t end;

    uint32_t recvLen;
    uint8_t uart_cache[RX_BUFF_SIZE];
} uart_io_t;
typedef enum
{
    UART1_EVENT_NONE                       = 0,

    UART1_EVENT_UART_IN                    = 0x00000001,
    UAR1T_EVENT_UART_TX_DOWN               = 0x00000002,

    UART1_EVENT_ALL                        = 0xffffffff,
} uart1_event_t;
//=============================================================================
//                Private Function Declaration
//=============================================================================

//=============================================================================
//                Private Global Variables
//=============================================================================
static TaskHandle_t sUartTask = NULL;
static uart1_event_t g_uart1_evt_var;
static uart_io_t g_uart_rx_io = { .start = 0, .end = 0, };
HOSAL_UART_DEV_DECL(uart1_dev, 1, 28, 29, UART_BAUDRATE_Baud115200)

static uint8_t app_uart_buf[MAX_UART_BUFFER_SIZE] = { 0 };
static bool app_uart_saved = false;
//=============================================================================
//                Public Global Variables
//=============================================================================

//=============================================================================
//                Private Definition of Compare/Operation/Inline function/
//=============================================================================

//=============================================================================
//                Functions
//=============================================================================

uint8_t crc_get(uint8_t *data, uint16_t len)
{
    uint8_t crc = 0;
    uint16_t i = 0;
    for (i = 0; i < (len - 1); i++)
    {
        crc += data[i];
    }
    return crc;
}
//example
//+-------------+-----------+-------------+--------+
//|  Header(4)  | Length(1) | Command (4) | CS (1) |
//+-------------+-----------+-------------+--------+
//| FF FC FC FF |           |             |        |
//+-------------+-----------+-------------+--------+
void app_uart_command_parse(uint8_t *data, uint16_t lens)
{
    static uint16_t total_len = 0;
    uint8_t header[4] = { 0xff, 0xfc, 0xfc, 0xff };

    do
    {
        if (lens < 5)
        {
            break;
        }
        if (app_uart_saved == false && memcmp(data, header, 4) == 0)
        {
            app_uart_saved = true;
        }

        if (app_uart_saved == true)
        {
            if (total_len > MAX_UART_BUFFER_SIZE)
            {
                total_len = 0;
            }
            mem_memcpy(app_uart_buf + total_len, data, lens);
            total_len += lens;

            if (total_len > 5 && total_len >= (app_uart_buf[4] + 6))
            {
                uint8_t crc_8 = crc_get(app_uart_buf, total_len);
                if (crc_8 == app_uart_buf[total_len - 1])
                {
                    util_log_mem(UTIL_LOG_INFO, "UART0 :", app_uart_buf, total_len, 0);
                }
                else
                {
                    info("crc error %x %x\n", crc_8, app_uart_buf[total_len - 1]);
                }
                app_uart_saved = false;
            }
        }

    } while (0);
}

void app_uart_send(uint8_t *pdata, uint32_t len)
{
    hosal_uart_send(&uart1_dev, pdata, len);
}

static void __uart1_signal(void)
{
    if (xPortIsInsideInterrupt())
    {
        BaseType_t pxHigherPriorityTaskWoken = pdTRUE;
        vTaskNotifyGiveFromISR( sUartTask, &pxHigherPriorityTaskWoken);
    }
    else
    {
        xTaskNotifyGive(sUartTask);
    }
}

static int __uart_rx_callback(void *p_arg)
{
    uint32_t new_data_len = 0;
    uint16_t start = g_uart_rx_io.start;
    uint16_t end = g_uart_rx_io.end;

    // Calculate the available buffer space for writing
    new_data_len = (start - end - 1 + RX_BUFF_SIZE) % RX_BUFF_SIZE;

    if (new_data_len == 0)
    {
        return 0;  // No space to write to
    }

    uint32_t received = hosal_uart_receive(p_arg, g_uart_rx_io.uart_cache + end, new_data_len);
    g_uart_rx_io.end = (g_uart_rx_io.end + received) % RX_BUFF_SIZE;

    // Update receiving length
    g_uart_rx_io.recvLen = (RX_BUFF_SIZE + g_uart_rx_io.start - g_uart_rx_io.end) % RX_BUFF_SIZE;

    return 0;
}

static int __uart_read(uint8_t *p_data, uint32_t p_data_len)
{
    uint32_t byte_cnt = 0;

    if (p_data == NULL || p_data_len == 0)
    {
        return 0;
    }
    enter_critical_section();

    uint16_t start = g_uart_rx_io.start;
    uint16_t end = g_uart_rx_io.end;
    uint32_t available = (RX_BUFF_SIZE + end - start) % RX_BUFF_SIZE; // Calculate the length of data that can be read

    if (available == 0)
    {
        leave_critical_section();
        return 0;  // No new data to read
    }

    byte_cnt = (available < p_data_len) ? available : p_data_len;

    if (start + byte_cnt < RX_BUFF_SIZE)
    {
        memcpy(p_data, g_uart_rx_io.uart_cache + start, byte_cnt);
        g_uart_rx_io.start = (start + byte_cnt) % RX_BUFF_SIZE;  // Update start pointer after read
    }
    else
    {
        uint32_t first_part = RX_BUFF_SIZE - start;
        memcpy(p_data, g_uart_rx_io.uart_cache + start, first_part);

        uint32_t second_part = byte_cnt - first_part;
        if (second_part > 0)
        {
            memcpy(p_data + first_part, g_uart_rx_io.uart_cache, second_part);
        }

        g_uart_rx_io.start = second_part;  // update start pointer after wraparound
    }

    g_uart_rx_io.recvLen = (RX_BUFF_SIZE + g_uart_rx_io.start - g_uart_rx_io.end) % RX_BUFF_SIZE;

    leave_critical_section();

    return byte_cnt;
}

static void __uart1_process()
{
    static uint8_t tmp_buff[128];
    uint16_t len = 0;


    len = __uart_read(tmp_buff, 128);

    // util_log_mem(UTIL_LOG_INFO, "  ", (uint8_t *)tmp_buff, len, 0);
    app_uart_command_parse((uint8_t *)&tmp_buff, len);

}

static void __uart_task(void *para)
{
    uart1_event_t sevent = UART1_EVENT_NONE;
    hosal_uart_init(&uart1_dev);
    NVIC_SetPriority(Uart1_IRQn, 6);

    hosal_uart_callback_set(&uart1_dev, HOSAL_UART_RX_CALLBACK, __uart_rx_callback, &uart1_dev);
    /* Configure UART to interrupt mode */
    hosal_uart_ioctl(&uart1_dev, HOSAL_UART_MODE_SET, (void *)HOSAL_UART_MODE_INT_RX);


    for (;;)
    {
        UART1_GET_NOTIFY(sevent);

        if ((UART1_EVENT_UART_IN & sevent))
        {
            __uart1_process();
        }

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

void uart1_task_start()
{
    xTaskCreate(__uart_task, "UART1", 512, NULL, TASK_PRIORITY_APP, &sUartTask);
}