/** @file
 *
 * @brief BLE example file.
 *
 */

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "project_config.h"
#include "uart_drv.h"
#include "retarget.h"
#include "rfb_sample.h"
#include "bsp.h"
#include "util_printf.h"
#include "util_log.h"

#include "bsp_led.h"
/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define RX_BUF_SIZE         128
#define TX_BUF_SIZE         128
#define GPIO_LED            22
#define GPIO_SWITCH_0       17
#define GPIO_SWITCH_1       21

/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */
#define PRINTF_ENABLE           (1)
#define PRINTF_BAUDRATE         UART_BAUDRATE_115200 //UART_BAUDRATE_2000000//

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
extern bool tx_done_check(void);
extern void rfb_tx_init(uint8_t data_rate, rfb_keying_type_t keying_mode, uint8_t filter_type);
extern void rfb_rx_init(uint32_t rx_timeout_timer, bool rx_continuous, uint8_t data_rate, rfb_keying_type_t keying_mode, uint8_t filter_type);
extern void rfb_port_auto_state_set(bool rxOnWhenIdle);
extern void rfb_port_idle_set(void);

extern uint32_t g_crc_success_count;
extern uint32_t g_crc_fail_count;
extern uint32_t g_rx_total_count;
extern uint32_t g_tx_total_count;
extern uint32_t g_tx_count_target;
extern uint32_t g_tx_no_ack_cnt;

bool RF_Rx_Switch;
extern bool RF_Time;
/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
RFB_PCI_TEST_CASE rfb_pci_test_case;
bsp_event_t keyevent = BSP_EVENT_NOTING;
/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
static void test_auto_state_set(bool_t rxOnWhenIdle)
{
    rfb_port_auto_state_set(rxOnWhenIdle);
    RF_Rx_Switch = rxOnWhenIdle;
}

/*
To change data rate for any button, simply modify the last 3 intput parameters for rfb_tx_init and rfb_rx_init.
For example, change the data rate from 6.25 kbps to 150 kbps for button 0 (case BSP_EVENT_BUTTONS_0):
rfb_tx_init(OQPSK_6P25K, RFB_KEYING_OQPSK, 0);           -> rfb_tx_init(FSK_150K, RFB_KEYING_FSK, GFSK);
rfb_rx_init(0, true, OQPSK_6P25K, RFB_KEYING_OQPSK, 0);  -> rfb_rx_init(0, true, FSK_150K, RFB_KEYING_FSK, GFSK);
Supported datarate and corresponding parameters:
(OQPSK_6P25K, RFB_KEYING_OQPSK, 0)  - 6.25 kbps
(OQPSK_12P5K, RFB_KEYING_OQPSK, 0)  - 12.5 kbps
(OQPSK_25K, RFB_KEYING_OQPSK, 0)    -   25 kbps
(FSK_50K, RFB_KEYING_FSK, GFSK)     -   50 kbps
(FSK_100K, RFB_KEYING_FSK, GFSK)    -  100 kbps
(FSK_150K, RFB_KEYING_FSK, GFSK)    -  150 kbps
(FSK_200K, RFB_KEYING_FSK, GFSK)    -  200 kbps
(FSK_300K, RFB_KEYING_FSK, GFSK)    -  300 kbps
*/

static void bsp_btn_event_handle(bsp_event_t event)
{
    switch (event)
    {
    case BSP_EVENT_BUTTONS_0:
        Timer_Stop(3);
        RF_Time = false;
        bsp_led_Off(BSP_LED_0);
        bsp_led_Off(BSP_LED_1);
        bsp_led_Off(BSP_LED_2);
        if (keyevent == BSP_EVENT_BUTTONS_0)
        {
            keyevent = BSP_EVENT_NOTING;
            g_crc_success_count = 0;
            g_tx_no_ack_cnt = 0;
            g_crc_fail_count = 0;
            g_rx_total_count = 0;
            g_tx_total_count = 0;
            g_tx_count_target = 300;
            /* Enable RX*/
            test_auto_state_set(true);
            if (rfb_pci_test_case == RFB_PCI_RX_TEST)
            {
                bsp_led_on(BSP_LED_1);
            }
        }
        else if (rfb_pci_test_case == RFB_PCI_BURST_TX_TEST)
        {
            /* Wait TX finish */
            while (tx_done_check() == false);
            g_tx_total_count = 0;
            g_tx_count_target = 0;
            rfb_tx_init(OQPSK_6P25K, RFB_KEYING_OQPSK, 0);
            printf("Tx: OQPSK_6.25K, RFB_KEYING_OQPSK\n");
            keyevent = BSP_EVENT_BUTTONS_0;
        }
        else
        {
            /* Disable RX*/
            test_auto_state_set(false);
            rfb_rx_init(0, true, OQPSK_6P25K, RFB_KEYING_OQPSK, 0);
            printf("Rx: OQPSK_6.25K, RFB_KEYING_OQPSK\n");
            keyevent = BSP_EVENT_BUTTONS_0;
        }

        break;
    case BSP_EVENT_BUTTONS_1:
        Timer_Stop(3);
        RF_Time = false;
        bsp_led_Off(BSP_LED_0);
        bsp_led_Off(BSP_LED_1);
        bsp_led_Off(BSP_LED_2);
        if (keyevent == BSP_EVENT_BUTTONS_1)
        {
            keyevent = BSP_EVENT_NOTING;
            g_crc_success_count = 0;
            g_tx_no_ack_cnt = 0;
            g_crc_fail_count = 0;
            g_rx_total_count = 0;
            g_tx_total_count = 0;
            g_tx_count_target = 300;
            /* Enable RX*/
            test_auto_state_set(true);
            if (rfb_pci_test_case == RFB_PCI_RX_TEST)
            {
                bsp_led_on(BSP_LED_1);
            }
        }
        else if (rfb_pci_test_case == RFB_PCI_BURST_TX_TEST)
        {
            /* Wait TX finish */
            while (tx_done_check() == false);
            g_tx_total_count = 0;
            g_tx_count_target = 0;
            rfb_tx_init(FSK_50K, RFB_KEYING_FSK, GFSK);
            printf("Tx: FSK_50K, RFB_KEYING_FSK\n");
            keyevent = BSP_EVENT_BUTTONS_1;
        }
        else
        {
            /* Disable RX*/
            test_auto_state_set(false);
            rfb_rx_init(0, true, FSK_50K, RFB_KEYING_FSK, GFSK);
            printf("Rx: FSK_50K, RFB_KEYING_FSK\n");
            keyevent = BSP_EVENT_BUTTONS_1;
        }
        break;
    case BSP_EVENT_BUTTONS_2:
        Timer_Stop(3);
        RF_Time = false;
        bsp_led_Off(BSP_LED_0);
        bsp_led_Off(BSP_LED_1);
        bsp_led_Off(BSP_LED_2);
        if (keyevent == BSP_EVENT_BUTTONS_2)
        {
            keyevent = BSP_EVENT_NOTING;
            g_crc_success_count = 0;
            g_tx_no_ack_cnt = 0;
            g_crc_fail_count = 0;
            g_rx_total_count = 0;
            g_tx_total_count = 0;
            g_tx_count_target = 300;
            /* Enable RX*/
            test_auto_state_set(true);
            if (rfb_pci_test_case == RFB_PCI_RX_TEST)
            {
                bsp_led_on(BSP_LED_1);
            }
        }
        else if (rfb_pci_test_case == RFB_PCI_BURST_TX_TEST)
        {
            /* Wait TX finish */
            while (tx_done_check() == false);
            g_tx_total_count = 0;
            g_tx_count_target = 0;
            rfb_tx_init(FSK_100K, RFB_KEYING_FSK, GFSK);
            printf("Tx: FSK_100K, RFB_KEYING_FSK\n");
            keyevent = BSP_EVENT_BUTTONS_2;
        }
        else
        {
            /* Disable RX*/
            test_auto_state_set(false);
            rfb_rx_init(0, true, FSK_100K, RFB_KEYING_FSK, GFSK);
            printf("Rx: FSK_100K, RFB_KEYING_FSK\n");
            keyevent = BSP_EVENT_BUTTONS_2;
        }
        break;
    case BSP_EVENT_BUTTONS_3:
        Timer_Stop(3);
        RF_Time = false;
        bsp_led_Off(BSP_LED_0);
        bsp_led_Off(BSP_LED_1);
        bsp_led_Off(BSP_LED_2);
        if (keyevent == BSP_EVENT_BUTTONS_3)
        {
            keyevent = BSP_EVENT_NOTING;
            g_crc_success_count = 0;
            g_tx_no_ack_cnt = 0;
            g_crc_fail_count = 0;
            g_rx_total_count = 0;
            g_tx_total_count = 0;
            g_tx_count_target = 300;
            /* Enable RX*/
            test_auto_state_set(true);
            if (rfb_pci_test_case == RFB_PCI_RX_TEST)
            {
                bsp_led_on(BSP_LED_1);
            }
        }
        else if (rfb_pci_test_case == RFB_PCI_BURST_TX_TEST)
        {
            /* Wait TX finish */
            while (tx_done_check() == false);
            g_tx_total_count = 0;
            g_tx_count_target = 0;
            rfb_tx_init(FSK_200K, RFB_KEYING_FSK, GFSK);
            printf("Tx: FSK_200K, RFB_KEYING_FSK\n");
            keyevent = BSP_EVENT_BUTTONS_3;
        }
        else
        {
            /* Disable RX*/
            test_auto_state_set(false);
            rfb_rx_init(0, true, FSK_200K, RFB_KEYING_FSK, GFSK);
            printf("Rx: FSK_200K, RFB_KEYING_FSK\n");
            keyevent = BSP_EVENT_BUTTONS_3;
        }
        break;
    case BSP_EVENT_BUTTONS_4:
        Timer_Stop(3);
        RF_Time = false;
        bsp_led_Off(BSP_LED_0);
        bsp_led_Off(BSP_LED_1);
        bsp_led_Off(BSP_LED_2);
        if (keyevent == BSP_EVENT_BUTTONS_4)
        {
            keyevent = BSP_EVENT_NOTING;
            g_crc_success_count = 0;
            g_tx_no_ack_cnt = 0;
            g_crc_fail_count = 0;
            g_rx_total_count = 0;
            g_tx_total_count = 0;
            g_tx_count_target = 300;

            /* Enable RX*/
            test_auto_state_set(true);
            if (rfb_pci_test_case == RFB_PCI_RX_TEST)
            {
                bsp_led_on(BSP_LED_1);
            }
        }
        else if (rfb_pci_test_case == RFB_PCI_BURST_TX_TEST)
        {
            /* Wait TX finish */
            while (tx_done_check() == false);
            g_tx_total_count = 0;
            g_tx_count_target = 0;
            rfb_tx_init(FSK_300K, RFB_KEYING_FSK, GFSK);
            printf("Tx: FSK_300K, RFB_KEYING_FSK\n");
            keyevent = BSP_EVENT_BUTTONS_4;
        }
        else
        {
            /* Disable RX*/
            test_auto_state_set(false);
            rfb_rx_init(0, true, FSK_300K, RFB_KEYING_FSK, GFSK);
            printf("Rx: FSK_300K, RFB_KEYING_FSK\n");
            keyevent = BSP_EVENT_BUTTONS_4;
        }
        break;
    case BSP_EVENT_UART_RX_RECV:
    case BSP_EVENT_UART_RX_DONE:
    {
        //        char ch;

        //        bsp_console_stdin_str(&ch, 1);

        //        if (uart_data_handler(ch) == true)
        //        {
        //            // recieved '\n' or '\r' --> enable sleep mode
        //            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_BLE_APP);
        //        }
    }
    break;

    case BSP_EVENT_UART_BREAK:
        // diable sleep mode
        //        Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_BLE_APP);
        break;

    default:
        break;
    }
}

void set_priotity(void)
{
    NVIC_SetPriority(Uart0_IRQn, 0x01);
    NVIC_SetPriority(CommSubsystem_IRQn, 0x00);
}

/*this is pin mux setting*/
void init_default_pin_mux(void)
{
#if (PRINTF_ENABLE == 1)
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/
#endif
    pin_set_pullopt(15, MODE_PULLUP_10K);
    pin_set_pullopt(31, MODE_PULLUP_10K);
    pin_set_pullopt(30, MODE_PULLUP_10K);
    pin_set_pullopt(29, MODE_PULLUP_10K);
    pin_set_pullopt(28, MODE_PULLUP_10K);
    pin_set_pullopt(23, MODE_PULLUP_10K);
    pin_set_pullopt(14, MODE_PULLUP_10K);
    pin_set_pullopt(9, MODE_PULLUP_10K);
    gpio_cfg_input(15, GPIO_PIN_INT_LEVEL_LOW);
    gpio_cfg_input(31, GPIO_PIN_INT_LEVEL_LOW);
    gpio_cfg_input(30, GPIO_PIN_INT_LEVEL_LOW);
    gpio_cfg_input(29, GPIO_PIN_INT_LEVEL_LOW);
    gpio_cfg_input(28, GPIO_PIN_INT_LEVEL_LOW);
    gpio_cfg_input(23, GPIO_PIN_INT_LEVEL_LOW);
    gpio_cfg_input(14, GPIO_PIN_INT_LEVEL_LOW);
    gpio_cfg_input(9, GPIO_PIN_INT_LEVEL_LOW);

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

void sys_init(void)
{
    NVIC_SetPriority(CommSubsystem_IRQn, 1);
    NVIC_SetPriority(Gpio_IRQn, 1);

    /* Enable 569 P0 */
    outp32(0x40800010, 0x77777777);
    outp32(0x4080003C, ((inp32(0x4080003C) & 0xF0FFFFFF) | 0x07000000));
}

/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
int32_t main(void)
{
    /* RF system priority set */
    set_priotity();

    /* Init debug pin*/
    init_default_pin_mux();

    dma_init();

    //    sys_init();

    /*init debug uart port for printf*/
#if (PRINTF_ENABLE == 1)
    util_uart_0_init();
#endif

    /* Set RFB test case
    1. RFB_PCI_BURST_TX_TEST: Tester sends a certain number of packets
    2. RFB_PCI_SLEEP_TX_TEST: Tester sends a certain number of packets and sleeps between each tx
    3. RFB_PCI_RX_TEST: Tester receives and verify packets
    */
    if (gpio_pin_get(15) == 0)
    {
        rfb_pci_test_case = RFB_PCI_RX_TEST;
        printf("Test Case: RFB_PCI_RX_TEST\n");
    }
    else
    {
        rfb_pci_test_case = RFB_PCI_BURST_TX_TEST;
        printf("Test Case: RFB_PCI_BURST_TX_TEST\n");
    }
    //    printf("Test Case:%X\n", rfb_pci_test_case);
    /* Init RFB*/
    rfb_sample_init(rfb_pci_test_case, FSK_200K);

    gpio_set_debounce_time(DEBOUNCE_SLOWCLOCKS_1024);

    /* initil Button and press button0 to disable sleep mode & initil Console & UART */
    bsp_init((BSP_INIT_BUTTONS |
              BSP_INIT_LEDS), bsp_btn_event_handle);

    Change_Ahb_System_Clk(SYS_48MHZ_CLK);


    while (1)
    {
        rfb_sample_entry(rfb_pci_test_case);
    }

}




