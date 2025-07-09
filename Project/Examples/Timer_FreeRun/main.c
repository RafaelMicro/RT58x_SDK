/** @file main.c
 *
 * @brief Timer example main file.
 *
 *
 */
/**
 * @defgroup Timer_example_group  Timer
 * @ingroup examples_group
 * @{
 * @brief Timer example demonstrate
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
#define GPIO0   0
#define GPIO1   1

/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */
#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

#define TIMER0_ID     0
#define TIMER3_ID     3
#define TIMER0_FREERUN_TICK   999
#define TIMER3_FREERUN_TICK   40

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/


/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
volatile uint32_t   timer0_tick_count = 0;
volatile uint32_t   timer3_tick_count = 0;


/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/


/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
/**
 * @ingroup Timer_example_group
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
 * @ingroup Timer_example_group
 * @brief Timer0 Interrupt callback handler
 * @param[in] timer_id Timer ID that triggered the timer interrupt
 * @return None
 */
void Timer0_Callback(uint32_t timer_id)
{
    timer0_tick_count++;

    gpio_pin_toggle(GPIO0);
    printf("\nTimer%d interrupt\n", timer_id);

    if (timer0_tick_count < 10)
    {
        Timer_Load(TIMER0_ID, TIMER0_FREERUN_TICK * (1 + timer0_tick_count));
    }
    else
    {
        Timer_Stop(TIMER0_ID);
        Timer_Close(TIMER0_ID);
    }
}


/**
 * @ingroup Timer_example_group
 * @brief Timer3 Interrupt callback handler
 * @param[in] timer_id Timer ID that triggered the timer interrupt
 * @return None
 */
void Timer3_Callback(uint32_t timer_id)
{
    timer3_tick_count++;

    gpio_pin_toggle(GPIO1);
    printf("\nTimer%d interrupt\n", timer_id);

    if (timer3_tick_count < 10)
    {
        Timer_Load(TIMER3_ID, TIMER3_FREERUN_TICK * (1 + timer3_tick_count));
    }
    else
    {
        Timer_Stop(TIMER3_ID);
        Timer_Close(TIMER3_ID);
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

    /** @ Timer interrupt enable, timerClk = 32MHz, clock is divided by 32, timer clock = 1MHz (1us), load = 999, timer interrupt = 1ms*/
    cfg.int_en = ENABLE;
    cfg.mode = TIMER_FREERUN_MODE;
    cfg.prescale = TIMER_PRESCALE_32;
    Timer_Open(TIMER0_ID, cfg, Timer0_Callback);
    Timer_Start(TIMER0_ID, TIMER0_FREERUN_TICK);
    gpio_pin_clear(GPIO0);

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
    /*we should set pinmux here or in SystemInit*/
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);
    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("\nTimer Demo Start!\n");

    /*debug used gpio*/
    gpio_pin_set(GPIO0);
    gpio_cfg_output(GPIO0);
    gpio_pin_set(GPIO1);
    gpio_cfg_output(GPIO1);

    Timer_Config();

    while (1)
    {
        if ((timer0_tick_count == 10) && (timer3_tick_count == 10))
        {
            printf("\nTimer Demo Done!\n");
            printf("Timer0 interrupt count = %d\n", timer0_tick_count);
            printf("Timer3 interrupt count = %d\n", timer3_tick_count);
            timer0_tick_count = 0;
            timer3_tick_count = 0;
        }
    }
}
/** @} */ /* end of examples group */


