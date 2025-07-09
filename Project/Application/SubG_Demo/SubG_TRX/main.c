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


/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
#if (SUBG_TEST_PLAN_BER)
extern uint8_t  g_start_flag;
#endif
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
#if (PRINTF_ENABLE == 1)
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/
#endif
}

void util_uart_0_init(void)
{
    bsp_init(BSP_INIT_DEBUG_CONSOLE, NULL);

    //retarget uart output
    utility_register_stdout(bsp_console_stdout_char, bsp_console_stdout_string);

    util_log_init();
}

/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
int32_t main(void)
{
    RFB_PCI_TEST_CASE rfb_pci_test_case;

    /* RF system priority set */
    set_priotity();

    /* Init debug pin*/
    init_default_pin_mux();

    dma_init();

    /*init debug uart port for printf*/
#if (PRINTF_ENABLE == 1)
    util_uart_0_init();
#endif

#if (MODULE_ENABLE(SUPPORT_DEBUG_CONSOLE_CLI))
    extern int cli_console_init(void);
    extern int cli_console_proc(void);
    cli_console_init();
#if (SUBG_TEST_PLAN_BER)
    while (!g_start_flag)
    {
        cli_console_proc();
    }
#endif
#endif
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    /* Set RFB test case
    1. RFB_PCI_BURST_TX_TEST: Tester sends a certain number of packets
    2. RFB_PCI_SLEEP_TX_TEST: Tester sends a certain number of packets and sleeps between each tx
    3. RFB_PCI_RX_TEST: Tester receives and verify packets
    */
    rfb_pci_test_case = RFB_PCI_RX_TEST;
    printf("Test Case:%X\n", rfb_pci_test_case);

    /* Init RFB */
    rfb_sample_init(rfb_pci_test_case);

    while (1)
    {
        rfb_sample_entry(rfb_pci_test_case);
#if (MODULE_ENABLE(SUPPORT_DEBUG_CONSOLE_CLI))
        cli_console_proc();
#endif
    }

}




