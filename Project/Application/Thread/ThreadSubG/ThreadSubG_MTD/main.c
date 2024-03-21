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

#if PLAFFORM_CONFIG_ENABLE_SUBG
#define APP_RFB_FIX_TX_POWER_SUPPORT 0
#define RFB_DATA_RATE FSK_300K // Supported Value: [FSK_50K; FSK_100K; FSK_150K; FSK_200K; FSK_300K]
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

uint16_t calculateCRC(uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFF;  // Initial CRC value
    // Iterate over each byte in the data
    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)data[i];  // XOR with current data byte
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;  // XOR with CRC16 polynomial
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static void thread_app_sensor_data_piece(uint8_t *payload, uint16_t *payloadlength, void *data)
{
    app_sensor_data_t *sensor_data = (app_sensor_data_t *)data;
    uint8_t *tmp = payload, *tmp_command = NULL;
    mem_memcpy(tmp, &sensor_data->Preamble, 4);
    tmp += 4;
    *tmp++ = sensor_data->SyncWord;
    mem_memcpy(tmp, &sensor_data->NetWorkId, 6);
    tmp += 6;
    mem_memcpy(tmp, &sensor_data->NodeId, 6);
    tmp += 6;
    *tmp++ = sensor_data->FrameSN;
    *tmp++ = sensor_data->FrameControl;
    *tmp++ = sensor_data->FrameLengths;
    tmp_command = tmp;
    *tmp++ = sensor_data->FramePayLoad.CommandLengths;
    *tmp++ = sensor_data->FramePayLoad.CommandClass;
    *tmp++ = sensor_data->FramePayLoad.Command;
    mem_memcpy(tmp, &sensor_data->FramePayLoad.CommandData, sensor_data->FramePayLoad.CommandLengths);
    tmp += sensor_data->FramePayLoad.CommandLengths;
    sensor_data->FramePayLoad.CommandCRC = (calculateCRC(tmp_command, (sensor_data->FrameLengths - 1)) & 0xff);
    *tmp++ = sensor_data->FramePayLoad.CommandCRC;
    sensor_data->FrameCRC = calculateCRC(payload, (tmp - payload));
    mem_memcpy(tmp, &sensor_data->FrameCRC, 2);
    tmp += 2;
    *payloadlength = (tmp - payload);
}

static int thread_app_sensor_data_parse(uint8_t *payload, uint16_t payloadlength, void *data)
{
    app_sensor_data_t *sensor_data = (app_sensor_data_t *)data;
    uint8_t *tmp = payload, *tmp_command = NULL;;
    uint16_t crc = 0;
    mem_memcpy(&sensor_data->Preamble, tmp, 4);
    tmp += 4;
    sensor_data->SyncWord = *tmp++;
    mem_memcpy(&sensor_data->NetWorkId, tmp, 6);
    tmp += 6;
    mem_memcpy(&sensor_data->NodeId, tmp, 6);
    tmp += 6;
    sensor_data->FrameSN = *tmp++;
    sensor_data->FrameControl = *tmp++;
    sensor_data->FrameLengths = *tmp++;
    tmp_command = tmp;
    sensor_data->FramePayLoad.CommandLengths = *tmp++;
    sensor_data->FramePayLoad.CommandClass = *tmp++;
    sensor_data->FramePayLoad.Command = *tmp++;
    memset(&sensor_data->FramePayLoad.CommandData, 0x00, SENSOR_COMMAND_LENS_MAX);
    mem_memcpy(&sensor_data->FramePayLoad.CommandData, tmp, sensor_data->FramePayLoad.CommandLengths);
    tmp += sensor_data->FramePayLoad.CommandLengths;
    sensor_data->FramePayLoad.CommandCRC = *tmp++;
    crc = calculateCRC(tmp_command, (sensor_data->FrameLengths - 1) );
    if (sensor_data->FramePayLoad.CommandCRC != (crc & 0xff))
    {
        info("CommandCRC fail %04X %04X \n", sensor_data->FramePayLoad.CommandCRC, (crc & 0xff));
        return 1;
    }
    mem_memcpy(&sensor_data->FrameCRC, tmp, 2);
    tmp += 2;
    crc = calculateCRC(payload, (tmp - payload - 2));
    if (sensor_data->FrameCRC != crc)
    {
        info("FrameCRC fail %04X %04X \n", sensor_data->FrameCRC, crc);
        return 1;
    }
    if ((tmp - payload) != payloadlength)
    {
        info("parse fail (%u/%u)\n", (tmp - payload), payloadlength);
        return 1;
    }
    return 0;
}

int thread_app_sensor_data_received(uint8_t *data, uint16_t lens, uint8_t *data_seq, uint8_t *cmd)
{
    static app_sensor_data_t s_data;
    uint16_t i = 0;
    int ret = 1;
    do
    {
        if (thread_app_sensor_data_parse(data, lens, &s_data))
        {
            info("isn't sensor data \n");
            break;
        }

        info("==========================================\n");
        info("Preamble : %08X \n", s_data.Preamble);
        info("SyncWord : %02X \n", s_data.SyncWord);
        info("NetWorkId: ");
        for (i = 0; i < 6; i++)
        {
            info("%02X", s_data.NetWorkId[i]);
        }
        info("\n");
        info("NodeId: ");
        for (i = 0; i < 6; i++)
        {
            info("%02X", s_data.NodeId[i]);
        }
        info("\n");
        info("FrameSN: %u \n", s_data.FrameSN);
        *data_seq = s_data.FrameSN;
        info("FrameControl: %u \n", s_data.FrameControl);
        info("FrameLengths: %u \n", s_data.FrameLengths);
        info("FramePayLoad: \n");
        info("CommandLengths: %u\n", s_data.FramePayLoad.CommandLengths);
        info("CommandClass: %02X\n", s_data.FramePayLoad.CommandClass);
        info("Command: %02X\n", s_data.FramePayLoad.Command);
        *cmd = s_data.FramePayLoad.Command;
        if (s_data.FramePayLoad.Command == 0x0)
        {
            gpio_pin_write(20, 1);
        }
        else if (s_data.FramePayLoad.Command == 0x1)
        {
            gpio_pin_write(20, 0);
        }
        info("CommandData: ");
        for (i = 0; i < s_data.FramePayLoad.CommandLengths; i++)
        {
            info("%02X", s_data.FramePayLoad.CommandData[i]);
        }
        info("\n");
        info("CommandCRC: %02X\n", s_data.FramePayLoad.CommandCRC);
        info("FrameCRC: %04X \n", s_data.FrameCRC);
        info("==========================================\n");
        ret = 0;
    } while (0);
    return ret;
}

void thread_app_sensor_data_send(uint16_t PeerPort, otIp6Address PeerAddr, uint8_t senqueset, uint8_t cmd)
{
    app_sensor_data_t s_data;
    uint8_t *payload = NULL;
    uint16_t payloadlens = NULL;

    do
    {
        s_data.Preamble = 0xAABBCCDD;
        s_data.SyncWord = 0xEE;
        memset(s_data.NetWorkId, 0x11, 6);
        memset(s_data.NodeId, 0x22, 6);
        s_data.FrameSN = senqueset;
        s_data.FrameControl = 0x02;
        if (cmd == 0x02)
        {
            s_data.FrameLengths = 0x8; //4 command(version) + 4 command fix lens
        }
        else
        {
            s_data.FrameLengths = 0x4; //4 command fix lens
        }

        if (s_data.FrameLengths >= 4)
        {
            s_data.FramePayLoad.CommandLengths = s_data.FrameLengths - 4;
        }
        else
        {
            info("FrameLengths error %u \n", s_data.FrameLengths);
            break;
        }
        s_data.FramePayLoad.CommandClass = 0xF0 | cmd;
        s_data.FramePayLoad.Command = 0xF0 | cmd;

        if (cmd == 0x02)
        {
            uint32_t ota_ver = ota_get_image_version();
            mem_memcpy(s_data.FramePayLoad.CommandData, &ota_ver, sizeof(uint32_t));
        }

        payload = mem_malloc(sizeof(app_sensor_data_t));
        if (payload)
        {
            thread_app_sensor_data_piece(payload, &payloadlens, &s_data);
            _Udp_Data_Send(PeerPort, PeerAddr, payload, payloadlens);
        }
    } while (0);

    if (payload)
    {
        mem_free(payload);
    }
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
    if (length > 3 && buf[0] == 0x5a && buf[1] == 0x5a) //for Thread test plan
    {
        info("%d MSG : ", length);
        for (int i = 0; i < length; i++)
        {
            if (buf[i] != 0x5a)
            {
                info("%c", buf[i]);
            }
        }
        info("\n");
    }
    else
    {
        info("%d bytes from \n", length);
        info("ip : %s\n", string);
        info("port : %d \n", aMessageInfo->mSockPort);
        buf = mem_malloc(length);
        if (buf)
        {
            otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, length);

            if (!thread_app_sensor_data_received(buf, length, &data_seq, &cmd))
            {
                thread_app_sensor_data_send(aMessageInfo->mSockPort, aMessageInfo->mPeerAddr, data_seq, cmd);
            }
            else
            {
                info("Message Received : ");
                for (int i = 0; i < length; i++)
                {
                    info("%02x", buf[i]);
                }
                info("\n");
            }

            mem_free(buf);
        }
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
#if PLAFFORM_CONFIG_ENABLE_SUBG
    rafael_radio_subg_datarate_set(RFB_DATA_RATE);
    rafael_radio_subg_band_set(
        OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MIN,
        OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MAX,
        RFB_SUBG_FREQUENCY_BAND);
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
