/** @file
 *
 * @brief
 *
 */


/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "cm3_mcu.h"
#include "project_config.h"

#include "FreeRTOS.h"
#include "task.h"

#include "task_pci.h"

#include "zigbee_app.h"
#include "zigbee_lib_api.h"

/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
/**
 * @brief Initinal GPIO Pin mux  for PWM
 */
/*this is pin mux setting*/
static void init_default_pin_mux(void)
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

    return;
}
int main(void)
{
    check_reset();
    /*we should set pinmux here or in SystemInit */
    init_default_pin_mux();

    aes_fw_init();

    Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);
    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_GPIO);
    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_32K_TIMER);

    sys_set_random_seed(get_random_number());

    task_pci_init();

    zigbee_app_init();


    /* Start the scheduler. */
    vTaskStartScheduler();
    while (1)
    {
    }
}

