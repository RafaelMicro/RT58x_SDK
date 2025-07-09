
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "project_config.h"

#include "uart_drv.h"
#include "retarget.h"
#include "comm_subsystem_drv.h"
#include "rf_mcu_ahb.h"
/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

int main(void);

void SetClockFreq(void);

#define RANDOM_VALUE0    0xDEADBEEF
#define RANDOM_VALUE1    0x52464254
#define RANDOM_VALUE2    0xC0FFEECC
#define RANDOM_VALUE3    0x4A6F7921
#define RANDOM_VALUE4    0x37BCAF1C
#define RANDOM_VALUE5    0x00000000

#define GPIO29           29
#define GPIO31           31

/* In this example: Test timer start in 2021-07-22 11:40:00 */
#define TEST_YEAR        21
#define TEST_MONTH       07
#define TEST_DAY         22
#define TEST_HOUR        11
#define TEST_MIN         40
#define TEST_SEC         00

#define TEST_ALARM_YEAR            TEST_YEAR
#define TEST_ALARM_MONTH           TEST_MONTH
#define TEST_ALARM_DAY             TEST_DAY
#define TEST_ALARM_HOUR            TEST_HOUR
#define TEST_ALARM_MIN             TEST_MIN
#define TEST_ALARM_SEC             (TEST_SEC+1)

#define ASCII_0                    0x30
#define ASCII_2                    0x32

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/*Global parameter */

/************************************************************/

/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    /*uart0 pinmux, This is default setting,
      we set it for safety. */
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/

    /*set GPIO29 for debug*/
    pin_set_mode(GPIO29, MODE_GPIO);

    /*set GPIO31 for debug*/
    pin_set_mode(GPIO31, MODE_GPIO);

    /*set GPIO as ouput, system will set the pin to no pull mode*/
    gpio_cfg_output(GPIO29);
    gpio_pin_clear(GPIO29);

    gpio_cfg_output(GPIO31);
    gpio_pin_clear(GPIO31);

    return;
}

void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}
/* rtc_arm_isr is RTC user interrupt, please do NOT call any block function
 * And you should design the code as short as possible.
 * Here we demo here is a very bad example.
 */

void rtc_arm_isr(uint32_t rtc_status)
{
    /*toggle gpio29 for isr interrupt happen*/

    gpio_pin_toggle(GPIO29);

    return;
}

/*
 *    Because GPIO interrupt will wakeup SOC CPU, too.
 * For this sample code, we hope to check RTC wakeup only, not trigger by GPIO.
 * So we do NOT use press button to select power mode.
 *
 */

int main(void)
{
    rtc_time_t   current_time, tm, alarm_tm;
    uint32_t     check_value, alarm_mode;

    uart_status  uart0_state;
    uint8_t      select_option;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    printf("HELLO RTC sleep wakeup test\n");

    Comm_Subsystem_Sram_Deep_Sleep_Init();
    Comm_Subsystem_Disable_LDO_Mode();                                          /*if don't load 569 FW, need to call the function.*/
    Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);        /* default LOW_POWER_LEVEL_SLEEP0 */

    sys_get_retention_reg(2, &check_value);

    if (check_value == RANDOM_VALUE2)
    {
        /*For sleep mode, it should not goto here.*/
        printf("Error: Not cold reboot... \n");
    }
    else
    {
        printf("Cold boot \n");
    }

    rtc_get_time(&current_time);

    printf("current time is %2ld-%2ld-%2ld %2ld:%2ld:%2ld \n",
           current_time.tm_year, current_time.tm_mon, current_time.tm_day,
           current_time.tm_hour, current_time.tm_min, current_time.tm_sec);

    /*this is binary format data */
    current_time.tm_year =  TEST_YEAR;
    current_time.tm_mon  =  TEST_MONTH;
    current_time.tm_day  =  TEST_DAY;
    current_time.tm_hour =  TEST_HOUR;
    current_time.tm_min  =  TEST_MIN;
    current_time.tm_sec  =  TEST_SEC;

    /*set RTC current time*/
    rtc_set_time(&current_time);

    /*get RTC current time for compare*/
    rtc_get_time(&tm);

    /*just set some randome number in sys rention */
    sys_set_retention_reg(0, RANDOM_VALUE0);
    sys_set_retention_reg(1, RANDOM_VALUE1);
    sys_set_retention_reg(2, RANDOM_VALUE2);
    sys_set_retention_reg(3, RANDOM_VALUE3);
    sys_set_retention_reg(4, RANDOM_VALUE4);
    sys_set_retention_reg(5, RANDOM_VALUE5);

    /*test Alarm */

    alarm_tm.tm_year = TEST_YEAR;
    alarm_tm.tm_mon  = TEST_MONTH;
    alarm_tm.tm_day  = TEST_DAY;
    alarm_tm.tm_hour = TEST_HOUR;
    alarm_tm.tm_min  = TEST_MIN;
    alarm_tm.tm_sec  = (TEST_SEC + 1);

    /* following setting, interrupt in tm_sec = (TEST_SEC + 1) ---
     *  this interrupt will generated every minutue once for second match (TEST_SEC + 1).
     */
    //alarm_mode = RTC_MODE_MATCH_SEC_INTERRUPT| RTC_MODE_EN_SEC_INTERRUPT ;

    /* following setting, interrupt will generated every second,
     * that is it does not care what value of tm_sec
     */
    //alarm_mode = RTC_MODE_EVERY_SEC_INTERRUPT| RTC_MODE_EN_SEC_INTERRUPT;    /*Every second will generate RTC interrupt.*/

    /* following setting, interrrupt will generate every minutes mm:00 */
    alarm_mode = RTC_MODE_EVERY_MIN_INTERRUPT | RTC_MODE_EN_MIN_INTERRUPT ;

    rtc_set_alarm(&alarm_tm, alarm_mode, rtc_arm_isr);

    /*wait some uart message to print finish--- you can set a small delay timer, too */
    printf("Please select mode you want to test: \n");
    printf("0: For CPU sleep  RF run  \n");
    printf("1: For CPU sleep  RF sleep \n");
    printf("2: For CPU sleep  RF deep sleep\n");

    while (1)
    {
        /*wait user to select correct sleep mode 0/1/2*/
        select_option = getchar();

        if ((select_option >= ASCII_0) && (select_option <= ASCII_2))
        {
            break;
        }
    }

    printf("Select  LOW_POWER_LEVEL_SLEEP%c mode \n", select_option);
    printf("Testing... \n");

    /*
     * wait uart0 printf message finish. Remark: IT CAN NOT sleep when TX running!
     * wait TX finish or  abort non-transfer message....
     *
     */
    while (1)
    {
        uart0_state = uart_status_get(0);

        /*wait TX print message finish*/
        if ((uart0_state.tx_busy) == FALSE)
        {
            break;
        }
    }

    Lpm_Set_Low_Power_Level( (low_power_level_cfg_t) (LOW_POWER_LEVEL_SLEEP0 + (select_option - ASCII_0)));
    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_RTC_TIMER);     /*wakeup source is RTC*/

    /* In this example, system will enter sleep, it will wakeup at the "alarm time" depends on setting.
     * CPU will not response any ICE command..
     * So please use ISP tool to program new firmware or switch the EVK
     * jump to enter ISP mode to program new firmware.
     */

    while (1)
    {
        /* you can do something before enter sleep here...
         * in this example we only toggle GPIO31.
         */

        gpio_pin_toggle(GPIO31);

        Lpm_Enter_Low_Power_Mode();

        rtc_get_time(&current_time);

        printf("wakeup time is %2ld-%2ld-%2ld %2ld:%2ld:%2ld \n",
               current_time.tm_year, current_time.tm_mon, current_time.tm_day,
               current_time.tm_hour, current_time.tm_min, current_time.tm_sec);

        /*you can do something after wakeup here...*/
        while (1)
        {
            uart0_state = uart_status_get(0);

            /*wait TX print message finish*/
            if ((uart0_state.tx_busy) == FALSE)
            {
                break;
            }
        }

    }

}

void SetClockFreq(void)
{
    return;
}
