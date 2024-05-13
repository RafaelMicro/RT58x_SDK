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


//=============================================================================
//                Private Global Variables
//=============================================================================
/*! Active scan duration, valid range 0 ~ 14, (15.36ms * (2^SD +1)) ms in one channel.  */
uint8_t ZB_RAF_SCAN_DURATION = 5;
uint8_t join_fail_cnt = 0;
#define APP_KEEP_ALIVE_TIMEOUT 3000
#define MAX_JOIN_FAIL_CNT 3


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

extern uint8_t reset_to_default;
//static uint32_t button_0_state;
//=============================================================================
//                Functions
//=============================================================================
static void app_main_loop(uint32_t event)
{
    uint8_t  extPANID[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  //address all zero is able to join to any suitable PAN

    switch (event)
    {
    case APP_INIT_EVT:
        if (zigbee_ed_nwk_start_request(ZIGBEE_CHANNEL_ALL_MASK(), 0x0 /*ZIGBEE_CHANNEL_MASK(26)*/, false, APP_KEEP_ALIVE_TIMEOUT, extPANID, reset_to_default) == 0)
        {
            app_event_state = APP_IDLE_EVT;
        }
        break;

    case APP_NOT_JOINED_EVT:
        join_fail_cnt++;
        if (join_fail_cnt > MAX_JOIN_FAIL_CNT)
        {
            info("Suspend join\n");
            bsp_led_Off(BSP_LED_1);
            Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP3);
            Lpm_Enter_Low_Power_Mode();
        }
        else
        {
            zigbee_join_request();
        }
        app_event_state = APP_IDLE_EVT;
        break;

    case APP_ZB_JOINED_EVT:
        bsp_led_Off(BSP_LED_1);
        app_event_state = APP_IDLE_EVT;
        break;

    case APP_ZB_REJOIN_START_EVT:
        info_color(LOG_YELLOW, "Rejoin Start!\n");
        break;

    default:
        break;
    }
}
static void app_main_task(void *arg)
{
    app_queue_t t_app_q;
    app_event_state = APP_INIT_EVT;
    bsp_led_on(BSP_LED_1);

    for (;;)
    {
        app_main_loop(app_event_state);
        if (sys_queue_recv(&app_msg_q, &t_app_q, 0) != SYS_ARCH_TIMEOUT)
        {
            if (t_app_q.event == APP_QUEUE_ISR_BUTTON_EVT)
            {
                if (t_app_q.pin == BSP_EVENT_BUTTONS_4)
                {
                    zigbee_finding_binding_req(2, ZB_FB_INITIATOR_ROLE);
                }
            }
            if (t_app_q.event == APP_QUEUE_ZIGBEE_EVT)
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
    //util_log_on(UTIL_LOG_PROTOCOL);

    gt_app_cfg.p_zigbee_device_contex_t = &simple_desc_light_sensor_ctx;
    gt_app_cfg.pf_evt_indication = app_evt_indication_cb;

    info_color(LOG_BLUE, "Initial ZigBee stack\n");
    zigbee_stack_init(&gt_app_cfg);

    sys_queue_new(&app_msg_q, 16, sizeof(app_queue_t));

    info("Create app task\n");
    sys_task_new("app", app_main_task, NULL, 256, TASK_PRIORITY_APP);
}

