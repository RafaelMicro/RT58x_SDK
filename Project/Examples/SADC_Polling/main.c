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
#include "inttypes.h"
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
sadc_cb_t            sadc_result;
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

    Sadc_Config_Enable(SADC_RES_12BIT, SADC_OVERSAMPLE_64, NULL);

    Sadc_Compensation_Init(1);


    while (1)
    {
        if (sadc_convert_status == SADC_CONVERT_IDLE)
        {
            sadc_convert_status = SADC_CONVERT_START;

            while (sadc_convert_status != SADC_CONVERT_DONE)
            {
                if (Sadc_Channel_Polling_Read(read_ch) != STATUS_SUCCESS)
                {
                    sadc_convert_status = SADC_CONVERT_IDLE;
                }
                else
                {
                    sadc_convert_status = SADC_CONVERT_DONE;
                }
            }


            sadc_result = Sadc_Get_Result();

            switch (read_ch)
            {
            case SADC_CH_VBAT:
                printf("VBAT = %"PRIu32" mv \r\n", sadc_voltage_result(sadc_result.data.sample.value));
                read_ch = SADC_CH_AIN7;
                break;

            case SADC_CH_AIN7:
                printf("AIO7 = %"PRIu32" mv \r\n", sadc_voltage_result(sadc_result.data.sample.value));
                read_ch = SADC_CH_AIN6;
                break;

            case SADC_CH_AIN6:
                printf("AIO6 = %"PRIu32" mv \r\n", sadc_voltage_result(sadc_result.data.sample.value));
                read_ch = SADC_CH_VBAT;
                break;

            default:
                break;
            }
            sadc_convert_status = SADC_CONVERT_IDLE;

            Delay_ms(100);

        }
    }
}
/** @} */ /* end of examples group */
