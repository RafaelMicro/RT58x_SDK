/** @file main.c
 *
 * @brief GPIO example main file.
 *
 *
 */
/**
* @defgroup GPIO_example_group  GPIO
* @ingroup examples_group
* @{
* @brief GPIO example demonstrate
*/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "project_config.h"

#include "uart_drv.h"
#include "retarget.h"
#include "rf_mcu_ahb.h"
/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

int main(void);

#define GPIO20      20
/************************************************************/

/*this is pin mux setting*/
void init_default_pin_mux(void)
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

int main(void)
{

    /*we should set pinmux here or in SystemInit */
    init_default_pin_mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();  //if don't load 569 FW, need to call the function.

    printf("GPIO Output Demo Build %s \n", __DATE__);

    gpio_pin_clear(GPIO20);
    gpio_cfg_output(GPIO20);

    gpio_pin_set(GPIO20);       //GPIO 0 Outpu High
    Delay_ms(10);
    gpio_pin_clear(GPIO20); //GPIO 0 Outpu Low
    Delay_ms(10);

    while (1)
    {
        gpio_pin_toggle(GPIO20);    //GPIO 0 High/Low Toggle
        Delay_ms(500);
    }
}
/** @} */ /* end of examples group */
