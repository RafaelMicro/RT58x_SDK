
#ifndef __APP_OTA_DOWNLOAD_CMD_HANDLER_H__
#define __APP_OTA_DOWNLOAD_CMD_HANDLER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name APS addressing mode constants
 * @anchor aps_addr_mode
 */
/** @{ */
#define ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT  0x00U /*!< DstAddress and DstEndpoint not present  */
#define ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT  0x01U /*!< 16-bit group address for DstAddress; DstEndpoint not present */
#define ZB_APS_ADDR_MODE_16_ENDP_PRESENT            0x02U /*!< 16-bit address for DstAddress and DstEndpoint present */
#define ZB_APS_ADDR_MODE_64_ENDP_PRESENT            0x03U /*!< 64-bit extended address for DstAddress and DstEndpoint present  */
#define ZB_APS_ADDR_MODE_BIND_TBL_ID                0x04U /*!< "destination endpoint" is interpreted as an index in the binding table,
                                                              all other destination address information is ignored */


/** @} */
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

uart_handler_data_sts_t ota_download_cmd_parser(uint8_t *pBuf, uint16_t plen, uint16_t *datalen, uint16_t *offset);
void ota_download_cmd_proc(uint8_t *pBuf, uint32_t len);
void ota_download_cmd_send(uint32_t cmd_id, uint16_t addr, uint8_t addr_mode, uint8_t src_endp, uint8_t *pParam, uint32_t len);

#ifdef __cplusplus
};
#endif
#endif /* __OTA_DOWNLOAD_HANDLER_H__ */