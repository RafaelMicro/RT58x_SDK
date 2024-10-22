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

/* ZigBee Stack Library APIs */
#include "zigbee_stack_api.h"
#include "zigbee_app.h"

#include "bsp_led.h"

typedef void (*zcl_read_rsp_cb)(uint16_t cluster_id, uint16_t addr, uint8_t src_endp, uint8_t *pd, uint8_t pd_len);
typedef void (*zcl_write_rsp_cb)(uint16_t cluster_id, uint16_t addr, uint8_t src_endp, uint8_t *pd, uint8_t pd_len);
typedef void (*zcl_cfg_report_rsp_cb)(uint16_t cluster_id, uint16_t addr, uint8_t src_endp, uint8_t *pd, uint8_t pd_len);


static void _zcl_identify_handle(uint8_t cmd, uint16_t src_addr, uint8_t src_endp, uint16_t datalen, uint8_t *pdata);
static void _zcl_group_handle(uint8_t cmd, uint16_t src_addr, uint8_t src_endp, uint16_t datalen, uint8_t *pdata);
static void _zcl_scene_handle(uint8_t cmd, uint16_t src_addr, uint8_t src_endp, uint16_t datalen, uint8_t *pdata);

static void(*zcl_cluster_gen_func_list[])(uint8_t cmd, uint16_t src_addr, uint8_t src_endp, uint16_t datalen, uint8_t *pdata) =
{
    [ZB_ZCL_CLUSTER_ID_BASIC] = NULL,
    [ZB_ZCL_CLUSTER_ID_POWER_CONFIG] = NULL,
    [ZB_ZCL_CLUSTER_ID_DEVICE_TEMP_CONFIG] = NULL,
    [ZB_ZCL_CLUSTER_ID_IDENTIFY] = _zcl_identify_handle,
    [ZB_ZCL_CLUSTER_ID_GROUPS] = _zcl_group_handle,
    [ZB_ZCL_CLUSTER_ID_SCENES] = _zcl_scene_handle,
    [ZB_ZCL_CLUSTER_ID_ON_OFF] = NULL,
    [ZB_ZCL_CLUSTER_ID_ON_OFF_SWITCH_CONFIG] = NULL,
    [ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL] = NULL,
    [ZB_ZCL_CLUSTER_ID_ALARMS] = NULL,
    [ZB_ZCL_CLUSTER_ID_TIME] = NULL,
};


static void (*zcl_cluster_ms_func_list[])(uint8_t cmd, uint16_t datalen, uint8_t *pdata) =
{
    [ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT - ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT] = NULL,
    [ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT - ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT] = NULL,
};

static zcl_read_rsp_cb p_read_rsp_cb = NULL;
static zcl_write_rsp_cb p_write_rsp_cb = NULL;
static zcl_cfg_report_rsp_cb p_cfg_report_rsp_cb = NULL;

//=============================================================================
//                Private Function Declaration
//=============================================================================
static void _zcl_identify_handle(uint8_t cmd, uint16_t src_addr, uint8_t src_endp, uint16_t datalen, uint8_t *pdata)
{
    zigbee_gateway_cmd_send(0x00048001, src_addr, 0, src_endp, (uint8_t *)pdata, datalen);
}

static void _zcl_group_handle(uint8_t cmd, uint16_t src_addr, uint8_t src_endp, uint16_t datalen, uint8_t *pdata)
{
    zigbee_gateway_cmd_send(0x00058000 | cmd, src_addr, 0, src_endp, (uint8_t *)pdata, datalen);
}


static void _zcl_scene_handle(uint8_t cmd, uint16_t src_addr, uint8_t src_endp, uint16_t datalen, uint8_t *pdata)
{
    zigbee_gateway_cmd_send(0x00068000 | cmd, src_addr, 0, src_endp, (uint8_t *)pdata, datalen);
}


void zigbee_zcl_msg_read_rsp_cb_reg(void *cb)
{
    p_read_rsp_cb = (zcl_read_rsp_cb)cb;
}

void zigbee_zcl_msg_write_rsp_cb_reg(void *cb)
{
    p_write_rsp_cb = (zcl_write_rsp_cb)cb;
}

void zigbee_zcl_msg_cfg_report_rsp_cb_reg(void *cb)
{
    p_cfg_report_rsp_cb = (zcl_cfg_report_rsp_cb)cb;
}
uint16_t get_attribute_size(uint8_t attr_type, uint8_t *data)
{
    uint16_t ret;
    switch ( attr_type )
    {
    case ZB_ZCL_ATTR_TYPE_8BIT:
    case ZB_ZCL_ATTR_TYPE_U8:
    case ZB_ZCL_ATTR_TYPE_S8:
    case ZB_ZCL_ATTR_TYPE_BOOL:
    case ZB_ZCL_ATTR_TYPE_8BITMAP:
    case ZB_ZCL_ATTR_TYPE_8BIT_ENUM:
        ret = 1;
        break;

    case ZB_ZCL_ATTR_TYPE_16BIT:
    case ZB_ZCL_ATTR_TYPE_U16:
    case ZB_ZCL_ATTR_TYPE_S16:
    case ZB_ZCL_ATTR_TYPE_16BITMAP:
    case ZB_ZCL_ATTR_TYPE_16BIT_ENUM:
    case ZB_ZCL_ATTR_TYPE_SEMI:
    case ZB_ZCL_ATTR_TYPE_CLUSTER_ID:
    case ZB_ZCL_ATTR_TYPE_ATTRIBUTE_ID:
        ret = 2;
        break;

    case ZB_ZCL_ATTR_TYPE_32BIT:
    case ZB_ZCL_ATTR_TYPE_U32:
    case ZB_ZCL_ATTR_TYPE_S32:
    case ZB_ZCL_ATTR_TYPE_32BITMAP:
    case ZB_ZCL_ATTR_TYPE_UTC_TIME:
    case ZB_ZCL_ATTR_TYPE_TIME_OF_DAY:
    case ZB_ZCL_ATTR_TYPE_DATE:
    case ZB_ZCL_ATTR_TYPE_BACNET_OID:
    case ZB_ZCL_ATTR_TYPE_SINGLE:
        ret = 4;
        break;

    case ZB_ZCL_ATTR_TYPE_S48:
    case ZB_ZCL_ATTR_TYPE_U48:
    case ZB_ZCL_ATTR_TYPE_48BIT:
    case ZB_ZCL_ATTR_TYPE_48BITMAP:
        ret = 6;
        break;

    case ZB_ZCL_ATTR_TYPE_S24:
    case ZB_ZCL_ATTR_TYPE_U24:
    case ZB_ZCL_ATTR_TYPE_24BIT:
    case ZB_ZCL_ATTR_TYPE_24BITMAP:
        ret = 3;
        break;

    case ZB_ZCL_ATTR_TYPE_U40:
    case ZB_ZCL_ATTR_TYPE_S40:
    case ZB_ZCL_ATTR_TYPE_40BIT:
    case ZB_ZCL_ATTR_TYPE_40BITMAP:
        ret = 5;
        break;

    case ZB_ZCL_ATTR_TYPE_U56:
    case ZB_ZCL_ATTR_TYPE_S56:
    case ZB_ZCL_ATTR_TYPE_56BIT:
    case ZB_ZCL_ATTR_TYPE_56BITMAP:
        ret = 7;
        break;

    case ZB_ZCL_ATTR_TYPE_64BIT:
    case ZB_ZCL_ATTR_TYPE_64BITMAP:
    case ZB_ZCL_ATTR_TYPE_U64:
    case ZB_ZCL_ATTR_TYPE_S64:
    case ZB_ZCL_ATTR_TYPE_DOUBLE:
    case ZB_ZCL_ATTR_TYPE_IEEE_ADDR:
        ret = 8;
        break;
    case ZB_ZCL_ATTR_TYPE_128_BIT_KEY:
        ret = 16;
        break;
    case ZB_ZCL_ATTR_TYPE_OCTET_STRING:
    case ZB_ZCL_ATTR_TYPE_CHAR_STRING:
        ret = data[0] + 1;
        break;
    case ZB_ZCL_ATTR_TYPE_ARRAY:
    case ZB_ZCL_ATTR_TYPE_CUSTOM_32ARRAY:
    case ZB_ZCL_ATTR_TYPE_LONG_OCTET_STRING:
        ret = data[0] | (data[1] << 8) + 2;
        break;

    default:
        ret = 0;
        break;
    }
    return ret;
}
void _zcl_read_attr_report_process(uint8_t cmd, uint16_t clusterID, uint16_t srcAddr, uint16_t datalen, uint8_t *pdata)
{
    if (cmd == 0x01)
    {
        info("Read attribute from 0x%04x cluster 0x%04x\n", srcAddr, clusterID);
    }
    else if (cmd == 0x0a)
    {
        info("Report from 0x%04x cluster 0x%04x\n", srcAddr, clusterID);
    }
    uint16_t attr_id, attr_len;
    uint8_t attr_type;
    int i = 0, j;

    while (i < datalen)
    {
        attr_id = pdata[i] | (pdata[i + 1] << 8);
        if (cmd == 0x01)
        {
            i++;
        }
        attr_type = pdata[i + 2];

        attr_len = get_attribute_size(attr_type, pdata + i + 3);
        info("attribute id: 0x%04x, ", attr_id);
        info("type: 0x%x, ", attr_type);
        info("value:");
        for (j = 0; j < attr_len; j++)
        {
            if (attr_type == ZB_ZCL_ATTR_TYPE_OCTET_STRING ||
                    attr_type == ZB_ZCL_ATTR_TYPE_CHAR_STRING)
            {
                if (j > 0)
                {
                    info("%c", pdata[i + 3 + j]);
                }
            }
            else if (attr_type == ZB_ZCL_ATTR_TYPE_LONG_OCTET_STRING)
            {
                if (j > 1)
                {
                    info("%c", pdata[i + 3 + j]);
                }
            }
            else
            {
                info("%x", pdata[i + 2 + attr_len - j]);
            }
        }
        i += (3 + attr_len);
        info("\n");
    }
}
void zigbee_zcl_msg_handler(sys_tlv_t *pt_tlv)
{
    zigbee_zcl_data_idc_t *pt_zcl_msg = (zigbee_zcl_data_idc_t *)pt_tlv->value;
    uint8_t i = 0;

    do
    {
        if (!pt_zcl_msg)
        {
            break;
        }

        if (pt_zcl_msg->clusterID == ZB_ZCL_CLUSTER_ID_OTA_UPGRADE)
        {
            // ignore
            break;
        }

        if (pt_zcl_msg->clusterID == ZB_ZCL_CLUSTER_ID_CUSTOM)
        {
            if (pt_zcl_msg->cmd == UART_TRANSPARENT_CMD)
            {
                info_color(LOG_YELLOW, "[%04X] ", pt_zcl_msg->srcAddr);
                for (i = 0; i < pt_zcl_msg->cmdFormatLen; i++)
                {
                    info_color(LOG_YELLOW, "%c", pt_zcl_msg->cmdFormat[i]);
                }
                info_color(LOG_YELLOW, "\n");
            }
            break;
        }

        info("Recv ZCL message 0x%04X -> 0x%04X\n", pt_zcl_msg->srcAddr, pt_zcl_msg->dstAddr);
        info("Cluster %04x cmd %d seq %d\n", pt_zcl_msg->clusterID, pt_zcl_msg->cmd, pt_zcl_msg->seq_num);
        util_log_mem(UTIL_LOG_INFO, "  ", (uint8_t *)pt_zcl_msg->cmdFormat, pt_zcl_msg->cmdFormatLen, 0);

        if (pt_zcl_msg->is_common_command)
        {
            if (pt_zcl_msg->cmd == 0x01) // Read response
            {
                if (pt_zcl_msg->cmdFormat[2] == 0x00)
                {
                    _zcl_read_attr_report_process(pt_zcl_msg->cmd, pt_zcl_msg->clusterID, pt_zcl_msg->srcAddr, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat);
                }
                if (p_read_rsp_cb)
                {
                    p_read_rsp_cb(pt_zcl_msg->clusterID, pt_zcl_msg->srcAddr, pt_zcl_msg->srcEndpint, (uint8_t *)pt_zcl_msg->cmdFormat, pt_zcl_msg->cmdFormatLen);
                }
            }
            else if (pt_zcl_msg->cmd == 0x04) // Write response
            {
                if (p_write_rsp_cb)
                {
                    p_write_rsp_cb(pt_zcl_msg->clusterID, pt_zcl_msg->srcAddr, pt_zcl_msg->srcEndpint, (uint8_t *)pt_zcl_msg->cmdFormat, pt_zcl_msg->cmdFormatLen);
                }
            }
            else if (pt_zcl_msg->cmd == 0x07) // Config report response
            {
                if (p_cfg_report_rsp_cb)
                {
                    p_cfg_report_rsp_cb(pt_zcl_msg->clusterID, pt_zcl_msg->srcAddr, pt_zcl_msg->srcEndpint, (uint8_t *)pt_zcl_msg->cmdFormat, pt_zcl_msg->cmdFormatLen);
                }
            }
            else if (pt_zcl_msg->cmd == 0x0a) // Report
            {
                _zcl_read_attr_report_process(pt_zcl_msg->cmd, pt_zcl_msg->clusterID, pt_zcl_msg->srcAddr, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat);
                uint8_t p_report[pt_zcl_msg->cmdFormatLen + 2];
                p_report[0] = pt_zcl_msg->clusterID & 0xFF;
                p_report[1] = (pt_zcl_msg->clusterID >> 8) & 0xFF;
                memcpy(p_report + 2, pt_zcl_msg->cmdFormat, pt_zcl_msg->cmdFormatLen);
                zigbee_gateway_cmd_send(0x00028800, pt_zcl_msg->srcAddr, 0, pt_zcl_msg->srcEndpint, p_report, pt_zcl_msg->cmdFormatLen + 2);
            }
            if (pt_zcl_msg->cmd == 0x0b) // defaut response
            {
                zigbee_gateway_cmd_send(0x00018800, pt_zcl_msg->srcAddr, 0, pt_zcl_msg->srcEndpint, (uint8_t *)pt_zcl_msg->cmdFormat, pt_zcl_msg->cmdFormatLen);
            }
        }
        else
        {
            if (pt_zcl_msg->clusterID <= ZB_ZCL_CLUSTER_ID_TIME)
            {

                if (zcl_cluster_gen_func_list[pt_zcl_msg->clusterID])
                    zcl_cluster_gen_func_list[pt_zcl_msg->clusterID]
                    (pt_zcl_msg->cmd, pt_zcl_msg->srcAddr, pt_zcl_msg->srcEndpint, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat);

            }
            else if (pt_zcl_msg->clusterID >= ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT &&
                     pt_zcl_msg->clusterID <= ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT)
            {
                if (zcl_cluster_ms_func_list[pt_zcl_msg->clusterID - ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT])
                    zcl_cluster_ms_func_list[pt_zcl_msg->clusterID - ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT]
                    (pt_zcl_msg->cmd, pt_zcl_msg->cmdFormatLen, pt_zcl_msg->cmdFormat);
            }
            else if (pt_zcl_msg->clusterID == ZB_ZCL_CLUSTER_ID_IAS_ZONE)
            {
                if (pt_zcl_msg->cmd == ZB_ZCL_CMD_IAS_ZONE_ZONE_STATUS_CHANGE_NOT_ID) // zone status change notification
                {
                    // zigbee_app_zcl_zone_status_change_notification_reg(_zcl_zone_status_change_notificatin_cb);
                    // p_zone_status_change_notification_cb(cmd_info->cluster_id, src_addr, src_ep, pData, payload_size);
                    zigbee_gateway_cmd_send(0x00230000, pt_zcl_msg->srcAddr, 0, pt_zcl_msg->srcEndpint, (uint8_t *)pt_zcl_msg->cmdFormat, pt_zcl_msg->cmdFormatLen);
                }
            }
            else if (pt_zcl_msg->clusterID == ZB_ZCL_CLUSTER_ID_DOOR_LOCK)
            {
                zigbee_gateway_cmd_send(0x00248000 | pt_zcl_msg->cmd, pt_zcl_msg->srcAddr, 0, pt_zcl_msg->srcEndpint, (uint8_t *)pt_zcl_msg->cmdFormat, pt_zcl_msg->cmdFormatLen);
            }
            else if (pt_zcl_msg->clusterID >= 0xFC00)
            {
                uint8_t custom_payload_data[pt_zcl_msg->cmdFormatLen + 6];
                custom_payload_data[0] = pt_zcl_msg->clusterID & 0xFF;
                custom_payload_data[1] = (pt_zcl_msg->clusterID >> 8) & 0xFF;
                custom_payload_data[2] = pt_zcl_msg->manuCode & 0xFF;
                custom_payload_data[3] = (pt_zcl_msg->manuCode >> 8) & 0xFF;
                custom_payload_data[4] = pt_zcl_msg->cmd;
                custom_payload_data[5] = pt_zcl_msg->cmdFormatLen;
                if (pt_zcl_msg->cmdFormatLen > 0)
                {
                    memcpy(&custom_payload_data[6], pt_zcl_msg->cmdFormat, pt_zcl_msg->cmdFormatLen);
                }
                zigbee_gateway_cmd_send(0xFC008000, pt_zcl_msg->srcAddr, 0,
                                        pt_zcl_msg->srcEndpint, custom_payload_data, pt_zcl_msg->cmdFormatLen + 6);
            }
            else
            {
                info_color(LOG_RED, "Not support cluser %04X\n", pt_zcl_msg->clusterID);
            }
        }

    } while (0);
}
