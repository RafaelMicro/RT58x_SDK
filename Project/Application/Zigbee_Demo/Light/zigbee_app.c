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


#include "fota_define.h"

//=============================================================================
//                Private Global Variables
//=============================================================================
/*! Active scan duration, valid range 0 ~ 14, (15.36ms * (2^SD +1)) ms in one channel.  */
uint8_t ZB_RAF_SCAN_DURATION = 5;

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

void scene_db_check(void)
{
    ds_rw_t scene_dataset_read;
    scene_dataset_read.type = DS_TYPE_SCENE;
    ds_read(&scene_dataset_read);
    memcpy(&scene_table_db, (uint8_t *)scene_dataset_read.address, scene_dataset_read.len);
}

void scene_db_update(void)
{
    ds_rw_t scene_dataset_write;
    scene_dataset_write.type = DS_TYPE_SCENE;
    scene_dataset_write.len = sizeof(scene_db_t);
    scene_dataset_write.address = (uint32_t) &scene_table_db;
    ds_write(&scene_dataset_write);
}
void startup_db_check(void)
{
    ds_rw_t startup_dataset_read;
    startup_dataset_read.type = DS_TYPE_STARTUP_ENTRY;
    ds_read(&startup_dataset_read);
    memcpy(&startup_db, (uint8_t *)startup_dataset_read.address, startup_dataset_read.len);
}

void startup_db_update(void)
{
    ds_rw_t startup_dataset_write;
    startup_dataset_write.type = DS_TYPE_STARTUP_ENTRY;
    startup_dataset_write.len = sizeof(startup_entry_t);
    startup_dataset_write.address = (uint32_t) &startup_db;
    ds_write(&startup_dataset_write);
}
static void set_startup_status(void)
{
    uint8_t current_level, onoff_stat;
    /* set onoff*/
    switch (startup_db.startup_onoff)
    {
    case 0:
        onoff_stat = 0;
        break;
    case 1:
        onoff_stat = 1;
        break;
    case 2:
        onoff_stat = (startup_db.last_onoff_stat == 0) ? 1 : 0;
        break;
    case 0xff:
        onoff_stat = startup_db.last_onoff_stat;
        break;
    default:
        onoff_stat = 0;
        break;
    }
    /* set current level*/
    switch (startup_db.start_up_current_level)
    {
    case 0:
        current_level = 1;
        break;
    case 0xff:
        current_level = startup_db.last_level;
        break;
    default:
        current_level = startup_db.start_up_current_level;
        break;
    }

    set_startup_onoff(startup_db.startup_onoff);
    set_start_up_current_level(startup_db.start_up_current_level);
    set_on_off_status(onoff_stat);
    set_current_level(current_level);

    if (onoff_stat == 1)
    {
        set_duty_cycle(&pwm_para_config[0], current_level);
    }
    else
    {
        set_duty_cycle(&pwm_para_config[0], 0);
    }
    info("startup status: onoff = %d, level = %d\n", onoff_stat, current_level);
    startup_db.last_onoff_stat = onoff_stat;
    startup_db.last_level = current_level;
    startup_db_update();
}

static void app_ota_status_chk(void)
{
    uint32_t file_ver;
    fota_information_t t_bootloader_ota_info = {0};
    memcpy(&t_bootloader_ota_info, (uint8_t *)FOTA_UPDATE_BANK_INFO_ADDRESS, sizeof(t_bootloader_ota_info));
    file_ver = (uint32_t)t_bootloader_ota_info.reserved[0];

    do
    {
        if (!bsp_button_state_get(BSP_BUTTON_0))
        {
            memset(&t_bootloader_ota_info, 0xFF, sizeof(t_bootloader_ota_info));

            if (file_ver == 0xFFFFFFFF)
            {
                file_ver = z_get_file_version();
            }

            t_bootloader_ota_info.reserved[0] = file_ver;
        }

        if (t_bootloader_ota_info.fotabank_ready == FOTA_IMAGE_READY)
        {
            if (t_bootloader_ota_info.fota_result == FOTA_RESULT_SUCCESS)
            {
                file_ver = (uint32_t)t_bootloader_ota_info.reserved[0];
                z_set_file_version(file_ver);
                t_bootloader_ota_info.fotabank_ready++;
            }
        }
        else if (t_bootloader_ota_info.fotabank_ready == FOTA_IMAGE_READY + 1)
        {
            file_ver = (uint32_t)t_bootloader_ota_info.reserved[0];
            if (file_ver == 0xFFFFFFFF)
            {
                file_ver = z_get_file_version();
                t_bootloader_ota_info.reserved[0] = file_ver;
            }
            z_set_file_version(file_ver);
            break;
        }
        else
        {
            //file_ver = z_get_file_version();
            if (file_ver == 0xFFFFFFFF)
            {
                file_ver = z_get_file_version();
            }
            else
            {
                file_ver = (uint32_t)t_bootloader_ota_info.reserved[0];
                z_set_file_version(file_ver);
            }
            t_bootloader_ota_info.reserved[0] = file_ver;
            t_bootloader_ota_info.fotabank_ready = FOTA_IMAGE_READY + 1;
        }

        while (flash_check_busy());
        flash_erase(FLASH_ERASE_SECTOR, FOTA_UPDATE_BANK_INFO_ADDRESS);
        while (flash_check_busy());
        flash_write_page((uint32_t)&t_bootloader_ota_info, FOTA_UPDATE_BANK_INFO_ADDRESS);

    } while (0);


    info("ver : %08X\n", t_bootloader_ota_info.reserved[0]);

}
//=============================================================================
//                Private Global Variables
//=============================================================================
static sys_queue_t app_msg_q;
static zigbee_cfg_t gt_app_cfg;
static uint32_t app_event_state;
static uint32_t gu32_request_ota = 0;

static TimerHandle_t tmr_100ms;
static uint32_t gu32_join_state = 0;
extern uint8_t reset_to_default;

//=============================================================================
//                Functions
//=============================================================================

static void tmr_100ms_cb(TimerHandle_t t_timer)
{
    if (gu32_join_state)
    {
        bsp_led_Off(BSP_LED_1);
        xTimerStop(t_timer, 0);
    }
    else
    {
        bsp_led_toggle(BSP_LED_1);
    }
    if (gu32_request_ota)
    {
        zigbee_ota_client_start(0, 10);
        gu32_request_ota = 0;
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
        gu32_request_ota = 1;
        scene_db_check();
        startup_db_check();
        set_startup_status();
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
                if (t_app_q.pin == BSP_EVENT_BUTTONS_3)
                {
                    zigbee_send_permit_join(short_addr, 180);
                    zigbee_send_permit_join(0xfffc, 180);
                }
                if (t_app_q.pin == BSP_EVENT_BUTTONS_4)
                {
                    zigbee_finding_binding_req(LIGHT_EP, ZB_FB_TARGET_ROLE);
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
    bsp_init(BSP_INIT_DEBUG_CONSOLE, NULL);
    bsp_init((BSP_INIT_LEDS | BSP_INIT_BUTTONS), app_bsp_event_handle);

    /* Retarget stdout for utility & initial utility logging */
    utility_register_stdout(bsp_console_stdout_char, bsp_console_stdout_string);
    util_log_init();

#if (MODULE_ENABLE(SUPPORT_DEBUG_CONSOLE_CLI))
    extern int cli_console_init(void);
    cli_console_init();
#endif

    util_log_on(UTIL_LOG_PROTOCOL);
    //util_log_on(UTIL_LOG_DEBUG);

    /* flash dataset init*/
    ds_config_t app_ds_config;
    app_ds_config.start_address = DEVICE_DB_START_ADDRESS;
    app_ds_config.end_address = DEVICE_DB_END_ADDRESS;
    ds_initinal(app_ds_config);

    app_ota_status_chk();

    gt_app_cfg.p_zigbee_device_contex_t = &simple_desc_light_ctx;
    gt_app_cfg.pf_evt_indication = app_evt_indication_cb;

    info_color(LOG_BLUE, "Initial ZigBee stack\n");
    zigbee_stack_init(&gt_app_cfg);

    sys_queue_new(&app_msg_q, 16, sizeof(app_queue_t));

    info("Create app task\n");
    sys_task_new("app", app_main_task, NULL, 256, TASK_PRIORITY_APP);


}

