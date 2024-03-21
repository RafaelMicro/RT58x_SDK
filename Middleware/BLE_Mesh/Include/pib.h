// ---------------------------------------------------------------------------
//
// ---------------------------------------------------------------------------
#ifndef __PIB_H__
#define __PIB_H__

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
//                Include (Better to prevent)
//=============================================================================

#include "mmdl_defs.h"
#include "cfg_pib.h"
#include "ble_gap.h"

//=============================================================================
//                Public Definitions of const value
//=============================================================================


//=============================================================================
//                Public ENUM
//=============================================================================

#define FLASH_DATA_MAGIC_NUM              (0x88)

#define PROD_DATA_FLASH_ADDRESS           (0xF4000)
#define APP_KEY_FLASH_ADDRESS             (0xF5000)
#define BIND_TABLE_FLASH_ADDRESS          (0xF6000)
#define MDL_SUBSCRIBE_INFO_FLASH_ADDRESS  (0xF7000)  // for server device


#define DEVICE_LIST_FLASH_ADDRESS         (0xF7000)  // for client device

typedef struct
{
    uint32_t magic_num: 8,
             data_vaild: 8,
             reserved: 16;
} pib_flash_hdr_t;

typedef enum PIB_ERR
{
    PIB_ERR_INDEX_OUT_OF_RANGE  = -9,
    PIB_ERR_UPDATE_FORCE_FAIL   = -8,
    PIB_ERR_UPDATE_FAIL         = -7,
    PIB_ERR_SEARCH_FAIL         = -6,
    PIB_ERR_DELETE_FAIL         = -5,
    PIB_ERR_INDEX_PTR_NULL      = -4,
    PIB_ERR_POINTER_NULL        = -3,
    PIB_ERR_OP_NOT_SUPPORT      = -2,
    PIB_ERR_ID_NOT_SUPPORT      = -1,
    PIB_ERR_PASS                = 0,
} pib_err_t;

typedef enum PIB_OPERATION
{
    PIB_GET = 0,
    PIB_SET,
} pib_op_t;

typedef enum PIB_TABLE_OPERATION
{
    TABLE_SEARCH = 0,

    // if entry exist, it will update the value
    // if entry not exist, it will update to empty entry
    TABLE_UPDATE,

    // this op is the same with TABLE_UPDATE
    // if entry not exist and table full, it will force update to the oldest entry
    TABLE_UPDATE_FORCE,

    // only for routing table
    TABLE_UPDATE_ALL,

    TABLE_DELETE,

    TABLE_TIMER_CHECK,

    TABLE_VALID_ENTRY_COUNT_UPDATE,

    TABLE_RESET,

    TABLE_GET_INDEX,
    TABLE_SET_INDEX,

    TABLE_DISPLAY,
} pib_table_op_t;
//=============================================================================
//                Public Struct
//=============================================================================

//=============================================================================
//                Public Function Declaration
//=============================================================================

/** Rafael pib operation
 * @param t_pibid Rafael pib identifier
 * @param t_op get/set operation
 * @param p_data_buf the buffer of data, the compare parameter must fill in this buffer
 * @return PIB_ERR_PASS is succcess, other is fail
 */

int pib_init(cfg_pib_t *p_cfg_pib, ble_mesh_element_param_t *p_element, uint8_t element_cnt, ble_gap_addr_t *p_device_addr);

uint8_t pib_prov_start_address_set(uint16_t address);

pib_err_t pib_model_subscribe_list_add(uint16_t element_addr, uint32_t model_id, uint16_t sub_address);
pib_err_t pib_model_subscribe_list_remove(uint16_t element_addr, uint32_t model_id, uint16_t sub_address);

pib_err_t pib_model_app_key_bind(uint16_t element_addr, uint32_t model_id, uint16_t key_index);
pib_err_t pib_model_app_key_unbind(uint16_t element_addr, uint32_t model_id, uint16_t key_index);

uint16_t pib_model_bind_app_key_index_get(uint16_t element_addr, uint32_t model_id, uint16_t *p_key_index_list);

/** Rafael pib Reset
 * @return PIB_ERR_PASS is succcess, other is fail
 */
pib_err_t pib_reset(void);
void pib_data_entry_dispatch(uint8_t data_type, uint8_t data_len, uint8_t *p_data);

uint16_t pib_cid_get(void);
uint16_t pib_pid_get(void);
uint16_t pib_vid_get(void);
void pib_uuid_get(uint8_t *p_uuid);

uint16_t pib_element_count_get(void);
void pib_element_data_get(ble_mesh_element_param_t **p_element_data);

uint16_t pib_primary_address_get(void);
void pib_primary_address_set(uint16_t address);
void pib_provision_complete_set(uint8_t is_provisioned);

uint32_t pib_local_dev_key_get(uint8_t *p_dev_key);
uint32_t pib_device_key_set(uint8_t *p_new_dev_key);
uint32_t pib_device_key_get(uint8_t *p_dev_key);

uint32_t pib_local_nwk_key_get(uint8_t *p_nwk_key);
uint32_t pib_local_nwk_key_set(uint8_t *p_nwk_key);

uint32_t pib_local_app_key_set(uint8_t key_idx, uint8_t *p_app_key);
uint32_t pib_local_app_key_get_by_idx(uint8_t key_idx, uint8_t *p_app_key);

uint32_t pib_local_dev_role_get(void);

void pib_remote_dev_key_config(uint16_t dev_address, uint8_t *dev_key);

uint8_t pib_is_provisioned(void);
void pib_provision_info_local_reset(void);

void pib_flash_data_init(void);
void pib_prod_data_init(void);
void pib_prod_data_update(uint8_t reset_to_default);
void pib_flash_data_set(uint32_t flash_addr, uint32_t data_size, uint8_t *p_data);
uint8_t pib_flash_data_get(uint32_t flash_addr, uint32_t data_size, uint8_t *p_data);
void pib_flash_data_remove(uint32_t flash_addr, uint32_t data_size, uint8_t *p_data);

bool pib_device_table_is_full(void);

bool pib_device_cfg_key_get_by_addr(uint16_t target_addr, uint8_t *p_config_key);
void pib_device_data_init(void);
void pib_device_data_update(uint8_t remove_prod_data);
uint16_t pib_device_cnt_get(void);
void pib_device_table_add(mesh_prov_complete_idc_t *p_prov_device);
uint8_t pib_device_remove_by_address(uint16_t address);
void pib_device_table_reset(void);
uint16_t pib_device_address_get_by_idx(uint8_t device_idx);
uint8_t pib_device_uuid_get_by_idx(uint8_t device_idx, uint8_t *p_uuid);


/** PIB data init
 * @return COMM_IF_ERR_OK is succcess, other is fail
 */
int pib_data_init(void);


#ifdef __cplusplus
};
#endif
#endif /* __PIB_H__ */
