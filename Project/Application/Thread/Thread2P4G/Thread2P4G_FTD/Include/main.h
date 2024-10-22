#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/platform/logging.h>
#include <openthread/thread.h>
#include <openthread/thread_ftd.h>
#include <openthread/udp.h>
#include <openthread/logging.h>
#include <openthread/cli.h>

#include "openthread-system.h"
#include "cli/cli_config.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "cm3_mcu.h"
#include "mem_mgmt.h"

/* Utility Library APIs */
#include "util_log.h"
#include "util_printf.h"

#if PLAFFORM_CONFIG_ENABLE_SUBG == TRUE
#define DEF_CHANNEL 1
#else
#define DEF_CHANNEL 11
#endif

/*app_task.c*/
void app_task_init();
void app_task_process_action();
void app_task_exit();
otInstance *otGetInstance();
bool app_task_check_leader_pin_state();

/*app_udp.c*/
uint8_t app_sock_init(otInstance *instance);
otError app_udp_send(otIp6Address dst_addr, uint8_t *data, uint16_t data_lens);

/*app_uart.c*/
#define UART_HANDLER_PARSER_CB_NUM  3

typedef enum
{
    UART_DATA_VALID = 0,
    UART_DATA_VALID_CRC_OK,
    UART_DATA_INVALID,
    UART_DATA_CS_ERROR,
} uart_handler_data_sts_t;

typedef uart_handler_data_sts_t (*uart_parser_cb)(uint8_t *pBuf, uint16_t plen, uint16_t *datalen, uint16_t *offset);
typedef void (*uart_recv_cb)(uint8_t *pBuf, uint16_t plen);

typedef struct UART_HANDLER_PARM_T
{
    uart_parser_cb UartParserCB[UART_HANDLER_PARSER_CB_NUM];
    uart_recv_cb UartRecvCB[UART_HANDLER_PARSER_CB_NUM];
} uart_handler_parm_t;

void app_uart_init();
void app_uart_recv();
int app_uart_data_send(uint8_t u_port, uint8_t *p_data, uint16_t data_len);

#ifdef __cplusplus
};
#endif
#endif