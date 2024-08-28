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
static TimerHandle_t tmr_identify;
extern uint16_t short_addr;
//=============================================================================
//                Private Function Declaration
//=============================================================================

static void tmr_identify_cb(TimerHandle_t t_timer)
{
    uint8_t remaining_time;
    remaining_time = get_identify_time();


    bsp_led_toggle(BSP_LED_0);

    if (remaining_time == 0)
    {

        info("Identify complete\n");
        bsp_led_Off(BSP_LED_0);
    }
    else
    {
        xTimerStart(tmr_identify, 0);
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
        bsp_led_Off(BSP_LED_0);
    }
}
static void _zcl_indentify_process(uint16_t cmd, uint16_t datalen, uint8_t *pdata, uint32_t dir)
{
    if (cmd == ZB_ZCL_CMD_IDENTIFY_IDENTIFY_ID && dir == ZCL_FRAME_CLIENT_SERVER_DIR)
    {
        info("Identify process start, duration = %d\n", pdata[0] | (pdata[1] << 8));

        if (!tmr_identify)
        {
            tmr_identify = xTimerCreate("t_id", pdMS_TO_TICKS(200), pdFALSE, (void *)0, tmr_identify_cb);
        }
        if (!xTimerIsTimerActive(tmr_identify))
        {
            xTimerStart(tmr_identify, 0);
        }
    }
}

static void _zcl_doorlock_process(uint16_t cmd, uint16_t datalen, uint8_t *pdata, uint32_t srcAddr, uint32_t srcEndpint, uint32_t seqnum, uint32_t dstAddr, uint32_t disableDefaultRsp)
{
    switch (cmd)
    {
    case ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR:
    case ZB_ZCL_CMD_DOOR_LOCK_UNLOCK_DOOR:
    case ZB_ZCL_CMD_DOOR_LOCK_TOGGLE:
    {
        char *c = (cmd == ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR) ? "LOCK" :
                  (cmd == ZB_ZCL_CMD_DOOR_LOCK_UNLOCK_DOOR) ? "UNLOCK" : "TOGGLE";
        uint8_t code_len;
        info("\n");
        info("Received %s command\n", c);
        if (datalen > 0 && get_pincode()[0] > 0)
        {
            code_len = pdata[0];
            if (code_len > 0 && !memcmp(pdata, get_pincode(), code_len))
            {
                info("PIN code match\n");
                set_lock_state((cmd == ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR) ? ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED :
                               (cmd == ZB_ZCL_CMD_DOOR_LOCK_UNLOCK_DOOR) ? ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_UNLOCKED :
                               get_lock_state() == ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED ? ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_UNLOCKED : ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED);
                info("Lockstate: %d\n", get_lock_state());
            }
            else
            {
                info_color(LOG_RED, "PIN code not match\n");
            }
        }
        else if (get_pincode()[0] == 0)
        {
            set_lock_state((cmd == ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR) ? ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED :
                           (cmd == ZB_ZCL_CMD_DOOR_LOCK_UNLOCK_DOOR) ? ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_UNLOCKED :
                           get_lock_state() == ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED ? ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_UNLOCKED : ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED);
            info("Lockstate: %d\n", get_lock_state());
        }
    }
    break;
    case ZB_ZCL_CMD_DOOR_LOCK_SET_PIN_CODE:
    {
        uint8_t status = 0;
        if (datalen > 4)
        {
            if ((pdata[4] + 5) != datalen)
            {
                info_color(LOG_RED, "pincode len error\n");
                status = 1;// generic error
            }
            info("Set PIN code:");
            for (uint8_t i = 0; i < pdata[4]; i++)
            {
                info("%c", pdata[i + 5]);
            }
            info("\n");
            set_pincode(pdata + 4);
            pincode_update();
            zigbee_zcl_data_req_t *pt_data_req;

            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, ZIGBEE_DEFAULT_ENDPOINT,
                                ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                ZB_ZCL_CMD_DOOR_LOCK_SET_PIN_CODE_RESPONSE,
                                TRUE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, 1)


            if (pt_data_req)
            {
                pt_data_req->sp_seq_num = 1;
                pt_data_req->seq_num = seqnum;
                pt_data_req->cmdFormat[0] = status;
                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }
        }
    }
    case ZB_ZCL_CMD_DOOR_LOCK_GET_PIN_CODE:
    {
        if (datalen != 2)
        {
            break;
        }
        info("Received GET PINCODE command\n");
        uint8_t len;
        uint16_t uid = pdata[1] << 8 & pdata[0];
        info("uid %d\n", uid);
        uint8_t *buf;
        zigbee_zcl_data_req_t *pt_data_req;
        if (uid != 0)
        {
            len = 2;
            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, ZIGBEE_DEFAULT_ENDPOINT,
                                ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                ZB_ZCL_CMD_DEFAULT_RESP,
                                FALSE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, len)
            pt_data_req->sp_seq_num = 1;
            pt_data_req->seq_num = seqnum;
            if (pt_data_req)
            {
                pt_data_req->cmdFormat[0] = ZB_ZCL_CMD_DOOR_LOCK_GET_PIN_CODE;
                pt_data_req->cmdFormat[1] = ZB_ZCL_STATUS_FAIL;

                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }
        }
        else
        {
            buf = get_pincode();
            len = buf[0] + 5;
            ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, ZIGBEE_DEFAULT_ENDPOINT,
                                ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                ZB_ZCL_CMD_DOOR_LOCK_GET_PIN_CODE_RESPONSE,
                                TRUE, TRUE,
                                ZCL_FRAME_SERVER_CLIENT_DIR, 0, len)


            if (pt_data_req)
            {
                pt_data_req->sp_seq_num = 1;
                pt_data_req->seq_num = seqnum;

                pt_data_req->cmdFormat[0] = 0;
                pt_data_req->cmdFormat[1] = 0;
                pt_data_req->cmdFormat[2] = 0;
                pt_data_req->cmdFormat[3] = (buf[0] == 0) ? 0xFF : 0x00;
                memcpy(&pt_data_req->cmdFormat[4], buf, buf[0] + 1);
                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
                sys_free(pt_data_req);
            }
        }
    }
    break;
    case ZB_ZCL_CMD_DOOR_LOCK_CLEAR_PIN_CODE:
    case ZB_ZCL_CMD_DOOR_LOCK_CLEAR_ALL_PIN_CODES:
    {
        uint8_t status = 0;
        char *c = (cmd == ZB_ZCL_CMD_DOOR_LOCK_CLEAR_PIN_CODE) ? "CLEAR PINCODE" : "CLEAR ALL PINCODE";
        uint16_t uid = pdata[1] << 8 & pdata[0];
        info("Received %s command\n", c);
        if (uid != 0)
        {
            status = 1;    //fail
        }
        clear_pincode();
        zigbee_zcl_data_req_t *pt_data_req;

        ZIGBEE_ZCL_DATA_REQ(pt_data_req, srcAddr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, srcEndpint, ZIGBEE_DEFAULT_ENDPOINT,
                            ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                            (cmd == ZB_ZCL_CMD_DOOR_LOCK_CLEAR_PIN_CODE) ? ZB_ZCL_CMD_DOOR_LOCK_CLEAR_PIN_CODE_RESPONSE : ZB_ZCL_CMD_DOOR_LOCK_CLEAR_ALL_PIN_CODES_RESPONSE,
                            TRUE, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, 0, 1)

        if (pt_data_req)
        {
            pt_data_req->sp_seq_num = 1;
            pt_data_req->seq_num = seqnum;
            pt_data_req->cmdFormat[0] = status;
            zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
            sys_free(pt_data_req);
        }
    }
    break;
    default:
        break;
    }
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
            case ZB_ZCL_CLUSTER_ID_DOOR_LOCK:
                _zcl_doorlock_process(pt_zcl_msg->cmd, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat, pt_zcl_msg->srcAddr, pt_zcl_msg->srcEndpint, pt_zcl_msg->seq_num, pt_zcl_msg->dstAddr, pt_zcl_msg->disableDefaultRsp);
                break;
            default:
                break;
            }
        }

    } while (0);
}
