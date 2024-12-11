/**
 * Copyright (c) 2021 All Rights Reserved.
 */
/** @file zigbee_lib_api.c
 *
 * @author Rex
 * @version 0.1
 * @date 2021/2/18
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

#include "flashctl.h"
//=============================================================================
//                Private Definitions of const value
//=============================================================================

//=============================================================================
//                Private ENUM
//=============================================================================

//=============================================================================
//                Private Struct
//=============================================================================

//=============================================================================
//                Private Global Variables
//=============================================================================
uint8_t long_addr[8];
uint8_t reset_to_default;
uint16_t raf_image_type = 321;
//=============================================================================
//                Functions
//=============================================================================
static void zigbee_otp_mac_addr(uint8_t *addr)
{
    uint8_t  temp[256];
    flash_read_sec_register((uint32_t)temp, 0x1100);
    memcpy(addr, temp + 8, 8);
}

uint32_t zigbee_nwk_start_request(uint32_t device_role, uint32_t channel_mask, uint32_t primary_ch_mask, uint32_t maxChild, uint16_t panID, uint8_t  *extPANID, uint32_t reset)
{
    sys_tlv_t *pt_tlv;
    zigbee_nwk_start_req_t *pt_nwk_start_req;
    uint8_t otp_metadata[8];
    uint8_t ieeeAddr[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint32_t cfm_status = 0xFFFF;
    uint8_t i = 0;

    do
    {
        zigbee_otp_mac_addr(otp_metadata);

        pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_nwk_start_req_t));
        if (!pt_tlv)
        {
            break;
        }

        if (!memcmp(ieeeAddr, otp_metadata, 8))
        {
            if (device_role == ZIGBEE_DEVICE_ROLE_CORDINATOR)
            {
                //ieeeAddr[7] = 0x10;
            }
            else if (device_role == ZIGBEE_DEVICE_ROLE_ROUTER)
            {
                //ieeeAddr[7] = 0x11;
            }
            else
            {
                info_color(LOG_RED, "Not support role\n");
                break;
            }

            flash_get_unique_id((uint32_t)ieeeAddr, 8);
        }
        else
        {
            if (device_role == ZIGBEE_DEVICE_ROLE_CORDINATOR ||
                    device_role == ZIGBEE_DEVICE_ROLE_ROUTER)
            {
                //memcpy(ieeeAddr, otp_metadata, 8);
                for (i = 0; i < 8; i++)
                {
                    ieeeAddr[i] = otp_metadata[i];
                }
            }
            else
            {
                break;
            }
        }

        for (i = 0; i < 8; i++)
        {
            long_addr[i] = ieeeAddr[i];
        }

        pt_tlv->type = ZIGBEE_EVT_TYPE_NWK_START_REQ;
        pt_tlv->length = sizeof(zigbee_nwk_start_req_t);
        pt_nwk_start_req = (zigbee_nwk_start_req_t *)pt_tlv->value;
        pt_nwk_start_req->deviceRole = device_role;
        pt_nwk_start_req->channelMask = channel_mask;
        pt_nwk_start_req->primaryChannelMask = primary_ch_mask;
        //pt_nwk_start_req->secondaryChannelMask = secondary_ch_mask;
        pt_nwk_start_req->maxChild = maxChild;
        pt_nwk_start_req->PANID = panID;
        pt_nwk_start_req->reset = reset;
        //memcpy(pt_nwk_start_req->ieeeAddr, ieeeAddr, 8);
        for (i = 0; i < 8; i++)
        {
            pt_nwk_start_req->ieeeAddr[i] = ieeeAddr[i];
            pt_nwk_start_req->extendedPANID[i] = extPANID[i];
        }

        cfm_status = zigbee_event_msg_sendto(pt_tlv);

    } while (0);

    if (pt_tlv)
    {
        sys_free(pt_tlv);
    }

    return cfm_status;
}

uint32_t zigbee_ed_nwk_start_request(uint32_t channel_mask, uint32_t primary_ch_mask, uint32_t rx_allway_on, uint16_t keepalive, uint8_t  *extPANID, uint32_t reset)
{
    sys_tlv_t *pt_tlv;
    zigbee_nwk_start_req_t *pt_nwk_start_req;
    uint8_t otp_metadata[8];
    uint8_t ieeeAddr[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint32_t cfm_status = 0xFFFF;
    uint8_t i = 0;

    do
    {
        zigbee_otp_mac_addr(otp_metadata);

        pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_nwk_start_req_t));

        if (!pt_tlv)
        {
            break;
        }

        if (!memcmp(ieeeAddr, otp_metadata, 8))
        {
            //ieeeAddr[7] = 0x12;
            flash_get_unique_id((uint32_t)ieeeAddr, 8);
        }
        else
        {
            //memcpy(ieeeAddr, otp_metadata, 8);
            for (i = 0; i < 8; i++)
            {
                ieeeAddr[i] = otp_metadata[i];
            }
        }

        for (i = 0; i < 8; i++)
        {
            long_addr[i] = ieeeAddr[i];
        }

        pt_tlv->type = ZIGBEE_EVT_TYPE_NWK_START_REQ;
        pt_tlv->length = sizeof(zigbee_nwk_start_req_t);
        pt_nwk_start_req = (zigbee_nwk_start_req_t *)pt_tlv->value;
        pt_nwk_start_req->deviceRole = ZIGBEE_DEVICE_ROLE_ENDDEVICE;
        pt_nwk_start_req->channelMask = channel_mask;
        pt_nwk_start_req->primaryChannelMask = primary_ch_mask;
        pt_nwk_start_req->rx_always_on = rx_allway_on;
        pt_nwk_start_req->keepalive = keepalive;
        pt_nwk_start_req->reset = reset;
        memcpy(pt_nwk_start_req->extendedPANID, extPANID, 8);
        memcpy(pt_nwk_start_req->ieeeAddr, ieeeAddr, 8);

        cfm_status = zigbee_event_msg_sendto(pt_tlv);

    } while (0);

    if (pt_tlv)
    {
        sys_free(pt_tlv);
    }

    return cfm_status;
}


void zigbee_join_request(void)
{
    sys_tlv_t *pt_tlv;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE);

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_NWK_JOIN_REQ;
        pt_tlv->length = 0;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);

}

void zigbee_leave_req(void)
{
    sys_tlv_t *pt_tlv;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE);

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_NWK_LEAVE_VIA_LOCAL_REQ;
        pt_tlv->length = 0;


        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}



void zigbee_zcl_request(zigbee_zcl_data_req_t *pt_data_req, uint16_t data_len)
{
    sys_tlv_t *pt_tlv;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_zcl_data_req_t) + data_len);

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_ZCL_DATA_REQ;
        pt_tlv->length = sizeof(zigbee_zcl_data_req_t) + data_len;
        memcpy(pt_tlv->value, pt_data_req, sizeof(zigbee_zcl_data_req_t) + data_len);
        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_zcl_attr_read_request(zigbee_zcl_attr_rw_t *pt_data_req, uint16_t data_len)
{
    sys_tlv_t *pt_tlv;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_zcl_attr_rw_t) + data_len);

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_ZCL_ATTR_READ_REQ;
        pt_tlv->length = sizeof(zigbee_zcl_attr_rw_t) + data_len;
        memcpy(pt_tlv->value, pt_data_req, sizeof(zigbee_zcl_attr_rw_t) + data_len);
        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_zcl_attr_wirte_request(zigbee_zcl_attr_rw_t *pt_data_req, uint16_t data_len)
{
    sys_tlv_t *pt_tlv;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_zcl_attr_rw_t) + data_len);

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_ZCL_ATTR_WRITE_REQ;
        pt_tlv->length = sizeof(zigbee_zcl_attr_rw_t) + data_len;
        memcpy(pt_tlv->value, pt_data_req, sizeof(zigbee_zcl_attr_rw_t) + data_len);
        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_zcl_attr_set(zigbee_zcl_attr_set_t *pt_attr_set, uint16_t data_len)
{
    sys_tlv_t *pt_tlv;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_zcl_attr_set_t) + data_len);

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_ZCL_ATTR_SET_REQ;
        pt_tlv->length = sizeof(zigbee_zcl_attr_set_t) + data_len;
        memcpy(pt_tlv->value, pt_attr_set, sizeof(zigbee_zcl_attr_set_t) + data_len);
        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_aps_bind_set(uint8_t srcEP, uint8_t *srcIeee, uint8_t dstEP, uint8_t *dstAddr, uint16_t clusterID)
{
    sys_tlv_t *pt_tlv;
    zigbee_zdo_bind_req_t *pt_req;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_zdo_bind_req_t));

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_APS_BIND_SET_REQ;
        pt_tlv->length = sizeof(zigbee_zdo_bind_req_t);

        pt_req = (zigbee_zdo_bind_req_t *)pt_tlv->value;

        memcpy(pt_req->srcIeeeAddr, srcIeee, 8);

        pt_req->bind = 0;
        pt_req->srcEP = srcEP;
        pt_req->dstaddrmode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        memcpy(pt_req->dstAddr.addr_long, dstAddr, 8);

        pt_req->dstEP = dstEP;
        pt_req->clusterID = clusterID;
        pt_req->reqDstAddr = 0;
        pt_req->groupAddr = 0;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_finding_binding_req(uint8_t ep, uint8_t role)
{
    sys_tlv_t *pt_tlv;
    zigbee_start_fb_req_t *pt_req;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_start_fb_req_t));

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_START_FB_REQ;
        pt_tlv->length = sizeof(zigbee_start_fb_req_t);

        pt_req = (zigbee_start_fb_req_t *)pt_tlv->value;

        pt_req->ep = ep;
        pt_req->role = role;
        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_ota_insert_file(uint32_t file_addr, uint32_t file_size, uint32_t file_version, uint16_t image_type, uint16_t manufacturer_code)
{
    sys_tlv_t *pt_tlv;
    zigbee_ota_file_insert_req_t *pt_req;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_ota_file_insert_req_t));

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_OTA_FILE_INSERT_REQ;
        pt_tlv->length = sizeof(zigbee_ota_file_insert_req_t);

        pt_req = (zigbee_ota_file_insert_req_t *)pt_tlv->value;

        pt_req->file_version = file_version;
        pt_req->manufacturer_code = manufacturer_code;
        pt_req->image_type = image_type;
        pt_req->p_img_data = (uint8_t *)file_addr;
        pt_req->image_size = file_size;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_ota_client_start(uint16_t nwkAddr, uint8_t ep)
{
    sys_tlv_t *pt_tlv;
    zigbee_ota_client_start_req_t *pt_req;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_ota_client_start_req_t));

    do
    {
        if (!pt_tlv)
        {
            break;
        }
        pt_tlv->type = ZIGBEE_EVT_TYPE_OTA_CLIENT_START_REQ;
        pt_tlv->length = sizeof(zigbee_ota_client_start_req_t);

        pt_req = (zigbee_ota_client_start_req_t *)pt_tlv->value;

        pt_req->nwkAddr = nwkAddr;
        pt_req->ep = ep;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_act_ep_req(uint16_t addr)
{
    sys_tlv_t *pt_tlv;
    zigbee_zdo_act_ep_req_t *pt_req;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_zdo_act_ep_req_t));

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_ZDO_ACT_EP_REQ;
        pt_tlv->length = sizeof(zigbee_zdo_act_ep_req_t);

        pt_req = (zigbee_zdo_act_ep_req_t *)pt_tlv->value;

        pt_req->nwkAddr = addr;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_simple_desc_req(uint16_t addr, uint8_t endpoint)
{
    sys_tlv_t *pt_tlv;
    zigbee_zdo_simple_desc_req_t *pt_req;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_zdo_simple_desc_req_t));

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_ZDO_SIMPLE_DESC_REQ;
        pt_tlv->length = sizeof(zigbee_zdo_simple_desc_req_t);

        pt_req = (zigbee_zdo_simple_desc_req_t *)pt_tlv->value;

        pt_req->nwkAddr = addr;
        pt_req->endpoint = endpoint;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_bind_req(uint8_t bind, uint8_t srcEP, uint8_t *srcIeee, uint8_t dstEP, uint8_t *dstAddr, uint8_t dstAddrMode, uint16_t clusterID, uint16_t reqDstAddr)
{
    sys_tlv_t *pt_tlv;
    zigbee_zdo_bind_req_t *pt_req;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_zdo_bind_req_t));

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_ZDO_COMM_BIND_REQ;
        pt_tlv->length = sizeof(zigbee_zdo_bind_req_t);

        pt_req = (zigbee_zdo_bind_req_t *)pt_tlv->value;

        memcpy(pt_req->srcIeeeAddr, srcIeee, 8);

        pt_req->bind = bind;
        pt_req->srcEP = srcEP;
        pt_req->dstaddrmode = dstAddrMode;
        if (dstAddrMode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
        {
            memcpy(pt_req->dstAddr.addr_long, dstAddr, 8);
        }
        else if (dstAddrMode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT)
        {
            memcpy((uint8_t *)&pt_req->dstAddr.addr_short, dstAddr, 2);
        }
        else
        {
            info("Unsupported address mode\n");
            break;
        }
        pt_req->dstEP = dstEP;
        pt_req->clusterID = clusterID;
        pt_req->reqDstAddr = reqDstAddr;
        pt_req->groupAddr = 0;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}
void zigbee_rejoin_request(void)
{
    sys_tlv_t *pt_tlv;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE);

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_NWK_REJOIN_REQ;
        pt_tlv->length = 0;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);

}

void zigbee_mac_ed_scan_req(void)
{
    sys_tlv_t *pt_tlv;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE);

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_MAC_ED_SCAN_REQ;
        pt_tlv->length = 0;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_mac_address_get_req(void)
{
    sys_tlv_t *pt_tlv;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE);

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_MAC_ADDRESS_GET_REQ;
        pt_tlv->length = 0;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_ic_add(uint8_t *addr, uint8_t type, uint8_t *ic)
{
    sys_tlv_t *pt_tlv;
    zigbee_ic_add_t *pt_ic_add;


    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_ic_add_t));

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_IC_ADD;
        pt_tlv->length = sizeof(zigbee_ic_add_t);

        pt_ic_add = (zigbee_ic_add_t *)pt_tlv->value;

        memcpy(pt_ic_add->ieeeAddr, addr, 8);
        pt_ic_add->ic_type = type;
        memcpy(pt_ic_add->ic, ic, 19);

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}

void zigbee_is_factory_new_req(void)
{
    sys_tlv_t *pt_tlv;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE);

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_IS_FACTORY_NEW_REQ;
        pt_tlv->length = 0;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}
void zigbee_send_permit_join(uint16_t dstAddr, uint8_t permit_duration)
{
    sys_tlv_t *pt_tlv;
    zigbee_zdo_mgmt_permit_joining_req_t *pt_req;

    pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(zigbee_zdo_mgmt_permit_joining_req_t));

    do
    {
        pt_tlv->type = ZIGBEE_EVT_TYPE_MGMT_PERMIT_JOIN_REQ;
        pt_tlv->length = sizeof(zigbee_zdo_mgmt_permit_joining_req_t);

        pt_req = (zigbee_zdo_mgmt_permit_joining_req_t *)pt_tlv->value;

        pt_req->dest_addr = dstAddr;
        pt_req->permit_duration = permit_duration;
        pt_req->tc_significance = 1;

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}
void zigbee_raf_cmd_req(uint16_t dst_addr, uint32_t t_index, uint32_t u32_len, uint8_t *pu8_value)
{
    sys_tlv_t *pt_tlv = sys_malloc(SYS_TLV_HEADER_SIZE + sizeof(raf_cmd_req_t) + u32_len);
    raf_cmd_req_t *pt_cmd_req;

    do
    {
        if (pt_tlv == NULL)
        {
            info_color(LOG_RED, "CMD malloc fail\n");
            break;
        }
        memset(pt_tlv, 0, SYS_TLV_HEADER_SIZE + sizeof(raf_cmd_req_t) + u32_len);

        pt_tlv->type = ZIGBEE_EVT_TYPE_RAF_CMD_REQUEST;
        pt_tlv->length = sizeof(raf_cmd_req_t) + u32_len;

        pt_cmd_req = (raf_cmd_req_t *)pt_tlv->value;
        pt_cmd_req->cmd_type = RAF_CMD;
        pt_cmd_req->cmd_index = t_index;
        pt_cmd_req->cmd_length = u32_len;
        pt_cmd_req->cmd_dst_addr = dst_addr;
        memcpy(pt_cmd_req->cmd_value, pu8_value, u32_len);

        //util_log_mem(UTIL_LOG_INFO, "  ", (uint8_t *)pt_tlv, SYS_TLV_HEADER_SIZE + sizeof(raf_cmd_req_t) + u32_len, 0);

        zigbee_event_msg_sendto(pt_tlv);

        sys_free(pt_tlv);
    } while (0);
}
void check_reset(void)
{
    int ckeck_byte_pos;
    uint8_t check_byte_1st_block, check_byte_2nd_block, reset_count = 0;
    for (ckeck_byte_pos = 0; ckeck_byte_pos < CHECK_RESET_BLOCK_SIZE; ckeck_byte_pos++)
    {
        while (flash_check_busy()) {;}
        check_byte_1st_block = flash_read_byte(CHECK_RESET_START_ADDRESS + ckeck_byte_pos);
        while (flash_check_busy()) {;}
        check_byte_2nd_block = flash_read_byte(CHECK_RESET_START_ADDRESS + CHECK_RESET_BLOCK_SIZE + ckeck_byte_pos);
        /*
        bit 7:  valid bit, 1: valid,0: invalid
        bit 6&5:check bit, 0 if bad byte
        bit 4~0:power cycle count
        */
        if (((check_byte_1st_block & 0xE0) == 0xE0) && /*check bit 7,6,5*/
                check_byte_1st_block == check_byte_2nd_block)
        {
            break;
        }
        if (ckeck_byte_pos == CHECK_RESET_BLOCK_SIZE - 1)
        {
            while (flash_check_busy()) {;}
            flash_erase(FLASH_ERASE_SECTOR, CHECK_RESET_START_ADDRESS);
            /*check bad byte*/
            for (int i = 0; i < 2 * CHECK_RESET_BLOCK_SIZE; i++)
            {
                if (flash_read_byte(CHECK_RESET_START_ADDRESS + i) != 0xFF)
                {
                    while (flash_check_busy()) {;}
                    flash_write_byte(CHECK_RESET_START_ADDRESS + i, 0x9F /* bad byte, set bit 6&5 to 0*/);
                }
            }
            return;
        }
    }
    while ((check_byte_1st_block & (1 << reset_count)) == 0 && reset_count < 5)
    {
        reset_count++;
    }
    reset_count++;
    check_byte_1st_block &= ~( 1 << (reset_count - 1) );

    if (reset_count >= 5)
    {
        reset_to_default = 1;
    }
    else
    {
        while (flash_check_busy()) {;}
        flash_write_byte(CHECK_RESET_START_ADDRESS + ckeck_byte_pos, check_byte_1st_block);
        while (flash_check_busy()) {;}
        flash_write_byte(CHECK_RESET_START_ADDRESS + CHECK_RESET_BLOCK_SIZE + ckeck_byte_pos, check_byte_1st_block);
    }
    Delay_ms(500);
    while (flash_check_busy());
    flash_write_byte(CHECK_RESET_START_ADDRESS + ckeck_byte_pos, check_byte_1st_block &= ~(1 << 7));
    while (flash_check_busy());
    flash_write_byte(CHECK_RESET_START_ADDRESS + CHECK_RESET_BLOCK_SIZE + ckeck_byte_pos, check_byte_1st_block &= ~(1 << 7));
}
