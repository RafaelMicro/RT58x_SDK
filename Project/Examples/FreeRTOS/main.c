/** @file main.c
 *
 * @brief FreeRTOS example main file.
 *        Demonstrate creating FreeRTOS task and use GPIO to wake up enter sleep task.
 *        FreeRTOS porting and basic operation.
 *
 */
/**
 * @defgroup freertos_example_group FreeRTOS
 * @ingroup examples_group
 * @{
 * @brief FreeRTOS example demonstrate
 */

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "project_config.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
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
#if configUSE_TICKLESS_IDLE             //Support Sleep (FreeRtosConfig.h)
#define TIMER_CALLBACK_EXAMPLE    ENABLE
#define SLEEP_WAKEUP_EXAMPLE      ENABLE
#endif

#define GPIO0   0
#define GPIO1   1
#define GPIO2   2
#define GPIO3   3
#define GPIO11  11
#define GPIO12  12
#define GPIO13  13
#define GPIO14  14
#define GPIO20  20
#define GPIO21  21
#define GPIO28  28
#define GPIO29  29



#define NUM_TIMERS    5

#define RTOS_QUEUE_EVENT       0x55AA5A00
#define RTOS_QUEUE_EVENT_HEAP  0xAA55A500
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */
#define PRINTF_BAUDRATE      UART_BAUDRATE_115200


/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
struct cmd_message
{
    uint32_t message_id;
    uint8_t  cmd;
    uint8_t  status;
};

struct event_message
{
    uint32_t message_id;
    uint32_t event;
    uint8_t  status;
};


/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
/* An array to hold handles to the created timers. */
TimerHandle_t timer_handle[NUM_TIMERS];

QueueHandle_t task1_queue, task2_queue, task3_queue;

uint32_t msg_id = 0;

#if (SLEEP_WAKEUP_EXAMPLE == ENABLE)
TaskHandle_t xSleepTaskHandle;
TimerHandle_t timer_task_resume_handle;
#endif


/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
#if (SLEEP_WAKEUP_EXAMPLE == ENABLE)
static void Rtos_Sleep_Task(void *parameters_ptr);
#endif
static void Rtos_Task1(void *parameters_ptr);
static void Rtos_Task2(void *parameters_ptr);
static void Rtos_Task3(void *parameters_ptr);


/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
/**
 * @ingroup freertos_example_group
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
#if (TIMER_CALLBACK_EXAMPLE == ENABLE)
/* Define a callback function that will be used by multiple timer
   instances.  The callback function does nothing but count the number
   of times the associated timer expires, and stop the timer once the
   timer has expired 10 times.  The count is saved as the ID of the
   timer. */
void vTimerCallback(TimerHandle_t pxTimer)
{
    const uint32_t ulMaxExpiryCountBeforeStopping = 10;
    uint32_t ulCount;

    /* Optionally do something if the pxTimer parameter is NULL. */
    configASSERT(pxTimer);

    /* The number of times this timer has expired is saved as the timer's ID.  Obtain the count. */
    ulCount = (uint32_t) pvTimerGetTimerID(pxTimer);

    /* Increment the count, then test to see if the timer has expired ulMaxExpiryCountBeforeStopping yet. */
    ulCount++;

    /* If the timer has expired 10 times then stop it from running. */
    if (ulCount >= ulMaxExpiryCountBeforeStopping)
    {
        /* Do not use a block time if calling a timer API function
        from a timer callback function, as doing so could cause a
        deadlock! */
        xTimerStop(pxTimer, 0);
    }
    else
    {
        /* Store the incremented count back into the timer's ID field
        so it can be read back again the next time this software timer
        expires. */
        vTimerSetTimerID(pxTimer, (void *) ulCount);
    }
}

#else
/**
 * @ingroup freertos_example_group
 * @brief Timer 0 callback function
 * @param[in] timer_ptr
 * @return None
 */
void Timer0_Callback(TimerHandle_t timer_ptr)
{
    /* Optionally do something if the timer_ptr parameter is NULL. */
    configASSERT(timer_ptr);
}

/**
 * @ingroup freertos_example_group
 * @brief Timer 1 callback function
 * @param[in] timer_ptr
 * @return None
 */
void Timer1_Callback(TimerHandle_t timer_ptr)
{
    /* Optionally do something if the timer_ptr parameter is NULL. */
    configASSERT(timer_ptr);
}

/**
 * @ingroup freertos_example_group
 * @brief Timer 3 callback function
 * @param[in] timer_ptr
 * @return None
 */
void Timer2_Callback(TimerHandle_t timer_ptr)
{
    /* Optionally do something if the timer_ptr parameter is NULL. */
    configASSERT(timer_ptr);
}

#endif

#if (SLEEP_WAKEUP_EXAMPLE == ENABLE)
/**
 * @ingroup freertos_example_group
 * @brief Use gpin20 to wakeup the rtos
 * @param[in] pin
 * @param[in] isr_param
 * @return None
 */
void User_Gpio20_Isr_Handler(uint32_t pin, void *isr_param)
{
    //BaseType_t xYieldRequired;

    gpio_pin_clear(GPIO13);

    /* Resume the suspended task. */
    vTaskResume(xSleepTaskHandle);

    gpio_pin_set(GPIO13);
}

#endif

int main(void)
{
#if (TIMER_CALLBACK_EXAMPLE == ENABLE)
    uint32_t x;
#endif

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.
    /*init delay function*/
    Delay_Init();

    printf("FreeRTOS Demo build %s %s \n", __DATE__, __TIME__);

    /*debug used gpio*/
    gpio_pin_set(GPIO0);
    gpio_cfg_output(GPIO0);
    gpio_pin_set(GPIO1);
    gpio_cfg_output(GPIO1);
    gpio_pin_set(GPIO2);
    gpio_cfg_output(GPIO2);
    gpio_pin_set(GPIO3);
    gpio_cfg_output(GPIO3);
    gpio_pin_set(GPIO11);
    gpio_cfg_output(GPIO11);
    gpio_pin_set(GPIO12);
    gpio_cfg_output(GPIO12);
    gpio_pin_set(GPIO13);
    gpio_cfg_output(GPIO13);
    gpio_pin_set(GPIO14);
    gpio_cfg_output(GPIO14);


#if (SLEEP_WAKEUP_EXAMPLE == ENABLE)


    Comm_Subsystem_Sram_Deep_Sleep_Init();
    Comm_Subsystem_Disable_LDO_Mode();
    Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);
    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_GPIO);

    gpio_set_debounce_time(DEBOUNCE_SLOWCLOCKS_64);

    gpio_cfg_input(GPIO20, GPIO_PIN_INT_EDGE_FALLING);
    gpio_debounce_enable(GPIO20);
    gpio_register_isr(GPIO20, User_Gpio20_Isr_Handler, NULL);
    gpio_int_enable(GPIO20);

#endif

#if (TIMER_CALLBACK_EXAMPLE == ENABLE)
    /* Create then start some timers.  Starting the timers before
    the RTOS scheduler has been started means the timers will start
    running immediately that the RTOS scheduler starts. */
    for (x = 0; x < NUM_TIMERS; x++)
    {
        if (x == 0)
        {
            timer_handle[x] = xTimerCreate
                              ( /* Just a text name, not used by the RTOS kernel. */
                                  "Timer",
                                  /* The timer period in ticks, must be greater than 0. */
                                  pdMS_TO_TICKS(100),
                                  /* The timers will auto-reload themselves when they expire. */
                                  pdTRUE,
                                  /* The ID is used to store a count of the number of times the timer has expired, which is initialised to 0. */
                                  (void *) 0,
                                  /* Each timer calls the same callback when it expires. */
                                  vTimerCallback
                              );
        }
        else
        {
            timer_handle[x] = xTimerCreate
                              ( /* Just a text name, not used by the RTOS kernel. */
                                  "Timer",
                                  /* The timer period in ticks, must be greater than 0. */
                                  (100 * x) + 100,
                                  /* The timers will auto-reload themselves when they expire. */
                                  pdTRUE,
                                  /* The ID is used to store a count of the number of times the timer has expired, which is initialised to 0. */
                                  (void *) 0,
                                  /* Each timer calls the same callback when it expires. */
                                  vTimerCallback
                              );
        }

        if (timer_handle[x] == NULL)
        {
            /* The timer was not created. */
        }
        else
        {
            /* Start the timer.  No block time is specified, and
            even if one was it would be ignored because the RTOS
            scheduler has not yet been started. */
            if (xTimerStart(timer_handle[x], 0) != pdPASS)
            {
                /* The timer could not be set into the Active state. */
            }
        }
    }

#else
    timer_handle[0] = xTimerCreate
                      ( /* Just a text name, not used by the RTOS kernel. */
                          "Timer0",
                          /* The timer period in ticks, must be greater than 0. */
                          pdMS_TO_TICKS(10),
                          /* The timers will auto-reload themselves when they expire. */
                          pdTRUE,
                          /* The ID is used to store a count of the number of times the timer has expired, which is initialised to 0. */
                          (void *) 0,
                          /* Each timer calls the same callback when it expires. */
                          Timer0_Callback
                      );

    if (timer_handle[0] == NULL)
    {
        /* The timer was not created. */
    }
    else
    {
        /* Start the timer.  No block time is specified, and
        even if one was it would be ignored because the RTOS
        scheduler has not yet been started. */
        if (xTimerStart(timer_handle[0], 0) != pdPASS)
        {
            /* The timer could not be set into the Active state. */
        }
    }

    timer_handle[1] = xTimerCreate
                      ( /* Just a text name, not used by the RTOS kernel. */
                          "Timer1",
                          /* The timer period in ticks, must be greater than 0. */
                          20,
                          /* The timers will auto-reload themselves when they expire. */
                          pdTRUE,
                          /* The ID is used to store a count of the number of times the timer has expired, which is initialised to 0. */
                          (void *) 0,
                          /* Each timer calls the same callback when it expires. */
                          Timer1_Callback
                      );

    if (timer_handle[1] == NULL)
    {
        /* The timer was not created. */
    }
    else
    {
        /* Start the timer.  No block time is specified, and
        even if one was it would be ignored because the RTOS
        scheduler has not yet been started. */
        if (xTimerStart(timer_handle[1], 0) != pdPASS)
        {
            /* The timer could not be set into the Active state. */
        }
    }

    timer_handle[2] = xTimerCreate
                      ( /* Just a text name, not used by the RTOS kernel. */
                          "Timer2",
                          /* The timer period in ticks, must be greater than 0. */
                          40,
                          /* The timers will auto-reload themselves when they expire. */
                          pdFALSE,
                          /* The ID is used to store a count of the number of times the timer has expired, which is initialised to 0. */
                          (void *) 0,
                          /* Each timer calls the same callback when it expires. */
                          Timer2_Callback
                      );

    if (timer_handle[2] == NULL)
    {
        /* The timer was not created. */
    }
    else
    {
        /* Start the timer.  No block time is specified, and
        even if one was it would be ignored because the RTOS
        scheduler has not yet been started. */
        if (xTimerStart(timer_handle[2], 0) != pdPASS)
        {
            /* The timer could not be set into the Active state. */
        }
    }
#endif

    task1_queue = xQueueCreate(10, sizeof(struct event_message));
    task2_queue = xQueueCreate(10, sizeof(struct cmd_message));
    task3_queue = xQueueCreate(10, sizeof(struct cmd_message *));

    /* Start the tasks defined within this file/specific to this demo. */
#if (SLEEP_WAKEUP_EXAMPLE == ENABLE)
    xTaskCreate(Rtos_Sleep_Task, "SLEEP_TASK", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1), &xSleepTaskHandle);
    xTaskCreate(Rtos_Task1, "TASK1", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1), NULL);
    xTaskCreate(Rtos_Task2, "TASK2", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1), NULL);
    xTaskCreate(Rtos_Task3, "TASK3", configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1), NULL);
#else
    xTaskCreate(Rtos_Task1, "TASK1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(Rtos_Task2, "TASK2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(Rtos_Task3, "TASK3", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
#endif

    /* Start the scheduler. */
    vTaskStartScheduler();

    while (1)
    {
    }
}

#if (SLEEP_WAKEUP_EXAMPLE == ENABLE)
/**
 * @brief Freertos sleep task
 * @param[in] parameters_ptr
 * @return None
 */
static void Rtos_Sleep_Task(void *parameters_ptr)
{
    for (;;)
    {
        gpio_pin_set(GPIO0);
        gpio_pin_set(GPIO2);
        gpio_pin_set(GPIO3);
        gpio_pin_toggle(GPIO1);


        if (gpio_pin_get(GPIO20) == 1)
        {
            //if (xTimerReset(timer_task_resume_handle, 0) != pdPASS) {
            //    /* The reset command was not executed successfully. Take appropriate action here. */
            //}

            vTaskSuspend(xSleepTaskHandle);
        }
    }
}

#endif
/**
 * @brief Freertos Task 1
 * @param[in] parameters_ptr
 * @return None
 */
static void Rtos_Task1(void *parameters_ptr)
{
    struct cmd_message task1_cmd;
    struct event_message task1_event;
    struct cmd_message *task1_cmd_ptr;
    struct event_message *task1_event_ptr;

#if (SLEEP_WAKEUP_EXAMPLE == ENABLE)
    vTaskSuspend(NULL);
#endif

    for (;;)
    {
        gpio_pin_set(GPIO0);
        gpio_pin_set(GPIO2);
        gpio_pin_set(GPIO3);
        gpio_pin_toggle(GPIO1);

        //printf("TASK1 Running...\n");

        if (xPortGetFreeHeapSize() == 0)
        {
            while (1);
        }

        task1_cmd.message_id = msg_id;
        task1_cmd.cmd = (msg_id & 0xFF);

        if (xQueueSend(task2_queue, (void *) &task1_cmd, (TickType_t) 0) == pdPASS)
        {
            if (xQueueReceive(task1_queue, (void *) &task1_event, (TickType_t) 10) == pdPASS)
            {
                if (task1_event.event == (RTOS_QUEUE_EVENT + (msg_id & 0xFF)))
                {
                    //msg_id++;
                }
                else
                {
                    while (1);
                }
            }
        }

        task1_cmd_ptr = pvPortMalloc(sizeof(struct cmd_message));

        if (task1_cmd_ptr == NULL)
        {
            while (1);
        }

        task1_cmd_ptr->message_id = msg_id;
        task1_cmd_ptr->cmd = (msg_id & 0xFF);
        if (xQueueSend(task3_queue, (void *) &task1_cmd_ptr, (TickType_t) 0) == pdFAIL)
        {
            while (1);
        }

        task1_cmd_ptr = pvPortMalloc(sizeof(struct cmd_message));

        if (task1_cmd_ptr == NULL)
        {
            while (1);
        }

        task1_cmd_ptr->message_id = (msg_id + 1);
        task1_cmd_ptr->cmd = ((msg_id + 1) & 0xFF);
        if (xQueueSend(task3_queue, (void *) &task1_cmd_ptr, (TickType_t) 0) == pdFAIL)
        {
            while (1);
        }

        if (xQueueReceive(task1_queue, (void *) &task1_event_ptr, (TickType_t) 10) == pdPASS)
        {
            if (task1_event_ptr->event == (RTOS_QUEUE_EVENT_HEAP + (msg_id & 0xFF)))
            {
                vPortFree(task1_event_ptr);
            }
            else
            {
                while (1);
            }
        }

        if (xQueueReceive(task1_queue, (void *) &task1_event_ptr, (TickType_t) 10) == pdPASS)
        {
            if (task1_event_ptr->event == (RTOS_QUEUE_EVENT_HEAP + ((msg_id + 1) & 0xFF)))
            {
                vPortFree(task1_event_ptr);
                msg_id++;
            }
            else
            {
                while (1);
            }
        }
    }
}
/**
 * @brief Freertos Task 2
 * @param[in] parameters_ptr
 * @return None
 */
static void Rtos_Task2(void *parameters_ptr)
{
    struct cmd_message task2_cmd;
    struct event_message task2_event;

    for (;;)
    {
        gpio_pin_set(GPIO0);
        gpio_pin_set(GPIO1);
        gpio_pin_set(GPIO3);
        gpio_pin_toggle(GPIO2);

        //printf("TASK2 Running...\n");

        if (xQueueReceive(task2_queue, (void *) &task2_cmd, (TickType_t) 10) == pdPASS)
        {
            if (task2_cmd.cmd == (msg_id & 0xFF))
            {
                task2_event.message_id = task2_cmd.message_id;
                task2_event.event = (RTOS_QUEUE_EVENT + (msg_id & 0xFF));
                if (xQueueSend(task1_queue, (void *) &task2_event, (TickType_t) 0) != pdPASS)
                {
                }
            }
            else
            {
                while (1);
            }
        }
#if (SLEEP_WAKEUP_EXAMPLE == ENABLE)
        else
        {
            vTaskSuspend(NULL);
        }
#endif
    }
}
/**
 * @brief Freertos Task 3
 * @param[in] parameters_ptr
 * @return None
 */
static void Rtos_Task3(void *parameters_ptr)
{
    struct cmd_message *task3_cmd_ptr;
    struct event_message *task3_event_ptr;

    for (;;)
    {
        gpio_pin_set(GPIO0);
        gpio_pin_set(GPIO1);
        gpio_pin_set(GPIO2);
        gpio_pin_toggle(GPIO3);

        //printf("TASK3 Running...\n");

        if (xQueueReceive(task3_queue, (void *) &task3_cmd_ptr, (TickType_t) 10) == pdPASS)
        {
            if (task3_cmd_ptr->cmd == (msg_id & 0xFF))
            {
                task3_event_ptr = pvPortMalloc(sizeof(struct event_message));

                if (task3_event_ptr == NULL)
                {
                    while (1);
                }

                task3_event_ptr->message_id = task3_cmd_ptr->message_id;
                task3_event_ptr->event = (RTOS_QUEUE_EVENT_HEAP + (msg_id & 0xFF));
                if (xQueueSend(task1_queue, (void *) &task3_event_ptr, (TickType_t) 0) != pdPASS)
                {
                }

                vPortFree(task3_cmd_ptr);
            }
            else
            {
                while (1);
            }
        }
#if (SLEEP_WAKEUP_EXAMPLE == ENABLE)
        else
        {
            vTaskSuspend(NULL);
        }
#endif

        if (xQueueReceive(task3_queue, (void *) &task3_cmd_ptr, (TickType_t) 10) == pdPASS)
        {
            if (task3_cmd_ptr->cmd == ((msg_id + 1) & 0xFF))
            {
                task3_event_ptr = pvPortMalloc(sizeof(struct event_message));

                if (task3_event_ptr == NULL)
                {
                    while (1);
                }

                task3_event_ptr->message_id = task3_cmd_ptr->message_id;
                task3_event_ptr->event = (RTOS_QUEUE_EVENT_HEAP + ((msg_id + 1) & 0xFF));
                if (xQueueSend(task1_queue, (void *) &task3_event_ptr, (TickType_t) 0) != pdPASS)
                {
                }

                vPortFree(task3_cmd_ptr);
            }
            else
            {
                while (1);
            }
        }
#if (SLEEP_WAKEUP_EXAMPLE == ENABLE)
        else
        {
            vTaskSuspend(NULL);
        }
#endif
    }
}
