/** @file cli_cmd_gateway.c
 *
 * @author Rex
 * @version 0.1
 * @date 2022/01/17
 * @license
 * @description
 */


#include "project_config.h"
#include "types.h"
#include "util_string.h"
#include "bsp_console.h"

#include "shell.h"
#include "sys_arch.h"
#include "FreeRTOS.h"
#include "task.h"

#include "mem_mgmt.h"

#include "zigbee_stack_api.h"
#include "zigbee_lib_api.h"

#include "zigbee_app.h"
#include "zigbee_evt_handler.h"

//=============================================================================
//                  Constant Definition
//=============================================================================

//=============================================================================
//                  Macro Definition
//=============================================================================

//=============================================================================
//                  Structure Definition
//=============================================================================

//=============================================================================
//                  Global Data Definition
//=============================================================================
extern zb_uint8_t custom_dst_endpoint;
extern zb_uint16_t custom_dst_short_addr;
static int start_flag = 0;
//=============================================================================
//                  Private Function Definition
//=============================================================================
static int
_cli_cmd_s2e(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_app_dump_device_table();
    return 0;
}

static int
_cli_cmd_shdev_num(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    uint32_t dev_num = 0U;
    dev_num = zigbee_app_get_device_num();

    info_color(LOG_GREEN, "Device joined number: %d\n", dev_num);
    return 0;
}


static int
_cli_cmd_xmodem(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    return 0;
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

static int
_cli_cmd_ota(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    uint32_t *psize = (uint32_t *)0x00007c000;
    uint32_t poffset;
    uint32_t size = *psize;
    uint32_t version;
    uint32_t crc32 = crc32checksum(0x0007C100, size);
    uint8_t *p_cache;

    do
    {
        p_cache = sys_malloc(0x1000);

        if (p_cache == NULL)
        {
            break;
        }

        size += (((0x0007C100 + size) % 0x100) != 0)
                ? (0x100 - ((0x0007C100 + size) % 0x100))
                : 0;

        if (((0x0007C100 + size) % 0x1000) > 0)
        {
            poffset = (0x0007C100 + size) - ((0x0007C100 + size) % 0x1000);
        }

        for (int i = 0; i < 0x10; i++)
        {
            while (flash_check_busy());
            flash_read_page((uint32_t)&p_cache[i * 0x100], poffset + (i * 0x100));
        }

        memcpy(&p_cache[((0x0007C100 + size) % 0x1000)], &crc32, 4);

        while (flash_check_busy());
        flash_erase(FLASH_ERASE_PAGE, poffset);

        for (int i = 0; i < 0x10; i++)
        {
            while (flash_check_busy());
            flash_write_page((uint32_t)&p_cache[i * 0x100], poffset + (i * 0x100));
        }

        size += 4;

        log_out("size %d\n", size);
        log_out("crc %x\n", crc32);

        version = (*(argv[1] + 1) == 'x')
                  ? utility_strtox(argv[1] + 2, 0)
                  : utility_strtol(argv[1], 0);

        zigbee_ota_insert_file(0x0007C100, size, version);

        if (p_cache)
        {
            sys_free(p_cache);
        }
    } while (0);
    return 0;
}

static int
_cli_cmd_zcl_on_off(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_data_req_t *pt_data_req;
    uint16_t addr;
    uint8_t on_off_cmd, ep;

    do
    {
        if (argc < 4)
        {
            break;
        }

        addr = (*(argv[1] + 1) == 'x')
               ? utility_strtox(argv[1] + 2, 0)
               : utility_strtol(argv[1], 0);

        ep = (*(argv[2] + 1) == 'x')
             ? utility_strtox(argv[2] + 2, 0)
             : utility_strtol(argv[2], 0);

        on_off_cmd = (*(argv[3] + 1) == 'x')
                     ? utility_strtox(argv[3] + 2, 0)
                     : utility_strtol(argv[3], 0);

        ZIGBEE_ZCL_DATA_REQ(pt_data_req, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ep, ZIGBEE_DEFAULT_ENDPOINT,
                            ZB_ZCL_CLUSTER_ID_ON_OFF,
                            on_off_cmd,
                            TRUE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, 0)

        if (pt_data_req)
        {
            zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}

static int
_cli_cmd_zcl_attr_read(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_attr_rw_t *pt_data_req;
    uint16_t dst_addr, cluster_ID, attr_ID;
    uint8_t dst_ep;

    do
    {
        if (argc < 5)
        {
            break;
        }

        dst_addr = (*(argv[1] + 1) == 'x')
                   ? utility_strtox(argv[1] + 2, 0)
                   : utility_strtol(argv[1], 0);

        dst_ep = (*(argv[2] + 1) == 'x')
                 ? utility_strtox(argv[2] + 2, 0)
                 : utility_strtol(argv[2], 0);

        cluster_ID = (*(argv[3] + 1) == 'x')
                     ? utility_strtox(argv[3] + 2, 0)
                     : utility_strtol(argv[3], 0);

        attr_ID = (*(argv[4] + 1) == 'x')
                  ? utility_strtox(argv[4] + 2, 0)
                  : utility_strtol(argv[4], 0);


        ZIGBEE_ZCL_ATTR_RW_REQ(pt_data_req, dst_addr, dst_ep, ZIGBEE_DEFAULT_ENDPOINT, cluster_ID,
                               attr_ID,
                               0,
                               0)

        if (pt_data_req)
        {
            zigbee_zcl_attr_read_request(pt_data_req, 0);
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}

static int
_cli_cmd_zcl_attr_write(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_attr_rw_t *pt_data_req;
    uint32_t attr_value;
    uint16_t dst_addr, cluster_ID, attr_ID;
    uint8_t dst_ep, attr_Type, len = 0, i;

    do
    {
        if (argc < 7)
        {
            break;
        }

        dst_addr = (*(argv[1] + 1) == 'x')
                   ? utility_strtox(argv[1] + 2, 0)
                   : utility_strtol(argv[1], 0);

        dst_ep = (*(argv[2] + 1) == 'x')
                 ? utility_strtox(argv[2] + 2, 0)
                 : utility_strtol(argv[2], 0);

        cluster_ID = (*(argv[3] + 1) == 'x')
                     ? utility_strtox(argv[3] + 2, 0)
                     : utility_strtol(argv[3], 0);

        attr_ID = (*(argv[4] + 1) == 'x')
                  ? utility_strtox(argv[4] + 2, 0)
                  : utility_strtol(argv[4], 0);

        attr_Type = (*(argv[5] + 1) == 'x')
                    ? utility_strtox(argv[5] + 2, 0)
                    : utility_strtol(argv[5], 0);

        if (attr_Type == ZB_ZCL_ATTR_TYPE_OCTET_STRING ||
                attr_Type == ZB_ZCL_ATTR_TYPE_CHAR_STRING)
        {
            len = strlen(argv[6]) + 1;
        }
        else if (attr_Type == ZB_ZCL_ATTR_TYPE_LONG_OCTET_STRING )
        {
            len = strlen(argv[6]) + 2;
        }
        else
        {
            attr_value = (*(argv[6] + 1) == 'x')
                         ? utility_strtox(argv[6] + 2, 0)
                         : utility_strtol(argv[6], 0);
            len = sizeof(attr_value);
        }
        ZIGBEE_ZCL_ATTR_RW_REQ(pt_data_req, dst_addr, dst_ep, ZIGBEE_DEFAULT_ENDPOINT, cluster_ID,
                               attr_ID,
                               attr_Type,
                               len)

        if (pt_data_req)
        {
            for (i = 0; i < len; i++)
            {

                if (attr_Type == ZB_ZCL_ATTR_TYPE_OCTET_STRING ||
                        attr_Type == ZB_ZCL_ATTR_TYPE_CHAR_STRING)
                {
                    if (i == 0)
                    {
                        pt_data_req->attrValue[i] = strlen(argv[6]);
                    }
                    else
                    {
                        pt_data_req->attrValue[i] = *(argv[6] + i - 1);
                    }
                }
                else if (attr_Type == ZB_ZCL_ATTR_TYPE_LONG_OCTET_STRING)
                {
                    if (i < 2)
                    {
                        pt_data_req->attrValue[0] = strlen(argv[6]) & 0xFF;
                        pt_data_req->attrValue[1] = (strlen(argv[6]) >> 8) & 0xFF;
                    }
                    else
                    {
                        pt_data_req->attrValue[i] = *(argv[6] + i - 2);
                    }
                }
                else
                {
                    pt_data_req->attrValue[i] = (attr_value >> 8 * i) & 0xFF;
                }
            }

            zigbee_zcl_attr_wirte_request(pt_data_req, len);
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}

static int
_cli_cmd_zcl_group(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_data_req_t *pt_data_req;
    uint16_t addr, group_id, ep, cmd;

    do
    {
        if (argc < 4)
        {
            break;
        }

        if (memcmp("a", argv[1], 1) == 0)
        {
            cmd = ZB_ZCL_CMD_GROUPS_ADD_GROUP;
        }
        else if (memcmp("r", argv[1], 1) == 0)
        {
            cmd = ZB_ZCL_CMD_GROUPS_REMOVE_GROUP;
        }
        else
        {
            break;
        }

        addr = (*(argv[2] + 1) == 'x')
               ? utility_strtox(argv[2] + 2, 0)
               : utility_strtol(argv[2], 0);
        ep = (*(argv[3] + 1) == 'x')
             ? utility_strtox(argv[3] + 2, 0)
             : utility_strtol(argv[3], 0);
        group_id = (*(argv[4] + 1) == 'x')
                   ? utility_strtox(argv[4] + 2, 0)
                   : utility_strtol(argv[4], 0);


        ZIGBEE_ZCL_DATA_REQ(pt_data_req, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ep, ZIGBEE_DEFAULT_ENDPOINT,
                            ZB_ZCL_CLUSTER_ID_GROUPS,
                            cmd,
                            TRUE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, 3)

        if (pt_data_req)
        {
            pt_data_req->cmdFormat[0] = group_id & 0xFF;
            pt_data_req->cmdFormat[1] = (group_id >> 8) & 0xFF;
            pt_data_req->cmdFormat[2] = 0x00;     /* string lenght */
            zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}

static int
_cli_cmd_zcl_identify(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_data_req_t *pt_data_req;
    uint16_t addr;
    uint8_t ep;

    do
    {
        if (argc < 3)
        {
            break;
        }

        addr = (*(argv[1] + 1) == 'x')
               ? utility_strtox(argv[1] + 2, 0)
               : utility_strtol(argv[1], 0);

        ep = (*(argv[2] + 1) == 'x')
             ? utility_strtox(argv[2] + 2, 0)
             : utility_strtol(argv[2], 0);



        ZIGBEE_ZCL_DATA_REQ(pt_data_req, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ep, ZIGBEE_DEFAULT_ENDPOINT,
                            ZB_ZCL_CLUSTER_ID_IDENTIFY,
                            ZB_ZCL_CMD_IDENTIFY_IDENTIFY_ID,
                            TRUE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, 2)

        if (pt_data_req)
        {
            pt_data_req->cmdFormat[0] = 0x05;
            pt_data_req->cmdFormat[1] = 0x00;
            zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}

static int
_cli_cmd_setpt(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_data_req_t *pt_data_req;
    uint16_t addr, amount;
    uint8_t ep, mode;

    do
    {
        if (argc < 5)
        {
            break;
        }

        addr = (*(argv[1] + 1) == 'x')
               ? utility_strtox(argv[1] + 2, 0)
               : utility_strtol(argv[1], 0);

        ep = (*(argv[2] + 1) == 'x')
             ? utility_strtox(argv[2] + 2, 0)
             : utility_strtol(argv[2], 0);

        mode = (*(argv[3] + 1) == 'x')
               ? utility_strtox(argv[3] + 2, 0)
               : utility_strtol(argv[3], 0);

        amount = (*(argv[4] + 1) == 'x')
                 ? utility_strtox(argv[4] + 2, 0)
                 : utility_strtol(argv[4], 0);



        ZIGBEE_ZCL_DATA_REQ(pt_data_req, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ep, ZIGBEE_DEFAULT_ENDPOINT,
                            ZB_ZCL_CLUSTER_ID_THERMOSTAT,
                            ZB_ZCL_CMD_THERMOSTAT_SETPOINT_RAISE_LOWER,
                            TRUE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, 2)

        if (pt_data_req)
        {
            pt_data_req->cmdFormat[0] = mode;
            pt_data_req->cmdFormat[1] = amount;
            zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}

static int
_cli_cmd_zcl_level_ctrl(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_data_req_t *pt_data_req;
    uint16_t addr;
    uint8_t ep, dir;

    do
    {
        if (argc < 4)
        {
            break;
        }

        if (memcmp("up", argv[1], 2) == 0)
        {
            dir = 0x00;
        }
        else if (memcmp("dn", argv[1], 2) == 0)
        {
            dir = 0x01;
        }
        addr = (*(argv[2] + 1) == 'x')
               ? utility_strtox(argv[2] + 2, 0)
               : utility_strtol(argv[2], 0);

        ep = (*(argv[3] + 1) == 'x')
             ? utility_strtox(argv[3] + 2, 0)
             : utility_strtol(argv[3], 0);



        ZIGBEE_ZCL_DATA_REQ(pt_data_req, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ep, ZIGBEE_DEFAULT_ENDPOINT,
                            ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
                            ZB_ZCL_CMD_LEVEL_CONTROL_STEP_WITH_ON_OFF,
                            TRUE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, 4)

        if (pt_data_req)
        {
            pt_data_req->cmdFormat[0] = dir;
            pt_data_req->cmdFormat[1] = 0x0F;
            pt_data_req->cmdFormat[2] = 0x05;
            pt_data_req->cmdFormat[3] = 0x00;
            zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}
static int
_cli_cmd_zcl_scene(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_data_req_t *pt_data_req;
    uint16_t addr, group_id, scence_id, cmd, ep;

    do
    {

        if (memcmp("s", argv[1], 1) == 0)
        {
            cmd = ZB_ZCL_CMD_SCENES_STORE_SCENE;
        }
        else if (memcmp("rc", argv[1], 2) == 0)
        {
            cmd = ZB_ZCL_CMD_SCENES_RECALL_SCENE;
        }
        else if (memcmp("re", argv[1], 2) == 0)
        {
            cmd = ZB_ZCL_CMD_SCENES_REMOVE_SCENE;
        }
        else if (memcmp("v", argv[1], 1) == 0)
        {
            cmd = ZB_ZCL_CMD_SCENES_VIEW_SCENE;
        }

        addr = (*(argv[2] + 1) == 'x')
               ? utility_strtox(argv[2] + 2, 0)
               : utility_strtol(argv[2], 0);

        ep = (*(argv[3] + 1) == 'x')
             ? utility_strtox(argv[3] + 2, 0)
             : utility_strtol(argv[3], 0);

        group_id = (*(argv[4] + 1) == 'x')
                   ? utility_strtox(argv[4] + 2, 0)
                   : utility_strtol(argv[4], 0);

        scence_id = (*(argv[5] + 1) == 'x')
                    ? utility_strtox(argv[5] + 2, 0)
                    : utility_strtol(argv[5], 0);



        ZIGBEE_ZCL_DATA_REQ(pt_data_req, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, (ep & 0xFF), ZIGBEE_DEFAULT_ENDPOINT,
                            ZB_ZCL_CLUSTER_ID_SCENES,
                            cmd,
                            TRUE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, 3)

        if (pt_data_req)
        {
            pt_data_req->cmdFormat[0] = group_id & 0xFF;
            pt_data_req->cmdFormat[1] = (group_id >> 8) & 0xFF;

            pt_data_req->cmdFormat[2] = scence_id & 0xFF;

            zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}

static int
_cli_cmd_zcl_config_report(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_data_req_t *pt_data_req;
    uint16_t addr, cluster_id, attr_id, min_interval, max_interval;
    uint8_t ep, data_type, data_len;

    do
    {
        if (argc < 6)
        {
            break;
        }

        addr = (*(argv[1] + 1) == 'x')
               ? utility_strtox(argv[1] + 2, 0)
               : utility_strtol(argv[1], 0);

        ep = (*(argv[2] + 1) == 'x')
             ? utility_strtox(argv[2] + 2, 0)
             : utility_strtol(argv[2], 0);

        cluster_id = (*(argv[3] + 1) == 'x')
                     ? utility_strtox(argv[3] + 2, 0)
                     : utility_strtol(argv[3], 0);

        attr_id = (*(argv[4] + 1) == 'x')
                  ? utility_strtox(argv[4] + 2, 0)
                  : utility_strtol(argv[4], 0);

        data_type = (*(argv[5] + 1) == 'x')
                    ? utility_strtox(argv[5] + 2, 0)
                    : utility_strtol(argv[5], 0);

        min_interval = (*(argv[6] + 1) == 'x')
                       ? utility_strtox(argv[6] + 2, 0)
                       : utility_strtol(argv[6], 0);

        max_interval = (*(argv[7] + 1) == 'x')
                       ? utility_strtox(argv[7] + 2, 0)
                       : utility_strtol(argv[7], 0);

        switch (data_type)//check if it is analog data type
        {
        case ZB_ZCL_ATTR_TYPE_U8:
        case ZB_ZCL_ATTR_TYPE_S8:
            data_len = 9;
            break;
        case ZB_ZCL_ATTR_TYPE_U16:
        case ZB_ZCL_ATTR_TYPE_S16:
            data_len = 10;
            break;
        case ZB_ZCL_ATTR_TYPE_U24:
        case ZB_ZCL_ATTR_TYPE_S24:
            data_len = 11;
            break;
        case ZB_ZCL_ATTR_TYPE_U32:
        case ZB_ZCL_ATTR_TYPE_S32:
        case ZB_ZCL_ATTR_TYPE_UTC_TIME:
            data_len = 12;
            break;
        case ZB_ZCL_ATTR_TYPE_U48:
        case ZB_ZCL_ATTR_TYPE_S48:
            data_len = 14;
            break;
        default:
            data_len = 8;
            break;
        }

        ZIGBEE_ZCL_DATA_REQ(pt_data_req, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ep, ZIGBEE_DEFAULT_ENDPOINT,
                            cluster_id,
                            ZB_ZCL_CMD_CONFIG_REPORT,
                            FALSE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, data_len)

        if (pt_data_req)
        {
            pt_data_req->cmdFormat[0] = 0x00;
            pt_data_req->cmdFormat[1] = attr_id & 0xFF;
            pt_data_req->cmdFormat[2] = (attr_id >> 8) & 0xFF;
            pt_data_req->cmdFormat[3] = data_type;
            pt_data_req->cmdFormat[4] = min_interval & 0xFF;
            pt_data_req->cmdFormat[5] = (min_interval >> 8) & 0xFF;
            pt_data_req->cmdFormat[6] = max_interval & 0xFF;
            pt_data_req->cmdFormat[7] = (max_interval >> 8) & 0xFF;
            if (data_len > 8)
            {
                memset(pt_data_req->cmdFormat + 8, 0x00, data_len - 8);
            }
            zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}
static int
_cli_cmd_zdo_act_ep_req(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    uint16_t addr;

    do
    {
        if (argc < 2)
        {
            break;
        }

        addr = (*(argv[1] + 1) == 'x')
               ? utility_strtox(argv[1] + 2, 0)
               : utility_strtol(argv[1], 0);

        zigbee_act_ep_req(addr);
    } while (0);
    return 0;
}

static int
_cli_cmd_zdo_simple_desc_req(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    uint16_t addr;
    uint8_t endpoint;

    do
    {
        if (argc < 3)
        {
            break;
        }

        addr = (*(argv[1] + 1) == 'x')
               ? utility_strtox(argv[1] + 2, 0)
               : utility_strtol(argv[1], 0);

        endpoint = (*(argv[2] + 1) == 'x')
                   ? utility_strtox(argv[2] + 2, 0)
                   : utility_strtol(argv[2], 0);

        zigbee_simple_desc_req(addr, endpoint);
        info("tick %d\n", sys_now());
    } while (0);
    return 0;
}

extern void zb_get_long_address(zb_ieee_addr_t addr);
static int
_cli_cmd_zdo_bind_req(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    uint16_t req_addr;
    uint16_t dst_addr, cluster_id;
    uint8_t src_endpoint, dst_endpoint;
    uint8_t dstAddrMode;
    uint8_t src_ieeeaddr[8], dst_ieeeaddr[8];
    int i;

    memset(src_ieeeaddr, 0, 8);
    memset(dst_ieeeaddr, 0, 8);
    do
    {
        if ((argc < 6))
        {
            break;
        }

        dstAddrMode = (*(argv[1] + 1) == 'x')
                      ? utility_strtox(argv[1] + 2, 0)
                      : utility_strtol(argv[1], 0);

        req_addr = (*(argv[2] + 1) == 'x')
                   ? utility_strtox(argv[2] + 2, 0)
                   : utility_strtol(argv[2], 0);

        src_endpoint = (*(argv[3] + 1) == 'x')
                       ? utility_strtox(argv[3] + 2, 0)
                       : utility_strtol(argv[3], 0);

        dst_addr = (*(argv[4] + 1) == 'x')
                   ? utility_strtox(argv[4] + 2, 0)
                   : utility_strtol(argv[4], 0);

        dst_endpoint = (*(argv[5] + 1) == 'x')
                       ? utility_strtox(argv[5] + 2, 0)
                       : utility_strtol(argv[5], 0);

        cluster_id = (*(argv[6] + 1) == 'x')
                     ? utility_strtox(argv[6] + 2, 0)
                     : utility_strtol(argv[6], 0);



        for (i = 0; i < 200; i++)
        {
            if (gt_app_db.device_table[i].short_addr == req_addr)
            {
                memcpy(src_ieeeaddr, gt_app_db.device_table[i].ieee_addr, 8);
                break;
            }
        }
        if (i == 200)
        {
            return 1;
        }

        if (dstAddrMode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
        {
            if (dst_addr == 0x0000)
            {
                zb_get_long_address(dst_ieeeaddr);
            }
            else
            {

                for (i = 0; i < 200; i++)
                {
                    if (gt_app_db.device_table[i].short_addr == dst_addr)
                    {
                        memcpy(dst_ieeeaddr, gt_app_db.device_table[i].ieee_addr, 8);
                        break;
                    }
                }

                if (i == 200)
                {
                    info("address not found!\n");
                    return 1;
                }
            }
            zigbee_bind_req(1, src_endpoint, src_ieeeaddr, dst_endpoint, dst_ieeeaddr, dstAddrMode, cluster_id, req_addr);
        }
        else if (dstAddrMode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT)
        {
            zigbee_bind_req(1, src_endpoint, src_ieeeaddr, dst_endpoint, (uint8_t *)&dst_addr, dstAddrMode, cluster_id, req_addr);
        }
        else
        {
            info("Unsupported address mode\n");
            break;
        }

    } while (0);
    return 0;
}

static int
_cli_cmd_zdo_unbind_req(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    uint16_t req_addr;
    uint16_t dst_addr, cluster_id;
    uint8_t src_endpoint, dst_endpoint;
    uint8_t dstAddrMode;
    uint8_t src_ieeeaddr[8], dst_ieeeaddr[8];
    int i;

    memset(src_ieeeaddr, 0, 8);
    memset(dst_ieeeaddr, 0, 8);
    do
    {
        if ((argc < 6))
        {
            break;
        }

        dstAddrMode = (*(argv[1] + 1) == 'x')
                      ? utility_strtox(argv[1] + 2, 0)
                      : utility_strtol(argv[1], 0);

        req_addr = (*(argv[2] + 1) == 'x')
                   ? utility_strtox(argv[2] + 2, 0)
                   : utility_strtol(argv[2], 0);

        src_endpoint = (*(argv[3] + 1) == 'x')
                       ? utility_strtox(argv[3] + 2, 0)
                       : utility_strtol(argv[3], 0);

        dst_addr = (*(argv[4] + 1) == 'x')
                   ? utility_strtox(argv[4] + 2, 0)
                   : utility_strtol(argv[4], 0);

        dst_endpoint = (*(argv[5] + 1) == 'x')
                       ? utility_strtox(argv[5] + 2, 0)
                       : utility_strtol(argv[5], 0);

        cluster_id = (*(argv[6] + 1) == 'x')
                     ? utility_strtox(argv[6] + 2, 0)
                     : utility_strtol(argv[6], 0);



        for (i = 0; i < 200; i++)
        {
            if (gt_app_db.device_table[i].short_addr == req_addr)
            {
                memcpy(src_ieeeaddr, gt_app_db.device_table[i].ieee_addr, 8);
                break;
            }
        }
        if (i == 200)
        {
            return 1;
        }

        if (dstAddrMode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
        {
            if (dst_addr == 0x0000)
            {
                zb_get_long_address(dst_ieeeaddr);
            }
            else
            {

                for (i = 0; i < 200; i++)
                {
                    if (gt_app_db.device_table[i].short_addr == dst_addr)
                    {
                        memcpy(dst_ieeeaddr, gt_app_db.device_table[i].ieee_addr, 8);
                        break;
                    }
                }

                if (i == 200)
                {
                    info("address not found!\n");
                    return 1;
                }
            }
            zigbee_bind_req(0, src_endpoint, src_ieeeaddr, dst_endpoint, dst_ieeeaddr, dstAddrMode, cluster_id, req_addr);
        }
        else if (dstAddrMode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT)
        {
            zigbee_bind_req(0, src_endpoint, src_ieeeaddr, dst_endpoint, (uint8_t *)&dst_addr, dstAddrMode, cluster_id, req_addr);
        }
        else
        {
            info("Unsupported address mode\n");
            break;
        }
    } while (0);
    return 0;
}
static int
_cli_cmd_permit_join(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    uint16_t enable, timeout;

    do
    {
        enable = (*(argv[1] + 1) == 'x')
                 ? utility_strtox(argv[1] + 2, 0)
                 : utility_strtol(argv[1], 0);
        if (!enable)
        {
            timeout = 0;
        }
        else if (argc == 3)
        {
            timeout = (*(argv[2] + 1) == 'x')
                      ? utility_strtox(argv[2] + 2, 0)
                      : utility_strtol(argv[2], 0);
        }

        zigbee_send_permit_join(0x0000, timeout);
        zigbee_send_permit_join(0xfffc, timeout);
    } while (0);
    return 0;
}
static int
_cli_cmd_ed_scan(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_mac_ed_scan_req();
    return 0;
}

static int
_cli_cmd_start_pan(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    uint16_t channel, panId, maxChild, reset;
    uint8_t  extPANID[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    do
    {
        if (argc < 5)
        {
            log_out("Invalid parameter\n");
            break;
        }

        if (start_flag == 1)
        {
            info_color(LOG_RED, "Device already started\n");
            break;
        }
        reset = (*(argv[1] + 1) == 'x')
                ? utility_strtox(argv[1] + 2, 0)
                : utility_strtol(argv[1], 0);
        channel = (*(argv[2] + 1) == 'x')
                  ? utility_strtox(argv[2] + 2, 0)
                  : utility_strtol(argv[2], 0);

        panId = (*(argv[3] + 1) == 'x')
                ? utility_strtox(argv[3] + 2, 0)
                : utility_strtol(argv[3], 0);

        maxChild = (*(argv[4] + 1) == 'x')
                   ? utility_strtox(argv[4] + 2, 0)
                   : utility_strtol(argv[4], 0);


        if (channel > 26 || channel < 11)
        {
            log_out("Invalid channel %d \n", channel);
            break;
        }

        if (panId >= 0xFFFF || panId == 0)
        {
            log_out("Invalid panId %d \n", panId);
            break;
        }

        if (maxChild > 200 || maxChild == 0)
        {
            log_out("Invalid maxChild %d \n", maxChild);
            break;
        }

        if (reset)
        {
            memset(&gt_app_db, 0, sizeof(gt_app_db));
            app_db_update();
        }

        /* primary channel is invalid in ZC */
        if (zigbee_nwk_start_request(ZIGBEE_DEVICE_ROLE_CORDINATOR, ZIGBEE_CHANNEL_MASK(channel), 0x0, maxChild, panId, extPANID, reset) != 0)
        {
            log_out("Start PAN failed\n");
        }
        else
        {
            start_flag = 1;
        }
    } while (0);


    return 0;
}

static int
_cli_cmd_set_custom_target(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{

    do
    {
        if (argc < 3)
        {
            log_out("Invalid parameter\n");
            break;
        }

        custom_dst_short_addr = (*(argv[1] + 1) == 'x')
                                ? utility_strtox(argv[1] + 2, 0)
                                : utility_strtol(argv[1], 0);

        custom_dst_endpoint = (*(argv[2] + 1) == 'x')
                              ? utility_strtox(argv[2] + 2, 0)
                              : utility_strtol(argv[2], 0);

    } while (0);
    return 0;
}

static int
_cli_cmd_zcl_send_custom(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_data_req_t *pt_data_req;
    uint8_t len = 0, i, j = 0, k = 0;

    do
    {
        if (argc < 2)
        {
            break;
        }

        for (i = 0; i < argc - 1; i++)
        {
            len += strlen(argv[i + 1]);
        }

        len += (argc - 1 - 1);

        ZIGBEE_ZCL_DATA_REQ(pt_data_req, custom_dst_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, custom_dst_endpoint, ZIGBEE_DEFAULT_ENDPOINT,
                            ZB_ZCL_CLUSTER_ID_CUSTOM,
                            UART_TRANSPARENT_CMD,
                            TRUE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, len)

        if (pt_data_req)
        {
            for (i = 0; i < argc - 1; i++)
            {
                for (j = 0; j < strlen(argv[i + 1]); j++)
                {
                    pt_data_req->cmdFormat[k] = *(argv[i + 1] + j);
                    k++;
                }
                pt_data_req->cmdFormat[k] = 0x20;
                k++;
            }
            if (k - 1 < 80)
            {
                zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
            }
            else
            {
                log_out("The UART Transparent data should less than 80 bytes, abort!\n");
            }
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}

static int
_cli_cmd_ic_add(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_ic_add_t pt_ic_add;
    uint8_t  i;
    uint32_t addr_tmp;

    do
    {
        if (argc < 3)
        {
            break;
        }

        pt_ic_add.ic_type = 0x03;

        addr_tmp = utility_strtox((argv[1]), 0);
        for (i = 0; i < 4; i++)
        {
            pt_ic_add.ieeeAddr[i + 4] = (addr_tmp >> i * 8) & 0xff;
        }

        addr_tmp = utility_strtox((argv[2]), 0);
        for (i = 0; i < 4; i++)
        {
            pt_ic_add.ieeeAddr[i] = (addr_tmp >> i * 8) & 0xff;
        }

        for (i = 3; i < 21; i++)
        {
            pt_ic_add.ic[i - 3] = utility_strtox(argv[i], 0);
        }
        zigbee_ic_add(pt_ic_add.ieeeAddr, pt_ic_add.ic_type, pt_ic_add.ic);

    } while (0);
    return 0;
}

static int
_cli_cmd_zcl_door_lock(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_data_req_t *pt_data_req;
    uint16_t addr;
    uint8_t ep, cmd;
    uint8_t pincode_len = 8;
    uint8_t pin_code[] = {8, '1', '2', '3', '4', '5', '6', '7', '8'};
    do
    {
        if (argc < 5)
        {
            break;
        }

        if (memcmp("lock", argv[1], 4) == 0)
        {
            cmd = ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR;
        }
        else if (memcmp("unlock", argv[1], 6) == 0)
        {
            cmd = ZB_ZCL_CMD_DOOR_LOCK_UNLOCK_DOOR;
        }
        addr = (*(argv[2] + 1) == 'x')
               ? utility_strtox(argv[2] + 2, 0)
               : utility_strtol(argv[2], 0);

        ep = (*(argv[3] + 1) == 'x')
             ? utility_strtox(argv[3] + 2, 0)
             : utility_strtol(argv[3], 0);

        pincode_len = strlen(argv[4]);
        memset(pin_code + 1, 0, pincode_len);
        pin_code[0] = pincode_len;
        memcpy(pin_code + 1, argv[4], pincode_len);


        ZIGBEE_ZCL_DATA_REQ(pt_data_req, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ep, ZIGBEE_DEFAULT_ENDPOINT,
                            ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                            cmd,
                            TRUE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, pincode_len + 1)

        if (pt_data_req)
        {
            memcpy(pt_data_req->cmdFormat, pin_code, pincode_len + 1);
            zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}
static int

_cli_cmd_zcl_door_lock_set_pin_code(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    zigbee_zcl_data_req_t *pt_data_req;
    uint16_t addr;
    uint8_t ep, pincode_len;
    char *pincode;

    do
    {
        if (argc < 4)
        {
            break;
        }


        pincode_len = strlen(argv[1]);
        pincode = argv[1];

        addr = (*(argv[2] + 1) == 'x')
               ? utility_strtox(argv[2] + 2, 0)
               : utility_strtol(argv[2], 0);

        ep = (*(argv[3] + 1) == 'x')
             ? utility_strtox(argv[3] + 2, 0)
             : utility_strtol(argv[3], 0);



        ZIGBEE_ZCL_DATA_REQ(pt_data_req, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ep, ZIGBEE_DEFAULT_ENDPOINT,
                            ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                            ZB_ZCL_CMD_DOOR_LOCK_SET_PIN_CODE,
                            TRUE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, 0, 5 + pincode_len)

        if (pt_data_req)
        {
            pt_data_req->cmdFormat[0] = 0x00;
            pt_data_req->cmdFormat[1] = 0x00;
            pt_data_req->cmdFormat[2] = 0x01;
            pt_data_req->cmdFormat[3] = 0x00;
            pt_data_req->cmdFormat[4] = pincode_len;
            memcpy(&pt_data_req->cmdFormat[5], pincode, pincode_len);
            zigbee_zcl_request(pt_data_req, pt_data_req->cmdFormatLen);
            sys_free(pt_data_req);
        }

    } while (0);
    return 0;
}
//=============================================================================
//                  Public Function Definition
//=============================================================================
const sh_cmd_t  g_cli_cmd_s2e __cli_cmd_pool =
{
    .pCmd_name      = "s2e",
    .cmd_exec       = _cli_cmd_s2e,
    .pDescription   = "Show device table\n"
    "  usage: s2e\n"
    "    e.g. s2e",
};

const sh_cmd_t  g_cli_cmd_xmode __cli_cmd_pool =
{
    .pCmd_name      = "xmodem",
    .cmd_exec       = _cli_cmd_xmodem,
    .pDescription   = "Enter xmodem mode\n"
    "  usage: xmodem\n"
    "    e.g. xmodem r",
};

const sh_cmd_t  g_cli_cmd_devn __cli_cmd_pool =
{
    .pCmd_name      = "devn",
    .cmd_exec       = _cli_cmd_shdev_num,
    .pDescription   = "Show device joined number\n"
    "  usage: devn\n"
    "    e.g. devn",
};

const sh_cmd_t  g_cli_cmd_ota __cli_cmd_pool =
{
    .pCmd_name      = "ota",
    .cmd_exec       = _cli_cmd_ota,
    .pDescription   = "ota\n"
    "  usage: ota\n"
    "    e.g. ota r",
};

const sh_cmd_t  g_cli_cmd_act_ep_req __cli_cmd_pool =
{
    .pCmd_name      = "ep",
    .cmd_exec       = _cli_cmd_zdo_act_ep_req,
    .pDescription   = "ep\n"
    "  usage: ep [addr]\n"
    "    e.g. ep 0x0001",
};

const sh_cmd_t  g_cli_cmd_simple_desc_req __cli_cmd_pool =
{
    .pCmd_name      = "simple",
    .cmd_exec       = _cli_cmd_zdo_simple_desc_req,
    .pDescription   = "simple\n"
    "  usage: simple [addr] [ep]\n"
    "    e.g. simple 0x0001 2",
};

const sh_cmd_t  g_cli_cmd_on_off_req __cli_cmd_pool =
{
    .pCmd_name      = "onoff",
    .cmd_exec       = _cli_cmd_zcl_on_off,
    .pDescription   = "onfoff\n"
    "  usage: onfoff [addr] [ep] [cmd]\n"
    "    e.g. onfoff 0x0001 1 2",
};

const sh_cmd_t  g_cli_cmd_comm_bind_req __cli_cmd_pool =
{
    .pCmd_name      = "bind",
    .cmd_exec       = _cli_cmd_zdo_bind_req,
    .pDescription   = "bind\n"
    "  usage: bind [dstAddrMode] [src_addr] [src_endpoint] [dst_addr/group_addr] [dst_endpoint] [cluster_id]\n"
    "    e.g. bind 0x03 0x5eab 0x02 0x0000 0x02 0x0006",
};

const sh_cmd_t  g_cli_cmd_comm_unbind_req __cli_cmd_pool =
{
    .pCmd_name      = "unbind",
    .cmd_exec       = _cli_cmd_zdo_unbind_req,
    .pDescription   = "unbind\n"
    "  usage: unbind [dstAddrMode] [src_addr] [src_endpoint] [dst_addr/group_addr] [dst_endpoint] [cluster_id]\n"
    "    e.g. unbind 0x03 0x5eab 0x02 0x0000 0x02 0x0006",
};


const sh_cmd_t  g_cli_cmd_attr_read __cli_cmd_pool =
{
    .pCmd_name      = "ra",
    .cmd_exec       = _cli_cmd_zcl_attr_read,
    .pDescription   = "read attribute\n"
    "  usage: ra [DstShort] [DstEP] [ClusterID] [AttrID]\n"
    "    e.g. ra 0xbc2e 0x02 0x0003 0x0000\n",
};

const sh_cmd_t  g_cli_cmd_attr_write __cli_cmd_pool =
{
    .pCmd_name      = "wa",
    .cmd_exec       = _cli_cmd_zcl_attr_write,
    .pDescription   = "write attribute\n"
    "  usage: wa [DstShort] [DstEP] [ClusterID] [AttrID] [AttrType] [Value]\n"
    "    e.g. wa 0xbc2e 0x02 0x0003 0x0000 0x21 10 \n",
};

const sh_cmd_t  g_cli_cmd_zcl_group __cli_cmd_pool =
{
    .pCmd_name      = "group",
    .cmd_exec       = _cli_cmd_zcl_group,
    .pDescription   = "group\n"
    "  usage: group [action] [addr] [ep] [group id]\n"
    "    e.g. Add :  group a 0x123 1 0x0001\n",
    "         Remove:group r 0x123 1 0x0001",
};
const sh_cmd_t  g_cli_cmd_identify __cli_cmd_pool =
{
    .pCmd_name      = "id",
    .cmd_exec       = _cli_cmd_zcl_identify,
    .pDescription   = "id\n"
    "  usage: id [addr] [ep]\n"
    "    e.g. id 0x0001 2\n",
};
const sh_cmd_t  g_cli_cmd_setpt __cli_cmd_pool =
{
    .pCmd_name      = "setpt",
    .cmd_exec       = _cli_cmd_setpt,
    .pDescription   = "adjust heat/cool setpoint by amount\n"
    "  usage: setpt [short address] [ep] [mode] [amount]\n"
    "    e.g. setpt 0x1234 2 0 30\n",
    "    e.g. setpt 0x1234 2 0 -20\n",
};
const sh_cmd_t  g_cli_cmd_level_ctrl_req __cli_cmd_pool =
{
    .pCmd_name      = "level",
    .cmd_exec       = _cli_cmd_zcl_level_ctrl,
    .pDescription   = "level\n"
    "  usage: level [dir] [addr] [ep]\n"
    "    e.g. level up 0x0001 2\n"
    "         level dn 0x0001 2\n",
};
const sh_cmd_t  g_cli_cmd_zcl_scene __cli_cmd_pool =
{
    .pCmd_name      = "scene",
    .cmd_exec       = _cli_cmd_zcl_scene,
    .pDescription   = "scene\n"
    "  usage: scene [action] [addr] [ep] [group id] [scence id]\n"
    "    e.g. Store : scene s 0x123 2 0x0001 1 \n"
    "         Remove: scene re 0x123 2 0x0001 1\n"
    "         Recall: scene rc 0x123 2 0x0001 1\n"
    "         View  : scene v 0x123 2 0x0001 1\n",
};
const sh_cmd_t  g_cli_cmd_zcl_config_report __cli_cmd_pool =
{
    .pCmd_name      = "cr",
    .cmd_exec       = _cli_cmd_zcl_config_report,
    .pDescription   = "configure report\n"
    "  usage: cr [addr] [ep] [cluster id] [attribute id] [data type] [min_interval] [max interval]\n"
    "    e.g. cr 0x1234 2 0x0006 0x0000 0x10 0x1E 0x3D\n",
};
const sh_cmd_t  g_cli_cmd_permit_join __cli_cmd_pool =
{
    .pCmd_name      = "pj",
    .cmd_exec       = _cli_cmd_permit_join,
    .pDescription   = "pj\n"
    "  usage: pj [enable] [timeout]\n"
    "    e.g. pj 1 60\n"
    "         pj 0",
};

const sh_cmd_t  g_cli_cmd_ed_scan __cli_cmd_pool =
{
    .pCmd_name      = "edscan",
    .cmd_exec       = _cli_cmd_ed_scan,
    .pDescription   = "edscan\n"
    "  usage: edscan \n"
    "    e.g. edscan\n",
};

const sh_cmd_t  g_cli_cmd_start_pan __cli_cmd_pool =
{
    .pCmd_name      = "start",
    .cmd_exec       = _cli_cmd_start_pan,
    .pDescription   = "start\n"
    "  usage: start [reset] [Channel] [PAN ID] [Max child]\n"
    "    e.g. start 1 11 0x123 30\n",
};

const sh_cmd_t  g_cli_cmd_set_custom_target __cli_cmd_pool =
{
    .pCmd_name      = "ct",
    .cmd_exec       = _cli_cmd_set_custom_target,
    .pDescription   = "set target to send custom clster packet to\n"
    "  usage: ct [short address] [end point]\n"
    "    e.g. ct 0x1234 02\n",
};

const sh_cmd_t  g_cli_cmd_zcl_send_custom __cli_cmd_pool =
{
    .pCmd_name      = "cs",
    .cmd_exec       = _cli_cmd_zcl_send_custom,
    .pDescription   = "send custom clster packet\n"
    "  usage: cs [value]\n"
    "    e.g. cs 0xaa\n",
};
const sh_cmd_t  g_cli_cmd_door_lock __cli_cmd_pool =
{
    .pCmd_name      = "doorlock",
    .cmd_exec       = _cli_cmd_zcl_door_lock,
    .pDescription   = "send doorlock command\n"
    "  usage: doorlock [lock/unlock] [short address] [end point] pincode\n"
    "    e.g. doorlock lock 0x1234 2 12345678\n",
};
const sh_cmd_t  g_cli_cmd_set_pin_code __cli_cmd_pool =
{
    .pCmd_name      = "pincode",
    .cmd_exec       = _cli_cmd_zcl_door_lock_set_pin_code,
    .pDescription   = "set pin code command\n"
    "  usage: pincode [pincode] [short address] [end point]\n"
    "    e.g. pincode 12345678 0x1234 2\n",
};


const sh_cmd_t  g_cli_cmd_ic_add __cli_cmd_pool =
{
    .pCmd_name      = "ic",
    .cmd_exec       = _cli_cmd_ic_add,
    .pDescription   = "add install code\n"
    "  usage: ic [IEEE addr 7 ~ 4 byte] [IEEE addr 3 ~ 0 byte] [install code with CRC]\n"
    "    e.g. ic 352E4532 33323850 85 FE D3 40 7A 93 97 23 a5 c6 39 b2 69 16 d5 05 90 89 \n",
};
