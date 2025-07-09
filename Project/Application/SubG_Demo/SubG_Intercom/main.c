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
#include "bsp_console.h"
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
#define PRINTF_BAUDRATE         UART_BAUDRATE_115200 //UART_BAUDRATE_2000000//

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/


/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
uint8_t              button_event = 0xFF;
/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
void set_priotity(void)
{
    NVIC_SetPriority(Uart0_IRQn, 0x01);
    NVIC_SetPriority(CommSubsystem_IRQn, 0x00);
}

/*this is pin mux setting*/
void init_default_pin_mux(void)
{
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/
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
    bsp_init(BSP_INIT_DEBUG_CONSOLE, NULL);

    //retarget uart output
    utility_register_stdout(bsp_console_stdout_char, bsp_console_stdout_string);

    util_log_init();
}

static void bsp_btn_event_handle(bsp_event_t event)
{
    switch (event)
    {
    case BSP_EVENT_BUTTONS_0:
        button_event = 0;
        break;

    case BSP_EVENT_BUTTONS_1:
        button_event = 1;
        break;

    default:
        break;
    }
}
/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
int32_t main(void)
{
    RFB_PCI_TEST_CASE rfb_pci_test_case;

    /* RF system priority set */
    set_priotity();

    dma_init();

    util_uart_0_init();

    /* initil Button and press button0 to disable sleep mode & initil Console & UART */
    bsp_init((BSP_INIT_BUTTONS |
              BSP_INIT_LEDS), bsp_btn_event_handle);

    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    /* Set RFB test case
    1. RFB_PCI_AUDIO_RELAY_TEST
    2. RFB_PCI_AUDIO_FIELD_TEST
    */
    rfb_pci_test_case = RFB_PCI_AUDIO_FIELD_TEST;
    printf("SubG Audio Demo: case %d\n", rfb_pci_test_case);

    /* Init RFB */
    rfb_sample_init(rfb_pci_test_case);

    while (1)
    {
        rfb_sample_entry(rfb_pci_test_case);
    }

}




