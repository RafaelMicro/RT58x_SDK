/** @file main.c
 *
 * @brief Sleep example main file.
 *        Demonstrate use the GPIO let the MCU enter different sleep mode
 *
 */
/**
 * @defgroup Sleep_example_group Sleep
 * @ingroup examples_group
 * @{
 * @brief Sleep example demonstrate.
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
#include "comm_subsystem_drv.h"
#include "rf_mcu_ahb.h"
/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/


/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define GPIO0            0
#define GPIO1            1
#define GPIO2            2
#define GPIO3            3
#define GPIO4            4
#define GPIO16           16
#define GPIO17           17
#define GPIO20           20
#define GPIO24           24

#define PWR_LEVEL3_KEY   GPIO3
#define PWR_LEVEL2_KEY   GPIO2
#define PWR_LEVEL1_KEY   GPIO1
#define PWR_LEVEL0_KEY   GPIO0
#define WAKEUP_LED       GPIO20
#define WAKEUP_KEY       GPIO4
#define LPM_WAKEUP_KEY   LOW_POWER_WAKEUP_GPIO4

#define dgpio_pin_clear(n)      gpio_pin_clear(n)
#define dgpio_pin_set(n)        gpio_pin_set(n)
#define dgpio_cfg_output(n)     gpio_cfg_output(n)

#define LPM_SRAM0_RETAIN 0x1E
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200
/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
comp_config_t p_comp_config;
uint32_t enter_sleep_mode = FALSE;

/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 TX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 RX*/
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
 * @brief Wake up Interrupt Callback function
 * @param[in] pin  GPIO wakeup pin number
 * @param[in] isr_param NULL
 * @return None
 */
void Wakeup_Int_Callback(uint32_t pin, void *isr_param)
{
    return;
}

/**
 * @brief Power Level_3 Interrupt Callback function
 * @param[in] pin GPIO wakeup pin number
 * @param[in] isr_param NULL
 * @return None
 */
void Pwr_Level3_Int_Callback(uint32_t pin, void *isr_param)
{
    //Lpm_Comm_Subsystem_Wakeup_Mask(COMM_SUBSYS_WAKEUP_MASK_BIT_RESERVED31);
    Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP3);

    enter_sleep_mode = TRUE;

    return;
}
/**
 * @brief Power Level_2 Interrupt Callback function
 * @param[in] pin GPIO wakeup pin number
 * @param[in] isr_param NULL
 * @return None
 */
void Pwr_Level2_Int_Callback(uint32_t pin, void *isr_param)
{
    Lpm_Comm_Subsystem_Wakeup_Mask(COMM_SUBSYS_WAKEUP_MASK_BIT_RESERVED31);
    Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP2);

    enter_sleep_mode = TRUE;

    return;
}
/**
 * @brief Power Level_1 Interrupt Callback function
 * @param[in] pin GPIO wakeup pin number
 * @param[in] isr_param NULL
 * @return None
 */
void Pwr_Level1_Int_Callback(uint32_t pin, void *isr_param)
{
    Lpm_Comm_Subsystem_Wakeup_Mask(COMM_SUBSYS_WAKEUP_MASK_BIT_RESERVED31);
    Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP1);

    enter_sleep_mode = TRUE;

    return;
}
/**
 * @brief Power Level_0 Interrupt Callback function
 * @param[in] pin GPIO wakeup pin number
 * @param[in] isr_param NULL
 * @return None
 */
void Pwr_Level0_Int_Callback(uint32_t pin, void *isr_param)
{
    //Lpm_Comm_Subsystem_Wakeup_Mask(COMM_SUBSYS_WAKEUP_MASK_BIT_RESERVED31);
    Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);

    enter_sleep_mode = TRUE;

    return;
}

/**
 * @brief Initinal GPIO Pin for enter sleep
 */
void Gpio_Init(void)
{
#if (0)
    pin_set_mode(GPIO16, MODE_GPIO);           /*Select GPIO16*/
    pin_set_mode(GPIO17, MODE_GPIO);           /*Select GPIO17*/

    gpio_cfg_input(GPIO16, GPIO_PIN_NOINT);    /*Configure GPIO16 input*/
    pin_set_pullopt(GPIO16, PULL_UP_100K);     /*Configure GPIO16 100K pull-up*/
    gpio_cfg_input(GPIO17, GPIO_PIN_NOINT);    /*Configure GPIO17 input*/
    pin_set_pullopt(GPIO17, PULL_UP_100K);     /*Configure GPIO17 100K pull-up*/
#endif
    gpio_pin_clear(WAKEUP_LED);
    gpio_cfg_output(WAKEUP_LED);               /*Configure WAKEUP_LED output low*/
    pin_set_pullopt(WAKEUP_LED, PULL_NONE);    /*Configure WAKEUP_LED no-pull*/
#if (CHIP_VERSION == RT58X_MPA)
    pin_set_pullopt(GPIO24, PULL_NONE);        /*Configure GPIO24 no-pull*/
#endif

    pin_set_mode(PWR_LEVEL3_KEY, MODE_GPIO);                      /*Select PWR_LEVEL3_KEY as GPIO mode*/
    pin_set_pullopt(PWR_LEVEL3_KEY, PULL_UP_100K);                /*Configure PWR_LEVEL3_KEY 100K pull-up*/
    gpio_cfg_input(PWR_LEVEL3_KEY, GPIO_PIN_INT_EDGE_FALLING);    /*Configure PWR_LEVEL3_KEY input*/
    gpio_register_isr(PWR_LEVEL3_KEY, Pwr_Level3_Int_Callback, NULL);
    gpio_int_enable(PWR_LEVEL3_KEY);                              /*enable PWR_LEVEL3_KEY pin for interrupt source*/
    gpio_debounce_enable(PWR_LEVEL3_KEY);

    pin_set_mode(PWR_LEVEL2_KEY, MODE_GPIO);                      /*Select PWR_LEVEL2_KEY as GPIO mode*/
    pin_set_pullopt(PWR_LEVEL2_KEY, PULL_UP_100K);                /*Configure PWR_LEVEL2_KEY 100K pull-up*/
    gpio_cfg_input(PWR_LEVEL2_KEY, GPIO_PIN_INT_EDGE_FALLING);    /*Configure PWR_LEVEL2_KEY input*/
    gpio_register_isr(PWR_LEVEL2_KEY, Pwr_Level2_Int_Callback, NULL);
    gpio_int_enable(PWR_LEVEL2_KEY);                              /*enable PWR_LEVEL2_KEY pin for interrupt source*/
    gpio_debounce_enable(PWR_LEVEL2_KEY);

    pin_set_mode(PWR_LEVEL1_KEY, MODE_GPIO);                      /*Select PWR_LEVEL1_KEY as GPIO mode*/
    pin_set_pullopt(PWR_LEVEL1_KEY, PULL_UP_100K);                /*Configure PWR_LEVEL1_KEY 100K pull-up*/
    gpio_cfg_input(PWR_LEVEL1_KEY, GPIO_PIN_INT_EDGE_FALLING);    /*Configure PWR_LEVEL1_KEY input*/
    gpio_register_isr(PWR_LEVEL1_KEY, Pwr_Level1_Int_Callback, NULL);
    gpio_int_enable(PWR_LEVEL1_KEY);                              /*enable PWR_LEVEL1_KEY pin for interrupt source*/
    gpio_debounce_enable(PWR_LEVEL1_KEY);

    pin_set_mode(PWR_LEVEL0_KEY, MODE_GPIO);                      /*Select PWR_LEVEL0_KEY as GPIO mode*/
    pin_set_pullopt(PWR_LEVEL0_KEY, PULL_UP_100K);                /*Configure PWR_LEVEL0_KEY 100K pull-up*/
    gpio_cfg_input(PWR_LEVEL0_KEY, GPIO_PIN_INT_EDGE_FALLING);    /*Configure PWR_LEVEL0_KEY input*/
    gpio_register_isr(PWR_LEVEL0_KEY, Pwr_Level0_Int_Callback, NULL);
    gpio_int_enable(PWR_LEVEL0_KEY);                              /*enable PWR_LEVEL0_KEY pin for interrupt source*/
    gpio_debounce_enable(PWR_LEVEL0_KEY);

    /*set WAKEUP_KEY for gpio input wakeup source*/
    /*In this example,  WAKEUP_KEY falling edge will generate wakeup interrupt*/
    pin_set_mode(WAKEUP_KEY, MODE_GPIO);
    pin_set_pullopt(WAKEUP_KEY, PULL_UP_100K);
    gpio_cfg_input(WAKEUP_KEY, GPIO_PIN_INT_EDGE_FALLING);
    gpio_register_isr(WAKEUP_KEY, Wakeup_Int_Callback, NULL);
    gpio_int_enable(WAKEUP_KEY);    /*enable WAKEUP_KEY pin for interrupt source*/
    gpio_debounce_enable(WAKEUP_KEY);   /*enable debounce for WAKEUP_KEY*/

    Lpm_Enable_Low_Power_Wakeup(LPM_WAKEUP_KEY);

    gpio_set_debounce_time(DEBOUNCE_SLOWCLOCKS_1024);
}


int main(void)
{

    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();//open uart

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    printf("Sleep Example\r\n");
    printf("press PWR_LEVEL3_KEY to enter sleep mode 3\r\n");
    printf("press PWR_LEVEL2_KEY to enter sleep mode 2\r\n");
    printf("press PWR_LEVEL1_KEY to enter sleep mode 1\r\n");
    printf("press PWR_LEVEL0_KEY to enter sleep mode 0\r\n");
    printf("press WAKEUP_KEY can wake up these power saving modes\r\n");

    Delay_Init();
    Gpio_Init();

    Comm_Subsystem_Sram_Deep_Sleep_Init();
    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.
    Lpm_Set_Sram_Sleep_Deepsleep_Shutdown(LPM_SRAM0_RETAIN);
    Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);

    /*
     * In this example,
     * press PWR_LEVEL3_KEY to enter sleep mode 3,
     * press PWR_LEVEL2_KEY to enter sleep mode 2,
     * press PWR_LEVEL1_KEY to enter sleep mode 1,
     * press PWR_LEVEL0_KEY to enter sleep mode 0,
     * and press WAKEUP_KEY can wake up these power saving modes.
     */
    while (1)
    {

        gpio_pin_toggle(WAKEUP_LED);

        if (enter_sleep_mode == TRUE)
        {
            enter_sleep_mode = FALSE;
            gpio_pin_set(WAKEUP_LED);
            Lpm_Enter_Low_Power_Mode();
        }

        Delay_ms(1000);
    }
}

/** @} */ /* end of examples group */

