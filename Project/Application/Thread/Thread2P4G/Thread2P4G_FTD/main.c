#include "project_config.h"
#include "rfb.h"
#include "bsp.h"
#include "main.h"
#include "sw_timer.h"
#include "uart_stdio.h"
#include "bin_version.h"

#define RAIDO_MAC_ADDR_MP_SECTOR_ENABLE 1
extern void rafael_radio_mac_read_config_set(uint8_t mode);

#define RFB_CCA_THRESHOLD 75 // Default: 75 (-75 dBm)
extern void rafael_radio_cca_threshold_set(uint8_t datarate);

//BIN_TYPE_ARR fixed len : 12, If all bytes are set to  0, the OTA update will always trigger a reboot.
#define BIN_TYPE_ARR 't','h','r','e','a','d','f','t','d','b','i','n'
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
    rafael_radio_mac_read_config_set(RAIDO_MAC_ADDR_MP_SECTOR_ENABLE);
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
    uart_stdio_init(NULL);
    utility_register_stdout(uart_stdio_write_ch, uart_stdio_write);
    util_log_init();

    info("Rafale 2.4G Thread FTD \r\n");
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

    //not use freertos
    sw_timer_init();

    /*bin download will use uart 1 */
    app_uart_init();

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
