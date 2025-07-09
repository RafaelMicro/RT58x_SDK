/** @file  main.c
 *
 * @brief Software interrupt example main file.
 *        Demonstrate how to use the software interrupt and callback function
 *        Demonstrate SW interrupt operation. SW can actively trigger interrupt
 *
 */
/**
* @defgroup Swi_example_group SWI
* @ingroup examples_group
* @{
* @brief SWI example demonstrate.
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
#define GPIO2   2
#define GPIO3   3

#define RX_BUF_SIZE   128
#define TX_BUF_SIZE   128

/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */
#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/


/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/


/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/


/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
/**
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
 * @brief Software Interrupt callback handler 0
 *        When the software
 * @param[in] swi_id Software Interrupt ID
 *            \ref SWI_ID_0 ~ SWI_ID_31
 * @return None
 */
void Swi_Int_Callback_Handler_0(uint32_t swi_id)
{
    gpio_pin_clear(GPIO0);

    if (swi_id != SWI_ID_0)
    {
        ASSERT();
    }

    gpio_pin_set(GPIO0);
}

/**
 * @brief Software Interrupt callback handler 15
 * @param[in] swi_id Software Interrupt ID
 *            \ref SWI_ID_0 ~ SWI_ID_31
 * @return None
 */
void Swi_Int_Callback_Handler_15(uint32_t swi_id)
{
    gpio_pin_clear(GPIO1);

    if (swi_id != SWI_ID_15)
    {
        ASSERT();
    }

    Swi_Int_Trigger(SWI_ID_0);
    Swi_Int_Trigger(SWI_ID_31);

    gpio_pin_set(GPIO1);
}

/**
 * @brief Software Interrupt callback handler 31
 * @param[in] swi_id Software Interrupt ID
 *            \ref SWI_ID_0 ~ SWI_ID_31
 * @return None
 */
void Swi_Int_Callback_Handler_31(uint32_t swi_id)
{
    gpio_pin_clear(GPIO2);

    if (swi_id != SWI_ID_31)
    {
        ASSERT();
    }

    gpio_pin_set(GPIO2);
}

/**
 * @brief clear software interrupt callback function
 *        register software interrupt callback function
 * @return STATUS_SUCCESS the SWI clear and register callback function is successful
 */
uint32_t Swi_Init(void)
{
    Swi_Int_Callback_Clear();
    Swi_Int_Enable(SWI_ID_0, Swi_Int_Callback_Handler_0);
    Swi_Int_Enable(SWI_ID_15, Swi_Int_Callback_Handler_15);
    Swi_Int_Enable(SWI_ID_31, Swi_Int_Callback_Handler_31);

    return STATUS_SUCCESS;
}


int main(void)
{
    uint32_t i = 0;
    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);
    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("SWI TEST\n");

    /*debug used gpio*/
    gpio_pin_set(GPIO0);
    gpio_cfg_output(GPIO0);
    gpio_pin_set(GPIO1);
    gpio_cfg_output(GPIO1);
    gpio_pin_set(GPIO2);
    gpio_cfg_output(GPIO2);
    gpio_pin_set(GPIO3);
    gpio_cfg_output(GPIO3);

    Delay_Init();       /*Init Delay*/
    Swi_Init();         /*Init SWI*/

    while (1)
    {
        gpio_pin_toggle(GPIO3);
        Swi_Int_Trigger(SWI_ID_15);

        i++;

        if (i == 10)
        {
            Swi_Int_Disable(SWI_ID_0);
        }
        if (i == 20)
        {
            Swi_Int_Disable(SWI_ID_31);
        }
        if (i == 30)
        {
            Swi_Int_Disable(SWI_ID_15);
        }

        Delay_ms(10);
    }
}
/** @} */ /* end of examples group */
