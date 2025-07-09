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
#include "project_config.h"
#include "bsp_led.h"

/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/

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
#define A_TURNAROUND_TIMR             1000;
#define A_UNIT_BACKOFF_PERIOD         320;
#define MAC_ACK_WAIT_DURATION         16000 // For OQPSK mode; FSK: 2000 non-beacon mode; 864 for beacon mode
#define MAC_MAX_BE                    5
#define MAC_MAX_FRAME_TOTAL_WAIT_TIME 16416
#define MAC_MAX_FRAME_RETRIES         3
#define MAC_MAX_CSMACA_BACKOFFS       4
#define MAC_MIN_BE                    3
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
extern RFB_EVENT_STATUS rfb_port_tx_power_set(uint8_t band_type, uint8_t power_index);
extern RFB_EVENT_STATUS rfb_port_tx_power_set_oqpsk(uint8_t band_type, uint8_t power_index);
extern void rfb_intercom_mode_set(uint8_t mode_enable);
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


/* TX length for TX transmit test*/
uint16_t             g_tx_len;

/* TX/RX done finish flag*/
bool                 g_tx_done;
bool                 g_rx_done;
/* frequency lists*/
uint32_t             g_470MHz_band_freq_support[5] = {470000, 480000, 490000, 500000, 510000};
uint32_t             g_915MHz_band_freq_support[5] = {903000, 909000, 915000, 921000, 927000};

rfb_subg_ctrl_t    *g_rfb_ctrl;

uint16_t             data_success_cnt = 0;
uint8_t              intercom_test_data[84] = {0, 0, 0, 0,
                                               1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                               1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                               1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                               1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                               1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                               1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                               1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                               1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                              };
extern uint8_t       button_event;

//For RFB_PCI_AUDIO_RELAY_TEST
uint8_t              intercom_status = 0; //0: RX_unlocked(init); 1: RX_locked(after receiving 0x58 packet); 2: TX(after pushing button 0)
uint8_t              rsn;

//For RFB_PCI_AUDIO_FIELD_TEST
bool data_ready = 0;
bool test_fail = 0;
uint8_t cnt_idx = 0xFF; //initial state
uint16_t data_success_cnt_last;
uint16_t data_success_cnt_steps_sum = 0;
uint16_t data_success_cnt_pass_threshold;
uint16_t rx_pkt_cnt[10];
uint32_t rtc_time_start;
uint32_t rtc_time_step;
/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

void rfb_rx_done_intercom(uint16_t packet_length, uint8_t *rx_data_address, uint8_t crc_status, uint8_t rssi, uint8_t snr)
{
    uint8_t pkt_type;
    uint16_t i;
    uint8_t header_length = ((g_rfb.modem_type == RFB_MODEM_FSK) ? FSK_RX_HEADER_LENGTH : OQPSK_RX_HEADER_LENGTH);
    uint16_t rx_data_len;
    uint8_t phr_length = ((g_rfb.modem_type == RFB_MODEM_FSK) ? FSK_PHR_LENGTH : OQPSK_PHR_LENGTH);

    g_rx_total_count++;
    if (crc_status == 0)
    {
        rx_data_len = packet_length - (RUCI_PHY_STATUS_LENGTH + phr_length + RX_APPEND_LENGTH);
        pkt_type = *(rx_data_address + header_length);
        if (rx_data_len == 4 && ((pkt_type == 0x58) || (pkt_type == 0x5a)))
        {
            if (*(rx_data_address + header_length + 2) == 0 && *(rx_data_address + header_length + 3) == 0)
            {
                Delay_us(330);
            }
            g_tx_len = 4;
            g_rfb_ctrl->data_send((rx_data_address + header_length), 4, 0x02, 0);
            if (pkt_type == 0x58)
            {
                intercom_status = 1;
                bsp_led_on(BSP_LED_1);
                rsn = *(rx_data_address + header_length + 1) + 1;
            }
            else if (pkt_type == 0x5a)
            {
                intercom_status = 0;
                data_success_cnt = 0;
                bsp_led_Off(BSP_LED_0);
                bsp_led_Off(BSP_LED_1);
                bsp_led_Off(BSP_LED_2);
            }
        }
        else if (rx_data_len == 84 && pkt_type == 0x59)
        {
            if (*(rx_data_address + header_length + 2) == 0 && *(rx_data_address + header_length + 3) == 0)
            {
                Delay_us(1550);
            }
            g_tx_len = 84;
            g_rfb_ctrl->data_send((rx_data_address + header_length), 84, 0x02, 0);
            if (*(rx_data_address + header_length + 4) == 1 && *(rx_data_address + header_length + 5) == 2 && *(rx_data_address + header_length + 6) == 3)
            {
                rsn++;
                data_success_cnt++;
                if (data_success_cnt < 225)
                {
                    bsp_led_toggle(BSP_LED_0);
                }
                else
                {
                    bsp_led_Off(BSP_LED_0);
                }
            }
        }
        printf("RX: ");
        for (i = 0; i < 4; i++)
        {
            printf(" %x", *(rx_data_address + header_length + i));
        }
        printf("\n");
        g_crc_success_count ++;
        printf("Rx done(len:%d) total:%d Fail:%d rssi:%d snr:%d Audio frame cnt:%d\n", rx_data_len, g_rx_total_count, g_crc_fail_count, rssi, snr, data_success_cnt);
    }
    else
    {
        g_crc_fail_count ++;
        printf("Rx done total:%d Fail:%d rssi:%d\n", g_rx_total_count, g_crc_fail_count, rssi);
    }
    g_rx_done = true;
}

void rfb_rx_done_fieldtest(uint16_t packet_length, uint8_t *rx_data_address, uint8_t crc_status, uint8_t rssi, uint8_t snr)
{
    uint8_t pkt_type;
    uint8_t header_length = ((g_rfb.modem_type == RFB_MODEM_FSK) ? FSK_RX_HEADER_LENGTH : OQPSK_RX_HEADER_LENGTH);
    uint16_t rx_data_len;
    uint8_t phr_length = ((g_rfb.modem_type == RFB_MODEM_FSK) ? FSK_PHR_LENGTH : OQPSK_PHR_LENGTH);

    g_rx_total_count++;
    if (crc_status == 0)
    {
        rx_data_len = packet_length - (RUCI_PHY_STATUS_LENGTH + phr_length + RX_APPEND_LENGTH);
        pkt_type = *(rx_data_address + header_length);
        if (rx_data_len == 4 && ((pkt_type == 0x58) || (pkt_type == 0x5a)))
        {
            if (*(rx_data_address + header_length + 2) == 0 && *(rx_data_address + header_length + 3) == 0)
            {
                Delay_us(330);
            }
            g_tx_len = 4;
            g_rfb_ctrl->data_send((rx_data_address + header_length), 4, 0x02, 0);
        }
        else if (rx_data_len == 84 && pkt_type == 0x59)
        {
            if (*(rx_data_address + header_length + 2) == 0 && *(rx_data_address + header_length + 3) == 0)
            {
                Delay_us(1550);
            }
            g_tx_len = 84;
            g_rfb_ctrl->data_send((rx_data_address + header_length), 84, 0x02, 0);
            if (*(rx_data_address + header_length + 4) == 1 && *(rx_data_address + header_length + 5) == 2 && *(rx_data_address + header_length + 6) == 3)
            {
                data_success_cnt++;
            }
        }
    }
    g_rx_done = true;
}


void rfb_tx_done(uint8_t tx_status)
{
    g_tx_total_count++;
    g_tx_done = true;

    /* tx_status =
    0x00: TX success
    0x40: TX success and ACK is received
    0x80: TX success, ACK is received, and frame pending is true
    */

    if ((tx_status != 0) && (tx_status != 0x40) && (tx_status != 0x80))
    {
        g_tx_fail_Count ++;

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
    if (button_event != 1)
    {
        printf("Tx (len:%d)done total:%d Fail:%d CaFail:%d NoAck:%d TxFail%d \n", g_tx_len, g_tx_total_count, g_tx_fail_Count, g_tx_csmaca_fail_cnt, g_tx_no_ack_cnt, g_tx_fail_cnt);
    }
}

void rfb_rx_timeout(void)
{
    g_rx_timeout_count ++;
    g_rx_done = true;
    printf("RX timeout:%d\n", g_rx_timeout_count);
}

void rfb_trx_init(uint8_t RfbPciTestCase, uint32_t rx_timeout_timer, bool rx_continuous)
{
    /*Set RF State to Idle*/
    g_rfb_ctrl->idle_set();

    /*Set TX config*/
    g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, FSK_300K, 8, MOD_1, CRC_16, WHITEN_ENABLE, GFSK);

    /*Set RX config*/
    if (rx_continuous == true)
    {
        g_rfb_ctrl->rx_config_set(FSK_300K, 8, MOD_1, CRC_16, WHITEN_ENABLE, 0, rx_continuous, GFSK);
    }
    else
    {
        g_rfb_ctrl->rx_config_set(FSK_300K, 8, MOD_1, CRC_16, WHITEN_ENABLE, rx_timeout_timer, rx_continuous, GFSK);
    }

    if (RfbPciTestCase == RFB_PCI_AUDIO_RELAY_TEST)
    {
        rfb_port_tx_power_set(BAND_SUBG_433M, 7);
        g_rfb_ctrl->frequency_set(g_470MHz_band_freq_support[0]);
    }
    else if (RfbPciTestCase == RFB_PCI_AUDIO_FIELD_TEST)
    {
        if (gpio_pin_get(31) == 0)
        {
            g_rfb_ctrl->frequency_set(g_915MHz_band_freq_support[0]);
            printf("Frequency: 903 MHz\n");
        }
        else if (gpio_pin_get(30) == 0)
        {
            g_rfb_ctrl->frequency_set(g_915MHz_band_freq_support[1]);
            printf("Frequency: 909 MHz\n");
        }
        else if (gpio_pin_get(29) == 0)
        {
            g_rfb_ctrl->frequency_set(g_915MHz_band_freq_support[3]);
            printf("Frequency: 921 MHz\n");
        }
        else if (gpio_pin_get(28) == 0)
        {
            g_rfb_ctrl->frequency_set(g_915MHz_band_freq_support[4]);
            printf("Frequency: 927 MHz\n");
        }
        else
        {
            g_rfb_ctrl->frequency_set(g_915MHz_band_freq_support[2]);
            printf("Frequency: 915 MHz\n");
        }
    }
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

/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
void rfb_sample_init(uint8_t RfbPciTestCase)
{
    uint32_t FwVer;

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
    uint16_t phy_cca_duration = 100;

    /* AUTO ACK Enable Flag */
    uint8_t auto_ack_enable = false;

    /* Frame Pending Bit */
    uint8_t frame_pending_bit = true;

    /* Address Filter Set */
    uint16_t short_addr = 0x1234;
    uint32_t long_addr_0 = 0x11223333;
    uint32_t long_addr_1 = 0x55667788;
    uint16_t pan_id = 0x1AAA;
    uint8_t is_coordinator = true;
    uint8_t mac_promiscuous_mode = true;

    uint8_t subg_band;

    /* Register rfb interrupt event */
    if (RfbPciTestCase == RFB_PCI_AUDIO_RELAY_TEST)
    {
        subg_band = BAND_SUBG_433M;
        struct_rfb_interrupt_event.rx_done = rfb_rx_done_intercom;
    }
    else if (RfbPciTestCase == RFB_PCI_AUDIO_FIELD_TEST)
    {
        subg_band = BAND_SUBG_915M;
        struct_rfb_interrupt_event.rx_done = rfb_rx_done_fieldtest;
    }
    struct_rfb_interrupt_event.tx_done = rfb_tx_done;
    struct_rfb_interrupt_event.rx_timeout = rfb_rx_timeout;

    /* Init rfb */
    g_rfb_ctrl = rfb_subg_init();

    g_rfb_ctrl->init(&struct_rfb_interrupt_event, RFB_KEYING_FSK, subg_band);

    g_rfb_ctrl->mac_pib_set(a_unit_backoff_period, mac_ack_wait_duration, mac_max_BE, mac_max_CSMA_backoffs,
                            mac_max_frame_total_wait_time, mac_max_frame_retries, mac_min_BE);

    g_rfb_ctrl->phy_pib_set(a_turnaround_time, phy_cca_mode, phy_cca_threshold, phy_cca_duration);

    g_rfb_ctrl->auto_ack_set(auto_ack_enable);

    g_rfb_ctrl->address_filter_set(mac_promiscuous_mode, short_addr, long_addr_0, long_addr_1, pan_id, is_coordinator);

    g_rfb_ctrl->frame_pending_set(frame_pending_bit);

    rfb_intercom_mode_set(1);

    /* Init test counters*/
    g_crc_success_count = 0;
    g_crc_fail_count = 0;
    g_rx_total_count = 0;
    g_tx_total_count = 0;

    if (RfbPciTestCase == RFB_PCI_AUDIO_FIELD_TEST)
    {
        if (gpio_pin_get(14) == 0)
        {
            data_success_cnt_pass_threshold = 90;
            rtc_time_step = 400000;
            printf("PER time: 4 s\n");
        }
        else if (gpio_pin_get(9) == 0)
        {
            data_success_cnt_pass_threshold = 45;
            rtc_time_step = 200000;
            printf("PER time: 2 s\n");
        }
        else
        {
            data_success_cnt_pass_threshold = 23; //22.5
            rtc_time_step = 100000;
            printf("PER time: 1 s\n");
        }
    }

    /* Set test parameters*/
    rfb_trx_init(RfbPciTestCase, 0, true);
    g_rfb_ctrl->auto_state_set(true);

    FwVer = g_rfb_ctrl->fw_version_get();
    printf("RFB Firmware version: %d\n", FwVer);
}

void rfb_sample_entry(uint8_t RfbPciTestCase)
{
    uint32_t rtc_time_tmp;
    uint16_t data_success_cnt_step;
    uint16_t i;

    switch (RfbPciTestCase)
    {
    case RFB_PCI_AUDIO_RELAY_TEST:
        if (button_event == 0)
        {
            button_event = 0xFF;
            if (intercom_status == 1)
            {
                printf("Can't TX when another device is transmitting!\n");
            }
            else
            {
                bsp_led_toggle(BSP_LED_0);
                if (intercom_status == 0)
                {
                    intercom_status = 2;
                    intercom_test_data[0] = 0x58;
                    g_rfb_ctrl->auto_state_set(false);
                }
                else
                {
                    intercom_status = 0;
                    intercom_test_data[0] = 0x5a;
                }
                g_tx_done = false;
                g_tx_len = 4;
                g_rfb_ctrl->data_send(intercom_test_data, 4, 0x80, 0);
                while (tx_done_check() == false);
                intercom_test_data[1]++;
                if (intercom_status == 0)
                {
                    Delay_us(40000);
                    g_rfb_ctrl->auto_state_set(true);
                }
            }
        }
        else if (button_event == 1)
        {
            button_event = 0xFF;
            if (intercom_status == 2)
            {
                intercom_test_data[0] = 0x59;
                for (i = 0; i < 250; i++)
                {
                    g_tx_done = false;
                    g_tx_len = 84;
                    g_rfb_ctrl->data_send(intercom_test_data, 84, 0x80, 0);
                    intercom_test_data[1]++;
                    while (tx_done_check() == false);
                    Delay_us(20000);
                }
            }
            else
            {
                printf("Can't TX! Push button 0 first!\n");
            }
        }

        if (data_success_cnt > 225)
        {
            bsp_led_on(BSP_LED_2);
        }
        else
        {
            bsp_led_Off(BSP_LED_2);
        }
        break;

    case RFB_PCI_AUDIO_FIELD_TEST:
        if (button_event == 0)
        {
            g_rfb_ctrl->auto_state_set(false);
            //audio start packet
            intercom_test_data[0] = 0x58;
            g_tx_done = false;
            g_tx_len = 4;
            g_rfb_ctrl->data_send(intercom_test_data, 4, 0x80, 0);
            while (tx_done_check() == false);
            intercom_test_data[1]++;

            //audio data packets
            rtc_time_tmp = g_rfb_ctrl->rtc_time_read();
            intercom_test_data[0] = 0x59;
            bsp_led_on(BSP_LED_0);
            g_tx_len = 84;
            while (1)
            {
                g_tx_done = false;
                if (gpio_pin_get(15) == 0)
                {
                    while (g_rfb_ctrl->rtc_time_read() < (rtc_time_tmp + 40000));
                    rtc_time_tmp += 40000;
                    g_rfb_ctrl->data_send(intercom_test_data, 84, 0, 0);
                }
                else
                {
                    Delay_us(20000);
                    g_rfb_ctrl->data_send(intercom_test_data, 84, 0x80, 0);
                }
                intercom_test_data[1]++;
                while (tx_done_check() == false);
            }
        }
        else if (button_event == 1)
        {
            if (cnt_idx == 0xFF)
            {
                rtc_time_start = g_rfb_ctrl->rtc_time_read();
                data_success_cnt_last = data_success_cnt;
                cnt_idx = 0;
            }
            else
            {
                rtc_time_tmp = g_rfb_ctrl->rtc_time_read();
                if (rtc_time_tmp >= ((cnt_idx + 1) * rtc_time_step + rtc_time_start))
                {
                    data_success_cnt_step = data_success_cnt - data_success_cnt_last;
                    data_success_cnt_steps_sum = data_success_cnt_steps_sum + data_success_cnt_step - rx_pkt_cnt[cnt_idx];
                    rx_pkt_cnt[cnt_idx] = data_success_cnt_step;
                    data_success_cnt_last = data_success_cnt;

                    if (cnt_idx == 9)
                    {
                        cnt_idx = 0;
                        rtc_time_start += (rtc_time_step * 10);
                        data_ready = 1;
                    }
                    else
                    {
                        cnt_idx++;
                    }

                    if (data_ready)
                    {
                        bsp_led_Off(BSP_LED_0);
                        printf("pkt cnt: %d\n", data_success_cnt_steps_sum);
                        if (data_success_cnt_steps_sum >= data_success_cnt_pass_threshold && test_fail == 0) //90%
                        {
                            bsp_led_on(BSP_LED_2);
                        }
                        else
                        {
                            test_fail = 1;
                            bsp_led_Off(BSP_LED_2);
                            bsp_led_on(BSP_LED_1);
                        }
                    }
                    else
                    {
                        bsp_led_on(BSP_LED_0);
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}
