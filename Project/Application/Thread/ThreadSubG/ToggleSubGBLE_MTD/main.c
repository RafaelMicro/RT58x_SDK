#include "openthread-core-RT58x-config.h"
#include <openthread-core-config.h>
#include <openthread/config.h>
#include <openthread/thread.h>

#include "openthread-system.h"
#include "cm3_mcu.h"
#include "rfb.h"
#include "bsp.h"
#include "bsp_button.h"
#include "util_log.h"
#include "app.h"
#include "app_uart_handler.h"
#include "ble_app.h"
#include "thread_app.h"
#include "rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "platform-rt58x.h"
#include "uart_stdio.h"
/* Utility Library APIs */
#include "util_log.h"
#include "util_printf.h"

#if PLAFFORM_CONFIG_ENABLE_SUBG
#define RFB_DATA_RATE FSK_300K // Supported Value: [FSK_50K; FSK_100K; FSK_150K; FSK_200K; FSK_300K]
extern void rafael_radio_subg_datarate_set(uint8_t datarate);
extern void rafael_radio_subg_band_set(uint8_t ch_min, uint8_t ch_max, uint8_t band);
#endif

#define RFB_CCA_THRESHOLD 75 // Default: 75 (-75 dBm)
#define RTC_WAKE_UP_INTERVEL 3 //minutes

extern void rafael_radio_cca_threshold_set(uint8_t datarate);

static void gpio6_handler();

static void rtc_arm_isr(uint32_t rtc_status);

/* pin mux setting init*/
static void pin_mux_init(void)
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

/* button callbake function*/
static void bsp_btn_event_handle(bsp_event_t event)
{
    switch (event)
    {
    case BSP_EVENT_BUTTONS_0:
        /*trigger button 0 will erase commission content*/
        app_commission_erase();
        NVIC_SystemReset();
        break;
#if !APP_DOOR_SENSOR_USE
    case BSP_EVENT_BUTTONS_1:
        /*trigger button 1 will send sensor data*/
        g_button1_hold = true;
        if (app_commission_data_check() == true)
        {
            /*send after confirming have connission content*/
            thread_app_sensor_data_generate(APP_SENSOR_CONTROL_EVENT);
        }
        else
        {
            /*not commission*/
        }
        break;
#endif
    default:
        break;
    }
}

static void app_rtc_alarm_set(rtc_time_t current_time)
{
    rtc_time_t alarm_tm;
    uint32_t alarm_mode;
    int carry = 0;

    /*check if RTC interval is within range*/
    if (RTC_WAKE_UP_INTERVEL > 0 && RTC_WAKE_UP_INTERVEL <= 10080)
    {
        alarm_tm.tm_year = current_time.tm_year;
        alarm_tm.tm_mon = current_time.tm_mon;
        alarm_tm.tm_day = current_time.tm_day;
        alarm_tm.tm_hour = current_time.tm_hour;
        alarm_tm.tm_min = (current_time.tm_min + RTC_WAKE_UP_INTERVEL); //RTC wake up intervel
        alarm_tm.tm_sec = current_time.tm_sec;

        /*handle carry of seconds*/
        carry = alarm_tm.tm_sec / 60;
        alarm_tm.tm_sec %= 60;
        alarm_tm.tm_min += carry;

        /*handle carry of minutes*/
        carry = alarm_tm.tm_min / 60;
        alarm_tm.tm_min %= 60;
        alarm_tm.tm_hour += carry;

        /*handle carry of hours*/
        carry = alarm_tm.tm_hour / 24;
        alarm_tm.tm_hour %= 24;
        alarm_tm.tm_day += carry;

        /*handle carry of days*/
        while (alarm_tm.tm_day  > 28)
        {
            if (alarm_tm.tm_mon == 2)
            {
                if ((alarm_tm.tm_year % 4 == 0 && alarm_tm.tm_year % 100 != 0) || (alarm_tm.tm_year % 400 == 0))
                {
                    if (alarm_tm.tm_day > 29)
                    {
                        alarm_tm.tm_day -= 29;
                        ++alarm_tm.tm_mon;
                    }
                }
                else
                {
                    if (alarm_tm.tm_day > 28)
                    {
                        alarm_tm.tm_day -= 28;
                        ++alarm_tm.tm_mon;
                    }
                }
            }
            else if (alarm_tm.tm_mon == 4 || alarm_tm.tm_mon == 6 || alarm_tm.tm_mon == 9 || alarm_tm.tm_mon == 11)
            {
                if (alarm_tm.tm_day > 30)
                {
                    alarm_tm.tm_day -= 30;
                    ++alarm_tm.tm_mon;
                }
            }
            else
            {
                if (alarm_tm.tm_day > 31)
                {
                    alarm_tm.tm_day -= 31;
                    ++alarm_tm.tm_mon;
                }
            }

            if (alarm_tm.tm_mon > 12)
            {
                alarm_tm.tm_mon -= 12;
                ++alarm_tm.tm_year;
            }
        }
        alarm_mode = RTC_MODE_MONTH_EVENT_INTERRUPT | RTC_MODE_EVENT_INTERRUPT ;

        rtc_set_alarm(&alarm_tm, alarm_mode, rtc_arm_isr);
    }
    else
    {
        /*RTC_WAKE_UP_INTERVEL set failed */
    }
}

/* rtc callbake function*/
static void rtc_arm_isr(uint32_t rtc_status)
{
    rtc_time_t  current_time;

    rtc_get_time(&current_time);

    app_rtc_alarm_set(current_time);

    return;
}

#if APP_DOOR_SENSOR_USE
static void gpio6_handler()
{
}
#endif

int main()
{
#if PLAFFORM_CONFIG_ENABLE_SUBG
    rafael_radio_subg_datarate_set(RFB_DATA_RATE);
    rafael_radio_subg_band_set(
        OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MIN,
        OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MAX,
        RFB_SUBG_FREQUENCY_BAND);
#endif //PLAFFORM_CONFIG_ENABLE_SUBG
    rafael_radio_cca_threshold_set(RFB_CCA_THRESHOLD);

    /* pinmux init */
    pin_mux_init();
#if APP_DOOR_SENSOR_USE
    /*for door sensor application*/
    gpio_cfg_input(6, GPIO_PIN_INT_EDGE_FALLING);
    pin_set_pullopt(6, PULL_UP_100K);
    gpio_register_isr(6, gpio6_handler, NULL);
    gpio_debounce_enable(6);
    gpio_int_enable(6);
#endif

    /* buttons initial setting */
    bsp_init(BSP_INIT_BUTTONS, bsp_btn_event_handle);

    /*uart 0 init*/
    uart_stdio_init(otSysEventSignalPending);
    utility_register_stdout(uart_stdio_write_ch, uart_stdio_write);
    util_log_init();

    app_commission_t commission;
    rtc_time_t current_time, alarm_tm;

    /*get RTC current time */
    rtc_get_time(&current_time);

    /* when button0 is triggered in deep sleep */
    if (!bsp_button_state_get(BSP_BUTTON_0))
    {
        /*trigger button 0 will erase commission content*/
        app_commission_erase();
    }

#if !APP_DOOR_SENSOR_USE
    /* when button1 is triggered in deep sleep */
    if (!bsp_button_state_get(BSP_BUTTON_1))
    {
        /*trigger button 1 will send sensor data*/
        g_button1_hold = true;
        if (app_commission_data_check() == true)
        {
            /*send after confirming have connission content*/
            g_sensor_event = APP_SENSOR_CONTROL_EVENT;
        }
        else
        {
            /*BNT 1 not commission*/
        }
    }
#endif

    /* when RTC event is triggered in deep sleep */
    if (RTC->RTC_INT_STATUS & RTC_INT_EVENT_BIT)
    {
        //send sensor data
        if (app_commission_data_check() == true)
        {
            /*send ota ask after confirming have connission content*/
            g_sensor_event = APP_SENSOR_GET_OTA_VERSION_EVENT;
        }
        else
        {
            /*not commission*/
        }
        RTC->RTC_INT_CLEAR = (RTC->RTC_INT_STATUS & ~RTC_INT_MINUTE_BIT);
    }

    /* low power mode init */
    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_GPIO0);
#if APP_DOOR_SENSOR_USE
    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_GPIO6);
#else
    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_GPIO1);
#endif
    Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_RTC_TIMER);

    /* to get commision content */
    app_commission_get(&commission);

    /* check if commission content is erase*/
    if (commission.started == 0xFF)
    {
        /* open ble and wait for commission*/
        ble_app_init();
    }
    else if (commission.started == 0x01)
    {
        /* check if commission done*/
        if (app_commission_data_check() == true)
        {
            /*open thread after confirming have connission content*/
            thread_app_task_start();
            if (g_sensor_event != APP_SENSOR_CONTROL_EVENT)
            {
                /*set rtc intervel*/
                app_rtc_alarm_set(current_time);
            }
        }
        else
        {
            /*not commission*/
            Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP3);
            Lpm_Enter_Low_Power_Mode();
        }
    }
    else
    {
        /* commission content is error */
        Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP3);
        Lpm_Enter_Low_Power_Mode();
    }

    uart1_task_start();
    vTaskStartScheduler();

    while (1)
    {
    }
    return 0;
}
