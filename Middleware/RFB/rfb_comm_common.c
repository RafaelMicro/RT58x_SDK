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
#include "cm3_mcu.h"
#include "ruci.h"
#include "rfb_comm.h"
#include "rfb.h"
#include "rf_common_init.h"
/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/

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
rfb_interrupt_event_t *rfb_interrupt_event;
static rfb_rx_queue_t g_rx_queue;
extern void enter_critical_section(void);
extern void leave_critical_section(void);
extern void RfMcu_MemoryGet(uint16_t reg_address, uint8_t *p_rx_data, uint16_t rx_data_length);
/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

RF_MCU_RX_CMDQ_ERROR rfb_event_read(uint8_t *packet_length, uint8_t *event_address)
{
    RF_MCU_RX_CMDQ_ERROR rx_confirm_error = RF_MCU_RX_CMDQ_ERR_INIT;
    uint8_t state = 0;
    do
    {
        state = (uint8_t)RfMcu_McuStateRead();
        state = state & RF_MCU_STATE_EVENT_DONE;
    } while (RF_MCU_STATE_EVENT_DONE != state);
    RfMcu_HostCmdSet(RF_MCU_STATE_EVENT_DONE);
    state = 0;
    do
    {
        state = (uint8_t)RfMcu_McuStateRead();
        state = state & RF_MCU_STATE_EVENT_DONE;
    } while (0 != state);

    (*packet_length)  = RfMcu_EvtQueueRead(event_address, &rx_confirm_error);


    return rx_confirm_error;
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

void rfb_send_cmd(uint8_t *cmd_address, uint8_t cmd_length)
{

    if (RF_MCU_TX_CMDQ_SET_SUCCESS != RfMcu_CmdQueueSend(cmd_address, cmd_length))
    {
        printf("[E] command queue is FULL\n");
    }
}
/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/

void rfb_isr_handler(uint8_t interrupt_status)
{
    static COMM_SUBSYSTEM_INTERRUPT interrupt_state_value;
    uint32_t pro_grm_cnt;
    uint8_t tx_status, rx_numIdx;
    bool isRemainingRxQ;
    RF_MCU_RXQ_ERROR rxq_status = RF_MCU_RXQ_ERR_INIT;
    static rfb_rx_queue_t *pRxQueue = &g_rx_queue;
    interrupt_state_value.u8 = interrupt_status;

    /* wake up RF to clear interrupt status */
    RfMcu_HostWakeUpMcu();
    if (RfMcu_PowerStateCheck() != 0x03)
    {
        /* FOR LEVEL TRIGGER ONLY, the MCU will keep entering INT if status not cleared */
        printf("[W] PWR state error in rfb_isr_handler\n");
        return;
    }

    if (interrupt_state_value.bf.EVENT_DONE)
    {
        RfMcu_InterruptClear((interrupt_status & 0x01));
    }
    if (interrupt_state_value.bf.TX_DONE)
    {
        /* Read Tx Status */
        tx_status = (uint8_t)RfMcu_McuStateRead();

        /* Clear MCU state by sending host command */
        RfMcu_HostCmdSet((tx_status & 0xF8));
        RfMcu_InterruptClear((interrupt_status & 0x02));
        rfb_interrupt_event->tx_done(tx_status);
    }
    if (interrupt_state_value.bf.RFB_TRAP)
    {
        printf("[Error] RFB Trap !!!\n");
        RfMcu_MemoryGet(0x4008, (uint8_t *)&pro_grm_cnt, 4);
        printf("PC= %X\n", pro_grm_cnt);
        RfMcu_MemoryGet(0x01E0, (uint8_t *)&pro_grm_cnt, 4);
        printf("MAC err status= %X\n", pro_grm_cnt);
        RfMcu_MemoryGet(0x0198, (uint8_t *)&pro_grm_cnt, 4);
        printf("MAC task status= %X\n", pro_grm_cnt);
        RfMcu_MemoryGet(0x0048, (uint8_t *)&pro_grm_cnt, 4);
        printf("BMU err status= %X\n", pro_grm_cnt);
        RfMcu_InterruptClear((interrupt_status & 0x04));
        while (1);
    }
    if (interrupt_state_value.bf.RX_DONE)
    {
        RfMcu_InterruptClear((interrupt_status & 0x20));
        g_rx_queue.rx_num = 0;
        do
        {
            rxq_status = rfb_comm_rx_data_read(pRxQueue->rx_buf[g_rx_queue.rx_num].rx_data, &pRxQueue->rx_buf[g_rx_queue.rx_num].rx_control_field);
            if (rxq_status != RF_MCU_RXQ_GET_SUCCESS)
            {
                return;
            }
            isRemainingRxQ = RfMcu_RxQueueCheck();
            g_rx_queue.rx_num ++;
            if (g_rx_queue.rx_num > MAX_RX_BUFF_NUM)
            {
                return;
            }
        } while (isRemainingRxQ);

        for (rx_numIdx = 0; rx_numIdx < g_rx_queue.rx_num; rx_numIdx++)
            rfb_interrupt_event->rx_done(pRxQueue->rx_buf[rx_numIdx].rx_control_field.Length, pRxQueue->rx_buf[rx_numIdx].rx_data, pRxQueue->rx_buf[rx_numIdx].rx_control_field.CrcStatus,
                                         pRxQueue->rx_buf[rx_numIdx].rx_control_field.Rssi, pRxQueue->rx_buf[rx_numIdx].rx_control_field.Snr);

    }

    if (interrupt_state_value.bf.RTC_WAKEUP)
    {
        rfb_interrupt_event->rtc();
        RfMcu_InterruptClear((interrupt_status & 0x80));
    }
    if (interrupt_state_value.bf.RX_TIMEOUT)
    {
        RfMcu_InterruptClear((interrupt_status & 0x08));
        rfb_interrupt_event->rx_timeout();
    }
    if (interrupt_state_value.bf.MHR_DONE)
    {
        RfMcu_InterruptClear((interrupt_status & 0x10));
    }

}

RFB_EVENT_STATUS rfb_comm_frequency_set(uint32_t rf_frequency)
{
    ruci_para_set_rf_frequency_t sSetFrequencyCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_RF_FREQUENCY(&sSetFrequencyCmd, rf_frequency);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sSetFrequencyCmd, RUCI_SET_RF_FREQUENCY);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sSetFrequencyCmd, RUCI_LEN_SET_RF_FREQUENCY);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

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

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sSetWorCmd, RUCI_LEN_SET_WAKE_ON_RADIO);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

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

RFB_EVENT_STATUS rfb_comm_2ch_scan_frequency_set(uint8_t scan_enable, uint32_t rf_freq1, uint32_t rf_freq2)
{
    ruci_para_set_2ch_scan_frequency_t sSetScanCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_2CH_SCAN_FREQUENCY(&sSetScanCmd, scan_enable, rf_freq1, rf_freq2);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sSetScanCmd, RUCI_SET_2CH_SCAN_FREQUENCY);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sSetScanCmd, RUCI_LEN_SET_2CH_SCAN_FREQUENCY);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_2CH_SCAN_FREQUENCY)
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

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sStModeEnCmd_t, RUCI_LEN_SET_SINGLE_TONE_MODE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

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

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sRxReqCmd, RUCI_LEN_SET_RX_ENABLE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

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


RFB_EVENT_STATUS rfb_comm_rx_reserve_set(uint32_t rx_start_time, uint32_t rx_on_time)
{
    ruci_para_set_rx_reserve_enable_t sRxReqCmd = {0};
    ruci_para_cnf_event_t sCnfEvent = {0};
    uint8_t event_len = 0;
    RF_MCU_RX_CMDQ_ERROR event_status = RF_MCU_RX_CMDQ_ERR_INIT;

    SET_RUCI_PARA_SET_RX_RESERVE_ENABLE(&sRxReqCmd, rx_start_time, rx_on_time);

    RUCI_ENDIAN_CONVERT((uint8_t *)&sRxReqCmd, RUCI_SET_RX_RESERVE_ENABLE);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sRxReqCmd, RUCI_LEN_SET_RX_RESERVE_ENABLE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

    RUCI_ENDIAN_CONVERT((uint8_t *)&sCnfEvent, RUCI_CNF_EVENT);
    if (event_status != RF_MCU_RX_CMDQ_GET_SUCCESS)
    {
        return RFB_CNF_EVENT_NOT_AVAILABLE;
    }
    if (sCnfEvent.pci_cmd_subheader != RUCI_CODE_SET_RX_RESERVE_ENABLE)
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

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sRfIdleSetCmd, RUCI_LEN_SET_RF_IDLE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

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
    /* wake up RF to clear interrupt status */
    RfMcu_HostWakeUpMcu();
    while (RfMcu_PowerStateCheck() != 0x03)
    {
        printf("[TX] Pwr chk\n");
    }
    SET_RUCI_PARA_SET_TX_CONTROL_FIELD(&sTxControlField, mac_control, mac_dsn);
    sTxControlField.length += packet_length;

    RUCI_ENDIAN_CONVERT((uint8_t *)&sTxControlField, RUCI_SET_TX_CONTROL_FIELD);
    memcpy(&txData[0], (uint8_t *)&sTxControlField, RUCI_LEN_SET_TX_CONTROL_FIELD);
    memcpy(&txData[RUCI_LEN_SET_TX_CONTROL_FIELD], tx_data_address, packet_length);

    if (RF_MCU_TXQ_FULL == RfMcu_TxQueueSendById(RF_TX_Q_ID, &txData[0], packet_length + RUCI_LEN_SET_TX_CONTROL_FIELD))
    {
        return RFB_WRITE_TXQ_FULL;
    }
    return RFB_WRITE_TXQ_SUCCESS;

}

void rfb_comm_init(rfb_interrupt_event_t *_rfb_interrupt_event)
{
    /* Rsgister Interrupt event for application use*/
    rfb_interrupt_event = _rfb_interrupt_event;

    rf_common_init_by_fw(RF_FW_LOAD_SELECT_RUCI_CMD, rfb_isr_handler);
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

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sGetRssiCmd, RUCI_LEN_GET_RSSI);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    get_rssi_event_status = rfb_event_read(&get_rssi_event_len, (uint8_t *)&sGetRssiEvent);
    leave_critical_section();

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

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sAgcSetCmd, RUCI_LEN_SET_AGC);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

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

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sRfSleepSetCmd, RUCI_LEN_SET_RF_SLEEP);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

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

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sGetRfbVerCmd, RUCI_LEN_GET_FW_VER);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCmnCnfEvent);
    get_rfb_ver_event_status = rfb_event_read(&get_rfb_ver_event_len, (uint8_t *)&sGetRfbVerEvent);
    leave_critical_section();

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

    RUCI_ENDIAN_CONVERT((uint8_t *)&sRfbAutoStateSet, RUCI_SET_RFB_AUTO_STATE);

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sRfbAutoStateSet, RUCI_LEN_SET_RFB_AUTO_STATE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

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

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sRfbClockModeSet, RUCI_LEN_SET_CLOCK_MODE);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

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

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sTxPwrSetCmd_t, RUCI_LEN_SET_TX_POWER);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCmnCnfEvent);
    leave_critical_section();

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

    enter_critical_section();
    rfb_send_cmd((uint8_t *)&sKeyCmd_t, RUCI_LEN_SET_RFE_SECURITY);
    event_status = rfb_event_read(&event_len, (uint8_t *)&sCnfEvent);
    leave_critical_section();

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
