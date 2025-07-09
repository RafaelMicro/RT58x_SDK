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
======================== (Leader Device Process) ========================

+------------------------------------------------+
| 1. Start Polling Timer (Long Interval)         |
+------------------+-----------------------------+
                     |
                     v
+------------------------------------------------+
| 2. Has Polling Time Expired?                   |
+------------------+-----------------------------+
        | No                                     | Yes
        v                                        v
+------------------------------+   +--------------------------------------------------+
| 6. Continue Polling Process  |   | 3. Check Router Table:                           |
+------------------------------+   |    - Are there any Routers?                      |
                                   |    - Determine the next Router to ask.           |
                                   +----------------+---------------------------------+
                                           | Yes                             | No
                                           v                                 v
                            +-------------------------------+      +----------------------+
                            | 4. Ask the selected Router    |      | 5. Return to Step 1  |
                            |    & retrieve child info.     |      |    (Restart Process) |
                            |  - Start Polling Timer        |      +----------------------+
                            |    (Short Interval).          |
                            |  - Return to Step 2.          |
                            +-------------------------------+

*/
#include "openthread-core-RT58x-config.h"
#include <openthread/random_noncrypto.h>
#include "main.h"
#include "sw_timer.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"

#include "bin_version.h"
#if CFG_USE_CENTRAK_CONFIG
#define NET_MGM_NODE_TATLE_MAX_SIZE (OPENTHREAD_CONFIG_MLE_MAX_ROUTERS + 300)

#define NET_MGM_ASK_NODE_NUM_POLLING_SHORT_TIME 10
#define NET_MGM_ASK_NODE_NUM_POLLING_LONG_TIME 120
#define NET_MGM_NODE_SURVIVAL_TIME ((NET_MGM_ASK_NODE_NUM_POLLING_SHORT_TIME*OPENTHREAD_CONFIG_MLE_MAX_ROUTERS)+(NET_MGM_ASK_NODE_NUM_POLLING_LONG_TIME*2))

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
} __attribute__((packed)) net_mgm_node_ask_t;

typedef struct
{
    uint32_t header;
    uint32_t version;
    uint16_t parent;
    uint16_t self_rloc;
    uint8_t self_extaddr[OT_EXT_ADDRESS_SIZE];
    uint16_t num;
    net_mgm_node_info_t child_info[OPENTHREAD_CONFIG_MLE_MAX_CHILDREN];
} __attribute__((packed)) net_mgm_node_reply_t;

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

typedef struct
{
    uint8_t used;
    uint8_t role;
    uint16_t parent;
    uint16_t rloc;
    uint8_t extaddr[OT_EXT_ADDRESS_SIZE];
    int8_t rssi;
    uint32_t version;
    uint16_t survivaltime;
} net_mgm_node_table_t;

static net_mgm_node_table_t *net_mgm_node_table = NULL;

static bool net_mgm_node_reset_erase = false;
static bool net_mgm_network_comple = false;
static uint8_t net_mgm_node_challenge_counts = 0;
static uint8_t last_router_id = 0xff;
static sw_timer_t *net_mgm_timer = NULL;
static uint16_t net_mgm_ask_node_polling_timer = 0;
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

bool net_mgm_check_leader_pin_state()
{
    return (gpio_pin_get(23) == 0);
}

static int net_mgm_node_table_add(uint8_t role, uint16_t parent, uint16_t rloc, uint8_t *extaddr, int8_t rssi, uint32_t version)
{
    uint16_t i;
    uint16_t freeRouterIdx = NET_MGM_NODE_TATLE_MAX_SIZE;
    uint16_t freeChildIdx = NET_MGM_NODE_TATLE_MAX_SIZE;

    if (!net_mgm_node_table)
    {
        net_mgm_printf("net_mgm_node_table is NULL \r\n");
        return -1;
    }

    // Check if the entry already exists
    for (i = 0; i < NET_MGM_NODE_TATLE_MAX_SIZE; i++)
    {
        if (net_mgm_node_table[i].used &&
                memcmp(net_mgm_node_table[i].extaddr, extaddr, OT_EXT_ADDRESS_SIZE) == 0)
        {
            enter_critical_section();
            net_mgm_node_table[i].parent = parent;
            net_mgm_node_table[i].role = role;
            net_mgm_node_table[i].rloc = rloc;
            net_mgm_node_table[i].rssi = rssi;
            if (version != 0xFFFFFFFF)
            {
                net_mgm_node_table[i].version = version;
            }
            net_mgm_node_table[i].survivaltime = NET_MGM_NODE_SURVIVAL_TIME;
            leave_critical_section();
            net_mgm_printf("Update %02X%02X%02X%02X%02X%02X%02X%02X \n",
                           net_mgm_node_table[i].extaddr[0],
                           net_mgm_node_table[i].extaddr[1],
                           net_mgm_node_table[i].extaddr[2],
                           net_mgm_node_table[i].extaddr[3],
                           net_mgm_node_table[i].extaddr[4],
                           net_mgm_node_table[i].extaddr[5],
                           net_mgm_node_table[i].extaddr[6],
                           net_mgm_node_table[i].extaddr[7]);
            return 0;
        }
    }

    // Search for an available slot
    for (i = 0; i < NET_MGM_NODE_TATLE_MAX_SIZE; i++)
    {
        if (net_mgm_node_table[i].used == 0)
        {
            if (i < OPENTHREAD_CONFIG_MLE_MAX_ROUTERS)
            {
                if (freeRouterIdx == NET_MGM_NODE_TATLE_MAX_SIZE)
                {
                    freeRouterIdx = i; //Router slot is found
                }
            }
            else
            {
                if (freeChildIdx == NET_MGM_NODE_TATLE_MAX_SIZE)
                {
                    freeChildIdx = i; // Record the first available child slot
                    break;
                }
            }
        }
    }

    // Determine the insertion index
    uint16_t insertIdx = (role == OT_DEVICE_ROLE_ROUTER) ? freeRouterIdx : freeChildIdx;
    if (insertIdx == NET_MGM_NODE_TATLE_MAX_SIZE)
    {
        info("No available slot \r\n");
        return -1;    // No available slot
    }

    enter_critical_section();
    net_mgm_node_table[insertIdx].used = 1;
    net_mgm_node_table[insertIdx].parent = parent;
    net_mgm_node_table[insertIdx].role = role;
    net_mgm_node_table[insertIdx].rloc = rloc;
    memcpy(net_mgm_node_table[insertIdx].extaddr, extaddr, OT_EXT_ADDRESS_SIZE);
    net_mgm_node_table[insertIdx].rssi = rssi;
    net_mgm_node_table[insertIdx].version = version;
    net_mgm_node_table[insertIdx].survivaltime = NET_MGM_NODE_SURVIVAL_TIME;
    leave_critical_section();
    net_mgm_printf("add %s %04X %02X%02X%02X%02X%02X%02X%02X%02X %d %08x \n",
                   otThreadDeviceRoleToString(net_mgm_node_table[i].role),
                   net_mgm_node_table[i].rloc,
                   net_mgm_node_table[i].extaddr[0],
                   net_mgm_node_table[i].extaddr[1],
                   net_mgm_node_table[i].extaddr[2],
                   net_mgm_node_table[i].extaddr[3],
                   net_mgm_node_table[i].extaddr[4],
                   net_mgm_node_table[i].extaddr[5],
                   net_mgm_node_table[i].extaddr[6],
                   net_mgm_node_table[i].extaddr[7],
                   net_mgm_node_table[i].rssi,
                   net_mgm_node_table[i].version);
    return 0;
}

bool net_mgm_node_table_find(uint8_t *aExtAddress)
{
    bool ret = false;
    uint16_t i = 0;
    if (net_mgm_node_table)
    {
        for (i = 0; i < NET_MGM_NODE_TATLE_MAX_SIZE; i++)
        {
            if (net_mgm_node_table[i].used &&
                    memcmp(net_mgm_node_table[i].extaddr, aExtAddress, OT_EXT_ADDRESS_SIZE) == 0)
            {
                net_mgm_printf("fined %02X%02X%02X%02X%02X%02X%02X%02X \n", aExtAddress[0], aExtAddress[1], aExtAddress[2],
                               aExtAddress[3], aExtAddress[4], aExtAddress[5], aExtAddress[6], aExtAddress[7]);
                ret = true;
                break;
            }
        }
        if (ret == false)
        {
            net_mgm_printf("not find %02X%02X%02X%02X%02X%02X%02X%02X \n", aExtAddress[0], aExtAddress[1], aExtAddress[2],
                           aExtAddress[3], aExtAddress[4], aExtAddress[5], aExtAddress[6], aExtAddress[7]);
        }
    }
    return ret;
}

void net_mgm_node_table_display()
{
    uint16_t i = 0, count = 0;
    if (net_mgm_node_table)
    {
        info("index role parent rloc extaddr rssi version survivaltime history_rloc\r\n");
        info("===============================================\r\n");
        for (i = 0; i < NET_MGM_NODE_TATLE_MAX_SIZE; i++)
        {
            if (net_mgm_node_table[i].used && (net_mgm_node_table[i].role == OT_DEVICE_ROLE_ROUTER))
            {
                ++count;
                info("[%u] %s %04X %04X %02X%02X%02X%02X%02X%02X%02X%02X %d 0x%08x %u \r\n",
                     count,
                     otThreadDeviceRoleToString(net_mgm_node_table[i].role),
                     net_mgm_node_table[i].parent,
                     net_mgm_node_table[i].rloc,
                     net_mgm_node_table[i].extaddr[0],
                     net_mgm_node_table[i].extaddr[1],
                     net_mgm_node_table[i].extaddr[2],
                     net_mgm_node_table[i].extaddr[3],
                     net_mgm_node_table[i].extaddr[4],
                     net_mgm_node_table[i].extaddr[5],
                     net_mgm_node_table[i].extaddr[6],
                     net_mgm_node_table[i].extaddr[7],
                     net_mgm_node_table[i].rssi,
                     net_mgm_node_table[i].version,
                     net_mgm_node_table[i].survivaltime);
            }
        }
        for (i = 0; i < NET_MGM_NODE_TATLE_MAX_SIZE; i++)
        {
            if (net_mgm_node_table[i].used)
            {
                if (net_mgm_node_table[i].role == OT_DEVICE_ROLE_CHILD)
                {
                    ++count;
                    info("[%u] %s %04X %04X %02X%02X%02X%02X%02X%02X%02X%02X %d 0x%08x %u \r\n",
                         count,
                         otThreadDeviceRoleToString(net_mgm_node_table[i].role),
                         net_mgm_node_table[i].parent,
                         net_mgm_node_table[i].rloc,
                         net_mgm_node_table[i].extaddr[0],
                         net_mgm_node_table[i].extaddr[1],
                         net_mgm_node_table[i].extaddr[2],
                         net_mgm_node_table[i].extaddr[3],
                         net_mgm_node_table[i].extaddr[4],
                         net_mgm_node_table[i].extaddr[5],
                         net_mgm_node_table[i].extaddr[6],
                         net_mgm_node_table[i].extaddr[7],
                         net_mgm_node_table[i].rssi,
                         net_mgm_node_table[i].version,
                         net_mgm_node_table[i].survivaltime);
                }
            }
        }
        for (i = 0; i < NET_MGM_NODE_TATLE_MAX_SIZE; i++)
        {
            if (net_mgm_node_table[i].used)
            {

                if ((net_mgm_node_table[i].role != OT_DEVICE_ROLE_CHILD) &&
                        (net_mgm_node_table[i].role != OT_DEVICE_ROLE_ROUTER))
                {
                    ++count;
                    info("[%u] %s %04X %04X %02X%02X%02X%02X%02X%02X%02X%02X %d 0x%08x %u \r\n",
                         count,
                         otThreadDeviceRoleToString(net_mgm_node_table[i].role),
                         net_mgm_node_table[i].parent,
                         net_mgm_node_table[i].rloc,
                         net_mgm_node_table[i].extaddr[0],
                         net_mgm_node_table[i].extaddr[1],
                         net_mgm_node_table[i].extaddr[2],
                         net_mgm_node_table[i].extaddr[3],
                         net_mgm_node_table[i].extaddr[4],
                         net_mgm_node_table[i].extaddr[5],
                         net_mgm_node_table[i].extaddr[6],
                         net_mgm_node_table[i].extaddr[7],
                         net_mgm_node_table[i].rssi,
                         net_mgm_node_table[i].version,
                         net_mgm_node_table[i].survivaltime);
                }
            }
        }
        info("=============================================== \n");
        info("total num %u \n", count);
    }
}

uint16_t nwk_mgm_node_num_display()
{
    uint16_t count = 0, i = 0;
    if (net_mgm_node_table)
    {
        for (i = 0; i < NET_MGM_NODE_TATLE_MAX_SIZE; i++)
        {
            if (net_mgm_node_table[i].used)
            {

                if ((net_mgm_node_table[i].role == OT_DEVICE_ROLE_CHILD) ||
                        (net_mgm_node_table[i].role == OT_DEVICE_ROLE_ROUTER))
                {
                    ++count;
                }
            }
        }
    }
    return count;
}

/*
======================== (node_ask) ========================
+-------------+
|  Header(4)  |
+-------------+
| 88 88 00 82 |
+-------------+
======================== (node_reply) ========================
+-------------+--------------+-------------+----------------+-------------------+-------------+-------------------------------------------------------+
|  Header(4)  |  version(4)  |  parent(2)  |  self rloc(2)  |  self extaddr(8)  |  child num  | child_info (sizeof(net_mgm_node_info_t) * child num)  |
+-------------+--------------+-------------+----------------+-------------------+-------------+-------------------------------------------------------+
| 88 88 00 83 |              |             |                |                   |             |                                                       |
+-------------+--------------+-------------+----------------+-------------------+-------------+-------------------------------------------------------+
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
    net_mgm_node_ask_t *net_mgm_node_ask = NULL;
    net_mgm_node_reply_t *net_mgm_node_reply = NULL;
    net_mgm_node_challenge_t *net_mgm_node_challenge = NULL;
    net_mgm_node_accepted_t *net_mgm_node_accepted = NULL;
    net_mgm_node_reset_t *net_mgm_node_reset = NULL;

    uint8_t *tmp = payload;

    if (header == NET_MGM_NODE_ASK_HEADER)
    {
        net_mgm_node_ask = (net_mgm_node_ask_t *)data;
        memcpy(&net_mgm_node_ask->header, tmp, 4);
        tmp += 4;
    }
    else if (header == NET_MGM_NODE_REPLY_HEADER)
    {
        net_mgm_node_reply = (net_mgm_node_reply_t *)data;
        memcpy(&net_mgm_node_reply->header, tmp, 4);
        tmp += 4;
        memcpy(&net_mgm_node_reply->version, tmp, 4);
        tmp += 4;
        memcpy(&net_mgm_node_reply->parent, tmp, 2);
        tmp += 2;
        memcpy(&net_mgm_node_reply->self_rloc, tmp, 2);
        tmp += 2;
        memcpy(&net_mgm_node_reply->self_extaddr, tmp, OT_EXT_ADDRESS_SIZE);
        tmp += OT_EXT_ADDRESS_SIZE;
        memcpy(&net_mgm_node_reply->num, tmp, 2);
        tmp += 2;
        if (net_mgm_node_reply->num > 0 && net_mgm_node_reply->num <= OPENTHREAD_CONFIG_MLE_MAX_CHILDREN)
        {
            memcpy(&net_mgm_node_reply->child_info, tmp, (net_mgm_node_reply->num * sizeof(net_mgm_node_info_t)));
            tmp += (net_mgm_node_reply->num * sizeof(net_mgm_node_info_t));
        }
    }
    else if (header == NET_MGM_NODE_CHALLENGE_HEADER)
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
    net_mgm_node_ask_t *net_mgm_node_ask = NULL;
    net_mgm_node_reply_t *net_mgm_node_reply = NULL;
    net_mgm_node_challenge_t *net_mgm_node_challenge = NULL;
    net_mgm_node_accepted_t *net_mgm_node_accepted = NULL;
    net_mgm_node_reset_t *net_mgm_node_reset = NULL;

    uint8_t *data = (uint8_t *)buf;
    uint8_t *tmp = payload;
    memcpy(tmp, data, 4);
    tmp += 4;

    if (header == NET_MGM_NODE_ASK_HEADER)
    {
        /*only header*/
    }
    else if (header == NET_MGM_NODE_REPLY_HEADER)
    {
        net_mgm_node_reply = (net_mgm_node_reply_t *)data;
        memcpy(tmp, &net_mgm_node_reply->version, 4);
        tmp += 4;
        memcpy(tmp, &net_mgm_node_reply->parent, 2);
        tmp += 2;
        memcpy(tmp, &net_mgm_node_reply->self_rloc, 2);
        tmp += 2;
        memcpy(tmp, &net_mgm_node_reply->self_extaddr, OT_EXT_ADDRESS_SIZE);
        tmp += OT_EXT_ADDRESS_SIZE;
        memcpy(tmp, &net_mgm_node_reply->num, 2);
        tmp += 2;
        if (net_mgm_node_reply->num > 0 && net_mgm_node_reply->num <= OPENTHREAD_CONFIG_MLE_MAX_CHILDREN)
        {
            memcpy(tmp, &net_mgm_node_reply->child_info, (net_mgm_node_reply->num * sizeof(net_mgm_node_info_t)));
            tmp += (net_mgm_node_reply->num * sizeof(net_mgm_node_info_t));
        }
    }
    else if (header == NET_MGM_NODE_CHALLENGE_HEADER)
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

void net_mgm_node_ask_send(uint16_t rloc16)
{
    uint8_t *payload = NULL;
    uint16_t payloadlength = 0;
    net_mgm_node_ask_t net_mgm_node_ask;

    otIp6Address dst_rloc_ip = *otThreadGetRloc(otGetInstance());

    dst_rloc_ip.mFields.m8[14] = (rloc16 & 0xff00) >> 8;
    dst_rloc_ip.mFields.m8[15] = (rloc16 & 0xff);

    net_mgm_node_ask.header = NET_MGM_NODE_ASK_HEADER;

    payload = mem_malloc(sizeof(net_mgm_node_ask_t));
    if (payload)
    {
        nwk_node_data_piece(NET_MGM_NODE_ASK_HEADER, payload, &payloadlength, &net_mgm_node_ask);
        if (app_udp_send(dst_rloc_ip, payload, payloadlength))
        {
            info("[error] net node ask send error \r\n");
        }
        char string[OT_IP6_ADDRESS_STRING_SIZE];
        otIp6AddressToString(&dst_rloc_ip, string, sizeof(string));
        info("[Networking] >> polling ask (%s)\r\n", string);
        mem_free(payload);
    }
    else
    {
        net_mgm_printf("net_mgm_node_ask_send alloc fail \n");
    }
}

void net_mgm_node_reply_send(otIp6Address dst_addr)
{
    net_mgm_node_reply_t net_mgm_node_reply;
    uint8_t *payload = NULL;
    uint16_t payloadlength = 0;
    otLeaderData leader_data;
    otRouterInfo router_info;
    otExtAddress aExtAddress;
    otChildInfo childInfo;
    uint16_t next_hop_rloc16;
    uint8_t  path_cost;

    memset(&net_mgm_node_reply, 0x0, sizeof(net_mgm_node_reply_t));
    net_mgm_node_reply.header = NET_MGM_NODE_REPLY_HEADER;

    do
    {
        if (otThreadGetLeaderData(otGetInstance(), &leader_data) != OT_ERROR_NONE)
        {
            break;
        }
        if (otThreadGetRouterInfo(otGetInstance(), leader_data.mLeaderRouterId, &router_info) != OT_ERROR_NONE)
        {
            break;
        }
        otThreadGetNextHopAndPathCost(otGetInstance(), router_info.mRloc16, &next_hop_rloc16, &path_cost);
        net_mgm_node_reply.version = GET_BIN_VERSION(systeminfo.sysinfo);
        net_mgm_node_reply.parent = next_hop_rloc16;
        net_mgm_node_reply.self_rloc = otThreadGetRloc16(otGetInstance());
        aExtAddress = *otLinkGetExtendedAddress(otGetInstance());
        memcpy(net_mgm_node_reply.self_extaddr, aExtAddress.m8, OT_EXT_ADDRESS_SIZE);

        net_mgm_node_reply.num = 0;
        uint16_t childernmax = otThreadGetMaxAllowedChildren(otGetInstance());

        for (uint16_t i = 0; i < childernmax; i++)
        {
            if ((otThreadGetChildInfoByIndex(otGetInstance(), i, &childInfo) != OT_ERROR_NONE) ||
                    childInfo.mIsStateRestoring)
            {
                continue;
            }

            net_mgm_node_reply.child_info[net_mgm_node_reply.num].rloc = childInfo.mRloc16;
            memcpy(net_mgm_node_reply.child_info[net_mgm_node_reply.num].extaddr, &childInfo.mExtAddress, OT_EXT_ADDRESS_SIZE);
            net_mgm_node_reply.child_info[net_mgm_node_reply.num].rssi = childInfo.mLastRssi;
            net_mgm_node_reply.num++;
        }

        payload = mem_malloc(sizeof(net_mgm_node_reply_t));
        if (payload)
        {
            nwk_node_data_piece(NET_MGM_NODE_REPLY_HEADER, payload, &payloadlength, &net_mgm_node_reply);
            if (app_udp_send(dst_addr, payload, payloadlength))
            {
                info("[error] net_mgm_node_reply_send error \r\n");
            }
            char string[OT_IP6_ADDRESS_STRING_SIZE];
            otIp6AddressToString(&dst_addr, string, sizeof(string));
            info("[Networking] >> polling reply (%s)\r\n", string);
            mem_free(payload);
        }
        else
        {
            net_mgm_printf("net_mgm_node_reply_send alloc fail \n");
        }
    } while (0);

    return;
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

void net_mgm_node_accepted_send(otIp6Address dst_addr, int status)
{
    uint8_t *payload = NULL;
    uint16_t payloadlength = 0;
    net_mgm_node_accepted_t net_mgm_node_accepted;

    net_mgm_node_accepted.header = NET_MGM_NODE_ACCEPTED_HEADER;
    net_mgm_node_accepted.status = status;

    payload = mem_malloc(sizeof(net_mgm_node_accepted_t));
    if (payload)
    {
        nwk_node_data_piece(NET_MGM_NODE_ACCEPTED_HEADER, payload, &payloadlength, &net_mgm_node_accepted);
        if (app_udp_send(dst_addr, payload, payloadlength))
        {
            info("[error] net_mgm_node_accepted_send error \r\n");
        }
        char string[OT_IP6_ADDRESS_STRING_SIZE];
        otIp6AddressToString(&dst_addr, string, sizeof(string));
        info("[Networking] >> accepted (%s)\r\n", string);
        mem_free(payload);
    }
    else
    {
        net_mgm_printf("net_mgm_node_accepted_send alloc fail \n");
    }
}

void net_mgm_node_reset_send(bool is_need_erase)
{
    uint8_t *payload = NULL;
    uint16_t payloadlength = 0;
    otError err;
    net_mgm_node_reset_t net_mgm_node_reset;

    otIp6Address multicast_addr;
    char multicast_ip[] = "ff03::1";
    err = otIp6AddressFromString(multicast_ip, &multicast_addr);

    net_mgm_node_reset.header = NET_MGM_NODE_RESET_HEADER;
    net_mgm_node_reset.need_erase = is_need_erase;
    payload = mem_malloc(sizeof(net_mgm_node_reset_t));
    if (payload)
    {
        nwk_node_data_piece(NET_MGM_NODE_RESET_HEADER, payload, &payloadlength, &net_mgm_node_reset);
        if (app_udp_send(multicast_addr, payload, payloadlength))
        {
            info("[error] net node reset send error \r\n");
        }
        char string[OT_IP6_ADDRESS_STRING_SIZE];
        otIp6AddressToString(&multicast_addr, string, sizeof(string));
        info("[Networking] >> reset (%s)\r\n", string);
        mem_free(payload);
    }
    else
    {
        net_mgm_printf("net_mgm_node_reset_send alloc fail \n");
    }
}

void app_udp_comm_net_mgm_node_ack_proc(uint8_t *data, uint16_t lens, otIp6Address src_addr)
{
    uint8_t *payload = NULL;
    uint16_t payloadlength = 0;
    net_mgm_node_ask_t net_mgm_node_ask;

    if (!nwk_node_data_parse(NET_MGM_NODE_ASK_HEADER, data, lens, &net_mgm_node_ask))
    {
        char string[OT_IP6_ADDRESS_STRING_SIZE];
        otIp6AddressToString(&src_addr, string, sizeof(string));
        info("[Networking] << polling ask (%s)\r\n", string);
        net_mgm_node_reply_send(src_addr);
    }
}

void app_udp_comm_net_mgm_node_reply_proc(uint8_t *data, uint16_t lens, otIp6Address src_addr)
{
    otDeviceRole mRole;
    otError    error           = OT_ERROR_NONE;
    otNeighborInfo  neighborInfo;
    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    static net_mgm_node_reply_t net_mgm_node_reply;
    uint16_t *ack_rloc = NULL;
    uint8_t *payload = NULL;
    uint16_t i = 0, k = 0, payloadlength;
    mRole = otThreadGetDeviceRole(otGetInstance());
    do
    {
        if (lens > 0)
        {
            if (mRole == OT_DEVICE_ROLE_LEADER)
            {
                if (nwk_node_data_parse(NET_MGM_NODE_REPLY_HEADER, data, lens, &net_mgm_node_reply))
                {
                    error = OT_ERROR_FAILED;
                    break;
                }
                char string[OT_IP6_ADDRESS_STRING_SIZE];
                otIp6AddressToString(&src_addr, string, sizeof(string));
                info("[Networking] << polling reply(%s)\r\n", string);

                net_mgm_printf("self %04x,%04x, %02X%02X%02X%02X%02X%02X%02X%02X \n",
                               net_mgm_node_reply.parent,
                               net_mgm_node_reply.self_rloc,
                               net_mgm_node_reply.self_extaddr[0],
                               net_mgm_node_reply.self_extaddr[1],
                               net_mgm_node_reply.self_extaddr[2],
                               net_mgm_node_reply.self_extaddr[3],
                               net_mgm_node_reply.self_extaddr[4],
                               net_mgm_node_reply.self_extaddr[5],
                               net_mgm_node_reply.self_extaddr[6],
                               net_mgm_node_reply.self_extaddr[7]);

                /*add my self*/
                int8_t self_rssi = (-128);
                while (otThreadGetNextNeighborInfo(otGetInstance(), &iterator, &neighborInfo) == OT_ERROR_NONE)
                {
                    if (neighborInfo.mRloc16 == net_mgm_node_reply.self_rloc)
                    {
                        self_rssi = neighborInfo.mLastRssi;
                        break;
                    }
                }
                if (net_mgm_node_table_add(OT_DEVICE_ROLE_ROUTER,
                                           net_mgm_node_reply.parent,
                                           net_mgm_node_reply.self_rloc,
                                           net_mgm_node_reply.self_extaddr, self_rssi, net_mgm_node_reply.version))
                {
                    net_mgm_printf("unexpected router add fail\n");
                }
                /*check child in nwk table*/
                if (net_mgm_node_table[i].used)
                {
                    for (i = 0; i < NET_MGM_NODE_TATLE_MAX_SIZE; i++)
                    {
                        if (net_mgm_node_table[i].parent == net_mgm_node_reply.self_rloc && net_mgm_node_table[i].role == OT_DEVICE_ROLE_CHILD)
                        {
                            /*if same parent but not in reply data need remove*/
                            for (k = 0; k < net_mgm_node_reply.num; k++)
                            {
                                if (memcmp(net_mgm_node_table[i].extaddr, net_mgm_node_reply.child_info[k].extaddr, OT_EXT_ADDRESS_SIZE) == 0)
                                {
                                    break;
                                }
                            }

                            if (k >= net_mgm_node_reply.num)
                            {
                                net_mgm_printf("repky rm %04x %04x %02X%02X%02X%02X%02X%02X%02X%02X\n",
                                               net_mgm_node_reply.self_rloc,
                                               net_mgm_node_table[i].rloc,
                                               net_mgm_node_table[i].extaddr[0],
                                               net_mgm_node_table[i].extaddr[1],
                                               net_mgm_node_table[i].extaddr[2],
                                               net_mgm_node_table[i].extaddr[3],
                                               net_mgm_node_table[i].extaddr[4],
                                               net_mgm_node_table[i].extaddr[5],
                                               net_mgm_node_table[i].extaddr[6],
                                               net_mgm_node_table[i].extaddr[7]);
                                net_mgm_node_table[i].role = OT_DEVICE_ROLE_DETACHED;
                                net_mgm_node_table[i].survivaltime = 0;
                            }
                        }
                    }
                }
                /*updata child table*/
                net_mgm_printf("rece %u \n", net_mgm_node_reply.num);
                for (i = 0; i < net_mgm_node_reply.num; i++)
                {
                    net_mgm_printf("child %04x,%04x, %02X%02X%02X%02X%02X%02X%02X%02X, %d \n",
                                   net_mgm_node_reply.self_rloc,
                                   net_mgm_node_reply.child_info[i].rloc,
                                   net_mgm_node_reply.child_info[i].extaddr[0],
                                   net_mgm_node_reply.child_info[i].extaddr[1],
                                   net_mgm_node_reply.child_info[i].extaddr[2],
                                   net_mgm_node_reply.child_info[i].extaddr[3],
                                   net_mgm_node_reply.child_info[i].extaddr[4],
                                   net_mgm_node_reply.child_info[i].extaddr[5],
                                   net_mgm_node_reply.child_info[i].extaddr[6],
                                   net_mgm_node_reply.child_info[i].extaddr[7],
                                   net_mgm_node_reply.child_info[i].rssi);
                    if (net_mgm_node_table_add(OT_DEVICE_ROLE_CHILD,
                                               net_mgm_node_reply.self_rloc,
                                               net_mgm_node_reply.child_info[i].rloc,
                                               net_mgm_node_reply.child_info[i].extaddr,
                                               net_mgm_node_reply.child_info[i].rssi,
                                               0xffffffff))
                    {
                        net_mgm_printf("unexpected child add fail\n");
                    }
                }
            }
            else
            {
                net_mgm_printf("isn't leader \n");
            }
        }
    } while (0);
}

void app_udp_comm_net_mgm_node_challenge_proc(uint8_t *data, uint16_t lens, otIp6Address src_addr)
{
    uint8_t *payload = NULL;
    uint16_t payloadlength = 0;
    net_mgm_node_challenge_t net_mgm_node_challenge;

    if (!nwk_node_data_parse(NET_MGM_NODE_CHALLENGE_HEADER, data, lens, &net_mgm_node_challenge))
    {
        char string[OT_IP6_ADDRESS_STRING_SIZE];
        otIp6AddressToString(&src_addr, string, sizeof(string));
        info("[Networking] << challenge(%s)\r\n", string);
        net_mgm_node_accepted_send(src_addr, net_mgm_node_table_add(net_mgm_node_challenge.role,
                                   net_mgm_node_challenge.parent,
                                   net_mgm_node_challenge.self_rloc,
                                   (uint8_t *)&net_mgm_node_challenge.self_extaddr,
                                   net_mgm_node_challenge.rssi,
                                   net_mgm_node_challenge.version));
    }
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

/*network management mapping router table*/
static void net_mgm_node_table_router_update()
{
    otError          error = OT_ERROR_NONE;
    otRouterInfo routerInfo;
    uint8_t maxRouterId = otThreadGetMaxRouterId(otGetInstance());
    uint16_t i = 0;
    if (net_mgm_node_table)
    {
        for (i = 0; i < NET_MGM_NODE_TATLE_MAX_SIZE; i++)
        {
            if (net_mgm_node_table[i].used && (net_mgm_node_table[i].role == OT_DEVICE_ROLE_ROUTER))
            {
                bool mapping = false;
                for (uint8_t j = 0; j <= maxRouterId; j++)
                {
                    if (otThreadGetRouterInfo(otGetInstance(), j, &routerInfo) == OT_ERROR_NONE)
                    {
                        if (memcmp(routerInfo.mExtAddress.m8, net_mgm_node_table[i].extaddr, OT_EXT_ADDRESS_SIZE) == 0)
                        {
                            mapping = true;
                        }
                    }
                }
                if (mapping == false)
                {
                    /*remove same parent child*/
                    for (uint16_t k = 0; k < NET_MGM_NODE_TATLE_MAX_SIZE; k++)
                    {
                        if (net_mgm_node_table[k].parent == net_mgm_node_table[i].rloc && net_mgm_node_table[k].role == OT_DEVICE_ROLE_CHILD)
                        {
                            net_mgm_printf("route mapping rm %04x %02X%02X%02X%02X%02X%02X%02X%02X\n",
                                           net_mgm_node_table[k].rloc,
                                           net_mgm_node_table[k].extaddr[0],
                                           net_mgm_node_table[k].extaddr[1],
                                           net_mgm_node_table[k].extaddr[2],
                                           net_mgm_node_table[k].extaddr[3],
                                           net_mgm_node_table[k].extaddr[4],
                                           net_mgm_node_table[k].extaddr[5],
                                           net_mgm_node_table[k].extaddr[6],
                                           net_mgm_node_table[k].extaddr[7]);
                            net_mgm_node_table[k].role = OT_DEVICE_ROLE_DETACHED;
                            net_mgm_node_table[k].survivaltime = 0;
                        }
                    }
                    net_mgm_node_table[i].role = OT_DEVICE_ROLE_DETACHED;
                    net_mgm_node_table[i].survivaltime = 0;
                }
            }
        }
    }
}

/*network management mapping child table*/
static void net_mgm_node_table_child_update()
{
    otError          error = OT_ERROR_NONE;
    otChildInfo      childInfo;
    uint16_t maxChildren;
    maxChildren = otThreadGetMaxAllowedChildren(otGetInstance());

    for (uint16_t i = 0; i < maxChildren; i++)
    {
        if ((otThreadGetChildInfoByIndex(otGetInstance(), i, &childInfo) != OT_ERROR_NONE) ||
                childInfo.mIsStateRestoring)
        {
            continue;
        }

        net_mgm_node_table_add(OT_DEVICE_ROLE_CHILD,
                               otThreadGetRloc16(otGetInstance()),
                               childInfo.mRloc16,
                               (uint8_t *)&childInfo.mExtAddress.m8,
                               childInfo.mAverageRssi,
                               0xffffffff);
    }
}

static void net_mgm_node_ask_router()
{
    uint8_t maxRouterId;
    otRouterInfo routerInfo;
    bool find_lash_router = false;
    maxRouterId = otThreadGetMaxRouterId(otGetInstance());
    uint8_t i = 0;
    for (i = 0; i <= maxRouterId; i++)
    {
        if (otThreadGetRouterInfo(otGetInstance(), i, &routerInfo) == OT_ERROR_NONE)
        {
            if (routerInfo.mRloc16 == otThreadGetRloc16(otGetInstance()))
            {
                net_mgm_node_table_child_update();
                continue;
            }

            if (0xff == last_router_id || find_lash_router == true)
            {
                /*Leader Device Process step 4*/
                net_mgm_ask_node_polling_timer = NET_MGM_ASK_NODE_NUM_POLLING_SHORT_TIME;
                /*ask router*/
                last_router_id = routerInfo.mRouterId;
                net_mgm_node_ask_send(routerInfo.mRloc16);
                break;
            }
            else if (routerInfo.mRouterId == last_router_id)
            {
                find_lash_router = true;
                continue;
            }
        }
    }
    if (i >= maxRouterId)
    {
        /*Leader Device Process step 5*/
        net_mgm_ask_node_polling_timer = NET_MGM_ASK_NODE_NUM_POLLING_LONG_TIME;
        last_router_id = 0xff;
        net_mgm_node_table_router_update();
    }
}

/*network management adk node polling time handle*/
static void net_mgm_ask_node_polling_time_handler()
{
    /*update router*/
    net_mgm_node_ask_router();

}

/*network management node table surival time handle*/
static void net_mgm_node_table_survive_time_handler()
{
    if (net_mgm_node_table)
    {
        for (uint16_t i = 0; i < NET_MGM_NODE_TATLE_MAX_SIZE; i++)
        {
            if (net_mgm_node_table[i].used)
            {
                if (net_mgm_node_table[i].survivaltime > 0 && --net_mgm_node_table[i].survivaltime == 0)
                {
                    net_mgm_node_table[i].role = OT_DEVICE_ROLE_DETACHED;
                    net_mgm_printf("timeout %s %04X %02X%02X%02X%02X%02X%02X%02X%02X \n",
                                   otThreadDeviceRoleToString(net_mgm_node_table[i].role),
                                   net_mgm_node_table[i].rloc,
                                   net_mgm_node_table[i].extaddr[0],
                                   net_mgm_node_table[i].extaddr[1],
                                   net_mgm_node_table[i].extaddr[2],
                                   net_mgm_node_table[i].extaddr[3],
                                   net_mgm_node_table[i].extaddr[4],
                                   net_mgm_node_table[i].extaddr[5],
                                   net_mgm_node_table[i].extaddr[6],
                                   net_mgm_node_table[i].extaddr[7]);
                    // memset(&net_mgm_node_table[i], 0x0, sizeof(net_mgm_node_table_t));
                }
            }
        }
    }
}

/*network management time*/
static void net_mgm_timer_handler(void *p_param)
{
    if (net_mgm_check_leader_pin_state() == true)
    {
        /*Leader Device Process step 2*/
        net_mgm_printf("polling %u \r\n", net_mgm_ask_node_polling_timer);
        if (net_mgm_ask_node_polling_timer > 0 && (--net_mgm_ask_node_polling_timer == 0))
        {
            /*Leader Device Process step 3*/
            net_mgm_ask_node_polling_time_handler();
        }
        net_mgm_node_table_survive_time_handler();
    }
    else
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
        if (changeRole != OT_DEVICE_ROLE_DISABLED)
        {
            if (net_mgm_check_leader_pin_state() == false)
            {
                if (changeRole == OT_DEVICE_ROLE_LEADER)
                {
                    memset(&ds, 0, sizeof(otOperationalDataset));
                    if (otDatasetSetActive(otGetInstance(), &ds) != OT_ERROR_NONE)
                    {
                        info("otDatasetSetActive fail \r\n");
                    }
                    if (otThreadBecomeDetached(otGetInstance()) != OT_ERROR_NONE)
                    {
                        info("state_change otThreadBecomeDetached fail \r\n");
                    }
                    return;
                }
            }
            else
            {
                if (changeRole != OT_DEVICE_ROLE_LEADER)
                {
                    memset(&ds, 0, sizeof(otOperationalDataset));
                    ds.mActiveTimestamp.mSeconds = 1;
                    ds.mComponents.mIsActiveTimestampPresent = true;
                    if (otDatasetSetActive(otGetInstance(), &ds) != OT_ERROR_NONE)
                    {
                        info("otDatasetSetActive fail \r\n");
                    }
                    if (otThreadBecomeLeader(otGetInstance()) != OT_ERROR_NONE)
                    {
                        info("otThreadBecomeLeader fail \r\n");
                        if (otThreadBecomeDetached(otGetInstance()) != OT_ERROR_NONE)
                        {
                            info("state_change otThreadBecomeDetached2 fail \r\n");
                        }
                    }
                    return;
                }
            }
        }
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

    if (net_mgm_check_leader_pin_state() == true && NULL == net_mgm_node_table)
    {
        net_mgm_node_table = mem_malloc(sizeof(net_mgm_node_table_t) * NET_MGM_NODE_TATLE_MAX_SIZE);
        if (net_mgm_node_table)
        {
            memset(net_mgm_node_table, 0x0, sizeof(net_mgm_node_table_t) * NET_MGM_NODE_TATLE_MAX_SIZE);
        }
        uint32_t partitionId = 0x0;
        ds_config_t ds_init_cfg;
        ds_rw_t t_ds_r, t_ds_w;
        uint8_t ds_buf[4] = {};
        uint32_t ds_ret = 0;
        ds_init_cfg.start_address = 0xF2000;
        ds_init_cfg.end_address = 0xF4000;
        ds_ret = ds_initinal(ds_init_cfg);

        if (ds_ret == STATUS_SUCCESS)
        {
            t_ds_r.type = 1;
            if (ds_read(&t_ds_r) == STATUS_SUCCESS)
            {
                for (uint8_t j = 0; j < sizeof(uint32_t); j++)
                {
                    ds_buf[j] = flash_read_byte(t_ds_r.address + j);
                }
                memcpy(&partitionId, ds_buf, sizeof(uint32_t));
            }
            t_ds_w.type = 1;
            t_ds_w.len = sizeof(uint32_t);
            partitionId++;
            memcpy(ds_buf, &partitionId, sizeof(uint32_t));
            t_ds_w.address = (uint32_t)ds_buf;
            if (ds_write(&t_ds_w) != STATUS_SUCCESS)
            {
                info("Record partition id fail %d\r\n", ds_ret);
            }
        }
        else
        {
            info("Record partition id false init fail %d\r\n", ds_ret);
        }

        otThreadSetPreferredLeaderPartitionId(otGetInstance(), partitionId);
        if (otThreadSetPreferredRouterId(otGetInstance(), 0) != OT_ERROR_NONE)
        {
            info("otThreadSetPreferredRouterId fail \r\n");
        }

        otThreadSetLocalLeaderWeight(otGetInstance(), 128);

        if (otThreadBecomeLeader(otGetInstance()) != OT_ERROR_NONE)
        {
            info("otThreadBecomeLeader fail \r\n");
        }
        /*Leader Device Process step 1*/
        net_mgm_ask_node_polling_timer = NET_MGM_ASK_NODE_NUM_POLLING_SHORT_TIME;
    }

    if (otSetStateChangedCallback(otGetInstance(), net_mgm_interface_state_change, 0) != OT_ERROR_NONE)
    {
        info("otSetStateChangedCallback fail \r\n");
    }
}
#endif