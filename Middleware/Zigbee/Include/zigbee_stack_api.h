// ---------------------------------------------------------------------------
//
// ---------------------------------------------------------------------------
#ifndef __ZIGBEE_STACK_API_H__
#define __ZIGBEE_STACK_API_H__

/**
*@defgroup ZigBee
*@brief Define the Zigbee stack definitions, structures, arrtibutes and functions
*/

/**
*@defgroup ZCL
*@brief Define the ZCL definitions, structures, arrtibutes and
*functions @{
*@ingroup ZigBee
*@}
*/

/**
*@defgroup ZigBee_Library
*@brief Define the Rafael ZigBee library definitions, structures, arrtibutes and functions
*@{
*@ingroup ZigBee
*@}
*/

/**
*@defgroup ZigBee_Stack
*@brief Define the Rafael ZigBee stack definitions, structures, arrtibutes and functions
*@{
*@ingroup ZigBee
*@}
*/

/**
*@defgroup ZB_ZCL_COMMANDS
*@brief Define the zcl commands definitions, structures, arrtibutes and functions
*@{
*@ingroup ZCL
*@}
*/

/**
*@defgroup ZB_ZCL_COMMON
*@brief Define the zcl common related definitions, structures, arrtibutes and functions
*@{
*@ingroup ZCL
*@}
*/

/**
*@defgroup ZB_ZCL_BASIC
*@brief Define the Basic cluster definitions, structures, arrtibutes and functions
*@{
*@ingroup ZCL
*@}
*/


/**
*@defgroup ZB_ZCL_COLOR_CONTROL
*@brief Define the color control cluster definitions, structures, arrtibutes and functions
*@{
*@ingroup ZCL
*@}
*/

/**
*@defgroup ZB_ZCL_GROUPS
*@brief Define the group cluster definitions, structures, arrtibutes and functions
*@{
*@ingroup ZCL
*@}
*/

/**
*@defgroup ZB_ZCL_IAS_ZONE
*@brief Define the ias zone cluster definitions, structures, arrtibutes and functions
*@{
*@ingroup ZCL
*@}
*/

/**
*@defgroup ZB_ZCL_IDENTIFY
*@brief Define the identify cluster definitions, structures, arrtibutes and functions
*@{
*@ingroup ZCL
*@}
*/

/**
*@defgroup ZB_ZCL_LEVEL_CONTROL
*@brief Define the level control cluster definitions, structures, arrtibutes and functions
*@{
*@ingroup ZCL
*@}
*/

/**
*@defgroup ZB_ZCL_ON_OFF
*@brief Define the on off cluster definitions, structures, arrtibutes and functions
*@{
*@ingroup ZCL
*@}
*/

/**
*@defgroup ZB_ZCL_SCENES
*@brief Define the scenes cluster definitions, structures, arrtibutes and functions
*@{
*@ingroup ZCL
*@}
*/
#ifdef __cplusplus
extern "C"
{
#endif

//=============================================================================
//                Include (Better to prevent)
//=============================================================================
#include "sys_arch.h"
#ifdef ZB_LIB
#include "zboss_api.h"
#else
#include "zb_config.h"
#include "zb_types.h"
#include "zboss_api_buf.h"
#include "zboss_api_nwk.h"
#include "zboss_api_af.h"
#include "zboss_api_aps.h"
#include "zboss_api_core.h"
#include "zboss_api_zcl.h"
#include "zigbee_app.h"
#include "zb_zcl_common.h"
#include "zb_zcl_basic.h"
#include "zb_zcl_on_off.h"
#include "zb_zcl_level_control.h"
#include "zb_zcl_color_control.h"
#include "zb_zcl_identify.h"
#include "zb_zcl_groups.h"
#include "zb_zcl_reporting.h"
#include "zb_zcl_scenes.h"
#include "zb_zcl_thermostat.h"
#include "zb_zcl_ias_zone.h"
#include "zb_zcl_cvc_commands.h"
#include "zb_zcl_ota_upgrade.h"
#include "zcl_ota_upgrade_commands.h"
#include "zb_zcl_custom_cluster.h"
#include "zb_zcl_metering.h"
#include "zb_zcl_power_config.h"
#include "zb_zcl_temp_measurement.h"
#include "zb_zcl_el_measurement.h"
#include "zb_zcl_illuminance_measurement.h"
#include "zb_zcl_rel_humidity_measurement.h"
#include "zb_zcl_door_lock.h"
#include "zb_zcl_time.h"
#endif
/** @addtogroup ZigBee_Stack
 *  @{
 */

//=============================================================================
//                Public Definitions of const value
//=============================================================================
#define ZIGBEE_CHANNEL_MASK(n)  (1l << n)
#define ZIGBEE_CHANNEL_ALL_MASK() (0x7FFF800l)

#define CHECK_RESET_START_ADDRESS 0xF2000
#define CHECK_RESET_BLOCK_SIZE 2048

typedef void(pf_evt_indication)(uint32_t data_len);

#define ZIGBEE_DEFAULT_ENDPOINT                     0x02

#define ZIGBEE_DEVICE_ROLE_CORDINATOR               0x00
#define ZIGBEE_DEVICE_ROLE_ROUTER                   0x01
#define ZIGBEE_DEVICE_ROLE_ENDDEVICE                0x02

/** HA profile ID*/
#define HA_PROFILE_ID                               0x0104


/*** Frame Client/Server Directions ***/
#define ZCL_FRAME_CLIENT_SERVER_DIR                     0x00
#define ZCL_FRAME_SERVER_CLIENT_DIR                     0x01

/** Macro to construct the zcl data request */
#define ZIGBEE_ZCL_DATA_REQ(data_req_name, daddr, daddrmode, dep, sep, cid, c, sp, disdefrsp, dir, mcode, len) \
    do { \
    data_req_name = sys_malloc(sizeof(zigbee_zcl_data_req_t) + len); \
    if(data_req_name == NULL) \
        break; \
    data_req_name->dstAddr = daddr; \
    data_req_name->addrmode = daddrmode; \
    data_req_name->dstEndpint = dep; \
    data_req_name->srcEndPoint = sep; \
    data_req_name->clusterID = cid; \
    data_req_name->cmd = c; \
    data_req_name->specific = sp; \
    data_req_name->disableDefaultRsp = disdefrsp; \
    data_req_name->direction = dir; \
    data_req_name->manuCode = mcode; \
    data_req_name->cmdFormatLen = len; } while(0); \

/** Macro to construct the zcl attr rw request */
#define ZIGBEE_ZCL_ATTR_RW_REQ(data_req_name, daddr, dep, sep, cid, aid, at, len) \
    do { \
    data_req_name = sys_malloc(sizeof(zigbee_zcl_attr_rw_t) + len); \
    if(data_req_name == NULL) \
        break; \
    data_req_name->dstShortAddr = daddr; \
    data_req_name->dstEndPoint = dep; \
    data_req_name->srcEndPoint = sep; \
    data_req_name->clusterID = cid; \
    data_req_name->attrID = aid; \
    data_req_name->attrType = at; \
    data_req_name->attrLen = len; } while(0); \

/** Macro to construct the zcl attribute set */
#define ZIGBEE_ZCL_ATTR_SET(zcl_attr_set, endpoint, clusterid, clusterrole, attrid, checkaccess, len) \
    do { \
    zcl_attr_set = sys_malloc(sizeof(zigbee_zcl_attr_set_t) + len); \
    if(zcl_attr_set == NULL) \
        break; \
    zcl_attr_set->ep = endpoint; \
    zcl_attr_set->cluster_id = clusterid; \
    zcl_attr_set->cluster_role = clusterrole; \
    zcl_attr_set->attr_id = attrid; \
    zcl_attr_set->check_access = checkaccess; \
    } while(0); \

//=============================================================================
//                Public ENUM
//=============================================================================
/** @brief HA Device identifiers. */
enum
{
    /*! General On/Off switch */
    HA_ON_OFF_SWITCH_DEVICE_ID = 0x0000,
    /*! Level Control Switch  */
    HA_LEVEL_CONTROL_SWITCH_DEVICE_ID = 0x0001,
    /*! General On/Off output */
    HA_ON_OFF_OUTPUT_DEVICE_ID = 0x0002,
    /*! Level Controllable Output */
    HA_LEVEL_CONTROLLABLE_OUTPUT_DEVICE_ID = 0x0003,
    /*! Scene Selector */
    HA_SCENE_SELECTOR_DEVICE_ID = 0x0004,
    /*! Configuration Tool */
    HA_CONFIGURATION_TOOL_DEVICE_ID = 0x0005,
    /*! Remote Control */
    HA_REMOTE_CONTROL_DEVICE_ID = 0x0006,
    /*! Combined Interface */
    HA_COMBINED_INTERFACE_DEVICE_ID = 0x0007,
    /*! Range Extender */
    HA_RANGE_EXTENDER_DEVICE_ID = 0x0008,
    /*! Mains Power Outlet */
    HA_MAINS_POWER_OUTLET_DEVICE_ID = 0x0009,
    /*! Door lock client */
    HA_DOOR_LOCK_DEVICE_ID = 0x000A,
    /*! Door lock controller */
    HA_DOOR_LOCK_CONTROLLER_DEVICE_ID = 0x000B,
    /*! Simple Sensor device */
    HA_SIMPLE_SENSOR_DEVICE_ID = 0x000C,
    /*! Consumption Awareness Device */
    HA_CONSUMPTION_AWARENESS_DEVICE_ID = 0x000D,
    /*! Home Gateway */
    HA_HOME_GATEWAY_DEVICE_ID = 0x0050,
    /*! Smart plug */
    HA_SMART_PLUG_DEVICE_ID = 0x0051,
    /*! White Goods */
    HA_WHITE_GOODS_DEVICE_ID = 0x0052,
    /*! Meter Interface */
    HA_METER_INTERFACE_DEVICE_ID = 0x0053,
    /* Lightning */
    /*! On/Off Light Device */
    HA_ON_OFF_LIGHT_DEVICE_ID = 0x0100,
    /*! Dimmable Light Device */
    HA_DIMMABLE_LIGHT_DEVICE_ID = 0x0101,
    /*! Color Dimmable Light Device */
    HA_COLOR_DIMMABLE_LIGHT_DEVICE_ID = 0x0102,
    /*! On/off Switch Device */
    HA_ON_OFF_LIGHT_SWITCH_DEVICE_ID = 0x0103,
    /*! Dimmer Switch Device */
    HA_DIMMER_SWITCH_DEVICE_ID = 0x0104,
    /*! Color Dimmer Switch Device */
    HA_COLOR_DIMMER_SWITCH_DEVICE_ID = 0x0105,
    /*! Light Sensor Device */
    HA_LIGHT_SENSOR_DEVICE_ID = 0x0106,
    /*! On/Off Plug-in Unit Device */
    HA_ON_OFF_PLUG_IN_UNIT_DEVICE_ID = 0x010A,
    /*! Dimmable Plug-in Unit Device */
    HA_DIMMER_PLUG_IN_UNIT_DEVICE_ID = 0x010B,
    /*! Shade */
    HA_SHADE_DEVICE_ID = 0x0200,
    /*! Shade controller */
    HA_SHADE_CONTROLLER_DEVICE_ID = 0x0201,
    /*! Window Covering client*/
    HA_WINDOW_COVERING_DEVICE_ID = 0x0202,
    /*! Window Covering controller */
    HA_WINDOW_COVERING_CONTROLLER_DEVICE_ID = 0x0203,
    /* HVAC */
    /*! Heating/Cooling Unit device */
    HA_HEATING_COOLING_UNIT_DEVICE_ID = 0x0300,
    /*! Thermostat Device */
    HA_THERMOSTAT_DEVICE_ID = 0x0301,
    /*! Temperature Sensor */
    HA_TEMPERATURE_SENSOR_DEVICE_ID = 0x0302,
    /* IAS */
    /*! IAS Control and Indicating Equipment */
    HA_IAS_CONTROL_INDICATING_EQUIPMENT_ID = 0x0400,
    /*! IAS Ancillary Control Equipment */
    HA_IAS_ANCILLARY_CONTROL_EQUIPMENT_ID = 0x0401,
    /*! IAS Zone */
    HA_IAS_ZONE_ID = 0x0402,
    /*! IAS Warning Device */
    HA_IAS_WARNING_DEVICE_ID = 0x0403,
    /*! Custom HA device for test */
    HA_TEST_DEVICE_ID = 0xfff0,
    /*! Custom Tunnel device (should declared in private profile) */
    HA_CUSTOM_TUNNEL_DEVICE_ID = 0xfff1,
    /*! Custom Attributes Device */
    HA_CUSTOM_ATTR_DEVICE_ID = 0xfff2
};

typedef enum ZIGBEE_EVENT_TYPE
{
    ZIGBEE_EVT_TYPE_NWK_START_REQ = 0x0000,
    ZIGBEE_EVT_TYPE_NWK_JOIN_REQ,
    ZIGBEE_EVT_TYPE_ZCL_DATA_REQ,
    ZIGBEE_EVT_TYPE_ZCL_ATTR_READ_REQ,
    ZIGBEE_EVT_TYPE_ZCL_ATTR_WRITE_REQ,
    ZIGBEE_EVT_TYPE_ZCL_ATTR_SET_REQ,
    ZIGBEE_EVT_TYPE_APS_BIND_SET_REQ,
    ZIGBEE_EVT_TYPE_ZDO_SIMPLE_DESC_REQ,
    ZIGBEE_EVT_TYPE_ZDO_BIND_REQ,
    ZIGBEE_EVT_TYPE_ZDO_COMM_BIND_REQ,
    ZIGBEE_EVT_TYPE_ZDO_ACT_EP_REQ,

    ZIGBEE_EVT_TYPE_NWK_RESET_REQ,
    ZIGBEE_EVT_TYPE_NWK_REJOIN_REQ,
    ZIGBEE_EVT_TYPE_NWK_LEAVE_VIA_LOCAL_REQ,

    ZIGBEE_EVT_TYPE_OTA_FILE_INSERT_REQ,
    ZIGBEE_EVT_TYPE_OTA_CLIENT_START_REQ,

    ZIGBEE_EVT_TYPE_MAC_ED_SCAN_REQ,
    ZIGBEE_EVT_TYPE_MAC_ADDRESS_GET_REQ,
    ZIGBEE_EVT_TYPE_IS_FACTORY_NEW_REQ,
    ZIGBEE_EVT_TYPE_START_FB_REQ,
    ZIGBEE_EVT_TYPE_MGMT_PERMIT_JOIN_REQ,

    ZIGBE_EVT_TYPE_NWK_START_CFM  = 0x0100,

    ZIGBEE_EVT_TYPE_START_IDC = 0x1000,
    ZIGBEE_EVT_TYPE_ZDO_START_IDC = ZIGBEE_EVT_TYPE_START_IDC,
    ZIGBEE_EVT_TYPE_DEVICE_ANNCE_IDC,
    ZIGBEE_EVT_TYPE_DEVICE_AUTHORIZED_IDC,
    ZIGBEE_EVT_TYPE_DEVICE_ASSOCIATED_IDC,
    ZIGBEE_EVT_TYPE_LEAVE_IDC,
    ZIGBEE_EVT_TYPE_REJOIN_FAILURE_IDC,
    ZIGBEE_EVT_TYPE_PANID_CONFLICT_IDC,
    ZIGBEE_EVT_TYPE_ZDO_ACT_EP_IDC,
    ZIGBEE_EVT_TYPE_ZDO_SIMPLE_DESC_IDC,
    ZIGBEE_EVT_TYPE_ZDO_PERMIT_JOIN_IDC,
    ZIGBEE_EVT_TYPE_MAC_ED_SCAN_IDC,
    ZIGBEE_EVT_TYPE_MAC_ADDRESS_GET_IDC,
    ZIGBEE_EVT_TYPE_DEVICE_START_REJOIN,
    ZIGBEE_EVT_TYPE_IC_ADD,
    ZIGBEE_EVT_TYPE_IS_FACTORY_NEW_IDC,
    ZIGBEE_EVT_TYPE_ZDO_FINISH_IDC = ZIGBEE_EVT_TYPE_IS_FACTORY_NEW_IDC,

    ZIGBEE_EVT_TYPE_ZCL_DATA_IDC,

    ZIGBEE_EVT_TYPE_RAF_CMD_REQUEST = 0xF000,
    ZIGBEE_EVT_TYPE_RAF_CMD_CONFIRM = 0xF001,

    ZIGBEE_EVT_TYPE_MAX,
} zigbee_event_type_e;


typedef enum RAF_CMD_TYPE
{
    RAF_CMD = 0,
    RAF_CMD_MAC,
    RAF_CMD_BDB,
    RAF_CMD_NWK,
    RAF_CMD_APS,
    RAF_CMD_ZDO,
    RAF_CMD_ZCL,
    RAF_CMD_DEBUG,
} raf_cmd_type_t;

typedef enum RAF_CMD
{
    RAF_CMD_NWK_ADDRESS_REQUEST                         = 0x00000000,
    RAF_CMD_IEEE_ADDRESS_REQUEST,
    RAF_CMD_NODE_DESCRIPTOR_REQUEST,
    RAF_CMD_POWER_DESCRIPTOR_REQUEST,
    RAF_CMD_SIMPLE_DESCRIPTOR_REQUEST,
    RAF_CMD_ACTIVE_ENDPOINT_REQUEST,
    RAF_CMD_MATCH_DESCRIPTOR_REQUEST,
    RAF_CMD_BIND_REQUEST                                = 0x00000021,
    RAF_CMD_UNBIND_REQUEST,
    RAF_CMD_NWK_MANAGEMENT_LQI_REQUEST                  = 0x00000031,
    RAF_CMD_NWK_MANAGEMENT_RTG_REQUEST,
    RAF_CMD_DEVICE_BINDING_INFORMATION_REQUEST          = 0x00000033,
    RAF_CMD_DEVICE_LEAVE_REQUEST,
    RAF_CMD_DIRECT_JOIN_REQUEST,
    RAF_CMD_PERMIT_JOIN_REQUEST,
    RAF_CMD_NETWORK_UPDATE_REQUEST                      = 0x00000038,
    RAF_CMD_NETWORK_START_REQUEST,
    RAF_CMD_DEVICE_RESET_REQUEST                        = 0x00000040,
    RAF_CMD_GATEWAY_EXT_ADDRESS_REQUEST                 = 0x00000041,
    RAF_CMD_GATEWAY_PERMIT_JOIN_STATUS_REQUEST          = 0x00000042,
    RAF_CMD_GATEWAY_PANID_CHANNEL_REQUEST               = 0x00000043,
    RAF_CMD_GATEWAY_INSTALL_CODE_SET_REQUEST            = 0x00000044,
    RAF_CMD_GATEWAY_INSTALL_CODE_REMOVE_REQUEST         = 0x00000045,
    RAF_CMD_GATEWAY_INSTALL_CODE_REMOVE_ALL_REQUEST     = 0x00000046,
    RAF_CMD_GATEWAY_STANDARD_TIME_SET                   = 0x00000047,
    RAF_CMD_FINISH,
} raf_cmd_t;


typedef enum RAF_CMD_MAC_INDEX
{
    MAC_START = 0,
    MAC_GET_VISIBLE_LONG_LIST,
    MAC_ADD_VISIBLE_LONG,
    MAC_GET_INVISIBLE_SHORT_LIST,
    MAC_ADD_INVISIBLE_SHORT,
    MAC_ADD_VISIBLE_SHORT,
    MAC_FINISH = MAC_ADD_INVISIBLE_SHORT,
} raf_cmd_mac_t;

typedef enum RAF_CMD_BDB_INDEX
{
    BDB_START = 0,
    BDB_GET_SHORT_ADDR,
    BDB_SET_SHORT_ADDR,
    BDB_GET_LONG_ADDR,
    BDB_SET_LONG_ADDR,
    BDB_GET_PAN_ID,
    BDB_SET_PAN_ID,
    BDB_GET_EXT_PAN_ID,
    BDB_SET_EXT_PAN_ID,
    BDB_GET_CHANNEL_LIST,
    BDB_SET_CHANNEL,
    BDB_GET_ROLE,
    BDB_SET_ROLE,
    BDB_START_NETWORK,
    BDB_SET_NVRAM_ERASE_AT_START,
    BDB_GET_MAX_CHILDREN,
    BDB_SET_MAX_CHILDREN,
    BDB_GET_RX_ON_WHEN_IDLE,
    BDB_SET_RX_ON_WHEN_IDLE,
    BDB_GET_LEGACY_DEVICE_SUPPORT,
    BDB_SET_LEGACY_DEVICE_SUPPORT,
    BDB_GET_ED_KEEPALIVE_TIMEOUT,
    BDB_SET_ED_KEEPALIVE_TIMEOUT,
    BDB_SET_ED_AGING_TIMEOUT,
    BDB_SET_AIB_TRUST_CENTER_ADDRESS,
    BDB_SET_KEEPALIVE_MODE,
    BDB_SET_NWK_SECURITY_LEVEL,
    BDB_LAST,
    BDB_FINISH = BDB_LAST,
} raf_cmd_bdb_t;

typedef enum RAF_CMD_NWK_INDEX
{
    NWK_START = 0,
    NWK_GET_ROUTING_TABLE,
    NWK_DELETE_ROUTING_ENTRY,
    NWK_SET_CONCENTRATOR_MODE,
    NWK_SEND_ROUTE_RECORD,
    NWK_SEND_STATUS,
    NWK_GET_PANID_CONFLICT_RESOLUTION,
    NWK_SET_PANID_CONFLICT_RESOLUTION,
    NWK_DO_NLME_JOIN_REQUEST,
    NWK_DO_NLME_LEAVE_REQUEST,
    NWK_GET_LEAVE_REQ_ALLOWED,
    NWK_SET_LEAVE_REQ_ALLOWED,
    NWK_SEND_ROUTE_ROUTE_DISCOVERY,
    NWK_SEND_EXECUTE_SELF_REJOIN,
    NWK_SEND_BROADCAST_NWK_KEY,
    //NWK_NIB_SET_USE_MUTICAST,
    NWK_LAST,
    NWK_FINISH = NWK_LAST,
} raf_cmd_nwk_t;

typedef enum RAF_CMD_ZDO_INDEX
{
    ZDO_START = 0,
    ZDO_GET_PERMIT_JOIN,
    ZDO_SET_PERMIT_JOIN,
    ZDO_GET_SELF_DEVICE_NEIGHBOR_TABLE,
    ZDO_GET_SPECIFIC_DEVICE_NEIGHBOR_TABLE,
    ZDO_GET_UNSECURE_JOIN,
    ZDO_SET_UNSECURE_JOIN,
    ZDO_GET_AUTHENTICATE_ALWAYS,
    ZDO_SET_AUTHENTICATE_ALWAYS,
    ZDO_SEND_SELF_DEVICE_ANNCE,
    ZDO_SEND_SPECIFIC_DEVICE_ANNCE,
    ZDO_SEND_SERVER_DISCOVERY,
    ZDO_SEND_MGMT_NWK_UPDATE,
    ZDO_DO_SET_CHANNEL,
    ZDO_SEND_NWK_ADDR_REQUEST,
    ZDO_SEND_IEEE_ADDR_REQUEST,
    ZDO_SEND_MGMT_LEAVE_REQUEST,
    ZDO_SEBD_NODE_DESC_REQUEST,
    ZDO_SEND_BIND_UNBIND_REQUEST,
    ZDO_SEND_MGMT_BIND_REQUEST,
    ZDO_SEND_SIMPLE_DESC_REQUEST,
    ZDO_LAST,
    ZDO_FINISH = ZDO_LAST,
} raf_cmd_zdo_t;

typedef enum RAF_CMD_APS_INDEX
{
    APS_START = 0,
    APS_SET_FRAGMENTATION_CONFIGURATION,
    APS_SEND_APSE_DATA_REQUEST,
    APS_SET_SECURITY_UPDATE_KEY_PAIR,
    APS_SET_PRECONFIGURE_NETWORK_KEY,
    APS_SET_SEND_APSEME_REMOVE_DEVICE,
    APS_SET_GROUP_MANAGEMENT_CMD,
    APS_SEND_REQUEST_KEY_REQ,
    APS_SEND_TRANSPORT_KEY_REQ,
    APS_SEND_BIND_REQ,
    APS_LAST,
    APS_FINISH = APS_LAST,
} raf_cmd_aps_t;

typedef enum RAF_CMD_ZCL_INDEX
{
    ZCL_START = 0,
    ZCL_SEND_BUFFER_TEST,
    ZCL_SEND_COUNTED_PACKET,
    ZCL_RESET_SELF_PACKET_COUNTER,
    ZCL_SEND_RESET_PACKET_COUNTER,
    ZCL_GET_SELF_PACKET_COUNTER,
    ZCL_SEND_RETRIEVE_PACKET_COUNTER,
    ZCL_LAST,
    ZCL_FINISH = ZCL_LAST,
} raf_cmd_zcl_t;

typedef enum RAF_CMD_DEBUG_INDEX
{
    DEBUG_START = 0,
    DEBUG_CHANGE_LINK_STATUS,
    DEBUG_SET_ADDR_ASSIGNMENT_CB,
    DEBUG_SET_CERTIFICATION_HACKS,
    DEBUG_ZB_ADDRESS_UPDATE,
    DEBUG_SET_SIMPLE_DESCRIPTOR_IN_OUT_CLUSTER_ID,
    DEBUG_EXECUTE_FUNCTION_TRIGGER_SEND_PACKET,
    DEBUG_LAST,
    DEBUG_FINISH = DEBUG_LAST,
} raf_cmd_debug_t;

typedef enum ZB_FINDING_FINDING_ROLE_TYPE
{
    ZB_FB_TARGET_ROLE = 0,
    ZB_FB_INITIATOR_ROLE,
} zb_finding_binding_role_t;
//=============================================================================
//                Public Struct
//=============================================================================
typedef struct ZIGBEE_CONFIG_T
{
    zb_af_device_ctx_t *p_zigbee_device_contex_t;
    pf_evt_indication *pf_evt_indication;
} zigbee_cfg_t;

typedef struct ZIGBEE_NWK_START_REQ_T
{
    uint32_t deviceRole     : 2;
    uint32_t maxChild       : 8;
    uint32_t reset          : 1;
    uint32_t                : 4;
    // endevice only
    uint32_t rx_always_on   : 1;
    uint32_t keepalive      : 16; // micro second
    uint32_t channelMask;
    uint16_t PANID;
    uint16_t shortAddr;
    uint8_t  extendedPANID[8];

    uint8_t  ieeeAddr[8];
    uint32_t primaryChannelMask;
    uint32_t secondaryChannelMask; /*!< Beacon requests are sent on these channels if no network is found after the energy scan on the primary channel mask. */
} zigbee_nwk_start_req_t;

typedef struct ZIGBEE_IC_ADD_T
{
    uint8_t  ic_type;
    uint8_t  ieeeAddr[8];
    uint8_t  ic[19];
} zigbee_ic_add_t;

typedef struct ZIGBEE_ZDO_MGMT_PETMIT_JOINING_REQ_T
{
    uint16_t dest_addr;
    uint8_t permit_duration;
    uint8_t tc_significance;
} zigbee_zdo_mgmt_permit_joining_req_t;

typedef struct ZIGBEE_BUFFER_TEST_REQ_PARAM_T
{
    uint8_t len;           /*!< Length (in bytes) of the packet */
    uint16_t dst_addr;     /*!< destination address */
    uint8_t addr_mode;     /*!< address type (group or device address) */
    uint8_t src_ep;        /*!< Source endpoint */
    uint8_t dst_ep;        /*!< Destination endpoint */
    uint8_t radius;        /*!< Radius */
    uint16_t profile_id;    /*!< profile id */
}
zigbee_buffer_test_req_param_t;

typedef struct ZIGBEE_COUNTED_PACKETS_REQ_PARAM_T
{
    uint8_t  len;            /*!< Length (in bytes) of the packet */
    uint16_t packets_number; /*!< packets number to send */
    uint16_t idle_time;      /*!< time before next packet is sent */
    uint16_t dst_addr;       /*!< destination address */
    uint8_t  addr_mode;      /*!< address type (group or device address) */
    uint8_t  src_ep;         /*!< source endpoint id */
    uint8_t  dst_ep;         /*!< destination endpoint id */
    uint8_t  radius;         /*!< radius (set to 0 to use default) */
}
zigbee_counted_packets_req_param_t;

typedef struct ZIGBEE_ZDO_SYSTEM_SERVER_DISCOVERY_REQ_T
{
    uint16_t server_mask; /*!< Server mask for device discovery */
}
zigbee_zdo_system_server_discovery_req_t;

typedef struct ZIGBEE_NWK_NLME_JOIN_REQ_PARAM_T
{
    uint8_t extended_pan_id[8]; /**< Extended Pan ID
                                                        * of the network */
    uint32_t scan_channels_list[1]; /**< The list of all channel pages and
                                                               * the associated channels that shall be scanned. */
    uint8_t capability_information; /**< The operating capabilities of the
                                      * device */
    uint8_t rejoin_network; /**< Join network method @see @ref nlme_rejoin_method */
    uint8_t scan_duration; /**< Time to spend scanning each channel */
    uint8_t security_enable;   /*!< If the value of RejoinNetwork is 0x02 and this is TRUE than the device will try to rejoin securely.
                                  Otherwise, this is set to FALSE.  */
}
zigbee_nwk_nlme_join_req_param_t;

typedef struct ZIGBEE_NWK_NLME_LEAVE_REQ_PARAM_T
{
    uint8_t device_address[8]; /**< 64-bit IEEE address of the device to
                                  * remove, zero fill if device itself */
    uint8_t remove_children; /**< If true - remove child devices from the
                                    * network */
    uint8_t rejoin; /**< If true - Join after leave */
}
zigbee_nwk_nlme_leave_req_param_t;

typedef struct ZIGBEE_NWK_START_CFM_T
{
    uint32_t status;
    uint16_t panID;
    uint16_t nwkAddr;
    uint8_t ieee_addr[8];
    uint8_t channel;
} zigbee_nwk_start_cfm_t, zigbee_nwk_start_idc_t;

typedef struct ZIGBEE_ZDO_NWK_ADDR_REQ_PARAM_T
{
    uint16_t      dst_addr;     /*!< Destination address */
    uint8_t       ieee_addr[8];    /*!< IEEE address to be matched by the
                                   Remote Device  */
    uint8_t       request_type; /*!< Request type for this command:
                                   0x00  Single device response
                                   0x01  Extended response  */
    uint8_t       start_index; /*!< If the Request type for this command is
                                  Extended response, the StartIndex
                                  provides the starting index for the
                                  requested elements of the associated
                                  devices list  */
} zigbee_zdo_nwk_addr_req_param_t;

typedef struct ZIGBEE_ZDO_IEEE_ADDR_REQ_PARAM_T
{
    uint16_t      dst_addr;     /*!< Destination address */
    uint16_t      nwk_addr;    /*!< NWK address that is used for IEEE
                                  address mapping.  */
    uint8_t       request_type; /*!< Request type for this command:
                                   0x00  Single device response
                                   0x01  Extended response  */
    uint8_t       start_index; /*!< If the Request type for this command is
                                  Extended response, the StartIndex
                                  provides the starting index for the
                                  requested elements of the associated
                                  devices list  */
} zigbee_zdo_ieee_addr_req_param_t;

typedef struct ZIGBEE_ZDO_DEVICE_ANNOUNCE_IDC_T
{
    uint16_t  shortAddr;
    uint8_t   ieeeAddr[8];
    uint8_t   capability;
} zigbee_zdo_device_annce_idc_t;

typedef struct ZIGBEE_ZDO_DEVICE_LEAVE_IDC_T
{
    uint8_t   ieeeAddr[8];
    uint16_t  shortAddr;
    uint8_t   rejoin;
} zigbee_zdo_device_leave_idc_t;

typedef struct ZIGBEE_ZDO_DEVICE_PERMIT_JOIN_IDC_T
{
    uint8_t permit_duration;
} zigbee_zdo_device_permit_join_idc_t;

typedef struct ZIGBEE_ZDO_DEVICE_AUTHORIZED_IDC_T
{
    uint8_t   ieeeAddr[8]; /* Long Address of the updated device */
    uint16_t  shortAddr;   /* Short Address of the updated device */
    uint8_t   authorizationtype;  /* Type of the authorization procedure */
    uint8_t   authorizationStatus;  /* Status of the authorization procedure */
} zigbee_zdo_device_authorized_idc_t;

typedef struct ZIGBEE_ZDO_DEVICE_ANNOUNCE_PARAM_T
{
    uint8_t       tsn;         /*!< TSN of command */
    uint16_t      nwk_addr;    /*!< NWK address for the Local Device  */
    uint8_t   ieee_addr[8];   /*!< IEEE address for the Local Device  */
    uint8_t       capability;  /*!< Capability of the local device */
} zigbee_zdo_device_annce_param_t;

typedef struct ZIGBEE_SPECIFIC_ZCL_DATA_IDC_T
{
    uint32_t srcAddr        : 16;
    uint32_t srcEndpint     : 8;
    uint32_t                : 8;

    uint32_t dstAddr        : 16;
    uint32_t dstEndpint     : 8;
    uint32_t seq_num        : 8;

    uint32_t clusterID      : 16;

    uint32_t cmd               : 8;
    uint32_t specific          : 1;
    uint32_t disableDefaultRsp : 1;
    uint32_t direction         : 1;
    uint32_t is_common_command : 1;
    uint32_t manuCode          : 16;
    uint32_t                   : 4;

    uint32_t cmdFormatLen      : 16;
    uint32_t                   : 16;

    uint8_t cmdFormat[];

} zigbee_zcl_data_idc_t;

typedef struct ZIGBEE_ZCL_DATA_REQ_T
{
    uint32_t dstAddr        : 16;
    uint32_t dstEndpint     : 8;
    uint32_t srcEndPoint    : 8;
    uint32_t seq_num        : 8;
    uint32_t sp_seq_num     : 1;


    uint32_t clusterID         : 16;
    uint32_t cmd               : 8;
    uint32_t specific          : 1;
    uint32_t disableDefaultRsp : 1;
    uint32_t direction         : 1;
    uint32_t addrmode          : 3;
    uint32_t                   : 2;

    uint32_t manuCode          : 16;
    uint32_t cmdFormatLen      : 16;

    uint8_t cmdFormat[];

} zigbee_zcl_data_req_t;

typedef struct ZIGBEE_ZCL_ATTR_RW_T
{
    uint16_t dstShortAddr;
    uint8_t  dstEndPoint;
    uint8_t  srcEndPoint;
    uint16_t clusterID;
    uint16_t attrID;
    uint8_t  attrType;
    uint8_t  attrLen;
    uint8_t  attrValue[];

} zigbee_zcl_attr_rw_t;

typedef struct ZIGBEE_ZCL_ATTR_SET_T
{
    uint32_t ep                : 8;
    uint32_t cluster_role      : 8;
    uint32_t check_access      : 8;
    uint32_t                   : 8;

    uint32_t cluster_id        : 16;
    uint32_t attr_id           : 16;

    uint8_t value[];

} zigbee_zcl_attr_set_t;



typedef struct ZIGBEE_NWK_PERMIT_JOIN_REQ_T
{
    uint16_t enable;
    uint16_t timeout; // second;
} zigbee_nwk_permit_join_req_t;

typedef struct ZIGBEE_ZDO_SIMPLE_DESC_REQ_T
{
    uint16_t nwkAddr;
    uint8_t endpoint;
} zigbee_zdo_simple_desc_req_t;

typedef struct ZIGBEE_ZDO_SIMPLE_DESC_IDC_T
{
    uint8_t status;
    uint8_t endpoint;
    uint16_t nwkAddr;

    uint16_t profileID;
    uint16_t deviceID;

    uint16_t deviceVer;
    uint16_t in_cluster_count;

    uint16_t out_cluster_count;
    uint16_t clusterID[];
} zigbee_zdo_simple_desc_idc_t;

typedef struct ZIGBEE_ZDO_BIND_REQ_T
{
    uint8_t bind;               // 0:unbind, 1:bind
    uint8_t srcEP;
    zb_uint8_t dstEP;
    zb_uint8_t dstaddrmode;    /*!< Destination address mode @ref bind_dst_addr_mode */

    uint16_t clusterID;
    uint16_t reqDstAddr;        //the destination of the binding command sent to

    uint16_t groupAddr;
    zb_addr_u dstAddr;       /*!< The destination address for the*/

    uint8_t srcIeeeAddr[8];
} zigbee_zdo_bind_req_t, zigbee_zdo_unbind_req_t;


typedef struct ZIGBEE_ZDO_ACTIVE_EP_REQ_T
{
    uint16_t nwkAddr;
} zigbee_zdo_act_ep_req_t;

typedef struct ZIGBEE_ZDO_ACTIVE_EP_IDC_T
{
    uint8_t status;
    uint8_t epCounts;
    uint16_t nwkAddr;

    uint8_t ep_list[];
} zigbee_zdo_act_ep_idc_t;

typedef struct ZIGBEE_OTA_FILE_INSERT_REQ_T
{
    uint16_t manufacturer_code;
    uint16_t image_type;
    uint32_t file_version;
    uint32_t image_size;
    uint8_t *p_img_data;
} zigbee_ota_file_insert_req_t;

typedef struct ZIGBEE_OTA_CLIENT_START_REQ_T
{
    uint16_t nwkAddr;
    uint8_t ep;
} zigbee_ota_client_start_req_t;

typedef struct ZIGBEE_MAC_ED_SCAN_IDC_T
{
    uint8_t EDValue[16];
} zigbee_mac_ed_scan_idc_t;

typedef struct ZIGBEE_MAC_ADDRESS_GET_IDC_T
{
    uint8_t mac_address[8];
} zigbee_mac_address_get_idc_t;

typedef struct ZIGBEE_IS_FACTORY_NEW_IDC_T
{
    uint8_t factory_new;
} zigbee_is_factory_new_idc_t;

typedef struct ZIGBEE_START_FB_REQ_T
{
    uint8_t ep;
    uint8_t role;
} zigbee_start_fb_req_t;
typedef struct RAF_CMD_REQUEST_PARAMETER
{
    uint32_t    cmd_type;
    uint32_t    cmd_index;
    uint16_t    cmd_length;
    uint16_t    cmd_dst_addr;
    uint32_t    cmd_value[];
} raf_cmd_req_t;

typedef struct RAF_CMD_CONFIRM_PARAMETER
{
    uint32_t    Status;
    uint32_t    cmd_type;
    uint32_t    cmd_index;
    uint16_t    cmd_length;
    uint16_t    cmd_dst_addr;
    uint32_t    cmd_value[];
} raf_cmd_cfm_t;

typedef struct install_code_add
{
    uint8_t  ic_type;
    uint8_t  ieeeAddr[8];
    uint8_t  ic[19];
} install_code_add_t;
//=============================================================================
//                Public Function Declaration
//=============================================================================
/** stack interface init
 * @return 0 is success, other is fail
 */
int zigbee_stack_init(zigbee_cfg_t *pt_cfg);

/** Receive TLV packet from zigbee module via stack interface.
 *  When stack module call evt_indication of register function, APP
 *  can use this function to receive TLV packet from zigbee module.
 * @param pu8_buf the packet buffer pointer
 * @param pu32_buf_length the packet buffer length
 * @return 0 is success, other is fail
 */
int zigbee_event_msg_recvfrom(uint8_t *pu8_buf, uint32_t *pu32_buf_len);

/** Send TLV packet to zigbee module via stack interface.
 *  This function will call memory_alloc of register function, and
 *  copy packet, then send to stack module.
 * @param pt_tlv stack interface TLV packet pointer
 * @return 0 is success, other is fail
 */
int zigbee_event_msg_sendto(sys_tlv_t *pt_tlv);

/**
 *  @}
 */
#ifdef __cplusplus
};
#endif
#endif /* __ZIGBEE_STACK_API_H__ */
