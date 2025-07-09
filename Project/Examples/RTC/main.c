/** @file main.c
 *
 * @brief RTC example main file.
 *
 *
 */
/**
* @defgroup RTC_example_group  RTC
* @ingroup examples_group
* @{
* @brief RTC example demonstrate
*/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "project_config.h"
#include "rf_mcu_ahb.h"
#include "uart_drv.h"
#include "retarget.h"

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

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02

#define GPIO29           29


#ifdef _fast_test_
/*this is just for fast test RTC counter*/
#define SCALAR           32
#else
/*This is real 1 second.*/
#define SCALAR          4000
#endif


#define dgpio_pin_clear(n)      gpio_pin_clear(n)
#define dgpio_pin_set(n)        gpio_pin_set(n)
#define dgpio_cfg_output(n)     gpio_cfg_output(n)

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

    return;
}

void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}
volatile uint32_t   TIMER0TickCount = 0;
#define RT_TIMER0     0

void timer_handler(uint32_t timer_id)
{
    rtc_time_t current_time;

    /* here we check RTC "every second" */
    if ((TIMER0TickCount % 60) == 0)
    {
        rtc_get_time(&current_time);

        /* we start timer at tm_sec=01, and after "1 second" later
         * timer0 interrupt generated, and now TIMER0TickCount is 0
         * , tm_sec should be tm_sec=02
         */


    }



    TIMER0TickCount++;


}

void do_timer_init(void)
{
    timer_config_mode_t cfg;

    cfg.mode = TIMER_PERIODIC_MODE;
    /*the input clock is 32M/s, so it will become 1M ticks per second */
    cfg.prescale = TIMER_PRESCALE_32;
    cfg.int_en  =  ENABLE;

    Timer_Open(RT_TIMER0, cfg, timer_handler);

}

/* rtc_arm_isr is RTC user interrupt, please do NOT call any block function
 * And you should design the code as short as possible.
 * Here we demo here is a very bad example.
 */

void rtc_arm_isr(uint32_t rtc_status)
{
    rtc_time_t  current_time, alarm_tm;
    uint32_t    alarm_mode;
    int        show_time_info = 0;
    static  int  first = 1;

    rtc_get_time(&current_time);

    /*check status to know why interrupt source*/
    if (rtc_status & RTC_INT_SECOND_BIT)
    {
        if (first)
        {
            /* In our test design, the default clk is 32000,
             * we should wait it to end. All code here is for test only.
             * It will not (and should not) used in real product.
             */
            Timer_Start(RT_TIMER0, (250 * SCALAR));   /*one scalar is 250us*/
            first = 0;

            alarm_tm.tm_year = 20;
            alarm_tm.tm_mon = 03;
            alarm_tm.tm_day = 21;
            alarm_tm.tm_hour = 12;
            alarm_tm.tm_min = 30;
            alarm_tm.tm_sec = 48;

            /*we hope to get interrupt at
                      XX:XX:00  -- every new second.
                      XX:30:00  -- for every hour
                      12:00:00  -- for every day   --RTC_MODE_MATCH_HOUR_INTERRUPT
                      00:00:00 -- for every day    --RTC_MODE_EVERY_DAY_INTERRUPT

                   01 00:00:00 -- for every month
                01-01 00:00:00 -- for every year
                And last we hope to get
                      XX:30:48 ... for event interrupt.
             */

            alarm_mode = RTC_MODE_EVERY_SEC_INTERRUPT  | RTC_MODE_EN_SEC_INTERRUPT  |
                         RTC_MODE_MATCH_MIN_INTERRUPT  | RTC_MODE_EN_MIN_INTERRUPT  |
                         RTC_MODE_MATCH_HOUR_INTERRUPT | RTC_MODE_EN_HOUR_INTERRUPT |
                         RTC_MODE_EVERY_DAY_INTERRUPT  | RTC_MODE_EN_DAY_INTERRUPT  |
                         RTC_MODE_EVERY_MONTH_INTERRUPT | RTC_MODE_EN_MONTH_INTERRUPT |
                         RTC_MODE_EVERY_YEAR_INTERRUPT | RTC_MODE_EN_YEAR_INTERRUPT |
                         RTC_MODE_HOUR_EVENT_INTERRUPT | RTC_MODE_EVENT_INTERRUPT ;


            rtc_set_alarm(&alarm_tm, alarm_mode, rtc_arm_isr);

        }
        else
        {
            /*we don't show meesage.. because too many to show*/
            gpio_pin_toggle(GPIO29);
        }
    }

    if (rtc_status & RTC_INT_MINUTE_BIT)
    {
        show_time_info = 1;
    }

    if (rtc_status & RTC_INT_HOUR_BIT)
    {
        show_time_info = 1;
    }

    if (rtc_status & RTC_INT_DAY_BIT)
    {
        show_time_info = 1;
        printf("day int %lx\n", rtc_status);
    }


    if (rtc_status & RTC_INT_MONTH_BIT)
    {
        show_time_info = 1;
        printf("month int %lx\n", rtc_status);
    }

    if (rtc_status & RTC_INT_YEAR_BIT)
    {
        show_time_info = 1;
        printf("year int %lx\n", rtc_status);
    }

    if (rtc_status & RTC_INT_EVENT_BIT)
    {
        show_time_info = 1;
        printf("event int %lx\n", rtc_status);
    }

    if (show_time_info)
    {
        printf("RTC INT time is %2ld-%2ld-%2ld %2ld:%2ld:%2ld, count %ld \n",
               current_time.tm_year, current_time.tm_mon, current_time.tm_day,
               current_time.tm_hour, current_time.tm_min, current_time.tm_sec, TIMER0TickCount);

    }

    return;
}



int main(void)
{
    rtc_time_t current_time, tm, alarm_tm;
    uint32_t   check_value, i, alarm_mode;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.
    /*    Warning: We will call printf in timer0 and RTC ISR...
     * This is very STUPID design.. here we just do this for debug used
     * But "printf in ISR" will block the ISR when the sendbuf fifo full.
     * If RTC isr priority higher or equal the uart0 priority,
     * It will deadlock!!
     * To fix this deadlock, we should set uart priority higher than
     *  RTC and timer0
     *   priority 0 is highest priority. priority7 is smallest priority.
     *
     *  Notice: priority setting is MSB 3 bits in hardware, but in
     *  function NVIC_SetPriority(..., priority) it will use LSB 3 bits.
     *  In that function, it will adjust the priority value to (priority_n<<5)
     *  for hardware register setting.
     *  Please use IRQ_PRIORITY_XXXX to set interrupt priority, instead of number.
     */


    /*
     * Even timer0 irq priority is the same as RTC,
     * timer0 is still higher priority than RTC. This is because
     * hardware design.
     */

    NVIC_SetPriority(Timer0_IRQn, IRQ_PRIORITY_LOWEST);

    NVIC_SetPriority(Rtc_IRQn, IRQ_PRIORITY_LOWEST);


    printf("HELLO RTC test\n");
    printf("This example show rtc interrupt in different mode \n");
    printf("GPIO29 will toggle every second\n");

    rtc_get_time(&current_time);

    printf("current time is %2ld-%2ld-%2ld %2ld:%2ld:%2ld \n",
           current_time.tm_year, current_time.tm_mon, current_time.tm_day,
           current_time.tm_hour, current_time.tm_min, current_time.tm_sec);

    /*this is binary format data, we set 2020-02-28 23:58:00 */
    current_time.tm_year = 20;
    current_time.tm_mon = 02;
    current_time.tm_day = 28;
    current_time.tm_hour = 23;
    current_time.tm_min = 58;
    current_time.tm_sec = 00;

#ifdef  _fast_test_
    /*This is test mode. one scalar is 250 us */
    rtc_set_clk((SCALAR << 3));
#else
    /*This is FPGA adjust clock --- this value should change depends on real ASIC*/
#if (EXT32K_GPIO_ENABLE==1)
    rtc_set_clk(32768);     //External 32.768khz
#else
    rtc_set_clk(39990);     //Internal 40khz
#endif
#endif

    /*we will set timer clock to help verify */
    gpio_cfg_output(GPIO29);
    gpio_pin_clear(GPIO29);

    do_timer_init();

    /*set RTC current time*/
    rtc_set_time(&current_time);



    /*get RTC current time for compare*/
    rtc_get_time(&tm);

    printf("Read current time is %02ld-%02ld-%02ld %02ld:%02ld:%02ld \n",
           tm.tm_year, tm.tm_mon, tm.tm_day,
           tm.tm_hour, tm.tm_min, tm.tm_sec);

    /*check read write difference...*/
    if ( (current_time.tm_year != tm.tm_year) ||
            (current_time.tm_mon != tm.tm_mon) ||
            (current_time.tm_day != tm.tm_day) ||
            (current_time.tm_hour != tm.tm_hour) ||
            (current_time.tm_min != tm.tm_min) ||
            (current_time.tm_sec != tm.tm_sec) )
    {
        printf("Set RTC and Get RTC is different \n");
        while (1);
    }

    /*check bit access of scratchpad*/
    for (i = 0; i <= 5; i++)
    {
        sys_set_retention_reg(i, 0x55555555);
    }

    for (i = 0; i <= 5; i++)
    {
        sys_get_retention_reg(i, &check_value);

        if (check_value != 0x55555555)
        {
            printf("error scratchpad test\n");
            while (1);
        }
    }

    for (i = 0; i <= 5; i++)
    {
        sys_set_retention_reg(i, 0xAAAAAAAA);
    }

    for (i = 0; i <= 5; i++)
    {
        sys_get_retention_reg(i, &check_value);

        if (check_value != 0xAAAAAAAA)
        {
            printf("error scratchpad test 2\n");
            while (1);
        }
    }

    /*just set some randome number in sys rention */
    sys_set_retention_reg(0, RANDOM_VALUE0);
    sys_set_retention_reg(1, RANDOM_VALUE1);
    sys_set_retention_reg(2, RANDOM_VALUE2);
    sys_set_retention_reg(3, RANDOM_VALUE3);
    sys_set_retention_reg(4, RANDOM_VALUE4);
    sys_set_retention_reg(5, RANDOM_VALUE5);


    /*test Alarm */

    alarm_tm.tm_year = 20;
    alarm_tm.tm_mon = 03;
    alarm_tm.tm_day = 03;
    alarm_tm.tm_hour = 23;
    alarm_tm.tm_min = 58;
    alarm_tm.tm_sec = 01;

    /*First interrupt in tm_sec = 01*/
    alarm_mode = RTC_MODE_MATCH_SEC_INTERRUPT | RTC_MODE_EN_SEC_INTERRUPT ;

    rtc_set_alarm(&alarm_tm, alarm_mode, rtc_arm_isr);

    while (1)
    {
        __WFI();
    }
}

void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
