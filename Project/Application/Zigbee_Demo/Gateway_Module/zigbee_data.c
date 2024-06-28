/**
 * Copyright (c) 2021 All Rights Reserved.
 */
/** @file zigbee_data.c
 *
 * @author Rex
 * @version 0.1
 * @date 2021/12/09
 * @license
 * @description
 */

//=============================================================================
//                Include
//=============================================================================
#include "sys_arch.h"
#include "zigbee_stack_api.h"
//=============================================================================
//                Private Global Variables
//=============================================================================
#define GW_IN_CLUSTER_NUM           4
#define GW_OUT_CLUSTER_NUM          14

#define GW_DEVICE_VER               0
#define GW_DEVICE_ID                0

/* Basic cluster attributes */
static uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

/* OTA Upgrade server cluster attributes */
static uint8_t query_jitter = 0x64;
static uint32_t current_time = 0x00000000;

/*! Define a default global trust center link key */
uint8_t ZB_STANDARD_TC_KEY[] = {0x5A, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6C, 0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0x30, 0x39 };
uint8_t install_code_key[18] = {0x83, 0xFE, 0xD3, 0x40, 0x7A, 0x93, 0x97, 0x23, 0xA5, 0xC6, 0x39, 0xB2, 0x69, 0x16, 0xD5, 0x05, 0xC3, 0xB5};
/* Identify cluster attributes */
static uint16_t attr_identify_time = 0;


/* Custom cluster attributes */
zb_uint8_t custom_dst_endpoint = 0xFF;
zb_uint16_t custom_dst_short_addr = 0xFFFF;
zb_uint8_t g_attr_u8 = ZB_ZCL_CUSTOM_CLUSTER_ATTR_U8_DEFAULT_VALUE;
zb_int16_t g_attr_s16 = ZB_ZCL_CUSTOM_CLUSTER_ATTR_S16_DEFAULT_VALUE;
zb_uint24_t g_attr_24bit = ZB_ZCL_CUSTOM_CLUSTER_ATTR_24BIT_DEFAULT_VALUE;
zb_uint32_t g_attr_32bitmap = ZB_ZCL_CUSTOM_CLUSTER_ATTR_32BITMAP_DEFAULT_VALUE;
zb_ieee_addr_t g_attr_ieee = ZB_ZCL_CUSTOM_CLUSTER_ATTR_IEEE_DEFAULT_VALUE;
zb_char_t g_attr_char_string[ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_MAX_SIZE] =
    ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_DEFAULT_VALUE;
zb_time_t g_attr_utc_time = ZB_ZCL_CUSTOM_CLUSTER_ATTR_UTC_TIME_DEFAULT_VALUE;
zb_uint8_t g_attr_byte_array[ZB_ZCL_CUSTOM_CLUSTER_ATTR_BYTE_ARRAY_MAX_SIZE] =
    ZB_ZCL_CUSTOM_CLUSTER_ATTR_BYTE_ARRAY_DEFAULT_VALUE;
zb_bool_t g_attr_bool = ZB_ZCL_CUSTOM_CLUSTER_ATTR_BOOL_DEFAULT_VALUE;
zb_uint8_t g_attr_128_bit_key[16] = ZB_ZCL_CUSTOM_CLUSTER_ATTR_128_BIT_KEY_DEFAULT_VALUE;
//=============================================================================
//                Global Variables
//=============================================================================

//=============================================================================
//                Attribute definitions
//=============================================================================
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(
    basic_attr_list,
    &attr_zcl_version,
    &attr_power_source);

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(
    identify_attr_list,
    &attr_identify_time);

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST_SERVER(
    ota_upgrade_attr_list,
    &query_jitter,
    &current_time,
    1);

ZB_ZCL_DECLARE_CUSTOM_ATTR_CLUSTER_ATTRIB_LIST(custom_attr_list,
        &g_attr_u8,
        &g_attr_s16,
        &g_attr_24bit,
        &g_attr_32bitmap,
        g_attr_ieee,
        g_attr_char_string,
        &g_attr_utc_time,
        g_attr_byte_array,
        &g_attr_bool,
        g_attr_128_bit_key);
//=============================================================================
//                Cluster definitions
//=============================================================================
zb_zcl_cluster_desc_t g_zigbee_cluster_list[] =
{
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_OTA_UPGRADE,
        ZB_ZCL_ARRAY_SIZE(ota_upgrade_attr_list, zb_zcl_attr_t),
        (ota_upgrade_attr_list),
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_BASIC,
        ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t),
        (basic_attr_list),
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_IDENTIFY,
        ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t),
        (identify_attr_list),
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_CUSTOM,
        ZB_ZCL_ARRAY_SIZE(custom_attr_list, zb_zcl_attr_t),
        (custom_attr_list),
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),

    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_BASIC,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_ON_OFF,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_SCENES,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_GROUPS,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_IAS_ZONE,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_METERING,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_ELECTRICAL_MEASUREMENT,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_CUSTOM,
        ZB_ZCL_ARRAY_SIZE(custom_attr_list, zb_zcl_attr_t),
        (custom_attr_list),
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_IDENTIFY,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    ),
    ZB_ZCL_CLUSTER_DESC(
        ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
        0,
        NULL,
        ZB_ZCL_CLUSTER_CLIENT_ROLE,
        ZB_ZCL_MANUF_CODE_INVALID
    )
};
//=============================================================================
//                Simple desc definitions
//=============================================================================
ZB_DECLARE_SIMPLE_DESC(4, 14);

ZB_AF_SIMPLE_DESC_TYPE(GW_IN_CLUSTER_NUM, GW_OUT_CLUSTER_NUM) simple_desc_gateway =
{
    2,                                  /* Endpoint */
    ZB_AF_HA_PROFILE_ID,                /* Application profile identifier */
    GW_DEVICE_ID,                       /* Application device identifier */
    GW_DEVICE_VER,                      /* Application device version */
    0,                                  /* Reserved */
    GW_IN_CLUSTER_NUM,                  /* Application input cluster count */
    GW_OUT_CLUSTER_NUM,                 /* Application output cluster count */
    /* Application input and output cluster list */
    {
        ZB_ZCL_CLUSTER_ID_OTA_UPGRADE,
        ZB_ZCL_CLUSTER_ID_BASIC,
        ZB_ZCL_CLUSTER_ID_IDENTIFY,
        ZB_ZCL_CLUSTER_ID_CUSTOM,
        ZB_ZCL_CLUSTER_ID_BASIC,
        ZB_ZCL_CLUSTER_ID_ON_OFF,
        ZB_ZCL_CLUSTER_ID_SCENES,
        ZB_ZCL_CLUSTER_ID_GROUPS,
        ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
        ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
        ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
        ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
        ZB_ZCL_CLUSTER_ID_IAS_ZONE,
        ZB_ZCL_CLUSTER_ID_METERING,
        ZB_ZCL_CLUSTER_ID_ELECTRICAL_MEASUREMENT,
        ZB_ZCL_CLUSTER_ID_CUSTOM,
        ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
        ZB_ZCL_CLUSTER_ID_IDENTIFY,
    }
};

ZB_AF_DECLARE_ENDPOINT_DESC(
    simple_desc_gateway_ep,
    2,
    ZB_AF_HA_PROFILE_ID,
    0,
    NULL,
    ZB_ZCL_ARRAY_SIZE(g_zigbee_cluster_list, zb_zcl_cluster_desc_t),
    g_zigbee_cluster_list,
    (zb_af_simple_desc_1_1_t *)&simple_desc_gateway,
    0,
    NULL,
    0,
    NULL);

ZBOSS_DECLARE_DEVICE_CTX_1_EP(simple_desc_gateway_ctx, simple_desc_gateway_ep);
