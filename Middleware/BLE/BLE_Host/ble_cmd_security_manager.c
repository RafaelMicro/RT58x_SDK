/** @file ble_cmd_security_manager.c
 *
 * @brief Define BLE security manager command definition, structure and functions.
 *
 */

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <string.h>
#include "FreeRTOS.h"
#include "ble_api.h"
#include "ble_security_manager.h"
#include "ble_printf.h"

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

/** BLE send security request.
 *
 */
ble_err_t ble_cmd_security_request_set(uint8_t host_id)
{
    int status;
    ble_tlv_t                       *p_ble_tlv;
    ble_sm_security_request_param_t *p_sec_req_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_sm_security_request_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_SM_SECURITY_REQ_SET;
        p_ble_tlv->length = sizeof(ble_sm_security_request_param_t);
        p_sec_req_param = (ble_sm_security_request_param_t *)p_ble_tlv->value;
        p_sec_req_param->host_id = host_id;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_SECURITY_REQ_SET> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_SECURITY_REQ_SET> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** Set BLE Pairing PassKey Value
 */
ble_err_t ble_cmd_passkey_set(ble_sm_passkey_param_t *p_param)
{
    int status;
    ble_tlv_t              *p_ble_tlv;
    ble_sm_passkey_param_t *ble_passkey_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_sm_passkey_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_SM_PASSKEY_SET;
        p_ble_tlv->length = sizeof(ble_sm_passkey_param_t);
        ble_passkey_param = (ble_sm_passkey_param_t *)p_ble_tlv->value;
        ble_passkey_param->host_id = p_param->host_id;
        ble_passkey_param->passkey = p_param->passkey;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_PASSKEY_SET> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_PASSKEY_SET> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** Set BLE Numeric Comparison Value Result
 */
ble_err_t ble_cmd_numeric_comp_result_set(ble_sm_numeric_comp_result_param_t *p_param)
{
    int status;
    ble_tlv_t              *p_ble_tlv;
    ble_sm_numeric_comp_result_param_t *ble_num_comp_result_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_sm_numeric_comp_result_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_SM_NUMERIC_COMP_RESULT_SET;
        p_ble_tlv->length = sizeof(ble_sm_passkey_param_t);
        ble_num_comp_result_param = (ble_sm_numeric_comp_result_param_t *)p_ble_tlv->value;
        ble_num_comp_result_param->host_id = p_param->host_id;
        ble_num_comp_result_param->same_numeric = p_param->same_numeric;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_NUMERIC_COMP_RESULT_SET> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_NUMERIC_COMP_RESULT_SET> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** Set BLE IO Capabilities
 */
ble_err_t ble_cmd_io_capability_set(ble_sm_io_cap_param_t *p_param)
{
    int status;
    ble_tlv_t             *p_ble_tlv;
    ble_sm_io_cap_param_t *ble_io_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_sm_io_cap_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_SM_IO_CAPABILITY_SET;
        p_ble_tlv->length = sizeof(ble_sm_io_cap_param_t);
        ble_io_param = (ble_sm_io_cap_param_t *)p_ble_tlv->value;
        ble_io_param->io_caps_param = p_param->io_caps_param;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_IO_CAPABILITY_SET> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_IO_CAPABILITY_SET> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** Set BLE Bonding Flags
 */
ble_err_t ble_cmd_bonding_flag_set(ble_sm_bonding_flag_param_t *p_param)
{
    int status;
    ble_tlv_t                   *p_ble_tlv;
    ble_sm_bonding_flag_param_t *ble_bonding_flag_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_sm_bonding_flag_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_SM_BONDING_FLAG_SET;
        p_ble_tlv->length = sizeof(ble_sm_bonding_flag_param_t);
        ble_bonding_flag_param = (ble_sm_bonding_flag_param_t *)p_ble_tlv->value;
        ble_bonding_flag_param->bonding_flag = p_param->bonding_flag;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_BONDING_FLAG_SET> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_BONDING_FLAG_SET> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE retoste cccd command
 */
ble_err_t ble_cmd_cccd_restore(uint8_t host_id)
{
    int status;
    ble_tlv_t                   *p_ble_tlv;
    ble_sm_restore_cccd_param_t *ble_cccd_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_sm_restore_cccd_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_SM_CCCD_RESTORE;
        p_ble_tlv->length = sizeof(ble_sm_restore_cccd_param_t);
        ble_cccd_param = (ble_sm_restore_cccd_param_t *)p_ble_tlv->value;
        ble_cccd_param->host_id = host_id;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_CCCD_RESTORE> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_CCCD_RESTORE> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE bonding space init
 */
ble_err_t ble_cmd_bonding_space_init(void)
{
    int status;
    ble_tlv_t *p_ble_tlv;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_SM_BOND_SPACE_INIT;
        p_ble_tlv->length = 0;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_BOND_INIT> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_BOND_INIT> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE write identity resolving key command
 */
ble_err_t ble_cmd_write_identity_resolving_key(ble_sm_irk_param_t *p_param)
{
    int status;
    ble_tlv_t *p_ble_tlv;
    ble_sm_irk_param_t *p_irk_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_sm_irk_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_SM_IDENTITY_RESOLVING_KEY_SET;
        p_ble_tlv->length = sizeof(ble_sm_irk_param_t);
        p_irk_param = (ble_sm_irk_param_t *)p_ble_tlv->value;
        memcpy(p_irk_param, p_param, sizeof(ble_sm_irk_param_t));

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_IRK_SET> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_IRK_SET> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE LESC init
 */
ble_err_t ble_cmd_lesc_init(void)
{
    int status;
    ble_tlv_t *p_ble_tlv;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_SM_LESC_INIT;
        p_ble_tlv->length = 0;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_BOND_INIT> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<SM_BOND_INIT> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}


