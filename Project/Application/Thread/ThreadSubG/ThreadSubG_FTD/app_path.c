/**
 * @file app_path.c
 * @author
 * @brief
 * @version 0.1
 * @date 2022-03-31
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "openthread-core-RT58x-config.h"
#include "main.h"
#include "sw_timer.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"

#define PATH_HOP_LIMIT 31

typedef struct
{
    uint8_t path_iid[8];
    uint32_t recv_time;
    uint16_t path_rloc16;
    int8_t recv_rssi;
} __attribute__((packed)) path_info_t;

typedef struct
{
    uint32_t header;
    uint8_t ttl;
    uint16_t src_rloc16;
    uint16_t dst_rloc16;
    uint8_t dst_iid[8];
    path_info_t path_info[PATH_HOP_LIMIT];
} __attribute__((packed)) path_request_t;

typedef struct
{
    uint32_t header;
    uint8_t ttl;
    uint16_t src_rloc16;
    uint16_t dst_rloc16;
    path_info_t path_info[PATH_HOP_LIMIT];
} __attribute__((packed)) path_respond_t;


static sw_timer_t *path_req_timer = NULL;
static volatile uint32_t path_req_curr_time = 0;

/*paht data use*/
int8_t path_find_rloc16_last_rssi(uint16_t rloc16)
{
    int8_t rssi = -128, parent_rssi;
    otNeighborInfo         neighborInfo;
    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;

    while (otThreadGetNextNeighborInfo(otGetInstance(), &iterator, &neighborInfo) == OT_ERROR_NONE)
    {
        if (neighborInfo.mRloc16 == rloc16)
        {
            rssi = neighborInfo.mLastRssi;
            break;
        }
    }

    if ((rssi == -128) && (otThreadGetParentLastRssi(otGetInstance(), &parent_rssi) == OT_ERROR_NONE))
    {
        rssi = parent_rssi;
    }
    return rssi;
}

uint16_t path_find_dst_ip_rloc16(otIp6Address dst_addr)
{
    uint16_t rloc16 = 0xffff;
    otCacheEntryIterator iter;
    otCacheEntryInfo     eid_entry;
    const otIp6Address *rloc_ip = otThreadGetRloc(otGetInstance());
    otNeighborInfo         neighborInfo;
    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;

    memset(&iter, 0, sizeof(iter));

    do
    {
        if (memcmp(&dst_addr.mFields.m8[8], &rloc_ip->mFields.m8[8], 6) == 0)
        {
            rloc16 = (dst_addr.mFields.m8[14] << 8) | dst_addr.mFields.m8[15];
            break;
        }

        while (otThreadGetNextNeighborInfo(otGetInstance(), &iterator, &neighborInfo) == OT_ERROR_NONE)
        {
            if (memcmp(neighborInfo.mExtAddress.m8, &dst_addr.mFields.m8[8], 8) == 0)
            {
                rloc16 = neighborInfo.mRloc16;
                break;
            }
        }

        while (OT_ERROR_NONE == otThreadGetNextCacheEntry(otGetInstance(), &eid_entry, &iter))
        {
            if (memcmp(&eid_entry.mTarget.mFields.m8, dst_addr.mFields.m8, OT_IP6_ADDRESS_SIZE) == 0)
            {
                rloc16 = eid_entry.mRloc16;
                break;
            }
        }

    } while (0);

    return rloc16;
}

/*paht data use*/
otError path_find_next_hop_ip(uint16_t dst_rloc16, otIp6Address *next_ip)
{
    otError error = OT_ERROR_NOT_FOUND;
    const otIp6Address *rloc_ip = otThreadGetRloc(otGetInstance());
    uint8_t r_max_id = 0;
    bool is_neighbor = false;
    otRouterInfo parentInfo;
    otRouterInfo r_info, n_info;
    otNeighborInfo         neighborInfo;
    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    otDeviceRole Role = otThreadGetDeviceRole(otGetInstance());
    do
    {
        memcpy(next_ip->mFields.m8, rloc_ip->mFields.m8, OT_IP6_ADDRESS_SIZE);

        if (Role == OT_DEVICE_ROLE_CHILD)
        {
            error = otThreadGetParentInfo(otGetInstance(), &parentInfo);
            next_ip->mFields.m8[14] = (parentInfo.mRloc16 & 0xff00) >> 8;
            next_ip->mFields.m8[15] = parentInfo.mRloc16 & 0x00ff;
            break;
        }

        if (!otThreadGetLinkMode(otGetInstance()).mDeviceType)
        {
            /*not ftd */
            otRouterInfo parent;
            if (!otThreadGetParentInfo(otGetInstance(), &parent))
            {
                next_ip->mFields.m8[14] = (parent.mRloc16 & 0xff00) >> 8;
                next_ip->mFields.m8[15] = parent.mRloc16 & 0x00ff;
                error = OT_ERROR_NONE;
            }
            break;
        }

        while (otThreadGetNextNeighborInfo(otGetInstance(), &iterator, &neighborInfo) == OT_ERROR_NONE)
        {
            if (neighborInfo.mRloc16 == dst_rloc16)
            {
                next_ip->mFields.m8[14] = (neighborInfo.mRloc16 & 0xff00) >> 8;
                next_ip->mFields.m8[15] = neighborInfo.mRloc16 & 0x00ff;
                error = OT_ERROR_NONE;
                is_neighbor = true;
                break;
            }
        }

        if (true == is_neighbor)
        {
            break;
        }

        r_max_id = otThreadGetMaxRouterId(otGetInstance());

        for (uint8_t i = 0; i <= r_max_id; i++)
        {
            if (otThreadGetRouterInfo(otGetInstance(), i, &r_info) == OT_ERROR_NONE)
            {
                if (r_info.mRloc16 == (dst_rloc16 & 0xFC00))
                {
                    if (r_info.mLinkEstablished != 1)
                    {
                        /*is not one hop*/
                        for (uint8_t k = 0; k <= r_max_id; k++)
                        {
                            if (otThreadGetRouterInfo(otGetInstance(), k, &n_info) == OT_ERROR_NONE)
                            {
                                if (r_info.mNextHop == n_info.mRouterId)
                                {
                                    next_ip->mFields.m8[14] = (n_info.mRloc16 & 0xff00) >> 8;
                                    next_ip->mFields.m8[15] = n_info.mRloc16 & 0x00ff;
                                    error = OT_ERROR_NONE;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        /*is one hop*/
                        next_ip->mFields.m8[14] = (r_info.mRloc16 & 0xff00) >> 8;
                        next_ip->mFields.m8[15] = r_info.mRloc16 & 0x00ff;
                    }
                    error = OT_ERROR_NONE;
                    break;
                }
            }
        }
    } while (0);

    return error;
}

static int path_req_udp_data_parse(uint8_t *payload, uint16_t payloadlength, void *data)
{
    path_request_t *path_req = NULL;
    uint8_t *tmp = payload;

    path_req = (path_request_t *)data;
    memcpy(&path_req->header, tmp, 4);
    tmp += 4;
    path_req->ttl = *tmp++;
    memcpy(&path_req->src_rloc16, tmp, 2);
    tmp += 2;
    memcpy(&path_req->dst_rloc16, tmp, 2);
    tmp += 2;
    memcpy(&path_req->dst_iid, tmp, 8);
    tmp += 8;
    memcpy(&path_req->path_info, tmp, ((PATH_HOP_LIMIT - path_req->ttl) * sizeof(path_info_t)));
    tmp += ((PATH_HOP_LIMIT - path_req->ttl) * sizeof(path_info_t));

    if ((tmp - payload) != payloadlength)
    {
        info("path req parse fail (%u/%u)\n", (tmp - payload), payloadlength);
        return 1;
    }
    return 0;
}

static int path_resp_udp_data_parse(uint8_t *payload, uint16_t payloadlength, void *data)
{
    path_respond_t *path_resp = NULL;
    uint8_t *tmp = payload;

    path_resp = (path_respond_t *)data;
    memcpy(&path_resp->header, tmp, 4);
    tmp += 4;
    path_resp->ttl = *tmp++;
    memcpy(&path_resp->src_rloc16, tmp, 2);
    tmp += 2;
    memcpy(&path_resp->dst_rloc16, tmp, 2);
    tmp += 2;
    memcpy(&path_resp->path_info, tmp, ((PATH_HOP_LIMIT - path_resp->ttl) * sizeof(path_info_t)));
    tmp += ((PATH_HOP_LIMIT - path_resp->ttl) * sizeof(path_info_t));

    if ((tmp - payload) != payloadlength)
    {
        info("path resp parse fail (%u/%u)\n", (tmp - payload), payloadlength);
        return 1;
    }
    return 0;
}

static void path_req_udp_data_piece(uint8_t *payload, uint16_t *payloadlength, void *buf)
{
    path_request_t *path_req = NULL;
    uint8_t *data = (uint8_t *)buf;
    uint8_t *tmp = payload;
    memcpy(tmp, data, 4);
    tmp += 4;
    path_req = (path_request_t *)data;
    *tmp++ = path_req->ttl;
    memcpy(tmp, &path_req->src_rloc16, 2);
    tmp += 2;
    memcpy(tmp, &path_req->dst_rloc16, 2);
    tmp += 2;
    memcpy(tmp, &path_req->dst_iid, 8);
    tmp += 8;
    memcpy(tmp, &path_req->path_info, ((PATH_HOP_LIMIT - path_req->ttl) * sizeof(path_info_t)));
    tmp += ((PATH_HOP_LIMIT - path_req->ttl) * sizeof(path_info_t));

    *payloadlength = (tmp - payload);
}

static void path_resp_udp_data_piece(uint8_t *payload, uint16_t *payloadlength, void *buf)
{
    path_respond_t *path_resp = NULL;
    uint8_t *data = (uint8_t *)buf;
    uint8_t *tmp = payload;
    memcpy(tmp, data, 4);
    tmp += 4;

    path_resp = (path_respond_t *)data;
    *tmp++ = path_resp->ttl;
    memcpy(tmp, &path_resp->src_rloc16, 2);
    tmp += 2;
    memcpy(tmp, &path_resp->dst_rloc16, 2);
    tmp += 2;
    memcpy(tmp, &path_resp->path_info, ((PATH_HOP_LIMIT - path_resp->ttl) * sizeof(path_info_t)));
    tmp += ((PATH_HOP_LIMIT - path_resp->ttl) * sizeof(path_info_t));

    *payloadlength = (tmp - payload);
}


/*path requeset data send*/
otError path_req_send(otIp6Address dst_addr, uint32_t timeout)
{
    otError error = OT_ERROR_NONE;
    uint8_t *r_data = NULL;
    uint16_t r_data_lens = 0;
    uint16_t next_rloc16;
    otIp6Address next_ip;
    const otMeshLocalPrefix *mesh_prefix = otThreadGetMeshLocalPrefix(otGetInstance());

    do
    {
        if (sw_timer_get_running(path_req_timer))
        {
            error = OT_ERROR_ALREADY;
            break;
        }

        r_data = mem_malloc(sizeof(path_request_t));
        if (r_data)
        {
            path_request_t path_req;

            path_req.header = PATH_REQUEST_HEADER;
            path_req.ttl = PATH_HOP_LIMIT;
            //src_rloc16
            path_req.src_rloc16 = otThreadGetRloc16(otGetInstance());
            //dst_rloc16
            path_req.dst_rloc16 = path_find_dst_ip_rloc16(dst_addr);
            //dst_iid
            memcpy(path_req.dst_iid, &dst_addr.mFields.m8[8], 8);
            if (path_req.dst_rloc16 == 0xffff)
            {
                info("[error] rloc not find \r\n");
                error = OT_ERROR_NOT_FOUND;
                break;
            }
            if (memcmp(mesh_prefix->m8, dst_addr.mFields.m8, (OT_IP6_ADDRESS_SIZE / 2)) == 0)
            {
                //is Mesh prefix ip
                if (!path_find_next_hop_ip(path_req.dst_rloc16, &next_ip))
                {
                    memcpy(dst_addr.mFields.m8, next_ip.mFields.m8, OT_IP6_ADDRESS_SIZE);
                }
                else
                {
                    info("[error] next hop not find \r\n");
                    error = OT_ERROR_NOT_FOUND;
                    break;
                }
            }
            else
            {
                //is LinkLocal prefix ip
            }
            path_req_udp_data_piece(r_data, &r_data_lens, &path_req);
            info("path req udp send \n");
            if (app_udp_send(dst_addr, r_data, r_data_lens) == OT_ERROR_NONE)
            {
                /*timeout hander start*/
                sw_timer_change_period(path_req_timer, timeout);
                if (!sw_timer_get_running(path_req_timer))
                {
                    sw_timer_start(path_req_timer);
                }
                /*record sender current time*/
                path_req_curr_time = sw_timer_get_tick();
            }
            else
            {
                info("path req udp send fail \r\n");
            }
        }
        else
        {
            info("r_data malloc fail \r\n");
        }
    } while (0);
    if (r_data)
    {
        mem_free(r_data);
    }
    return error;
}

/*path requeset data process*/
void app_udp_comm_path_req_proc(uint8_t *data, uint16_t lens)
{
    path_request_t path_req;
    path_respond_t path_resp;
    path_request_t new_path_req;
    otIp6Address next_ip;
    const otIp6Address *rloc_ip = otThreadGetRloc(otGetInstance());
    const otIp6Address *mesh_ip = otThreadGetMeshLocalEid(otGetInstance());
    const otMeshLocalPrefix *mesh_prefix = otThreadGetMeshLocalPrefix(otGetInstance());
    uint8_t *s_buf = NULL;
    uint16_t s_buf_lens = 0;
    do
    {
        if (lens > 0)
        {
            if (path_req_udp_data_parse(data, lens, &path_req))
            {
                break;
            }
            if (path_req.ttl > 0)
            {
                info("req [%d] %04X -> %04x \r\n", path_req.ttl, path_req.src_rloc16, path_req.dst_rloc16);
                if (otThreadGetRloc16(otGetInstance()) == path_req.src_rloc16)
                {
                    info("src ip is me .... \r\n");
                }
                else if (otThreadGetRloc16(otGetInstance()) == path_req.dst_rloc16)
                {
                    if (((path_req.dst_iid[6] << 8) | path_req.dst_iid[7]) == path_req.dst_rloc16 ||
                            memcmp(path_req.dst_iid, &mesh_ip->mFields.m8[8], 8) == 0)
                    {
                        /*do path respond*/
                        path_resp.header = PATH_RESPOND_HEADER;
                        //respond ttl
                        path_resp.ttl = path_req.ttl - 1;
                        //respond src_rloc16
                        path_resp.src_rloc16 = path_req.dst_rloc16;
                        //respond src_rloc16
                        path_resp.dst_rloc16 = path_req.src_rloc16;
                        //copy request path info
                        memcpy(path_resp.path_info, path_req.path_info, ((PATH_HOP_LIMIT - path_req.ttl)*sizeof(path_info_t)));
                        /*add src_iid*/
                        memcpy(path_resp.path_info[PATH_HOP_LIMIT - path_req.ttl].path_iid, &mesh_ip->mFields.m8[8], (OT_IP6_ADDRESS_SIZE / 2));
                        path_resp.path_info[PATH_HOP_LIMIT - path_req.ttl].recv_time = sw_timer_get_tick();
                        path_resp.path_info[PATH_HOP_LIMIT - path_req.ttl].path_rloc16 = otThreadGetRloc16(otGetInstance());
                        if (PATH_HOP_LIMIT == path_req.ttl)
                        {
                            path_resp.path_info[PATH_HOP_LIMIT - path_req.ttl].recv_rssi =
                                path_find_rloc16_last_rssi(path_req.src_rloc16);
                        }
                        else
                        {
                            path_resp.path_info[PATH_HOP_LIMIT - path_req.ttl].recv_rssi =
                                path_find_rloc16_last_rssi(path_resp.path_info[PATH_HOP_LIMIT - path_req.ttl - 1].path_rloc16);
                        }

                        s_buf = mem_malloc(sizeof(path_respond_t));
                        if (s_buf)
                        {
                            path_resp_udp_data_piece(s_buf, &s_buf_lens, &path_resp);

                            if (!path_find_next_hop_ip(path_resp.dst_rloc16, &next_ip))
                            {
                                if (app_udp_send(next_ip, s_buf, s_buf_lens))
                                {
                                    info("[error] path respond send fail \r\n");
                                }
                            }
                            else
                            {
                                info("[error] path respond route not find \r\n");
                            }
                            mem_free(s_buf);
                        }
                        else
                        {
                            info("[error] path respond forwarder to child malloc fail \r\n");
                        }
                    }
                    else
                    {
                        /*is my child*/
                        uint16_t  max_child = otThreadGetMaxAllowedChildren(otGetInstance());
                        otChildInfo      child_info;
                        for (uint16_t i = 0; i < max_child; i++)
                        {
                            if (otThreadGetChildInfoByIndex(otGetInstance(), i, &child_info) == OT_ERROR_NONE)
                            {
                                if (memcmp(child_info.mExtAddress.m8, path_req.dst_iid, 8) == 0)
                                {
                                    new_path_req.header = PATH_REQUEST_HEADER;
                                    new_path_req.src_rloc16 = path_req.src_rloc16;
                                    memcpy(new_path_req.dst_iid, path_req.dst_iid, 8);
                                    new_path_req.ttl = path_req.ttl - 1;
                                    new_path_req.dst_rloc16 = child_info.mRloc16;
                                    memcpy(&new_path_req.path_info, &path_req.path_info, (PATH_HOP_LIMIT - path_req.ttl)*sizeof(path_info_t));
                                    memcpy(new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl].path_iid, &mesh_ip->mFields.m8[8], (OT_IP6_ADDRESS_SIZE / 2));
                                    new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl].recv_time = sw_timer_get_tick();
                                    new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl].path_rloc16 = otThreadGetRloc16(otGetInstance());
                                    if (PATH_HOP_LIMIT == path_req.ttl)
                                    {
                                        new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl].recv_rssi =
                                            path_find_rloc16_last_rssi(new_path_req.src_rloc16);
                                    }
                                    else
                                    {
                                        new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl].recv_rssi =
                                            path_find_rloc16_last_rssi(new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl - 1].path_rloc16);
                                    }
                                    s_buf = mem_malloc(sizeof(path_request_t));
                                    if (s_buf)
                                    {
                                        path_req_udp_data_piece(s_buf, &s_buf_lens, &new_path_req);

                                        memcpy(next_ip.mFields.m8, rloc_ip->mFields.m8, OT_IP6_ADDRESS_SIZE);
                                        next_ip.mFields.m8[14] = (child_info.mRloc16 & 0xff00) >> 8;
                                        next_ip.mFields.m8[15] = child_info.mRloc16 & 0x00ff;
                                        if (app_udp_send(next_ip, s_buf, s_buf_lens))
                                        {
                                            info("[error] ptah request forwarder to child error \r\n");
                                        }
                                        mem_free(s_buf);
                                    }
                                    else
                                    {
                                        info("[error] path request forwarder to child malloc fail \r\n");
                                    }
                                }
                            }  //if (otThreadGetChildInfoByIndex(otGetInstance(), i, &child_info)
                        }  // for (uint16_t i = 0; i < max_child; i++)
                    }
                }  //if(otThreadGetRloc16(otGetInstance()) == path_req->dst_rloc16)
                else
                {
                    if (!path_find_next_hop_ip(path_req.dst_rloc16, &next_ip))
                    {
                        new_path_req.header = PATH_REQUEST_HEADER;
                        new_path_req.src_rloc16 = path_req.src_rloc16;
                        new_path_req.dst_rloc16 = path_req.dst_rloc16;
                        memcpy(new_path_req.dst_iid, path_req.dst_iid, 8);
                        new_path_req.ttl = path_req.ttl - 1;
                        memcpy(&new_path_req.path_info, &path_req.path_info, (PATH_HOP_LIMIT - path_req.ttl)*sizeof(path_info_t));
                        memcpy(new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl].path_iid, &mesh_ip->mFields.m8[8], (OT_IP6_ADDRESS_SIZE / 2));
                        new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl].recv_time = sw_timer_get_tick();
                        new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl].path_rloc16 = otThreadGetRloc16(otGetInstance());
                        if (PATH_HOP_LIMIT == path_req.ttl)
                        {
                            new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl].recv_rssi =
                                path_find_rloc16_last_rssi(new_path_req.src_rloc16);
                        }
                        else
                        {
                            new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl].recv_rssi =
                                path_find_rloc16_last_rssi(new_path_req.path_info[PATH_HOP_LIMIT - path_req.ttl - 1].path_rloc16);
                        }

                        s_buf = mem_malloc(sizeof(path_request_t));
                        if (s_buf)
                        {
                            path_req_udp_data_piece(s_buf, &s_buf_lens, &new_path_req);

                            if (app_udp_send(next_ip, s_buf, s_buf_lens))
                            {
                                info("[error] ptah request forwarder to child error \r\n");
                            }
                            mem_free(s_buf);
                        }
                        else
                        {
                            info("[error] path request forwarder malloc fail \r\n");
                        }
                    }
                    else
                    {
                        info("[error] path request not find \r\n");
                    }
                }
            }  //if(path_req->ttl > 0)
            else
            {
                info("[error] path hop remain %d \r\n", path_req.ttl);
            }
        }
    } while (0);
}

void app_udp_comm_path_resp_proc(uint8_t *data, uint16_t lens)
{
    path_respond_t path_resp;
    path_respond_t new_path_resp;
    otIp6Address next_ip;
    const otIp6Address *rloc_ip = otThreadGetRloc(otGetInstance());
    const otIp6Address *mesh_ip = otThreadGetMeshLocalEid(otGetInstance());
    const otMeshLocalPrefix *mesh_prefix = otThreadGetMeshLocalPrefix(otGetInstance());
    uint8_t *s_buf = NULL;
    uint16_t s_buf_lens = 0;

    do
    {
        if (path_resp_udp_data_parse(data, lens, &path_resp))
        {
            break;
        }
        uint8_t path_cost[(PATH_HOP_LIMIT / 2) + 1];
        if (path_resp.ttl > 0)
        {
            info("resp [%d] %04X -> %04x \r\n", path_resp.ttl, path_resp.src_rloc16, path_resp.dst_rloc16);
            if (otThreadGetRloc16(otGetInstance()) == path_resp.src_rloc16)
            {
                info("[error] resp src ip is me .... \r\n");
            }
            else if (otThreadGetRloc16(otGetInstance()) == path_resp.dst_rloc16)
            {
                info("rtt : %lu ms\r\n", (sw_timer_get_tick() - path_req_curr_time));
                path_cost[0] = (sw_timer_get_tick() - path_req_curr_time);
                info("ttl : %u (max %u)\r\n", path_resp.ttl, PATH_HOP_LIMIT);
                info("[s]-%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X (%04X) \r\n",
                     mesh_ip->mFields.m8[0], mesh_ip->mFields.m8[1], mesh_ip->mFields.m8[2], mesh_ip->mFields.m8[3], \
                     mesh_ip->mFields.m8[4], mesh_ip->mFields.m8[5], mesh_ip->mFields.m8[6], mesh_ip->mFields.m8[7], \
                     mesh_ip->mFields.m8[8], mesh_ip->mFields.m8[9], mesh_ip->mFields.m8[10], mesh_ip->mFields.m8[11], \
                     mesh_ip->mFields.m8[12], mesh_ip->mFields.m8[13], mesh_ip->mFields.m8[14], mesh_ip->mFields.m8[15], \
                     otThreadGetRloc16(otGetInstance()));
                for (int i = 0; i < PATH_HOP_LIMIT - path_resp.ttl; i++)
                {
                    info("[%u]-%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X (%04X) [%d]\r\n", i, \
                         mesh_prefix->m8[0], mesh_prefix->m8[1], mesh_prefix->m8[2], mesh_prefix->m8[3], \
                         mesh_prefix->m8[4], mesh_prefix->m8[5], mesh_prefix->m8[6], mesh_prefix->m8[7], \
                         path_resp.path_info[i].path_iid[0], path_resp.path_info[i].path_iid[1], \
                         path_resp.path_info[i].path_iid[2], path_resp.path_info[i].path_iid[3], \
                         path_resp.path_info[i].path_iid[4], path_resp.path_info[i].path_iid[5], \
                         path_resp.path_info[i].path_iid[6], path_resp.path_info[i].path_iid[7], \
                         path_resp.path_info[i].path_rloc16, path_resp.path_info[i].recv_rssi);
                }
                info("[e]-%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X (%04X) [%d]\r\n",
                     mesh_ip->mFields.m8[0], mesh_ip->mFields.m8[1], mesh_ip->mFields.m8[2], mesh_ip->mFields.m8[3], \
                     mesh_ip->mFields.m8[4], mesh_ip->mFields.m8[5], mesh_ip->mFields.m8[6], mesh_ip->mFields.m8[7], \
                     mesh_ip->mFields.m8[8], mesh_ip->mFields.m8[9], mesh_ip->mFields.m8[10], mesh_ip->mFields.m8[11], \
                     mesh_ip->mFields.m8[12], mesh_ip->mFields.m8[13], mesh_ip->mFields.m8[14], mesh_ip->mFields.m8[15], \
                     otThreadGetRloc16(otGetInstance()), path_find_rloc16_last_rssi(path_resp.path_info[PATH_HOP_LIMIT - path_resp.ttl - 1].path_rloc16));
                for (int i = 0; i < ((PATH_HOP_LIMIT - path_resp.ttl) / 2); i++)
                {
                    for (int j = (PATH_HOP_LIMIT - path_resp.ttl); j > ((PATH_HOP_LIMIT - path_resp.ttl) / 2) ; j--)
                    {
                        if (memcmp(path_resp.path_info[i].path_iid, path_resp.path_info[j].path_iid, 8) == 0)
                        {
                            path_cost[i + 1] = (path_resp.path_info[j].recv_time - path_resp.path_info[i].recv_time);
                            break;
                        }
                    }
                }
                info("[s]->[0], RTT : %u ms\r\n", ((path_cost[0] - path_cost[1]) / 2));

                for (int i = 0; i < ((PATH_HOP_LIMIT - path_resp.ttl) / 2); i++)
                {
                    //     info("[%u]->[%u], RTT : %u ms\r\n", (i+1) ,(i+2) , ((path_cost[i+1]-path_cost[i+2])/2));
                    if (i == (((PATH_HOP_LIMIT - path_resp.ttl) / 2) - 1))
                    {
                        info("[%u]->[%u], RTT : %u ms\r\n", i, (i + 1), (path_cost[i + 1] / 2));
                    }
                    else
                    {
                        info("[%u]->[%u], RTT : %u ms\r\n", i, (i + 1), ((path_cost[i + 1] - path_cost[i + 2]) / 2));
                    }
                }

                if (sw_timer_get_running(path_req_timer))
                {
                    sw_timer_stop(path_req_timer);
                    path_req_curr_time = 0;
                }
            }
            else
            {
                if (!path_find_next_hop_ip(path_resp.dst_rloc16, &next_ip))
                {
                    /*do path respond*/
                    new_path_resp.header = PATH_RESPOND_HEADER;
                    //respond ttl
                    new_path_resp.ttl = path_resp.ttl - 1;
                    //respond src_rloc16
                    new_path_resp.src_rloc16 = path_resp.src_rloc16;
                    //respond src_rloc16
                    new_path_resp.dst_rloc16 = path_resp.dst_rloc16;
                    //copy request path info
                    memcpy(new_path_resp.path_info, path_resp.path_info, ((PATH_HOP_LIMIT - path_resp.ttl)*sizeof(path_info_t)));
                    /*add src_iid*/
                    memcpy(new_path_resp.path_info[PATH_HOP_LIMIT - path_resp.ttl].path_iid, &mesh_ip->mFields.m8[8], (OT_IP6_ADDRESS_SIZE / 2));
                    new_path_resp.path_info[PATH_HOP_LIMIT - path_resp.ttl].recv_time = sw_timer_get_tick();
                    new_path_resp.path_info[PATH_HOP_LIMIT - path_resp.ttl].path_rloc16 = otThreadGetRloc16(otGetInstance());
                    if (PATH_HOP_LIMIT == path_resp.ttl)
                    {
                        new_path_resp.path_info[PATH_HOP_LIMIT - path_resp.ttl].recv_rssi =
                            path_find_rloc16_last_rssi(path_resp.dst_rloc16);
                    }
                    else
                    {
                        new_path_resp.path_info[PATH_HOP_LIMIT - path_resp.ttl].recv_rssi =
                            path_find_rloc16_last_rssi(path_resp.path_info[PATH_HOP_LIMIT - path_resp.ttl - 1].path_rloc16);
                    }
                    s_buf = mem_malloc(sizeof(path_respond_t));
                    if (s_buf)
                    {
                        path_resp_udp_data_piece(s_buf, &s_buf_lens, &new_path_resp);

                        if (!path_find_next_hop_ip(path_resp.dst_rloc16, &next_ip))
                        {
                            if (app_udp_send(next_ip, s_buf, s_buf_lens))
                            {
                                info("[error] path respond send fail \r\n");
                            }
                        }
                        else
                        {
                            info("[error] path respond route not find \r\n");
                        }
                        mem_free(s_buf);
                    }
                    else
                    {
                        info("[error] path resp forwarder malloc fail \r\n");
                    }
                }
                else
                {
                    info("[error] path resp route not find \r\n");
                }
            }  //if(otThreadGetRloc16(otGetInstance()) == path_resp->src_rloc16)
        } // if(path_resp->ttl > 0)
        else
        {
            info("[error] path resp hop remain %d \r\n", path_resp.ttl);
        }
    } while (0);
}

/*path request & respnse time*/
static void app_udp_comm_req_timer_handler()
{
    info("Path Request timeout \r\n");
    path_req_curr_time = 0;
}


void path_init()
{
    /*path request & respnse*/
    path_req_timer = sw_timer_create("path_req_timer",
                                     SW_TIMER_MAX_PERIOD,
                                     false,
                                     SW_TIMER_EXECUTE_ONCE_FOR_EACH_TIMEOUT,
                                     NULL,
                                     app_udp_comm_req_timer_handler);
}