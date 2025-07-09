#include "main.h"
#include "mac_frame_gen.h"
#include "uart_drv.h"
#include "retarget.h"
#include "rfb.h"
#include "rfb_comm_15p4Mac.h"
#include "rfb_comm_common.h"
#include "bsp.h"
#include "bsp_led.h"
#include "lpm.h"
/* Utility Library APIs */
#include "util_log.h"
#include "util_printf.h"

#define RX_BUF_SIZE   128
#define TX_BUF_SIZE   128
#define GPIO_LED      22
#define GPIO_SWITCH_0 17
#define GPIO_SWITCH_1 21

#define SUBG_MAC (true)

#define RUCI_HEADER_LENGTH     (1)
#define RUCI_SUB_HEADER_LENGTH (1)
#define RUCI_LENGTH            (2)
#define RUCI_PHY_STATUS_LENGTH (3)
#define RX_CONTROL_FIELD_LENGTH                                                \
    (RUCI_HEADER_LENGTH + RUCI_SUB_HEADER_LENGTH + RUCI_LENGTH                 \
     + RUCI_PHY_STATUS_LENGTH)
#define RX_STATUS_LENGTH       (5)
#define FSK_PHR_LENGTH         (2)
#define OQPSK_PHR_LENGTH       (1)
#define CRC16_LENGTH           (2)
#define CRC32_LENGTH           (4)
#define FSK_MAX_RF_LEN         2063 //2047+16
#define OQPSK_MAX_RF_LEN       142  //127+15
#define FSK_RX_HEADER_LENGTH   (RX_CONTROL_FIELD_LENGTH + FSK_PHR_LENGTH)
#define OQPSK_RX_HEADER_LENGTH (RX_CONTROL_FIELD_LENGTH + OQPSK_PHR_LENGTH)
#define RX_APPEND_LENGTH       (RX_STATUS_LENGTH + CRC16_LENGTH)
#define FSK_RX_LENGTH                                                          \
    (FSK_MAX_RF_LEN - FSK_RX_HEADER_LENGTH - RX_APPEND_LENGTH) //2047
#define OQPSK_RX_LENGTH                                                        \
    (OQPSK_MAX_RF_LEN - OQPSK_RX_HEADER_LENGTH - RX_APPEND_LENGTH) //127

#if SUBG_MAC
#define SUBG_PHY_TURNAROUND_TIMER  1000
#define SUBG_PHY_CCA_DETECTED_TIME 640 // 8 symbols for 50 kbps-data rate
#define SUBG_PHY_CCA_DETECT_MODE   0
#define SUBG_PHY_CCA_THRESHOLD     65

#define SUBG_MAC_UNIT_BACKOFF_PERIOD           320
#define SUBG_MAC_MAC_ACK_WAIT_DURATION         16000
#define SUBG_MAC_MAC_MAX_BE                    5
#define SUBG_MAC_MAC_MAX_FRAME_TOTAL_WAIT_TIME 16416
#define SUBG_MAC_MAC_MAX_FRAME_RETRIES         3
#define SUBG_MAC_MAC_MAX_CSMACA_BACKOFFS       4
#define SUBG_MAC_MAC_MIN_BE                    3
#endif

#define SUBG_RX_ON_RADIO_SLEEP_TIME 1000

#define SUBG_FREQ_PIN_MAX 7

typedef enum
{
    TRANSFER_TX_MODE = 0x1,
    TRANSFER_SLEEP_MODE = 0x2
} transfer_mode_t;


typedef enum
{
    APP_NONE_EVT = 0x0,
    APP_BUTTON0_EVT = 0x00000002,
    APP_BUTTON1_EVT = 0x00000004,
    APP_BUTTON2_EVT = 0x00000008,
    APP_BUTTON3_EVT = 0x00000010,
    APP_BUTTON4_EVT = 0x00000020,
    APP_TX_DONE_EVT = 0x00000040,
    APP_RX_DONE_EVT = 0x00000080,
} app_evt_t;

static volatile app_evt_t g_app_evt = APP_NONE_EVT;

static bool g_timer3_eable = false;

static uint16_t g_rx_time = 0;

static uint8_t g_tx_status = 0;

static uint32_t g_rx_lens = 0;

static rfb_interrupt_event_t g_rfb_evt;

static rfb_keying_type_t g_modem_type = RFB_KEYING_FSK;

static rfb_subg_ctrl_t    *g_rfb_ctrl;
/* Supported frequency lists for gpio 31, 30, 29, 28, 23, 14, 9
   User can modify content of g_freq_support to change supported frequency. */
uint32_t g_freq_support[SUBG_FREQ_PIN_MAX] = {470000, 474000, 478000, 482000,
                                              486000, 490000, 494000
                                             }; //Unit: kHz
uint8_t g_freq_gpio[SUBG_FREQ_PIN_MAX] = {31, 30, 29, 28, 23, 14, 9 };
static void subg_mac_tx_done(uint8_t tx_status);

static void subg_mac_rx_done(uint16_t packet_length, uint8_t *rx_data_address,
                             uint8_t crc_status, uint8_t rssi, uint8_t snr);

static transfer_mode_t transfer_mode_get()
{
    transfer_mode_t transfer_mode;
    if (gpio_pin_get(15) == 0)
    {
        transfer_mode = TRANSFER_TX_MODE;
    }
    else
    {
        transfer_mode = TRANSFER_SLEEP_MODE;
    }

    return transfer_mode;
}

uint32_t gpio_frequency_get()
{
    uint32_t freq;
    if (gpio_pin_get(g_freq_gpio[0]) == 0)
    {
        freq = g_freq_support[0];
    }
    else if (gpio_pin_get(g_freq_gpio[1]) == 0)
    {
        freq = g_freq_support[1];
    }
    else if (gpio_pin_get(g_freq_gpio[2]) == 0)
    {
        freq = g_freq_support[2];
    }
    else if (gpio_pin_get(g_freq_gpio[3]) == 0)
    {
        freq = g_freq_support[3];
    }
    else if (gpio_pin_get(g_freq_gpio[4]) == 0)
    {
        freq = g_freq_support[4];
    }
    else if (gpio_pin_get(g_freq_gpio[5]) == 0)
    {
        freq = g_freq_support[5];
    }
    else if (gpio_pin_get(g_freq_gpio[6]) == 0)
    {
        freq = g_freq_support[6];
    }
    else
    {
        freq = g_freq_support[0];
    }
    return freq;
}

void gpio_frequency_chek()
{
    if (gpio_pin_get(g_freq_gpio[0]) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[0]);
        printf("RF Frequency is %d kHz\r\n", g_freq_support[0]);
    }
    else if (gpio_pin_get(g_freq_gpio[1]) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[1]);
        printf("RF Frequency is %d kHz\r\n", g_freq_support[1]);
    }
    else if (gpio_pin_get(g_freq_gpio[2]) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[2]);
        printf("RF Frequency is %d kHz\r\n", g_freq_support[2]);
    }
    else if (gpio_pin_get(g_freq_gpio[3]) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[3]);
        printf("RF Frequency is %d kHz\r\n", g_freq_support[3]);
    }
    else if (gpio_pin_get(g_freq_gpio[4]) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[4]);
        printf("RF Frequency is %d kHz\r\n", g_freq_support[4]);
    }
    else if (gpio_pin_get(g_freq_gpio[5]) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[5]);
        printf("RF Frequency is %d kHz\r\n", g_freq_support[5]);
    }
    else if (gpio_pin_get(g_freq_gpio[6]) == 0)
    {
        g_rfb_ctrl->frequency_set(g_freq_support[6]);
        printf("RF Frequency is %d kHz\r\n", g_freq_support[6]);
    }
    else
    {
        g_rfb_ctrl->frequency_set(g_freq_support[0]);
        printf("RF Frequency is %d kHz\r\n", g_freq_support[0]);
    }
}

void subg_cfg_set(rfb_keying_type_t mode, uint8_t data_rate)
{
    g_modem_type = mode;
    /*Set RF State to Idle*/
    g_rfb_ctrl->idle_set();
    if (mode == RFB_KEYING_FSK)
    {
        g_rfb_ctrl->init(&g_rfb_evt, mode, BAND_SUBG_470M);

        /*Set TX config*/
        g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, data_rate, 8, MOD_1, CRC_16, WHITEN_ENABLE, GFSK);

        /*Set RX config*/
        g_rfb_ctrl->rx_config_set(data_rate, 8, MOD_1, CRC_16,
                                  WHITEN_ENABLE, 0, RFB_KEYING_FSK, GFSK);
    }
    else
    {
        g_rfb_ctrl->init(&g_rfb_evt, mode, BAND_OQPSK_SUBG_470M);

        /*Set TX config*/
        g_rfb_ctrl->tx_config_set(TX_POWER_20dBm, data_rate, 8, MOD_1, CRC_16, WHITEN_ENABLE, GFSK);

        /*Set RX config*/
        g_rfb_ctrl->rx_config_set(data_rate, 8, MOD_1, CRC_16,
                                  WHITEN_ENABLE, 0, RFB_KEYING_FSK, GFSK);
    }

    gpio_frequency_chek();
}

static void subg_data_transmission(uint16_t rx_on_radio_time)
{
    uint8_t tx_control = 0;
    uint8_t dsn = 0;
    static uint8_t mac_tx_data[MAX_DATA_SIZE];
    uint16_t mac_tx_data_lens = 0;
    char wakeup_data[] = "RAFAEL_WAKEUP";
    printf("mac tx start \r\n");
    memset(mac_tx_data, 0x0, MAX_DATA_SIZE);
    g_timer3_eable = true;
    Timer_Start(3, 999 * (g_rx_time + (SUBG_RX_ON_RADIO_SLEEP_TIME / 1000)));
    while (g_timer3_eable != false)
    {
        if (g_tx_status != 0xff)
        {
            bsp_led_on(BSP_LED_1);
            /* Generate IEEE802.15.4 MAC Header and append data */
            subg_mac_broadcast_hdr_gen(mac_tx_data, &mac_tx_data_lens, dsn);
            memcpy(&mac_tx_data[mac_tx_data_lens], wakeup_data,
                   sizeof(wakeup_data));
            mac_tx_data_lens += sizeof(wakeup_data);
            g_tx_status = 0xff;
            g_rfb_ctrl->data_send(mac_tx_data, mac_tx_data_lens, tx_control, dsn);
            dsn++;
            bsp_led_Off(BSP_LED_1);
        }
    }
    printf("mac tx end\r\n");
}

static void timer3_cb(uint32_t timer_id)
{
    g_timer3_eable = false;
}

static void button_cb(bsp_event_t event)
{
    switch (event)
    {
    case BSP_EVENT_BUTTONS_0:
        g_app_evt |= APP_BUTTON0_EVT;
        break;
    case BSP_EVENT_BUTTONS_1:
        g_app_evt |= APP_BUTTON1_EVT;
        break;
    case BSP_EVENT_BUTTONS_2:
        g_app_evt |= APP_BUTTON2_EVT;
        break;
    case BSP_EVENT_BUTTONS_3:
        g_app_evt |= APP_BUTTON3_EVT;
        break;
    case BSP_EVENT_BUTTONS_4:
        g_app_evt |= APP_BUTTON4_EVT;
        break;
    default:
        break;
    }

    return;
}

static void app_button_process(app_evt_t evt)
{
    bsp_led_Off(BSP_LED_0);
    bsp_led_Off(BSP_LED_1);
    bsp_led_Off(BSP_LED_2);

    if ( evt & APP_BUTTON0_EVT)
    {
        subg_cfg_set(RFB_KEYING_OQPSK, OQPSK_6P25K);
        g_rx_time = 35;
        if (transfer_mode_get() == TRANSFER_SLEEP_MODE)
        {
            printf("6.25K Wake on radio start: sleep %d ms ,rx %d ms \r\n",
                   SUBG_RX_ON_RADIO_SLEEP_TIME, g_rx_time);
        }
        else
        {
            printf("6.25K TX start %d ms \r\n", g_rx_time);
        }
    }
    else if (evt & APP_BUTTON1_EVT)
    {
        subg_cfg_set(RFB_KEYING_FSK, FSK_50K);
        g_rx_time = 8;
        if (transfer_mode_get() == TRANSFER_SLEEP_MODE)
        {
            printf("50K Wake on radio start: sleep %d ms ,rx %d ms \r\n",
                   SUBG_RX_ON_RADIO_SLEEP_TIME, g_rx_time);
        }
        else
        {
            printf("50K TX start %d ms \r\n", g_rx_time);
        }
    }
    else if (evt & APP_BUTTON2_EVT)
    {
        subg_cfg_set(RFB_KEYING_FSK, FSK_100K);
        g_rx_time = 6;
        if (transfer_mode_get() == TRANSFER_SLEEP_MODE)
        {
            printf("100K Wake on radio start: sleep %d ms ,rx %d ms \r\n",
                   SUBG_RX_ON_RADIO_SLEEP_TIME, g_rx_time);
        }
        else
        {
            printf("100K TX start %d ms \r\n", g_rx_time);
        }
    }
    else if (evt & APP_BUTTON3_EVT)
    {
        subg_cfg_set(RFB_KEYING_FSK, FSK_200K);
        g_rx_time = 4;
        if (transfer_mode_get() == TRANSFER_SLEEP_MODE)
        {
            printf("200K Wake on radio start: sleep %d ms ,rx %d ms \r\n",
                   SUBG_RX_ON_RADIO_SLEEP_TIME, g_rx_time);
        }
        else
        {
            printf("200K TX start %d ms \r\n", g_rx_time);
        }
    }
    else if (evt & APP_BUTTON4_EVT)
    {
        subg_cfg_set(RFB_KEYING_FSK, FSK_300K);
        g_rx_time = 3;
        if (transfer_mode_get() == TRANSFER_SLEEP_MODE)
        {
            printf("300K Wake on radio start: sleep %d ms ,rx %d ms \r\n",
                   SUBG_RX_ON_RADIO_SLEEP_TIME, g_rx_time);
        }
        else
        {
            printf("300K TX start %d ms \r\n", g_rx_time);
        }
    }
    if (transfer_mode_get() == TRANSFER_SLEEP_MODE)
    {
        /* Disable RX*/
        g_rfb_ctrl->auto_state_set(false);
        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_RESERVED27);
        rfb_comm_wake_on_radio_set(gpio_frequency_get(), g_rx_time, SUBG_RX_ON_RADIO_SLEEP_TIME);
    }
    else
    {
        subg_data_transmission(g_rx_time);
    }
}

static void app_tx_done_process(uint32_t tx_status)
{
    /* tx_status =
    0x00: TX success
    0x40: TX success and ACK is received
    0x80: TX success, ACK is received, and frame pending is true
    */
    if ((tx_status != 0) && (tx_status != 0x40) && (tx_status != 0x80))
    {
        printf("Tx done Status : %X\r\n", tx_status);
    }
}

static void app_rx_done_process(uint32_t rx_data_len)
{
    printf("Wakeup RX done len : %d\r\r\n", rx_data_len);
    bsp_led_on(BSP_LED_0);
#if CONFIG_HOSAL_SOC_IDLE_SLEEP
    Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_RESERVED27);
#endif
}

static void app_main_task(void)
{
    g_app_evt = APP_NONE_EVT;
    for (;;)
    {
        if (g_app_evt & APP_BUTTON0_EVT || g_app_evt & APP_BUTTON1_EVT || \
                g_app_evt & APP_BUTTON2_EVT || g_app_evt & APP_BUTTON3_EVT || \
                g_app_evt & APP_BUTTON4_EVT)
        {
            app_button_process(g_app_evt);
        }
        if (g_app_evt & APP_TX_DONE_EVT)
        {
            app_tx_done_process(g_tx_status);
        }
        if (g_app_evt & APP_RX_DONE_EVT)
        {
            app_rx_done_process(g_rx_lens);
        }
        g_app_evt = APP_NONE_EVT;
        if (transfer_mode_get() == TRANSFER_SLEEP_MODE)
        {
            Lpm_Enter_Low_Power_Mode();
        }
    }
}

static void subg_mac_tx_done(uint8_t tx_status)
{
    g_tx_status = tx_status;
    g_app_evt |= APP_TX_DONE_EVT;
}

static void subg_mac_rx_done(uint16_t packet_length, uint8_t *rx_data_address,
                             uint8_t crc_status, uint8_t rssi, uint8_t snr)
{
    bsp_led_on(BSP_LED_0);

    uint16_t rx_data_len = 0;
    uint8_t phr_length = ((g_modem_type == RFB_KEYING_FSK)
                          ? FSK_PHR_LENGTH
                          : OQPSK_PHR_LENGTH);
    if (crc_status == 0)
    {
        /* Calculate PHY payload length*/
        rx_data_len = packet_length
                      - (RUCI_PHY_STATUS_LENGTH + phr_length
                         + RX_APPEND_LENGTH);

        g_rx_lens = rx_data_len;
        g_app_evt |= APP_RX_DONE_EVT;
    }
}

void subg_config_init()
{
    /* RF system priority set */
    NVIC_SetPriority(CommSubsystem_IRQn, 0x01);

    /* Register rfb interrupt event */
    g_rfb_evt.tx_done = subg_mac_tx_done;
    g_rfb_evt.rx_done = subg_mac_rx_done;

    /* Init rfb */
    g_rfb_ctrl = rfb_subg_init();
    g_rfb_ctrl->init(&g_rfb_evt, RFB_KEYING_FSK, BAND_SUBG_470M);

    /* MAC PIB Parameters */
    g_rfb_ctrl->mac_pib_set(SUBG_MAC_UNIT_BACKOFF_PERIOD, SUBG_MAC_MAC_ACK_WAIT_DURATION, SUBG_MAC_MAC_MAX_BE, SUBG_MAC_MAC_MAX_CSMACA_BACKOFFS,
                            SUBG_MAC_MAC_MAX_FRAME_TOTAL_WAIT_TIME, SUBG_MAC_MAC_MAX_FRAME_RETRIES, SUBG_MAC_MAC_MIN_BE);

    /* PHY PIB Parameters */
    g_rfb_ctrl->phy_pib_set(SUBG_PHY_TURNAROUND_TIMER, SUBG_PHY_CCA_DETECT_MODE, SUBG_PHY_CCA_THRESHOLD, SUBG_PHY_CCA_DETECTED_TIME);

    /* AUTO ACK Enable Flag */
    g_rfb_ctrl->auto_ack_set(true);

    g_rfb_ctrl->idle_set();

    /*Set RF State to SLEEP*/
    // g_rfb_ctrl->sleep_set();

    uint16_t short_addr = SUBG_MAC_SHORT_ADDR;

    uint32_t long_addr_0 = (SUBG_MAC_LONG_ADDR >> 32);

    uint32_t long_addr_1 = SUBG_MAC_LONG_ADDR & 0xFFFFFFFF;

    uint16_t pnaid = 0x1AAA;

    g_rfb_ctrl->address_filter_set(false, short_addr, long_addr_0, long_addr_1, pnaid, true);

    /* Frame Pending Bit */
    g_rfb_ctrl->frame_pending_set(true);

    subg_cfg_set(RFB_KEYING_FSK, FSK_300K);

    if (transfer_mode_get() == TRANSFER_SLEEP_MODE)
    {
        printf("[SubG transfer sleep mode] \r\n");
        /*initil sleep*/
        Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);
        Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_GPIO0);
        Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_GPIO1);
        Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_GPIO2);
        Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_GPIO3);
        Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_GPIO4);

        /*wake on radio setting*/
        g_rx_time = 3;
        g_rfb_ctrl->auto_state_set(false);
        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_RESERVED27);
        rfb_comm_wake_on_radio_set(gpio_frequency_get(), g_rx_time, SUBG_RX_ON_RADIO_SLEEP_TIME);
        printf("Define 300K RX on radio start: sleep %d ms ,rx %d ms \r\n",
               SUBG_RX_ON_RADIO_SLEEP_TIME, g_rx_time);
    }
    else
    {
        printf("[SubG transfer Tx mode] \r\n");
    }
}

/*this is pin mux setting*/
void init_default_pin_mux(void)
{
    /*set all pin to gpio, except GPIO16, GPIO17 */
    for (int i = 0; i < 32; i++)
    {
        if ((i != 16) && (i != 17))
        {
            pin_set_mode(i, MODE_GPIO);
        }
    }
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/
}

void util_uart_0_init(void)
{

    bsp_stdio_t     bsp_io = { .pf_stdout_char = 0, };

    bsp_init(BSP_INIT_DEBUG_CONSOLE, NULL);


    //retarget uart output
    utility_register_stdout(bsp_io.pf_stdout_char,
                            bsp_io.pf_stdout_string);

    util_log_init();

}

int main(void)
{
    /* Init debug pin*/
    init_default_pin_mux();

    /* initil uart0*/
    util_uart_0_init();

    /*initil Timer*/
    timer_config_mode_t cfg;
    cfg.int_en = ENABLE;
    cfg.mode = TIMER_FREERUN_MODE;
    cfg.prescale = TIMER_PRESCALE_32;
    Timer_Open(3, cfg, timer3_cb);
    Timer_Stop(3);

    /* initil Button & led*/
    bsp_init((BSP_INIT_BUTTONS |
              BSP_INIT_LEDS), button_cb);
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    bsp_led_Off(BSP_LED_0);
    bsp_led_Off(BSP_LED_1);
    bsp_led_Off(BSP_LED_2);

    /* initil SubG*/
    subg_config_init();

    gpio_set_debounce_time(DEBOUNCE_SLOWCLOCKS_1024);

    printf("GPIO : Frequency (kHz) : \r\n");
    for (int i = 0 ; i < SUBG_FREQ_PIN_MAX ; i++)
    {
        printf("%d : %d(khz), ", g_freq_gpio[i], g_freq_support[i]);
    }
    printf("\r\n");
    printf("Buttion : Data Rate (Kbps) : 6.25Kpbs(0), 50Kpbs(1), 100Kpbs(2), "
           "200Kpbs(3), 300Kpbs(4) \r\n");

    app_main_task();
    return 0;
}
