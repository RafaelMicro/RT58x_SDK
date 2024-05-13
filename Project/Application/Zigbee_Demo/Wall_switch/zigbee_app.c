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
#include "task_pci.h"

/* BSP APIs */
#include "bsp.h"
#include "bsp_console.h"
#include "bsp_led.h"
#include "bsp_button.h"

/* MFS */
#include "mfs.h"

//=============================================================================
//                Private Global Variables
//=============================================================================
/*! Active scan duration, valid range 0 ~ 14, (15.36ms * (2^SD +1)) ms in one channel.  */
uint8_t ZB_RAF_SCAN_DURATION = 5;

//=============================================================================
//                Private Definitions of const value
//=============================================================================
/* Settings for the send step command */
#define LIGHT_CONTROL_DIMM_STEP 15
#define LIGHT_CONTROL_DIMM_TRANSACTION_TIME 2

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


const char *scene_name = "DB";
//=============================================================================
//                Private Function Declaration
//=============================================================================

void scene_db_check(void)
{
    /* Open or Create file */
    int fp;

    fp = fs_open(scene_name);

    fs_read(fp, (uint8_t *)&scene_table_db, sizeof(scene_table_db));

    fs_close(fp);
}

void scene_db_update(void)
{
    /* Open or Create file */
    int fp;

    fp = fs_open(scene_name);

    fs_seek(fp, 0, FS_SEEK_HEAD);
    fs_write(fp, (uint8_t *)&scene_table_db, sizeof(scene_table_db));

    fs_close(fp);
}
static void send_toggle(void)
{
    // on/off no payload
    zigbee_zcl_data_req_t *pt_data_req;

    ZIGBEE_ZCL_DATA_REQ(pt_data_req, 0, ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT, 0, WALL_SWITCH_EP, ZB_ZCL_CLUSTER_ID_ON_OFF,
                        ZB_ZCL_CMD_ON_OFF_TOGGLE_ID, TRUE, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR, 0, 0)


    if (pt_data_req)
    {
        zigbee_zcl_request(pt_data_req, 0);
        sys_free(pt_data_req);
    }
}
static void send_level_step(uint8_t dir)
{
    zigbee_zcl_data_req_t *pt_data_req;

    ZIGBEE_ZCL_DATA_REQ(pt_data_req, 0, ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT, 0, WALL_SWITCH_EP, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
                        ZB_ZCL_CMD_LEVEL_CONTROL_STEP_WITH_ON_OFF, TRUE, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR, 0, 4)


    if (pt_data_req)
    {
        pt_data_req->cmdFormat[0] = dir;
        pt_data_req->cmdFormat[1] = LIGHT_CONTROL_DIMM_STEP;
        pt_data_req->cmdFormat[2] = LIGHT_CONTROL_DIMM_TRANSACTION_TIME & 0xFF;
        pt_data_req->cmdFormat[3] = (LIGHT_CONTROL_DIMM_TRANSACTION_TIME >> 8) & 0xFF;
        zigbee_zcl_request(pt_data_req, 4);
        sys_free(pt_data_req);
    }
}
//=============================================================================
//                Private Global Variables
//=============================================================================
static sys_queue_t app_msg_q;
static zigbee_cfg_t gt_app_cfg;
static uint32_t app_event_state;

static TimerHandle_t tmr_100ms;
static uint32_t gu32_join_state = 0;
extern uint8_t reset_to_default;
//static uint32_t button_0_state;
//=============================================================================
//                Functions
//=============================================================================
static void tmr_100ms_cb(TimerHandle_t t_timer)
{
    if (gu32_join_state)
    {
        bsp_led_Off(BSP_LED_1);
    }
    else
    {
        bsp_led_toggle(BSP_LED_1);
    }
}
static void app_main_loop(uint32_t event)
{
    uint8_t  extPANID[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  //address all zero is able to join to any suitable PAN

    switch (event)
    {
    case APP_INIT_EVT:
        if (zigbee_nwk_start_request(ZIGBEE_DEVICE_ROLE_ROUTER, ZIGBEE_CHANNEL_ALL_MASK(), 0x0 /*ZIGBEE_CHANNEL_MASK(26)*/, 20, 0xFFFF, extPANID, 0) == 0)
        {
            app_event_state = APP_IDLE_EVT;
            if (reset_to_default)
            {
                zigbee_leave_req();
            }
        }
        break;

    case APP_NOT_JOINED_EVT:
        zigbee_join_request();
        app_event_state = APP_IDLE_EVT;
        break;

    case APP_ZB_JOINED_EVT:
        gu32_join_state = 1;
        scene_db_check();
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

    tmr_100ms = xTimerCreate("100ms", pdMS_TO_TICKS(100), pdTRUE, (void *)0, tmr_100ms_cb);
    xTimerStart(tmr_100ms, 0);

    for (;;)
    {
        app_main_loop(app_event_state);
        if (sys_queue_recv(&app_msg_q, &t_app_q, 20) != SYS_ARCH_TIMEOUT)
        {
            if (t_app_q.event == APP_QUEUE_ISR_BUTTON_EVT)
            {
                if (t_app_q.pin == BSP_EVENT_BUTTONS_0)
                {
                    send_toggle();
                }
                if (t_app_q.pin == BSP_EVENT_BUTTONS_1)
                {
                    send_level_step(0);
                }
                if (t_app_q.pin == BSP_EVENT_BUTTONS_2)
                {
                    send_level_step(1);
                }
            }
            else if (t_app_q.event == APP_QUEUE_ZIGBEE_EVT)
            {
                switch (t_app_q.pt_tlv->type)
                {
                case ZIGBEE_EVT_TYPE_ZCL_DATA_IDC:
                    zigbee_zcl_msg_handler(t_app_q.pt_tlv);
                    break;

                default:
                    zigbee_evt_handler(t_app_q.pt_tlv);
                    break;
                }
                if (t_app_q.pt_tlv)
                {
                    sys_free(t_app_q.pt_tlv);
                }
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
    app_queue_t t_app_q;
    switch (event)
    {
    case BSP_EVENT_BUTTONS_0:
    case BSP_EVENT_BUTTONS_1:
    case BSP_EVENT_BUTTONS_2:
    case BSP_EVENT_BUTTONS_3:
    case BSP_EVENT_BUTTONS_4:
        t_app_q.event = APP_QUEUE_ISR_BUTTON_EVT;
        t_app_q.pin = event;
        sys_queue_send_from_isr(&app_msg_q, &t_app_q);

        break;
    default:
        break;
    }
}

void zigbee_app_evt_change(uint32_t evt)
{
    app_event_state = evt;
}

void zigbee_app_init(void)
{
    /* Initil LED, Button, Console or UART */
    bsp_init((BSP_INIT_DEBUG_CONSOLE | BSP_INIT_LEDS), NULL);
    bsp_init(BSP_INIT_BUTTONS, app_bsp_event_handle);

    /* Retarget stdout for utility & initial utility logging */
    utility_register_stdout(bsp_console_stdout_char, bsp_console_stdout_string);
    util_log_init();

    gt_app_cfg.p_zigbee_device_contex_t = &simple_desc_switch_ctx;
    gt_app_cfg.pf_evt_indication = app_evt_indication_cb;

    info_color(LOG_BLUE, "Initial ZigBee stack\n");
    zigbee_stack_init(&gt_app_cfg);

    sys_queue_new(&app_msg_q, 16, sizeof(app_queue_t));

    info("Create app task\n");
    sys_task_new("app", app_main_task, NULL, 256, TASK_PRIORITY_APP);
    file_system_init();

}

