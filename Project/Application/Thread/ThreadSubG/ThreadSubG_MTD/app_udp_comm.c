/**
 * @file app_udp_comm.c
 * @author
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
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "main.h"

//=============================================================================
//                Define
//=============================================================================
#define UDP_COMM_HEADER 0x88880000
//=============================================================================
//                Private ENUM
//=============================================================================

//=============================================================================
//                Private Struct
//=============================================================================

//=============================================================================
//                Private Function Declaration
//=============================================================================

//=============================================================================
//                Functions
//=============================================================================

bool app_udp_comm_header_check(uint8_t *head, uint8_t lens)
{
    bool ret = false;
    do
    {
        if (lens < 4)
        {
            break;
        }
        uint32_t header = *(uint32_t *)(head);
        if ((header & 0xffff0000) != UDP_COMM_HEADER)
        {
            break;
        }
        ret = true;
    } while (0);

    return ret;
}

/*app udp comm process*/
void app_udp_comm_process(uint8_t *data, uint16_t lens, otIp6Address src_addr)
{
    uint32_t header = *(uint32_t *)(data);

    switch (header)
    {
    case PATH_REQUEST_HEADER:
        app_udp_comm_path_req_proc(data, lens);
        break;
    case PATH_RESPOND_HEADER:
        app_udp_comm_path_resp_proc(data, lens);
        break;
#if CFG_USE_CENTRAK_CONFIG
    case NET_MGM_NODE_ACCEPTED_HEADER:
        app_udp_comm_net_mgm_node_accepted_proc(data, lens, src_addr);
        break;
    case NET_MGM_NODE_RESET_HEADER:
        app_udp_comm_net_mgm_node_reset_proc(data, lens, src_addr);
        break;
#endif
    default:
        break;
    }
}

void app_udp_comm_init(otInstance *aInstance)
{
    path_init();
#if CFG_USE_CENTRAK_CONFIG
    net_mgm_init();
#endif
}