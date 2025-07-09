/** @file
 *
 * @brief FreeRTOSbySystem example file.
 *
 */


/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "cm3_mcu.h"

#include "project_config.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "util_printf.h"
#include "util_log.h"

#include "bsp.h"
#include "bsp_console.h"

#include "mesh_app.h"

/**************************************************************************************************
 *    MACROS
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
extern void app_init(void);

/*this is pin mux setting*/
static void init_default_pin_mux(void)
{
    int i;

    /*set all pin to gpio, except GPIO16, GPIO17 */
    for (i = 0; i < 32; i++)
    {
        if ((i != 16) && (i != 17) && (i != 28) && (i != 29))
        {
            pin_set_mode(i, MODE_GPIO);
        }
    }

    /*uart0 pinmux*/
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/

    return;
}

void app_bsp_isr_callback(bsp_event_t event)
{
    BaseType_t context_switch = pdFALSE;
    app_queue_t t_app_q;

    switch (event)
    {
    case BSP_EVENT_BUTTONS_0:
    case BSP_EVENT_BUTTONS_1:
    case BSP_EVENT_BUTTONS_2:
    case BSP_EVENT_BUTTONS_3:
    case BSP_EVENT_BUTTONS_4:
        t_app_q.event = APP_BUTTON_EVT;
        t_app_q.data = event;

        xQueueSendToBackFromISR(app_msg_q, &t_app_q, &context_switch);
        break;

    default:
        break;
    }

}


int32_t main(void)
{
    uint32_t status;

    /* Setting the System clock */
    status = Change_Ahb_System_Clk(SYS_48MHZ_CLK);
    if (status != STATUS_SUCCESS)
    {
        /* System clock cannot be switched correctly. */
        while (1);
    }

    init_default_pin_mux();

    bsp_init(BSP_INIT_DEBUG_CONSOLE | BSP_INIT_UART | BSP_INIT_BUTTONS, app_bsp_isr_callback);

    utility_register_stdout(bsp_console_stdout_char, bsp_console_stdout_string);
    util_log_init();

#if (MODULE_ENABLE(SUPPORT_DEBUG_CONSOLE_CLI))
    extern int cli_console_init(void);
    cli_console_init();
#endif
    app_init();

    /* Start the scheduler. */
    vTaskStartScheduler();
    while (1)
    {
    }
}

