/**
 * @file ota_download_cmd_handle.c
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
/* Utility Library APIs */
#include "util_printf.h"
#include "util_log.h"

/* BSP APIs */
#include "bsp.h"
#include "bsp_led.h"
#include "bsp_button.h"

// #include "mfs.h"

#include "app_uart_handler.h"
#include "ota_download_cmd_handler.h"
#include "ota_handler.h"
#include "mem_mgmt.h"
//=============================================================================
//                Private Function Declaration
//=============================================================================
#define SWAP_UINT32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))
//=============================================================================
//                Private ENUM
//=============================================================================

//=============================================================================
//                Private Struct
//=============================================================================
typedef struct __attribute__((packed))
{
    uint8_t header[4];
    uint8_t len;
}
ota_download_cmd_hdr;
typedef struct __attribute__((packed))
{
    uint32_t command_id;
    uint16_t address;
    uint8_t address_mode;
    uint8_t parameter[];
}
ota_download_cmd_pd;

typedef struct __attribute__((packed))
{
    uint8_t cs;
}
ota_download_cmd_end;



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

static uint16_t g_tsn_adrr_tbl[0xFF] = {0};
static uint32_t gu32_gw_start_flag = 0;
//=============================================================================
//                Functions
//=============================================================================
static uint8_t _gateway_checksum_calc(uint8_t *pBuf, uint8_t len)
{
    uint8_t cs = 0;

    for (int i = 0; i < len; i++)
    {
        cs += pBuf[i];
    }
    return (~cs);
}

void ota_download_cmd_send(uint32_t cmd_id, uint16_t addr, uint8_t addr_mode, uint8_t src_endp, uint8_t *pParam, uint32_t len)
{
    uint8_t *ota_download_cmd_pkt;
    uint32_t pkt_len;
    uint8_t idx = 0;

    do
    {
        pkt_len = sizeof(ota_download_cmd_hdr) + sizeof(ota_download_cmd_pd) + len + sizeof(ota_download_cmd_end);

        if (src_endp != 0)
        {
            pkt_len += 1;
        }

        ota_download_cmd_pkt = mem_malloc(pkt_len);

        if (ota_download_cmd_pkt == NULL)
        {
            break;
        }

        ((ota_download_cmd_hdr *)(ota_download_cmd_pkt))->header[0] = 0xFF;
        ((ota_download_cmd_hdr *)(ota_download_cmd_pkt))->header[1] = 0xFC;
        ((ota_download_cmd_hdr *)(ota_download_cmd_pkt))->header[2] = 0xFC;
        ((ota_download_cmd_hdr *)(ota_download_cmd_pkt))->header[3] = 0xFF;
        ((ota_download_cmd_hdr *)(ota_download_cmd_pkt))->len = sizeof(ota_download_cmd_pd) + len;

        if (src_endp != 0)
        {
            ((ota_download_cmd_hdr *)(ota_download_cmd_pkt))->len += 1;
        }

        idx += sizeof(ota_download_cmd_hdr);

        ((ota_download_cmd_pd *)(&ota_download_cmd_pkt[idx]))->command_id = cmd_id;
        ((ota_download_cmd_pd *)(&ota_download_cmd_pkt[idx]))->address = addr;
        ((ota_download_cmd_pd *)(&ota_download_cmd_pkt[idx]))->address_mode = addr_mode;

        if (src_endp != 0)
        {
            ((ota_download_cmd_pd *)(&ota_download_cmd_pkt[idx]))->parameter[0] = src_endp;
            memcpy(((ota_download_cmd_pd *)(&ota_download_cmd_pkt[idx]))->parameter + 1, pParam, len);
        }
        else
        {
            memcpy(((ota_download_cmd_pd *)(&ota_download_cmd_pkt[idx]))->parameter, pParam, len);
        }

        idx += sizeof(ota_download_cmd_pd) + len;

        if (src_endp != 0)
        {
            idx += 1;
        }
        ((ota_download_cmd_end *)(&ota_download_cmd_pkt[idx]))->cs = _gateway_checksum_calc((uint8_t *) & (((ota_download_cmd_hdr *)(ota_download_cmd_pkt))->len),
                ((ota_download_cmd_hdr *)(ota_download_cmd_pkt))->len + 1);

#if 0
        {
            uint8_t i = 0;

            info(">>Send cmd: total lenght: %d \n", (sizeof(ota_download_cmd_hdr) + (((ota_download_cmd_hdr *)(ota_download_cmd_pkt))->len) + sizeof(ota_download_cmd_end)));

            info("| ");

            /* show Header(4) */
            for (i = 0; i < (sizeof(ota_download_cmd_hdr) - 1); i++)
            {
                info("%02X ", ota_download_cmd_pkt[i]);
            }

            info("| ");

            /* show Length(1) */
            info("%02X ", ota_download_cmd_pkt[(sizeof(ota_download_cmd_hdr) - 1)]);

            info("| ");

            /* show Data(N) */
            for (i = sizeof(ota_download_cmd_hdr); i < (sizeof(ota_download_cmd_hdr) + (((ota_download_cmd_hdr *)(ota_download_cmd_pkt))->len)); i++)
            {
                info("%02X ", ota_download_cmd_pkt[i]);
            }
            info("| ");

            /* show CRC(1) */
            info("%02X ", (((ota_download_cmd_end *)(&ota_download_cmd_pkt[((sizeof(ota_download_cmd_hdr) + (((ota_download_cmd_hdr *)(ota_download_cmd_pkt))->len)))]))->cs));

            info("|\n");
            /* GP add, debug */

        }
#endif
        msg(UTIL_LOG_DEBUG, "------------------------      GW >>>> ------------------------\n");
        util_log_mem(UTIL_LOG_DEBUG, "  ", ota_download_cmd_pkt, pkt_len, 0);
        app_uart_handler_send(ota_download_cmd_pkt, pkt_len);
        if (ota_download_cmd_pkt)
        {
            mem_free(ota_download_cmd_pkt);
        }
    } while (0);

}

static void ota_download_handle(uint32_t cmd_id, uint8_t *pBuf)
{
    //  +--------------------------+-------> 0x0000_0000
    //  |     Bootloader (32K)     |
    //  +--------------------------+-------> 0x0000_8000
    //  |                          |
    //  |     Application (580K)   |
    //  |                          |
    //  +--------------------------+-------> 0x0009_9000
    //  |                          |
    //  |     OTA Target  (348K)   |
    //  |                          |
    //  +--------------------------+-------> 0x000F_0000
    //  |     Reserved    (16K)    |
    //  +--------------------------+-------> 0x000F_4000

    int i;
    uint32_t status = 0;
    static uint8_t *p_tmp_buf;
    static uint32_t recv_cnt = 0;
    static uint32_t flash_addr = FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB, tmp_len = 0;
    uint32_t crc32, poffset;
    ota_img_info_t *upg_data;

    // erase
    if (cmd_id == 0xF0000000)
    {
        for (i = 0; i < 0x57; i++)
        {
            // Page erase (4096 bytes)
            while (flash_check_busy());
            flash_erase(FLASH_ERASE_SECTOR, FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB + (0x1000 * i));
        }
        ota_download_cmd_send(0xF0008000, 0, 0, 0, (uint8_t *)&status, 4);
    }
    else if (cmd_id == 0xF0000001)
    {
        do
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
                gp_ota_imgae_cache = mem_malloc(0x1000);
            }

            if (gp_ota_imgae_cache)
            {
                if (upg_data->pkt_len + recv_cnt >= 0x1000)
                {
                    memcpy(&gp_ota_imgae_cache[recv_cnt], upg_data->pkt, 0x1000 - recv_cnt);
                    tmp_len = upg_data->pkt_len - (0x1000 - recv_cnt);
                    p_tmp_buf = mem_malloc(tmp_len);
                    if (p_tmp_buf)
                    {

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
                            mem_free(p_tmp_buf);
                        }
                    }
                    else
                    {
                        status = 0xFFFFFFFF;
                        info("alloc fail \n");
                        break;
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
                    crc32 = crc32checksum(FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB, gt_img_info.image_size);
                    // info("crc32 %x \n", crc32);
                    //if (((FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB + gt_img_info.image_size) % 0x1000) > 0)
                    {
                        poffset = (FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB + gt_img_info.image_size) - ((FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB + gt_img_info.image_size) % 0x1000);
                    }

                    for (int i = 0; i < 0x10; i++)
                    {
                        while (flash_check_busy());
                        flash_read_page((uint32_t)&gp_ota_imgae_cache[i * 0x100], poffset + (i * 0x100));
                    }

                    memcpy(&gp_ota_imgae_cache[((FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB + gt_img_info.image_size) % 0x1000)], &crc32, 4);

                    while (flash_check_busy());
                    flash_erase(FLASH_ERASE_SECTOR, poffset);

                    for (int i = 0; i < 0x10; i++)
                    {
                        while (flash_check_busy());
                        flash_write_page((uint32_t)&gp_ota_imgae_cache[i * 0x100], poffset + (i * 0x100));
                    }
                    /*ota use*/
                    static uint8_t read_buf[0x100];
                    while (flash_check_busy());
                    flash_read_page((uint32_t)(read_buf), FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB);
                    while (flash_check_busy());
                    ota_set_image_size(SWAP_UINT32(*(uint32_t *)(read_buf + 24)) + 0x20);
                    ota_set_image_version(SWAP_UINT32(*(uint32_t *)read_buf));
                    ota_set_image_crc(SWAP_UINT32(*(uint32_t *)(read_buf + 16)));
                    ota_bootinfo_reset();

                    // zigbee_ota_insert_file(FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB, gt_img_info.image_size + 4, gt_img_info.file_version);

                    if (gp_ota_imgae_cache)
                    {
                        if (gp_ota_imgae_cache)
                        {
                            mem_free(gp_ota_imgae_cache);
                        }
                        gp_ota_imgae_cache = NULL;
                    }
                }
                status = upg_data->cur_pkt;
            }
            else
            {
                status = 0xFFFFFFFF;
            }
        } while (0);
        ota_download_cmd_send(0xF0008000, 0, 0, 0, (uint8_t *)&status, 4);
    }

}
void ota_download_cmd_proc(uint8_t *pBuf, uint32_t len)
{
    uint32_t cmd_index;
    uint32_t cmdID = ((ota_download_cmd_pd *)(&pBuf[5]))->command_id;
    uint8_t timeoutNum = 0;

    msg(UTIL_LOG_DEBUG, "------------------------ >>>> GW      ------------------------\n");
    util_log_mem(UTIL_LOG_DEBUG, "  ", pBuf, len, 0);

    cmd_index = cmdID;
    ota_download_cmd_pd *pt_pd = (ota_download_cmd_pd *)&pBuf[5];

    if (cmd_index >= 0xF0000000 & cmd_index < 0xF0000002)
    {
        ota_download_handle(cmd_index, pt_pd->parameter);
    }
}

uart_handler_data_sts_t
ota_download_cmd_parser(uint8_t *pBuf,
                        uint16_t plen,
                        uint16_t *datalen,
                        uint16_t *offset)
{
    //+-------------+-----------+---------------+------------+-----------------+--------------+--------+
    //|  Header(4)  | Length(1) | Command ID(4) | Address(2) | Address Mode(1) | Parameter(N) | CS (1) |
    //+-------------+-----------+---------------+------------+-----------------+--------------+--------+
    //| FF FC FC FF |           |               |            |                 |              |        |
    //+-------------+-----------+---------------+------------+-----------------+--------------+--------+
    uart_handler_data_sts_t t_return = UART_DATA_INVALID;

    uint16_t i = 0;
    uint16_t idx = 0;
    uint8_t cs = 0;

    uint8_t find = 0;
    uint16_t totalLen = 0;

    ota_download_cmd_hdr *hdr = NULL;
    ota_download_cmd_end *end = NULL;
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

        if ((plen - idx) < (sizeof(ota_download_cmd_hdr) + sizeof(ota_download_cmd_end)))
        {
            break;
        }

        hdr = (ota_download_cmd_hdr *)(pBuf + idx);
        if (plen < (idx + sizeof(ota_download_cmd_hdr) + hdr->len +  sizeof(ota_download_cmd_end)))
        {
            break;
        }

        end = (ota_download_cmd_end *)&pBuf[idx + sizeof(ota_download_cmd_hdr) + hdr->len];
        cs = _gateway_checksum_calc(&pBuf[idx + 4], (hdr->len + 1));

#if 0
        //  else if (cs == end->cs)   /* show receved command data */
        if (1)
        {
            totalLen = (sizeof(ota_download_cmd_hdr) + hdr->len + sizeof(ota_download_cmd_end));
            info("\n<<Recv cmd: total lenght: %d, Checksum: Calc(0x%x),Correct(0x%x) \n", totalLen, cs, end->cs);

            info("| ");

            /* show Header(4) */
            for (i = 0; i < (sizeof(ota_download_cmd_hdr) - 1); i++)
            {
                info("%02X ", pBuf[idx + i]);
            }
            info("| ");
            /* show Length(1) */
            info("%02X ", pBuf[idx + (sizeof(ota_download_cmd_hdr) - 1)]);
            info("| ");

            /* show Data(N) */
            for (i = sizeof(ota_download_cmd_hdr); i < (sizeof(ota_download_cmd_hdr) + hdr->len); i++)
            {
                info("%02X ", pBuf[idx + i]);
            }
            info("| ");
            /* show Length(1) */
            info("%02X ", end->cs);
            info("|\n");
        }
#endif

        if (cs != end->cs)
        {
            t_return = UART_DATA_CS_ERROR;
            err(">> %s: checksum error:0x%x, correct:0x%x\n", __FUNCTION__, end->cs, cs);
            break;
        }
        else if (cs == end->cs)   /* show receved command data */
        {

            *datalen = sizeof(ota_download_cmd_hdr) + hdr->len + sizeof(ota_download_cmd_end);
            *offset = idx;
            t_return = UART_DATA_VALID_CRC_OK;
            break;
        }


        *datalen = sizeof(ota_download_cmd_hdr) + hdr->len + sizeof(ota_download_cmd_end);
        *offset = idx;
        t_return = UART_DATA_VALID;

    } while (0);

    return t_return;
}