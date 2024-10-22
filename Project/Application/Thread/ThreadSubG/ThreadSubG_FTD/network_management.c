/**
 * @file nwtwork_management.c
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
#include <openthread/coap.h>
#include "sw_timer.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "main.h"

//=============================================================================
//                Private Function Declaration
//=============================================================================
#define RAFAEL_NWK_MGM_URL "nwk"

#define NWK_MGM_CHILD_REQ_TATLE_SIZE (256)

#define NWK_MGM_CHILD_REQ_ENTRIES_TIME OPENTHREAD_CONFIG_MLE_CHILD_TIMEOUT_DEFAULT
#define NWK_MGM_ROUTER_KEEP_ALIVE_TIME ((OPENTHREAD_CONFIG_MLE_CHILD_TIMEOUT_DEFAULT/3)+1)

#define NWK_MGM_CHILD_INFO_SIZE (OPENTHREAD_CONFIG_MLE_MAX_CHILDREN + 1)

unsigned int nwk_mgm_debug_flags = 0;
#define nwk_mgm_printf(args...)      \
    do                               \
    {                                \
        if (nwk_mgm_debug_flags > 0) \
            info(args);              \
    } while (0);
//=============================================================================
//                Private ENUM
//=============================================================================
enum
{
    NWK_MGM_TYPE_CHILD_REG,
    NWK_MGM_TYPE_CHILD_REG_ACK
};
//=============================================================================
//                Private Struct
//=============================================================================

typedef struct
{
    uint16_t rloc;
    uint8_t extaddr[OT_EXT_ADDRESS_SIZE];
    int8_t rssi;
} nwk_mgm_child_info_t;

typedef struct
{
    uint8_t data_type;
    uint16_t parent;
    uint16_t self_rloc;
    uint8_t self_extaddr[OT_EXT_ADDRESS_SIZE];
    uint16_t num;
    nwk_mgm_child_info_t child_info[NWK_MGM_CHILD_INFO_SIZE];
} nwk_mgm_child_reg_data_t;

typedef struct
{
    uint8_t data_type;
    uint8_t state;
} nwk_mgm_child_reg_ack_info_t;

typedef struct
{
    uint8_t used;
    uint8_t role;
    uint16_t parent;
    uint16_t rloc;
    uint8_t extaddr[OT_EXT_ADDRESS_SIZE];
    int8_t rssi;
    uint16_t validtime;
} nwk_mgm_child_reg_table_t;

otCoapResource nwk_mgm_resource;

static nwk_mgm_child_reg_table_t *nwk_mgm_child_reg_table = NULL;

static sw_timer_t *nwk_mgm_timer = NULL;

static uint16_t mgm_reg_send_timer = 0;
static uint16_t mgm_router_up_timer = 0;
static bool reg_table_is_full = false;

//=============================================================================
//                Private Function Declaration
//=============================================================================
static void nwk_mgm_kick_child_post(uint16_t rloc, uint16_t panid, uint8_t *extaddr);

//=============================================================================
//                Functions
//=============================================================================

void nwk_mgm_debug_level(unsigned int level)
{
    nwk_mgm_debug_flags = level;
}

void nwk_mgm_reg_send_timer_set(uint16_t time)
{
    mgm_reg_send_timer = time;
}

void nwk_mgm_router_update_timer_set(uint16_t time)
{
    mgm_router_up_timer = time;
}

static int nwk_mgm_child_reg_table_add(uint8_t role, uint16_t parent, uint16_t rloc, uint8_t *extaddr, int8_t rssi)
{
    uint16_t i = 0;
    if (nwk_mgm_child_reg_table)
    {
        enter_critical_section();
        for (i = 0; i < NWK_MGM_CHILD_REQ_TATLE_SIZE; i++)
        {
            if (nwk_mgm_child_reg_table[i].used &&
                    memcmp(nwk_mgm_child_reg_table[i].extaddr, extaddr, OT_EXT_ADDRESS_SIZE) == 0)
            {
                nwk_mgm_printf("updata %02X%02X%02X%02X%02X%02X%02X%02X \n",
                               nwk_mgm_child_reg_table[i].extaddr[0],
                               nwk_mgm_child_reg_table[i].extaddr[1],
                               nwk_mgm_child_reg_table[i].extaddr[2],
                               nwk_mgm_child_reg_table[i].extaddr[3],
                               nwk_mgm_child_reg_table[i].extaddr[4],
                               nwk_mgm_child_reg_table[i].extaddr[5],
                               nwk_mgm_child_reg_table[i].extaddr[6],
                               nwk_mgm_child_reg_table[i].extaddr[7]);
                nwk_mgm_child_reg_table[i].parent = parent;
                nwk_mgm_child_reg_table[i].role = role;
                nwk_mgm_child_reg_table[i].rloc = rloc;
                nwk_mgm_child_reg_table[i].rssi = rssi;
                nwk_mgm_child_reg_table[i].validtime = NWK_MGM_CHILD_REQ_ENTRIES_TIME;
                break;
            }
        }
        if (i >= NWK_MGM_CHILD_REQ_TATLE_SIZE)
        {
            for (i = 0; i < NWK_MGM_CHILD_REQ_TATLE_SIZE; i++)
            {
                if (0 == nwk_mgm_child_reg_table[i].used)
                {
                    nwk_mgm_child_reg_table[i].parent = parent;
                    nwk_mgm_child_reg_table[i].role = role;
                    nwk_mgm_child_reg_table[i].rloc = rloc;
                    mem_memcpy(nwk_mgm_child_reg_table[i].extaddr, extaddr, OT_EXT_ADDRESS_SIZE);
                    nwk_mgm_child_reg_table[i].rssi = rssi;
                    nwk_mgm_child_reg_table[i].used = 1;
                    nwk_mgm_child_reg_table[i].validtime = NWK_MGM_CHILD_REQ_ENTRIES_TIME;
                    nwk_mgm_printf("add %s %04X %02X%02X%02X%02X%02X%02X%02X%02X %d \n",
                                   otThreadDeviceRoleToString(nwk_mgm_child_reg_table[i].role),
                                   nwk_mgm_child_reg_table[i].rloc,
                                   nwk_mgm_child_reg_table[i].extaddr[0],
                                   nwk_mgm_child_reg_table[i].extaddr[1],
                                   nwk_mgm_child_reg_table[i].extaddr[2],
                                   nwk_mgm_child_reg_table[i].extaddr[3],
                                   nwk_mgm_child_reg_table[i].extaddr[4],
                                   nwk_mgm_child_reg_table[i].extaddr[5],
                                   nwk_mgm_child_reg_table[i].extaddr[6],
                                   nwk_mgm_child_reg_table[i].extaddr[7],
                                   nwk_mgm_child_reg_table[i].rssi);
                    break;
                }
            }
        }
        leave_critical_section();
    }
    return (i >= NWK_MGM_CHILD_REQ_TATLE_SIZE);
}

static void nwk_mgm_child_reg_table_remove(uint8_t role, uint8_t *extaddr)
{
    if (nwk_mgm_child_reg_table)
    {
        enter_critical_section();
        for (uint16_t i = 0; i < NWK_MGM_CHILD_REQ_TATLE_SIZE; i++)
        {
            if (nwk_mgm_child_reg_table[i].used &&
                    memcmp(nwk_mgm_child_reg_table[i].extaddr, extaddr, OT_EXT_ADDRESS_SIZE) == 0)
            {
                if (role == nwk_mgm_child_reg_table[i].role)
                {
                    nwk_mgm_printf("remove %s %04X %02X%02X%02X%02X%02X%02X%02X%02X %d \n",
                                   otThreadDeviceRoleToString(nwk_mgm_child_reg_table[i].role),
                                   nwk_mgm_child_reg_table[i].rloc,
                                   nwk_mgm_child_reg_table[i].extaddr[0],
                                   nwk_mgm_child_reg_table[i].extaddr[1],
                                   nwk_mgm_child_reg_table[i].extaddr[2],
                                   nwk_mgm_child_reg_table[i].extaddr[3],
                                   nwk_mgm_child_reg_table[i].extaddr[4],
                                   nwk_mgm_child_reg_table[i].extaddr[5],
                                   nwk_mgm_child_reg_table[i].extaddr[6],
                                   nwk_mgm_child_reg_table[i].extaddr[7],
                                   nwk_mgm_child_reg_table[i].rssi);
                    nwk_mgm_child_reg_table[i].role = OT_DEVICE_ROLE_DETACHED;
                }
            }
        }
        leave_critical_section();
    }
}

static void nwk_mgm_child_reg_table_time_handler()
{
    if (nwk_mgm_child_reg_table)
    {
        enter_critical_section();
        for (uint16_t i = 0; i < NWK_MGM_CHILD_REQ_TATLE_SIZE; i++)
        {
            if (nwk_mgm_child_reg_table[i].used)
            {
                if (nwk_mgm_child_reg_table[i].validtime > 0 && --nwk_mgm_child_reg_table[i].validtime == 0)
                {
                    nwk_mgm_printf("timeout %s %04X %02X%02X%02X%02X%02X%02X%02X%02X \n",
                                   otThreadDeviceRoleToString(nwk_mgm_child_reg_table[i].role),
                                   nwk_mgm_child_reg_table[i].rloc,
                                   nwk_mgm_child_reg_table[i].extaddr[0],
                                   nwk_mgm_child_reg_table[i].extaddr[1],
                                   nwk_mgm_child_reg_table[i].extaddr[2],
                                   nwk_mgm_child_reg_table[i].extaddr[3],
                                   nwk_mgm_child_reg_table[i].extaddr[4],
                                   nwk_mgm_child_reg_table[i].extaddr[5],
                                   nwk_mgm_child_reg_table[i].extaddr[6],
                                   nwk_mgm_child_reg_table[i].extaddr[7]);
                    memset(&nwk_mgm_child_reg_table[i], 0x0, sizeof(nwk_mgm_child_reg_table_t));
                }
            }
        }
        leave_critical_section();
    }
}

bool otPlatFindNwkMgmChildRegTable(uint8_t *aExtAddress)
{
    bool ret = false;
    uint16_t i = 0;
    if (nwk_mgm_child_reg_table)
    {
        for (i = 0; i < NWK_MGM_CHILD_REQ_TATLE_SIZE; i++)
        {
            if (nwk_mgm_child_reg_table[i].used &&
                    memcmp(nwk_mgm_child_reg_table[i].extaddr, aExtAddress, OT_EXT_ADDRESS_SIZE) == 0)
            {
                nwk_mgm_printf("fined %02X%02X%02X%02X%02X%02X%02X%02X \n", aExtAddress[0], aExtAddress[1], aExtAddress[2],
                               aExtAddress[3], aExtAddress[4], aExtAddress[5], aExtAddress[6], aExtAddress[7]);
                ret = true;
                break;
            }
        }
        if (ret == false)
        {
            nwk_mgm_printf("not find %02X%02X%02X%02X%02X%02X%02X%02X \n", aExtAddress[0], aExtAddress[1], aExtAddress[2],
                           aExtAddress[3], aExtAddress[4], aExtAddress[5], aExtAddress[6], aExtAddress[7]);
        }
    }
    return ret;
}

bool otPlatNwkMgmIsFull()
{
    return reg_table_is_full;
}

void nwk_mgm_child_reg_table_display()
{
    uint16_t i = 0, count = 0;
    if (nwk_mgm_child_reg_table)
    {
        info("index role parent rloc extaddr rssi \r\n");
        info("===============================================\r\n");
        for (i = 0; i < NWK_MGM_CHILD_REQ_TATLE_SIZE; i++)
        {
            if (nwk_mgm_child_reg_table[i].used && (nwk_mgm_child_reg_table[i].role == OT_DEVICE_ROLE_ROUTER))
            {
                ++count;
                info("[%u] %s %04X %04X %02X%02X%02X%02X%02X%02X%02X%02X %d %u\n",
                     count,
                     otThreadDeviceRoleToString(nwk_mgm_child_reg_table[i].role),
                     nwk_mgm_child_reg_table[i].parent,
                     nwk_mgm_child_reg_table[i].rloc,
                     nwk_mgm_child_reg_table[i].extaddr[0],
                     nwk_mgm_child_reg_table[i].extaddr[1],
                     nwk_mgm_child_reg_table[i].extaddr[2],
                     nwk_mgm_child_reg_table[i].extaddr[3],
                     nwk_mgm_child_reg_table[i].extaddr[4],
                     nwk_mgm_child_reg_table[i].extaddr[5],
                     nwk_mgm_child_reg_table[i].extaddr[6],
                     nwk_mgm_child_reg_table[i].extaddr[7],
                     nwk_mgm_child_reg_table[i].rssi);
            }
        }
        for (i = 0; i < NWK_MGM_CHILD_REQ_TATLE_SIZE; i++)
        {
            if (nwk_mgm_child_reg_table[i].used)
            {
                if (nwk_mgm_child_reg_table[i].role == OT_DEVICE_ROLE_CHILD)
                {
                    ++count;
                    info("[%u] %s %04X %04X %02X%02X%02X%02X%02X%02X%02X%02X %d\n",
                         count,
                         otThreadDeviceRoleToString(nwk_mgm_child_reg_table[i].role),
                         nwk_mgm_child_reg_table[i].parent,
                         nwk_mgm_child_reg_table[i].rloc,
                         nwk_mgm_child_reg_table[i].extaddr[0],
                         nwk_mgm_child_reg_table[i].extaddr[1],
                         nwk_mgm_child_reg_table[i].extaddr[2],
                         nwk_mgm_child_reg_table[i].extaddr[3],
                         nwk_mgm_child_reg_table[i].extaddr[4],
                         nwk_mgm_child_reg_table[i].extaddr[5],
                         nwk_mgm_child_reg_table[i].extaddr[6],
                         nwk_mgm_child_reg_table[i].extaddr[7],
                         nwk_mgm_child_reg_table[i].rssi);
                }
            }
        }
        for (i = 0; i < NWK_MGM_CHILD_REQ_TATLE_SIZE; i++)
        {
            if (nwk_mgm_child_reg_table[i].used)
            {

                if ((nwk_mgm_child_reg_table[i].role != OT_DEVICE_ROLE_CHILD) &&
                        (nwk_mgm_child_reg_table[i].role != OT_DEVICE_ROLE_ROUTER))
                {
                    ++count;
                    info("[%u] %s %04X %04X %02X%02X%02X%02X%02X%02X%02X%02X %d\n",
                         count,
                         otThreadDeviceRoleToString(nwk_mgm_child_reg_table[i].role),
                         nwk_mgm_child_reg_table[i].parent,
                         nwk_mgm_child_reg_table[i].rloc,
                         nwk_mgm_child_reg_table[i].extaddr[0],
                         nwk_mgm_child_reg_table[i].extaddr[1],
                         nwk_mgm_child_reg_table[i].extaddr[2],
                         nwk_mgm_child_reg_table[i].extaddr[3],
                         nwk_mgm_child_reg_table[i].extaddr[4],
                         nwk_mgm_child_reg_table[i].extaddr[5],
                         nwk_mgm_child_reg_table[i].extaddr[6],
                         nwk_mgm_child_reg_table[i].extaddr[7],
                         nwk_mgm_child_reg_table[i].rssi);
                }
            }
        }
        info("=============================================== \n");
        info("total num %u \n", count);
    }
}

static int nwk_mgm_data_parse(uint8_t type, uint8_t *payload, uint16_t payloadlength, void *data)
{
    nwk_mgm_child_reg_data_t *child_reg_data = NULL;
    nwk_mgm_child_reg_ack_info_t *child_reg_data_ack = NULL;
    uint8_t *tmp = payload;

    if (type == NWK_MGM_TYPE_CHILD_REG)
    {
        child_reg_data = (nwk_mgm_child_reg_data_t *)data;
        child_reg_data->data_type = *tmp++;
        mem_memcpy(&child_reg_data->parent, tmp, 2);
        tmp += 2;
        mem_memcpy(&child_reg_data->self_rloc, tmp, 2);
        tmp += 2;
        mem_memcpy(&child_reg_data->self_extaddr, tmp, OT_EXT_ADDRESS_SIZE);
        tmp += OT_EXT_ADDRESS_SIZE;
        mem_memcpy(&child_reg_data->num, tmp, 2);
        tmp += 2;
        if (child_reg_data->num > 0)
        {
            mem_memcpy(&child_reg_data->child_info, tmp, (child_reg_data->num * sizeof(nwk_mgm_child_info_t)));
            tmp += (child_reg_data->num * sizeof(nwk_mgm_child_info_t));
        }
    }
    else if (type == NWK_MGM_TYPE_CHILD_REG_ACK)
    {
        child_reg_data_ack = (nwk_mgm_child_reg_ack_info_t *)data;
        child_reg_data_ack->data_type = *tmp++;
        child_reg_data_ack->state = *tmp++;
    }
    else
    {
        nwk_mgm_printf("unknow parse type %u \n", type);
    }
    if ((tmp - payload) != payloadlength)
    {
        nwk_mgm_printf("parse fail %u (%u/%u)\n", type, (tmp - payload), payloadlength);
        return 1;
    }
    return 0;
}
static void nwk_mgm_data_piece(uint8_t type, uint8_t *payload, uint16_t *payloadlength, uint8_t *data)
{
    nwk_mgm_child_reg_data_t *child_reg_data = NULL;
    nwk_mgm_child_reg_ack_info_t *child_reg_data_ack = NULL;

    uint8_t *tmp = payload;
    *tmp++ = data[0];

    if (type == NWK_MGM_TYPE_CHILD_REG)
    {
        child_reg_data = (nwk_mgm_child_reg_data_t *)data;
        mem_memcpy(tmp, &child_reg_data->parent, 2);
        tmp += 2;
        mem_memcpy(tmp, &child_reg_data->self_rloc, 2);
        tmp += 2;
        mem_memcpy(tmp, &child_reg_data->self_extaddr, OT_EXT_ADDRESS_SIZE);
        tmp += OT_EXT_ADDRESS_SIZE;
        mem_memcpy(tmp, &child_reg_data->num, 2);
        tmp += 2;
        if (child_reg_data->num > 0)
        {
            mem_memcpy(tmp, &child_reg_data->child_info, (child_reg_data->num * sizeof(nwk_mgm_child_info_t)));
            tmp += (child_reg_data->num * sizeof(nwk_mgm_child_info_t));
        }
    }
    else if (type == NWK_MGM_TYPE_CHILD_REG_ACK)
    {
        child_reg_data_ack = (nwk_mgm_child_reg_ack_info_t *)data;

        *tmp++ = child_reg_data_ack->state;
    }
    else
    {
        nwk_mgm_printf("unknow piece type %u \n", type);
    }
    *payloadlength = (tmp - payload);
}

static void nwk_mgm_childs_register_proccess(otMessage *aMessage, const otMessageInfo *aMessageInfo, uint8_t *buf, uint16_t length)
{
    otDeviceRole mRole;
    otError    error           = OT_ERROR_NONE;
    otMessage *responseMessage = NULL;
    otCoapCode responseCode = OT_COAP_CODE_EMPTY;
    otNeighborInfo  neighborInfo;
    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    nwk_mgm_child_reg_data_t child_reg_data;
    nwk_mgm_child_reg_ack_info_t child_reg_data_ack;
    uint16_t *ack_rloc = NULL;
    uint8_t *payload = NULL;
    uint16_t i = 0, k = 0, payloadlength;
    mRole = otThreadGetDeviceRole(otGetInstance());
    do
    {
        if (length > 0)
        {
            if (NULL != buf)
            {
                if (otCoapMessageGetType(aMessage) == OT_COAP_TYPE_CONFIRMABLE)
                {
                    /*do ack packet*/
                    responseCode = OT_COAP_CODE_CONTENT;
                    responseMessage = otCoapNewMessage(otGetInstance(), NULL);
                    if (responseMessage == NULL)
                    {
                        error = OT_ERROR_NO_BUFS;
                        break;
                    }
                    error = otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, responseCode);
                    if (error != OT_ERROR_NONE)
                    {
                        break;
                    }
                    memset(&child_reg_data_ack, 0x0, sizeof(nwk_mgm_child_reg_ack_info_t));
                    child_reg_data_ack.data_type = NWK_MGM_TYPE_CHILD_REG_ACK;
                    child_reg_data_ack.state = 0; // success = 0, fail = 1
                    reg_table_is_full = false;

                    if (mRole == OT_DEVICE_ROLE_LEADER)
                    {
                        if (nwk_mgm_data_parse(NWK_MGM_TYPE_CHILD_REG, buf, length, &child_reg_data))
                        {
                            error = OT_ERROR_FAILED;
                            break;
                        }
                        nwk_mgm_printf("self %04x,%04x, %02X%02X%02X%02X%02X%02X%02X%02X \n",
                                       child_reg_data.parent,
                                       child_reg_data.self_rloc,
                                       child_reg_data.self_extaddr[0],
                                       child_reg_data.self_extaddr[1],
                                       child_reg_data.self_extaddr[2],
                                       child_reg_data.self_extaddr[3],
                                       child_reg_data.self_extaddr[4],
                                       child_reg_data.self_extaddr[5],
                                       child_reg_data.self_extaddr[6],
                                       child_reg_data.self_extaddr[7]);

                        /*add my self*/
                        int8_t self_rssi = (-128);
                        while (otThreadGetNextNeighborInfo(otGetInstance(), &iterator, &neighborInfo) == OT_ERROR_NONE)
                        {
                            if (neighborInfo.mRloc16 == child_reg_data.self_rloc)
                            {
                                self_rssi = neighborInfo.mAverageRssi;
                                break;
                            }
                        }
                        if (nwk_mgm_child_reg_table_add(OT_DEVICE_ROLE_ROUTER,
                                                        child_reg_data.parent,
                                                        child_reg_data.self_rloc,
                                                        child_reg_data.self_extaddr, self_rssi))
                        {
                            nwk_mgm_printf("unexpected router add fail\n");
                            reg_table_is_full = true;
                            child_reg_data_ack.state = 1; // success = 0, fail = 1
                        }
                        /*check child table*/
                        for (i = 0; i < NWK_MGM_CHILD_REQ_TATLE_SIZE; i++)
                        {
                            if (nwk_mgm_child_reg_table[i].used)
                            {
                                if (nwk_mgm_child_reg_table[i].parent == child_reg_data.self_rloc)
                                {
                                    for (k = 0; k < child_reg_data.num; k++)
                                    {
                                        if (memcmp(nwk_mgm_child_reg_table[i].extaddr, child_reg_data.child_info[k].extaddr, OT_EXT_ADDRESS_SIZE) == 0)
                                        {
                                            break;
                                        }
                                    }

                                    if (k >= child_reg_data.num)
                                    {
                                        nwk_mgm_printf("data rm %04x %04x %02X%02X%02X%02X%02X%02X%02X%02X\n",
                                                       child_reg_data.self_rloc,
                                                       nwk_mgm_child_reg_table[i].rloc,
                                                       nwk_mgm_child_reg_table[i].extaddr[0],
                                                       nwk_mgm_child_reg_table[i].extaddr[1],
                                                       nwk_mgm_child_reg_table[i].extaddr[2],
                                                       nwk_mgm_child_reg_table[i].extaddr[3],
                                                       nwk_mgm_child_reg_table[i].extaddr[4],
                                                       nwk_mgm_child_reg_table[i].extaddr[5],
                                                       nwk_mgm_child_reg_table[i].extaddr[6],
                                                       nwk_mgm_child_reg_table[i].extaddr[7]);
                                        nwk_mgm_child_reg_table_remove(OT_DEVICE_ROLE_CHILD, (uint8_t *)&nwk_mgm_child_reg_table[i].extaddr);
                                    }
                                }
                            }
                        }
                        /*updata child table*/
                        nwk_mgm_printf("rece %u \n", child_reg_data.num);
                        for (i = 0; i < child_reg_data.num; i++)
                        {
                            nwk_mgm_printf("child %04x,%04x, %02X%02X%02X%02X%02X%02X%02X%02X, %d \n",
                                           child_reg_data.self_rloc,
                                           child_reg_data.child_info[i].rloc,
                                           child_reg_data.child_info[i].extaddr[0],
                                           child_reg_data.child_info[i].extaddr[1],
                                           child_reg_data.child_info[i].extaddr[2],
                                           child_reg_data.child_info[i].extaddr[3],
                                           child_reg_data.child_info[i].extaddr[4],
                                           child_reg_data.child_info[i].extaddr[5],
                                           child_reg_data.child_info[i].extaddr[6],
                                           child_reg_data.child_info[i].extaddr[7],
                                           child_reg_data.child_info[i].rssi);
                            if (nwk_mgm_child_reg_table_add(OT_DEVICE_ROLE_CHILD,
                                                            child_reg_data.self_rloc,
                                                            child_reg_data.child_info[i].rloc,
                                                            child_reg_data.child_info[i].extaddr,
                                                            child_reg_data.child_info[i].rssi))
                            {
                                reg_table_is_full = true;
                                child_reg_data_ack.state = 1; // success = 0, fail = 1
                            }
                        }
                    }
                    else
                    {
                        nwk_mgm_printf("isn't leader \n");
                    }
                    payload = mem_malloc(sizeof(nwk_mgm_child_reg_ack_info_t));
                    if (payload)
                    {
                        nwk_mgm_data_piece(NWK_MGM_TYPE_CHILD_REG_ACK, payload, &payloadlength, (uint8_t *)&child_reg_data_ack);
                        nwk_mgm_printf("ack marker\n");
                        error = otCoapMessageSetPayloadMarker(responseMessage);
                        if (error != OT_ERROR_NONE)
                        {
                            break;
                        }
                        nwk_mgm_printf("ack append \n");
                        error = otMessageAppend(responseMessage, payload, payloadlength);
                        if (error != OT_ERROR_NONE)
                        {
                            break;
                        }
                    }
                    else
                    {
                        nwk_mgm_printf("reg_ack alloc fail \r\n");
                    }
                    nwk_mgm_printf("ack parneters\n");
                    error = otCoapSendResponseWithParameters(otGetInstance(), responseMessage, aMessageInfo, NULL);
                    if (error != OT_ERROR_NONE)
                    {
                        break;
                    }
                }
            }
        }
    } while (0);
    nwk_mgm_printf("coap send response error %d: %s \r\n", error, otThreadErrorToString(error));
    if (error != OT_ERROR_NONE && responseMessage != NULL)
    {
        otMessageFree(responseMessage);
    }
    if (payload)
    {
        mem_free(payload);
    }
}

static void nwk_mgm_childs_register_ack_proccess(uint8_t *buf, uint16_t length)
{
    otDeviceRole mRole = otThreadGetDeviceRole(otGetInstance());
    nwk_mgm_child_reg_ack_info_t child_reg_data_ack;
    otExtAddress aExtAddress;
    aExtAddress = *otLinkGetExtendedAddress(otGetInstance());
    do
    {
        if (length > 0)
        {
            if (NULL != buf)
            {
                if (mRole == OT_DEVICE_ROLE_ROUTER)
                {
                    if (nwk_mgm_data_parse(NWK_MGM_TYPE_CHILD_REG_ACK, buf, length, &child_reg_data_ack))
                    {
                        break;
                    }
                    nwk_mgm_printf("ack state %u\n", child_reg_data_ack.state);
                }
            }
        }
    } while (0);
}

static void nwk_mgm_request_proccess(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    uint8_t *buf = NULL;
    uint16_t length;
    uint8_t nwk_mgm_data_type = 0xff;
    otIp6AddressToString(&aMessageInfo->mPeerAddr, string, sizeof(string));
    length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);

    do
    {
        if (length > 0)
        {
            buf = mem_malloc(length);
            if (NULL != buf)
            {
                otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, length);
                nwk_mgm_data_type = buf[0];
                switch (nwk_mgm_data_type)
                {
                case NWK_MGM_TYPE_CHILD_REG:
                    nwk_mgm_childs_register_proccess(aMessage, aMessageInfo, buf, length);
                    break;
                default:
                    break;
                }
            }
        }
    } while (0);

    if (buf)
    {
        mem_free(buf);
    }
}

static void nwk_mgm_ack_process(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aResult)
{
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    uint8_t *buf = NULL;
    uint16_t length;
    uint8_t nwk_mgm_data_type = 0xff;
    otIp6AddressToString(&aMessageInfo->mPeerAddr, string, sizeof(string));
    length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    do
    {
        if (length > 0)
        {
            buf = mem_malloc(length);
            if (NULL != buf)
            {
                otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, length);
                nwk_mgm_data_type = buf[0];
                switch (nwk_mgm_data_type)
                {
                case NWK_MGM_TYPE_CHILD_REG_ACK:
                    nwk_mgm_childs_register_ack_proccess(buf, length);
                    break;
                default:
                    break;
                }
            }
        }
    } while (0);

    if (buf)
    {
        mem_free(buf);
    }
}

otError nwk_mgm_coap_request(otCoapCode aCoapCode, otIp6Address coapDestinationIp, otCoapType coapType, uint8_t *payload, uint16_t payloadLength, const char *coap_Path)
{
    otError error = OT_ERROR_NONE;
    otMessage *message = NULL;
    otMessageInfo messageInfo;

    do
    {
        message = otCoapNewMessage(otGetInstance(), NULL);
        if (NULL == message)
        {
            error = OT_ERROR_NO_BUFS;
            break;
        }
        otCoapMessageInit(message, coapType, aCoapCode);
        otCoapMessageGenerateToken(message, OT_COAP_DEFAULT_TOKEN_LENGTH);

        error = otCoapMessageAppendUriPathOptions(message, coap_Path);
        if (OT_ERROR_NONE != error)
        {
            break;
        }

        if (payloadLength > 0)
        {
            error = otCoapMessageSetPayloadMarker(message);
            if (OT_ERROR_NONE != error)
            {
                break;
            }
        }

        // Embed content into message if given
        if (payloadLength > 0)
        {
            error = otMessageAppend(message, payload, payloadLength);
            if (OT_ERROR_NONE != error)
            {
                break;
            }
        }

        memset(&messageInfo, 0, sizeof(messageInfo));
        messageInfo.mPeerAddr = coapDestinationIp;
        messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

        if ((coapType == OT_COAP_TYPE_CONFIRMABLE) || (aCoapCode == OT_COAP_CODE_GET))
        {
            error = otCoapSendRequestWithParameters(otGetInstance(), message, &messageInfo, &nwk_mgm_ack_process,
                                                    NULL, NULL);
        }
        else
        {
            error = otCoapSendRequestWithParameters(otGetInstance(), message, &messageInfo, NULL, NULL, NULL);
        }
    } while (0);

    if ((error != OT_ERROR_NONE) && (message != NULL))
    {
        otMessageFree(message);
    }
    return error;
}

void nwk_mgm_child_register_post()
{
    otError error = OT_ERROR_NONE;
    otCoapCode CoapCode = OT_COAP_CODE_POST;
    otIp6Address coapDestinationIp = *otThreadGetRloc(otGetInstance());
    coapDestinationIp.mFields.m8[14] = 0xfc;
    coapDestinationIp.mFields.m8[15] = 0x00;
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    otIp6AddressToString(&coapDestinationIp, string, sizeof(string));

    otCoapType coapType = OT_COAP_TYPE_CONFIRMABLE;
    nwk_mgm_child_reg_data_t child_reg_data;
    uint8_t *payload = NULL;
    uint16_t payloadlength = 0;
    otRouterInfo parentInfo;
    otExtAddress aExtAddress;
    otChildInfo childInfo;

    memset(&child_reg_data, 0x0, sizeof(nwk_mgm_child_reg_data_t));
    child_reg_data.data_type = NWK_MGM_TYPE_CHILD_REG;
    error = otThreadGetParentInfo(otGetInstance(), &parentInfo);
    child_reg_data.parent = parentInfo.mRloc16;
    child_reg_data.self_rloc = otThreadGetRloc16(otGetInstance());
    aExtAddress = *otLinkGetExtendedAddress(otGetInstance());
    mem_memcpy(child_reg_data.self_extaddr, aExtAddress.m8, OT_EXT_ADDRESS_SIZE);

    child_reg_data.num = 0;
    uint16_t childernmax = otThreadGetMaxAllowedChildren(otGetInstance());

    for (uint16_t i = 0; i < childernmax; i++)
    {
        if ((otThreadGetChildInfoByIndex(otGetInstance(), i, &childInfo) != OT_ERROR_NONE) ||
                childInfo.mIsStateRestoring)
        {
            continue;
        }

        child_reg_data.child_info[child_reg_data.num].rloc = childInfo.mRloc16;
        mem_memcpy(child_reg_data.child_info[child_reg_data.num].extaddr, &childInfo.mExtAddress, OT_EXT_ADDRESS_SIZE);
        child_reg_data.child_info[child_reg_data.num].rssi = childInfo.mAverageRssi;
        child_reg_data.num++;
    }

    payload = mem_malloc(sizeof(nwk_mgm_child_reg_data_t));
    if (payload)
    {
        nwk_mgm_data_piece(NWK_MGM_TYPE_CHILD_REG, payload, &payloadlength, (uint8_t *)&child_reg_data);
        error = nwk_mgm_coap_request(CoapCode, coapDestinationIp, coapType, payload, payloadlength, RAFAEL_NWK_MGM_URL);
        nwk_mgm_printf("register %s %u %u \n", string, error, payloadlength);
        mem_free(payload);
        nwk_mgm_router_update_timer_set(NWK_MGM_ROUTER_KEEP_ALIVE_TIME);
    }

    return;
}

void _Network_Interface_Neighbor_Table_change(otNeighborTableEvent aEvent, const otNeighborTableEntryInfo *aEntryInfo)
{
    otDeviceRole mRole;

    mRole = otThreadGetDeviceRole(otGetInstance());

    if (mRole == OT_DEVICE_ROLE_LEADER)
    {
        switch (aEvent)
        {
        case OT_NEIGHBOR_TABLE_EVENT_CHILD_ADDED:
            if (nwk_mgm_child_reg_table_add(OT_DEVICE_ROLE_CHILD,
                                            otThreadGetRloc16(otGetInstance()),
                                            aEntryInfo->mInfo.mChild.mRloc16,
                                            (uint8_t *)&aEntryInfo->mInfo.mChild.mExtAddress.m8,
                                            aEntryInfo->mInfo.mChild.mAverageRssi))
            {
                nwk_mgm_printf("leader add fail \n");
            }
            nwk_mgm_router_update_timer_set(NWK_MGM_ROUTER_KEEP_ALIVE_TIME);
            break;

        case OT_NEIGHBOR_TABLE_EVENT_CHILD_REMOVED:
            nwk_mgm_printf("leader remove \n");
            nwk_mgm_child_reg_table_remove(OT_DEVICE_ROLE_CHILD, (uint8_t *)&aEntryInfo->mInfo.mChild.mExtAddress.m8);
            break;

        default:
            break;
        }
    }
    else if (mRole == OT_DEVICE_ROLE_ROUTER)
    {
        if (aEvent == OT_NEIGHBOR_TABLE_EVENT_CHILD_ADDED || aEvent == OT_NEIGHBOR_TABLE_EVENT_CHILD_REMOVED)
        {
            nwk_mgm_reg_send_timer_set(10);
        }
    }
}

static void nwk_mgm_child_register_post_resend()
{
    otDeviceRole mRole = otThreadGetDeviceRole(otGetInstance());

    if (mRole == OT_DEVICE_ROLE_ROUTER)
    {
        nwk_mgm_child_register_post();
    }
}

static void mgm_router_update_timer_handler()
{
    otDeviceRole mRole = otThreadGetDeviceRole(otGetInstance());

    if (mRole == OT_DEVICE_ROLE_ROUTER)
    {
        nwk_mgm_child_register_post();
    }
    else if (mRole == OT_DEVICE_ROLE_LEADER)
    {
        otChildInfo childInfo;
        uint16_t childernmax = otThreadGetMaxAllowedChildren(otGetInstance());

        for (uint16_t i = 0; i < childernmax; i++)
        {
            if ((otThreadGetChildInfoByIndex(otGetInstance(), i, &childInfo) != OT_ERROR_NONE) ||
                    childInfo.mIsStateRestoring)
            {
                continue;
            }

            nwk_mgm_child_reg_table_add(OT_DEVICE_ROLE_CHILD,
                                        otThreadGetRloc16(otGetInstance()),
                                        childInfo.mRloc16,
                                        childInfo.mExtAddress.m8,
                                        childInfo.mAverageRssi);
        }
    }
    nwk_mgm_router_update_timer_set(NWK_MGM_ROUTER_KEEP_ALIVE_TIME);
}

static void nwk_mgm_timer_handler(void *p_param)
{
    if ((mgm_reg_send_timer > 0 && (--mgm_reg_send_timer == 0)))
    {
        nwk_mgm_child_register_post_resend();
    }
    if (mgm_router_up_timer > 0 && (--mgm_router_up_timer == 0))
    {
        mgm_router_update_timer_handler();
    }

    /*no timeout required*/
    // nwk_mgm_child_reg_table_time_handler();

    sw_timer_start(nwk_mgm_timer);
}

void nwk_mgm_init(otInstance *aInstance, bool is_leader)
{
    memset(&nwk_mgm_resource, 0, sizeof(nwk_mgm_resource));

    nwk_mgm_resource.mUriPath = RAFAEL_NWK_MGM_URL;
    nwk_mgm_resource.mContext = aInstance;
    nwk_mgm_resource.mHandler = &nwk_mgm_request_proccess;
    nwk_mgm_resource.mNext = NULL;

    otCoapAddResource(aInstance, &nwk_mgm_resource);

    if (is_leader == true && NULL == nwk_mgm_child_reg_table)
    {
        nwk_mgm_child_reg_table = mem_malloc(sizeof(nwk_mgm_child_reg_table_t) * NWK_MGM_CHILD_REQ_TATLE_SIZE);
        if (nwk_mgm_child_reg_table)
        {
            memset(nwk_mgm_child_reg_table, 0x0, sizeof(nwk_mgm_child_reg_table_t) * NWK_MGM_CHILD_REQ_TATLE_SIZE);
        }
    }

    if (NULL == nwk_mgm_timer)
    {
        nwk_mgm_timer = sw_timer_create("nwk_mgm_timer",
                                        (1000),
                                        false,
                                        SW_TIMER_EXECUTE_ONCE_FOR_EACH_TIMEOUT,
                                        NULL,
                                        nwk_mgm_timer_handler);
        sw_timer_start(nwk_mgm_timer);
    }
    else
    {
        info("nwk_mgm_timer exist\n");
    }
}