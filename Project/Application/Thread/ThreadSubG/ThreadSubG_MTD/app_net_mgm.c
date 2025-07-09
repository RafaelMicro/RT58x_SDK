/**
 * @file app_net_mgm.c
 * @author
 * @brief
 * @version 0.1
 * @date 2025-02-05
 *
 * @copyright Copyright (c) 2025
 *
 */
/* flow diagram
======================== (Non-Leader Device Process) ========================
+-----------------+
| 1. Start Thread |
+-----------------+
        |
        v
+------------------------------+
| 2. Became a router or Child? |
+------------------+-----------+
        | Router         | Child
        |                v
        |   +------------------------------------------+
        |   | 3. Send Challenge Message and start      |
        |   |    Challenge Timer?                      |
        |   +----------------+-------------------------+
        |                |
        |                v
        |   +------------------------------------------+
        |   | 4. Challenge Time Expired?               |
        |   +----------------+-------------------------+
        |         | Yes                            | No
        |         v                                v
        |   +-------------------------------+    +----------------------------------+
        |   | 5. Received Accepted Message? |    | 6. Continue Wait Challenge Time. |
        |   +--------------+----------------+    +--------------+-------------------+
        |         | Yes                | No
        |         |                    v
        |         |        +---------------------------------------------------------------+
        |         |        | 7. Challenge Try counter Increase, if counter Greater than 5. |
        |         |        +---------------------------------------------------------------+
        |         |                    | Yes                                | No
        |         |                    v                                    v
        |         |        +----------------------+                 +----------------------+
        |         |        | 8. Return to Step 3. |                 | 9. Return to Step 1. |
        v         v        +----------------------+                 +----------------------+
+-----------------------+
| 10. Networking Comple. |
+-----------------------+
*/
#include "openthread-core-RT58x-config.h"
#include <openthread/random_noncrypto.h>
#include "main.h"
#include "sw_timer.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"

#include "bin_version.h"
#if CFG_USE_CENTRAK_CONFIG
#define NET_MGM_NODE_CHALLENGE_TIME 35

typedef struct
{
    uint16_t rloc;
    uint8_t extaddr[OT_EXT_ADDRESS_SIZE];
    int8_t rssi;
} __attribute__((packed)) net_mgm_node_info_t;

typedef struct
{
    uint32_t header;
    bool need_erase;
} __attribute__((packed)) net_mgm_node_reset_t;


typedef struct
{
    uint32_t header;
    uint8_t role;
    uint16_t parent;
    uint16_t self_rloc;
    uint8_t self_extaddr[OT_EXT_ADDRESS_SIZE];
    int8_t rssi;
    uint32_t version;
} __attribute__((packed)) net_mgm_node_challenge_t;

typedef struct
{
    uint32_t header;
    int status;
} __attribute__((packed)) net_mgm_node_accepted_t;


static bool net_mgm_node_reset_erase = false;
static bool net_mgm_network_comple = false;
static uint8_t net_mgm_node_challenge_counts = 0;
static uint8_t last_router_id = 0xff;
static sw_timer_t *net_mgm_timer = NULL;
static uint16_t net_mgm_node_challenge_timer = 0;
static uint16_t net_mgm_node_attach_timer = 0;
static uint16_t net_mgm_node_reset_contdown = 0xffff;

unsigned int net_mgm_debug_flags = 0;
#define net_mgm_printf(args...)      \
    do                               \
    {                                \
        if (net_mgm_debug_flags > 0) \
            info(args);              \
    } while (0);

void net_mgm_debug_level(unsigned int level)
{
    net_mgm_debug_flags = level;
}

/*
======================== (node_challenge) ========================
+-------------+-----------+-------------+----------------+-------------------+-----------+---------------+
|  Header(4)  |  role(2)  |  parent(2)  |  self rloc(2)  |  self extaddr(8)  |  rssi(1)  |  version (4)  |
+-------------+-----------+-------------+----------------+-------------------+-----------+---------------+
| 88 88 00 84 |           |             |                |                   |           |               |
+-------------+-----------+-------------+----------------+-------------------+-----------+---------------+
======================== (node_accepted) ========================
+-------------+-------------+
|  Header(4)  |  status(4)  |
+-------------+-------------+
| 88 88 00 85 |             |
+-------------+-------------+
*/
static int nwk_node_data_parse(uint32_t header, uint8_t *payload, uint16_t payloadlength, void *data)
{
    net_mgm_node_challenge_t *net_mgm_node_challenge = NULL;
    net_mgm_node_accepted_t *net_mgm_node_accepted = NULL;
    net_mgm_node_reset_t *net_mgm_node_reset = NULL;

    uint8_t *tmp = payload;

    if (header == NET_MGM_NODE_CHALLENGE_HEADER)
    {
        net_mgm_node_challenge = (net_mgm_node_challenge_t *)data;
        memcpy(&net_mgm_node_challenge->header, tmp, 4);
        tmp += 4;
        net_mgm_node_challenge->role = *tmp++;
        memcpy(&net_mgm_node_challenge->parent, tmp, 2);
        tmp += 2;
        memcpy(&net_mgm_node_challenge->self_rloc, tmp, 2);
        tmp += 2;
        memcpy(&net_mgm_node_challenge->self_extaddr, tmp, OT_EXT_ADDRESS_SIZE);
        tmp += OT_EXT_ADDRESS_SIZE;
        net_mgm_node_challenge->rssi = *tmp++;
        memcpy(&net_mgm_node_challenge->version, tmp, 4);
        tmp += 4;
    }
    else if (header == NET_MGM_NODE_ACCEPTED_HEADER)
    {
        net_mgm_node_accepted = (net_mgm_node_accepted_t *)data;
        memcpy(&net_mgm_node_accepted->header, tmp, 4);
        tmp += 4;
        memcpy(&net_mgm_node_accepted->status, tmp, 4);
        tmp += 4;
    }
    else if (header == NET_MGM_NODE_RESET_HEADER)
    {
        net_mgm_node_reset = (net_mgm_node_reset_t *)data;
        memcpy(&net_mgm_node_reset->header, tmp, 4);
        tmp += 4;
        net_mgm_node_reset->need_erase = *tmp++;
    }

    if ((tmp - payload) != payloadlength)
    {
        net_mgm_printf("nwk node %08x parse fail (%u/%u)\n", header, (tmp - payload), payloadlength);
        return 1;
    }
    return 0;
}

static void nwk_node_data_piece(uint32_t header, uint8_t *payload, uint16_t *payloadlength, void *buf)
{
    net_mgm_node_challenge_t *net_mgm_node_challenge = NULL;
    net_mgm_node_accepted_t *net_mgm_node_accepted = NULL;
    net_mgm_node_reset_t *net_mgm_node_reset = NULL;

    uint8_t *data = (uint8_t *)buf;
    uint8_t *tmp = payload;
    memcpy(tmp, data, 4);
    tmp += 4;

    if (header == NET_MGM_NODE_CHALLENGE_HEADER)
    {
        net_mgm_node_challenge = (net_mgm_node_challenge_t *)data;
        *tmp++ = net_mgm_node_challenge->role;
        memcpy(tmp, &net_mgm_node_challenge->parent, 2);
        tmp += 2;
        memcpy(tmp, &net_mgm_node_challenge->self_rloc, 2);
        tmp += 2;
        memcpy(tmp, &net_mgm_node_challenge->self_extaddr, OT_EXT_ADDRESS_SIZE);
        tmp += OT_EXT_ADDRESS_SIZE;
        *tmp++ = net_mgm_node_challenge->rssi;
        memcpy(tmp, &net_mgm_node_challenge->version, 4);
        tmp += 4;
    }
    else if (header == NET_MGM_NODE_ACCEPTED_HEADER)
    {
        net_mgm_node_accepted = (net_mgm_node_accepted_t *)data;
        memcpy(tmp, &net_mgm_node_accepted->status, 4);
        tmp += 4;
    }
    else if (header == NET_MGM_NODE_RESET_HEADER)
    {
        net_mgm_node_reset = (net_mgm_node_reset_t *)data;
        *tmp++ = net_mgm_node_reset->need_erase;
    }
    *payloadlength = (tmp - payload);
}

void net_mgm_node_challenge_send()
{
    net_mgm_node_challenge_t net_mgm_node_challenge;
    uint8_t *payload = NULL;
    uint16_t payloadlength = 0;
    otExtAddress aExtAddress;
    otRouterInfo parentInfo;
    otNeighborInfo         neighborInfo;
    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;

    memset(&net_mgm_node_challenge, 0x0, sizeof(net_mgm_node_challenge_t));
    net_mgm_node_challenge.header = NET_MGM_NODE_CHALLENGE_HEADER;

    do
    {
        if (otThreadGetParentInfo(otGetInstance(), &parentInfo) != OT_ERROR_NONE)
        {
            net_mgm_printf("get parent fail \r\n");
            break;
        }

        while (otThreadGetNextNeighborInfo(otGetInstance(), &iterator, &neighborInfo) == OT_ERROR_NONE)
        {
            if (neighborInfo.mRloc16 == parentInfo.mRloc16)
            {
                break;
            }
        }
        net_mgm_node_challenge.role = otThreadGetDeviceRole(otGetInstance());
        net_mgm_printf("parent %04X %04X", neighborInfo.mRloc16, parentInfo.mRloc16);
        net_mgm_node_challenge.parent = parentInfo.mRloc16;
        net_mgm_node_challenge.self_rloc = otThreadGetRloc16(otGetInstance());
        aExtAddress = *otLinkGetExtendedAddress(otGetInstance());
        memcpy(net_mgm_node_challenge.self_extaddr, aExtAddress.m8, OT_EXT_ADDRESS_SIZE);

        net_mgm_node_challenge.rssi = neighborInfo.mLastRssi;
        net_mgm_node_challenge.version = GET_BIN_VERSION(systeminfo.sysinfo);

        payload = mem_malloc(sizeof(net_mgm_node_challenge_t));
        if (payload)
        {
            nwk_node_data_piece(NET_MGM_NODE_CHALLENGE_HEADER, payload, &payloadlength, &net_mgm_node_challenge);

            otIp6Address dst_addr = *otThreadGetRloc(otGetInstance());
            dst_addr.mFields.m8[14] = 0xFC;
            dst_addr.mFields.m8[15] = 0x00;
            if (app_udp_send(dst_addr, payload, payloadlength))
            {
                info("[error] net_mgm_node_challenge_send error \r\n");
            }
            char string[OT_IP6_ADDRESS_STRING_SIZE];
            otIp6AddressToString(&dst_addr, string, sizeof(string));
            info("[Networking] >> challenge (%s)\r\n", string);
            mem_free(payload);
        }
        else
        {
            net_mgm_printf("net_mgm_node_challenge_send alloc fail \n");
        }
    } while (0);

    return;
}

void app_udp_comm_net_mgm_node_accepted_proc(uint8_t *data, uint16_t lens, otIp6Address src_addr)
{
    uint8_t *payload = NULL;
    uint16_t payloadlength = 0;
    net_mgm_node_accepted_t net_mgm_node_accepted;

    if (!nwk_node_data_parse(NET_MGM_NODE_ACCEPTED_HEADER, data, lens, &net_mgm_node_accepted))
    {
        char string[OT_IP6_ADDRESS_STRING_SIZE];
        otIp6AddressToString(&src_addr, string, sizeof(string));
        info("[Networking] << accepted[%08x] (%s)\r\n", net_mgm_node_accepted.status, string);
        if (!net_mgm_node_accepted.status)
        {
            net_mgm_network_comple = true;
        }
    }
}

void app_udp_comm_net_mgm_node_reset_proc(uint8_t *data, uint16_t lens, otIp6Address src_addr)
{
    uint8_t *payload = NULL;
    uint16_t payloadlength = 0;
    net_mgm_node_reset_t net_mgm_node_reset;

    if (!nwk_node_data_parse(NET_MGM_NODE_RESET_HEADER, data, lens, &net_mgm_node_reset))
    {
        char string[OT_IP6_ADDRESS_STRING_SIZE];
        otIp6AddressToString(&src_addr, string, sizeof(string));
        info("[Networking] << reset %s \r\n", (net_mgm_node_reset.need_erase == true ? "need erase" : "no erase"));
        net_mgm_node_reset_contdown = 30;
        net_mgm_node_reset_erase = net_mgm_node_reset.need_erase;
        info("perform node reset %d seconds later\r\n", net_mgm_node_reset_contdown);
    }
}

/*network management time*/
static void net_mgm_timer_handler(void *p_param)
{
    if (net_mgm_network_comple == false)
    {
        net_mgm_node_attach_timer++;
        net_mgm_printf("attach %u \r\n", net_mgm_node_attach_timer);
        app_set_led1_toggle();
    }
    else
    {
        app_set_led1_on();
    }

    /* Non-Leader Device Process step 4 */
    if (net_mgm_node_challenge_timer > 0 && (--net_mgm_node_challenge_timer == 0))
    {
        /* Non-Leader Device Process step 5 */
        if (net_mgm_network_comple == true)
        {
            net_mgm_node_challenge_timer = 0;
            net_mgm_node_challenge_counts = 0;
        }
        else
        {
            /* Non-Leader Device Process step 7 */
            net_mgm_node_challenge_counts++;
            if (net_mgm_node_challenge_counts > 5)
            {
                /* Non-Leader Device Process step 9 */
                net_mgm_node_challenge_counts = 0;
                if (otThreadSetEnabled(otGetInstance(), false) != OT_ERROR_NONE)
                {
                    info("otThreadSetEnabled false fail \r\n");
                }
                if (otInstanceErasePersistentInfo(otGetInstance()) != OT_ERROR_NONE)
                {
                    info("otInstanceErasePersistentInfo fail \r\n");
                }
                if (otThreadSetEnabled(otGetInstance(), true) != OT_ERROR_NONE)
                {
                    info("otThreadSetEnabled true fail \r\n");
                }
            }
            else
            {
                /* Non-Leader Device Process step 8 */
                info("[Networking] challenge try %d \r\n", net_mgm_node_challenge_counts);
                net_mgm_node_challenge_send();
                net_mgm_node_challenge_timer = NET_MGM_NODE_CHALLENGE_TIME;
            }
        }
    }

    if (net_mgm_node_reset_contdown != 0xffff)
    {
        if (net_mgm_node_reset_contdown == 0)
        {
            if (net_mgm_node_reset_erase)
            {
                otInstanceFactoryReset(otGetInstance());
            }
            else
            {
                otInstanceReset(otGetInstance());
            }
        }
        else
        {
            info("node reset contdown %d\r\n", net_mgm_node_reset_contdown);
            net_mgm_node_reset_contdown--;
        }
    }
    sw_timer_start(net_mgm_timer);
}

void net_mgm_interface_state_change(uint32_t aFlags, void *aContext)
{
    uint8_t show_ip = 0, is_child = 0;
    otOperationalDataset ds;
    if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        otDeviceRole changeRole = otThreadGetDeviceRole(otGetInstance());
        switch (changeRole)
        {
        case OT_DEVICE_ROLE_DETACHED:
            info("Change to detached \r\n");
            net_mgm_node_attach_timer = 0;
            net_mgm_node_challenge_timer = 0;
            net_mgm_network_comple = false;
            /* Non-Leader Device Process step 1 */
            break;
        case OT_DEVICE_ROLE_DISABLED:
            info("Change to disabled \r\n");
            break;
        case OT_DEVICE_ROLE_LEADER:
            info("Change to leader \r\n");
            otLeaderData leaderData;

            app_set_led1_on();

            if (otThreadGetLeaderData(otGetInstance(), &leaderData) != OT_ERROR_NONE)
            {
                info("otThreadGetLeaderData fail \r\n");
            }
            info("Leaderdata \r\n");
            info("Partition ID        : %u \r\n", leaderData.mPartitionId);
            info("Weighting           : %u \r\n", leaderData.mWeighting);
            info("Data Version        : %u \r\n", leaderData.mDataVersion);
            info("Stable Data Version : %u \r\n", leaderData.mStableDataVersion);
            info("Leader Router ID    : %u \r\n", leaderData.mLeaderRouterId);
            show_ip = 1;
            break;
        case OT_DEVICE_ROLE_ROUTER:
            info("Change to router \r\n");
            show_ip = 1;
            net_mgm_network_comple = true;
            /* Non-Leader Device Process step 2 */
            break;
        case OT_DEVICE_ROLE_CHILD:
            info("Change to child \r\n");
            is_child = 1;
            show_ip = 1;
            break;
        default:
            break;
        }

        if (show_ip)
        {
            const otNetifAddress *unicastAddress = otIp6GetUnicastAddresses(otGetInstance());

            for (const otNetifAddress *addr = unicastAddress; addr; addr = addr->mNext)
            {
                char string[OT_IP6_ADDRESS_STRING_SIZE];

                otIp6AddressToString(&addr->mAddress, string, sizeof(string));
                info("%s\n", string);
            }
        }
        if (is_child)
        {
            /* Non-Leader Device Process step 3 */
            net_mgm_network_comple = false;
            net_mgm_node_challenge_send();
            net_mgm_node_challenge_timer = NET_MGM_NODE_CHALLENGE_TIME;
        }
    }
}


void net_mgm_init()
{
    /*network management*/
    info("network management eanble \r\n");
    if (NULL == net_mgm_timer)
    {
        net_mgm_timer = sw_timer_create("nwk_mgm_timer",
                                        (1000),
                                        false,
                                        SW_TIMER_EXECUTE_ONCE_FOR_EACH_TIMEOUT,
                                        NULL,
                                        net_mgm_timer_handler);

        sw_timer_start(net_mgm_timer);
    }
    else
    {
        info("nwk_mgm_timer exist\n");
    }

    if (otThreadSetEnabled(otGetInstance(), true) != OT_ERROR_NONE)
    {
        info("otThreadSetEnabled fail \r\n");
    }

    if (otSetStateChangedCallback(otGetInstance(), net_mgm_interface_state_change, 0) != OT_ERROR_NONE)
    {
        info("otSetStateChangedCallback fail \r\n");
    }
}
#endif