/**
 * @file zigbee_lib_api.h
 *
 * @brief Rafael zigbee lib api
 *
 * @details Rafael zigbee lib api
 *
 * @version V1.0.0 Alpha
 * @date 16. March 2020
 * @date 2022-03-20 16:03
 *
 * @note
 * Copyright (c) 2020, Rafael Microelectronics, Inc. All rights reserved.
 *
 * @par
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * www.rafaelmicro.com
 */

#ifndef __ZIGBEE_APP_API_H__
#define __ZIGBEE_APP_API_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "zigbee_stack_api.h"

/** @addtogroup ZigBee_Library
 *  @{
 */

/**
 * @brief start zigbee network function
 *
 * @param  device_role       ZIGBEE_DEVICE_ROLE_CORDINATOR,
 *                           ZIGBEE_DEVICE_ROLE_ROUTER,
 *                           ZIGBEE_DEVICE_ROLE_ENDDEVICE
 * @param  channel_mask      use ZIGBEE_CHANNEL_MASK(n)to set channel n: 11~ 26 or
 *                           ZIGBEE_CHANNEL_ALL_MASK(), scan for all channels
 * @param  primary_ch_mask   use ZIGBEE_CHANNEL_MASK(n)to set channel n: 11~ 26 or
 *                           (ZIGBEE_CHANNEL_MASK(n1) | ZIGBEE_CHANNEL_MASK(n2)), scan for select channel by mask.
 * @param  maxChild          number of maximun child
 * @param  panID             PAN ID to be joined of started
 * @param  extPANID          set to use the extended PAN ID
 * @param  reset             start with network reset; 1: reset, 0: use original network settings
 *
 * @return uint32_t          return 0 for success
 */
uint32_t zigbee_nwk_start_request(uint32_t device_role, uint32_t channel_mask, uint32_t primary_ch_mask, uint32_t maxChild, uint16_t panID, uint8_t  *extPANID, uint32_t reset);
/**
 * @brief start zigbee network function for zigbee end device
 *
 * @param  channel_mask     use ZIGBEE_CHANNEL_MASK(n)to set channel n: 11~ 26 or
 *                          ZIGBEE_CHANNEL_ALL_MASK(), scan for all channels
 * @param  primary_ch_mask   use ZIGBEE_CHANNEL_MASK(n)to set channel n: 11~ 26 or
 *                           (ZIGBEE_CHANNEL_MASK(n1) | ZIGBEE_CHANNEL_MASK(n2)), scan for select channel by mask.
 * @param  rx_allway_on     enable rx always on
 * @param  keepalive        setup the keep alive timeout
 * @param  extPANID         set to use the extended PAN ID
 * @param  reset            start with network reset; 1: reset, 0: use original network settings
 *
 * @return uint32_t         return 0 for success
 */
uint32_t zigbee_ed_nwk_start_request(uint32_t channel_mask, uint32_t primary_ch_mask, uint32_t rx_allway_on, uint16_t keepalive, uint8_t  *extPANID, uint32_t reset);

/**
 * @brief start join request
 *
 *
 */
void zigbee_join_request(void);

/**
 * @brief leave netwok request
 *
 *
 */
void zigbee_leave_req(void);

/**
 * @brief send ZCL packet
 *
 *
 * @param  pt_data_req      pointer to ZCL data request
 * @param  data_len         length of ZCL data
 *
 */
void zigbee_zcl_request(zigbee_zcl_data_req_t *pt_data_req, uint16_t data_len);

/**
 * @brief send ZCL attribute read request
 *
 *
 * @param  pt_data_req      pointer to ZCL attribute read
 * @param  data_len         length of ZCL attribute data
 *
 */
void zigbee_zcl_attr_read_request(zigbee_zcl_attr_rw_t *pt_data_req, uint16_t data_len);

/**
 * @brief send ZCL attribute write request
 *
 *
 * @param  pt_data_req      pointer to ZCL attribute write
 * @param  data_len         length of ZCL attribute data
 *
 */
void zigbee_zcl_attr_wirte_request(zigbee_zcl_attr_rw_t *pt_data_req, uint16_t data_len);

/**
 * @brief set ZCL attribute
 *
 *
 * @param  pt_attr_set      pointer to ZCL attr set
 * @param  data_len         length of ZCL attribute
 *
 */
void zigbee_zcl_attr_set(zigbee_zcl_attr_set_t *pt_attr_set, uint16_t data_len);

/**
 * @brief set internal binding table
 *
 *
 * @param  srcEP            set the source endpoint for bind request
 * @param  srcIeee          set the source IEEE address for bind request
 * @param  dstEP            set the destination endpoint for bind request
 * @param  dstaddr          set the destination address for bind request
 * @param  clusterID        cluster ID for bind request
 *
 */
void zigbee_aps_bind_set(uint8_t srcEP, uint8_t *srcIeee, uint8_t dstEP, uint8_t *dstAddr, uint16_t clusterID);
/**
 * @brief insert the OTA file on the OTA server side
 *
 * @param  file_addr        OTA file location address
 * @param  file_size        size of OTA file
 * @param  file_version     Version of OTA file
 * @param  image_type       image_type of OTA file
 * @param  manufacturer_code     manufacturer_code of OTA file
 *
 */
void zigbee_ota_insert_file(uint32_t file_addr, uint32_t file_size, uint32_t file_version, uint16_t image_type, uint16_t manufacturer_code);

/**
 * @brief send the simple descript request
 *
 *
 * @param  addr             target's short(network) address
 * @param  endpoint         target's end point
 *
 */
void zigbee_simple_desc_req(uint16_t addr, uint8_t endpoint);


/**
 * @brief send the bind request
 *
 * @param  bind             0: unbind, 1: bind
 * @param  srcEP            set the source endpoint for bind request
 * @param  srcIeee          set the source IEEE address for bind request
 * @param  dstEP            set the destination endpoint for bind request
 * @param  dstaddr          set the destination address for bind request
 * @param  dstAddrMode      set thee destination address mode
 * @param  clusterID        cluster ID for bind request
 * @param  reqDstAddr       destination short(network) address send to the target device
 *
 */
void zigbee_bind_req(uint8_t bind, uint8_t srcEP, uint8_t *srcIeee, uint8_t dstEP, uint8_t *dstAddr, uint8_t dstAddrMode, uint16_t clusterID, uint16_t reqDstAddr);
/**
 * @brief request for active end point
 *
 * @param  addr             target's short(network) address
 *
 */
void zigbee_act_ep_req(uint16_t addr);

/**
 * @brief perform the energy detection for each channel
 *
 */
void zigbee_mac_ed_scan_req(void);

/**
 * @brief request for get device mac address
 *
 */
void zigbee_mac_address_get_req(void);


/**
 * @brief add the install code
 *
 */
void zigbee_ic_add(uint8_t *addr, uint8_t type, uint8_t *ic);


/**
 * @brief confirm is a factory new device or not
 *
 */
void zigbee_is_factory_new_req(void);

/**
 * @brief request for finding and binding process
 *
 * @param  ep     source endpoint
 * @param  role   must be ZB_FB_TARGET_ROLE or ZB_FB_INITIATOR_ROLE
 *
 */
void zigbee_finding_binding_req(uint8_t ep, uint8_t role);

/**
 * @brief send permit join to target short address
 *
 * @param  dstAddr     target short address
 * @param  permit_duration   permit join duration, set 0 to close permit join
 *
 */
void zigbee_send_permit_join(uint16_t dstAddr, uint8_t permit_duration);
/**
 * @brief Start the OTA client function
 *
 * @param  nwkAddr          short(network) address for OTA server
 * @param  ep               end point for OTA server
 *
 */
void zigbee_ota_client_start(uint16_t nwkAddr, uint8_t ep);

/**
 * @brief Rafael defined zigbee command request function
 *
 * @param  dst_addr          short(network) address to the target destination
 * @param  t_index           command index
 * @param  u32_len           command length
 * @param  *pu8_value        command data pointer
 *
 */
void zigbee_raf_cmd_req(uint16_t dst_addr, uint32_t t_index, uint32_t u32_len, uint8_t *pu8_value);
/**
 * @brief Power cycle reset process
 *
 */
void check_reset(void);
/** @}  */

#ifdef __cplusplus
};
#endif
#endif /* __ZIGBEE_APP_API_H__ */
