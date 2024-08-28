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

/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/
#if (SUBG_TEST_PLAN_BER)
#define SUBG_MAC (false)
#else
#define SUBG_MAC (false)
#endif

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
#define MAC_ACK_WAIT_DURATION         16000 // For OQPSK mode; FSK: 2000 non-beacon mode; 864 for beacon mode
#define MAC_MAX_BE                    5
#define MAC_MAX_FRAME_TOTAL_WAIT_TIME 16416
#define MAC_MAX_FRAME_RETRIES         3
#define MAC_MAX_CSMACA_BACKOFFS       4
#define MAC_MIN_BE                    3
#endif
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
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
bool                 g_tx_done;
bool                 g_rx_done;
/* frequency lists*/
uint32_t             g_50k_freq_support[10] = {920000, 920500, 921000, 921500, 922000, 922500, 923000, 923500, 924000, 924500};
uint32_t             g_100k_freq_support[10] = {920000, 920500, 921000, 921500, 922000, 922500, 923000, 923500, 924000, 924500};

rfb_subg_ctrl_t    *g_rfb_ctrl;

#if (SUBG_TEST_PLAN_BER)
bool                 tx_trigger;
uint32_t             g_rx_bit_error_count = 0;
uint16_t             g_rx_pkt_error_count = 0;
uint16_t             g_rx_pkt_loss_count;
uint8_t              BitCount_Tab[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
extern uint32_t      g_data_rate;  //unit: bps
extern uint32_t      g_frequency;  //unit: kHz
extern uint16_t      g_pkt_number; //unit: k pkts
extern uint8_t       g_band_type;
#endif

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
#if (SUBG_TEST_PLAN_BER)
void dump_rx_error_count(void)
{
    if ((g_rx_total_count + g_rx_pkt_loss_count) > 0 && (g_rx_total_count + g_rx_pkt_loss_count) % 1000 == 0)
    {
        printf("%d / %d / %d / %d / %d\n", (g_rx_total_count + g_rx_pkt_loss_count), g_rx_total_count, g_rx_pkt_loss_count, g_rx_bit_error_count, g_rx_pkt_error_count);
    }
}

static void _timer_isr_handler_tx(uint32_t timer_id)
{
    timer_config_mode_t cfg0;

    cfg0.int_en = ENABLE;
    cfg0.mode = TIMER_PERIODIC_MODE;
    cfg0.prescale = TIMER_PRESCALE_1;

    Timer_Open(timer_id, cfg0, _timer_isr_handler_tx);

    uint32_t delay_tick = 39000000;
    delay_tick = ((g_data_rate >= 100000) ? 400 : (delay_tick / g_data_rate));
    Timer_Start(3, delay_tick);

    tx_trigger = 1;
}

static void _timer_isr_handler_rx(uint32_t timer_id)
{
    timer_config_mode_t cfg0;

    if ((g_rx_total_count + g_rx_pkt_loss_count) < g_tx_count_target)
    {
        cfg0.int_en = ENABLE;
        cfg0.mode = TIMER_PERIODIC_MODE;
        cfg0.prescale = TIMER_PRESCALE_1;

        Timer_Open(timer_id, cfg0, _timer_isr_handler_rx);

        uint32_t delay_tick = 39000000;
        delay_tick = ((g_data_rate >= 100000) ? 400 : (delay_tick / g_data_rate));
        Timer_Start(3, delay_tick);

        g_rx_pkt_loss_count++;
        dump_rx_error_count();
    }
}
#endif

static void _timer_isr_handler(uint32_t timer_id)
{

}

void rfb_rx_done(uint16_t ruci_packet_length, uint8_t *rx_data_address, uint8_t crc_status, uint8_t rssi, uint8_t snr)
{
#if (!SUBG_MAC)
    uint16_t i;
    uint8_t header_length = ((g_rfb.modem_type == RFB_MODEM_FSK) ? FSK_RX_HEADER_LENGTH : OQPSK_RX_HEADER_LENGTH);
#if (SUBG_TEST_PLAN_BER)
    uint8_t bit_check_tmp;
    uint8_t packet_status = 0;
#endif
#endif
    uint16_t rx_data_len;
    uint8_t phr_length = ((g_rfb.modem_type == RFB_MODEM_FSK) ? FSK_PHR_LENGTH : OQPSK_PHR_LENGTH);

    g_rx_total_count++;
    rx_data_len = ruci_packet_length - (RUCI_PHY_STATUS_LENGTH + phr_length + RX_APPEND_LENGTH);

#if (SUBG_TEST_PLAN_BER)
    if (g_rx_total_count != 1)
    {
        Timer_Close(3);
    }
    timer_config_mode_t cfg0;

    cfg0.int_en = ENABLE;
    cfg0.mode = TIMER_PERIODIC_MODE;
    cfg0.prescale = TIMER_PRESCALE_1;

    Timer_Open(3, cfg0, _timer_isr_handler_rx);

    uint32_t delay_tick = 42900000;
    delay_tick = ((g_data_rate >= 100000) ? 440 : (delay_tick / g_data_rate));
    Timer_Start(3, delay_tick);

    packet_status = ((rx_data_len == 64) ? 0 : 1);
    g_rx_bit_error_count += ((rx_data_len < 64) ? (64 - rx_data_len) : 0);
    rx_data_len = 64;

    for (i = 0; i < rx_data_len; i++)
    {
        bit_check_tmp = g_prbs9_buf[i] ^ *(rx_data_address + header_length + i);

        if (bit_check_tmp != 0)
        {
            packet_status = 1;
            g_rx_bit_error_count += (BitCount_Tab[(bit_check_tmp >> 4)] + BitCount_Tab[(bit_check_tmp & 0x0F)]);
        }
    }
    if (packet_status)
    {
        g_rx_pkt_error_count++;
    }
    dump_rx_error_count();
#else
    if (crc_status == 0)
    {
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
    printf("RX (len:%d) done, Success:%d Fail:%d\n", rx_data_len, g_crc_success_count, g_crc_fail_count);
#endif
    g_rx_done = true;
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
#if (SUBG_TEST_PLAN_BER)
    if ((g_tx_total_count % 1000) == 0) //one msg every 1000 tx pkts
#endif
        printf("TX (len:%d) done total:%d Fail:%d\n", g_tx_len, g_tx_total_count, g_tx_fail_Count);
#endif


}

void rfb_rx_timeout(void)
{
    g_rx_timeout_count ++;
    g_rx_done = true;
    printf("RX timeout:%d\n", g_rx_timeout_count);
}

void rfb_tx_init(void)
{
    /*Set RF State to Idle*/
    g_rfb_ctrl->idle_set();

    /*Set TX config*/
#if (SUBG_TEST_PLAN_BER)
    switch (g_data_rate)
    {
    case 6250:
        g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, OQPSK_6P25K, 8, MOD_UNDEF, CRC_16, WHITEN_ENABLE, OQPSK);
        break;
    case 12500:
        g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, OQPSK_12P5K, 8, MOD_UNDEF, CRC_16, WHITEN_ENABLE, OQPSK);
        break;
    case 25000:
        g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, OQPSK_25K, 8, MOD_UNDEF, CRC_16, WHITEN_ENABLE, OQPSK);
        break;
    case 50000:
        g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, FSK_50K, 8, MOD_1, CRC_16, WHITEN_ENABLE, GFSK);
        break;
    case 100000:
        g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, FSK_100K, 8, MOD_1, CRC_16, WHITEN_ENABLE, GFSK);
        break;
    case 150000:
        g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, FSK_150K, 8, MOD_1, CRC_16, WHITEN_ENABLE, GFSK);
        break;
    case 200000:
        g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, FSK_200K, 8, MOD_1, CRC_16, WHITEN_ENABLE, GFSK);
        break;
    case 300000:
        g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, FSK_300K, 8, MOD_1, CRC_16, WHITEN_ENABLE, GFSK);
        break;

    default:
        g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, FSK_100K, 8, MOD_1, CRC_16, WHITEN_ENABLE, GFSK);
        break;
    }
#else
    g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, FSK_100K, 8, MOD_1, CRC_16, WHITEN_ENABLE, GFSK);
#endif
    /*
    * Set channel frequency :
    * For band is subg, units is kHz
    * For band is 2.4g, units is mHz
    */

#if (SUBG_TEST_PLAN_BER)
    g_rfb_ctrl->frequency_set(g_frequency);
#else
    g_rfb_ctrl->frequency_set(g_100k_freq_support[0]);
#endif

    rfb_port_tx_power_set(BAND_SUBG_915M, 30);

    g_tx_len = PHY_MIN_LENGTH;
}

void rfb_rx_init(uint32_t rx_timeout_timer, bool rx_continuous)
{
    /*Set RF State to Idle*/
    g_rfb_ctrl->idle_set();

    /*Set RX config*/
    if (rx_continuous == true)
    {
#if (SUBG_TEST_PLAN_BER)
        switch (g_data_rate)
        {
        case 6250:
            g_rfb_ctrl->rx_config_set(OQPSK_6P25K, 8, MOD_UNDEF, CRC_16, WHITEN_ENABLE, 0, rx_continuous, OQPSK);
            break;
        case 12500:
            g_rfb_ctrl->rx_config_set(OQPSK_12P5K, 8, MOD_UNDEF, CRC_16, WHITEN_ENABLE, 0, rx_continuous, OQPSK);
            break;
        case 25000:
            g_rfb_ctrl->rx_config_set(OQPSK_25K, 8, MOD_UNDEF, CRC_16, WHITEN_ENABLE, 0, rx_continuous, OQPSK);
            break;
        case 50000:
            g_rfb_ctrl->rx_config_set(FSK_50K, 8, MOD_1, CRC_16, WHITEN_ENABLE, 0, rx_continuous, GFSK);
            break;
        case 100000:
            g_rfb_ctrl->rx_config_set(FSK_100K, 8, MOD_1, CRC_16, WHITEN_ENABLE, 0, rx_continuous, GFSK);
            break;
        case 150000:
            g_rfb_ctrl->rx_config_set(FSK_150K, 8, MOD_1, CRC_16, WHITEN_ENABLE, 0, rx_continuous, GFSK);
            break;
        case 200000:
            g_rfb_ctrl->rx_config_set(FSK_200K, 8, MOD_1, CRC_16, WHITEN_ENABLE, 0, rx_continuous, GFSK);
            break;
        case 300000:
            g_rfb_ctrl->rx_config_set(FSK_300K, 8, MOD_1, CRC_16, WHITEN_ENABLE, 0, rx_continuous, GFSK);
            break;

        default:
            g_rfb_ctrl->rx_config_set(FSK_100K, 8, MOD_1, CRC_16, WHITEN_ENABLE, 0, rx_continuous, GFSK);
            break;
        }
#else
        g_rfb_ctrl->rx_config_set(FSK_100K, 8, MOD_1, CRC_16, WHITEN_ENABLE, 0, rx_continuous, GFSK);
#endif
    }
    else
    {
        g_rfb_ctrl->rx_config_set(FSK_100K, 8, MOD_1, CRC_16, WHITEN_ENABLE, rx_timeout_timer, rx_continuous, GFSK);
    }
    /* Set channel frequency */
#if (SUBG_TEST_PLAN_BER)
    g_rfb_ctrl->frequency_set(g_frequency);
#else
    g_rfb_ctrl->frequency_set(g_100k_freq_support[0]);
#endif
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
void rfb_sample_init(uint8_t RfbPciTestCase)
{
    uint32_t FwVer;
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
#if (SUBG_TEST_PLAN_BER)
    if (g_data_rate >= 50000)
    {
        g_rfb_ctrl->init(&struct_rfb_interrupt_event, RFB_KEYING_FSK, g_band_type);
    }
    else
    {
        g_rfb_ctrl->init(&struct_rfb_interrupt_event, RFB_KEYING_OQPSK, g_band_type);
    }
#else
    g_rfb_ctrl->init(&struct_rfb_interrupt_event, RFB_KEYING_FSK, BAND_SUBG_915M);
#endif

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
#if (SUBG_TEST_PLAN_BER)
    g_tx_count_target = (g_pkt_number * 1000) + 1;
#else
    g_tx_count_target = 1000;
#endif

    data_gen(&g_prbs9_buf[0], FSK_RX_LENGTH);

    /* Set test parameters*/
    switch (RfbPciTestCase)
    {
    case RFB_PCI_BURST_TX_TEST:
    case RFB_PCI_SLEEP_TX_TEST:
        rfb_tx_init();
        break;
    case RFB_PCI_RX_TEST:
        rfb_rx_init(0, true);
        /* Enable RF Rx*/
#if (SUBG_MAC)
        g_rfb_ctrl->auto_state_set(true);
#else
        g_rfb_ctrl->rx_start();
#endif
        break;
    }
    FwVer = g_rfb_ctrl->fw_version_get();
    printf("RFB Firmware version: %d\n", FwVer);
}

void mac_data_gen(MacBuffer_t *MacBuf, uint8_t *tx_control, uint8_t *Dsn)
{
    uint16_t mac_data_len = 0;
    uint16_t max_length = ((g_rfb.modem_type == RFB_MODEM_FSK) ? 2045 : 125);

    mac_data_len = (uint16_t)((g_tx_total_count) & 0x7FF);

    if (mac_data_len > max_length)
    {
        mac_data_len = max_length;
    }

    *Dsn = (uint8_t)((g_tx_total_count) & 0x7F);
    Rfb_MacFrameGen(MacBuf, tx_control, *Dsn, mac_data_len);

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

#if (SUBG_MAC)
        /* Generate IEEE802.15.4 MAC Header and append data */
        mac_data_gen(&MacBuf, &tx_control, &Dsn);
        g_rfb_ctrl->data_send(MacBuf.dptr, MacBuf.len, tx_control, Dsn);
        g_tx_len = MacBuf.len;
#else
        /* Determine TX packet length*/
#if (SUBG_TEST_PLAN_BER)
        g_tx_len = 0x40;
        if (g_tx_total_count == 0)
        {
            timer_config_mode_t cfg0;

            cfg0.int_en = ENABLE;
            cfg0.mode = TIMER_PERIODIC_MODE;
            cfg0.prescale = TIMER_PRESCALE_1;

            Timer_Open(3, cfg0, _timer_isr_handler_tx);

            uint32_t delay_tick = 39000000;
            delay_tick = ((g_data_rate >= 100000) ? 400 : (delay_tick / g_data_rate));
            Timer_Start(3, delay_tick);
        }
        while (tx_trigger == 0);
        tx_trigger = 0;
#else
        g_tx_len ++;
        if (g_tx_len > (max_length - CRC16_LENGTH))
        {
            g_tx_len = PHY_MIN_LENGTH;
        }
#endif
        /* Send data */
        g_rfb_ctrl->data_send(&g_prbs9_buf[0], g_tx_len, 0, 0);
#endif
        /* Wait TX finish */
        while (tx_done_check() == false);

        /* Add delay to increase TX interval*/
#if (!SUBG_TEST_PLAN_BER)
        Delay_us(30000);
#endif
        break;

    case RFB_PCI_SLEEP_TX_TEST:

        /* Abort test if TX count is reached in burst tx test */
        if (burst_tx_abort())
        {
            break;
        }

        g_tx_done = false;

#if (SUBG_MAC)
        g_rfb_ctrl->auto_state_set(true);

        /* Generate IEEE802.15.4 MAC Header and append data */
        mac_data_gen(&MacBuf, &tx_control, &Dsn);
        g_rfb_ctrl->data_send(MacBuf.dptr, MacBuf.len, tx_control, Dsn);
        g_tx_len = MacBuf.len;
#else
        g_rfb_ctrl->idle_set();

        /* Determine TX packet length*/
        g_tx_len ++;
        if (g_tx_len > (max_length - CRC16_LENGTH))
        {
            g_tx_len = PHY_MIN_LENGTH;
        }

        /* Send data */
        g_rfb_ctrl->data_send(&g_prbs9_buf[0], g_tx_len, 0, 0);
#endif
        /* Wait TX finish */
        while (tx_done_check() == false);

        /*Set RF State to SLEEP*/
#if (SUBG_MAC)
        g_rfb_ctrl->auto_state_set(false);
#else
        g_rfb_ctrl->sleep_set();
#endif

        /*Start timer to wake me up later and sleep*/
        timer_config_mode_t cfg;

        cfg.int_en = ENABLE;
        cfg.mode = TIMER_PERIODIC_MODE;
        cfg.prescale = TIMER_PRESCALE_1;

        Timer_Open(3, cfg, _timer_isr_handler);
        Timer_Start(3, 800000); //for 40kHz timer, 40k = 1s;

        Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);
        Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_32K_TIMER);
        Lpm_Enter_Low_Power_Mode();

        break;

    case RFB_PCI_RX_TEST:
#if (!SUBG_TEST_PLAN_BER)
        g_rx_total_count_last = g_rx_total_count;

        /* Check whether RX data is comming during certain interval */
        Delay_us(1000000);
        if (g_rx_total_count_last == g_rx_total_count)
        {
            printf("[E] No RX data in this period\n");
        }
#endif
        break;
    }

}

