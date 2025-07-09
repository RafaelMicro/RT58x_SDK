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

//#include "rf_tx_comp.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "rf_mcu_ahb.h"
#include "inttypes.h"
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
sadc_value_t         sadc_convert_value, sadc_convert_thers_value;


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

        printf("\nADC CH%"PRIu32": adc = %"PRIu32", comp = %"PRIu32", cal = %"PRIu32"\n", p_cb->data.sample.channel, p_cb->raw.conversion_value, p_cb->raw.compensation_value, p_cb->raw.calibration_value);
    }
}


static void Sadc_Task(void *parameters_ptr)
{
    sadc_input_ch_t read_ch = SADC_CH_AIN7;

    Sadc_Config_Enable(SADC_RES_12BIT, SADC_OVERSAMPLE_256, Sadc_Int_Callback_Handler);

    Sadc_Compensation_Init(1);

    for (;;)
    {
        if (sadc_convert_status == SADC_CONVERT_IDLE)
        {
            sadc_convert_status = SADC_CONVERT_START;

            if (Sadc_Channel_Read(read_ch) != STATUS_SUCCESS)
            {
                sadc_convert_status = SADC_CONVERT_IDLE;
            }
        }

        if (sadc_convert_status == SADC_CONVERT_DONE)
        {
            switch (read_ch)
            {
            case SADC_CH_AIN7:
                sadc_convert_thers_value = sadc_voltage_result(sadc_convert_value);
                printf("AIO7 ADC = %"PRIu32"mv\n", sadc_convert_thers_value);
                read_ch = SADC_CH_VBAT;
                break;

            case SADC_CH_VBAT:
                sadc_convert_thers_value = sadc_voltage_result(sadc_convert_value);
                printf("VBAT ADC = %"PRIu32"mv\n", sadc_convert_thers_value);
                read_ch = SADC_CH_AIN6;
                break;

            case SADC_CH_AIN6:
                sadc_convert_thers_value = sadc_voltage_result(sadc_convert_value);
                printf("AIO6 ADC = %"PRIu32"mv\n", sadc_convert_thers_value);
                read_ch = SADC_CH_AIN7;
                break;

            default:
                break;
            }

            Delay_ms(1000);

            sadc_convert_status = SADC_CONVERT_IDLE;
        }
    }
}


static void TxPwrComp_Task(void *parameters_ptr)
{
    //Sadc_Config_Enable(SADC_RES_12BIT, SADC_OVERSAMPLE_256, Sadc_Int_Callback_Handler);

    //Tx_Power_Compensation_Init(3);

    for (;;)
    {
    }
}

int main(void)
{
    Delay_Init();

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("SADC Demo\n");

    /*debug used gpio*/
    gpio_pin_set(GPIO0);
    gpio_cfg_output(GPIO0);
    gpio_pin_set(GPIO1);
    gpio_cfg_output(GPIO1);
    gpio_pin_set(GPIO2);
    gpio_cfg_output(GPIO2);
    gpio_pin_set(GPIO3);
    gpio_cfg_output(GPIO3);

    xTaskCreate(Sadc_Task, "SADC_TASK", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1), NULL);
    xTaskCreate(TxPwrComp_Task, "TX_PWR_COMP_TASK", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1), NULL);

    /* Start the scheduler. */
    vTaskStartScheduler();

    while (1)
    {
    }
}
/** @} */ /* end of examples group */
