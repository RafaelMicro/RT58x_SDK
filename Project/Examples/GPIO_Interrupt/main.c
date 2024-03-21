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

#define GPIO0       0
#define GPIO1       1

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/************************************************************/

/*this is pin mux setting*/
void init_default_pin_mux(void)
{
    uint32_t i;

    /*set all pin to gpio, except GPIO16, GPIO17 */
    for (i = 0; i < 32; i++)
    {
        pin_set_mode(i, MODE_GPIO);
    }

    /*uart0 pinmux*/
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/

    //Set gpio1 outpout pin
    gpio_pin_clear(GPIO1);
    gpio_cfg_output(GPIO1);
    return;
}

void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}

void user_gpio0_isr_handler(uint32_t pin, void *isr_param)
{
    gpio_pin_toggle(GPIO1);
    printf("GPIO1=%d\r\n", gpio_pin_get(GPIO1));
}

int main(void)
{
    /*we should set pinmux here or in SystemInit */
    init_default_pin_mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("GPIO0 Input Interrupt Demo Build %s %s\n", __DATE__, __TIME__);
    printf("Press EVK SW2 Key0,The GPIO1 will Toggle High/Low\r\n");

    gpio_set_debounce_time(DEBOUNCE_SLOWCLOCKS_1024);                       //set GPIO debounce time clock
    gpio_debounce_enable(GPIO0);                                                                //enable gpio0 debounce time.

    pin_set_pullopt(GPIO0, MODE_PULLUP_100K);                           /*set the pin to default pull high 100K mode*/

    gpio_cfg_input(GPIO0, GPIO_PIN_INT_EDGE_FALLING);                       /*set gpio 0 falling and rigger edge Interrupt Mode*/
    gpio_register_isr( GPIO0, user_gpio0_isr_handler, NULL);
    gpio_int_enable(GPIO0);

    while (1) {;}
}
/** @} */ /* end of examples group */
