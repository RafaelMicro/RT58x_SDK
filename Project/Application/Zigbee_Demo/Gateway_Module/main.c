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
#include "mfs.h"

#include "zigbee_app.h"
/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/

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
    /*we should set pinmux here or in SystemInit */
    init_default_pin_mux();

    sys_set_random_seed(get_random_number());

    aes_fw_init();

    task_pci_init();

    zigbee_app_init();

    /* Start the scheduler. */
    vTaskStartScheduler();
    while (1)
    {
    }
}

