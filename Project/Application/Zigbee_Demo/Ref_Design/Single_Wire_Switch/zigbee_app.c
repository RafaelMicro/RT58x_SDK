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
//                Private Definitions of const value
//=============================================================================
/* Settings for the send step command */
#define LIGHT_CONTROL_DIMM_STEP 15
#define LIGHT_CONTROL_DIMM_TRANSACTION_TIME 2
/*! Active scan duration, valid range 0 ~ 14, (15.36ms * (2^SD +1)) ms in one channel.  */
uint8_t ZB_RAF_SCAN_DURATION = 3;

//#define APP_KEEP_ALIVE_TIMEOUT  360*100   //for TJ Gateway test
//#define APP_KEEP_ALIVE_TIMEOUT  250 //for RT582 EVK Gateway test
#define APP_KEEP_ALIVE_TIMEOUT  500
uint8_t g_time_once = 0;
int g_button3_reset = 0;
uint8_t g_button3_count = 0;
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
//=============================================================================
//                Private Global Variables
//=============================================================================
static sys_queue_t app_msg_q;
static zigbee_cfg_t gt_app_cfg;
static uint32_t app_event_state;

static TimerHandle_t tmr_3000ms;
//static uint32_t gu32_join_state = 0;


static uint32_t button_0_state;
extern void  plug_gpio_init(void);
//=============================================================================
//                Functions
//=============================================================================
void Led_control_display(uint32_t Pin_Num, uint8_t count)
{
    int i;
    for (i = 0; i < count ; i++)
    {
        bsp_led_on(Pin_Num);
        Delay_ms(100);
        bsp_led_Off(Pin_Num);
        Delay_ms(250);
    }
}
static void app_reset_device(void *arg)
{
    info_color(LOG_WHITE, "app_reset_device ,reset flag:%d \r\n", g_button3_reset);
    uint8_t  extPANID[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  //address all zero is able to join to any suitable PAN

    if (g_button3_reset)
    {
        plug_gpio_init();
        Delay_ms(100);
        flash_erase(FLASH_ERASE_32K, 0x000F4000);
        info_color(LOG_WHITE, "erase flah done \r\n");
        Delay_ms(800);
        zigbee_ed_nwk_start_request(ZIGBEE_CHANNEL_ALL_MASK(), 0x0 /*ZIGBEE_CHANNEL_MASK(26)*/, false, APP_KEEP_ALIVE_TIMEOUT, extPANID, !g_button3_reset);
        info_color(LOG_WHITE, "device leave done \r\n");
    }
    Delay_ms(800);
    g_button3_reset = 0;
    __NVIC_SystemReset();
}

static void tmr_3000ms_cb(TimerHandle_t t_timer)
{
    g_time_once = 0 ;
    info_color(LOG_GREEN, "tmr_3000ms_cb \n");
    xTimerStop(tmr_3000ms, 0);
    if (g_button3_count == 3)
    {
        // reset
        g_button3_reset = 1;
        Led_control_display(BSP_LED_1, 5);
        sys_task_new("reset_device", app_reset_device, NULL, 128, TASK_PRIORITY_APP);
    }
#if 0
    else
    {
        info_color(LOG_GREEN, "g_button3_count :0x%02x \n", g_button3_count);
    }
#endif
    g_button3_count = 0;
}

static void app_main_loop(uint32_t event)
{
    uint8_t  extPANID[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  //address all zero is able to join to any suitable PAN

    switch (event)
    {
    case APP_INIT_EVT:
        if (zigbee_ed_nwk_start_request(ZIGBEE_CHANNEL_ALL_MASK(), 0x0 /*ZIGBEE_CHANNEL_MASK(26)*/, false, APP_KEEP_ALIVE_TIMEOUT, extPANID, 0) == 0)
        {
            if (!button_0_state)
            {
                memset(&scene_table_db, 0, sizeof(scene_table_db));
                scene_db_update();
                info("clear scene DB\n");
            }
            app_event_state = APP_IDLE_EVT;
        }
        break;

    case APP_NOT_JOINED_EVT:
        zigbee_join_request();
        app_event_state = APP_IDLE_EVT;
        break;

    case APP_ZB_JOINED_EVT:
        bsp_led_Off(BSP_LED_1);
        Led_control_display(BSP_LED_0, 3);
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
                if (t_app_q.pin == BSP_EVENT_BUTTONS_0)
                {
                    Light_key_onoff_process(BUTTON_1_EP, !get_on_off_status(BUTTON_1_EP));
                }
                if (t_app_q.pin == BSP_EVENT_BUTTONS_1)
                {
                    Light_key_onoff_process(BUTTON_2_EP, !get_on_off_status(BUTTON_2_EP));
                }
                if (t_app_q.pin == BSP_EVENT_BUTTONS_2)
                {
                    Light_key_onoff_process(BUTTON_3_EP, !get_on_off_status(BUTTON_3_EP));
                }
                if (t_app_q.pin == BSP_EVENT_BUTTONS_3)
                {
                    g_button3_count++;
                    info_color(LOG_WHITE, "button3_count(%d)\r\n", g_button3_count);
                    if (!g_time_once)
                    {
                        g_time_once = 1;
                        tmr_3000ms = xTimerCreate("3000ms", pdMS_TO_TICKS(600), pdFALSE, (void *)0, tmr_3000ms_cb);
                        if (xTimerIsTimerActive(tmr_3000ms) != pdTRUE )
                        {
                            info_color(LOG_WHITE, "timer is not activer!\r\n");
                            xTimerStart(tmr_3000ms, 0);
                        }
                    }
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
    case BSP_EVENT_BUTTONS_4:
        t_app_q.event = APP_QUEUE_ISR_BUTTON_EVT;
        t_app_q.pin = event;
        sys_queue_send_from_isr(&app_msg_q, &t_app_q);

        break;
    case BSP_EVENT_BUTTONS_3:
    {
        t_app_q.event = APP_QUEUE_ISR_BUTTON_EVT;
        t_app_q.pin = event;
        sys_queue_send_from_isr(&app_msg_q, &t_app_q);
        info_color(LOG_WHITE, "Button[%d] press!\r\n", event - BSP_EVENT_BUTTONS_0);
    }
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
    sys_task_new("app", app_main_task, NULL, 256, TASK_PRIORITY_PROTOCOL_NORMAL);
    file_system_init();

}

void zigbee_uart_init(void)
{
    /* Initil LED, Button, Console or UART */
    bsp_init((BSP_INIT_DEBUG_CONSOLE | BSP_INIT_LEDS), NULL);
    //bsp_init(BSP_INIT_BUTTONS, app_bsp_event_handle);

    /* Retarget stdout for utility & initial utility logging */
    utility_register_stdout(bsp_console_stdout_char, bsp_console_stdout_string);
    util_log_init();
}
