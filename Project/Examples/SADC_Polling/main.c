/** @file main.c
 *
 * @brief SAR ADC example main file.
 *        Demonstrate the SADC config and start to trigger
 *
 */
/**
 * @defgroup SADC_example_group SADC
 * @ingroup examples_group
 * @{
 * @brief SAR ADC example demonstrate.
 */


/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "project_config.h"
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

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
sadc_convert_state_t sadc_convert_status = SADC_CONVERT_IDLE;
uint32_t             sadc_convert_input;
sadc_value_t         sadc_convert_value;
/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
/**
 * @ingroup Sadc_example_group
 * @brief this is pin mux setting for message output
 *
 */
void Init_Default_Pin_Mux(void)
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
 * @ingroup Sadc_example_group
 * @brief Sadc Interrupt callback handler
 * @param[in] p_cb  point a sadc callback function
 * @return None
 */
void Sadc_Int_Callback_Handler(sadc_cb_t *p_cb)
{
    if (p_cb->type == SADC_CB_SAMPLE)
    {
        sadc_convert_input = p_cb->data.sample.channel;
        sadc_convert_value = p_cb->data.sample.value;
        sadc_convert_status = SADC_CONVERT_DONE;
    }
}

int main(void)
{
    Delay_Init();

    /*we should set pinmux here or in SystemInit */
    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("SADC Polling Mode Demo Build %s %s\n", __DATE__, __TIME__ );

    sadc_input_ch_t read_ch = SADC_CH_VBAT;

    Sadc_Config_Enable(SADC_RES_12BIT, SADC_OVERSAMPLE_256, Sadc_Int_Callback_Handler);

    Sadc_Compensation_Init(1);


    while (1)
    {
        if (sadc_convert_status == SADC_CONVERT_DONE)
        {
            printf("VBAT=%dmv\r\n", sadc_convert_value);

            Delay_ms(100); //For wait printf VBAT Data;

            sadc_convert_status = SADC_CONVERT_IDLE;

            read_ch = SADC_CH_VBAT;
        }
        else if (sadc_convert_status == SADC_CONVERT_IDLE)
        {
            sadc_convert_status = SADC_CONVERT_START;

            if (Sadc_Channel_Read(read_ch) != STATUS_SUCCESS)
            {
                sadc_convert_status = SADC_CONVERT_IDLE;
            }
        }
    }
}
/** @} */ /* end of examples group */
