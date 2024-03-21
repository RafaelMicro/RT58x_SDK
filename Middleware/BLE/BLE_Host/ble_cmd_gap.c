/** @file ble_cmd_gap.c
 *
 * @brief Define BLE GAP command definition, structure and functions.
 *
 */

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <string.h>
#include "FreeRTOS.h"
#include "ble_api.h"
#include "ble_gap.h"
#include "ble_printf.h"

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

/** Get BLE device address and device address type.
 *
*/
ble_err_t ble_cmd_device_addr_get(ble_gap_addr_t *p_addr)
{
    int status;
    ble_tlv_t *p_ble_tlv;
    ble_gap_get_addr_t *p_target;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_get_addr_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_DEVICE_ADDR_GET;
        p_ble_tlv->length = sizeof(ble_gap_get_addr_t);
        p_target = (ble_gap_get_addr_t *)p_ble_tlv->value;

        p_target->p_addr_param = p_addr;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<DEVICE_ADDR_GET> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<DEVICE_ADDR_GET> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** Set BLE device address and device address type.
 *
*/
ble_err_t ble_cmd_device_addr_set(ble_gap_addr_t *p_addr)
{
    int status;
    ble_tlv_t      *p_ble_tlv;
    ble_gap_addr_t *ble_gap_device_addr;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_addr_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_DEVICE_ADDR_SET;
        p_ble_tlv->length = sizeof(ble_gap_addr_t);
        ble_gap_device_addr = (ble_gap_addr_t *)p_ble_tlv->value;

        memcpy(ble_gap_device_addr, p_addr, sizeof(ble_gap_addr_t));

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<DEVICE_ADDR_SET> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<DEVICE_ADDR_SET> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE connection create command.
 *
 */
ble_err_t ble_cmd_conn_create(ble_gap_create_conn_param_t *p_param)
{
    int status;
    ble_tlv_t                   *p_ble_tlv;
    ble_gap_create_conn_param_t *ble_create_conn_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_create_conn_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_CONNECTION_CREATE;
        p_ble_tlv->length = sizeof(ble_gap_create_conn_param_t);
        ble_create_conn_param = (ble_gap_create_conn_param_t *)p_ble_tlv->value;

        ble_create_conn_param->host_id = p_param->host_id;
        ble_create_conn_param->own_addr_type = p_param->own_addr_type;
        ble_create_conn_param->scan_interval = p_param->scan_interval;
        ble_create_conn_param->scan_window = p_param->scan_window;
        ble_create_conn_param->init_filter_policy = p_param->init_filter_policy;
        ble_create_conn_param->conn_param.min_conn_interval = p_param->conn_param.min_conn_interval;
        ble_create_conn_param->conn_param.max_conn_interval = p_param->conn_param.max_conn_interval;
        ble_create_conn_param->conn_param.periph_latency = p_param->conn_param.periph_latency;
        ble_create_conn_param->conn_param.supv_timeout = p_param->conn_param.supv_timeout;
        ble_create_conn_param->peer_addr.addr_type = p_param->peer_addr.addr_type;
        memcpy(ble_create_conn_param->peer_addr.addr, p_param->peer_addr.addr, BLE_ADDR_LEN);

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<CONNECTION_CREATE> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<CONNECTION_CREATE> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE cancel create connection process command.
 *
 */
ble_err_t ble_cmd_conn_create_cancel(void)
{
    int status;
    ble_tlv_t *p_ble_tlv;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_CONNECTION_CANCEL;
        p_ble_tlv->length = 0;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<CONNECTION_CANCEL> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<CONNECTION_CANCEL> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE connection parameter update.
 *
 */
ble_err_t ble_cmd_conn_param_update(ble_gap_conn_param_update_param_t *p_param)
{
    int status;
    ble_tlv_t                         *p_ble_tlv;
    ble_gap_conn_param_update_param_t *ble_conn_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_conn_param_update_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_CONNECTION_UPDATE;
        p_ble_tlv->length = sizeof(ble_gap_conn_param_update_param_t);
        ble_conn_param = (ble_gap_conn_param_update_param_t *)p_ble_tlv->value;

        ble_conn_param->host_id = p_param->host_id;
        ble_conn_param->ble_conn_param.min_conn_interval = p_param->ble_conn_param.min_conn_interval;
        ble_conn_param->ble_conn_param.max_conn_interval = p_param->ble_conn_param.max_conn_interval;
        ble_conn_param->ble_conn_param.periph_latency = p_param->ble_conn_param.periph_latency;
        ble_conn_param->ble_conn_param.supv_timeout = p_param->ble_conn_param.supv_timeout;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<CONNECTION_UPDATE> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "CONNECTION_UPDATE> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE terminate the BLE connection link.
 *
 */
ble_err_t ble_cmd_conn_terminate(uint8_t host_id)
{
    int status;
    ble_tlv_t                      *p_ble_tlv;
    ble_gap_conn_terminate_param_t *ble_terminate_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_conn_terminate_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_CONNECTION_TERMINATE;
        p_ble_tlv->length = sizeof(ble_gap_conn_terminate_param_t);
        ble_terminate_param = (ble_gap_conn_terminate_param_t *)p_ble_tlv->value;
        ble_terminate_param->host_id = host_id;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<CONNECTION_TERMINATE> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<CONNECTION_TERMINATE> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE PHY update.
 *
 */
ble_err_t ble_cmd_phy_update(ble_gap_phy_update_param_t *p_param)
{
    int status;
    ble_tlv_t                  *p_ble_tlv;
    ble_gap_phy_update_param_t *ble_phy_update_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_phy_update_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_PHY_UPDATE;
        p_ble_tlv->length = sizeof(ble_gap_phy_update_param_t);
        ble_phy_update_param = (ble_gap_phy_update_param_t *)p_ble_tlv->value;

        ble_phy_update_param->host_id = p_param->host_id;
        ble_phy_update_param->tx_phy = p_param->tx_phy;
        ble_phy_update_param->rx_phy = p_param->rx_phy;
        ble_phy_update_param->coded_phy_option = p_param->coded_phy_option;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<PHY_UPDATE> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<PHY_UPDATE> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE read PHY.
 *
 */
ble_err_t ble_cmd_phy_read(uint8_t host_id)
{
    int status;
    ble_tlv_t                *p_ble_tlv;
    ble_gap_phy_read_param_t *ble_phy_read_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_phy_read_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_PHY_READ;
        p_ble_tlv->length = sizeof(ble_gap_phy_read_param_t);
        ble_phy_read_param = (ble_gap_phy_read_param_t *)p_ble_tlv->value;

        ble_phy_read_param->host_id = host_id;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<PHY_READ> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<PHY_READ> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE preferred PHY set.
 *
 */
ble_err_t ble_cmd_default_phy_set(ble_gap_default_phy_set_param_t *p_param)
{
    int status;
    ble_tlv_t                  *p_ble_tlv;
    ble_gap_default_phy_set_param_t *ble_default_phy_set_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_default_phy_set_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_DEFAULT_PHY_SET;
        p_ble_tlv->length = sizeof(ble_gap_default_phy_set_param_t);
        ble_default_phy_set_param = (ble_gap_default_phy_set_param_t *)p_ble_tlv->value;

        ble_default_phy_set_param->tx_phy = p_param->tx_phy;
        ble_default_phy_set_param->rx_phy = p_param->rx_phy;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<PHY_UPDATE> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<PHY_UPDATE> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE read RSSI.
 *
 */
ble_err_t ble_cmd_rssi_read(uint8_t host_id)
{
    int status;
    ble_tlv_t *p_ble_tlv;
    ble_gap_rssi_read_param_t *ble_rssi_read_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_rssi_read_param_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_RSSI_READ;
        p_ble_tlv->length = sizeof(ble_gap_rssi_read_param_t);
        ble_rssi_read_param = (ble_gap_rssi_read_param_t *)p_ble_tlv->value;

        ble_rssi_read_param->host_id = host_id;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<RSSI_READ> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<RSSI_READ> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE set le host channel classification.
 *
 */
ble_err_t ble_cmd_host_ch_classif_set(ble_gap_host_ch_classif_t *p_param)
{
    int status;
    ble_tlv_t *p_ble_tlv;
    ble_gap_host_ch_classif_t *p_ch_classif_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_host_ch_classif_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_HOST_CHANNEL_CLASSIFICATION_SET;
        p_ble_tlv->length = sizeof(ble_gap_host_ch_classif_t);
        p_ch_classif_param = (ble_gap_host_ch_classif_t *)p_ble_tlv->value;

        memcpy(p_ch_classif_param, p_param, sizeof(ble_gap_host_ch_classif_t));

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<CHANNEL_CLASSIFICATION_SET> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<CHANNEL_CLASSIFICATION_SET> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE read channel map.
 *
 */
ble_err_t ble_cmd_channel_map_read(uint8_t host_id)
{
    int status;
    ble_tlv_t *p_ble_tlv;
    ble_gap_channel_map_read_t *p_read_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_channel_map_read_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_CHANNEL_MAP_READ;
        p_ble_tlv->length = sizeof(ble_gap_channel_map_read_t);
        p_read_param = (ble_gap_channel_map_read_t *)p_ble_tlv->value;

        p_read_param->host_id = host_id;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<CHANNEL_MAP_READ> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<CHANNEL_MAP_READ> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE initial resolvable address.
 *
 */
ble_err_t ble_cmd_resolvable_address_init(void)
{
    int status;
    ble_tlv_t *p_ble_tlv;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_RESOLVABLE_ADDR_INIT;
        p_ble_tlv->length = 0;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<RESOLVABLE_ADDR_INIT> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<RESOLVABLE_ADDR_INIT> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}

/** BLE regenerate resolvable address.
 *
 */
ble_err_t ble_cmd_regenerate_resolvable_address(uint8_t host_id, uint8_t en_new_irk)
{
    int status;
    ble_tlv_t *p_ble_tlv;
    ble_gap_regen_resol_addr_t *p_param;

    status = BLE_ERR_OK;
    p_ble_tlv = pvPortMalloc(sizeof(ble_tlv_t) + sizeof(ble_gap_regen_resol_addr_t));

    if (p_ble_tlv != NULL)
    {
        p_ble_tlv->type = TYPE_BLE_GAP_REGEN_RESOLVABLE_ADDRESS;
        p_ble_tlv->length = sizeof(ble_gap_regen_resol_addr_t);
        p_param = (ble_gap_regen_resol_addr_t *)p_ble_tlv->value;
        p_param->host_id = host_id;
        p_param->gen_new_irk = en_new_irk;

        status = ble_event_msg_sendto(p_ble_tlv);
        if (status != BLE_ERR_OK) // send to BLE stack
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<REGEN_RESOLVABLE_ADDRESS> Send to BLE stack fail\n");
        }
        vPortFree(p_ble_tlv);
    }
    else
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "<REGEN_RESOLVABLE_ADDRESS> malloc fail\n");
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return (ble_err_t)status;
}
