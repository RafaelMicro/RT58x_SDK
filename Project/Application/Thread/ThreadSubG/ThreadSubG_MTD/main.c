#include "openthread-core-RT58x-config.h"
#include <openthread-core-config.h>
#include <openthread/config.h>

#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/platform/logging.h>
#include <openthread/thread.h>
#include <openthread/udp.h>
#include <openthread/logging.h>

#include "openthread-system.h"
#include "cli/cli_config.h"
#include "common/code_utils.hpp"

#include "project_config.h"
#include "rfb.h"
#include "rfb_port.h"
#include "cm3_mcu.h"
#include "bsp.h"
#include "app.h"
#include "mem_mgmt.h"
#include "ota_handler.h"
#include "uart_stdio.h"
/* Utility Library APIs */
#include "util_log.h"
#include "util_printf.h"

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

otInstance *otGetInstance(void);

void _Sleep_Init()
{
    otError error;

    otLinkModeConfig config;

    error = otLinkSetPollPeriod(otGetInstance(), 1000);
    if (error != OT_ERROR_NONE)
    {
        err("otLinkSetPollPeriod failed with %d %s\r\n", error, otThreadErrorToString(error));
    }
    config.mRxOnWhenIdle = false;
    config.mNetworkData = false;
    config.mDeviceType = false;

    error = otThreadSetLinkMode(otGetInstance(), config);

    if (error != OT_ERROR_NONE)
    {
        err("otThreadSetLinkMode failed with %d %s\r\n", error, otThreadErrorToString(error));
    }

    /* low power mode init */
    Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);
    Lpm_Enable_Low_Power_Wakeup((LOW_POWER_WAKEUP_32K_TIMER | LOW_POWER_WAKEUP_UART0_RX));
}

void UdpReceiveCallBack(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aContext);

    uint8_t *buf = NULL;
    uint8_t data_seq = 0, cmd = 0xFF;
    int length;
    char string[OT_IP6_ADDRESS_STRING_SIZE];

    otIp6AddressToString(&aMessageInfo->mPeerAddr, string, sizeof(string));
    length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    info("%d bytes from \n", length);
    info("ip : %s\n", string);
    info("port : %d \n", aMessageInfo->mSockPort);
    buf = mem_malloc(length);
    if (buf)
    {
        otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, length);

        info("Message Received : ");
        for (int i = 0; i < length; i++)
        {
            info("%02x", buf[i]);
        }
        info("\n");

        mem_free(buf);
    }
}

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
    rafael_radio_mac_read_config_set(RAIDO_MAC_ADDR_FLASH_ID_MODE);
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
    gpio_pin_write(20, 1);
    gpio_pin_write(21, 1);
    gpio_pin_write(22, 1);

    /*uart 0 init*/
    uart_stdio_init(NULL);
    utility_register_stdout(uart_stdio_write_ch, uart_stdio_write);
    util_log_init();

    otSysInit(argc, argv);

    info("SubG Thread Init ability MTD \n");

    _app_init();

    while (!otSysPseudoResetWasRequested())
    {
        _app_process_action();
    }

    _app_exit();

    return 0;
}
