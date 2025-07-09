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
#include "bsp_uart.h"

#include "mesh_app.h"
#include "ble_mesh_element.h"
#include "ble_mesh_element.h"
/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define PWM_PWM0_COUNTER_MODE   PWM_COUNTER_MODE_UP         /*UP is up counter Mode ;UD is up and down Mode*/
#define PWM_PWM0_SEQ_ORDER      PWM_SEQ_ORDER_R             /*Rseq or Tseq Selection or both*/
#define PWM_PWM0_ELEMENT_NUM    1                           /*genrator Pulse number*/
#define PWM_PWM0_REPEAT_NUM     0                           /*Repeat Pulse number*/
#define PWM_PWM0_DLY_NUM        0                           /*Pulse delay number*/
#define PWM_PWM0_CNT_END_VAL    10000                       /*Count end Value*/
#define PWM_PWM0_CLK_DIV        PWM_CLK_DIV_4               /*PWM Input Clock Div*/
#define PWM_PWM0_TRIG_SRC       PWM_TRIGGER_SRC_SELF        /*PWM Trigger Source by self or PWM1~PWM4*/
#define PWM_PWM0_SEQ_MODE       PWM_SEQ_MODE_CONTINUOUS     /*Continuous and Noncontinuous mode*/
#define PWM_PWM0_PLAY_CNT       0                           /*0:is infinite*/
#define PWM_PWM0_SEQ_NUM        PWM_SEQ_NUM_1               /*use rdma single simple or two simple*/
#define PWM_PWM0_DMA_SML_FMT    PWM_DMA_SMP_FMT_1           /*Pwm DMA Simple Format 0 or 1*/

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/

/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
pwm_seq_para_head_t pwm_para_config[3];

uint32_t pwm_rdma0_addr_temp[3];
uint32_t pwm_rdma1_addr_temp[3];
uint16_t pwm_count_end_val[3] = {PWM_PWM0_CNT_END_VAL, PWM_PWM0_CNT_END_VAL, PWM_PWM0_CNT_END_VAL};
pwm_clk_div_t pwm_clk_div[3] = {PWM_PWM0_CLK_DIV, PWM_PWM0_CLK_DIV, PWM_PWM0_CLK_DIV};
pwm_seq_order_t pwm_seq_order[3] = {PWM_PWM0_SEQ_ORDER, PWM_PWM0_SEQ_ORDER, PWM_PWM0_SEQ_ORDER};
pwm_trigger_src_t pwm_trigger_src[3] = {PWM_PWM0_TRIG_SRC, PWM_PWM0_TRIG_SRC, PWM_PWM0_TRIG_SRC};
uint16_t pwm_play_cnt[3] = {PWM_PWM0_PLAY_CNT, PWM_PWM0_PLAY_CNT, PWM_PWM0_PLAY_CNT};
pwm_seq_num_t pwm_seq_num[3] = {PWM_PWM0_SEQ_NUM, PWM_PWM0_SEQ_NUM, PWM_PWM0_SEQ_NUM};
pwm_seq_mode_t pwm_seq_mode[3] = {PWM_PWM0_SEQ_MODE, PWM_PWM0_SEQ_MODE, PWM_PWM0_SEQ_MODE};
pwm_counter_mode_t pwm_counter_mode[3] = {PWM_PWM0_COUNTER_MODE, PWM_PWM0_COUNTER_MODE, PWM_PWM0_COUNTER_MODE};
pwm_dma_smp_fmt_t pwm_dma_smp_fmt[3] = {PWM_PWM0_DMA_SML_FMT, PWM_PWM0_DMA_SML_FMT, PWM_PWM0_DMA_SML_FMT};
uint8_t pwm_element_arr[3] = {PWM_PWM0_ELEMENT_NUM, PWM_PWM0_ELEMENT_NUM, PWM_PWM0_ELEMENT_NUM};
uint8_t pwm_rep_arr[3] = {PWM_PWM0_REPEAT_NUM, PWM_PWM0_REPEAT_NUM, PWM_PWM0_REPEAT_NUM};
uint8_t pwm_dly_arr[3] = {PWM_PWM0_DLY_NUM, PWM_PWM0_DLY_NUM, PWM_PWM0_DLY_NUM};
/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
extern int retarget_stdout_string(char *str, int length, UART_T *p_csr);
extern int retarget_stdout_char(int ch, UART_T *p_csr);
extern void app_init(void);

/*this is pin mux setting*/
static void init_default_pin_mux(void)
{
    int i;

    /*set all pin to gpio, except GPIO16, GPIO17 */
    for (i = 0; i < 32; i++)
    {
        if (i == 20)
        {
            pin_set_mode(i, MODE_PWM0);
        }
        else if (i == 21)
        {
            pin_set_mode(i, MODE_PWM1);
        }
        else if (i == 22)
        {
            pin_set_mode(i, MODE_PWM2);
        }
        else if ((i != 16) && (i != 17))
        {
            pin_set_mode(i, MODE_GPIO);
        }
    }
    gpio_cfg_output(23);
    gpio_pin_set(23);


    return;
}

void pwm_init_parameter(pwm_id_t id, pwm_seq_para_head_t *pwm_para_config)
{
    pwm_seq_para_t *pwm_seq = NULL;

    pwm_para_config->pwm_id            = id;
    pwm_para_config->pwm_play_cnt      = pwm_play_cnt[id]; //0 means continuous
    pwm_para_config->pwm_seq_order     = pwm_seq_order[id];
    pwm_para_config->pwm_seq_num       = pwm_seq_num[id];
    pwm_para_config->pwm_seq_mode      = pwm_seq_mode[id];
    pwm_para_config->pwm_triggered_src = pwm_trigger_src[id];
    pwm_para_config->pwm_clk_div       = pwm_clk_div[id];
    pwm_para_config->pwm_counter_mode  = pwm_counter_mode[id];
    pwm_para_config->pwm_dma_smp_fmt   = pwm_dma_smp_fmt[id];

    if (pwm_para_config->pwm_seq_num == PWM_SEQ_NUM_2)
    {
        pwm_seq = &(pwm_para_config->pwm_seq0);
        pwm_seq->pwm_element_num    = pwm_element_arr[id];
        pwm_seq->pwm_repeat_num     = pwm_rep_arr[id];
        pwm_seq->pwm_delay_num      = pwm_dly_arr[id];
        pwm_seq->pwm_rdma_addr      = (uint32_t)&pwm_rdma0_addr_temp[id];

        pwm_seq = &(pwm_para_config->pwm_seq1);
        pwm_seq->pwm_element_num    = pwm_element_arr[id];
        pwm_seq->pwm_repeat_num     = pwm_rep_arr[id];
        pwm_seq->pwm_delay_num      = pwm_dly_arr[id];
        pwm_seq->pwm_rdma_addr      = (uint32_t)&pwm_rdma1_addr_temp[id];
    }
    else if (pwm_para_config->pwm_seq_num == PWM_SEQ_NUM_1)
    {
        if (pwm_para_config->pwm_seq_order == PWM_SEQ_ORDER_R)
        {
            pwm_seq = &(pwm_para_config->pwm_seq0);
            pwm_seq->pwm_rdma_addr  = (uint32_t)&pwm_rdma0_addr_temp[id];
        }
        else if (pwm_para_config->pwm_seq_order == PWM_SEQ_ORDER_T)
        {
            pwm_seq = &(pwm_para_config->pwm_seq1);
            pwm_seq->pwm_rdma_addr  = (uint32_t)&pwm_rdma1_addr_temp[id];
        }

        pwm_seq->pwm_element_num    = pwm_element_arr[id];
        pwm_seq->pwm_repeat_num     = pwm_rep_arr[id];
        pwm_seq->pwm_delay_num      = pwm_dly_arr[id];
    }

    if (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_0)
    {
        pwm_para_config->pwm_count_end_val = pwm_count_end_val[id];
    }
    else
    {
        pwm_para_config->pwm_count_end_val = 0x00;
    }

}

void set_duty_cycle(pwm_seq_para_head_t *pwm_para_config, uint16_t current_lv)
{
    uint32_t dutycycle ;
    dutycycle = (4000 * current_lv) / 65535;

    pwm_seq_para_t *pwm_seq;
    uint32_t *rdma_addr;

    pwm_seq = &pwm_para_config->pwm_seq0;
    rdma_addr = (uint32_t *)pwm_seq->pwm_rdma_addr;
    *(rdma_addr)  = PWM_FILL_SAMPLE_DATA_MODE1(0, 4000 - dutycycle, 4000);

    Pwm_Start(pwm_para_config);
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
    case BSP_EVENT_UART_RX_RECV:
    case BSP_EVENT_UART_RX_DONE:
    {
        t_app_q.event = APP_UART_EVT;

        xQueueSendToBackFromISR(app_msg_q, &t_app_q, &context_switch);
    }
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
    Delay_Init();

    bsp_init((BSP_INIT_BUTTONS |
              BSP_INIT_DEBUG_CONSOLE), app_bsp_isr_callback);

    utility_register_stdout(bsp_console_stdout_char, bsp_console_stdout_string);
    util_log_init();

    pwm_init_parameter((pwm_id_t)0, &pwm_para_config[0]);
    Pwm_Init(&pwm_para_config[0]);
    pwm_init_parameter((pwm_id_t)1, &pwm_para_config[1]);
    Pwm_Init(&pwm_para_config[1]);
    pwm_init_parameter((pwm_id_t)2, &pwm_para_config[2]);
    Pwm_Init(&pwm_para_config[2]);
    set_duty_cycle(&pwm_para_config[0], 0xFFFF);
    set_duty_cycle(&pwm_para_config[1], 0xFFFF);
    set_duty_cycle(&pwm_para_config[2], 0xFFFF);
    app_init();

    /* Start the scheduler. */
    vTaskStartScheduler();
    while (1)
    {
    }
}

