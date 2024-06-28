/**
 * Copyright (c) 2021 All Rights Reserved.
 */
/** @file zigbee_evt_handler.c
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

#include "zigbee_lib_api.h"

app_db_t gt_app_db;

//=============================================================================
//                Private Function Declaration
//=============================================================================
static void _zdo_evt_device_announce(sys_tlv_t *pt_tlv);
static void _zdo_evt_permit_join_status(sys_tlv_t *pt_tlv);
static void _zdo_evt_start(sys_tlv_t *pt_tlv);
static void _zdo_evt_simple_desc_idc(sys_tlv_t *pt_tlv);
static void _zdo_evt_act_eo_idc(sys_tlv_t *pt_tlv);
static void _mac_ed_scan_idc(sys_tlv_t *pt_tlv);
static void _zdo_evt_device_leave_idc(sys_tlv_t *pt_tlv);
static void(*zdo_evt_idc_func_list[])(sys_tlv_t *) =
{
    [ZIGBEE_EVT_TYPE_START_IDC - ZIGBEE_EVT_TYPE_ZDO_START_IDC] = _zdo_evt_start,
    [ZIGBEE_EVT_TYPE_DEVICE_ANNCE_IDC - ZIGBEE_EVT_TYPE_ZDO_START_IDC] = _zdo_evt_device_announce,
    [ZIGBEE_EVT_TYPE_ZDO_PERMIT_JOIN_IDC - ZIGBEE_EVT_TYPE_ZDO_START_IDC] = _zdo_evt_permit_join_status,
    [ZIGBEE_EVT_TYPE_LEAVE_IDC - ZIGBEE_EVT_TYPE_ZDO_START_IDC] = _zdo_evt_device_leave_idc,
    [ZIGBEE_EVT_TYPE_DEVICE_ASSOCIATED_IDC - ZIGBEE_EVT_TYPE_ZDO_START_IDC] = NULL,
    [ZIGBEE_EVT_TYPE_PANID_CONFLICT_IDC - ZIGBEE_EVT_TYPE_ZDO_START_IDC] = NULL,
    [ZIGBEE_EVT_TYPE_ZDO_ACT_EP_IDC - ZIGBEE_EVT_TYPE_ZDO_START_IDC] = _zdo_evt_act_eo_idc,
    [ZIGBEE_EVT_TYPE_ZDO_SIMPLE_DESC_IDC - ZIGBEE_EVT_TYPE_ZDO_START_IDC] = _zdo_evt_simple_desc_idc,
    [ZIGBEE_EVT_TYPE_MAC_ED_SCAN_IDC - ZIGBEE_EVT_TYPE_ZDO_START_IDC] = _mac_ed_scan_idc,

};


static void _zdo_evt_act_eo_idc(sys_tlv_t *pt_tlv)
{
    int i = 0;
    zigbee_zdo_act_ep_idc_t *pt_idc;
    do
    {
        if (!pt_tlv)
        {
            break;
        }

        pt_idc = (zigbee_zdo_act_ep_idc_t *)pt_tlv->value;

        info_color(LOG_CYAN, "Active Ep : Addr %02X, Endpoint ", pt_idc->nwkAddr);

        for (i = 0; i < pt_idc->epCounts; i++)
        {
            info_color(LOG_CYAN, "%02X", pt_idc->ep_list[i]);
        }
        info("\n");

    } while (0);
}

static void _zdo_evt_simple_desc_idc(sys_tlv_t *pt_tlv)
{
    zigbee_zdo_simple_desc_idc_t *pt_idc;
    do
    {
        if (!pt_tlv)
        {
            break;
        }

        pt_idc = (zigbee_zdo_simple_desc_idc_t *)pt_tlv->value;

        info_color(LOG_CYAN, "Simple desc : Addr %02X, Endpoint %02X, Profile %04X, DeviceID %04X\n",
                   pt_idc->nwkAddr, pt_idc->endpoint, pt_idc->profileID, pt_idc->deviceID);


    } while (0);
}


static void _zdo_evt_device_announce(sys_tlv_t *pt_tlv)
{
    int i;
    zigbee_zdo_device_annce_idc_t *pt_dev_annce;
    do
    {
        if (!pt_tlv)
        {
            break;
        }
        pt_dev_annce = (zigbee_zdo_device_annce_idc_t *)pt_tlv->value;
        info_color(LOG_GREEN, "Device Announce :\n");
        info_color(LOG_GREEN, "\tIEEE %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
                   pt_dev_annce->ieeeAddr[7], pt_dev_annce->ieeeAddr[6],
                   pt_dev_annce->ieeeAddr[5], pt_dev_annce->ieeeAddr[4],
                   pt_dev_annce->ieeeAddr[3], pt_dev_annce->ieeeAddr[2],
                   pt_dev_annce->ieeeAddr[1], pt_dev_annce->ieeeAddr[0]);
        info_color(LOG_GREEN, "\tShort address 0x%04x, Cap %02X\n", pt_dev_annce->shortAddr, pt_dev_annce->capability);
        zigbee_gateway_cmd_send((0x0013), 0x0000, 0, 0, (uint8_t *)pt_tlv->value, 11);
        zigbee_act_ep_req(pt_dev_annce->shortAddr);

        for (i = 0; i < ZB_GW_SUPPORT_MAX_DEVICE_NUM; i++)
        {
            if (ZB_IEEE_ADDR_CMP(gt_app_db.device_table[i].ieee_addr, pt_dev_annce->ieeeAddr))
            {
                gt_app_db.device_table[i].short_addr = pt_dev_annce->shortAddr;
                gt_app_db.device_table[i].valid = 1;
                gt_app_db.device_table[i].joined = 2;
                if ((pt_dev_annce->capability & 0xE) == 0xE)
                {
                    gt_app_db.device_table[i].cap = 1;
                }
                else
                {
                    gt_app_db.device_table[i].cap = 0;
                }
                app_db_update();
                break;
            }

            if (gt_app_db.device_table[i].valid == 0)
            {
                memcpy(gt_app_db.device_table[i].ieee_addr, pt_dev_annce->ieeeAddr, 8);
                gt_app_db.device_table[i].short_addr = pt_dev_annce->shortAddr;
                gt_app_db.device_table[i].valid = 1;
                gt_app_db.device_table[i].joined = 2;
                if ((pt_dev_annce->capability & 0xE) == 0xE)
                {
                    gt_app_db.device_table[i].cap = 1;
                }
                else
                {
                    gt_app_db.device_table[i].cap = 0;
                }

                gt_app_db.device_number++;
                app_db_update();
                break;
            }
        }
    } while (0);

}

static void _zdo_evt_permit_join_status(sys_tlv_t *pt_tlv)
{

    zigbee_zdo_device_permit_join_idc_t *pt_permit_join;
    do
    {
        if (!pt_tlv)
        {
            break;
        }
        pt_permit_join = (zigbee_zdo_device_permit_join_idc_t *)pt_tlv->value;

        if (pt_permit_join->permit_duration == 0x00)
        {
            //info("enlarge permit join\n");
            //zigbee_send_permit_join(0x0000, 180);
            //zigbee_send_permit_join(0xfffc, 180);
            zigbee_gateway_cmd_send(0x8037, 0, 0, 0, NULL, 0);
            break;
        }
    } while (0);

}

static void _zdo_evt_start(sys_tlv_t *pt_tlv)
{
    zigbee_nwk_start_idc_t *pt_start_idc = (zigbee_nwk_start_idc_t *)pt_tlv->value;
    if (pt_start_idc->status != 0)
    {
        info_color(LOG_RED, "Start fail\n");
        zigbee_app_evt_change(APP_INIT_EVT);
    }
    else
    {
        info_color(LOG_GREEN, "Device start success\n");
        info_color(LOG_GREEN, "PAN: %04X, ShortAddr: %04X, on channel: %02d, MAC: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
                   pt_start_idc->panID, pt_start_idc->nwkAddr, pt_start_idc->channel,
                   pt_start_idc->ieee_addr[7], pt_start_idc->ieee_addr[6],
                   pt_start_idc->ieee_addr[5], pt_start_idc->ieee_addr[4],
                   pt_start_idc->ieee_addr[3], pt_start_idc->ieee_addr[2],
                   pt_start_idc->ieee_addr[1], pt_start_idc->ieee_addr[0]);

        /* update device info from DS flash */
        app_db_check();
        zigbee_app_evt_change(APP_ZB_START_EVT);
    }
}

static void _mac_ed_scan_idc(sys_tlv_t *pt_tlv)
{
    zigbee_mac_ed_scan_idc_t *pt_idc = (zigbee_mac_ed_scan_idc_t *)pt_tlv->value;
    int i;
    for (i = 0; i < sizeof(pt_idc->EDValue); i++)
    {
        info("channel %d value: %d\n", i + 11, pt_idc->EDValue[i]);
    }
}

static void _zdo_evt_device_leave_idc(sys_tlv_t *pt_tlv)
{
    int i = 0;
    zigbee_zdo_device_leave_idc_t *pt_dev_leave;
    do
    {
        if (!pt_tlv)
        {
            break;
        }
        pt_dev_leave = (zigbee_zdo_device_leave_idc_t *)pt_tlv->value;
        info_color(LOG_RED, "Device Leave :\n");
        info_color(LOG_RED, "\tIEEE %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
                   pt_dev_leave->ieeeAddr[7], pt_dev_leave->ieeeAddr[6],
                   pt_dev_leave->ieeeAddr[5], pt_dev_leave->ieeeAddr[4],
                   pt_dev_leave->ieeeAddr[3], pt_dev_leave->ieeeAddr[2],
                   pt_dev_leave->ieeeAddr[1], pt_dev_leave->ieeeAddr[0]);
        info_color(LOG_RED, "\tShort address 0x%04x, rejoin %02X\n", pt_dev_leave->shortAddr, pt_dev_leave->rejoin);

        for (i = 0; i < ZB_GW_SUPPORT_MAX_DEVICE_NUM; i++)
        {

            /* IEEE addr exist */
            if (ZB_IEEE_ADDR_CMP(gt_app_db.device_table[i].ieee_addr, pt_dev_leave->ieeeAddr))
            {

                memmove(&gt_app_db.device_table[i], &gt_app_db.device_table[i + 1U],
                        (gt_app_db.device_number - 1U - i)*sizeof(app_device_entry_t));

                memset(&gt_app_db.device_table[gt_app_db.device_number - 1U], 0, sizeof(app_device_entry_t));
                gt_app_db.device_number--;

                app_db_update();
                break;
            }

        }

    } while (0);
}

void zigbee_evt_handler(sys_tlv_t *pt_tlv)
{
    if ((pt_tlv->type >= ZIGBEE_EVT_TYPE_ZDO_START_IDC) &&
            (pt_tlv->type <= ZIGBEE_EVT_TYPE_ZDO_FINISH_IDC))
    {
        if (zdo_evt_idc_func_list[pt_tlv->type - ZIGBEE_EVT_TYPE_ZDO_START_IDC])
        {
            zdo_evt_idc_func_list[pt_tlv->type - ZIGBEE_EVT_TYPE_ZDO_START_IDC](pt_tlv);
        }
    }
}

void zigbee_app_dump_device_table(void)
{
    int i/*, j*/;
    uint8_t join_state[3];
    uint8_t dev_cap[3];
    /* let user store those data  */

    info("  Short addr | Joined | Cap | \n");
    info("================================\n");

    for (i = 0; i < ZB_GW_SUPPORT_MAX_DEVICE_NUM; i++)
    {

        if (!gt_app_db.device_table[i].valid)
        {
            continue;
        }

        if (gt_app_db.device_table[i].joined != 0)
        {
            if (gt_app_db.device_table[i].joined == 1)
            {
                memcpy(join_state, "AD", sizeof(join_state));
            }
            else if (gt_app_db.device_table[i].joined == 2)
            {
                memcpy(join_state, " O", sizeof(join_state));
            }
            else if (gt_app_db.device_table[i].joined == 3)
            {
                memcpy(join_state, "RE", sizeof(join_state));
            }
        }
        if (gt_app_db.device_table[i].cap == 1)
        {
            memcpy(dev_cap, "ZR", sizeof(join_state));
        }
        else
        {
            memcpy(dev_cap, " ZD", sizeof(join_state));
        }

        if (gt_app_db.device_table[i].joined != 0)
        {
            info("[%03d]:", i);
            info("0x%04X |", gt_app_db.device_table[i].short_addr);
            info("  %s    |", (const char *)join_state);
            info(" %s  |", (const char *)dev_cap);
            /* let user store those data  */
            info("\n");

        }

    }
}

uint32_t zigbee_app_get_device_num()
{
    uint32_t i;
    uint32_t cnt = 0;
    for (i = 0 ; i < ZB_GW_SUPPORT_MAX_DEVICE_NUM ; i++)
    {
        if (gt_app_db.device_table[i].valid)
        {
            cnt++;
        }
    }

    return cnt;
}

