/**
 * @file rfb_sample.c
 * @author
 * @date
 * @brief Brief single line description use for indexing
 *
 * More detailed description can go here
 *
 *
 * @see http://
 */
/**************************************************************************************************
*    INCLUDES
*************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "radio.h"
#include "rfb_sample.h"
#include "rfb.h"
#include "rfb_port.h"
#include "mac_frame_gen.h"

#include "bsp_led.h"
/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/
#define SUBG_MAC (true)


/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define RUCI_HEADER_LENGTH      (1)
#define RUCI_SUB_HEADER_LENGTH  (1)
#define RUCI_LENGTH             (2)
#define RUCI_PHY_STATUS_LENGTH  (3)
#define RX_CONTROL_FIELD_LENGTH (RUCI_HEADER_LENGTH+RUCI_SUB_HEADER_LENGTH+RUCI_LENGTH+RUCI_PHY_STATUS_LENGTH)
#define RX_STATUS_LENGTH        (5)
#define FSK_PHR_LENGTH          (2)
#define OQPSK_PHR_LENGTH        (1)
#define CRC16_LENGTH            (2)
#define CRC32_LENGTH            (4)
#define FSK_RX_HEADER_LENGTH    (RX_CONTROL_FIELD_LENGTH + FSK_PHR_LENGTH)
#define OQPSK_RX_HEADER_LENGTH  (RX_CONTROL_FIELD_LENGTH + OQPSK_PHR_LENGTH)
#define RX_APPEND_LENGTH        (RX_STATUS_LENGTH + CRC16_LENGTH)
#define FSK_RX_LENGTH           (FSK_MAX_RF_LEN - FSK_RX_HEADER_LENGTH - RX_APPEND_LENGTH)  //2047
#define OQPSK_RX_LENGTH         (OQPSK_MAX_RF_LEN - OQPSK_RX_HEADER_LENGTH - RX_APPEND_LENGTH)  //127
#define PHY_MIN_LENGTH          (3)
#define PRBS9_LENGTH            (255)
#if (SUBG_MAC)
#define A_TURNAROUND_TIMR             1000;
#define A_UNIT_BACKOFF_PERIOD         320;
#define MAC_ACK_WAIT_DURATION         16000//2000 // non-beacon mode; 864 for beacon mode
#define MAC_MAX_BE                    5
#define MAC_MAX_FRAME_TOTAL_WAIT_TIME 16416
#define MAC_MAX_FRAME_RETRIES         3
#define MAC_MAX_CSMACA_BACKOFFS       4
#define MAC_MIN_BE                    3
#endif
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
extern bool RF_Rx_Switch;
bool RF_Time = 0;
extern RFB_EVENT_STATUS rfb_port_auto_state_set(bool rxOnWhenIdle);
extern RFB_EVENT_STATUS rfb_port_tx_power_set(uint8_t band_type, uint8_t power_index);
extern RFB_EVENT_STATUS rfb_port_tx_power_set_oqpsk(uint8_t band_type, uint8_t power_index);
/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
rfb_interrupt_event_t struct_rfb_interrupt_event;
extern rfb_t g_rfb;

/* g_rx_total_count = g_crc_success_count + g_crc_fail_count*/
uint32_t             g_crc_success_count;
uint32_t             g_crc_fail_count;
uint32_t             g_rx_total_count;
uint32_t             g_rx_total_count_last; // last rx count
uint32_t             g_rx_timeout_count;
/* g_tx_total_count = g_tx_success_Count + g_tx_fail_Count*/
uint16_t             g_tx_total_count;
uint16_t             g_tx_fail_Count;
uint16_t             g_tx_success_Count;
uint32_t             g_tx_csmaca_fail_cnt;
uint32_t             g_tx_no_ack_cnt;
uint32_t             g_tx_fail_cnt;

/* Burst TX test target*/
uint16_t             g_tx_count_target;

/* Use PRBS9 as data content*/
const uint8_t        g_Prbs9Content[] = PRBS9_CONTENT;

/* Rx buffer to store data from RFB*/
uint8_t              g_prbs9_buf[FSK_RX_LENGTH];

/* TX length for TX transmit test*/
uint16_t             g_tx_len;

/* TX/RX done finish flag*/
bool                 g_tx_done = true;
bool                 g_rx_done = true;

/* Supported frequency lists for gpio 31, 30, 29, 28, 23, 14, 9
   User can modify content of g_freq_support to change supported frequency. */
uint32_t             g_freq_support[7] = {903000, 907000, 911000, 915000, 919000, 923000, 927000}; //Unit: kHz

rfb_subg_ctrl_t    *g_rfb_ctrl;

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
static void _timer_isr_handler(uint32_t timer_id)
{
    bsp_led_toggle(BSP_LED_1);
}
void rfb_rx_done(uint16_t ruci_packet_length, uint8_t *rx_data_address, uint8_t crc_status, uint8_t rssi, uint8_t snr)
{

    Timer_Stop(3);
    RF_Time = false;
    bsp_led_on(BSP_LED_1);


#if (!SUBG_MAC)
    uint16_t i;
    uint8_t header_length = ((g_rfb.modem_type == RFB_MODEM_FSK) ? FSK_RX_HEADER_LENGTH : OQPSK_RX_HEADER_LENGTH);
#endif
    uint16_t rx_data_len;
    uint8_t phr_length = ((g_rfb.modem_type == RFB_MODEM_FSK) ? FSK_PHR_LENGTH : OQPSK_PHR_LENGTH);
    g_rx_total_count++;
    if (crc_status == 0)
    {
        /* Calculate PHY payload length*/
        rx_data_len = ruci_packet_length - (RUCI_PHY_STATUS_LENGTH + phr_length + RX_APPEND_LENGTH);
#if (!SUBG_MAC)
        /* Verify data content*/
        for (i = 0; i < rx_data_len; i++)
        {
            if (g_prbs9_buf[i] != *(rx_data_address + header_length + i))
            {
                printf("[E] data content error\n");
            }
        }
        printf("\n");
#endif
        g_crc_success_count ++;
    }
    else
    {
        g_crc_fail_count ++;
    }
    g_rx_done = true;
    printf("RX (len:%d) done, Success:%d Fail:%d\n", rx_data_len, g_crc_success_count, g_crc_fail_count);
}

void rfb_tx_done(uint8_t tx_status)
{
    g_tx_total_count++;
    g_tx_done = true;
#if (SUBG_MAC)
    /* tx_status =
    0x00: TX success
    0x40: TX success and ACK is received
    0x80: TX success, ACK is received, and frame pending is true
    */
    if ((tx_status != 0) && (tx_status != 0x40) && (tx_status != 0x80))
    {
        g_tx_fail_Count ++;
        //printf("Tx done Fail:%d Status:%X\n", g_tx_fail_Count, tx_status);
        if (tx_status == 0x10)
        {
            g_tx_csmaca_fail_cnt++;
        }
        else if (tx_status == 0x20)
        {
            g_tx_no_ack_cnt++;
        }
        else if (tx_status == 0x08)
        {
            g_tx_fail_cnt++;
        }
    }
    printf("Tx (len:%d)done total:%d Fail:%d CaFail:%d NoAck:%d TxFail%d \n", g_tx_len, g_tx_total_count, g_tx_fail_Count, g_tx_csmaca_fail_cnt, g_tx_no_ack_cnt, g_tx_fail_cnt);
#else
    printf("TX (len:%d) done total:%d Fail:%d\n", g_tx_len, g_tx_total_count, g_tx_fail_Count);
#endif


}

void rfb_rx_timeout(void)
{
    g_rx_timeout_count ++;
    g_rx_done = true;
    printf("RX timeout:%d\n", g_rx_timeout_count);
}

void rfb_tx_init(uint8_t data_rate, rfb_keying_type_t keying_mode, uint8_t filter_type)
{
    /*Set RF State to Idle*/
    g_rfb_ctrl->idle_set();

    if (keying_mode == RFB_KEYING_FSK)
    {
        g_rfb_ctrl->init(&struct_rfb_interrupt_event, keying_mode, BAND_SUBG_915M);
    }
    else
    {
        g_rfb_ctrl->init(&struct_rfb_interrupt_event, keying_mode, BAND_OQPSK_SUBG_915M);
    }

    /*Set TX config*/
    g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, data_rate, 8, MOD_1, CRC_16, WHITEN_DISABLE, filter_type);

    /*
    * Set channel frequency :
    * For band is subg, units is kHz
    * For band is 2.4g, units is mHz
    */
    if (gpio_pin_get(31) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[0]);
    }
    else if (gpio_pin_get(30) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[1]);
    }
    else if (gpio_pin_get(29) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[2]);
    }
    else if (gpio_pin_get(28) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[3]);
    }
    else if (gpio_pin_get(23) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[4]);
    }
    else if (gpio_pin_get(14) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[5]);
    }
    else if (gpio_pin_get(9) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[6]);
    }
    else
    {
        g_rfb_ctrl->frequency_set(g_freq_support[0]);
    }


    if (keying_mode == RFB_KEYING_FSK)
    {
        rfb_port_tx_power_set(BAND_SUBG_915M, 30);
    }
    else
    {
        rfb_port_tx_power_set_oqpsk(BAND_SUBG_915M, 30);
    }

    g_tx_len = PHY_MIN_LENGTH;
}

void rfb_rx_init(uint32_t rx_timeout_timer, bool rx_continuous, uint8_t data_rate, rfb_keying_type_t keying_mode, uint8_t filter_type)
{

    /*Set RF State to Idle*/
    g_rfb_ctrl->idle_set();
    if (keying_mode == RFB_KEYING_FSK)
    {
        g_rfb_ctrl->init(&struct_rfb_interrupt_event, keying_mode, BAND_SUBG_915M);
    }
    else
    {
        g_rfb_ctrl->init(&struct_rfb_interrupt_event, keying_mode, BAND_OQPSK_SUBG_915M);
    }


    /*Set RX config*/
    if (rx_continuous == true)
        g_rfb_ctrl->rx_config_set(data_rate, 8, MOD_1, CRC_16,
                                  WHITEN_DISABLE, 0, rx_continuous, filter_type);
    else
        g_rfb_ctrl->rx_config_set(data_rate, 8, MOD_1, CRC_16,
                                  WHITEN_DISABLE, rx_timeout_timer, rx_continuous, filter_type);
    /* Set channel frequency */
    if (gpio_pin_get(31) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[0]);
    }
    else if (gpio_pin_get(30) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[1]);
    }
    else if (gpio_pin_get(29) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[2]);
    }
    else if (gpio_pin_get(28) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[3]);
    }
    else if (gpio_pin_get(23) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[4]);
    }
    else if (gpio_pin_get(14) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[5]);
    }
    else if (gpio_pin_get(9) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[6]);
    }
    else
    {
        g_rfb_ctrl->frequency_set(g_freq_support[0]);
    }
}

bool burst_tx_abort(void)
{


    if (g_tx_total_count != g_tx_count_target)
    {
        return false;
    }

    return true;
}

bool tx_done_check(void)
{
    return g_tx_done;
}

bool rx_done_check(void)
{
    return g_rx_done;
}
void wait(uint32_t delay_cnt)
{
    while (delay_cnt--);
}

void data_gen(uint8_t *pbuf, uint16_t len)
{
    uint8_t  idx ;
    for (idx = 0; idx < (len >> 8); idx++)
    {
        memcpy(pbuf + (idx << 8), &(g_Prbs9Content[0]), 0x100);
    }
    if (len & 0xFF)
    {
        memcpy(pbuf + (idx << 8), &(g_Prbs9Content[0]), (len & 0xFF));
    }
}

/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
void rfb_sample_init(uint8_t RfbPciTestCase, uint8_t data_rate)
{
    uint32_t FwVer;
    timer_config_mode_t cfg;

#if (SUBG_MAC)
    /* MAC PIB Parameters */
    uint32_t a_unit_backoff_period = A_UNIT_BACKOFF_PERIOD;
    uint32_t mac_ack_wait_duration = MAC_ACK_WAIT_DURATION;
    uint8_t mac_max_BE = MAC_MAX_BE;
    uint8_t mac_max_CSMA_backoffs = MAC_MAX_CSMACA_BACKOFFS;
    uint32_t mac_max_frame_total_wait_time = MAC_MAX_FRAME_TOTAL_WAIT_TIME;
    uint8_t mac_max_frame_retries = MAC_MAX_FRAME_RETRIES;
    uint8_t mac_min_BE = MAC_MIN_BE;

    /* PHY PIB Parameters */
    uint16_t a_turnaround_time = A_TURNAROUND_TIMR;
    uint8_t phy_cca_mode = ENERGY_DETECTION_OR_CHARRIER_SENSING;
    uint8_t phy_cca_threshold = 80;
    uint16_t phy_cca_duration = A_TURNAROUND_TIMR;

    /* AUTO ACK Enable Flag */
    uint8_t auto_ack_enable = true;

    /* Frame Pending Bit */
    uint8_t frame_pending_bit = true;

    /* Address Filter Set */
    uint16_t short_addr = 0x1234;
    uint32_t long_addr_0 = 0x11223333;
    uint32_t long_addr_1 = 0x55667788;
    uint16_t pan_id = 0x1AAA;
    uint8_t is_coordinator = true;
    uint8_t mac_promiscuous_mode = false;
#endif
    /* Register rfb interrupt event */
    struct_rfb_interrupt_event.tx_done = rfb_tx_done;
    struct_rfb_interrupt_event.rx_done = rfb_rx_done;
    struct_rfb_interrupt_event.rx_timeout = rfb_rx_timeout;

    /* Init rfb */
    g_rfb_ctrl = rfb_subg_init();
    g_rfb_ctrl->init(&struct_rfb_interrupt_event, RFB_KEYING_FSK, BAND_SUBG_915M);
#if (SUBG_MAC)
    g_rfb_ctrl->mac_pib_set(a_unit_backoff_period, mac_ack_wait_duration, mac_max_BE, mac_max_CSMA_backoffs,
                            mac_max_frame_total_wait_time, mac_max_frame_retries, mac_min_BE);

    g_rfb_ctrl->phy_pib_set(a_turnaround_time, phy_cca_mode, phy_cca_threshold, phy_cca_duration);

    g_rfb_ctrl->auto_ack_set(auto_ack_enable);

    g_rfb_ctrl->address_filter_set(mac_promiscuous_mode, short_addr, long_addr_0, long_addr_1, pan_id, is_coordinator);

    g_rfb_ctrl->frame_pending_set(frame_pending_bit);
#endif
    /* Init test counters*/
    g_crc_success_count = 0;
    g_crc_fail_count = 0;
    g_rx_total_count = 0;
    g_tx_total_count = 0;
    g_tx_count_target = 0;

    data_gen(&g_prbs9_buf[0], FSK_RX_LENGTH);

    /* Set test parameters*/
    switch (RfbPciTestCase)
    {
    case RFB_PCI_BURST_TX_TEST:
    case RFB_PCI_SLEEP_TX_TEST:
        rfb_tx_init(data_rate, RFB_KEYING_FSK, GFSK);
        //          printf("Tx: OQPSK_200K, RFB_KEYING_FSK\n");
        break;
    case RFB_PCI_RX_TEST:
        cfg.int_en = ENABLE;
        cfg.mode = TIMER_PERIODIC_MODE;
        cfg.prescale = TIMER_PRESCALE_1;
        Timer_Open(3, cfg, _timer_isr_handler);
        Timer_Stop(3);
        rfb_rx_init(0, true, data_rate, RFB_KEYING_FSK, GFSK);
        //          printf("Rx: OQPSK_200K, RFB_KEYING_FSK\n");
        /* Enable RF Rx*/
#if (SUBG_MAC)
        //        g_rfb_ctrl->auto_state_set(true);
        g_rfb_ctrl->auto_state_set(false);
#else
        g_rfb_ctrl->rx_start();
#endif
        break;
    }
    FwVer = 20221202;//g_rfb_ctrl->fw_version_get();
    printf("RFB Firmware version: %d\n", FwVer);
    if (gpio_pin_get(31) == 0)
    {
        printf("RF Frequency is 903MHz\n");
    }
    else if (gpio_pin_get(30) == 0)
    {
        printf("RF Frequency is 907MHz\n");
    }
    else if (gpio_pin_get(29) == 0)
    {
        printf("RF Frequency is 911MHz\n");
    }
    else if (gpio_pin_get(28) == 0)
    {
        printf("RF Frequency is 915MHz\n");
    }
    else if (gpio_pin_get(23) == 0)
    {
        printf("RF Frequency is 919MHz\n");
    }
    else if (gpio_pin_get(14) == 0)
    {
        printf("RF Frequency is 923MHz\n");
    }
    else if (gpio_pin_get(9) == 0)
    {
        printf("RF Frequency is 927MHz\n");
    }
    else
    {
        printf("RF Frequency is 903MHz\n");
    }
}

void mac_data_gen(MacBuffer_t *MacBuf, uint8_t *tx_control, uint8_t *Dsn)
{
    uint16_t mac_data_len = 0;
    uint16_t max_length = ((g_rfb.modem_type == RFB_MODEM_FSK) ? 2045 : 125);

    mac_data_len = 100;//(uint16_t)((g_tx_total_count) & 0x7FD);
    if (mac_data_len > max_length)
    {
        mac_data_len = max_length;
    }
    *Dsn = (uint8_t)((g_tx_total_count) & 0x7F);
    Rfb_MacFrameGen(MacBuf, tx_control, *Dsn, mac_data_len);
    //printf("mac_data_len =%d MacBuf.len=%d\n", mac_data_len, MacBuf->len);
    g_tx_len = MacBuf->len;
}

void rfb_sample_entry(uint8_t rfb_pci_test_case)
{
#if (SUBG_MAC)
    uint8_t tx_control = 0;
    uint8_t Dsn = 0;
    static MacBuffer_t MacBuf;
#else
    uint16_t max_length = ((g_rfb.modem_type == RFB_MODEM_FSK) ? 2047 : 127);
#endif
    switch (rfb_pci_test_case)
    {
    case RFB_PCI_BURST_TX_TEST:
        /* Abort test if TX count is reached in burst tx test */
        if (burst_tx_abort())
        {
            break;
        }

        g_tx_done = false;
        bsp_led_on(BSP_LED_0);
#if (SUBG_MAC)
        /* Generate IEEE802.15.4 MAC Header and append data */
        mac_data_gen(&MacBuf, &tx_control, &Dsn);
        g_rfb_ctrl->data_send(MacBuf.dptr, MacBuf.len, tx_control, Dsn);
        g_tx_len = MacBuf.len;
#else
        /* Determine TX packet length*/
#if 1   // Increment TX length
        g_tx_len ++;
        if (g_tx_len > (max_length - FSK_CRC16_LENGTH))
        {
            g_tx_len = PHY_MIN_LENGTH;
        }
#else   // Fix TX length
        g_tx_len = PHY_MIN_LENGTH;
#endif

        /* Send data */
        g_tx_done = false;
        g_rfb_ctrl->data_send(&g_prbs9_buf[0], g_tx_len, 0, 0);
#endif
        /* Wait TX finish */
        while (tx_done_check() == false);

        /* Add delay to increase TX interval*/
        Delay_us(30000);
        if (g_tx_total_count >= 300)
        {
            bsp_led_Off(BSP_LED_0);
            if ((g_tx_total_count - g_tx_no_ack_cnt) < (g_tx_total_count * 0.8))
            {
                bsp_led_on(BSP_LED_1);
            }
            else
            {
                bsp_led_on(BSP_LED_2);
            }
        }
        break;

    case RFB_PCI_SLEEP_TX_TEST:
        /* Abort test if TX count is reached in burst tx test */
        if (burst_tx_abort())
        {
            break;
        }

        g_tx_done = false;

#if (SUBG_MAC)
        /* Generate IEEE802.15.4 MAC Header and append data */
        g_rfb_ctrl->auto_state_set(true);
        mac_data_gen(&MacBuf, &tx_control, &Dsn);
        g_rfb_ctrl->data_send(MacBuf.dptr, MacBuf.len, tx_control, Dsn);
        g_tx_len = MacBuf.len;
#else
        g_rfb_ctrl->idle_set();
        /* Determine TX packet length*/
#if 1   // Increment TX length
        g_tx_len ++;
        if (g_tx_len > (max_length - FSK_CRC16_LENGTH))
        {
            g_tx_len = PHY_MIN_LENGTH;
        }
#else   // Fix TX length
        g_tx_len = PHY_MIN_LENGTH;
#endif

        /* Send data */
        g_tx_done = false;
        g_rfb_ctrl->data_send(&g_prbs9_buf[0], g_tx_len, 0, 0);
#endif
        /* Wait TX finish */
        while (tx_done_check() == false);

        /* Add delay to increase TX interval*/
#if (SUBG_MAC)
        /*Set RF State to SLEEP*/
        g_rfb_ctrl->auto_state_set(false);

        timer_config_mode_t cfg;

        cfg.int_en = ENABLE;
        cfg.mode = TIMER_PERIODIC_MODE;
        cfg.prescale = TIMER_PRESCALE_1;

        Timer_Open(3, cfg, _timer_isr_handler);
        Timer_Start(3, 120000); //for 40kHz timer, 40k = 1s;
        Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);
        Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_32K_TIMER);

        Lpm_Enter_Low_Power_Mode();
        //              Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_BLE_APP);
#else
        /*Set RF State to SLEEP*/
        g_rfb_ctrl->sleep_set();

        /* Add delay to sleep longer */
        timer_config_mode_t cfg;

        cfg.int_en = ENABLE;
        cfg.mode = TIMER_PERIODIC_MODE;
        cfg.prescale = TIMER_PRESCALE_1;

        Timer_Open(3, cfg, _timer_isr_handler);
        Timer_Start(3, 120000); //for 40kHz timer, 40k = 1s;
        Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);
        Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_32K_TIMER);
        Lpm_Enter_Low_Power_Mode();
#endif
        break;

    case RFB_PCI_RX_TEST:
        g_rx_total_count_last = g_rx_total_count;

        /* Check whether RX data is comming during certain interval */
        Delay_us(1000000);
        if ((RF_Rx_Switch == 1) && (g_rx_total_count_last == g_rx_total_count))
        {
            //                    if(g_rx_total_count_last>0)
            //                      {
            //
            //                           if(g_rx_total_count_last>=700)
            //                           {
            //                                bsp_led_Off(BSP_LED_1);
            //                                bsp_led_on(BSP_LED_2);
            //                                /* Disable RX*/
            //                  g_rfb_ctrl->auto_state_set(false);
            //
            //                           }
            //                           else{
            //                                bsp_led_toggle(BSP_LED_1);
            //                           }
            //
            //                      }

            printf("[E] No RX data in this period\n");
            if (RF_Time == 0)
            {
                Timer_Start(3, 6000);
                RF_Time = true;
            }

        }
        break;
    }

}

