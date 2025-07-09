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
#define DEF_CHANNEL 2
#else
#define DEF_CHANNEL 11
#endif

#define CFG_USE_CENTRAK_CONFIG 0

/*app_task.c*/
typedef enum
{
    APP_LED_ACK_STATE_OFF    = 0,
    APP_LED_ACK_STATE_ON,  // 1
    APP_LED_ACK_STATE_TOGGLE,  //2
} app_led_ack_t;

typedef struct
{
    otIp6Address src_ip;
    uint8_t led_status; //0:off, 1:on, 2:toggle
} app_led_ack_timer_para;

void app_task_init();
void app_task_process_action();
void app_task_exit();
otInstance *otGetInstance();
void app_set_led0_on(void);
void app_set_led0_off(void);
void app_set_led0_toggle(void);
void app_set_led0_flash(void);
void app_set_led1_on(void);
void app_set_led1_off(void);
void app_set_led1_toggle(void);

/*app_udp.c*/
uint8_t app_sock_init(otInstance *instance);
otError app_udp_send(otIp6Address dst_addr, uint8_t *data, uint16_t data_lens);
void app_udp_message_queue_process();

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
void app_mcu_runtime_record_init(void);

/*app_udp_comm.c*/
#define PATH_REQUEST_HEADER 0x88880080
#define PATH_RESPOND_HEADER 0x88888081
#define NET_MGM_NODE_ASK_HEADER 0x88880082
#define NET_MGM_NODE_REPLY_HEADER 0x88888083
#define NET_MGM_NODE_CHALLENGE_HEADER 0x88880084
#define NET_MGM_NODE_ACCEPTED_HEADER 0x88888085
#define NET_MGM_NODE_RESET_HEADER 0x88888086

void app_udp_comm_init(otInstance *aInstance);
bool app_udp_comm_header_check(uint8_t *head, uint8_t lens);
void app_udp_comm_process(uint8_t *data, uint16_t lens, otIp6Address src_addr);


/*app_path.c*/
void path_init();
void app_udp_comm_path_req_proc(uint8_t *data, uint16_t lens);
void app_udp_comm_path_resp_proc(uint8_t *data, uint16_t lens);
otError path_req_send(otIp6Address dst_addr, uint32_t timeout);

/*app_net_mgm.c*/
void net_mgm_init();
void net_mgm_debug_level(unsigned int level);
void net_mgm_node_reset_send(bool is_need_erase);

uint16_t nwk_mgm_node_num_display();
void net_mgm_node_table_display();
bool net_mgm_node_table_find(uint8_t *aExtAddress);
void net_mgm_node_table_display();
void app_udp_comm_net_mgm_node_ack_proc(uint8_t *data, uint16_t lens, otIp6Address src_addr);
void app_udp_comm_net_mgm_node_reply_proc(uint8_t *data, uint16_t lens, otIp6Address src_addr);
void app_udp_comm_net_mgm_node_challenge_proc(uint8_t *data, uint16_t lens, otIp6Address src_addr);
void app_udp_comm_net_mgm_node_accepted_proc(uint8_t *data, uint16_t lens, otIp6Address src_addr);
void app_udp_comm_net_mgm_node_reset_proc(uint8_t *data, uint16_t lens, otIp6Address src_addr);
void net_mgm_neighbor_table_change_cb(otNeighborTableEvent aEvent, const otNeighborTableEntryInfo *aEntryInfo);

#ifdef __cplusplus
};
#endif
#endif