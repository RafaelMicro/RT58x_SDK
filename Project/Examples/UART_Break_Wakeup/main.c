/** @file main.c
 *
 * @brief UART_Break_Wakeup example main file.
 *
 *
 */
/**
* @defgroup UART_Break_Wakeup_example_group  UART_Break_Wakeup
* @ingroup examples_group
* @{
* @brief UART_Break_Wakeup example demonstrate
*/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "project_config.h"

#include "uart_drv.h"
#include "retarget.h"

#include "comm_subsystem_drv.h"
#include "rf_mcu_ahb.h"

int main(void);

void SetClockFreq(void);

/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

#define GPIO30               30


#define ASCII_0              0x30
#define ASCII_2              0x32

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/************************************************************
 * In this example, we use uart0 as loop test port
 * To test loopback test, please do NOT call any printf function
 ************************************************************/

/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    /*set gpio 30 for debug used*/
    pin_set_mode(GPIO30, MODE_GPIO);

    gpio_cfg_output(GPIO30);
    gpio_pin_clear(GPIO30);


    /*uart0 pinmux, This is default setting,
      we set it for safety. */
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
int main(void)
{
    uint32_t               wakeup_times = 0;

    uart_status            uart0_state;
    uint8_t                select_option, temp_char;


    /*
     *  This sample is just to show how to use break to wakeup RT581 in sleep state.
     *  In this example, connect uart0 to PC..
     *  RT581 device will enter sleep mode after you select sleep mode.
     *  And using PC tty console tool, like tera term, to send break to wakeup RT581.
     *
     *  For example, in tera term, press "Control -> send break" to send a very long break signial in uart0_rx.
     *  Then press any button once for sending data to RT581, then you will see the character show in the console.
     *
     *  After RT581 get a character from PC, excluding break trash character, RT581 will enter sleep again.
     *
     *  Notice: After wakeup from break.... RT581 can not go to sleep before break end. Otherwise, RT581 will not
     *  enter sleep mode.
     */


    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);
    Init_Default_Pin_Mux();


    /*init debug uart port for printf*/

    console_drv_init(PRINTF_BAUDRATE);

    printf("uart break wakeup sample code %s %s\n", __DATE__, __TIME__);

    printf("this program use uart0 as uart break wakeup example \n");
    printf("After SOC sleep, you can use console to send break to wakup the SOC \n");
    printf("User can check GPIO30 toggle for system wakeup \n");

    /*wait some uart message to print finish--- you can set a small delay timer, too */
    printf("Please select mode you want to test: \n");
    printf("0: For CPU sleep  RF run  \n");
    printf("1: For CPU sleep  RF sleep \n");
    printf("2: For CPU sleep  RF deep sleep\n");

    while (1)
    {
        /*wait user to select correct sleep mode 0/1/2*/
        select_option = getchar();

        if ((select_option >= ASCII_0) && (select_option <= ASCII_2))
        {
            break;
        }
    }

    printf("Select  LOW_POWER_LEVEL_SLEEP%c mode \n", select_option);
    printf("Testing... \n");

    /*
     * wait uart0 printf message finish. Remark: IT CAN NOT sleep when TX running!
     * wait TX finish or  abort non-transfer message....
     *
     */
    while (1)
    {
        uart0_state = uart_status_get(0);

        /*wait TX print message finish*/
        if ((uart0_state.tx_busy) == FALSE)
        {
            break;
        }
    }

    Comm_Subsystem_Sram_Deep_Sleep_Init();
    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.
    Lpm_Set_Low_Power_Level( (low_power_level_cfg_t) (LOW_POWER_LEVEL_SLEEP0 + (select_option - ASCII_0)));

    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_UART0_RX);     /*wakeup source is UART0*/

    wakeup_times = 0;

    while (1)
    {
        /*
         * before we sleep, we should read all data from uart rx
         * Here we assume all rx data has been read...
         */

        /* before sleep... we shall make sure uart tx in high state.
         * if some data in uart TX fifo... maybe we should give up those message or
         * wait them sent finish
         */

        while (1)
        {
            uart0_state = uart_status_get(0);

            /*wait TX print message finish*/
            if ((uart0_state.tx_busy) == FALSE)
            {
                break;
            }
        }

        gpio_pin_toggle(GPIO30);

        Lpm_Enter_Low_Power_Mode();             /*Sleep here*/


        /*wakeup*/
        wakeup_times++;
        printf("break wakeup %ld \n", wakeup_times);

        /*check uart rx--- although we wakeup from break, there is one trash byte in uart rx! */
        /*break trash--- it should be zero*/
        temp_char = getchar();

        /* in fact, we don't know how long for break signal....
         * if we sleep before break end... RT58x will not sleep
         * so we add a getchar function here...
         * in normal case application, system will not sleep so short!
         */

        /*wait user input...*/
        temp_char = getchar();
        printf("we got input byte %c \n", temp_char);

    }

}

void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
