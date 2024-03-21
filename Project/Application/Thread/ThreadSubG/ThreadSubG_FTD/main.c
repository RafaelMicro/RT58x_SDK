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
#include "ota_handler.h"
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
    uint8_t *tmp = payload;
    mem_memcpy(tmp, &sensor_data->Preamble, 2);
    tmp += 2;
    *tmp++ = sensor_data->FrameLengths;
    *tmp++ = sensor_data->FrameSN;
    *tmp++ = sensor_data->FrameClass;
    mem_memcpy(tmp, &sensor_data->NetWorkId, 8);
    tmp += 8;
    *tmp++ = sensor_data->PID;
    *tmp++ = sensor_data->CID;
    *tmp++ = sensor_data->CommandLengths;
    if (sensor_data->CommandLengths > 0)
    {
        mem_memcpy(tmp, &sensor_data->CommandData, sensor_data->CommandLengths);
        tmp += sensor_data->CommandLengths;
    }
    if (((tmp - payload) - 1) != sensor_data->FrameLengths)
    {
        info("piece lens fail (%u/%u)\r\n", (tmp - payload), sensor_data->FrameLengths);
        return;
    }
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
    mem_memcpy(&sensor_data->Preamble, tmp, 2);
    tmp += 2;
    sensor_data->FrameLengths = *tmp++;
    sensor_data->FrameSN = *tmp++;
    sensor_data->FrameClass = *tmp++;
    mem_memcpy(&sensor_data->NetWorkId, tmp, 8);
    tmp += 8;
    sensor_data->PID = *tmp++;
    sensor_data->CID = *tmp++;
    sensor_data->CommandLengths = *tmp++;
    if (sensor_data->CommandLengths > 0)
    {
        memset(&sensor_data->CommandData, 0x00, SENSOR_COMMAND_LENS_MAX);
        mem_memcpy(&sensor_data->CommandData, tmp, sensor_data->CommandLengths);
        tmp += sensor_data->CommandLengths;
    }
    mem_memcpy(&sensor_data->FrameCRC, tmp, 2);
    tmp += 2;
    crc = calculateCRC(payload, (tmp - payload - 2));
    if (sensor_data->FrameCRC != crc)
    {
        info("FrameCRC fail %04X %04X \r\n", sensor_data->FrameCRC, crc);
        return 1;
    }

    if ((tmp - payload) != payloadlength)
    {
        info("parse fail (%u/%u)\r\n", (tmp - payload), payloadlength);
        return 1;
    }
    return 0;
}

int thread_app_sensor_data_received(uint8_t *data, uint16_t lens, uint8_t *data_seq, uint8_t *cmd, uint8_t *pid)
{
    static app_sensor_data_t s_data;
    uint16_t i = 0;
    int ret = 1;
    do
    {
        if (thread_app_sensor_data_parse(data, lens, &s_data))
        {
            info("isn't sensor data \r\n");
            break;
        }
        info("==========================================\r\n");
        info("Preamble       : %04X \r\n", s_data.Preamble);
        info("FrameLengths   : %u \r\n", s_data.FrameLengths);
        info("FrameSN        : %u \r\n", s_data.FrameSN);
        *data_seq = s_data.FrameSN;
        info("FrameClass     : %u \r\n", s_data.FrameClass);
        info("NetWorkId      : ");
        for (i = 0; i < 8; i++)
        {
            info("%02X", s_data.NetWorkId[i]);
        }
        info("\r\n");
        info("PID            : %u \r\n", s_data.PID);
        *pid = s_data.PID;
        info("CID            : %u \r\n", s_data.CID);
        *cmd = s_data.CID;
        if (s_data.CID == 0x0)
        {
            gpio_pin_write(20, 1);
        }
        else if (s_data.CID == 0x1)
        {
            gpio_pin_write(20, 0);
        }
        info("CommandLengths : %u \r\n", s_data.CommandLengths);
        info("CommandData    : ");
        for (i = 0; i < s_data.CommandLengths; i++)
        {
            info("%02X", s_data.CommandData[i]);
        }
        info("\r\n");
        info("FrameCRC: %04X \r\n", s_data.FrameCRC);
        info("==========================================\r\n");
        ret = 0;
    } while (0);
    return ret;
}

void thread_app_sensor_data_send(uint16_t PeerPort, otIp6Address PeerAddr, uint8_t senqueset, uint8_t cmd, uint8_t pid)
{
    app_sensor_data_t s_data;
    uint8_t *payload = NULL;
    uint16_t payloadlens = NULL;
    uint32_t sensor_send_sn;
    const otExtAddress *extAddress;
    do
    {
        s_data.Preamble = 0xFAFB;

        sys_get_retention_reg(1, &sensor_send_sn);
        s_data.FrameSN = (uint8_t)sensor_send_sn++;
        sys_set_retention_reg(1, sensor_send_sn);

        s_data.FrameClass = 0x10; //write from leader to child

        extAddress = otLinkGetExtendedAddress(otGetInstance());
        mem_memcpy(s_data.NetWorkId, extAddress->m8, 8);

        s_data.PID = pid; //sensor type
        s_data.CID = cmd;
        if (s_data.CID == 0x02)
        {
            s_data.CommandLengths = 0x4; //4 command(version)
            uint32_t ota_ver = ota_get_image_version();
            mem_memcpy(s_data.CommandData, &ota_ver, sizeof(uint32_t));
        }

        s_data.FrameLengths = 15 + s_data.CommandLengths;

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
    uint8_t data_seq = 0, cmd = 0xFF, pid = 0xFF;
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

            if (!thread_app_sensor_data_received(buf, length, &data_seq, &cmd, &pid))
            {
                thread_app_sensor_data_send(aMessageInfo->mSockPort, aMessageInfo->mPeerAddr, data_seq, cmd, pid);
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

    info("SubG Thread Init ability FTD \n");

    _app_init();

    while (!otSysPseudoResetWasRequested())
    {
        _app_process_action();
    }

    _app_exit();

    return 0;
}
