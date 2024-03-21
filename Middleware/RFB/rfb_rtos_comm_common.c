/**
 * @file rfb_comm_common.c
 * @author
 * @date
 * @brief Apis for RFB communication subsystem and ruci format
 *
 * More detailed description can go here
 *
 *
 * @see http://
 */
/**************************************************************************************************
*    INCLUDES
*************************************************************************************************/
#include <string.h>
#include <stdio.h>
#include "project_config.h"
#include "cm3_mcu.h"
#include "ruci.h"
#include "rfb_comm.h"
#include "rfb.h"
#include "rf_common_init.h"
#include "sys_arch.h"

#include "task_pci.h"
#include "mem_mgmt.h"

#include "util_log.h"
/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/
#define RFB_CMD_QUEUE_SIZE   15

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define RX_ALWAYS_ON_MODE (0xFFFFFFFF)

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
typedef struct _rfb_rx_ctrl_field_t
{
    uint16_t        Length;
    uint8_t         CrcStatus;
    uint8_t         Rssi;
    uint8_t         Snr;
} rfb_rx_ctrl_field_t;

typedef struct _rfb_rx_buffer_s
{
    rfb_rx_ctrl_field_t rx_control_field;
    uint8_t rx_data[MAX_RF_LEN];
} rfb_rx_buffer_t;

typedef struct _rfb_rx_queue_t
{
    uint8_t rx_num;
    rfb_rx_buffer_t rx_buf[MAX_RX_BUFF_NUM];
} rfb_rx_queue_t;

/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
sys_queue_t g_rfb_event_queue_handle;

rfb_interrupt_event_t *rfb_interrupt_event;
extern void enter_critical_section(void);
extern void leave_critical_section(void);
extern void RfMcu_MemoryGet(uint16_t reg_address, uint8_t *p_rx_data, uint16_t rx_data_length);
/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

uint8_t rfb_send_cmd_to_pci(pci_task_common_queue_t pci_comm_msg)
{
    uint8_t status = FALSE;
    uint32_t remaining_size;

    vPortEnterCritical();

    if (sys_queue_remaining_size(&g_pci_common_handle) > 5)
    {
        remaining_size = sys_queue_remaining_size(&g_pci_tx_cmd_handle);
        if (remaining_size > (NUM_QUEUE_PCI_CMD >> 1))
        {
            if (sys_queue_trysend(&g_pci_common_handle, (void *)&pci_comm_msg) == ERR_OK)
            {
                status =  TRUE;
            }
        }
    }

    vPortExitCritical();
    return status;
}

uint8_t rfb_send_data_to_pci(pci_task_common_queue_t pci_comm_msg)
{
    uint8_t status = FALSE;
    uint32_t remaining_size;

    vPortEnterCritical();
    remaining_size = sys_queue_remaining_size(&g_pci_tx_data_handle);
    if (sys_queue_remaining_size(&g_pci_common_handle) > 5)
    {
        if (remaining_size > (NUM_QUEUE_PCI_DATA >> 1))
        {
            if (sys_queue_trysend(&g_pci_common_handle, (void *)&pci_comm_msg) == ERR_OK)
            {
                status =  TRUE;
            }
        }
        else
        {
            send_data_to_pci_fail(TX_SW_QUEUE_FULL);
        }
    }
    vPortExitCritical();

    return status;
}

RF_MCU_RX_CMDQ_ERROR rfb_event_read(uint8_t *packet_length, uint8_t *event_address)
{
    rfb_event_msg_t rfb_event_msg;

    if (sys_queue_recv(&g_rfb_event_queue_handle, &rfb_event_msg, 2000) == SYS_ARCH_TIMEOUT)
    {
        //err("rfb event wait timeout!!!\n");
        return RF_MCU_RX_CMDQ_ERR_INIT;
    }
    else
    {
        memcpy(event_address, rfb_event_msg.p_data, rfb_event_msg.length);
        *packet_length = rfb_event_msg.length;
        if (rfb_event_msg.length == 0)
        {
            return RF_MCU_RX_CMDQ_NOT_AVAILABLE;
        }
        else
        {
            mem_free(rfb_event_msg.p_data);
            return RF_MCU_RX_CMDQ_GET_SUCCESS;
        }
    }
}

RF_MCU_RXQ_ERROR rfb_comm_rx_data_read(uint8_t *rx_data_address, rfb_rx_ctrl_field_t *rx_control_field)
{
    RF_MCU_RXQ_ERROR RxQueueError = RF_MCU_RXQ_ERR_INIT;
    RfMcu_RxQueueRead(rx_data_address, &RxQueueError);
    RUCI_ENDIAN_CONVERT((uint8_t *)&sRxControlField, RUCI_RX_CONTROL_FIELD);
    /* Crc status , Rssi , and SNR will be put on first three byte of RX queue*/
    memcpy((uint8_t *)rx_control_field, &rx_data_address[2], RUCI_LEN_RX_CONTROL_FIELD - 2);
    return RxQueueError;
}

bool rfb_send_cmd(uint8_t *cmd_address, uint8_t cmd_length)
{
    pci_task_common_queue_t pci_comm_msg;

    pci_comm_msg.pci_msg_tag = PCI_MSG_TX_PCI_CMD;
    pci_comm_msg.pci_msg.param_type.p_pci_msg = mem_malloc(cmd_length);

    rfb_event_msg_t rfb_event_msg;

    while (sys_arch_queue_tryrecv(&g_rfb_event_queue_handle, &rfb_event_msg) == ERR_OK)
    {
        err("legacy RFB cmd confirm event found!!!\n");
    }

    if (pci_comm_msg.pci_msg.param_type.p_pci_msg != NULL)
    {
        memcpy(pci_comm_msg.pci_msg.param_type.p_pci_msg, cmd_address, cmd_length);
        if (rfb_send_cmd_to_pci(pci_comm_msg) == FALSE)
        {
            mem_free(pci_comm_msg.pci_msg.param_type.p_pci_msg);
            //err("rfb cmd send fail!!!\n");
            return false;
        }
        return true;

    }
    return false;
}
/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/



RFB_EVENT_STATUS rfb_comm_frequency_set(uint32_t rf_frequency)
{
    ruci_para_set_rf_frequency_t sSetFrequencyCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_RF_FREQUENCY(&sSetFrequencyCmd, rf_frequency);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sSetFrequencyCmd, RUCI_SET_RF_FREQUENCY);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sSetFrequencyCmd, RUCI_LEN_SET_RF_FREQUENCY) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_RF_FREQUENCY)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_wake_on_radio_set(uint32_t rf_frequency, uint16_t rx_on_time, uint32_t sleep_time)
{
    ruci_para_set_wake_on_radio_t sSetWorCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_WAKE_ON_RADIO(&sSetWorCmd, rf_frequency, rx_on_time, sleep_time);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sSetWorCmd, RUCI_SET_WAKE_ON_RADIO);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sSetWorCmd, RUCI_LEN_SET_WAKE_ON_RADIO) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_WAKE_ON_RADIO)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_single_tone_mode_set(uint8_t single_tone_mode)
{
    ruci_para_set_single_tone_mode_t sStModeEnCmd_t = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_SINGLE_TONE_MODE(&sStModeEnCmd_t, single_tone_mode);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sStModeEnCmd_t, RUCI_SET_SINGLE_TONE_MODE);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sStModeEnCmd_t, RUCI_LEN_SET_SINGLE_TONE_MODE) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_SINGLE_TONE_MODE)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_rx_enable_set(bool rx_continuous, uint32_t rx_timeout)
{
    ruci_para_set_rx_enable_t sRxReqCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_RX_ENABLE(&sRxReqCmd, ((rx_continuous == true) ? RX_ALWAYS_ON_MODE : rx_timeout));

    RUCI_ENDIAN_CONVERT((uint8_t *)&sRxReqCmd, RUCI_SET_RX_ENABLE);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sRxReqCmd, RUCI_LEN_SET_RX_ENABLE) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_RX_ENABLE)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}


RFB_EVENT_STATUS rfb_comm_rf_idle_set(void)
{
    ruci_para_set_rf_idle_t sRfIdleSetCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_RF_IDLE(&sRfIdleSetCmd);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sRfIdleSetCmd, RUCI_SET_RF_IDLE);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sRfIdleSetCmd, RUCI_LEN_SET_RF_IDLE) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_RF_IDLE)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}


RFB_WRITE_TXQ_STATUS rfb_comm_tx_data_send(uint16_t packet_length, uint8_t *tx_data_address, uint8_t mac_control, uint8_t mac_dsn)
{
    ruci_para_set_tx_control_field_t sTxControlField = {0};
    static uint8_t txData[MAX_RF_LEN];
    pci_task_common_queue_t pci_comm_msg;

    SET_RUCI_PARA_SET_TX_CONTROL_FIELD(&sTxControlField, mac_control, mac_dsn);
    sTxControlField.length += packet_length;

    RUCI_ENDIAN_CONVERT((uint8_t *)&sTxControlField, RUCI_SET_TX_CONTROL_FIELD);
    memcpy(&txData[0], (uint8_t *)&sTxControlField, RUCI_LEN_SET_TX_CONTROL_FIELD);
    memcpy(&txData[RUCI_LEN_SET_TX_CONTROL_FIELD], tx_data_address, packet_length);

    pci_comm_msg.pci_msg_tag = PCI_MSG_TX_PCI_DATA;
    pci_comm_msg.pci_msg.param_type.p_pci_msg = mem_malloc(packet_length + RUCI_LEN_SET_TX_CONTROL_FIELD);

    if (pci_comm_msg.pci_msg.param_type.p_pci_msg != NULL)
    {
        //printf("rfb data send 0x%02x%02x%02x %d %d\n", txData[0], txData[1], txData[2], packet_length, packet_length+RUCI_LEN_SET_TX_CONTROL_FIELD);
        memcpy(pci_comm_msg.pci_msg.param_type.p_pci_msg, txData, packet_length + RUCI_LEN_SET_TX_CONTROL_FIELD);

        if (rfb_send_data_to_pci(pci_comm_msg) == FALSE)
        {
            mem_free(pci_comm_msg.pci_msg.param_type.p_pci_msg);
            return RFB_WRITE_TXQ_FULL;
        }
    }
    else
    {
        send_data_to_pci_fail(TX_MALLOC_FAIL);
        return RFB_WRITE_TXQ_FULL;
    }
    return RFB_WRITE_TXQ_SUCCESS;
}

void rfb_comm_init(rfb_interrupt_event_t *_rfb_interrupt_event)
{
    /* Rsgister Interrupt event for application use*/
    rfb_interrupt_event = _rfb_interrupt_event;
    sys_queue_new(&g_rfb_event_queue_handle, RFB_CMD_QUEUE_SIZE, sizeof(rfb_event_msg_t));

}

RFB_EVENT_STATUS rfb_comm_rssi_read(uint8_t *rssi)
{
    ruci_para_get_rssi_t sGetRssiCmd = {0};
    ruci_para_get_rssi_event_t sGetRssiEvent = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;
    uint8_t get_rssi_event_len = 0;
    RF_MCU_RX_CMDQ_ERROR get_rssi_event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_GET_RSSI(&sGetRssiCmd);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGetRssiCmd, RUCI_GET_RSSI);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sGetRssiCmd, RUCI_LEN_GET_RSSI) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    get_rssi_event_status = rfb_event_read(&get_rssi_event_len, (uint8_t *)&sGetRssiEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_GET_RSSI)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGetRssiEvent, RUCI_GET_RSSI_EVENT);
    if (get_rssi_event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_RSP_EVENT_NOT_AVAILABLE;
    }
    if (sGetRssiEvent.sub_header != RUCI_CODE_GET_RSSI_EVENT)
    {
        return RFB_RSP_EVENT_CONTENT_ERROR;
    }
    *rssi = sGetRssiEvent.rssi;

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_agc_set(uint8_t agc_enable, uint8_t lna_gain, uint8_t vga_gain, uint8_t tia_gain)
{
    ruci_para_set_agc_t sAgcSetCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_AGC(&sAgcSetCmd, (agc_enable & 0x01), lna_gain, vga_gain, tia_gain);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sAgcSetCmd, RUCI_SET_AGC);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sAgcSetCmd, RUCI_LEN_SET_AGC) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_AGC)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_rf_sleep_set(bool enable_flag)
{
    ruci_para_set_rf_sleep_t sRfSleepSetCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_RF_SLEEP(&sRfSleepSetCmd, enable_flag);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sRfSleepSetCmd, RUCI_SET_RF_SLEEP);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sRfSleepSetCmd, RUCI_LEN_SET_RF_SLEEP) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_RF_SLEEP)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_fw_version_get(uint32_t *rfb_version)
{
    ruci_para_get_fw_ver_t sGetRfbVerCmd = {0};
    ruci_para_get_fw_ver_event_t sGetRfbVerEvent = {0};
    ruci_para_cmn_cnf_event_t sCmnCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;
    uint8_t get_rfb_ver_event_len = 0;
    RF_MCU_RX_CMDQ_ERROR get_rfb_ver_event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_GET_FW_VER(&sGetRfbVerCmd);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGetRfbVerCmd, RUCI_GET_FW_VER);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sGetRfbVerCmd, RUCI_LEN_GET_FW_VER) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCmnCnfEvent);
    get_rfb_ver_event_status = rfb_event_read(&get_rfb_ver_event_len, (uint8_t *)&sGetRfbVerEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCmnCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCmnCnfEvent.cmn_cmd_subheader != RUCI_CODE_GET_FW_VER)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCmnCnfEvent.status != RFB_CMN_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCmnCnfEvent.status;
    }

    RUCI_ENDIAN_CONVERT((uint8_t *)&sGetRfbVerEvent, RUCI_GET_FW_VER_EVENT);
    if (get_rfb_ver_event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_RSP_EVENT_NOT_AVAILABLE;
    }
    if (sGetRfbVerEvent.sub_header != RUCI_CODE_GET_FW_VER_EVENT)
    {
        return RFB_RSP_EVENT_CONTENT_ERROR;
    }
    *rfb_version = sGetRfbVerEvent.fw_ver;

    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_auto_state_set(bool rx_on_when_idle)
{
    ruci_para_set_rfb_auto_state_t sRfbAutoStateSet = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_RFB_AUTO_STATE(&sRfbAutoStateSet, rx_on_when_idle);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sRfbStateInit, RUCI_SET_RFB_AUTO_STATE);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sRfbAutoStateSet, RUCI_LEN_SET_RFB_AUTO_STATE) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_RFB_AUTO_STATE)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_clock_set(uint8_t modem_type, uint8_t band_type, uint8_t clock_mode)
{
    ruci_para_set_clock_mode_t sRfbClockModeSet = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_CLOCK_MODE(&sRfbClockModeSet, modem_type, band_type, clock_mode);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sRfbClockModeSet, RUCI_SET_CLOCK_MODE);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sRfbClockModeSet, RUCI_LEN_SET_CLOCK_MODE) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_CLOCK_MODE)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_tx_power_set(uint8_t band_type, uint8_t power_index)
{
    ruci_para_set_tx_power_t sTxPwrSetCmd_t;
    ruci_para_cmn_cnf_event_t sCmnCnfEvent;
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_TX_POWER(&sTxPwrSetCmd_t, band_type, power_index);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sTxPwrSetCmd_t, RUCI_SET_TX_POWER);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sTxPwrSetCmd_t, RUCI_LEN_SET_TX_POWER) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCmnCnfEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCmnCnfEvent, RUCI_CMN_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCmnCnfEvent.cmn_cmd_subheader != RUCI_CODE_SET_TX_POWER)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCmnCnfEvent.status != RFB_CMN_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCmnCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_tx_power_set_oqpsk(uint8_t band_type, uint8_t power_index)
{
    ruci_para_set_tx_power_oqpsk_t sTxPwrSetCmd_t;
    ruci_para_cmn_cnf_event_t sCmnCnfEvent;
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_TX_POWER_OQPSK(&sTxPwrSetCmd_t, band_type, power_index);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sTxPwrSetCmd_t, RUCI_SET_TX_POWER_OQPSK);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sTxPwrSetCmd_t, RUCI_LEN_SET_TX_POWER_OQPSK);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCmnCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCmnCnfEvent, RUCI_CMN_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCmnCnfEvent.cmn_cmd_subheader != RUCI_CODE_SET_TX_POWER_OQPSK)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCmnCnfEvent.status != RFB_CMN_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCmnCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_key_set(uint8_t *pKey)
{
    ruci_para_set_rfe_security_t sKeyCmd_t;
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_RFE_SECURITY(&sKeyCmd_t, *pKey, *(pKey + 1), *(pKey + 2), *(pKey + 3), *(pKey + 4)
                                   , *(pKey + 5), *(pKey + 6), *(pKey + 7), *(pKey + 8), *(pKey + 9), *(pKey + 10), *(pKey + 11)
                                   , *(pKey + 12), *(pKey + 13), *(pKey + 14), *(pKey + 15)
                                   , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

    RUCI_ENDIAN_CONVERT((uint8_t *)&sSecurityCmd_t, RUCI_SET_RFE_SECURITY);

    //enter_critical_section();
    if (rfb_send_cmd((uint8_t *)&sKeyCmd_t, RUCI_LEN_SET_RFE_SECURITY) == false)
    {
        return RFB_CNF_EVENT_TX_BUSY;
    }
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    //leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_RFE_SECURITY)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCnfEvent.status != RFB_CMN_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}

RFB_EVENT_STATUS rfb_comm_pta_control_set(uint8_t enable, uint8_t inverse)
{
    ruci_para_set_pta_default_t     sSetPtaDefault;
    ruci_para_cmn_cnf_event_t       sCmnCnfEvent;
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_PTA_DEFAULT(&sSetPtaDefault, enable, inverse);
    RUCI_ENDIAN_CONVERT((uint8_t *)&sSetPtaDefault, RUCI_SET_PTA_DEFAULT);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sSetPtaDefault, RUCI_LEN_SET_PTA_DEFAULT);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCmnCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCmnCnfEvent, RUCI_CMN_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCmnCnfEvent.cmn_cmd_subheader != RUCI_CODE_SET_PTA_DEFAULT)
    {
        return RFB_CNF_EVENT_CONTENT_ERROR;
    }
    if (sCmnCnfEvent.status != RFB_CMN_EVENT_SUCCESS)
    {
        return (RFB_EVENT_STATUS)sCmnCnfEvent.status;
    }
    return RFB_EVENT_SUCCESS;
}
