/**
 * @file rfb_port.c
 * @author
 * @date
 * @brief porting ruci related function for RFB used
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
#include <math.h>
#include "cm3_mcu.h"

#include "rfb_comm.h"
#include "rfb_port.h"
#include "ruci.h"
#include "chip_define.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define ADDR_RFB_STATE                         (0x0198)
#define R_RFB_STATE                             0x08, 0x00000700

#define ADDR_PKT_FMT                           (0x0100)
#define R_CRC_TYPE                             0x04, 0x00000030

#define RFB_DEBUG_PORT_EN (false)
/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
rfb_t g_rfb;
bool rfb_comm_init_done = 0;

#if RFB_FIX_TX_POWER_SUPPORT
/* fix tx power variables */
bool g_fix_power_enable = 0;
uint8_t g_fp_band_type;
uint8_t g_fp_power_index = 24;
sadc_value_t g_fp_threshold_15_dbm[9] = {3500, 3300, 3000, 2700, 2500, 2400, 2300, 2200, 0};
sadc_value_t g_fp_bat_voltage;
sadc_convert_state_t sadc_convert_status = SADC_CONVERT_IDLE;
#endif

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
void  reg_cal(
    uint32_t *reg_value,
    uint8_t bitShift,
    uint32_t bitMask,
    uint32_t data
)
{
    *reg_value &= (~bitMask);
    *reg_value |= ((data << bitShift) & bitMask);
}

void  reg_byte_invert(
    uint32_t reg_value,
    uint8_t bitShift,
    uint32_t bitMask,
    uint8_t *data
)
{
    reg_value &= (bitMask);
    *data = (uint8_t)((reg_value >> bitShift) & 0xFF);
}

void rfb_debug_port_init(void)
{
#if ((CHIP_VERSION == RT58X_MPA) || (CHIP_VERSION == RT58X_MPB))
    outp32(0x40800010, 0x77777777);
    outp32(0x4080003C, (inp32(0x4080003C) | 0x07000000));
#endif
}

#if RFB_FIX_TX_POWER_SUPPORT
void rfb_port_fix_tx_power_check(void)
{
    uint8_t i = 0;
    uint8_t power_index = 23;

    while (Sadc_Channel_Read(SADC_CH_VBAT) != STATUS_SUCCESS)
    {
        printf("Fix Txpower error: fail to read VBAT\n");
    }

    while (g_fp_bat_voltage < g_fp_threshold_15_dbm[i])
    {
        power_index++;
        i++;
    }
    if (g_fp_power_index != power_index)
    {
        g_fp_power_index = power_index;
        if (g_rfb.modem_type == RFB_MODEM_FSK)
        {
            rfb_port_tx_power_set(g_fp_band_type, g_fp_power_index);
        }
        else
        {
            rfb_port_tx_power_set_oqpsk(g_fp_band_type, g_fp_power_index);
        }
    }
}

void Sadc_Int_Callback_Handler(sadc_cb_t *p_cb)
{
    if (p_cb->type == SADC_CB_SAMPLE)
    {
        g_fp_bat_voltage = p_cb->data.sample.value;
    }
}
#endif
/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
RFB_EVENT_STATUS rfb_port_modem_set(rfb_modem_type_t modem, uint8_t band_type)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    if (g_rfb.modem_type == modem)
    {
        return event_status;
    }
    g_rfb.modem_type = modem;

    switch (modem)
    {
#if (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
    case RFB_MODEM_ZIGBEE:
        event_status = rfb_comm_zigbee_initiate();
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_zigbee_initiate fail, status:%d\n", event_status);
        }
        break;
#endif

#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
    case RFB_MODEM_FSK:
        event_status = rfb_comm_fsk_initiate(band_type);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_fsk_initiate fail, status:%d\n", event_status);
        }
        break;
    case RFB_MODEM_OQPSK:
        event_status = rfb_comm_oqpsk_initiate(band_type);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_oqpsk_initiate fail, status:%d\n", event_status);
        }
        break;
#endif

#if (defined RFB_BLE_ENABLED && RFB_BLE_ENABLED == 1)
    case RFB_MODEM_BLE:
        event_status = rfb_comm_clock_set(0, 1, 0); //BLE, 2.4G, 32M clock
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_clock_set fail, status:%d\n", event_status);
        }
        break;
#endif

    default:
        break;
    }

    return event_status;
}

#if (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
RFB_EVENT_STATUS rfb_port_zb_init(rfb_interrupt_event_t *_rfb_interrupt_event)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    rfb_comm_init(_rfb_interrupt_event);

    /*Set the initial modem type*/
    event_status = rfb_port_modem_set(RFB_MODEM_ZIGBEE, BAND_2P4G);

#if (RFB_DEBUG_PORT_EN)
    rfb_debug_port_init();
#endif
    return event_status;
}
#endif

#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
RFB_EVENT_STATUS rfb_port_subg_init(rfb_interrupt_event_t *_rfb_interrupt_event, rfb_keying_type_t keying_mode, uint8_t band_type)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    if (!rfb_comm_init_done)
    {
        rfb_comm_init(_rfb_interrupt_event);
        rfb_comm_init_done = 1;
    }

    /*Set the initial modem type*/
    if (keying_mode == RFB_KEYING_FSK)
    {
        event_status = rfb_port_modem_set(RFB_MODEM_FSK, band_type);
        g_rfb.modem_type = RFB_MODEM_FSK;
    }
    else if (keying_mode == RFB_KEYING_OQPSK)
    {
        event_status = rfb_port_modem_set(RFB_MODEM_OQPSK, band_type);
        g_rfb.modem_type = RFB_MODEM_OQPSK;
    }

#if (RFB_DEBUG_PORT_EN)
    rfb_debug_port_init();
#endif

    return event_status;
}
#endif

#if (defined RFB_BLE_ENABLED && RFB_BLE_ENABLED == 1)
RFB_EVENT_STATUS rfb_port_ble_init(rfb_interrupt_event_t *_rfb_interrupt_event)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    rfb_comm_init(_rfb_interrupt_event);

    /*Set the initial modem type*/
    event_status = rfb_port_modem_set(RFB_MODEM_BLE, BAND_2P4G);
    g_rfb.modem_type = RFB_MODEM_BLE;

#if (RFB_DEBUG_PORT_EN)
    rfb_debug_port_init();
#endif

    return event_status;
}
#endif

RFB_EVENT_STATUS rfb_port_frequency_set(uint32_t rf_frequency)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    event_status = rfb_comm_frequency_set(rf_frequency);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_frequency_set fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_wake_on_radio_set(uint32_t rf_frequency, uint16_t rx_on_time, uint32_t sleep_time)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    /* unit of rx_on_time & sleep_time: us. sleep_time should < 0x400000. */
    if (sleep_time > 0x400000)
    {
        printf("Error: rfb_comm_wake_on_radio_set fail, sleep_time %d is too big\n", sleep_time);
        return RFB_CNF_EVENT_INVALID_CMD;
    }

    event_status = rfb_comm_wake_on_radio_set(rf_frequency, rx_on_time, sleep_time);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_wake_on_radio_set fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_2ch_scan_frequency_set(uint8_t scan_enable, uint32_t rf_freq1, uint32_t rf_freq2)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    event_status = rfb_comm_2ch_scan_frequency_set(scan_enable, rf_freq1, rf_freq2);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_2ch_scan_frequency_set fail, status:%d\n", event_status);
    }

    return event_status;
}

#if (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
bool rfb_port_zb_is_channel_free(uint32_t rf_frequency, uint8_t rssi_threshold)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    uint8_t rssi = 0;
    bool is_channel_free;

    /* Set RF frequency */
    event_status = rfb_comm_frequency_set(rf_frequency);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_frequency_set fail, status:%d\n", event_status);
    }

    /* Enable RX*/
    rfb_port_auto_state_set(true);

    /* Get rssi */
    event_status = rfb_comm_rssi_read(&rssi);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rssi_read fail, status:%d\n", event_status);
    }

    /* Disable RX*/
    rfb_port_auto_state_set(false);

    /* determine channel status*/
    is_channel_free = (rssi > rssi_threshold) ? true : false;

    return is_channel_free;
}
#endif

#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
bool rfb_port_subg_is_channel_free(uint32_t rf_frequency, uint8_t rssi_threshold)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    uint8_t rssi = 0;
    bool is_channel_free;

    /* Init idle state */
    event_status = rfb_comm_rf_idle_set();
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rf_idle_set fail, status:%d\n", event_status);
    }

    /*Init Modem */
    if (rf_frequency >= 900000)
    {
        rfb_port_modem_set(RFB_MODEM_FSK, BAND_SUBG_915M);
    }
    else if (rf_frequency >= 800000)
    {
        rfb_port_modem_set(RFB_MODEM_FSK, BAND_SUBG_868M);
    }
    else if (rf_frequency >= 400000)
    {
        rfb_port_modem_set(RFB_MODEM_FSK, BAND_SUBG_433M);
    }
    else
    {
        rfb_port_modem_set(RFB_MODEM_FSK, BAND_SUBG_315M);
    }

    /* Set RF frequency */
    event_status = rfb_comm_frequency_set(rf_frequency);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_frequency_set fail, status:%d\n", event_status);
    }

    /* Enable RX*/
    event_status = rfb_comm_rx_enable_set(true, 0);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rx_enable_set fail, status:%d\n", event_status);
    }

    /* Get rssi */
    event_status = rfb_comm_rssi_read(&rssi);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rssi_read fail, status:%d\n", event_status);
    }

    /* go back to idle state */
    event_status = rfb_comm_rf_idle_set();
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rf_idle_set fail, status:%d\n", event_status);
    }

    /* determine channel status*/
    is_channel_free = (rssi > rssi_threshold) ? true : false;

    return is_channel_free;
}
#endif

RFB_WRITE_TXQ_STATUS rfb_port_data_send(uint8_t *tx_data_address, uint16_t packet_length, uint8_t InitialCwAckRequest, uint8_t Dsn)
{
    RFB_WRITE_TXQ_STATUS rfb_write_tx_queue_status;

#if RFB_FIX_TX_POWER_SUPPORT
    if (g_fix_power_enable)
    {
        rfb_port_fix_tx_power_check();
    }
#endif
    rfb_write_tx_queue_status = rfb_comm_tx_data_send(packet_length, tx_data_address, InitialCwAckRequest, Dsn);
    if (rfb_write_tx_queue_status != RFB_WRITE_TXQ_SUCCESS)
    {
        printf("[W] Send TX fail\n");
    }

    return rfb_write_tx_queue_status;
}

RFB_EVENT_STATUS rfb_port_tx_continuous_wave_set(uint32_t rf_frequency, tx_power_level_t tx_power)
{
    static uint8_t dummy_tx_data[10];
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    RFB_WRITE_TXQ_STATUS rfb_write_tx_queue_status;
    /*Set RF State to Idle*/
    event_status = rfb_comm_rf_idle_set();
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rf_idle_set fail, status:%d\n", event_status);

        return event_status;
    }

    /*
    * Set channel frequency :
    * For band is subg, units is kHz
    * For band is 2.4g, units is mHz
    */
    event_status = rfb_comm_frequency_set(rf_frequency);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_frequency_set fail, status:%d\n", event_status);

        return event_status;
    }

    event_status = rfb_comm_single_tone_mode_set(2);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_single_tone_mode_set fail, status:%d\n", event_status);

        return event_status;
    }

    rfb_write_tx_queue_status = rfb_comm_tx_data_send(10, &dummy_tx_data[0], 0, 0);
    if (rfb_write_tx_queue_status != RFB_WRITE_TXQ_SUCCESS)
    {
        printf("[W] rfb_comm_tx_data_send fail, status:%d\n", rfb_write_tx_queue_status);

        return event_status;
    }

    return event_status;
}

uint8_t rfb_port_rssi_read(rfb_modem_type_t modem)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    uint8_t rssi;
    event_status = rfb_comm_rssi_read(&rssi);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rssi_read fail, status:%d\n", event_status);
    }
    return rssi;
}



//#if (defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1)
#if ((defined RFB_ZIGBEE_ENABLED && RFB_ZIGBEE_ENABLED == 1) || (defined RFB_15p4_MAC_ENABLED && RFB_15p4_MAC_ENABLED == 1))
RFB_EVENT_STATUS rfb_port_15p4_address_filter_set(uint8_t mac_promiscuous_mode, uint16_t short_source_address, uint32_t long_source_address_0, uint32_t long_source_address_1, uint16_t pan_id, uint8_t isCoordinator)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_15p4_address_filter_set(mac_promiscuous_mode, short_source_address, long_source_address_0, long_source_address_1, pan_id, isCoordinator);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_15p4_address_filter_set fail, status:%d\n", event_status);
    }

    return event_status;
}
/* MULTI-PAN FUNCTIONS */
void rfb_mp_address_filter_set(uint8_t mac_promiscuous_mode, uint16_t short_source_address, uint32_t long_source_address_0, uint32_t long_source_address_1, uint16_t pan_id, uint8_t isCoordinator, uint8_t pan_num)
{
    uint32_t temp_data;
    uint8_t page;
    uint16_t q_addr;

    /* get address of 2nd page in local data q */
    RfMcu_MemoryGet(0x04038, (uint8_t *)&temp_data, 4);
    page = (uint8_t)((temp_data >> 8) & 0xff) + 1;

    /* Start from 0x4000 + (page*64), write the 8th byte */
    q_addr = 0x4000 + (page * 64) + (7 * 4);
    RfMcu_MemorySet(q_addr, (uint8_t *)&pan_num, 1);

    rfb_port_15p4_address_filter_set(mac_promiscuous_mode, short_source_address, long_source_address_0, long_source_address_1, pan_id, isCoordinator);
}
/* MULTI-PAN FUNCTIONS END*/

RFB_EVENT_STATUS rfb_port_15p4_mac_pib_set(uint32_t a_unit_backoff_period, uint32_t mac_ack_wait_duration, uint8_t mac_max_BE, uint8_t mac_max_CSMA_backoffs,
        uint32_t mac_max_frame_total_wait_time, uint8_t mac_max_frame_retries, uint8_t mac_min_BE)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_15p4_mac_pib_set(a_unit_backoff_period, mac_ack_wait_duration, mac_max_BE, mac_max_CSMA_backoffs, mac_max_frame_total_wait_time,
                   mac_max_frame_retries, mac_min_BE);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_15p4_mac_pib_set fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_15p4_phy_pib_set(uint16_t a_turnaround_time, uint8_t phy_cca_mode, uint8_t phy_cca_threshold, uint16_t phy_cca_duration)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_15p4_phy_pib_set(a_turnaround_time, phy_cca_mode, phy_cca_threshold, phy_cca_duration);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_15p4_phy_pib_set fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_15p4_auto_ack_set(uint8_t auto_ack_enable)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_15p4_auto_ack_set(auto_ack_enable);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_15p4_auto_ack_set fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_15p4_pending_bit_set(uint8_t pending_bit_enable)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_15p4_pending_bit_set(pending_bit_enable);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_15p4_pending_bit_set fail, status:%d\n", event_status);
    }

    return event_status;
}
#endif

RFB_EVENT_STATUS rfb_port_auto_state_set(bool rxOnWhenIdle)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_auto_state_set(rxOnWhenIdle);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_auto_state_set fail, status:%d\n", event_status);
    }

    return event_status;
}

uint32_t rfb_port_version_get(void)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    uint32_t fwVer;
    event_status = rfb_comm_fw_version_get(&fwVer);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_version_get fail, status:%d\n", event_status);
    }
    return fwVer;
}

#if (defined RFB_SUBG_ENABLED && RFB_SUBG_ENABLED == 1)
RFB_EVENT_STATUS rfb_port_subg_rx_config_set(uint8_t data_rate, uint16_t preamble_len, fsk_mod_t mod_idx, crc_type_t crc_type,
        whiten_enable_t whiten_enable, uint32_t rx_timeout, bool rx_continuous,
        uint8_t filter_type)

{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    g_rfb.rx_continuous = rx_continuous;
    if (rx_continuous == false)
    {
        g_rfb.rx_timeout = rx_timeout;
    }

    if (g_rfb.modem_type == RFB_MODEM_FSK)
    {
        /*Set Gfsk data rate and modulation index*/
        event_status = rfb_comm_fsk_modem_set(data_rate, mod_idx);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_fsk_modem_set fail, status:%d\n", event_status);

            return event_status;
        }

        /*Set fsk crc type and whitening*/
        event_status = rfb_comm_fsk_mac_set(crc_type, whiten_enable);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_fsk_mac_set fail, status:%d\n", event_status);

            return event_status;
        }

        /* For subg, the preamble length is 8 bytes and preamble type is 10101010*/
        event_status = rfb_comm_fsk_preamble_set(preamble_len);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_fsk_preamble_set fail, status:%d\n", event_status);

            return event_status;
        }

        /* For subg, the sfd length is 2 bytes and sfd content is 0x00007209*/
        event_status = rfb_comm_fsk_sfd_set(0x00007209);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_fsk_sfd_set fail, status:%d\n", event_status);

            return event_status;
        }

        event_status = rfb_comm_fsk_type_set(filter_type);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_fsk_type_set fail, status:%d\n", event_status);

            return event_status;
        }
    }
    else if (g_rfb.modem_type == RFB_MODEM_OQPSK)
    {
        /*Set Oqpsk data rate and modulation index*/
        event_status = rfb_comm_oqpsk_modem_set(data_rate);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_oqpsk_modem_set fail, status:%d\n", event_status);

            return event_status;
        }

        /*Set oqpsk crc type and whitening*/
        event_status = rfb_comm_oqpsk_mac_set(crc_type, whiten_enable);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_oqpsk_mac_set fail, status:%d\n", event_status);

            return event_status;
        }

        /* For subg, the preamble length is 8 bytes and preamble type is 10101010*/
        /*
        event_status = rfb_comm_oqpsk_preamble_set(preamble_len);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_oqpsk_preamble_set fail, status:%d\n", event_status);
        }
        */
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_subg_tx_config_set(tx_power_level_t tx_power, uint8_t data_rate, uint16_t preamble_len, fsk_mod_t mod_idx,
        crc_type_t crc_type, whiten_enable_t whiten_enable, uint8_t filter_type)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    if (g_rfb.modem_type == RFB_MODEM_FSK)
    {
        /*Set Gfsk data rate and modulation index*/
        event_status = rfb_comm_fsk_modem_set(data_rate, mod_idx);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_fsk_modem_set fail, status:%d\n", event_status);

            return event_status;
        }

        /*Set fsk crc type and whitening*/
        event_status = rfb_comm_fsk_mac_set(crc_type, whiten_enable);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_fsk_mac_set fail, status:%d\n", event_status);

            return event_status;
        }

        /* For subg, the preamble length is 8 bytes and preamble type is 10101010*/
        event_status = rfb_comm_fsk_preamble_set(preamble_len);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_fsk_preamble_set fail, status:%d\n", event_status);

            return event_status;
        }

        /* For subg, the sfd length is 2 bytes and sfd content is 0x00007209*/
        event_status = rfb_comm_fsk_sfd_set(0x00007209);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_fsk_sfd_set fail, status:%d\n", event_status);

            return event_status;
        }

        event_status = rfb_comm_fsk_type_set(filter_type);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_fsk_type_set fail, status:%d\n", event_status);

            return event_status;
        }
    }
    else if (g_rfb.modem_type == RFB_MODEM_OQPSK)
    {
        /*Set Oqpsk data rate and modulation index*/
        event_status = rfb_comm_oqpsk_modem_set(data_rate);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_oqpsk_modem_set fail, status:%d\n", event_status);

            return event_status;
        }

        /*Set oqpsk crc type and whitening*/
        event_status = rfb_comm_oqpsk_mac_set(crc_type, whiten_enable);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_oqpsk_mac_set fail, status:%d\n", event_status);

            return event_status;
        }

        /* For subg, the preamble length is 8 bytes and preamble type is 10101010*/
        /*
        event_status = rfb_comm_oqpsk_preamble_set(preamble_len);
        if (event_status != RFB_EVENT_SUCCESS)
        {
            printf("[W] rfb_comm_oqpsk_preamble_set fail, status:%d\n", event_status);
        }
        */
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_sleep_set(void)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_rf_sleep_set(true);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rf_sleep_set fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_idle_set(void)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    event_status = rfb_comm_rf_sleep_set(false);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rf_sleep_set fail, status:%d\n", event_status);

        return event_status;
    }

    event_status = rfb_comm_rf_idle_set();
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rf_idle_set fail, status:%d\n", event_status);

        return event_status;
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_rx_start(void)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    event_status = rfb_comm_rf_sleep_set(false);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rf_sleep_set fail, status:%d\n", event_status);

        return event_status;
    }

    event_status = rfb_comm_rx_enable_set(g_rfb.rx_continuous, g_rfb.rx_timeout);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rx_enable_set fail, status:%d\n", event_status);

        return event_status;
    }

    return event_status;
}
#endif

#if (defined RFB_BLE_ENABLED && RFB_BLE_ENABLED == 1)

RFB_EVENT_STATUS rfb_port_ble_modem_set(uint8_t data_rate, uint8_t coded_scheme)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_ble_modem_set(data_rate, coded_scheme);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_ble_modem_set fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_ble_mac_set(uint32_t sfd_content, uint8_t whitening_en, uint8_t whitening_init_value, uint32_t crc_init_value)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_ble_mac_set(sfd_content, whitening_en, whitening_init_value, crc_init_value);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_ble_mac_set fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_rx_start(void)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;

    event_status = rfb_comm_rf_sleep_set(false);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rf_sleep_set fail, status:%d\n", event_status);

        return event_status;
    }

    event_status = rfb_comm_rx_enable_set(true, 0xFFFFFFFF);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_comm_rx_enable_set fail, status:%d\n", event_status);

        return event_status;
    }

    return event_status;
}
#endif

RFB_EVENT_STATUS rfb_port_tx_power_set(uint8_t band_type, uint8_t power_index)
{
    /* Band type 0: 2.4GHz, 1: Sub-1GHz band 0, 2: Sub-1GHz band 1, 3: Sub-1GHz band 2 */
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_tx_power_set(band_type, power_index);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_tx_power_set fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_tx_power_set_oqpsk(uint8_t band_type, uint8_t power_index)
{
    /* Band type 0: 2.4GHz, 1: Sub-1GHz band 0, 2: Sub-1GHz band 1, 3: Sub-1GHz band 2 */
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_tx_power_set_oqpsk(band_type, power_index);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_tx_power_set_oqpsk fail, status:%d\n", event_status);
    }

    return event_status;
}

#if RFB_FIX_TX_POWER_SUPPORT
RFB_EVENT_STATUS rfb_port_fix_15dbm_tx_power_set(bool enable, uint8_t band_type)
{
    if (band_type != BAND_SUBG_433M) // only support SUBG 433M now
    {
        printf("[W] rfb_port_fix_15dbm_tx_power_set fail, unsupported band type: %d\n", band_type);
        return RFB_CNF_EVENT_INVALID_CMD;
    }
    g_fix_power_enable = enable;
    g_fp_band_type = band_type;

    if (g_fix_power_enable)
    {
        if (g_rfb.modem_type == RFB_MODEM_FSK)
        {
            rfb_port_tx_power_set(g_fp_band_type, g_fp_power_index);
        }
        else if (g_rfb.modem_type == RFB_MODEM_OQPSK)
        {
            rfb_port_tx_power_set_oqpsk(g_fp_band_type, g_fp_power_index);
        }
        else
        {
            printf("Fix Txpower error: unsupported band type: %d\n", g_rfb.modem_type);
            g_fix_power_enable = 0;
            return RFB_CNF_EVENT_INVALID_CMD;
        }
        Sadc_Config_Enable(SADC_RES_12BIT, SADC_OVERSAMPLE_256, Sadc_Int_Callback_Handler);
        Sadc_Compensation_Init(1);
    }
    return RFB_EVENT_SUCCESS;
}
#endif

RFB_EVENT_STATUS rfb_port_15p4_src_addr_match_ctrl(uint8_t ctrl_type)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_15p4_src_match_ctrl(ctrl_type);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_15p4_src_addr_match_ctrl fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_15p4_short_addr_ctrl(uint8_t ctrl_type, uint8_t *short_addr)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_15p4_src_match_short_entry(ctrl_type, short_addr);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_15p4_short_addr_ctrl fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_15p4_extend_addr_ctrl(uint8_t ctrl_type, uint8_t *extend_addr)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_15p4_src_match_extended_entry(ctrl_type, extend_addr);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_15p4_short_addr_ctrl fail, status:%d\n", event_status);
    }

    return event_status;
}

RFB_EVENT_STATUS rfb_port_key_set(uint8_t *key_addr)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_key_set(key_addr);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_key_set fail, status:%d\n", event_status);
    }

    return event_status;
}

/* MULTI-PAN FUNCTIONS */
void rfb_mp_src_addr_match_ctrl(uint8_t ctrl_type, uint8_t pan_num)
{
    uint32_t temp_data;
    uint8_t page;
    uint16_t q_addr;

    /* get address of 2nd page in local data q */
    RfMcu_MemoryGet(0x04038, (uint8_t *)&temp_data, 4);
    page = (uint8_t)((temp_data >> 8) & 0xff) + 1;

    /* Start from 0x4000 + (page*64), write the 8th byte */
    q_addr = 0x4000 + (page * 64) + (7 * 4);
    RfMcu_MemorySet(q_addr, (uint8_t *)&pan_num, 1);

    rfb_port_15p4_src_addr_match_ctrl(ctrl_type);
}

void rfb_mp_short_addr_ctrl(uint8_t ctrl_type, uint8_t *short_addr, uint8_t pan_num)
{
    uint32_t temp_data;
    uint8_t page;
    uint16_t q_addr;

    /* get address of 2nd page in local data q */
    RfMcu_MemoryGet(0x04038, (uint8_t *)&temp_data, 4);
    page = (uint8_t)((temp_data >> 8) & 0xff) + 1;

    /* Start from 0x4000 + (page*64), write the 8th byte */
    q_addr = 0x4000 + (page * 64) + (7 * 4);
    RfMcu_MemorySet(q_addr, (uint8_t *)&pan_num, 1);

    rfb_port_15p4_short_addr_ctrl(ctrl_type, short_addr);
}

void rfb_mp_extend_addr_ctrl(uint8_t ctrl_type, uint8_t *extend_addr, uint8_t pan_num)
{
    uint32_t temp_data;
    uint8_t page;
    uint16_t q_addr;

    /* get address of 2nd page in local data q */
    RfMcu_MemoryGet(0x04038, (uint8_t *)&temp_data, 4);
    page = (uint8_t)((temp_data >> 8) & 0xff) + 1;

    /* Start from 0x4000 + (page*64), write the 8th byte */
    q_addr = 0x4000 + (page * 64) + (7 * 4);
    RfMcu_MemorySet(q_addr, (uint8_t *)&pan_num, 1);

    rfb_port_15p4_extend_addr_ctrl(ctrl_type, extend_addr);
}

void rfb_mp_key_set(uint8_t *key_addr, uint8_t pan_num)
{
    uint32_t temp_data;
    uint8_t page;
    uint16_t q_addr;

    /* get address of 2nd page in local data q */
    RfMcu_MemoryGet(0x04038, (uint8_t *)&temp_data, 4);
    page = (uint8_t)((temp_data >> 8) & 0xff) + 1;

    /* Start from 0x4000 + (page*64), write the 8th byte */
    q_addr = 0x4000 + (page * 64) + (7 * 4);
    RfMcu_MemorySet(q_addr, (uint8_t *)&pan_num, 1);

    rfb_port_key_set(key_addr);
}
/* MULTI-PAN FUNCTIONS END*/

RFB_EVENT_STATUS rfb_port_csl_receiver_ctrl(uint8_t csl_receiver_ctrl, uint16_t csl_period)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_15p4_csl_receiver_ctrl(csl_receiver_ctrl, csl_period);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_csl_receiver_ctrl fail, status:%d\n", event_status);
    }

    return event_status;
}

uint8_t rfb_port_csl_accuracy_get(void)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    uint8_t csl_accuracy;
    event_status = rfb_comm_15p4_csl_accuracy_get(&csl_accuracy);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_csl_accuracy_get fail, status:%d\n", event_status);
    }
    return csl_accuracy;
}

uint8_t rfb_port_csl_uncertainty_get(void)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    uint8_t csl_uncertainty;
    event_status = rfb_comm_15p4_csl_uncertainty_get(&csl_uncertainty);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_csl_uncertainty_get fail, status:%d\n", event_status);
    }
    return csl_uncertainty;
}

RFB_EVENT_STATUS rfb_port_csl_sample_time_update(uint32_t csl_sample_time)
{
    RFB_EVENT_STATUS event_status = RFB_EVENT_SUCCESS;
    event_status = rfb_comm_15p4_csl_sample_time_update(csl_sample_time);
    if (event_status != RFB_EVENT_SUCCESS)
    {
        printf("[W] rfb_port_csl_sample_time_update fail, status:%d\n", event_status);
    }

    return event_status;
}

uint32_t rfb_port_rtc_time_read(void)
{
    uint32_t rtc_time, rtc_time_tmp;
    uint32_t rtc_trigger = 0x200;

    RfMcu_MemorySet(0x400, (uint8_t *)&rtc_trigger, 4);
    do
    {
        RfMcu_MemoryGet(0x414, (uint8_t *)&rtc_time_tmp, 4);
        RfMcu_MemoryGet(0x414, (uint8_t *)&rtc_time, 4);
    } while (rtc_time_tmp != rtc_time);
#if 0
    if (RfMcu_PowerStateCheck() != 0x03)
    {
        printf("[W] rfb_port_rtc_time_read is in sleep state\n");
        return 0;
    }
#endif
    return rtc_time;
}

uint8_t rfb_port_ack_packet_read(uint8_t *rx_data_address, uint8_t *rx_time_address, bool is2bytephr)
{
    uint32_t temp_data;
    uint16_t temp_addr;
    uint8_t packet_length;
    uint8_t ret_len;
    uint8_t page;
    uint8_t i;
    uint8_t tmp_rtc_time;
    bool skip = is2bytephr;

    /* get address of local data q */
    RfMcu_MemoryGet(0x4038, (uint8_t *)&temp_data, 4);
    page = (uint8_t)((temp_data >> 8) & 0xff);

    /* Start from 0x4000 + (page*64), the 8th bytes is packet len */
    temp_addr = 0x4000 + (page * 64) + 4;
    RfMcu_MemoryGet(temp_addr, (uint8_t *)&temp_data, 4);
    temp_addr += is2bytephr ? 4 : 0;
    RfMcu_MemoryGet(temp_addr, (uint8_t *)&temp_data, 4);
    packet_length = is2bytephr ? (uint8_t)(temp_data & 0xff) : (uint8_t)((temp_data >> 24) & 0xff);
    ret_len = packet_length;
    temp_addr += is2bytephr ? 0 : 4;

    while (packet_length > 0)
    {
        RfMcu_MemoryGet(temp_addr, (uint8_t *)&temp_data, 4);
        for (i = 0; i < 4; i++)
        {
            if (packet_length > 0)
            {
                if (skip)
                {
                    skip = 0;
                }
                else
                {
                    *rx_data_address = (uint8_t)((temp_data >> (8 * i)) & 0xff);
                    rx_data_address += sizeof(uint8_t);
                    packet_length--;
                }
            }
        }
        temp_addr += 4;
    }

    /* 2nd page of local data q, the 6th rtc_time q is for ack */
    page += 1;
    temp_addr = 0x4000 + (page * 64) + (5 * 4);
    RfMcu_MemoryGet(temp_addr, (uint8_t *)rx_time_address, 4);
    tmp_rtc_time = rx_time_address[3];
    rx_time_address[3] = rx_time_address[0];
    rx_time_address[0] = tmp_rtc_time;
    tmp_rtc_time = rx_time_address[1];
    rx_time_address[1] = rx_time_address[2];
    rx_time_address[2] = tmp_rtc_time;
    return ret_len;
}

uint32_t rfb_port_rx_rtc_time_get(uint8_t rx_cnt)
{
    uint32_t rtc_time;
    uint32_t temp_data;
    uint8_t page;
    uint16_t q_addr;

    /* get address of 2nd page in local data q */
    RfMcu_MemoryGet(0x04038, (uint8_t *)&temp_data, 4);
    page = (uint8_t)((temp_data >> 8) & 0xff) + 1;

    /* Start from 0x4000 + (page*64), valid rx_cnt value: 0~4 */
    q_addr = 0x4000 + (page * 64) + (rx_cnt * 4);
    RfMcu_MemoryGet(q_addr, (uint8_t *)&rtc_time, 4);

    rtc_time = (((rtc_time & 0xFF000000) >> 24) | ((rtc_time & 0x00FF0000) >> 8) | ((rtc_time & 0x0000FF00) << 8) | ((rtc_time & 0x000000FF) << 24));

    return rtc_time;
}

uint8_t rfb_port_current_channel_get(uint8_t rx_cnt)
{
    uint8_t channel_tmp;
    uint32_t temp_data;
    uint8_t page;
    uint16_t q_addr;


    /* get address of 2nd page in local data q */
    RfMcu_MemoryGet(0x04038, (uint8_t *)&temp_data, 4);
    page = (uint8_t)((temp_data >> 8) & 0xff) + 1;

    /* Start from 0x4000 + (page*64), the 9th 4-byte*/
    if (rx_cnt < 4)
    {
        q_addr = 0x4000 + (page * 64) + (9 * 4);
        RfMcu_MemoryGet(q_addr, (uint8_t *)&temp_data, 4);
        channel_tmp = (temp_data >> (8 * rx_cnt)) & 0xff;
    }
    else
    {
        q_addr = 0x4000 + (page * 64) + (10 * 4);
        RfMcu_MemoryGet(q_addr, (uint8_t *)&temp_data, 4);
        channel_tmp = temp_data & 0xff;
    }

    return channel_tmp;
}

uint32_t rfb_port_frame_counter_get(void)
{
    uint32_t frame_counter;
    uint32_t temp_data;
    uint8_t page;
    uint16_t q_addr;

    /* get address of 2nd page in local data q */
    RfMcu_MemoryGet(0x04038, (uint8_t *)&temp_data, 4);
    page = (uint8_t)((temp_data >> 8) & 0xff) + 1;

    /* Start from 0x4000 + (page*64), valid rx_cnt value: 0~4 */
    q_addr = 0x4000 + (page * 64) + (8 * 4);
    RfMcu_MemoryGet(q_addr, (uint8_t *)&frame_counter, 4);

    frame_counter = (((frame_counter & 0xFF000000) >> 24) | ((frame_counter & 0x00FF0000) >> 8) | ((frame_counter & 0x0000FF00) << 8) | ((frame_counter & 0x000000FF) << 24));

    return frame_counter;
}
