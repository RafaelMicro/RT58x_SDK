/**
 * Copyright (c) 2021 All Rights Reserved.
 */
/** @file zigbee_app.c
 *
 * @author Rex
 * @version 0.1
 * @date 2021/12/09
 * @license
 * @description
 */

//=============================================================================
//                Include
//=============================================================================
/* OS Wrapper APIs*/
#include "sys_arch.h"

/* Utility Library APIs */
#include "util_printf.h"
#include "util_log.h"

/* ZigBee Stack Library APIs */
#include "zigbee_stack_api.h"
#include "zigbee_app.h"

#include "zigbee_evt_handler.h"
#include "zigbee_zcl_msg_handler.h"
#include "zigbee_lib_api.h"

/* BSP APIs */
#include "bsp.h"
#include "bsp_console.h"
#include "bsp_led.h"
#include "bsp_button.h"

#include "mfs.h"

#include "uart_handler.h"
#include "timers.h"
//=============================================================================
//                Private Definitions of const value
//=============================================================================

//=============================================================================
//                Private ENUM
//=============================================================================

//=============================================================================
//                Private Struct
//=============================================================================
typedef struct
{
    sys_tlv_t *pt_tlv;
    uint32_t event;
    uint32_t pin;
} app_queue_t;


//=============================================================================
//                Private Function Declaration
//=============================================================================

//=============================================================================
//                Private Global Variables
//=============================================================================
static sys_queue_t app_msg_q;
static zigbee_cfg_t gt_app_cfg;
static uint32_t app_event_state;
static TimerHandle_t zb_time_timer;

#if (MODULE_ENABLE(USE_FILE_SYSTEM))
static const char *db_file = "DB";
#else
//#define RT58X_APP_DB_BASE_ADDR   0x900F0000
#define RT58X_APP_DB_BASE_ADDR   0x000F0000
#endif
//=============================================================================
//                Functions
//=============================================================================
//#if (MODULE_ENABLE(USE_FILE_SYSTEM))
#if 0
static void app_db_check(void)
{
    /* Open or Create file */
    int fp;

    fp = fs_open(db_file);

    fs_read(fp, (uint8_t *)&gt_app_db, sizeof(gt_app_db));

    fs_close(fp);
}

void app_db_update(void)
{
    /* Open or Create file */
    int fp;

    fp = fs_open(db_file);

    fs_seek(fp, 0, FS_SEEK_HEAD);
    fs_write(fp, (uint8_t *)&gt_app_db, sizeof(gt_app_db));

    fs_close(fp);
}
#else
void app_db_check(void)
{
    ds_rw_t app_dataset_read;
    uint32_t ds_status;

    app_dataset_read.type = DS_TYPE_DEVICE_INFO;
    ds_status = ds_read(&app_dataset_read);
    if (ds_status == STATUS_SUCCESS)
    {
        memcpy(&gt_app_db, (uint8_t *)app_dataset_read.address, app_dataset_read.len);
    }
    gt_app_db.device_number = zigbee_app_get_device_num();
}

void app_db_update(void)
{
    ds_rw_t app_dataset_write;
    uint8_t write_status;
    app_dataset_write.type = DS_TYPE_DEVICE_INFO;
    app_dataset_write.len = gt_app_db.device_number * sizeof(app_device_entry_t);
    app_dataset_write.address = (uint32_t) &gt_app_db;
    while (flash_check_busy()) {;}
    write_status = ds_write(&app_dataset_write);

    if (write_status != STATUS_SUCCESS)
    {
        info_color(LOG_RED, "DS write data fail!!! \n");
    }
}


void app_db_erase(void)
{
    ds_reset_to_default();
}
#endif

static void time_timer_handler(TimerHandle_t timer)
{
    uint32_t cur_time = get_gw_time();
    if (cur_time != ZB_ZCL_TIME_TIME_DEFAULT_VALUE)
    {
        set_gw_time(cur_time + 1);
    }
}
static void app_main_loop(uint32_t event)
{
    switch (event)
    {
    case APP_INIT_EVT:
        //if(zigbee_nwk_start_request(ZIGBEE_DEVICE_ROLE_CORDINATOR, ZIGBEE_CHANNEL_ALL_MASK(), 0x0, 20, 0x1234, !bsp_button_state_get(BSP_BUTTON_0)) == 0)
        app_event_state = APP_IDLE_EVT;
        break;

    case APP_IDLE_EVT:

        break;
    case APP_ZB_START_EVT:
        app_event_state = APP_IDLE_EVT;
        break;
    default:
        break;
    }
}
static void app_main_task(void *arg)
{
    app_queue_t t_app_q;
    app_event_state = APP_INIT_EVT;

    zb_time_timer = xTimerCreate("Time", pdMS_TO_TICKS(1000), pdTRUE, ( void * ) 0, time_timer_handler);
    xTimerStart(zb_time_timer, 0);
    for (;;)
    {
        app_main_loop(app_event_state);
        if (sys_queue_recv(&app_msg_q, &t_app_q, 1) != SYS_ARCH_TIMEOUT)
        {
            if (t_app_q.event == APP_QUEUE_ZIGBEE_EVT)
            {
                switch (t_app_q.pt_tlv->type)
                {
                case ZIGBEE_EVT_TYPE_ZCL_DATA_IDC:
                    zigbee_zcl_msg_handler(t_app_q.pt_tlv);
                    break;
                case ZIGBEE_EVT_TYPE_RAF_CMD_CONFIRM:
                    raf_cmd_cfm_handler(t_app_q.pt_tlv);
                    break;
                default:
                    zigbee_evt_handler(t_app_q.pt_tlv);
                    break;
                }
            }

            if (t_app_q.event == APP_QUEUE_UART_MSG_EVT)
            {
                zigbee_gateway_cmd_proc(t_app_q.pt_tlv->value, t_app_q.pt_tlv->length);
            }
            if (t_app_q.pt_tlv)
            {
                sys_free(t_app_q.pt_tlv);
            }
        }
    }
}

static void app_evt_indication_cb(uint32_t data_len)
{
    int i32_err;
    uint8_t *pBuf = sys_malloc(data_len);
    app_queue_t t_app_q;
    do
    {
        if (!pBuf)
        {
            break;
        }
        t_app_q.event = 0;
        i32_err = zigbee_event_msg_recvfrom(pBuf, &data_len);
        t_app_q.pt_tlv = (sys_tlv_t *)pBuf;
        if (i32_err == 0)
        {
            sys_queue_send_with_timeout(&app_msg_q, &t_app_q, 0);
        }
        else
        {
            info_color(LOG_RED, "[%s] sys_err = %d !\n", __func__, i32_err);
            sys_free(pBuf);
        }
    } while (0);
}

static void app_bsp_event_handle(bsp_event_t event)
{
    switch (event)
    {
    case BSP_EVENT_BUTTONS_0:
    case BSP_EVENT_BUTTONS_1:
    case BSP_EVENT_BUTTONS_2:
    case BSP_EVENT_BUTTONS_3:
    case BSP_EVENT_BUTTONS_4:

        break;
    default:
        break;
    }
}

static void zigbee_app_uart_msg_recv(sys_tlv_t *pt_tlv)
{
    app_queue_t t_app_q;

    t_app_q.event = APP_QUEUE_UART_MSG_EVT;
    t_app_q.pt_tlv = pt_tlv;

    sys_queue_send_with_timeout(&app_msg_q, &t_app_q, 0);
}
void zigbee_app_queue_send_to(uint32_t event, sys_tlv_t *pt_tlv)
{
    app_queue_t t_app_q;

    t_app_q.event = event;
    t_app_q.pt_tlv = pt_tlv;

    sys_queue_send_with_timeout(&app_msg_q, &t_app_q, 0);
}

void zigbee_app_evt_change(uint32_t evt)
{
    app_event_state = evt;
}

void zigbee_app_init(void)
{

    uart_handler_parm_t uart_handler_param = {0};
    /* Initil LED, Button, Console or UART */
    bsp_init(BSP_INIT_DEBUG_CONSOLE | BSP_INIT_UART, NULL);
    bsp_init((BSP_INIT_LEDS | BSP_INIT_BUTTONS), app_bsp_event_handle);

    /* Retarget stdout for utility & initial utility logging */
    utility_register_stdout(bsp_console_stdout_char, bsp_console_stdout_string);
    util_log_init();

#if (MODULE_ENABLE(SUPPORT_DEBUG_CONSOLE_CLI))
    extern int cli_console_init(void);
    cli_console_init();
#endif

    util_log_on(UTIL_LOG_PROTOCOL);

    /* flash dataset init*/
    ds_config_t app_ds_config;
    app_ds_config.start_address = DEVICE_DB_START_ADDRESS;
    app_ds_config.end_address = DEVICE_DB_END_ADDRESS;
    ds_initinal(app_ds_config);

    info_color(LOG_BLUE, "Initial ZigBee stack %p\n", &simple_desc_gateway_ctx);

    gt_app_cfg.p_zigbee_device_contex_t = &simple_desc_gateway_ctx;
    gt_app_cfg.pf_evt_indication = app_evt_indication_cb;
    zigbee_stack_init(&gt_app_cfg);

    sys_queue_new(&app_msg_q, 16, sizeof(app_queue_t));

    info("Create app task\n");
    sys_task_new("app", app_main_task, NULL, 512, TASK_PRIORITY_APP);


    uart_handler_param.UartParserCB[0] = zigbee_gateway_cmd_parser;
    uart_handler_param.UartRecvCB[0] = zigbee_app_uart_msg_recv;

    uart_handler_init(&uart_handler_param);
}

