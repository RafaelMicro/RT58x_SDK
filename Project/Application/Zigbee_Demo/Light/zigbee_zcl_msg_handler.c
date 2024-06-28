/**
 * Copyright (c) 2021 All Rights Reserved.
 */
/** @file zigbee_zcl_msg_handler.c
 *
 * @author Rex
 * @version 0.1
 * @date 2021/12/23
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
#include "mfs.h"

/* ZigBee Stack Library APIs */
#include "zigbee_stack_api.h"
#include "zigbee_app.h"
#include "zigbee_lib_api.h"
#include "util_string.h"

#include "bsp_led.h"

scene_db_t scene_table_db;
startup_entry_t startup_db;
static TimerHandle_t tmr_level_ctrl, tmr_identify;
static uint8_t target_level, identify_onoff;

//identify effect variable
static TimerHandle_t tmr_id_effect;
static uint8_t id_effect_onoff, cmd_effect, tmr_id_effect_total_cnt, tmr_id_effect_cnt, finish_effect_flag;
//global scene control variable
static uint8_t global_scene_ctrl = 1;
static TimerHandle_t tmr_on_with_timed_off;
uint16_t off_effect_counter;
//off effect variable
static uint8_t effect_id, effect_var;
static float org_dim, current_dim, step;
static TimerHandle_t tmr_off_effect;

//=============================================================================
//                Private Function Declaration
//=============================================================================

static int get_scene_count(void)
{
    uint8_t cur_idx, scene_cnt;

    for (cur_idx = 0; cur_idx < SCENE_TABLE_SIZE; cur_idx++)
    {
        if (scene_table_db.scene_table[cur_idx].occupied)
        {
            scene_cnt++;
        }
    }
    return scene_cnt;
}
static int get_group_table_idx(uint16_t group_id)
{
    uint8_t cur_idx;
    for (cur_idx = 0; cur_idx < GROUP_TABLE_SIZE; cur_idx++)
    {
        if (scene_table_db.group_table[cur_idx] == group_id)
        {
            return cur_idx;
        }
    }
    return -1;
}
static int get_scene_table_idx(uint16_t group_id, uint8_t scene_id)
{
    uint8_t cur_idx;

    for (cur_idx = 0; cur_idx < SCENE_TABLE_SIZE; cur_idx++)
    {
        if (scene_table_db.scene_table[cur_idx].occupied
                && scene_table_db.scene_table[cur_idx].group_id == group_id
                && scene_table_db.scene_table[cur_idx].scene_id == scene_id)
        {
            return cur_idx;
        }
    }
    return -1;
}
static int valid_group_table_idx(uint16_t group_id)
{
    int valid_group_idx;
    uint8_t cur_idx;
    valid_group_idx = -1;
    for (cur_idx = 0; cur_idx < GROUP_TABLE_SIZE; cur_idx++)
    {
        if (scene_table_db.group_table[cur_idx] == group_id)
        {
            valid_group_idx = cur_idx;
            break;
        }
        else if (scene_table_db.group_table[cur_idx] == 0 && valid_group_idx == -1)
        {
            valid_group_idx = cur_idx;
        }
    }
    return valid_group_idx;
}
static int valid_scene_table_idx(uint16_t group_id, uint8_t scene_id)
{
    int valid_scene_idx;
    uint8_t cur_idx;

    valid_scene_idx = -1;

    for (cur_idx = 0; cur_idx < SCENE_TABLE_SIZE; cur_idx++)
    {
        if (scene_table_db.scene_table[cur_idx].occupied
                && scene_table_db.scene_table[cur_idx].group_id == group_id
                && scene_table_db.scene_table[cur_idx].scene_id == scene_id)
        {
            valid_scene_idx = cur_idx;
            break;
        }
        else if (!scene_table_db.scene_table[cur_idx].occupied && valid_scene_idx < 0)
        {
            valid_scene_idx = cur_idx;
        }
    }
    return valid_scene_idx;
}

static void tmr_level_ctrl_cb(TimerHandle_t t_timer)
{
    uint8_t current_lv;
    current_lv = get_current_level();

    if (get_on_off_status())
    {
        set_duty_cycle(&pwm_para_config[0], current_lv);
        global_scene_ctrl = 1;
    }
    else
    {
        set_duty_cycle(&pwm_para_config[0], 0);
    }

    if (current_lv == target_level)
    {
        info("Move to level complete : %d\n", current_lv);
        info("----------------------------\n\n");
        xTimerStop(tmr_level_ctrl, 0);
        startup_db.last_level = current_lv;
        startup_db.last_onoff_stat = get_on_off_status();
        startup_db_update();
    }
    else
    {
        xTimerStart(tmr_level_ctrl, 0);
    }
}
static void tmr_identify_cb(TimerHandle_t t_timer)
{
    uint8_t current_lv;
    uint16_t remaining_time;
    current_lv = get_current_level();
    remaining_time = get_identify_time();
    if (identify_onoff == 0)
    {
        set_duty_cycle(&pwm_para_config[0], 128);
        identify_onoff = 1;
    }
    else
    {
        set_duty_cycle(&pwm_para_config[0], 0);
        identify_onoff = 0;
    }

    if (remaining_time == 0)
    {

        info("Identify complete\n");

        if (get_on_off_status())
        {
            set_duty_cycle(&pwm_para_config[0], current_lv);
        }
        else
        {
            set_duty_cycle(&pwm_para_config[0], 0);
        }
        xTimerStop(tmr_identify, 0);
    }
    else
    {
        xTimerStart(tmr_identify, 0);
    }
}

static void tmr_identify_effect_cb(TimerHandle_t t_timer)
{
    tmr_id_effect_cnt++;
    if (((tmr_id_effect_cnt > tmr_id_effect_total_cnt) || finish_effect_flag == 1) && (id_effect_onoff == 0))
    {
        finish_effect_flag = 0;
        tmr_id_effect_cnt = 0;
        if (get_on_off_status())
        {
            set_duty_cycle(&pwm_para_config[0], get_current_level());
        }
        else
        {
            set_duty_cycle(&pwm_para_config[0], 0);
        }
        info("Identify complete\n");
        xTimerStop(t_timer, 0);
    }
    else
    {
        switch (cmd_effect)
        {
        case ZB_ZCL_IDENTIFY_EFFECT_ID_BLINK:
        case ZB_ZCL_IDENTIFY_EFFECT_ID_BREATHE:
        case ZB_ZCL_IDENTIFY_EFFECT_ID_OKAY:
            if (id_effect_onoff == 0)
            {
                set_duty_cycle(&pwm_para_config[0], 255);
                id_effect_onoff = 1;
            }
            else
            {
                set_duty_cycle(&pwm_para_config[0], 0);
                id_effect_onoff = 0;
            }
            break;
        case ZB_ZCL_IDENTIFY_EFFECT_ID_CHANNEL_CHANGE:
            id_effect_onoff = 0;
            set_duty_cycle(&pwm_para_config[0], 1);
            break;
        default:
            break;
        }
        xTimerStart(t_timer, 0);
    }
}
static void tmr_on_with_timed_off_cb(TimerHandle_t t_timer)
{
    if (get_on_off_status())
    {
        set_duty_cycle(&pwm_para_config[0], get_current_level());
    }
    else
    {
        set_duty_cycle(&pwm_para_config[0], 0);
    }
    if (get_on_off_off_wait_time() == 0 && get_on_off_on_time() == 0)
    {
        xTimerStop(t_timer, 0);
    }
    else
    {
        xTimerStart(t_timer, 0);
    }
}
static void tmr_off_effect_cb(TimerHandle_t t_timer)
{
    if (effect_id == ZB_ZCL_ON_OFF_EFFECT_ID_DELAYED_ALL_OFF)
    {
        if (effect_var == ZB_ZCL_ON_OFF_EFFECT_VARIANT_FADE)
        {
            current_dim += step;
            if (current_dim < 0)
            {
                current_dim = 0;
                set_duty_cycle(&pwm_para_config[0], (uint8_t) current_dim);
                xTimerStop(t_timer, 0);
            }
            else
            {
                set_duty_cycle(&pwm_para_config[0], (uint8_t) current_dim);
                xTimerStart(t_timer, 0);
            }
        }
        else if (effect_var == ZB_ZCL_ON_OFF_EFFECT_VARIANT_50PART_FADE)
        {
            current_dim += step;
            if (current_dim < 0)
            {
                current_dim = 0;
                set_duty_cycle(&pwm_para_config[0], (uint8_t) current_dim);
                xTimerStop(t_timer, 0);
            }
            else if (current_dim < org_dim / 2)
            {
                step = -org_dim / 240;
                set_duty_cycle(&pwm_para_config[0], (uint8_t) current_dim);
                xTimerStart(t_timer, 0);
            }
            else
            {
                set_duty_cycle(&pwm_para_config[0], (uint8_t) current_dim);
                xTimerStart(t_timer, 0);
            }
        }
    }
    else if (effect_id == ZB_ZCL_ON_OFF_EFFECT_ID_DYING_LIGHT)
    {
        if (effect_var == ZB_ZCL_ON_OFF_EFFECT_VARIANT_20PART_FADE)
        {
            current_dim += step;
            if (current_dim > 254)
            {
                current_dim = 254;
                step = -254 / 10;
                xTimerStart(t_timer, 0);
            }
            else if (current_dim > 1.2 * org_dim)
            {
                step = -org_dim / 12;
                xTimerStart(t_timer, 0);
            }
            if (current_dim < 0)
            {
                current_dim = 0;
                set_duty_cycle(&pwm_para_config[0], current_dim);
                xTimerStop(t_timer, 0);
            }
            else
            {
                set_duty_cycle(&pwm_para_config[0], current_dim);
                xTimerStart(t_timer, 0);
            }
        }
    }
}
static void _zcl_common_command_process(uint16_t cmd, uint16_t datalen, uint8_t *pdata, uint32_t clusterID)
{
    do
    {
        if (cmd == ZB_ZCL_CMD_WRITE_ATTRIB ||
                cmd == ZB_ZCL_CMD_WRITE_ATTRIB_UNDIV ||
                cmd == ZB_ZCL_CMD_WRITE_ATTRIB_RESP ||
                cmd == ZB_ZCL_CMD_WRITE_ATTRIB_NO_RESP)
        {
            if (clusterID == ZB_ZCL_CLUSTER_ID_IDENTIFY && get_identify_time() != 0)
            {
                info("Identify process start, duration = %d\n", get_identify_time());

                if (!tmr_identify)
                {
                    tmr_identify = xTimerCreate("t_id", pdMS_TO_TICKS(200), pdFALSE, (void *)0, tmr_identify_cb);
                }
                if (!xTimerIsTimerActive(tmr_identify))
                {
                    xTimerStart(tmr_identify, 0);
                }
            }
            else if (clusterID == ZB_ZCL_CLUSTER_ID_ON_OFF)
            {

                if ((pdata[0] | pdata[1] << 8) == ZB_ZCL_ATTR_ON_OFF_START_UP_ON_OFF)
                {
                    startup_db.startup_onoff = pdata[3];
                    set_startup_onoff(pdata[3]);
                    startup_db_update();
                }
            }
            else if (clusterID == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL)
            {
                if ((pdata[0] | pdata[1] << 8) == ZB_ZCL_ATTR_LEVEL_CONTROL_START_UP_CURRENT_LEVEL_ID)
                {
                    startup_db.start_up_current_level = pdata[3];
                    set_start_up_current_level(pdata[3]);
                    startup_db_update();
                }
            }

        }
    } while (0);
}
static void _zcl_basic_process(uint16_t cmd, uint16_t datalen, uint8_t *pdata)
{
    if (cmd == ZB_ZCL_CMD_BASIC_RESET_ID)
    {
        taskENTER_CRITICAL();
        reset_attr();
        taskEXIT_CRITICAL();
        set_duty_cycle(&pwm_para_config[0], 0);
        for (int cur_idx = 0; cur_idx < SCENE_TABLE_SIZE; cur_idx++)
        {
            scene_table_db.scene_table[cur_idx].occupied = ZB_FALSE;
        }
        scene_db_update();
        startup_db.start_up_current_level = 0;
        startup_db.startup_onoff = 0;
        startup_db_update();
    }
}
static void _zcl_indentify_process(uint16_t cmd, uint16_t datalen, uint8_t *pdata, uint32_t dir)
{
    if (cmd == ZB_ZCL_CMD_IDENTIFY_IDENTIFY_ID && dir == ZCL_FRAME_CLIENT_SERVER_DIR)
    {
        info("Identify process start, duration = %d\n", pdata[0] | (pdata[1] << 8));
        if (tmr_id_effect && xTimerIsTimerActive(tmr_id_effect))
        {
            xTimerStop(tmr_id_effect, 0);
        }
        if (!tmr_identify)
        {
            tmr_identify = xTimerCreate("t_id", pdMS_TO_TICKS(200), pdFALSE, (void *)0, tmr_identify_cb);
        }
        if (!xTimerIsTimerActive(tmr_identify))
        {
            xTimerStart(tmr_identify, 0);
        }
    }
    else if (cmd == ZB_ZCL_CMD_IDENTIFY_TRIGGER_EFFECT_ID && dir == ZCL_FRAME_CLIENT_SERVER_DIR)
    {
        if (tmr_identify && xTimerIsTimerActive(tmr_identify))
        {
            xTimerStop(tmr_identify, 0);
        }
        if (!tmr_id_effect)
        {
            tmr_id_effect = xTimerCreate("t_ef", pdMS_TO_TICKS(200), pdFALSE, (void *)0, tmr_identify_effect_cb);
        }

        switch (pdata[0])
        {
        case ZB_ZCL_IDENTIFY_EFFECT_ID_BLINK:
            cmd_effect = ZB_ZCL_IDENTIFY_EFFECT_ID_BLINK;
            finish_effect_flag = 0;
            tmr_id_effect_cnt = 0;
            tmr_id_effect_total_cnt = 2;
            set_duty_cycle(&pwm_para_config[0], 0);
            if (xTimerIsTimerActive(tmr_id_effect))
            {
                xTimerStop(tmr_id_effect, 0);
            }
            xTimerChangePeriod(tmr_id_effect, pdMS_TO_TICKS(200), pdMS_TO_TICKS(200));
            break;
        case ZB_ZCL_IDENTIFY_EFFECT_ID_BREATHE:
            cmd_effect = ZB_ZCL_IDENTIFY_EFFECT_ID_BREATHE;
            finish_effect_flag = 0;
            tmr_id_effect_cnt = 0;
            tmr_id_effect_total_cnt = 30;
            set_duty_cycle(&pwm_para_config[0], 0);
            if (xTimerIsTimerActive(tmr_id_effect))
            {
                xTimerStop(tmr_id_effect, 0);
            }
            xTimerChangePeriod(tmr_id_effect, pdMS_TO_TICKS(500), pdMS_TO_TICKS(500));
            break;
        case ZB_ZCL_IDENTIFY_EFFECT_ID_OKAY:
            cmd_effect = ZB_ZCL_IDENTIFY_EFFECT_ID_OKAY;
            finish_effect_flag = 0;
            tmr_id_effect_cnt = 0;
            tmr_id_effect_total_cnt = 4;
            set_duty_cycle(&pwm_para_config[0], 0);
            if (xTimerIsTimerActive(tmr_id_effect))
            {
                xTimerStop(tmr_id_effect, 0);
            }
            xTimerChangePeriod(tmr_id_effect, pdMS_TO_TICKS(200), pdMS_TO_TICKS(200));
            break;
        case ZB_ZCL_IDENTIFY_EFFECT_ID_CHANNEL_CHANGE:
            cmd_effect = ZB_ZCL_IDENTIFY_EFFECT_ID_CHANNEL_CHANGE;
            finish_effect_flag = 0;
            tmr_id_effect_cnt = 0;
            tmr_id_effect_total_cnt = 16;
            set_duty_cycle(&pwm_para_config[0], 255);
            if (xTimerIsTimerActive(tmr_id_effect))
            {
                xTimerStop(tmr_id_effect, 0);
            }
            xTimerChangePeriod(tmr_id_effect, pdMS_TO_TICKS(500), pdMS_TO_TICKS(500));
            break;
        case ZB_ZCL_IDENTIFY_EFFECT_ID_FINISH_EFFECT:
            finish_effect_flag = 1;
            break;
        case ZB_ZCL_IDENTIFY_EFFECT_ID_STOP:
            info("Identify complete\n");
            if (xTimerIsTimerActive(tmr_id_effect))
            {
                xTimerStop(tmr_id_effect, 0);
            }
            if (get_on_off_status())
            {
                set_duty_cycle(&pwm_para_config[0], get_current_level());
            }
            else
            {
                set_duty_cycle(&pwm_para_config[0], 0);
            }
            break;
        default:
            break;
        }
    }
}
static void _zcl_onoff_process(uint16_t cmd, uint16_t datalen, uint8_t *pdata)
{
    do
    {
        uint8_t current_lv;
        current_lv = get_current_level();
        if (cmd == ZB_ZCL_CMD_ON_OFF_OFF_ID ||
                cmd == ZB_ZCL_CMD_ON_OFF_ON_ID  ||
                cmd == ZB_ZCL_CMD_ON_OFF_TOGGLE_ID )
        {
            const char *status[3] = {"OFF", "ON", "Toggle"};

            info_color(LOG_GREEN, "%s\n", status[cmd]);
            if (get_on_off_status())
            {
                set_duty_cycle(&pwm_para_config[0], current_lv);
                global_scene_ctrl = 1;
            }
            else
            {
                set_duty_cycle(&pwm_para_config[0], 0);
            }
            startup_db.last_onoff_stat = get_on_off_status();
            startup_db_update();
        }
        else if (cmd == ZB_ZCL_CMD_ON_OFF_OFF_WITH_EFFECT_ID)
        {
            effect_id = pdata[0];
            effect_var = pdata[1];
            if (global_scene_ctrl == 1)
            {
                global_scene_ctrl = 0;
                scene_table_db.global_scene.onoff_stat = 1;
                scene_table_db.global_scene.level = current_lv;
                scene_db_update();
                info("store global scene OK\n");
            }
            else if (global_scene_ctrl == 0)
            {
                set_duty_cycle(&pwm_para_config[0], 0);
                break;
            }
            if (!tmr_off_effect)
            {
                tmr_off_effect = xTimerCreate("t_off", pdMS_TO_TICKS(100), pdFALSE, (void *)0, tmr_off_effect_cb);
            }
            if (xTimerIsTimerActive(tmr_off_effect))
            {
                xTimerStop(tmr_off_effect, 0);
            }

            org_dim = current_lv;
            current_dim = org_dim;
            if (effect_id == ZB_ZCL_ON_OFF_EFFECT_ID_DELAYED_ALL_OFF)
            {
                if (effect_var == ZB_ZCL_ON_OFF_EFFECT_VARIANT_FADE)
                {
                    step = -org_dim / 8;
                    xTimerStart(tmr_off_effect, 100);
                }
                else if (effect_var == ZB_ZCL_ON_OFF_EFFECT_VARIANT_NO_FADE)
                {
                    set_duty_cycle(&pwm_para_config[0], 0);
                }
                else if (effect_var == ZB_ZCL_ON_OFF_EFFECT_VARIANT_50PART_FADE)
                {
                    step = -org_dim / 16;
                    xTimerStart(tmr_off_effect, 100);
                }
            }
            else if (effect_id == ZB_ZCL_ON_OFF_EFFECT_ID_DYING_LIGHT)
            {
                if (effect_var == ZB_ZCL_ON_OFF_EFFECT_VARIANT_20PART_FADE)
                {
                    step = org_dim / 25;
                    xTimerStart(tmr_off_effect, 100);
                }
            }

        }
        else if (cmd == ZB_ZCL_CMD_ON_OFF_ON_WITH_RECALL_GLOBAL_SCENE_ID && !global_scene_ctrl)
        {
            scene_db_check();
            while (flash_check_busy()) {};
            info("recall global scene: onoff= %d, current level=%d\n", scene_table_db.global_scene.onoff_stat, scene_table_db.global_scene.level);
            global_scene_ctrl = 1;
            set_on_off_status(scene_table_db.global_scene.onoff_stat);
            set_current_level(scene_table_db.global_scene.level);
            if (scene_table_db.global_scene.onoff_stat)
            {
                set_duty_cycle(&pwm_para_config[0], scene_table_db.global_scene.level);
            }
            else
            {
                set_duty_cycle(&pwm_para_config[0], 0);
            }
        }
        else if (cmd == ZB_ZCL_CMD_ON_OFF_ON_WITH_TIMED_OFF_ID)
        {
            if (!tmr_on_with_timed_off)
            {
                tmr_on_with_timed_off = xTimerCreate("t_ow", pdMS_TO_TICKS(100), pdFALSE, (void *)0, tmr_on_with_timed_off_cb);
            }
            if (!xTimerIsTimerActive(tmr_on_with_timed_off))
            {
                xTimerStart(tmr_on_with_timed_off, 0);
            }
        }
    } while (0);
}
static void _zcl_group_process(uint16_t cmd, uint16_t datalen, uint8_t *pdata)
{
    do
    {
        if (cmd == ZB_ZCL_CMD_GROUPS_ADD_GROUP || (cmd == ZB_ZCL_CMD_GROUPS_ADD_GROUP_IF_IDENTIFYING && get_identify_time() > 0))
        {
            int valid_idx;
            uint16_t group_id;
            group_id = pdata[0] | (pdata[1] << 8);
            if (group_id > 0xfff7 || group_id < 0x0001)
            {
                info("invalid value!! \n");
                break;
            }
            valid_idx = valid_group_table_idx(group_id);
            if (valid_idx != -1)
            {
                scene_table_db.group_table[valid_idx] = group_id;
            }

            info("add group complete\n");
            scene_db_update();
        }
        else if (cmd == ZB_ZCL_CMD_GROUPS_REMOVE_GROUP)
        {

            int valid_idx;
            uint16_t group_id;
            group_id = pdata[0] | (pdata[1] << 8);
            if (group_id > 0xfff7 || group_id < 0x0001)
            {
                break;
            }
            valid_idx = get_group_table_idx(group_id);
            if (valid_idx != -1)
            {
                scene_table_db.group_table[valid_idx] = 0;
            }
            for (int i = 0; i < SCENE_TABLE_SIZE; i++)
            {
                if (scene_table_db.scene_table[i].occupied
                        && scene_table_db.scene_table[i].group_id == group_id)
                {
                    info("remove scene id: %d, group id: %d\n", scene_table_db.scene_table[i].scene_id, scene_table_db.scene_table[i].group_id);
                    scene_table_db.scene_table[i].occupied = ZB_FALSE;
                }
            }
            scene_db_update();
        }
        else if (cmd == ZB_ZCL_CMD_GROUPS_REMOVE_ALL_GROUPS)
        {

            int i;
            for (i = 0; i < GROUP_TABLE_SIZE; i++)
            {
                scene_table_db.group_table[i] = 0;
            }
            for (i = 0; i < SCENE_TABLE_SIZE; i++)
            {
                scene_table_db.scene_table[i].occupied = ZB_FALSE;
            }
            info("remove all scenes complete\n");
            scene_db_update();
        }
    } while (0);
}
static void _zcl_level_ctrl_process(uint16_t cmd, uint16_t datalen, uint8_t *pdata)
{
    uint8_t current_lv;
    uint8_t temp_option, optionmask, optionmaskoverride;
    current_lv = get_current_level();
    temp_option = get_level_option();

    do
    {
        if (!tmr_level_ctrl)
        {
            tmr_level_ctrl = xTimerCreate("t_lv", pdMS_TO_TICKS(20), pdFALSE, (void *)0, tmr_level_ctrl_cb);
        }
        if (cmd == ZB_ZCL_CMD_LEVEL_CONTROL_STEP ||
                cmd == ZB_ZCL_CMD_LEVEL_CONTROL_STEP_WITH_ON_OFF)
        {
            if (datalen == 6)
            {
                optionmask = pdata[4];
                optionmaskoverride = pdata[5];
                info("optionmask %x\noptionoverride %x\n", optionmask, optionmaskoverride);
                temp_option = (optionmask & optionmaskoverride) | (~optionmask & get_level_option());
            }
            if (cmd == ZB_ZCL_CMD_LEVEL_CONTROL_STEP
                    && get_on_off_status() == 0
                    && !(temp_option & 0x1))
            {
                info("Do not execute!!\n");
                break;
            }
            if (pdata[0] == 0)
            {
                info_color(LOG_YELLOW, "Move up step :%d\n", pdata[1]);
                current_lv = (current_lv > 0) ? current_lv - 1 : 0;
                if ((current_lv + pdata[1]) < ZB_ZCL_LEVEL_CONTROL_LEVEL_MAX_VALUE)
                {
                    target_level = current_lv + pdata[1];
                }
                else
                {
                    target_level = ZB_ZCL_LEVEL_CONTROL_LEVEL_MAX_VALUE;
                }

            }
            else
            {
                info_color(LOG_CYAN, "Move down step :%d\n", pdata[1]);
                current_lv = (current_lv < ZB_ZCL_LEVEL_CONTROL_LEVEL_MAX_VALUE) ? current_lv + 1 : ZB_ZCL_LEVEL_CONTROL_LEVEL_MAX_VALUE;
                if (current_lv > pdata[1])
                {
                    target_level =  current_lv - pdata[1];
                }
                else
                {
                    target_level = ZB_ZCL_LEVEL_CONTROL_LEVEL_MIN_VALUE;
                }

            }

            if (!xTimerIsTimerActive(tmr_level_ctrl))
            {
                xTimerStart(tmr_level_ctrl, 0);
            }

        }
        else if (cmd == ZB_ZCL_CMD_LEVEL_CONTROL_MOVE_TO_LEVEL ||
                 cmd == ZB_ZCL_CMD_LEVEL_CONTROL_MOVE_TO_LEVEL_WITH_ON_OFF)
        {
            if (datalen == 5)
            {
                optionmask = pdata[3];
                optionmaskoverride = pdata[4];
                info("optionmask %x\noptionoverride %x\n", optionmask, optionmaskoverride);
                temp_option = (optionmask & optionmaskoverride) | (~optionmask & get_level_option());
            }
            if (cmd == ZB_ZCL_CMD_LEVEL_CONTROL_MOVE_TO_LEVEL
                    && get_on_off_status() == 0
                    && !(temp_option & 0x1))
            {
                info("Do not execute!!\n");
                break;
            }
            info_color(LOG_CYAN, "Move to level :%d\n", pdata[0]);
            target_level = pdata[0];
            if (target_level < ZB_ZCL_LEVEL_CONTROL_LEVEL_MIN_VALUE)
            {
                target_level = ZB_ZCL_LEVEL_CONTROL_LEVEL_MIN_VALUE;
            }
            else if (target_level > ZB_ZCL_LEVEL_CONTROL_LEVEL_MAX_VALUE)
            {
                target_level = ZB_ZCL_LEVEL_CONTROL_LEVEL_MAX_VALUE;
            }

            if (!xTimerIsTimerActive(tmr_level_ctrl))
            {
                xTimerStart(tmr_level_ctrl, 0);
            }
        }
        else if (cmd == ZB_ZCL_CMD_LEVEL_CONTROL_MOVE ||
                 cmd == ZB_ZCL_CMD_LEVEL_CONTROL_MOVE_WITH_ON_OFF)
        {
            if (datalen == 4)
            {
                optionmask = pdata[2];
                optionmaskoverride = pdata[3];
                info("optionmask %x\noptionoverride %x\n", optionmask, optionmaskoverride);
                temp_option = (optionmask & optionmaskoverride) | (~optionmask & get_level_option());
            }
            if (cmd == ZB_ZCL_CMD_LEVEL_CONTROL_MOVE
                    && get_on_off_status() == 0
                    && !(temp_option & 0x1))
            {
                info("Do not execute!!\n");
                break;
            }
            if (pdata[0] == 0)
            {
                info_color(LOG_YELLOW, "Move up rate :%d\n", pdata[1]);
                current_lv = (current_lv > ZB_ZCL_LEVEL_CONTROL_LEVEL_MIN_VALUE) ? current_lv - 1 : ZB_ZCL_LEVEL_CONTROL_LEVEL_MIN_VALUE;
                target_level = ZB_ZCL_LEVEL_CONTROL_LEVEL_MAX_VALUE;

            }
            else
            {
                info_color(LOG_CYAN, "Move down rate :%d\n", pdata[1]);
                current_lv = (current_lv < ZB_ZCL_LEVEL_CONTROL_LEVEL_MAX_VALUE) ? current_lv + 1 : ZB_ZCL_LEVEL_CONTROL_LEVEL_MAX_VALUE;
                target_level = ZB_ZCL_LEVEL_CONTROL_LEVEL_MIN_VALUE;

            }

            if (!xTimerIsTimerActive(tmr_level_ctrl))
            {
                xTimerStart(tmr_level_ctrl, 0);
            }
        }
        else if (cmd == ZB_ZCL_CMD_LEVEL_CONTROL_STOP ||
                 cmd == ZB_ZCL_CMD_LEVEL_CONTROL_STOP_WITH_ON_OFF)
        {
            if (datalen == 2)
            {
                optionmask = pdata[0];
                optionmaskoverride = pdata[1];
                info("optionmask %x\noptionoverride %x\n", optionmask, optionmaskoverride);
                temp_option = (optionmask & optionmaskoverride) | (~optionmask & get_level_option());
            }
            if (cmd == ZB_ZCL_CMD_LEVEL_CONTROL_STOP
                    && get_on_off_status() == 0
                    && !(temp_option & 0x1))
            {
                info("Do not execute!!\n");
                break;
            }
            if (xTimerIsTimerActive(tmr_level_ctrl))
            {
                xTimerStop(tmr_level_ctrl, 0);
            }
            info("Level control stop, now level : %d\n", current_lv);
            startup_db.last_level = get_current_level();
            startup_db.last_onoff_stat = get_on_off_status();
            startup_db_update();
            break;
        }
    } while (0);
}
static void _zcl_scene_process(uint16_t cmd, uint16_t datalen, uint8_t *pdata, uint32_t srcAddr, uint32_t srcEndpint, uint32_t seqnum, uint32_t dstAddr, uint32_t disableDefaultRsp)
{
    uint16_t group_id;
    uint8_t status, scene_id;
    zigbee_zcl_data_req_t *pt_data_req;
    do
    {
        if (cmd == ZB_ZCL_CMD_SCENES_ADD_SCENE)
        {

            int  valid_scene_idx, cluster_idx, field_set_len;
            uint8_t level, onoff;

            group_id = pdata[0] | (pdata[1] << 8);
            scene_id = pdata[2];



            valid_scene_idx = valid_scene_table_idx(group_id, scene_id);
            level = 0;
            onoff = 0;

            if (group_id > 0xfff7 || group_id < 0x0001)
            {
                status = ZB_ZCL_STATUS_INVALID_VALUE;
                info("invalid value!! \n");
                info("\n");
            }
            else if (get_group_table_idx(group_id) == -1)
            {
                status = ZB_ZCL_STATUS_INVALID_FIELD;
                info("invalid field!! \n");
                info("\n");
            }
            else if (valid_scene_idx == -1)
            {
                status = ZB_ZCL_STATUS_INSUFF_SPACE;
                info("no enough space!! \n");
                info("\n");
            }
            else
            {
                status = ZB_ZCL_STATUS_SUCCESS;
                cluster_idx = 6 + pdata[5];
                while (cluster_idx < datalen)
                {
                    field_set_len = pdata[cluster_idx + 2];
                    if ( (pdata[cluster_idx] | (pdata[cluster_idx + 1] << 8)) == ZB_ZCL_CLUSTER_ID_ON_OFF)
                    {
                        for (int i = 0; i < field_set_len; i++)
                        {
                            onoff = pdata[cluster_idx + 3 + i] << i | onoff;
                        }
                    }
                    else if ( (pdata[cluster_idx] | (pdata[cluster_idx + 1] << 8)) == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL)
                    {
                        for (int i = 0; i < field_set_len; i++)
                        {
                            level = pdata[cluster_idx + 3 + i] << i | level;
                        }
                    }
                    cluster_idx = cluster_idx + 3 + field_set_len;
                }

                scene_table_db.scene_table[valid_scene_idx].occupied = ZB_TRUE;
                scene_table_db.scene_table[valid_scene_idx].group_id = group_id;
                scene_table_db.scene_table[valid_scene_idx].scene_id = scene_id;
                scene_table_db.scene_table[valid_scene_idx].scene_trans_time = pdata[3] | (pdata[4] << 8);
                scene_table_db.scene_table[valid_scene_idx].onoff_stat = onoff;
                scene_table_db.scene_table[valid_scene_idx].level = level;

                info("add scene: \n");
                info("scene_id= %d \n", scene_table_db.scene_table[valid_scene_idx].scene_id);
                info("level= %d \n", scene_table_db.scene_table[valid_scene_idx].level);
                info("onoff_stat= %d \n", scene_table_db.scene_table[valid_scene_idx].onoff_stat);

                //write file
                scene_db_update();
                info("add scene OK\n");
                info("\n");
            }
            set_scene_count(get_scene_count());
            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, LIGHT_EP,
                                ZB_ZCL_CLUSTER_ID_SCENES,
                                ZB_ZCL_CMD_SCENES_ADD_SCENE_RESPONSE,
                                TRUE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, 4)
            pt_data_req->sp_seq_num = 1;
            pt_data_req->seq_num = seqnum;
            if (pt_data_req)
            {
                pt_data_req->cmdFormat[0] = status;
                pt_data_req->cmdFormat[1] = group_id & 0xFF;
                pt_data_req->cmdFormat[2] = (group_id >> 8) & 0xFF;
                pt_data_req->cmdFormat[3] = scene_id & 0xFF;

                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }

            break;

        }
        else if (cmd == ZB_ZCL_CMD_SCENES_VIEW_SCENE)
        {
            group_id = pdata[0] | (pdata[1] << 8);
            scene_id = pdata[2];
            uint8_t status, cmdLen;
            int valid_scene_idx;




            valid_scene_idx = get_scene_table_idx(group_id, scene_id);

            if (group_id > 0xfff7 || group_id < 0x0001)
            {
                status = ZB_ZCL_STATUS_INVALID_VALUE;
                cmdLen = 4;
                info("invalid value!! \n");
                info("\n");
            }
            else if (get_group_table_idx(group_id) == -1)
            {
                status = ZB_ZCL_STATUS_INVALID_FIELD;
                cmdLen = 4;
                info("invalid field!! \n");
                info("\n");
            }
            else if (valid_scene_idx == -1)
            {
                status = ZB_ZCL_STATUS_NOT_FOUND;
                cmdLen = 4;
                info("invalid scene!!\n");
                info("\n");
            }
            else
            {
                status = ZB_ZCL_STATUS_SUCCESS;
                cmdLen = 15;
                info("view scene: \n");
                info("scene_id: %d \n", scene_table_db.scene_table[valid_scene_idx].scene_id);
                info("level: %d \n", scene_table_db.scene_table[valid_scene_idx].level);
                info("onoff_stat: %d \n", scene_table_db.scene_table[valid_scene_idx].onoff_stat);
                info("\n");
            }


            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, LIGHT_EP,
                                ZB_ZCL_CLUSTER_ID_SCENES,
                                ZB_ZCL_CMD_SCENES_VIEW_SCENE_RESPONSE,
                                TRUE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, cmdLen)
            pt_data_req->sp_seq_num = 1;
            pt_data_req->seq_num = seqnum;

            if (pt_data_req)
            {
                pt_data_req->cmdFormat[0] = status;
                pt_data_req->cmdFormat[1] = group_id & 0xFF;
                pt_data_req->cmdFormat[2] = (group_id >> 8) & 0xFF;
                pt_data_req->cmdFormat[3] = scene_id & 0xFF;
                if (status == ZB_ZCL_STATUS_SUCCESS)
                {
                    pt_data_req->cmdFormat[4] = scene_table_db.scene_table[valid_scene_idx].scene_trans_time & 0xFF;
                    pt_data_req->cmdFormat[5] = (scene_table_db.scene_table[valid_scene_idx].scene_trans_time >> 8) & 0xFF;
                    pt_data_req->cmdFormat[6] = 0x00;
                    pt_data_req->cmdFormat[7] = ZB_ZCL_CLUSTER_ID_ON_OFF & 0xFF;
                    pt_data_req->cmdFormat[8] = (ZB_ZCL_CLUSTER_ID_ON_OFF >> 8) & 0xFF;
                    pt_data_req->cmdFormat[9] = 0x01;
                    pt_data_req->cmdFormat[10] = scene_table_db.scene_table[valid_scene_idx].onoff_stat;
                    pt_data_req->cmdFormat[11] = ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL & 0xFF;
                    pt_data_req->cmdFormat[12] = (ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL >> 8) & 0xFF;
                    pt_data_req->cmdFormat[13] = 0x01;
                    pt_data_req->cmdFormat[14] = scene_table_db.scene_table[valid_scene_idx].level;
                }
                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }
            break;

        }
        else if (cmd == ZB_ZCL_CMD_SCENES_REMOVE_SCENE)
        {
            group_id = pdata[0] | (pdata[1] << 8);
            scene_id = pdata[2];
            int scene_idx;




            scene_idx = get_scene_table_idx(group_id, scene_id);
            if (group_id > 0xfff7 || group_id < 0x0001)
            {
                status = ZB_ZCL_STATUS_INVALID_VALUE;
                info("invalid value!! \n");
                info("\n");
            }
            else if (get_group_table_idx(group_id) == -1)
            {
                status = ZB_ZCL_STATUS_INVALID_FIELD;
                info("invalid field!! \n");
                info("\n");
            }
            else if (scene_idx == -1)
            {
                status = ZB_ZCL_STATUS_NOT_FOUND;
                info("invalid scene!! \n");
                info("\n");
            }
            else
            {
                status = ZB_ZCL_STATUS_SUCCESS;
                scene_table_db.scene_table[scene_idx].occupied = ZB_FALSE;
                info("remove scene: %d\n", scene_table_db.scene_table[scene_idx].scene_id);
                //write file
                scene_db_update();

                info("remove scene OK\n");
                info("\n");
            }
            set_scene_count(get_scene_count());
            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, LIGHT_EP,
                                ZB_ZCL_CLUSTER_ID_SCENES,
                                ZB_ZCL_CMD_SCENES_REMOVE_SCENE_RESPONSE,
                                TRUE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, 4)
            pt_data_req->sp_seq_num = 1;
            pt_data_req->seq_num = seqnum;

            if (pt_data_req)
            {
                pt_data_req->cmdFormat[0] = status;
                pt_data_req->cmdFormat[1] = group_id & 0xFF;
                pt_data_req->cmdFormat[2] = (group_id >> 8) & 0xFF;
                pt_data_req->cmdFormat[3] = scene_id & 0xFF;

                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }
            break;
        }
        else if (cmd == ZB_ZCL_CMD_SCENES_REMOVE_ALL_SCENES)
        {


            int cur_idx;
            group_id = pdata[0] | (pdata[1] << 8);


            if (group_id > 0xfff7 || group_id < 0x0001)
            {
                status = ZB_ZCL_STATUS_INVALID_VALUE;
                info("invalid value!! \n");
                info("\n");
            }
            else if (get_group_table_idx(group_id) == -1)
            {
                status = ZB_ZCL_STATUS_INVALID_FIELD;
                info("invalid field!! \n");
                info("\n");
            }
            else
            {
                status = ZB_ZCL_STATUS_SUCCESS;
                for (cur_idx = 0; cur_idx < SCENE_TABLE_SIZE; cur_idx++)
                {
                    if (scene_table_db.scene_table[cur_idx].occupied
                            && scene_table_db.scene_table[cur_idx].group_id == group_id)
                    {
                        info("remove scene id: %d, group id: %d\n", scene_table_db.scene_table[cur_idx].scene_id, scene_table_db.scene_table[cur_idx].group_id);
                        scene_table_db.scene_table[cur_idx].occupied = ZB_FALSE;
                    }
                }
                //write file
                scene_db_update();
                info("remove all scene OK\n");
                info("\n");
            }
            set_scene_count(get_scene_count());
            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, LIGHT_EP,
                                ZB_ZCL_CLUSTER_ID_SCENES,
                                ZB_ZCL_CMD_SCENES_REMOVE_ALL_SCENES_RESPONSE,
                                TRUE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, 3)
            pt_data_req->sp_seq_num = 1;
            pt_data_req->seq_num = seqnum;

            if (pt_data_req)
            {
                pt_data_req->cmdFormat[0] = status;
                pt_data_req->cmdFormat[1] = group_id & 0xFF;
                pt_data_req->cmdFormat[2] = (group_id >> 8) & 0xFF;

                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }
            break;
        }
        else if (cmd == ZB_ZCL_CMD_SCENES_STORE_SCENE)
        {



            int valid_scene_idx;

            group_id = pdata[0] | (pdata[1] << 8);
            scene_id = pdata[2];

            valid_scene_idx = valid_scene_table_idx(group_id, scene_id);

            if (group_id > 0xfff7 || group_id < 0x0001)
            {
                status = ZB_ZCL_STATUS_INVALID_VALUE;
                info("invalid value!! \n");
                info("\n");
            }
            else if (get_group_table_idx(group_id) == -1)
            {
                status = ZB_ZCL_STATUS_INVALID_FIELD;
                info("invalid field!! \n");
                info("\n");
            }
            else if (valid_scene_idx == -1)
            {
                status = ZB_ZCL_STATUS_INSUFF_SPACE;
                info("no enough space!! \n");
                info("\n");
            }
            else
            {
                status = ZB_ZCL_STATUS_SUCCESS;
                scene_table_db.scene_table[valid_scene_idx].occupied = ZB_TRUE;
                scene_table_db.scene_table[valid_scene_idx].group_id = group_id;
                scene_table_db.scene_table[valid_scene_idx].scene_id = scene_id;
                scene_table_db.scene_table[valid_scene_idx].scene_trans_time = 0x0000;
                scene_table_db.scene_table[valid_scene_idx].onoff_stat = get_on_off_status();
                scene_table_db.scene_table[valid_scene_idx].level = get_current_level();

                info("store scene: \n");
                info("scene_id: %d \n", scene_table_db.scene_table[valid_scene_idx].scene_id);
                info("level: %d \n", scene_table_db.scene_table[valid_scene_idx].level);
                info("onoff_stat: %d \n", scene_table_db.scene_table[valid_scene_idx].onoff_stat);

                //write file
                scene_db_update();
                info("store scene OK\n");
                info("\n");
            }
            set_scene_count(get_scene_count());
            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, LIGHT_EP,
                                ZB_ZCL_CLUSTER_ID_SCENES,
                                ZB_ZCL_CMD_SCENES_STORE_SCENE_RESPONSE,
                                TRUE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, 4)
            pt_data_req->sp_seq_num = 1;
            pt_data_req->seq_num = seqnum;

            if (pt_data_req)
            {
                pt_data_req->cmdFormat[0] = status;
                pt_data_req->cmdFormat[1] = group_id & 0xFF;
                pt_data_req->cmdFormat[2] = (group_id >> 8) & 0xFF;
                pt_data_req->cmdFormat[3] = scene_id;

                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }
            break;
        }
        else if (cmd == ZB_ZCL_CMD_SCENES_RECALL_SCENE)
        {
            int scene_idx;


            group_id = pdata[0] | (pdata[1] << 8);
            scene_id = pdata[2];

            scene_idx = get_scene_table_idx(group_id, scene_id);

            if (group_id > 0xfff7 || group_id < 0x0001)
            {
                info("invalid value!! \n");
                status = ZB_ZCL_STATUS_INVALID_VALUE;
            }
            else if (get_group_table_idx(group_id) == -1)
            {
                info("invalid field!! \n");
                status = ZB_ZCL_STATUS_INVALID_FIELD;
                info("\n");
            }
            else if (scene_idx == -1)
            {
                info("invalid scene!! \n");
                status = ZB_ZCL_STATUS_NOT_FOUND;
            }
            else
            {
                status = ZB_ZCL_STATUS_SUCCESS;

                uint32_t level, onoff;
                level = scene_table_db.scene_table[scene_idx].level;
                onoff = scene_table_db.scene_table[scene_idx].onoff_stat;
                set_current_level(level);
                set_on_off_status(onoff);
                startup_db.last_level = get_current_level();
                startup_db.last_onoff_stat = get_on_off_status();
                startup_db_update();
                info("recall scene: \n");
                info("scene_id: %d \n", scene_table_db.scene_table[scene_idx].scene_id);
                info("level: %d \n", scene_table_db.scene_table[scene_idx].level);
                info("onoff_stat: %d \n", scene_table_db.scene_table[scene_idx].onoff_stat);
                info("\n");
                target_level = level;

                if (!tmr_level_ctrl)
                {
                    tmr_level_ctrl = xTimerCreate("t_lv", pdMS_TO_TICKS(20), pdFALSE, (void *)0, tmr_level_ctrl_cb);
                }
                if (!xTimerIsTimerActive(tmr_level_ctrl))
                {
                    xTimerStart(tmr_level_ctrl, 0);
                }
            }

            if (dstAddr == short_addr && ((status != ZB_ZCL_STATUS_SUCCESS) || (status == ZB_ZCL_STATUS_SUCCESS && disableDefaultRsp == FALSE)))
            {
                ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, LIGHT_EP,
                                    ZB_ZCL_CLUSTER_ID_SCENES,
                                    ZB_ZCL_CMD_DEFAULT_RESP,
                                    FALSE, TRUE,
                                    ZCL_FRAME_SERVER_CLIENT_DIR, 0, 2)
                pt_data_req->sp_seq_num = 1;
                pt_data_req->seq_num = seqnum;
                if (pt_data_req)
                {
                    pt_data_req->cmdFormat[0] = ZB_ZCL_CMD_SCENES_RECALL_SCENE;
                    pt_data_req->cmdFormat[1] = status;

                    zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                    sys_free(pt_data_req);
                }
            }
        }
        else if (cmd == ZB_ZCL_CMD_SCENES_GET_SCENE_MEMBERSHIP)
        {
            zigbee_zcl_data_req_t *pt_data_req;
            group_id = pdata[0] | (pdata[1] << 8);
            int cur_idx;
            uint8_t status, cmdLen, cap, scene_cnt;
            uint8_t scene_list[16];


            cap = 0;
            scene_cnt = 0;

            for (cur_idx = 0; cur_idx < SCENE_TABLE_SIZE; cur_idx++)
            {
                if (!scene_table_db.scene_table[cur_idx].occupied)
                {
                    cap++;
                }
                else if (scene_table_db.scene_table[cur_idx].occupied && scene_table_db.scene_table[cur_idx].group_id == group_id)
                {
                    if (scene_cnt < 16)
                    {
                        scene_cnt++;
                        scene_list[scene_cnt - 1] = scene_table_db.scene_table[cur_idx].scene_id;
                    }
                }
            }

            info("scene capability: %d \n", cap);
            info("\n");

            if (group_id > 0xfff7 || group_id < 0x0001)
            {
                status = ZB_ZCL_STATUS_INVALID_VALUE;
                info("invalid value!! \n");
                info("\n");
                cmdLen = 4;
            }
            else if (get_group_table_idx(group_id) == -1)
            {
                status = ZB_ZCL_STATUS_INVALID_FIELD;
                info("invalid field!! \n");
                info("\n");
                cmdLen = 4;
            }
            else
            {
                status = ZB_ZCL_STATUS_SUCCESS;
                cmdLen = 5 + scene_cnt;
            }

            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, LIGHT_EP,
                                ZB_ZCL_CLUSTER_ID_SCENES,
                                ZB_ZCL_CMD_SCENES_GET_SCENE_MEMBERSHIP_RESPONSE,
                                TRUE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, cmdLen)
            pt_data_req->sp_seq_num = 1;
            pt_data_req->seq_num = seqnum;

            if (pt_data_req)
            {
                pt_data_req->cmdFormat[0] = status;
                pt_data_req->cmdFormat[1] = cap;
                pt_data_req->cmdFormat[2] = group_id & 0xFF;
                pt_data_req->cmdFormat[3] = (group_id >> 8) & 0xFF;
                if (status == ZB_ZCL_STATUS_SUCCESS)
                {
                    pt_data_req->cmdFormat[4] = scene_cnt;

                    for (int i = 0; i < scene_cnt; i++)
                    {
                        pt_data_req->cmdFormat[5 + i] = scene_list[i];
                    }
                }
                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }
            break;
        }
        else if (cmd == ZB_ZCL_CMD_SCENES_ENHANCED_ADD_SCENE)
        {

            int  valid_scene_idx, cluster_idx, field_set_len;
            uint16_t scene_trans_time;
            uint8_t level, onoff;

            group_id = pdata[0] | (pdata[1] << 8);
            scene_id = pdata[2];



            valid_scene_idx = valid_scene_table_idx(group_id, scene_id);
            level = 0;
            onoff = 0;

            if (group_id > 0xfff7 || group_id < 0x0001)
            {
                status = ZB_ZCL_STATUS_INVALID_VALUE;
                info("invalid value!! \n");
                info("\n");
            }
            else if (get_group_table_idx(group_id) == -1)
            {
                status = ZB_ZCL_STATUS_INVALID_FIELD;
                info("invalid field!! \n");
                info("\n");
            }
            else if (valid_scene_idx == -1)
            {
                status = ZB_ZCL_STATUS_INSUFF_SPACE;
                info("no enough space!! \n");
                info("\n");
            }
            else
            {
                status = ZB_ZCL_STATUS_SUCCESS;
                cluster_idx = 6 + pdata[5];
                while (cluster_idx < datalen)
                {
                    field_set_len = pdata[cluster_idx + 2];
                    if ( (pdata[cluster_idx] | (pdata[cluster_idx + 1] << 8)) == ZB_ZCL_CLUSTER_ID_ON_OFF)
                    {
                        for (int i = 0; i < field_set_len; i++)
                        {
                            onoff = pdata[cluster_idx + 3 + i] << i | onoff;
                        }
                    }
                    else if ( (pdata[cluster_idx] | (pdata[cluster_idx + 1] << 8)) == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL)
                    {
                        for (int i = 0; i < field_set_len; i++)
                        {
                            level = pdata[cluster_idx + 3 + i] << i | level;
                        }
                    }
                    cluster_idx = cluster_idx + 3 + field_set_len;
                }
                scene_trans_time = pdata[3] | (pdata[4] << 8);
                scene_table_db.scene_table[valid_scene_idx].occupied = ZB_TRUE;
                scene_table_db.scene_table[valid_scene_idx].group_id = group_id;
                scene_table_db.scene_table[valid_scene_idx].scene_id = scene_id;
                scene_table_db.scene_table[valid_scene_idx].scene_trans_time = scene_trans_time / 10;
                scene_table_db.scene_table[valid_scene_idx].scene_trans_time_100ms = scene_trans_time % 10;
                scene_table_db.scene_table[valid_scene_idx].onoff_stat = onoff;
                scene_table_db.scene_table[valid_scene_idx].level = level;

                info("add scene: \n");
                info("scene_id= %d \n", scene_table_db.scene_table[valid_scene_idx].scene_id);
                info("level= %d \n", scene_table_db.scene_table[valid_scene_idx].level);
                info("onoff_stat= %d \n", scene_table_db.scene_table[valid_scene_idx].onoff_stat);

                //write file
                scene_db_update();
                info("add scene OK\n");
                info("\n");
            }
            set_scene_count(get_scene_count());
            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, LIGHT_EP,
                                ZB_ZCL_CLUSTER_ID_SCENES,
                                ZB_ZCL_CMD_SCENES_ENHANCED_ADD_SCENE_RESPONSE,
                                TRUE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, 4)
            pt_data_req->sp_seq_num = 1;
            pt_data_req->seq_num = seqnum;
            if (pt_data_req)
            {
                pt_data_req->cmdFormat[0] = status;
                pt_data_req->cmdFormat[1] = group_id & 0xFF;
                pt_data_req->cmdFormat[2] = (group_id >> 8) & 0xFF;
                pt_data_req->cmdFormat[3] = scene_id & 0xFF;

                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }

            break;

        }
        else if (cmd == ZB_ZCL_CMD_SCENES_ENHANCED_VIEW_SCENE)
        {
            group_id = pdata[0] | (pdata[1] << 8);
            scene_id = pdata[2];
            uint16_t scene_trans_time_100ms;
            uint8_t status, cmdLen;
            int valid_scene_idx;




            valid_scene_idx = get_scene_table_idx(group_id, scene_id);

            if (group_id > 0xfff7 || group_id < 0x0001)
            {
                status = ZB_ZCL_STATUS_INVALID_VALUE;
                cmdLen = 4;
                info("invalid value!! \n");
                info("\n");
            }
            else if (get_group_table_idx(group_id) == -1)
            {
                status = ZB_ZCL_STATUS_INVALID_FIELD;
                cmdLen = 4;
                info("invalid field!! \n");
                info("\n");
            }
            else if (valid_scene_idx == -1)
            {
                status = ZB_ZCL_STATUS_NOT_FOUND;
                cmdLen = 4;
                info("invalid scene!!\n");
                info("\n");
            }
            else
            {
                status = ZB_ZCL_STATUS_SUCCESS;
                cmdLen = 15;
                scene_trans_time_100ms = scene_table_db.scene_table[valid_scene_idx].scene_trans_time * 10 + scene_table_db.scene_table[valid_scene_idx].scene_trans_time_100ms;
                info("view scene: \n");
                info("scene_id: %d \n", scene_table_db.scene_table[valid_scene_idx].scene_id);
                info("level: %d \n", scene_table_db.scene_table[valid_scene_idx].level);
                info("onoff_stat: %d \n", scene_table_db.scene_table[valid_scene_idx].onoff_stat);
                info("\n");
            }


            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, LIGHT_EP,
                                ZB_ZCL_CLUSTER_ID_SCENES,
                                ZB_ZCL_CMD_SCENES_ENHANCED_VIEW_SCENE_RESPONSE,
                                TRUE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, cmdLen)
            pt_data_req->sp_seq_num = 1;
            pt_data_req->seq_num = seqnum;

            if (pt_data_req)
            {
                pt_data_req->cmdFormat[0] = status;
                pt_data_req->cmdFormat[1] = group_id & 0xFF;
                pt_data_req->cmdFormat[2] = (group_id >> 8) & 0xFF;
                pt_data_req->cmdFormat[3] = scene_id & 0xFF;
                if (status == ZB_ZCL_STATUS_SUCCESS)
                {
                    pt_data_req->cmdFormat[4] = scene_trans_time_100ms & 0xFF;
                    pt_data_req->cmdFormat[5] = (scene_trans_time_100ms >> 8) & 0xFF;
                    pt_data_req->cmdFormat[6] = 0x00;
                    pt_data_req->cmdFormat[7] = ZB_ZCL_CLUSTER_ID_ON_OFF & 0xFF;
                    pt_data_req->cmdFormat[8] = (ZB_ZCL_CLUSTER_ID_ON_OFF >> 8) & 0xFF;
                    pt_data_req->cmdFormat[9] = 0x01;
                    pt_data_req->cmdFormat[10] = scene_table_db.scene_table[valid_scene_idx].onoff_stat;
                    pt_data_req->cmdFormat[11] = ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL & 0xFF;
                    pt_data_req->cmdFormat[12] = (ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL >> 8) & 0xFF;
                    pt_data_req->cmdFormat[13] = 0x01;
                    pt_data_req->cmdFormat[14] = scene_table_db.scene_table[valid_scene_idx].level;
                }
                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }
            break;

        }
        else if (cmd == ZB_ZCL_CMD_SCENES_COPY_SCENE)
        {

            int  from_scene_idx, valid_scene_idx;
            uint16_t group_id_from, group_id_to;
            uint8_t mode, scene_id_from, scene_id_to, group_id_exist_cnt, empty_cnt, has_enough_space;

            mode = pdata[0] & 0x01;
            group_id_from = pdata[1] | (pdata[2] << 8);
            scene_id_from = pdata[3];
            group_id_to = pdata[4] | (pdata[5] << 8);
            scene_id_to = pdata[6];

            from_scene_idx = get_scene_table_idx(group_id_from, scene_id_from);
            valid_scene_idx = valid_scene_table_idx(group_id_to, scene_id_to);
            //check if enough space
            if (mode == 1)
            {
                for (int i = 0; i < SCENE_TABLE_SIZE; i++)
                {
                    if (scene_table_db.scene_table[i].occupied == true && scene_table_db.scene_table[i].group_id == group_id_from)
                    {
                        group_id_exist_cnt++;
                    }
                    else if (scene_table_db.scene_table[i].occupied == false)
                    {
                        empty_cnt++;
                    }
                }
                if (empty_cnt >= group_id_exist_cnt)
                {
                    has_enough_space = 1;
                }
            }
            else
            {
                if (valid_scene_idx != -1)
                {
                    has_enough_space = 1;
                }
            }

            if (group_id_from > 0xfff7 || group_id_from < 0x0001 || group_id_to > 0xfff7 || group_id_to < 0x0001)
            {
                status = ZB_ZCL_STATUS_INVALID_VALUE;
                info("invalid value!! \n");
                info("\n");
            }
            else if (get_group_table_idx(group_id_from) == -1 || get_group_table_idx(group_id_to) == -1)
            {
                status = ZB_ZCL_STATUS_INVALID_FIELD;
                info("invalid field!! \n");
                info("\n");
            }
            else if (from_scene_idx == -1)
            {
                status = ZB_ZCL_STATUS_NOT_FOUND;
                info("not found!! \n");
                info("\n");
            }
            else if (!has_enough_space)
            {
                status = ZB_ZCL_STATUS_INSUFF_SPACE;
                info("no enough space!! \n");
                info("\n");
            }
            else
            {
                status = ZB_ZCL_STATUS_SUCCESS;
                if (mode == 1)
                {
                    int new_scene_idx;
                    if (group_id_from != group_id_to)
                    {
                        for (int i = 0; i < SCENE_TABLE_SIZE; i++ )
                        {
                            if (scene_table_db.scene_table[i].group_id == group_id_from)
                            {
                                new_scene_idx = valid_scene_table_idx(group_id_from, scene_table_db.scene_table[i].scene_id);
                                scene_table_db.scene_table[new_scene_idx].occupied = ZB_TRUE;
                                scene_table_db.scene_table[new_scene_idx].group_id = group_id_to;
                                scene_table_db.scene_table[new_scene_idx].scene_id = scene_table_db.scene_table[i].scene_id;
                                scene_table_db.scene_table[new_scene_idx].scene_trans_time = scene_table_db.scene_table[i].scene_trans_time;
                                scene_table_db.scene_table[new_scene_idx].onoff_stat = scene_table_db.scene_table[i].onoff_stat;
                                scene_table_db.scene_table[new_scene_idx].level = scene_table_db.scene_table[i].level;
                            }
                        }
                    }
                }
                else
                {
                    scene_table_db.scene_table[valid_scene_idx].occupied = ZB_TRUE;
                    scene_table_db.scene_table[valid_scene_idx].group_id = group_id_to;
                    scene_table_db.scene_table[valid_scene_idx].scene_id = scene_id_to;
                    scene_table_db.scene_table[valid_scene_idx].scene_trans_time = scene_table_db.scene_table[from_scene_idx].scene_trans_time;
                    scene_table_db.scene_table[valid_scene_idx].onoff_stat = scene_table_db.scene_table[from_scene_idx].onoff_stat;
                    scene_table_db.scene_table[valid_scene_idx].level = scene_table_db.scene_table[from_scene_idx].level;
                }
                //write file
                scene_db_update();
                info("add scene OK\n");
                info("\n");
            }
            set_scene_count(get_scene_count());
            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, LIGHT_EP,
                                ZB_ZCL_CLUSTER_ID_SCENES,
                                ZB_ZCL_CMD_SCENES_COPY_SCENE_RESPONSE,
                                TRUE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, 4)
            pt_data_req->sp_seq_num = 1;
            pt_data_req->seq_num = seqnum;
            if (pt_data_req)
            {
                pt_data_req->cmdFormat[0] = status;
                pt_data_req->cmdFormat[1] = group_id_from & 0xFF;
                pt_data_req->cmdFormat[2] = (group_id_from >> 8) & 0xFF;
                pt_data_req->cmdFormat[3] = scene_id_from & 0xFF;

                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }
            break;
        }
    } while (0);
}
void zigbee_zcl_msg_handler(sys_tlv_t *pt_tlv)
{
    zigbee_zcl_data_idc_t *pt_zcl_msg = (zigbee_zcl_data_idc_t *)pt_tlv->value;
    do
    {
        if (!pt_zcl_msg)
        {
            break;
        }
        if (pt_zcl_msg->dstEndpint != LIGHT_EP)
        {
            break;
        }
        //info("Recv ZCL message from 0x%04X\n", pt_zcl_msg->srcAddr);
        //info("Cluster %04x, cmd %d\n", pt_zcl_msg->clusterID, pt_zcl_msg->cmd);
        //util_log_mem(UTIL_LOG_INFO, "  ", (uint8_t *)pt_zcl_msg->cmdFormat, pt_zcl_msg->cmdFormatLen, 0);
        if (pt_zcl_msg->is_common_command == 1)
        {
            _zcl_common_command_process(pt_zcl_msg->cmd, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat, pt_zcl_msg->clusterID);
        }
        else if (pt_zcl_msg->is_common_command == 0)
        {
            switch (pt_zcl_msg->clusterID)
            {
            case ZB_ZCL_CLUSTER_ID_BASIC:
                _zcl_basic_process(pt_zcl_msg->cmd, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat);
                break;
            case ZB_ZCL_CLUSTER_ID_IDENTIFY:
                _zcl_indentify_process(pt_zcl_msg->cmd, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat, pt_zcl_msg->direction);
                break;
            case ZB_ZCL_CLUSTER_ID_SCENES:
                _zcl_scene_process(pt_zcl_msg->cmd, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat, pt_zcl_msg->srcAddr, pt_zcl_msg->srcEndpint, pt_zcl_msg->seq_num, pt_zcl_msg->dstAddr, pt_zcl_msg->disableDefaultRsp);
                break;
            case ZB_ZCL_CLUSTER_ID_GROUPS:
                _zcl_group_process(pt_zcl_msg->cmd, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat);
                break;
            case ZB_ZCL_CLUSTER_ID_ON_OFF:
                _zcl_onoff_process(pt_zcl_msg->cmd, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat);
                break;
            case ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
                _zcl_level_ctrl_process(pt_zcl_msg->cmd, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat);
                break;

            default:
                break;
            }

        }

    } while (0);
}
