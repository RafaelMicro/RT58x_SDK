/** @file main.c
 *
 * @brief RTC_Deep_Sleep example main file.
 *
 *
 */
/**
* @defgroup RTC_Deep_Sleep_example_group  RTC_Deep_Sleep
* @ingroup examples_group
* @{
* @brief RTC_Deep_Sleep example demonstrate
*/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"

#include "project_config.h"

#include "uart_drv.h"
#include "retarget.h"
#include "comm_subsystem_drv.h"
#include "rf_mcu_ahb.h"
#define RANDOM_VALUE0    0xDEADBEEF
#define RANDOM_VALUE1    0x52464254
#define RANDOM_VALUE2    0xC0FFEEDC
#define RANDOM_VALUE3    0x4A6F7921
#define RANDOM_VALUE4    0x37BCAF1C


/* In this example: Test timer start in 2021-07-22 11:40:00 */
#define TEST_YEAR     21
#define TEST_MONTH    07
#define TEST_DAY      22
#define TEST_HOUR     11
#define TEST_MIN      40
#define TEST_SEC      00

#define TEST_ALARM_YEAR            TEST_YEAR
#define TEST_ALARM_MONTH           TEST_MONTH
#define TEST_ALARM_DAY             TEST_DAY
#define TEST_ALARM_HOUR            TEST_HOUR
#define TEST_ALARM_MIN             TEST_MIN
#define TEST_ALARM_SEC             01


#define GPIO20                     20
#define GPIO21                     21
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/*select one adress in retention memory to check it value is kept after deep wakeup */
#define TEST_SRAM_RETENTION_ADDR   0x20020100

/*
 *   Set SRAM retentsion memory only keep adress 0x20020000 ~0x20023FFF,
 * All other sram and cache will disappear in deepsleep.
 *
 *   In sleep mode, the following setting will keep all sram data.
 */
#define SRAM_RETENTSION_ONLY_KEPT_SRAM4_IN_DEEPSLEEP          0x300

/*
 *   All sram and cache will disappear in deepsleep.
 *
 *   In sleep mode, the following setting SRAM4 will disappear after sleep wakeup
 * This option is only used for power saving more. (SRAM4 turn off)
 */
#define SRAM_RETENTSION_TURN_OFF_ALLSRAM_IN_DEEPSLEEP           0x310

int main(void);

void SetClockFreq(void);

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200



/************************************************************/

/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    /*set gpio 20 21 for debug used*/
    pin_set_mode(GPIO20, MODE_GPIO);
    pin_set_mode(GPIO21, MODE_GPIO);

    /*uart2 pinmux*/
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

void rtc_arm_isr(uint32_t rtc_status)
{
    rtc_time_t  current_time;

    rtc_get_time(&current_time);

    /*we don't do anything here..*/

    return;
}

int main(void)
{
    rtc_time_t         current_time, tm, alarm_tm;
    uint32_t           check_value, alarm_mode, counter;
    uint32_t           *kept_ptr;
    volatile uint32_t  RTC_init_required = 1;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    /*
     * In this example, we use system retention register 2 to indicate
     * system is extern power-on reset or wakeup from deep sleep.
     *
     */
    sys_get_retention_reg(2, &check_value);

    if (check_value == RANDOM_VALUE2)
    {
        RTC_init_required = 0;
    }
    else
    {
        printf("HELLO RTC test  deeply sleep wakeup\n");
        printf("build %s %s \n", __DATE__, __TIME__);

        printf("Cold reboot \n");
        printf("This example will enter deep sleep mode\n");
        printf("It will wakeup every minute  XX:01 \n");
        printf("You can use let GPIO20 to low to wakeup the SOC too\n");
    }

    rtc_get_time(&current_time);

    printf("current time is %2ld-%2ld-%2ld %2ld:%2ld:%2ld \n",
           current_time.tm_year, current_time.tm_mon, current_time.tm_day,
           current_time.tm_hour, current_time.tm_min, current_time.tm_sec);

    /*
     * In this example, the whole system will enter SOC: deep sleep  RF communication:deep sleep
     *
     */

    Comm_Subsystem_Sram_Deep_Sleep_Init();
    Comm_Subsystem_Disable_LDO_Mode();                                          /*if don't load 569 FW, need to call the function.*/
    Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP3);        /*There is another example code for sleep. Don't change this option here.*/

    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_RTC_TIMER);     /*wakeup source is RTC*/
    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_GPIO20);        /*using gpio20 can wakeup deeply sleep,too. -- default is low active.*/

    if (RTC_init_required == 1)
    {
        current_time.tm_year = TEST_YEAR;
        current_time.tm_mon = TEST_MONTH;
        current_time.tm_day = TEST_DAY;
        current_time.tm_hour = TEST_HOUR;
        current_time.tm_min = TEST_MIN;
        current_time.tm_sec = TEST_SEC;

        /*set RTC current time*/
        rtc_set_time(&current_time);

        rtc_get_time(&tm);

        /*check read write difference...*/
        if ( (current_time.tm_year != tm.tm_year) ||
                (current_time.tm_mon != tm.tm_mon) ||
                (current_time.tm_day != tm.tm_day) ||
                (current_time.tm_hour != tm.tm_hour) ||
                (current_time.tm_min != tm.tm_min) ||
                (current_time.tm_sec != tm.tm_sec) )
        {
            printf("Set RTC and Geat RTC is different \n");
            while (1);
        }

        sys_set_retention_reg(7, 1);            /*next time deep wakeup will skip ISP waiting.*/

        /*just set some randome number in scratchpad*/
        sys_set_retention_reg(0, RANDOM_VALUE0);
        sys_set_retention_reg(1, RANDOM_VALUE1);
        sys_set_retention_reg(2, RANDOM_VALUE2);
        sys_set_retention_reg(3, RANDOM_VALUE3);
        sys_set_retention_reg(4, RANDOM_VALUE4);
        sys_set_retention_reg(5, 0);            /*using retention register 5 as counter.*/

        counter = 0;

        alarm_tm.tm_year = TEST_YEAR;
        alarm_tm.tm_mon = TEST_MONTH;
        alarm_tm.tm_day = TEST_DAY;
        alarm_tm.tm_hour = TEST_HOUR;
        alarm_tm.tm_min = TEST_MIN;
        alarm_tm.tm_sec = (TEST_SEC + 01);

        /* generate event interrrupt at every minute:01  */
        alarm_mode = RTC_MODE_MINUTE_EVENT_INTERRUPT | RTC_MODE_EVENT_INTERRUPT ;

        rtc_set_alarm(&alarm_tm, alarm_mode, rtc_arm_isr);

        /*Only kept address SRAM4 (0x20020000 ~0x20023FFF) in deep sleep period.*/
        Lpm_Set_Sram_Sleep_Deepsleep_Shutdown(SRAM_RETENTSION_ONLY_KEPT_SRAM4_IN_DEEPSLEEP);

        kept_ptr = (uint32_t *) TEST_SRAM_RETENTION_ADDR;

        *kept_ptr =  counter;
    }
    else
    {
        uint32_t   value;

        /*Comment: retention register still keep the value.*/
        sys_get_retention_reg(3, &value);
        if (value != RANDOM_VALUE3)
        {
            /*It will not happen this error*/
            printf("error .scratch3\n");
            while (1);
        }

        sys_get_retention_reg(4, &value);
        if (value != RANDOM_VALUE4)
        {
            /*It will not happen this error*/
            printf("error .scratch4\n");
            while (1);
        }

        /*using retention register as counter*/
        sys_get_retention_reg(5, &counter);

        /*check memory that we write last time  in SRAM4 */
        kept_ptr = (uint32_t *) TEST_SRAM_RETENTION_ADDR;

        if (*kept_ptr != counter)
        {
            printf("error... if you want to test this option, SRAM4 must be in retention mode!\n");
            while (1);
        }

        counter++;

        *kept_ptr = counter;
        sys_set_retention_reg(5, counter);

        sys_get_retention_reg(1, &value);
        if (value != RANDOM_VALUE1)
        {
            /*It will not happen this error*/
            printf("error .scratch1\n");
            while (1);
        }



        alarm_tm.tm_year = TEST_YEAR;
        alarm_tm.tm_mon = TEST_ALARM_MONTH;
        alarm_tm.tm_day = TEST_ALARM_DAY;
        alarm_tm.tm_hour = TEST_ALARM_HOUR;
        alarm_tm.tm_min = TEST_ALARM_MIN;
        alarm_tm.tm_sec =   TEST_ALARM_SEC;

        alarm_mode = RTC_MODE_MINUTE_EVENT_INTERRUPT | RTC_MODE_EVENT_INTERRUPT ;

        /*rtc interrupt will generate, we need it to enable interrupt, and set interrupt ISR*/
        rtc_set_alarm(&alarm_tm, alarm_mode, rtc_arm_isr);      /*reset alarm mode again.*/


    }

    /*
     *  before enter sleep, deep sleep, we must ensure that device in stop state.
     *  That is, if there are data in uart0 it should be flush or wait the message to output complete.
     *  In this example, it use to wait all message print
     */
    //uart_tx_abort(0);              /*urgent... so we don't care the message not sent....*/

    /*wait all message print....*/
    while (1)
    {
        uart_status  uart0_state;

        uart0_state = uart_status_get(0);

        /*wait TX print message finish*/
        if ((uart0_state.tx_busy) == FALSE)
        {
            break;
        }
    }

    Lpm_Enter_Low_Power_Mode();

    /*For deep sleep, code should never go to here.*/
}


void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
