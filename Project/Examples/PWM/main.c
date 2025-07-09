/** @file main.c
 *
 * @brief PWM example main file.
 *        Demonstrate configuration the pwm parameter and output waveform
 *        Demonstrate PWM API usage. PWM can wire with LED, motor, etc.
 */
/**
 * @defgroup PWM_example_group PWM
 * @ingroup examples_group
 * @{
 * @brief PWM example demonstrate.
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
// ---------- D E F I N I T I O N S -------------------- //
#define GPIO8            8              /* for PWM0 pin mux */
#define GPIO9            9              /* for PWM1 pin mux */
#define GPIO14           14             /* for PWM2 pin mux */
#define GPIO15           15             /* for PWM3 pin mux */
#define GPIO20           20             /* for PWM0~PWM4 pin mux */
#define GPIO21           21             /* for PWM0~PWM4 pin mux */
#define GPIO22           22             /* for PWM0~PWM4 pin mux */
#define GPIO23           23             /* for PWM0~PWM4 pin mux */

#if (CHIP_VERSION == RT58X_MPA)
#define PWM_PWM0_GPIO                   GPIO20
#define PWM_PWM1_GPIO                   GPIO21
#define PWM_PWM2_GPIO                   GPIO22
#define PWM_PWM3_GPIO                   GPIO23
#elif (CHIP_VERSION == RT58X_MPB)
#define PWM_PWM0_GPIO                   GPIO8
#define PWM_PWM1_GPIO                   GPIO20
#define PWM_PWM2_GPIO                   GPIO21
#define PWM_PWM3_GPIO                   GPIO22
#define PWM_PWM4_GPIO                   GPIO23
#endif

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/*--------------------------------------------------------------------------------------*/
/* PWM0 Config                                                                          */
/*                                                                                      */
/* PWM Clock  : PWM_PWM0_CNT_END_VAL 10000                                              */
/*              PWM_FILL_SAMPLE_DATA_MODE(PHA, THD1, THD2)                              */
/*                                                                                      */
/*    PWM_FILL_SAMPLE_DATA_MODE1(0, 400, 4000)                                          */
/*              Format 1 PWM Clock = ((48000000 /DIV_4)/4000) = 3Khz                    */
/*              Plus Duty = THD1/THD2 = 400/4000 = 10%                                  */
/*                                                                                      */
/*    PWM_FILL_SAMPLE_DATA_MODE0(0, 1000, 1500)                                         */
/*              Format 0 PWM Clock = ((48000000 /DIV_4)/10000) = 1.2Khz                 */
/*              Plus 1 Duty = THD1/PWM_PWM0_CNT_END_VAL = 10%                           */
/*              Plus 2 Duty = THD2/PWM_PWM0_CNT_END_VAL = 15%                           */
/*                                                                                      */
/* PWM_PWM0_SEQ_ORDER : R-seq or T-seq or both                                          */
/*                                                                                      */
/* PWM_PWM0_TRIG_SRC: Trigger PWM start working by self or otehr pwm                    */
/*                Example : PWM1 Tigger Source using PWM0                               */
/*                       ___   ___   ___   ___   ___   ___   ___   ___                  */
/* PWM 0            ____| 1 |_| 2 |_| 3 |_| 4 |_| 1 |_| 2 |_| 3 |_| 4 |_                */
/*                                                                                      */
/* PWM 1 Triger Source PWM0                                                             */
/*                 ______________________________   ___   ___   ___   ___               */
/*                                               |_| 1 |_| 2 |_| 3 |_| 4 |_             */
/*                                                                                      */
/* PWM_PWM0_DMA_SML_FMT : config the PMW is format 0 or 1                               */
/*                                                                                      */
/*                        PHA : polarity state of PWM output                            */
/*                              0 = PWM output polarity inverse Disabled                */
/*                              1 = PWM output polarity inverse Enabled                 */
/*                                                                                      */
/*                 PHA Example : PWM_FILL_SAMPLE_DATA_MODE1(0, 400, 4000);              */
/*                          ____________     _10%_          ___                         */
/*                                      |____| 1  |__90%___|   |_                       */
/*                                                                                      */
/*                          PWM_FILL_SAMPLE_DATA_MODE1(1, 400, 4000);                   */
/*                          ____________     ___90%___      ______                      */
/*                                      |____|  1    |_10%_|      |_                    */
/*                                                                                      */
/*                                                                                      */
/* PWM_PWM0_SEQ_NUM :  PWM_SEQ_NUM_2 Config R-seq and T-seq xdma params                 */
/*                     PWM_SEQ_NUM_1 Config R-seq or T-seq xdma params                  */
/*                                                                                      */
/* PWM_PWM0_REPEAT_NUM : config repeat the plus                                         */
/*              Example:Repeat Number is 2                                              */
/*                        ___   ___   ___   ___   ___   ___   ___   ___                 */
/*                  ____| 1 |_| 1 |_| 2 |_| 2 |_| 3 |_| 3 |_| 4 |_| 4 |_                */
/*                                                                                      */
/*PWM_PWM0_DLY_NUM : Pulse delay                                                        */
/*           Example: confg delay Number is 2                                           */
/*                       ___   ___   ___   ___   ___   ___   ___   ___   ___            */
/*                  ____| 1 |_| 1 |_| 2 |_| 2 |_| 3 |_| 3 |_| 4 |_| 4d|__|4d|_          */
/*                                                                                      */
/*PWM_SEQ_MODE_CONTINUOUS : Continuous or Noncontinuous mode                            */
/*            Continuous mode Example                                                   */
/*                  ___   ___   ___   ___   ___   ___   ___   ___    ___                */
/*             ____| 1 |_| 2 |_| 3 |_| 1 |_| 2 |_| 3 |_| 1 |_| 2 |__| 3 |_              */
/*                                                                                      */
/*            Noncontinuous mode Example                                                */
/*                 ___   ___   ___   ___   ___   ___   ___   ___                        */
/*            ____| 1 |_| 2 |_| 3 |_| 4 |_| 4 |_| 4 |_| 4 |_| 4 |_                      */
/*                                                                                      */
/*PWM_PWM0_PLAY_CNT : 2                                                                 */
/*                    Example                                                           */
/*                 ___   ___   ___   ___   ___   ___   ___   ___   ___   ___            */
/*            ____| 1 |_| 2 |_| 3 |_| 1 |_| 2 |_| 3 |_| 3 |_| 3 |_| 3 |_| 3 |_          */
/*                                                                                      */
/*--------------------------------------------------------------------------------------*/
#define PWM_PWM0_COUNTER_MODE   PWM_COUNTER_MODE_UP         /*UP is up counter Mode ;UD is up and down Mode*/
#define PWM_PWM0_SEQ_ORDER      PWM_SEQ_ORDER_R             /*Rseq or Tseq Selection or both*/
#define PWM_PWM0_ELEMENT_NUM    8                           /*genrator Pulse number*/
#define PWM_PWM0_REPEAT_NUM     0                           /*Repeat Pulse number*/
#define PWM_PWM0_DLY_NUM        0                           /*Pulse delay number*/
#define PWM_PWM0_CNT_END_VAL    10000                       /*Count end Value*/
#define PWM_PWM0_CLK_DIV        PWM_CLK_DIV_4               /*PWM Input Clock Div*/
#define PWM_PWM0_TRIG_SRC       PWM_TRIGGER_SRC_SELF        /*PWM Trigger Source by self or PWM1~PWM4*/
#define PWM_PWM0_SEQ_MODE       PWM_SEQ_MODE_CONTINUOUS     /*Continuous and Noncontinuous mode*/
#define PWM_PWM0_PLAY_CNT       0                           /*0:is infinite*/
#define PWM_PWM0_SEQ_NUM        PWM_SEQ_NUM_1               /*use rdma single simple or two simple*/
#define PWM_PWM0_DMA_SML_FMT    PWM_DMA_SMP_FMT_1           /*Pwm DMA Simple Format 0 or 1*/

#define PWM_PWM1_COUNTER_MODE   PWM_COUNTER_MODE_UP
#define PWM_PWM1_SEQ_ORDER      PWM_SEQ_ORDER_T
#define PWM_PWM1_ELEMENT_NUM    8
#define PWM_PWM1_REPEAT_NUM     1
#define PWM_PWM1_DLY_NUM        2
#define PWM_PWM1_CNT_END_VAL    10000
#define PWM_PWM1_CLK_DIV        PWM_CLK_DIV_8
#define PWM_PWM1_TRIG_SRC       PWM_TRIGGER_SRC_PWM0
#define PWM_PWM1_SEQ_MODE       PWM_SEQ_MODE_CONTINUOUS
#define PWM_PWM1_PLAY_CNT       3
#define PWM_PWM1_DMA_SML_FMT    PWM_DMA_SMP_FMT_1
#define PWM_PWM1_SEQ_NUM        PWM_SEQ_NUM_1

#define PWM_PWM2_COUNTER_MODE   PWM_COUNTER_MODE_UP
#define PWM_PWM2_SEQ_ORDER      PWM_SEQ_ORDER_R
#define PWM_PWM2_ELEMENT_NUM    8
#define PWM_PWM2_REPEAT_NUM     0
#define PWM_PWM2_DLY_NUM        0
#define PWM_PWM2_CNT_END_VAL    10000
#define PWM_PWM2_CLK_DIV        PWM_CLK_DIV_16
#define PWM_PWM2_TRIG_SRC       PWM_TRIGGER_SRC_SELF
#define PWM_PWM2_SEQ_MODE       PWM_SEQ_MODE_CONTINUOUS
#define PWM_PWM2_PLAY_CNT       0
#define PWM_PWM2_SEQ_NUM        PWM_SEQ_NUM_2
#define PWM_PWM2_DMA_SML_FMT    PWM_DMA_SMP_FMT_1

#define PWM_PWM3_COUNTER_MODE   PWM_COUNTER_MODE_UP
#define PWM_PWM3_SEQ_ORDER      PWM_SEQ_ORDER_T
#define PWM_PWM3_ELEMENT_NUM    8
#define PWM_PWM3_REPEAT_NUM     0
#define PWM_PWM3_DLY_NUM        0
#define PWM_PWM3_CNT_END_VAL    10000
#define PWM_PWM3_CLK_DIV        PWM_CLK_DIV_16
#define PWM_PWM3_TRIG_SRC       PWM_TRIGGER_SRC_SELF
#define PWM_PWM3_SEQ_MODE       PWM_SEQ_MODE_CONTINUOUS
#define PWM_PWM3_PLAY_CNT       3
#define PWM_PWM3_SEQ_NUM        PWM_SEQ_NUM_2
#define PWM_PWM3_DMA_SML_FMT    PWM_DMA_SMP_FMT_1

#define PWM_PWM4_COUNTER_MODE   PWM_COUNTER_MODE_UP
#define PWM_PWM4_SEQ_ORDER      PWM_SEQ_ORDER_R
#define PWM_PWM4_ELEMENT_NUM    8
#define PWM_PWM4_REPEAT_NUM     0
#define PWM_PWM4_DLY_NUM        0
#define PWM_PWM4_CNT_END_VAL    10000
#define PWM_PWM4_CLK_DIV        PWM_CLK_DIV_4
#define PWM_PWM4_TRIG_SRC       PWM_TRIGGER_SRC_PWM4
#define PWM_PWM4_SEQ_MODE       PWM_SEQ_MODE_CONTINUOUS
#define PWM_PWM4_PLAY_CNT       3
#define PWM_PWM4_SEQ_NUM        PWM_SEQ_NUM_2
#define PWM_PWM4_DMA_SML_FMT    PWM_DMA_SMP_FMT_0

/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */
#define PRINTF_BAUDRATE      UART_BAUDRATE_115200
/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
//==================================================================//
uint32_t pwm0_rdma0[PWM_PWM0_ELEMENT_NUM], pwm0_rdma1[PWM_PWM0_ELEMENT_NUM];
uint32_t pwm1_rdma0[PWM_PWM1_ELEMENT_NUM], pwm1_rdma1[PWM_PWM1_ELEMENT_NUM];
uint32_t pwm2_rdma0[PWM_PWM2_ELEMENT_NUM], pwm2_rdma1[PWM_PWM2_ELEMENT_NUM];
uint32_t pwm3_rdma0[PWM_PWM3_ELEMENT_NUM], pwm3_rdma1[PWM_PWM3_ELEMENT_NUM];
uint32_t pwm4_rdma0[PWM_PWM4_ELEMENT_NUM], pwm4_rdma1[PWM_PWM4_ELEMENT_NUM];
uint16_t pwm_count_end_val[5] = {PWM_PWM0_CNT_END_VAL, PWM_PWM1_CNT_END_VAL, PWM_PWM2_CNT_END_VAL, PWM_PWM3_CNT_END_VAL, PWM_PWM4_CNT_END_VAL};
pwm_clk_div_t pwm_clk_div[5] = {PWM_PWM0_CLK_DIV, PWM_PWM1_CLK_DIV, PWM_PWM2_CLK_DIV, PWM_PWM3_CLK_DIV, PWM_PWM4_CLK_DIV};
pwm_seq_order_t pwm_seq_order[5] = {PWM_PWM0_SEQ_ORDER, PWM_PWM1_SEQ_ORDER, PWM_PWM2_SEQ_ORDER, PWM_PWM3_SEQ_ORDER, PWM_PWM4_SEQ_ORDER};
pwm_trigger_src_t pwm_trigger_src[5] = {PWM_PWM0_TRIG_SRC, PWM_PWM1_TRIG_SRC, PWM_PWM2_TRIG_SRC, PWM_PWM3_TRIG_SRC, PWM_PWM4_TRIG_SRC};
uint16_t pwm_play_cnt[5] = {PWM_PWM0_PLAY_CNT, PWM_PWM1_PLAY_CNT, PWM_PWM2_PLAY_CNT, PWM_PWM3_PLAY_CNT, PWM_PWM4_PLAY_CNT};
pwm_seq_num_t pwm_seq_num[5] = {PWM_PWM0_SEQ_NUM, PWM_PWM1_SEQ_NUM, PWM_PWM2_SEQ_NUM, PWM_PWM3_SEQ_NUM, PWM_PWM4_SEQ_NUM};
pwm_seq_mode_t pwm_seq_mode[5] = {PWM_PWM0_SEQ_MODE, PWM_PWM1_SEQ_MODE, PWM_PWM2_SEQ_MODE, PWM_PWM3_SEQ_MODE, PWM_PWM4_SEQ_MODE};
pwm_counter_mode_t pwm_counter_mode[5] = {PWM_PWM0_COUNTER_MODE, PWM_PWM1_COUNTER_MODE, PWM_PWM2_COUNTER_MODE, PWM_PWM3_COUNTER_MODE, PWM_PWM4_COUNTER_MODE};
pwm_dma_smp_fmt_t pwm_dma_smp_fmt[5] = {PWM_PWM0_DMA_SML_FMT, PWM_PWM1_DMA_SML_FMT, PWM_PWM2_DMA_SML_FMT, PWM_PWM3_DMA_SML_FMT, PWM_PWM4_DMA_SML_FMT};
uint8_t pwm_element_arr[5] = {PWM_PWM0_ELEMENT_NUM, PWM_PWM1_ELEMENT_NUM, PWM_PWM2_ELEMENT_NUM, PWM_PWM3_ELEMENT_NUM, PWM_PWM4_ELEMENT_NUM};
uint8_t pwm_rep_arr[5] = {PWM_PWM0_REPEAT_NUM, PWM_PWM1_REPEAT_NUM, PWM_PWM2_REPEAT_NUM, PWM_PWM3_REPEAT_NUM, PWM_PWM4_REPEAT_NUM};
uint8_t pwm_dly_arr[5] = {PWM_PWM0_DLY_NUM, PWM_PWM1_DLY_NUM, PWM_PWM2_DLY_NUM, PWM_PWM3_DLY_NUM, PWM_PWM4_DLY_NUM};
/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
/**
 * @brief Initinal GPIO Pin mux  for PWM
 */
/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
#if (CHIP_VERSION == RT58X_MPA)
    pin_set_mode(PWM_PWM0_GPIO, MODE_PWM0);
    pin_set_mode(PWM_PWM1_GPIO, MODE_PWM1);
    pin_set_mode(PWM_PWM2_GPIO, MODE_PWM2);
    pin_set_mode(PWM_PWM3_GPIO, MODE_PWM3);
#elif (CHIP_VERSION == RT58X_MPB)
#if (PWM_PWM0_GPIO == GPIO8)
    pin_set_mode(PWM_PWM0_GPIO, MODE_PWM);
#else
    pin_set_mode(PWM_PWM0_GPIO, MODE_PWM0);
#endif

#if (PWM_PWM1_GPIO == GPIO9)
    pin_set_mode(PWM_PWM1_GPIO, MODE_PWM);
#else
    pin_set_mode(PWM_PWM1_GPIO, MODE_PWM1);
#endif

#if (PWM_PWM2_GPIO == GPIO14)
    pin_set_mode(PWM_PWM2_GPIO, MODE_PWM);
#else
    pin_set_mode(PWM_PWM2_GPIO, MODE_PWM2);
#endif

#if (PWM_PWM3_GPIO == GPIO15)
    pin_set_mode(PWM_PWM3_GPIO, MODE_PWM);
#else
    pin_set_mode(PWM_PWM3_GPIO, MODE_PWM3);
#endif

    pin_set_mode(PWM_PWM4_GPIO, MODE_PWM4);
#endif

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
 * @brief this is used to fill configuations for PWM parameter set
 * @param[in] id  Select Pmw channel
 * @param[in] pwm_para_config Pointer to PWM configuration parameters.
 *            \arg pwm_seq0             Handle of PWM sequence controller configurations for R-SEQ
 *            \arg pwm_seq1             Handle of PWM sequence controller configurations for T-SEQ
 *            \arg pwm_play_cnt         PWM play amount configuration
 *            \arg pwm_count_end_val    PWM counter end value configuration
 *            \arg pwm_seq_order        PWM sequence order configuration
 *            \arg pwm_triggered_src    PWM play trigger source configuration
 *            \arg pwm_seq_num          PWM sequence number configuration
 *            \arg pwm_id               PWM ID designation
 *            \arg pwm_clk_div          PWM clock divider value configuration
 *            \arg pwm_counter_mode     PWM counter mode configuration
 *            \arg pwm_dma_smp_fmt      PWM DMA sample format configuration
 *            \arg pwm_seq_mode         PWM sequence play mode configuration
 * @return None
 */
void pwm_init_parameter(pwm_id_t id, pwm_seq_para_head_t *pwm_para_config)
{
    pwm_seq_para_t *pwm_seq = NULL;
    uint32_t *first_rdma0_addr, *first_rdma1_addr;


    //out of PWM ID range
    if (id >= PWM_ID_MAX)
    {
        ASSERT();
    }

    switch (id)
    {
    case PWM_ID_0:
        first_rdma0_addr = pwm0_rdma0;
        first_rdma1_addr = pwm0_rdma1;
        break;

    case PWM_ID_1:
        first_rdma0_addr = pwm1_rdma0;
        first_rdma1_addr = pwm1_rdma1;
        break;

    case PWM_ID_2:
        first_rdma0_addr = pwm2_rdma0;
        first_rdma1_addr = pwm2_rdma1;
        break;

    case PWM_ID_3:
        first_rdma0_addr = pwm3_rdma0;
        first_rdma1_addr = pwm3_rdma1;
        break;

    case PWM_ID_4:
        first_rdma0_addr = pwm4_rdma0;
        first_rdma1_addr = pwm4_rdma1;
        break;
    default:
        break;
    }

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
        pwm_seq->pwm_rdma_addr      = (uint32_t)first_rdma0_addr;

        pwm_seq = &(pwm_para_config->pwm_seq1);
        pwm_seq->pwm_element_num    = pwm_element_arr[id];
        pwm_seq->pwm_repeat_num     = pwm_rep_arr[id];
        pwm_seq->pwm_delay_num      = pwm_dly_arr[id];
        pwm_seq->pwm_rdma_addr      = (uint32_t)first_rdma1_addr;
    }
    else if (pwm_para_config->pwm_seq_num == PWM_SEQ_NUM_1)
    {
        if (pwm_para_config->pwm_seq_order == PWM_SEQ_ORDER_R)
        {
            pwm_seq = &(pwm_para_config->pwm_seq0);
            pwm_seq->pwm_rdma_addr  = (uint32_t)first_rdma0_addr;
        }
        else if (pwm_para_config->pwm_seq_order == PWM_SEQ_ORDER_T)
        {
            pwm_seq = &(pwm_para_config->pwm_seq1);
            pwm_seq->pwm_rdma_addr  = (uint32_t)first_rdma1_addr;
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
/**
 * @brief Function according to the simple format to fill data into rdma memory
 * @param[in] pwm_para_config Pointer to PWM configuration parameters.
 *            \arg pwm_seq0             Handle of PWM sequence controller configurations for R-SEQ
 *            \arg pwm_seq1             Handle of PWM sequence controller configurations for T-SEQ
 *            \arg pwm_id               PWM ID designation
 *            \arg pwm_dma_smp_fmt      PWM DMA sample format configuration
 * @return None
 */
void pwm_fill_para_into_memory(pwm_seq_para_head_t *pwm_para_config)
{
    switch ((pwm_id_t)pwm_para_config->pwm_id)
    {
        pwm_seq_para_t *pwm_seq;
        uint32_t *rdma_addr;

    case PWM_ID_0:
        pwm_seq = &pwm_para_config->pwm_seq0;
        rdma_addr = (uint32_t *)pwm_seq->pwm_rdma_addr;
        *(rdma_addr + 0)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 1000, 1500);
        *(rdma_addr + 1)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 20, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 2000, 2500);
        *(rdma_addr + 2)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 3000, 3500);
        *(rdma_addr + 3)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 40, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4500);
        *(rdma_addr + 4)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 50, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 5000, 5500);
        *(rdma_addr + 5)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 60, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 6000, 6500);
        *(rdma_addr + 6)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 70, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 7000, 7500);
        *(rdma_addr + 7)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 80, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 8000, 8500);

        pwm_seq = &pwm_para_config->pwm_seq1;
        rdma_addr = (uint32_t *)pwm_seq->pwm_rdma_addr;
        *(rdma_addr + 0)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 1)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 2)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 3)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 4)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 5)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 6)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 7)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 80, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        break;
    case PWM_ID_1:
        pwm_seq = &pwm_para_config->pwm_seq0;
        rdma_addr = (uint32_t *)pwm_seq->pwm_rdma_addr;
        *(rdma_addr + 0)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 1000, 1000);
        *(rdma_addr + 1)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 1000, 1000);
        *(rdma_addr + 2)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 1000, 1000);
        *(rdma_addr + 3)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 1000, 1000);
        *(rdma_addr + 4)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 1000, 1000);
        *(rdma_addr + 5)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 1000, 1000);
        *(rdma_addr + 6)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 1000, 1000);
        *(rdma_addr + 7)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 80, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 1000, 1000);

        pwm_seq = &pwm_para_config->pwm_seq1;
        rdma_addr = (uint32_t *)pwm_seq->pwm_rdma_addr;
        *(rdma_addr + 0)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 5, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 1)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 15, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 2)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 25, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 3)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 35, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 4)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 45, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 5)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 55, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 6)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 65, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 7)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 75, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        break;
    case PWM_ID_2:
        pwm_seq = &pwm_para_config->pwm_seq0;
        rdma_addr = (uint32_t *)pwm_seq->pwm_rdma_addr;
        *(rdma_addr + 0)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 1)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 15, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 2)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 20, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 3)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 25, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 4)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 5)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 35, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 6)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 40, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 7)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 45, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);

        pwm_seq = &pwm_para_config->pwm_seq1;
        rdma_addr = (uint32_t *)pwm_seq->pwm_rdma_addr;
        *(rdma_addr + 0)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 50, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 1)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 55, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 2)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 60, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 3)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 65, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 4)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 70, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 5)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 75, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 6)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 80, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 7)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 85, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        break;
    case PWM_ID_3:
        pwm_seq = &pwm_para_config->pwm_seq0;
        rdma_addr = (uint32_t *)pwm_seq->pwm_rdma_addr;
        *(rdma_addr + 0)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 1)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 20, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 2)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 3)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 40, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 4)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 50, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 5)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 60, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 6)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 70, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 7)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 80, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);

        pwm_seq = &pwm_para_config->pwm_seq1;
        rdma_addr = (uint32_t *)pwm_seq->pwm_rdma_addr;
        *(rdma_addr + 0)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 80, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 1)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 70, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 2)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 60, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 3)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 50, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 4)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 40, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 5)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 6)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 20, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 7)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        break;
    case PWM_ID_4:
        pwm_seq = &pwm_para_config->pwm_seq0;
        rdma_addr = (uint32_t *)pwm_seq->pwm_rdma_addr;
        *(rdma_addr + 0)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 1000, 1000);
        *(rdma_addr + 1)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 20, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 2000, 2000);
        *(rdma_addr + 2)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 3000, 3000);
        *(rdma_addr + 3)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 40, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 4000, 4000);
        *(rdma_addr + 4)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 50, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 5000, 5000);
        *(rdma_addr + 5)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 60, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 6000, 6000);
        *(rdma_addr + 6)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 70, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 7000, 7000);
        *(rdma_addr + 7)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 80, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 8000, 8000);

        pwm_seq = &pwm_para_config->pwm_seq1;
        rdma_addr = (uint32_t *)pwm_seq->pwm_rdma_addr;
        *(rdma_addr + 0)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 80, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 5500, 5500);
        *(rdma_addr + 1)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 70, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 6500, 6500);
        *(rdma_addr + 2)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 60, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 7500, 7500);
        *(rdma_addr + 3)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 50, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 8500, 8500);
        *(rdma_addr + 4)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 40, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 9500, 9500);
        *(rdma_addr + 5)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 30, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 1500, 1500);
        *(rdma_addr + 6)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 20, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 2500, 2500);
        *(rdma_addr + 7)  = (pwm_para_config->pwm_dma_smp_fmt == PWM_DMA_SMP_FMT_1) ? PWM_FILL_SAMPLE_DATA_MODE1(0, 40 * 10, 4000) : PWM_FILL_SAMPLE_DATA_MODE0(0, 3500, 3500);
        break;
    default:
        break;
    }
}

int main(void)
{
    pwm_seq_para_head_t pwm_para_config[5];
    uint8_t loop;

    Change_Ahb_System_Clk(SYS_48MHZ_CLK);
    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("PWM Start\r\n");

    for (loop = 0; loop < 5; loop++)
    {
        /* fill the parameters will be used. */
        pwm_init_parameter((pwm_id_t)loop, &pwm_para_config[loop]);
        /* Initial PWM */
        Pwm_Init(&pwm_para_config[loop]);
        /*  Fill parameters into ram    */
        pwm_fill_para_into_memory(&pwm_para_config[loop]);
        /* Start PWM */
        Pwm_Start(&pwm_para_config[loop]);
    }

    while (1) {}

}
/** @} */ /* end of examples group */
