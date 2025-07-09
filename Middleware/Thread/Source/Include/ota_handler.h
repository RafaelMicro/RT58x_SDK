/**
 * @file uart_handler.h
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2022-03-24
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef __OTA_HANDLER_H__
#define __OTA_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
//                Include (Better to prevent)
//=============================================================================

//=============================================================================
//                Public Definitions of const value
//=============================================================================
#define BOOTLOADER_SIZE                               0x8000              /**< Total Size for bootloader area.*/
#define MP_SECTOR_SIZE                                0x10000             /**< Total Size for MP sector area.*/

#define FOTA_UPDATE_BANK_INFO_ADDRESS                 0x00007000          /**< Address of FOTA update information*/
#define APP_START_ADDRESS                             0x00008000          /**< Start address of application code.*/

#define FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB             0x00099000          /**< Address of FOTA update data.*/
#define SIZE_OF_FOTA_BANK_1MB                         0x00057000          /**< Total Size for saving FOTA update data.*/

#define FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB_UNCOMPRESS  0x0007C000          /**< Address of uncompressed FOTA update data.*/
#define SIZE_OF_FOTA_BANK_1MB_UNCOMPRESS              0x00074000          /**< Total Size for saving FOTA update data.*/

#define FOTA_UPDATE_BUFFER_FW_ADDRESS_512K            0x0004F000          /**< Address of FOTA update data.*/
#define SIZE_OF_FOTA_BANK_512K                        0x0002B000          /**< Total Size for saving FOTA update data.*/

#define FOTA_UPDATE_BUFFER_FW_ADDRESS_512K_UNCOMPRESS 0x00041000          /**< Address of FOTA update data.*/
#define SIZE_OF_FOTA_BANK_512K_UNCOMPRESS             0x00039000          /**< Total Size for saving FOTA update data.*/

#define FOTA_IMAGE_READY                              0xA55A6543          /**< Specific string for notify bootloader that bank1 exist image ready to update.*/

#define FOTABANK_STATUS_FLASH_PROGRAMMING             0x00000000
#define FOTABANK_STATUS_FLASH_ERASE_FAIL              0x00000001

#define FOTA_IMAGE_INFO_COMPRESSED                          0x01
#define FOTA_IMAGE_INFO_SIGNATURE                           0x02

#define FOTA_RESULT_SUCCESS                                 0x00
#define FOTA_RESULT_ERR_VERIFY_SIZE_NOT_FOUND               0x01
#define FOTA_RESULT_ERR_IMAGE_SIZE                          0x02
#define FOTA_RESULT_ERR_SECURE_MAGIC_PATTERN_MISMATCH       0x03
#define FOTA_RESULT_ERR_SECURE_VERIFY_SIZE_MISMATCH         0x04
#define FOTA_RESULT_ERR_IMAGE_ECDSA_VERIFY_FAIL             0x05
#define FOTA_RESULT_ERR_CHECK_IMAGE_CRC_FAIL                0x06
#define FOTA_RESULT_ERR_TARGET_ADDR_IS_ILLEGAL              0x07
#define FOTA_RESULT_ERR_COMPRESS_DECODE_FAIL                0x08

#define OTA_FLASH_START FOTA_UPDATE_BUFFER_FW_ADDRESS_1MB
#define RAFAEL_OTA_URL_DATA "ota/data"
#define RAFAEL_OTA_URL_REQ "ota/req"
#define RAFAEL_OTA_URL_RESP "ota/resp"
#define RAFAEL_OTA_URL_REPORT "ota/report"
#define RAFAEL_OTA_URL_WAKEUP "ota/wakeup"
#define OTA_SEGMENTS_MAX_SIZE 256
#define OTA_REQUEST_TABLE_SIZE 10
#define OTA_RESPONSE_TABLE_SIZE 40
#define OTA_REQUEST_TRY_MAX 30
#define OTA_RESPONESE_TIMEOUT (2000)
#define OTA_DONE_TIMEOUT (3*60*1000)
#define OTA_DEPORT_TIMEOUT (30*1000)
#define OTA_REBOOT_TIMEOUT (1*60*1000)

//=============================================================================
//                Public ENUM
//=============================================================================
typedef enum
{
    OTA_IDLE = 0x10,
    OTA_DATA_SENDING,
    OTA_DATA_RECEIVING,
    OTA_UNICASE_RECEIVING,
    OTA_REQUEST_SENDING,
    OTA_WAKEUP_RECEIVING,
    OTA_DONE,
    OTA_REBOOT,
} ota_state_t;

typedef enum
{
    OTA_PAYLOAD_TYPE_DATA = 0x20,
    OTA_PAYLOAD_TYPE_DATA_ACK,
    OTA_PAYLOAD_TYPE_REQUEST,
    OTA_PAYLOAD_TYPE_RESPONSE,
    OTA_PAYLOAD_TYPE_WAKEUP
} ota_payload_type_t;

typedef enum
{
    OTA_DATA_SEND_EVENT = 0x30,
    OTA_REQUEST_SEND_EVENT,
    OTA_RESPONSE_SEND_EVENT,
    OTA_DATA_RECEIVE_EVENT,
    OTA_REQUEST_RECEIVE_EVENT,
    OTA_RESPONSE_RECEIVE_EVENT,
    OTA_WAKEUP_RECEIVE_EVENT
} ota_event_state_t;
//=============================================================================
//                Public Struct
//=============================================================================
typedef uint32_t fotabank_status;
typedef uint8_t fotaimage_info;

typedef struct
{
    uint8_t event;
    uint8_t data[400];
    uint16_t data_lens;
} __attribute__((packed)) ota_event_data_t;

typedef struct
{
    uint32_t version;
    uint32_t size;
    uint32_t crc;
    uint16_t seq;
    uint16_t segments;
    uint16_t intervel;
    bool is_unicast;
    uint8_t data[OTA_SEGMENTS_MAX_SIZE];
} __attribute__((packed)) ota_data_t;

typedef struct
{
    uint8_t data_type;
    uint32_t version;
    uint16_t seq;
} __attribute__((packed)) ota_data_ack_t;

typedef struct
{
    uint32_t version;
    uint32_t size;
    uint16_t segments;
    uint16_t req_table[OTA_REQUEST_TABLE_SIZE];
} __attribute__((packed)) ota_request_t;

typedef struct
{
    uint32_t version;
    uint32_t size;
    uint32_t crc;
    uint16_t seq;
    uint16_t segments;
    uint8_t data[OTA_SEGMENTS_MAX_SIZE];
} __attribute__((packed)) ota_response_t;

typedef struct
{
    fotabank_status     status;                 /* Two possible results for Bank 1: already programming, erase failure */
    uint32_t            fotabank_ready;         /* Notify bootloader that new FW is stored in Bank 1 */
    uint32_t            fotabank_crc;           /* Crc32 checksum of FW in Bank 1 */
    uint32_t            fotabank_datalen;       /* Data length of FW in Bank 1 */
    uint32_t            fotabank_startaddr;     /* The starting flash address of fota bank */
    uint32_t            target_startaddr;       /* The starting flash address of Target */
    uint8_t             fota_result;            /* The FOTA result after Reboot */
    fotaimage_info      fota_image_info;        /* The FOTA image compressed */
    uint32_t            signature_len;          /* The length to generate signature */
    uint8_t             reserved_[2];           /* Reserved for future use */
    uint32_t            reserved[8];            /* Reserved for future use */
    uint32_t            expectaddr_initstep;    /* The initial (first) address for stored step that use for calculate expecting address to retransmission.
                                                For resume FOTA transmission, we use steps to counting so far how much data were store into fota bank,
                                                more step means more data been stored. Therefore, we can use step number to calculate "Expecting start address".*/
} fota_information_t;
//=============================================================================
//                Public Function Declaration
//=============================================================================

otError ota_init(otInstance *aInstance, void (*ota_state_change_cb)(uint8_t state));
void ota_stop();
void ota_send_wakeup();
void ota_start(uint16_t segments_size, uint16_t intervel);
uint8_t ota_get_state();
void ota_bootloader_info_check();
void ota_event_handler();
void ota_bootinfo_reset();
uint8_t ota_reset();
uint32_t ota_get_image_version();
uint32_t ota_get_image_size();
uint32_t ota_get_image_crc();
void ota_set_image_version(uint32_t version);
void ota_set_image_size(uint32_t size);
void ota_set_image_crc(uint32_t crc);
void ota_send(char *ipaddr_str);
void ota_debug_level(unsigned int level);
const char *OtaStateToString(ota_state_t state);
uint32_t crc32checksum(uint32_t flash_addr, uint32_t data_len);

#ifdef __cplusplus
};
#endif
#endif /* __OTA_HANDLER_H__ */
