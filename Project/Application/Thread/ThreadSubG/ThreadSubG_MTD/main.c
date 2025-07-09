#include "main.h"
#include "rfb.h"
#include "rfb_port.h"
#include "bsp.h"
#include "ota_handler.h"
#include "uart_stdio.h"
#include "sw_timer.h"
#include "bin_version.h"

#define RAIDO_MAC_ADDR_FLASH_ID_MODE 0
#define RAIDO_MAC_ADDR_MP_SECTOR_MODE 1
extern void rafael_radio_mac_read_config_set(uint8_t mode);
#if PLAFFORM_CONFIG_ENABLE_SUBG
#define APP_RFB_FIX_TX_POWER_SUPPORT  0
#define RFB_POWER_STAGLE_INDEX       (30) //7~30
#define RFB_DATA_RATE FSK_300K // Supported Value: [FSK_50K; FSK_100K; FSK_150K; FSK_200K; FSK_300K]
extern void rafael_radio_subg_power_index_set(uint8_t stage_index);
extern void rafael_radio_subg_datarate_set(uint8_t datarate);
extern void rafael_radio_subg_band_set(uint8_t ch_min, uint8_t ch_max, uint8_t band);
#endif

#define RFB_CCA_THRESHOLD 75 // Default: 75 (-75 dBm)
extern void rafael_radio_cca_threshold_set(uint8_t datarate);

//BIN_TYPE_ARR fixed len : 12, If all bytes are set to  0, the OTA update will always trigger a reboot.
#define BIN_TYPE_ARR 't','h','r','e','a','d','m','t','d','b','i','n'
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

int main(int argc, char *argv[])
{
    rafael_radio_mac_read_config_set(RAIDO_MAC_ADDR_MP_SECTOR_MODE);
#if PLAFFORM_CONFIG_ENABLE_SUBG
    rafael_radio_subg_datarate_set(RFB_DATA_RATE);
    rafael_radio_subg_band_set(
        OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MIN,
        OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MAX,
        RFB_SUBG_FREQUENCY_BAND);
    rafael_radio_subg_power_index_set(RFB_POWER_STAGLE_INDEX);
#if APP_RFB_FIX_TX_POWER_SUPPORT
    if (RFB_SUBG_FREQUENCY_BAND == 3)
    {
        rfb_port_fix_15dbm_tx_power_set(1, BAND_SUBG_433M);
    }
#endif
#endif //PLAFFORM_CONFIG_ENABLE_SUBG
    rafael_radio_cca_threshold_set(RFB_CCA_THRESHOLD);

    /* pinmux init */
    pin_mux_init();

    /* led init */
    gpio_cfg_output(20);
    gpio_cfg_output(21);
    gpio_cfg_output(22);
    gpio_cfg_output(23);
    gpio_pin_write(20, 1);
    gpio_pin_write(21, 1);
    gpio_pin_write(22, 1);

    /*uart 0 init*/
    uart_stdio_init(NULL);
    utility_register_stdout(uart_stdio_write_ch, uart_stdio_write);
    util_log_init();

    info("Rafale SubG over Thread MTD \r\n");
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
    otSysInit(argc, argv);
    app_task_init();

    /*bin download will use uart 1 */
    app_uart_init();

    //not use freertos
    sw_timer_init();
    while (!otSysPseudoResetWasRequested())
    {
        /*sw timer use*/
        sw_timer_proc();
        /*bin download will use uart 1 */
        app_uart_recv();
        /*openthread use*/
        app_task_process_action();
    }

    app_task_exit();

    return 0;
}
