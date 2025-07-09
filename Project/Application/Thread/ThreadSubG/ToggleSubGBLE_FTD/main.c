#include "bsp.h"
#include "bsp_button.h"
#include "main.h"
#include "app_uart.h"
#include "rfb.h"
#include "rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "uart_stdio.h"
#include "bin_version.h"

#define RAIDO_MAC_ADDR_FLASH_ID_MODE 0
#define RAIDO_MAC_ADDR_MP_SECTOR_MODE 1
extern void rafael_radio_mac_read_config_set(uint8_t mode);
#if PLAFFORM_CONFIG_ENABLE_SUBG
#define RFB_POWER_STAGLE_INDEX       (30) //7~30
#define RFB_DATA_RATE FSK_300K // Supported Value: [FSK_50K; FSK_100K; FSK_150K; FSK_200K; FSK_300K]
extern void rafael_radio_subg_power_index_set(uint8_t stage_index);
extern void rafael_radio_subg_datarate_set(uint8_t datarate);
extern void rafael_radio_subg_band_set(uint8_t ch_min, uint8_t ch_max, uint8_t band);
#endif

#define RFB_CCA_THRESHOLD 75 // Default: 75 (-75 dBm)
#define RTC_WAKE_UP_INTERVEL 3 //minutes

extern void rafael_radio_cca_threshold_set(uint8_t datarate);

static void gpio6_handler();

static void rtc_arm_isr(uint32_t rtc_status);

//BIN_TYPE_ARR fixed len : 12, If all bytes are set to  0, the OTA update will always trigger a reboot.
#define BIN_TYPE_ARR 't','o','g','g','l','e','f','t','d','b','i','n'
const sys_information_t systeminfo = SYSTEMINFO_INIT(BIN_TYPE_ARR);

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

static void bsp_btn_event_handle(bsp_event_t event)
{
    switch (event)
    {
    case BSP_EVENT_BUTTONS_0:
        app_commission_erase();
        NVIC_SystemReset();
        break;
    case BSP_EVENT_BUTTONS_1:
        //send sensor data
        app_commission_erase();
        NVIC_SystemReset();
        break;
    default:
        break;
    }
}

void bsp_uart0_isr_event_handle(bsp_event_t event)
{
    otSysEventSignalPending();
}

void bsp_uart1_isr_event_handle(bsp_event_t event)
{
    otSysEventSignalPending();
}

int main()
{
    rafael_radio_mac_read_config_set(RAIDO_MAC_ADDR_MP_SECTOR_MODE);
#if PLAFFORM_CONFIG_ENABLE_SUBG
    rafael_radio_subg_datarate_set(RFB_DATA_RATE);
    rafael_radio_subg_band_set(
        OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MIN,
        OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MAX,
        RFB_SUBG_FREQUENCY_BAND);
    rafael_radio_subg_power_index_set(RFB_POWER_STAGLE_INDEX);
#endif //PLAFFORM_CONFIG_ENABLE_SUBG
    rafael_radio_cca_threshold_set(RFB_CCA_THRESHOLD);

    /* pinmux init */
    pin_mux_init();

    /* led init */
    gpio_cfg_output(20);
    gpio_cfg_output(21);
    gpio_cfg_output(22);
    gpio_pin_write(20, 1);
    gpio_pin_write(21, 1);
    gpio_pin_write(22, 1);

    /*uart 0 init*/
    uart_stdio_init(otSysEventSignalPending);
    utility_register_stdout(uart_stdio_write_ch, uart_stdio_write);
    util_log_init();

    info("Rafale Toggle SubG over Thread FTD and ble \r\n");
    info("=================================\r\n");
    info("bin version         : ");
    for (uint8_t i = 0; i < PREFIX_LEN; i++)
    {
        info("%c", systeminfo.prefix[i]);
    }
    info(" ");
    for (uint8_t i = 0; i < FW_INFO_LEN; i++)
    {
        info("%02x", systeminfo.sysinfo[i]);
    }
    info("\r\n");

    bsp_init(BSP_INIT_BUTTONS, bsp_btn_event_handle);

    app_commission_t commission;

    app_commission_get(&commission);

    if (commission.started == 0xFF)
    {
        ble_app_init();
    }
    else if (commission.started == 0x01)
    {
        if (app_commission_data_check() == true)
        {
            thread_app_task_start();
        }
        else
        {
            app_commission_erase();
            NVIC_SystemReset();
        }
    }
    else
    {
        app_commission_erase();
        NVIC_SystemReset();
    }
    uart1_task_start();
    vTaskStartScheduler();

    while (1)
    {
    }
    return 0;
}
