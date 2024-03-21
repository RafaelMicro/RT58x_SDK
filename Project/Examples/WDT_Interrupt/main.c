/** @file main.c
 *
 * @brief   Watchdog example main file.
 *          Demonstrate how to cause WDT time-out reset system event
 *
 */
/**
* @defgroup Wdt_example_group WDT
* @ingroup examples_group
* @{
* @brief Watchdog Timer example demonstrate.
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
#include "rf_mcu_ahb.h"

/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/


/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */
#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

#define ms_sec(N)     (N*1000)

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/


/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/


/**************************************************************************************************
*    GLOBAL FUNCTIONS
*************************************************************************************************/
/**
 * @ingroup Wdt_example_group
 * @brief this is pin mux setting for message output
 *
 */
void Init_Default_Pin_Mux(void)
{
    int i;

    /*set all pin to gpio, except GPIO16, GPIO17 */
    for (i = 0; i < 32; i++)
    {
        pin_set_mode(i, MODE_GPIO);
    }

    /*uart0 pinmux*/
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/

    return;
}
void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}

/**
 * @ingroup Wdt_example_group
 * @brief Watchdog time interrupt callback function
 * @return None
 */
void Wdt_Interrupt_Callback(void)
{
    printf("WDT Interrupt\n");
    return;
}


/**
 * @ingroup Wdt_example_group
 * @brief Configure the watchdog registers that register a interrupt callback funciton and start the watchdog counter
 * @return None
 */
void Wdt_Interrupt_Init(void)
{
    wdt_config_mode_t wdt_mode;
    wdt_config_tick_t wdt_cfg_ticks;

    wdt_mode.int_enable = 1;                              /* wdt interrupt enable field*/
    wdt_mode.reset_enable = 1;                              /* wdt reset enable field*/
    wdt_mode.lock_enable = 1;                             /* wdt lock enable field*/
    wdt_mode.prescale = WDT_PRESCALE_32;                            /* SYS_CLK(32MHz)/32 = 1MHz*/

    wdt_cfg_ticks.wdt_ticks = ms_sec(1000);
    wdt_cfg_ticks.int_ticks = ms_sec(500);
    wdt_cfg_ticks.wdt_min_ticks = ms_sec(0);

    Wdt_Start(wdt_mode, wdt_cfg_ticks, Wdt_Interrupt_Callback);      /*wdt interrupt time = 100ms, window min not work*/
}


int main(void)
{


    /*we should set pinmux here or in SystemInit*/
    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("WDT Interrupt Demo Build %s %s\r\n", __DATE__, __TIME__);


    if (Wdt_Reset_Event_Get() == 0)
    {
        printf("Power On Boot\r\n");
    }
    else
    {
        printf("Watchdog Reset Success\r\n");
    }

    Wdt_Reset_Event_Clear();
    Wdt_Interrupt_Init();                   /*Watchdog initialization for watchdog interrupt*/

    printf("Wait WDT Reset\n");
    while (1);
}
/** @} */ /* end of examples group */


