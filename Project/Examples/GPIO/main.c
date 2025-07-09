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

void SetClockFreq(void);


#define GPIO28  28
#define GPIO29  29
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/************************************************************/

/*this is pin mux setting*/
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
/*
 * this function is ISR. so it should be as short as possible
 * You SHOULD NOT call any function that will block the ISR
 *
 */

void user_gpio_isr_handler(uint32_t pin, void *isr_param)
{
    /*
     * This is a very STUPID idea --- call printf in ISR.
     * Here we use it just for showing debug information easily...
     * In normal code, you SHOULD AVOID this poor design...
     */

    //printf("pin %ld\n", pin);

    /*we also use gpio 31 to show interrupt happen*/
    gpio_pin_toggle(31);

    return;
}

uint32_t valid_pin(uint32_t pin)
{
    if ((pin == 16) || (pin == 17))   /*uart0 pinmux */
    {
        return 0;
    }

    /*not package pin */
    if (((pin >= 10) && (pin <= 14)) || (pin == 18) || (pin == 19) || ((pin >= 24) && (pin <= 27)))
    {
        return 0;
    }

    return 1;
}


#define _user_input_  1

int main(void)
{
    int i;

#ifdef _user_input_
    uint8_t  temp;
#endif

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    /*    Warning: We will call printf in gpio ISR...
     * This is very STUPID design.. here we just do this for debug used.
     *
     *    If printf in ISR with TX Buffer full, the character will be droped.
     * So the message will lost.
     *
     */

    printf("Hello World, GPIO  TEST\n");

    printf("This example first will output low then high for every pins sequential \n");
    printf("Excluding pin16 (uart0.rx) and pin17 (uart0.tx ) \n");
    printf("Press any key in PC once, you will see gpios toggle \n");
    printf("Like GPIO0_Low->GPIO0_High->GPIO0_Low/GPIO1_High->GPIO1_Low/GPIO2_High-> .... \n");

    /*
     * Notice: For Package 52 pin IC ---
     * You can not use pins 10,11,12,13, 18, 19. 24, 25, 26, 27.
     * Those pins are not packaged!
     */

    /*set all pin for gpio out, and out to see the value*/
    /*
     * Please NOTICE: when we set the pin to output mode,
     * it will also change the pin to none-pull mode.
     */
    for (i = 0; i < 32; i++)
    {
        if (valid_pin(i))
        {
            gpio_cfg_output(i);
            /*default output low.*/
            gpio_pin_clear(i);
        }
    }

#ifdef _user_input_
    /*this is just a simple delay for user to check output*/
    temp = getchar();
    printf("%c \n", temp);
#endif

    /*
     * output high then low for each pin by pin
     *
     */
    for (i = 0; i < 32; i++)
    {
        if (valid_pin(i))
        {
            gpio_pin_set(i);


#ifdef _user_input_
            /*this is just a simple delay for user to check output*/
            temp = getchar();
            printf("%c \n", temp);
#endif

            gpio_pin_clear(i);
        }

    }

    printf("GPIO Test output ok \n");

    printf("\n");

    printf("Test GPIO input and interrupt pins \n");
    printf("Excluding GPIO31, GPIO16 and GPIO17 \n");
    printf("GPIO31 show interrupt generated, so it is output  indicate.\n");
    printf("When interrupt generated, GPIO31 will toggle once \n");
    printf("Please see the comment in this main.c for more detail information\n");

    /*set all pin for gpio input, and interrupt mode*/

    gpio_set_debounce_time(DEBOUNCE_SLOWCLOCKS_1024);

    /*
     *   Please notice: when CPU reboot default GPIO is pull high driving.
     * But when we set the pin to output pin in previous test, it will be set to none pull mode.
     *
     *   So here we change the pin driving mode to back pull high mode for input in this test.
     * Of course, in you application, you should set the pin driving mode depends on your real applicance.
     *
     *   If you just want to use GPIO for input without interrupt,
     * You can set
     *     gpio_cfg_input(i, GPIO_PIN_NOINT);
     *     gpio_debounce_disable(i);
     *
     *   If you use  GPIO_PIN_INT_LEVEL_LOW, then it will generate a very busy gpio interrupt when GPIO is connected low.
     * You will not see any message if gpio keep in low... because gpio interrupt busy.... (CPU always services GPIO ISR)
     * You can check gpio31 output (it will always toggle when test pin in low...)
     * until test pin goes back to high...then you will see the printf message in TX buffer
     * because cpu can service other function now, including printf.
     * (if too many output message in TX buffer, printf will drop the message)
     *
     *
     * For debounce input: debounce is used for filtering glitch.
     *  In most case, this function is used for human input, like press/release button or card insert/remove detected.
     *  Enable debounce needs a setup time to detect data valid! It will introduce a "delay" based on debounce_time.
     *
     *  If the gpio interrupt source is triggered by outside IC chips, the application should NOT enable deounce for the that input source!
     *
     */

    for (i = 0; i < 31; i++)            /*GPIO31 use for toggle message in output*/
    {
        if (valid_pin(i))
        {

            pin_set_pullopt(i, MODE_PULLUP_100K);         /*set the pin to default pull high 100K mode*/

            //gpio_cfg_input(i, GPIO_PIN_INT_LEVEL_LOW);     /*if you using interrupt low level trigger, please don't enable dobounce for this pin*/

            gpio_cfg_input(i, GPIO_PIN_INT_EDGE_FALLING);
            gpio_debounce_enable(i);

            gpio_register_isr( i, user_gpio_isr_handler, NULL);
            /*enable each gpio pin interrupt*/
            gpio_int_enable(i);
        }
    }

    /*ok now you can connect the test pin to low, then you will see the interrupt happen*/


    while (1)
        ;


}

void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
