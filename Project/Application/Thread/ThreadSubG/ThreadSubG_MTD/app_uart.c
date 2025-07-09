/**
 * @file app_uart.c
 * @author
 * @brief
 * @version 0.1
 * @date 2022-03-24
 *
 * @copyright Copyright (c) 2022
 *
 */



//=============================================================================
//                Include
//=============================================================================
#include "util_queue.h"
#include "hosal_uart.h"
#include "main.h"
//=============================================================================
//                Private Definitions of const value
//=============================================================================
#define UART_HANDLER_RX_CACHE_SIZE          128
#define RX_BUFF_SIZE                        484
#define MAX_UART_BUFFER_SIZE                384
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

//=============================================================================
//                Private Function Declaration
//=============================================================================

//=============================================================================
//                Private Global Variables
//=============================================================================
static uart_io_t g_uart1_rx_io = { .start = 0, .end = 0, .recvLen = 0};
HOSAL_UART_DEV_DECL(uart1_dev, 1, 28, 29, UART_BAUDRATE_Baud115200)

static uint8_t uart_buf[MAX_UART_BUFFER_SIZE] = { 0 };
static uint8_t g_tmp_buff[RX_BUFF_SIZE];
//=============================================================================
//                Public Global Variables
//=============================================================================
//=============================================================================
//                Private Definition of Compare/Operation/Inline function/
//=============================================================================
//=============================================================================
//                Functions
//=============================================================================
/*uart 1 use*/
static int __uart1_read(uint8_t *p_data, uint32_t p_data_len)
{
    uint32_t byte_cnt = 0;

    if (p_data == NULL || p_data_len == 0)
    {
        return 0;
    }
    enter_critical_section();

    uint16_t start = g_uart1_rx_io.start;
    uint16_t end = g_uart1_rx_io.end;
    uint32_t available = (RX_BUFF_SIZE + end - start) % RX_BUFF_SIZE; // Calculate the length of data that can be read

    if (available == 0)
    {
        leave_critical_section();
        return 0;  // No new data to read
    }

    byte_cnt = (available < p_data_len) ? available : p_data_len;

    if (start + byte_cnt < RX_BUFF_SIZE)
    {
        memcpy(p_data, g_uart1_rx_io.uart_cache + start, byte_cnt);
        g_uart1_rx_io.start = (start + byte_cnt) % RX_BUFF_SIZE;  // Update start pointer after read
    }
    else
    {
        uint32_t first_part = RX_BUFF_SIZE - start;
        memcpy(p_data, g_uart1_rx_io.uart_cache + start, first_part);

        uint32_t second_part = byte_cnt - first_part;
        if (second_part > 0)
        {
            memcpy(p_data + first_part, g_uart1_rx_io.uart_cache, second_part);
        }

        g_uart1_rx_io.start = second_part;  // update start pointer after wraparound
    }

    g_uart1_rx_io.recvLen = (RX_BUFF_SIZE + g_uart1_rx_io.start - g_uart1_rx_io.end) % RX_BUFF_SIZE;

    leave_critical_section();

    return byte_cnt;
}

static int __uart1_rx_callback(void *p_arg)
{
    uint32_t new_data_len = 0;
    uint16_t start = g_uart1_rx_io.start;
    uint16_t end = g_uart1_rx_io.end;

    // Calculate the available buffer space for writing
    new_data_len = (start - end - 1 + RX_BUFF_SIZE) % RX_BUFF_SIZE;

    if (new_data_len == 0)
    {
        return 0;  // No space to write to
    }

    uint32_t received = hosal_uart_receive(p_arg, g_uart1_rx_io.uart_cache + end, new_data_len);
    g_uart1_rx_io.end = (g_uart1_rx_io.end + received) % RX_BUFF_SIZE;

    // Update receiving length
    g_uart1_rx_io.recvLen = (RX_BUFF_SIZE + g_uart1_rx_io.start - g_uart1_rx_io.end) % RX_BUFF_SIZE;

    return 0;
}

static int __uart1_break_callback(void *p_arg)
{
    /*can't printf*/
    return 0;
}

void app_uart1_recv()
{
    static uint16_t total_len = 0;
    static uint8_t rx_buf[UART_HANDLER_RX_CACHE_SIZE] = { 0 };
    int len;
    do
    {
        if (total_len >= MAX_UART_BUFFER_SIZE)
        {
            total_len = 0;
        }

        len = __uart1_read(rx_buf, UART_HANDLER_RX_CACHE_SIZE);
        if (len > 0)
        {
            uint32_t space_left = MAX_UART_BUFFER_SIZE - total_len;
            uint32_t data_to_copy = (len > space_left) ? space_left : len;

            memcpy(uart_buf + total_len, rx_buf, data_to_copy);
            total_len += data_to_copy;

            util_log_mem(UTIL_LOG_INFO, "UART1 Recv:", uart_buf, total_len, 0);
        }

    } while (0);
}

int app_uart_data_send(uint8_t u_port, uint8_t *p_data, uint16_t data_len)
{
    hosal_uart_send(&uart1_dev, p_data, data_len);

    return 0;
}

void app_uart_recv()
{
    app_uart1_recv();
}

void app_uart_init()
{
    /*Init UART In the first place*/
    hosal_uart_init(&uart1_dev);
    /* Configure UART Rx interrupt callback function */
    hosal_uart_callback_set(&uart1_dev, HOSAL_UART_RX_CALLBACK, __uart1_rx_callback, &uart1_dev);

    /* Configure UART to interrupt mode */
    hosal_uart_ioctl(&uart1_dev, HOSAL_UART_MODE_SET, (void *)HOSAL_UART_MODE_INT_RX);

    __NVIC_SetPriority(Uart1_IRQn, 2);
}