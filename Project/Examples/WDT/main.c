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
#define GPIO0  0
#define GPIO1  1

/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */
#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

#define TIMER3_ID     3
#define TIMER3_FREERUN_TICK   40

#define ms_sec(N)     (N*1000)

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/


/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
uint32_t wdt_int_times = 0;


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
        if ((i != 16) && (i != 17))
        {
            pin_set_mode(i, MODE_GPIO);
        }
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
 * @brief Configure the Watchdog registers and start the watchdog counter
 * @return None
 */
void Wdt_Reset_Init(void)
{
    wdt_config_mode_t wdt_mode;
    wdt_config_tick_t wdt_cfg_ticks;

    /*
     *Remark: We should set each field of wdt_mode.
     *Otherwise, some field will become undefined.
     *It could be BUG.
     */
    wdt_mode.int_enable = 0;                              /*wdt interrupt enable field*/
    wdt_mode.reset_enable = 1;                          /*wdt reset enable field*/
    wdt_mode.lock_enable = 1;                             /*wdt lock enable field*/
    wdt_mode.prescale = WDT_PRESCALE_32;        /*SYS_CLK(32MHz)/32 = 1MHz*/

    wdt_cfg_ticks.wdt_ticks = ms_sec(50);
    wdt_cfg_ticks.int_ticks = ms_sec(0);
    wdt_cfg_ticks.wdt_min_ticks = ms_sec(1);

    Wdt_Start(wdt_mode, wdt_cfg_ticks, NULL);   /*wdt reset time = 50ms, window min 1ms*/
    gpio_pin_clear(GPIO0);                      /*debug used.*/
}


/**
 * @ingroup Wdt_example_group
 * @brief Watchdog time interrupt callback function
 * @return None
 */
void Wdt_Interrupt_Callback(void)
{
    /*This is user watchdog timer callback function.*/

    /*1. Kick 10 times for WDT will see GPIO1 toggle 10 times*/
    /*2. In fact, this interrupt is used for emergency backup used if WDT reboot enable
     */
    wdt_int_times++;

    if (wdt_int_times < 10)
    {
        Wdt_Kick();
    }

    gpio_pin_toggle(GPIO1);                /*debug used.*/

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

    /*
     *Remark: We should set each field of wdt_mode.
     *Otherwise, some field will become undefined.
     *It could be BUG.
     */
    wdt_mode.int_enable = 1;                              /* wdt interrupt enable field*/
    wdt_mode.reset_enable = 0;                          /* wdt reset enable field*/
    wdt_mode.lock_enable = 0;                             /* wdt lock enable field*/
    wdt_mode.prescale = WDT_PRESCALE_32;        /* SYS_CLK(32MHz)/32 = 1MHz*/

    wdt_cfg_ticks.wdt_ticks = ms_sec(120);
    wdt_cfg_ticks.int_ticks = ms_sec(20);
    wdt_cfg_ticks.wdt_min_ticks = ms_sec(0);

    Wdt_Start(wdt_mode, wdt_cfg_ticks, Wdt_Interrupt_Callback);      /*wdt interrupt time = 100ms, window min not work*/
    gpio_pin_clear(GPIO1);                      /*debug used.*/
}

/**
 * @ingroup Timer_example_group
 * @brief Timer3 Interrupt callback handler
 * @param[in] timer_id Timer ID that triggered the timer interrupt
 * @return None
 */
void Timer3_Callback(uint32_t timer_id)
{
    if (!Wdt_Reset_Event_Get())
    {
        gpio_pin_toggle(GPIO1);
        printf("Timer%d interrupt, reload WDT\n\n", timer_id);
        Wdt_Kick();
    }
}


/**
 * @ingroup Timer_example_group
 * @brief Timer configuration
 * @return None
 */
void Timer_Config(void)
{
    timer_config_mode_t cfg;


    /** @ Timer interrupt enable, timerClk = 40KHz, clock is divided by 1, timer clock = 40KHz (25us), load = 40, timer interrupt = 1ms*/
    cfg.int_en = ENABLE;
    cfg.mode = TIMER_FREERUN_MODE;
    cfg.prescale = TIMER_PRESCALE_1;
    Timer_Open(TIMER3_ID, cfg, Timer3_Callback);
    Timer_Start(TIMER3_ID, TIMER3_FREERUN_TICK);
    gpio_pin_clear(GPIO1);
}


int main(void)
{
    uint32_t wdt_int_times_buf;

    /*we should set pinmux here or in SystemInit*/
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);
    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("WDT Demo\n");

    /*debug used gpio*/
    gpio_pin_set(GPIO0);
    gpio_cfg_output(GPIO0);
    gpio_pin_set(GPIO1);
    gpio_cfg_output(GPIO1);

    wdt_int_times_buf = wdt_int_times;

    /*There is a reboot flag in WDT register for check "wdt" reboot happened or not*/
    if (!Wdt_Reset_Event_Get())
    {
        printf("Watchdog enable and will soon reset cause watchdog kick to early.\n");
        Timer_Config();

        //let log print complete
        for (uint32_t i = 0; i < 10000; i++);

        Wdt_Reset_Init();            /*Watchdog initialization for watchdog reset, and not kicking WDT will cause watchdog reset and reboot*/
    }
    else if (Wdt_Reset_Event_Get() == 1)
    {
        printf("Watchdog enable and will soon cause watchdog reset.\n");

        Wdt_Reset_Init();            /*Watchdog initialization for watchdog reset, and not kicking WDT will cause watchdog reset and reboot*/
    }
    else
    {
        printf("Watchdog reset and reboot!!!\n");

        Wdt_Reset_Event_Clear();

        printf("Watchdog enable and will soon cause watchdog interrupt.\n");

        Wdt_Interrupt_Init();        /*Watchdog initialization for watchdog interrupt*/
    }

    while (1)
    {
        if (wdt_int_times_buf != wdt_int_times)
        {
            printf("Watchdog interrupt %d\n", wdt_int_times);
            wdt_int_times_buf = wdt_int_times;
        }
    }
}


/** @} */ /* end of examples group */


