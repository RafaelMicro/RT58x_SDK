/**
 * @file zigbee_gateway_cmd_handle.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2022-03-31
 *
 * @copyright Copyright (c) 2022
 *
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
#include "ble_app.h"

#include "zigbee_gateway.h"

//=============================================================================
//                Private Definitions of const value
//=============================================================================

//=============================================================================
//                Private ENUM
//=============================================================================

//=============================================================================
//                Private Struct
//=============================================================================
typedef struct __attribute__((packed))
{
    uint16_t image_type;
    uint16_t manufacturer_code;
    uint32_t file_version;
    uint32_t image_size;
    uint32_t total_pkt;
    uint32_t cur_pkt;
    uint16_t pkt_len;
    uint8_t pkt[];
}
ota_img_info_t;
//=============================================================================
//                Private Function Declaration
//=============================================================================
static void _cmd_common_gen_req(uint32_t cmd_id, uint8_t *pkt);
//=============================================================================
//                Private Global Variables
//=============================================================================
static ota_img_info_t gt_img_info;
static uint8_t *gp_ota_imgae_cache = NULL;

extern void gw_cmd_app_service_handle(uint32_t cmd_id, uint8_t *pkt);
extern void gw_cmd_db_service_handle(uint32_t cmd_id, uint8_t *pkt);
//=============================================================================
//                Functions
//=============================================================================
static void _cmd_common_gen_req(uint32_t cmd_id, uint8_t *pkt)
{
    uint8_t *pt_req_pd, pd_len = 0;

    do
    {
        pd_len = ((gateway_cmd_hdr *)(pkt))->len - 7;
        pt_req_pd = sys_malloc(pd_len);

        if (!pt_req_pd)
        {
            break;
        }
        gateway_cmd_pd *pt_pd = (gateway_cmd_pd *)&pkt[sizeof(gateway_cmd_hdr)];

        memcpy(pt_req_pd, pt_pd->parameter, pd_len);

        zigbee_raf_cmd_req(pt_pd->address, cmd_id, pd_len, pt_req_pd);
    } while (0);

    if (pt_req_pd)
    {
        sys_free(pt_req_pd);
    }
}
static uint8_t _gateway_checksum_calc(uint8_t *pBuf, uint8_t len)
{
    uint8_t cs = 0;

    for (int i = 0; i < len; i++)
    {
        cs += pBuf[i];
    }
    return (~cs);
}

void zigbee_gateway_cmd_send(uint32_t cmd_id, uint16_t addr, uint8_t addr_mode, uint8_t src_endp, uint8_t *pParam, uint32_t len)
{
    uint8_t *gateway_cmd_pkt;
    uint32_t pkt_len;
    uint8_t idx = 0;

    do
    {
        pkt_len = sizeof(gateway_cmd_hdr) + sizeof(gateway_cmd_pd) + len + sizeof(gateway_cmd_end);

        if (src_endp != 0)
        {
            pkt_len += 1;
        }

        gateway_cmd_pkt = sys_malloc(pkt_len);

        if (gateway_cmd_pkt == NULL)
        {
            break;
        }

        ((gateway_cmd_hdr *)(gateway_cmd_pkt))->header[0] = 0xFF;
        ((gateway_cmd_hdr *)(gateway_cmd_pkt))->header[1] = 0xFC;
        ((gateway_cmd_hdr *)(gateway_cmd_pkt))->header[2] = 0xFC;
        ((gateway_cmd_hdr *)(gateway_cmd_pkt))->header[3] = 0xFF;
        ((gateway_cmd_hdr *)(gateway_cmd_pkt))->len = sizeof(gateway_cmd_pd) + len;

        if (src_endp != 0)
        {
            ((gateway_cmd_hdr *)(gateway_cmd_pkt))->len += 1;
        }

        idx += sizeof(gateway_cmd_hdr);

        ((gateway_cmd_pd *)(&gateway_cmd_pkt[idx]))->command_id = cmd_id;
        ((gateway_cmd_pd *)(&gateway_cmd_pkt[idx]))->address = addr;
        ((gateway_cmd_pd *)(&gateway_cmd_pkt[idx]))->address_mode = addr_mode;

        if (src_endp != 0)
        {
            ((gateway_cmd_pd *)(&gateway_cmd_pkt[idx]))->parameter[0] = src_endp;
            memcpy(((gateway_cmd_pd *)(&gateway_cmd_pkt[idx]))->parameter + 1, pParam, len);
        }
        else
        {
            memcpy(((gateway_cmd_pd *)(&gateway_cmd_pkt[idx]))->parameter, pParam, len);
        }

        idx += sizeof(gateway_cmd_pd) + len;

        if (src_endp != 0)
        {
            idx += 1;
        }
        ((gateway_cmd_end *)(&gateway_cmd_pkt[idx]))->cs = _gateway_checksum_calc((uint8_t *) & (((gateway_cmd_hdr *)(gateway_cmd_pkt))->len),
                ((gateway_cmd_hdr *)(gateway_cmd_pkt))->len + 1);


        //uart_handler_send(gateway_cmd_pkt, pkt_len);
        zb_event_get(gateway_cmd_pkt, pkt_len);
        info("------------------------      GW >>>> ------------------------\n");
        util_log_mem(UTIL_LOG_INFO, "  ", gateway_cmd_pkt, pkt_len, 0);
    } while (0);

}

static uint32_t crc32checksum(uint32_t flash_addr, uint32_t data_len)
{
    uint16_t k;
    uint32_t i;
    uint8_t *buf = ((uint8_t *)flash_addr);
    uint32_t chkSum = ~0, len = data_len;

    for (i = 0; i < len; i ++ )
    {
        chkSum ^= *buf++;
        for (k = 0; k < 8; k ++)
        {
            chkSum = chkSum & 1 ? (chkSum >> 1) ^ 0xedb88320 : chkSum >> 1;
        }
    }
    return ~chkSum;
}

static void zigbee_gw_ota_cmd_handle(uint32_t cmd_id, uint8_t *pBuf)
{
    //  +--------------------------+-------> 0x0000_0000
    //  |     Bootloader (32K)     |
    //  +--------------------------+-------> 0x0000_8000
    //  |                          |
    //  |     Application (464K)   |
    //  |                          |
    //  +--------------------------+-------> 0x0007_C000
    //  |                          |
    //  |     OTA Target  (464K)   |
    //  |                          |
    //  +--------------------------+-------> 0x000F_0000
    //  |     Reserved    (16K)    |
    //  +--------------------------+-------> 0x000F_4000

    int i;
    uint32_t status = 0;
    static uint8_t *p_tmp_buf;
    static uint32_t recv_cnt = 0;
    static uint32_t flash_addr = 0x7C000, tmp_len = 0;
    uint32_t crc32, poffset;
    ota_img_info_t *upg_data;

    // erase
    if (cmd_id == 0xF0000000)
    {

        for (i = 0; i < 0x74; i++)
        {
            // Page erase (4096 bytes)
            while (flash_check_busy());
            flash_erase(FLASH_ERASE_SECTOR, 0x7C000 + (0x1000 * i));
        }
        zigbee_gateway_cmd_send(0xF0008000, 0, 0, 0, (uint8_t *)&status, 4);
    }
    else if (cmd_id == 0xF0000001)
    {
        upg_data = (ota_img_info_t *)pBuf;
        if (upg_data->cur_pkt == 0)
        {
            memcpy((uint8_t *)&gt_img_info, pBuf, sizeof(gt_img_info));

            info("File Type: 0x%X\n", gt_img_info.image_type);
            info("Manufacturer Code: 0x%X\n", gt_img_info.manufacturer_code);
            info("File Version: 0x%X\n", gt_img_info.file_version);
            info("File Size: 0x%X\n", gt_img_info.image_size);
        }

        if (!gp_ota_imgae_cache)
        {
            gp_ota_imgae_cache = sys_malloc(0x1000);
        }

        if (gp_ota_imgae_cache)
        {
            if (upg_data->pkt_len + recv_cnt >= 0x1000)
            {
                memcpy(&gp_ota_imgae_cache[recv_cnt], upg_data->pkt, 0x1000 - recv_cnt);
                tmp_len = upg_data->pkt_len - (0x1000 - recv_cnt);
                p_tmp_buf = sys_malloc(tmp_len);
                memcpy(p_tmp_buf, &upg_data->pkt[0x1000 - recv_cnt], tmp_len);

                // page program (256 bytes)
                for (i = 0; i < 0x10; i++)
                {
                    while (flash_check_busy());
                    flash_write_page((uint32_t) & ((uint8_t *)gp_ota_imgae_cache)[i * 0x100], flash_addr);
                    flash_addr += 0x100;
                }
                recv_cnt = 0;
                memcpy(&gp_ota_imgae_cache[recv_cnt], p_tmp_buf, tmp_len);
                recv_cnt += tmp_len;

                if (p_tmp_buf)
                {
                    sys_free(p_tmp_buf);
                }
            }
            else
            {
                memcpy(&gp_ota_imgae_cache[recv_cnt], upg_data->pkt, upg_data->pkt_len);
                recv_cnt += upg_data->pkt_len;
            }
            if (upg_data->cur_pkt == (gt_img_info.total_pkt - 1))
            {
                for (i = 0; i < 0x10; i++)
                {
                    while (flash_check_busy());
                    flash_write_page((uint32_t) & ((uint8_t *)gp_ota_imgae_cache)[i * 0x100], flash_addr);
                    flash_addr += 0x100;
                }
                flush_cache();
                crc32 = crc32checksum(0x0007C000, gt_img_info.image_size);

                //if (((0x0007C000 + gt_img_info.image_size) % 0x1000) > 0)
                {
                    poffset = (0x0007C000 + gt_img_info.image_size) - ((0x0007C000 + gt_img_info.image_size) % 0x1000);
                }

                for (int i = 0; i < 0x10; i++)
                {
                    while (flash_check_busy());
                    flash_read_page((uint32_t)&gp_ota_imgae_cache[i * 0x100], poffset + (i * 0x100));
                }

                memcpy(&gp_ota_imgae_cache[((0x0007C000 + gt_img_info.image_size) % 0x1000)], &crc32, 4);

                while (flash_check_busy());
                flash_erase(FLASH_ERASE_SECTOR, poffset);

                for (int i = 0; i < 0x10; i++)
                {
                    while (flash_check_busy());
                    flash_write_page((uint32_t)&gp_ota_imgae_cache[i * 0x100], poffset + (i * 0x100));
                }
                zigbee_ota_insert_file(0x0007C000, gt_img_info.image_size + 4, gt_img_info.file_version, gt_img_info.image_type, gt_img_info.manufacturer_code);

                if (gp_ota_imgae_cache)
                {
                    sys_free(gp_ota_imgae_cache);
                    gp_ota_imgae_cache = NULL;
                }
            }
            status = upg_data->cur_pkt;
        }
        else
        {
            status = 0xFFFFFFFF;
        }

        zigbee_gateway_cmd_send(0xF0008000, 0, 0, 0, (uint8_t *)&status, 4);
    }
}

void zigbee_gateway_cmd_proc(uint8_t *pBuf, uint32_t len)
{
    uint32_t cmd_index;
    gateway_cmd_pd *pt_pd = (gateway_cmd_pd *)&pBuf[5];

    info("------------------------ >>>> GW      ------------------------\n");
    util_log_mem(UTIL_LOG_INFO, "  ", pBuf, len, 0);

    cmd_index = ((gateway_cmd_pd *)(&pBuf[5]))->command_id;

    if (cmd_index < RAF_CMD_FINISH)
    {
        if (pt_pd->command_id != RAF_CMD_NETWORK_START_REQUEST)
        {
            _cmd_common_gen_req(cmd_index, pBuf);
        }
        gw_cmd_check(cmd_index, pBuf);
    }
    else
    {
        if (cmd_index >= 0xF0000000 & cmd_index < 0xF0000002)
        {
            zigbee_gw_ota_cmd_handle(cmd_index, pt_pd->parameter);
        }
        else if ((cmd_index >= 0x10000) && (cmd_index < GW_CMD_DB_BASE))
        {
            /* transfer address mode value, "0" unicast address is ZB_APS_ADDR_MODE_16_ENDP_PRESENT, "1" group address is ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT */
            pt_pd->address_mode = (pt_pd->address_mode == 0) ? ZB_APS_ADDR_MODE_16_ENDP_PRESENT : ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
            gw_cmd_app_service_handle(cmd_index, pBuf);
        }
        else
        {
            gw_cmd_db_service_handle(cmd_index, pBuf);
        }
    }
}

zb_cmd_handler_data_sts_t zigbee_gateway_cmd_parser(uint8_t *pBuf, uint16_t plen, uint16_t *datalen, uint16_t *offset)
{
    //+-------------+-----------+---------------+------------+-----------------+--------------+--------+
    //|  Header(4)  | Length(1) | Command ID(4) | Address(2) | Address Mode(1) | Parameter(N) | CS (1) |
    //+-------------+-----------+---------------+------------+-----------------+--------------+--------+
    //| FF FC FC FF |           |               |            |                 |              |        |
    //+-------------+-----------+---------------+------------+-----------------+--------------+--------+
    zb_cmd_handler_data_sts_t t_return = ZB_DATA_INVALID;

    uint16_t i = 0;
    uint16_t idx = 0;
    uint8_t cs = 0;

    uint8_t find = 0;

    gateway_cmd_hdr *hdr = NULL;
    gateway_cmd_end *end = NULL;
    do
    {
        /* find tag */
        if (plen < 4)
        {
            break;
        }

        for (i = 0; i < plen; i++)
        {
            if ((pBuf[i] == 0xFF) && (pBuf[i + 1] == 0xFC) &&
                    (pBuf[i + 2] == 0xFC) && (pBuf[i + 3] == 0xFF))
            {
                if (offset)
                {
                    idx = i;
                }
                find = 1;
                break;
            }
        }

        if (!find)
        {
            break;
        }

        //plen -= idx;

        if ((plen - idx) < (sizeof(gateway_cmd_hdr) + sizeof(gateway_cmd_end)))
        {
            break;
        }

        hdr = (gateway_cmd_hdr *)(pBuf + idx);
        if (plen < (idx + sizeof(gateway_cmd_hdr) + hdr->len +  sizeof(gateway_cmd_end)))
        {
            break;
        }

        end = (gateway_cmd_end *)&pBuf[idx + sizeof(gateway_cmd_hdr) + hdr->len];
        cs = _gateway_checksum_calc(&pBuf[idx + 4], (hdr->len + 1));
        if (cs != end->cs)
        {
            t_return = ZB_DATA_CS_ERROR;
            err(">> %s: checksum error:0x%x, correct:0x%x\n", __FUNCTION__, end->cs, cs);
            break;
        }
        *datalen = sizeof(gateway_cmd_hdr) + hdr->len + sizeof(gateway_cmd_end);
        *offset = idx;
        t_return = ZB_DATA_VALID;

    } while (0);

    return t_return;
}
