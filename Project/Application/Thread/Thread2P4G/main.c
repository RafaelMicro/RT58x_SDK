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
#include "cm3_mcu.h"
#include "bsp.h"
#include "app.h"
#include "mem_mgmt.h"
#include "uart_stdio.h"
/* Utility Library APIs */
#include "util_log.h"
#include "util_printf.h"

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#define RFB_DATA_RATE FSK_300K // Supported Value: [FSK_50K; FSK_100K; FSK_150K; FSK_200K; FSK_300K]
extern void rafael_radio_subg_datarate_set(uint8_t datarate);
#endif

#define RFB_CCA_THRESHOLD 75 // Default: 75 (-75 dBm)
extern void rafael_radio_cca_threshold_set(uint8_t datarate);

otInstance *otGetInstance(void);

void _Sleep_Init()
{
    otError error;

    otLinkModeConfig config;

    config.mRxOnWhenIdle = true;
    config.mNetworkData = true;
    config.mDeviceType = true;

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
#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    rafael_radio_subg_datarate_set(RFB_DATA_RATE);
#endif
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

    info("2P4G Thread Init ability FTD \n");

    _app_init();

    while (!otSysPseudoResetWasRequested())
    {
        _app_process_action();
    }

    _app_exit();

    return 0;
}
