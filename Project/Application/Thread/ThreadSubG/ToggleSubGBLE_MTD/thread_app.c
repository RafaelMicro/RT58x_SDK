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
#include "common/debug.hpp"

#include "project_config.h"
#include "cm3_mcu.h"
#include "bsp.h"
#include "bsp_button.h"
#include "util_log.h"
#include "rfb.h"
#include "app.h"
#include "thread_app.h"
#include "mem_mgmt.h"
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include "ota_handler.h"
#include "timers.h"
#include "thread_queue.h"
#include "rf_mcu.h"

#ifndef APP_TASK_STACK_SIZE
#define APP_TASK_STACK_SIZE 4096
#endif

uint8_t g_sensor_event = 0xff;

bool g_button1_hold = false;
#if APP_DOOR_SENSOR_USE
uint8_t gpio6_last_state = 0;
#endif
static TaskHandle_t sAppTask = NULL;
static otUdpSocket socket;
static otSockAddr sockaddr;
static otInstance *g_app_instance = NULL;
extern otError ota_init(otInstance *aInstance);
extern void otAppCliInit(otInstance *aInstance);
static thread_queue_t app_sensor_event_queue;

void _Sleep_Init(void);
void UdpReceiveCallBack(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);

static TimerHandle_t Thread_Time_Tick;
static uint16_t sensor_send_tmr = 0;
static uint16_t networking_tmr = 0;
static uint16_t wait_ota_tmr = 0;
static uint16_t g_button1_tmr = 0;

/*timeout */
#define SENSOR_SEND_TIMEOUT 5
#define NETWORKING_TIMEOUT 10
#define WAIT_OTA_TIMEOUT 60

#define SENSOR_SNED_TMR_START() (sensor_send_tmr = SENSOR_SEND_TIMEOUT)
#define NETWORKING_TMR_START() (networking_tmr = NETWORKING_TIMEOUT)
#define WAIT_OTA_TMR_START() (wait_ota_tmr = WAIT_OTA_TIMEOUT)

#define SENSOR_SNED_TMR_STOP() (sensor_send_tmr = 0)
#define NETWORKING_TMR_STOP() (networking_tmr = 0)
#define WAIT_OTA_TMR_STOP() (wait_ota_tmr = 0)

otInstance *otGetInstance(void)
{
    return g_app_instance;
}

otError otParseDigit(char DigitChar, uint8_t *Value)
{
    otError error = OT_ERROR_NONE;

    do
    {
        if (('0' > DigitChar) && (DigitChar > '9'))
        {
            error = OT_ERROR_INVALID_ARGS;
            break;
        }
        *Value = (uint8_t)(DigitChar - '0');
    } while (0);
exit:
    return error;
}

otError otParseHexDigit(char HexChar, uint8_t *Value)
{
    otError error = OT_ERROR_NONE;
    do
    {
        if (('A' <= HexChar) && (HexChar <= 'F'))
        {
            *Value = (uint8_t)(HexChar - 'A' + 10);
            break;
        }

        if (('a' <= HexChar) && (HexChar <= 'f'))
        {
            *Value = (uint8_t)(HexChar - 'a' + 10);
            break;
        }
        error = otParseDigit(HexChar, Value);
    } while (0);
exit:
    return error;
}

otError otParseAsUint64(const char *String, uint64_t *Uint64)
{
    otError error = OT_ERROR_NONE;
    uint64_t value = 0;
    const char *cur = String;
    bool isHex = false;

    uint64_t MaxHexBeforeOveflow = (0xffffffffffffffffULL / 16);
    uint64_t MaxDecBeforeOverlow = (0xffffffffffffffffULL / 10);

    do
    {
        if (NULL == String)
        {
            error = OT_ERROR_INVALID_ARGS;
            break;
        }

        if (cur[0] == '0' && (cur[1] == 'x' || cur[1] == 'X'))
        {
            cur += 2;
            isHex = true;
        }
        do
        {
            uint8_t digit;
            uint64_t newValue;
            error = isHex ? otParseHexDigit(*cur, &digit) : otParseDigit(*cur, &digit);
            if (OT_ERROR_NONE != error)
            {
                break;
            }
            if (value > (isHex ? MaxHexBeforeOveflow : MaxDecBeforeOverlow))
            {
                error = OT_ERROR_INVALID_ARGS;
                break;
            }
            value = isHex ? (value << 4) : (value * 10);
            newValue = value + digit;
            if (newValue < value)
            {
                error = OT_ERROR_INVALID_ARGS;
                break;
            }
            value = newValue;
            cur++;
        } while (*cur != '\0');
    } while (0);

    *Uint64 = value;
exit:
    return error;
}

otError otParseHexString(const char *aString, uint8_t *aBuffer, uint16_t aSize)
{
    otError error = OT_ERROR_NONE;
    size_t parsedSize = 0;
    size_t stringLength;
    size_t expectedSize;
    bool skipFirstDigit;

    do
    {
        if (aString == NULL)
        {
            error = OT_ERROR_INVALID_ARGS;
            break;
        }

        stringLength = strlen(aString);
        expectedSize = (stringLength + 1) / 2;

        if (expectedSize != aSize)
        {
            error = OT_ERROR_INVALID_ARGS;
            break;
        }
        // If number of chars in hex string is odd, we skip parsing
        // the first digit.

        skipFirstDigit = ((stringLength & 1) != 0);

        while (parsedSize < expectedSize)
        {
            uint8_t digit;

            if (parsedSize == aSize)
            {
                // If partial parse mode is allowed, stop once we read the
                // requested size.
                error = OT_ERROR_PENDING;
                break;
            }

            if (skipFirstDigit)
            {
                *aBuffer = 0;
                skipFirstDigit = false;
            }
            else
            {
                error = otParseHexDigit(*aString, &digit);
                if (error != OT_ERROR_NONE)
                {
                    break;
                }
                aString++;
                *aBuffer = (uint8_t)(digit << 4);
            }

            error = otParseHexDigit(*aString, &digit);
            if (error != OT_ERROR_NONE)
            {
                break;
            }
            aString++;
            *aBuffer |= digit;
            aBuffer++;
            parsedSize++;
        }

        aSize = (uint16_t)(parsedSize);
    } while (0);

exit:
    return error;
}

static void thread_go_deep_sleep()
{
    if (ota_get_state() == OTA_IDLE && wait_ota_tmr == 0)
    {
#if APP_DOOR_SENSOR_USE
        Lpm_Set_GPIO_Deepsleep_Wakeup_Invert((gpio_pin_get(6) == 0 ? 0x40 : 0x00));
        if (gpio_pin_get(6) == 0)
        {
            pin_set_pullopt(6, PULL_DOWN_100K);
        }
#else
        if (bsp_button_state_get(BSP_BUTTON_1))
#endif
        {
            RfMcu_InterruptDisableAll();
            Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP3);
            Lpm_Enter_Low_Power_Mode();
        }
    }
    else
    {
        WAIT_OTA_TMR_START();
    }
}

static void _Set_Network_Configuration()
{
    static char aNetworkName[] = "Thread_RT58X";

    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x00, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00};

#if 0 // for certification
    uint8_t nwkkey[OT_NETWORK_KEY_SIZE] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
                                          };
#else
    uint8_t nwkkey[OT_NETWORK_KEY_SIZE] = {0xfe, 0xaa, 0x44, 0x8a, 0x67, 0x29, 0xfe, 0xab,
                                           0xab, 0xfe, 0x29, 0x67, 0x8a, 0x44, 0x83, 0xfe
                                          };
#endif

    uint8_t meshLocalPrefix[OT_MESH_LOCAL_PREFIX_SIZE] = {0xfd, 0x00, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00};
    uint8_t aPSKc[OT_PSKC_MAX_SIZE] = {0x74, 0x68, 0x72, 0x65,
                                       0x61, 0x64, 0x6a, 0x70,
                                       0x61, 0x6b, 0x65, 0x74,
                                       0x65, 0x73, 0x74, 0x00
                                      };

    otError error;
    otOperationalDataset aDataset;
    app_commission_t app_commission;

    app_commission_get(&app_commission);

    memset(&aDataset, 0, sizeof(otOperationalDataset));

#if 0//OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    /* Set BR mac filter */
    if (app_commission_data_check() == true)
    {
        otExtAddress br_extAddress;
        otLinkFilterSetAddressMode(g_app_instance, OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST);
        memcpy(br_extAddress.m8, app_commission.data.br_mac, OT_EXT_ADDRESS_SIZE);
        error = otLinkFilterAddAddress(g_app_instance, &br_extAddress);
    }
#endif

    aDataset.mActiveTimestamp.mSeconds = 1;
    aDataset.mComponents.mIsActiveTimestampPresent = true;

    /* Set Channel */
    if (app_commission_data_check() == true)
    {
        aDataset.mChannel = app_commission.data.br_channel;
    }
    else
    {
        aDataset.mChannel = DEF_CHANNEL;
    }
    aDataset.mComponents.mIsChannelPresent = true;

    /* Set Pan ID */
    if (app_commission_data_check() == true)
    {
        aDataset.mPanId = ((app_commission.data.br_panid & 0xff) << 8) |
                          ((app_commission.data.br_panid & 0xff00) >> 8);
    }
    else
    {
        aDataset.mPanId = (otPanId)0xbee0;
    }
    aDataset.mComponents.mIsPanIdPresent = true;

    /* Set Extended Pan ID */
    memcpy(aDataset.mExtendedPanId.m8, extPanId, OT_EXT_PAN_ID_SIZE);
    aDataset.mComponents.mIsExtendedPanIdPresent = true;

    /* Set network key */
    if (app_commission_data_check() == true)
    {
        memcpy(aDataset.mNetworkKey.m8, app_commission.data.br_networkkey, OT_NETWORK_KEY_SIZE);
    }
    else
    {
        memcpy(aDataset.mNetworkKey.m8, nwkkey, OT_NETWORK_KEY_SIZE);
    }
    aDataset.mComponents.mIsNetworkKeyPresent = true;

    /* Set Network Name */
    size_t length = strlen(aNetworkName);
    memcpy(aDataset.mNetworkName.m8, aNetworkName, length);
    aDataset.mComponents.mIsNetworkNamePresent = true;

    memcpy(aDataset.mMeshLocalPrefix.m8, meshLocalPrefix, OT_MESH_LOCAL_PREFIX_SIZE);
    aDataset.mComponents.mIsMeshLocalPrefixPresent = true;

    /* Set the Active Operational Dataset to this dataset */
    error = otDatasetSetActive(g_app_instance, &aDataset);
    if (error != OT_ERROR_NONE)
    {
        otCliOutputFormat("otDatasetSetActive failed with %d %s\r\n", error, otThreadErrorToString(error));
    }

    /* set extaddr to equal eui64*/
    otExtAddress extAddress;
    otLinkGetFactoryAssignedIeeeEui64(g_app_instance, &extAddress);
    error = otLinkSetExtendedAddress(g_app_instance, &extAddress);
    if (error != OT_ERROR_NONE)
    {
        otCliOutputFormat("set extaddr fail\r\n");
    }

    /* set mle eid to equal eui64*/
    otIp6InterfaceIdentifier iid;
    memcpy(iid.mFields.m8, extAddress.m8, OT_EXT_ADDRESS_SIZE);
    error = otIp6SetMeshLocalIid(g_app_instance, &iid);
    if (error != OT_ERROR_NONE)
    {
        otCliOutputFormat("set mle eid fail\r\n");
    }
}

void _Udp_Data_Send(uint16_t PeerPort, otIp6Address PeerAddr, uint8_t *data, uint16_t buffer_lens)
{
    otError error = OT_ERROR_NONE;
    otMessage *message = NULL;
    otMessageInfo messageInfo;
    otMessageSettings messageSettings = {true, OT_MESSAGE_PRIORITY_NORMAL};

    memset(&messageInfo, 0, sizeof(messageInfo));

    do
    {
        messageInfo.mPeerPort = PeerPort;
        messageInfo.mPeerAddr = PeerAddr;

        message = otUdpNewMessage(g_app_instance, &messageSettings);
        if (message == NULL)
        {
            error = OT_ERROR_NO_BUFS;
            break;
        }
        error = otMessageAppend(message, data, buffer_lens);
        if (error != OT_ERROR_NONE)
        {
            break;
        }

        error = otUdpSend(g_app_instance, &socket, message, &messageInfo);
        if (error != OT_ERROR_NONE)
        {
            break;
        }

        message = NULL;

    } while (0);

    if (message != NULL)
    {
        otMessageFree(message);
    }
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

static void thread_app_sensor_data_queue_push(uint8_t event, uint8_t *data, uint16_t data_lens)
{
    app_sensor_event_queue_t event_data;
    enter_critical_section();
    event_data.event = event;
    mem_memcpy(&event_data.data, data, data_lens);
    event_data.data_lens = data_lens;
    if (thread_enqueue(&app_sensor_event_queue, &event_data))
    {
        info("thread_app enqueue fail \r\n");
    }
    leave_critical_section();
    otSysEventSignalPending();
}

static void thread_app_sensor_data_send(uint8_t *data, uint16_t data_lens)
{
    otIp6Address LeaderIp = *otThreadGetRloc(otGetInstance());
    LeaderIp.mFields.m8[14] = 0xfc;
    LeaderIp.mFields.m8[15] = 0x00;

    if (sensor_send_tmr == 0)
    {
        _Udp_Data_Send(UDP_PORT, LeaderIp, data, data_lens);
        SENSOR_SNED_TMR_START();
    }
}

/* generate the srnsor data */
void thread_app_sensor_data_generate(uint8_t event)
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

        s_data.FrameClass = 0x1; //trigger from child

        extAddress = otLinkGetExtendedAddress(otGetInstance());
        mem_memcpy(s_data.NetWorkId, extAddress->m8, 8);

#if APP_DOOR_SENSOR_USE
        s_data.PID = 0x0; //sensor type
#else
        s_data.PID = 0x01; //sensor type
#endif
        s_data.CommandLengths = 0;
        if (event == APP_SENSOR_CONTROL_EVENT)
        {
#if APP_DOOR_SENSOR_USE
            gpio6_last_state = gpio_pin_get(6);
            s_data.CID = gpio6_last_state;
#else
            s_data.CID = ((sensor_send_sn % 2) == 0) ? 0x0 : 0x1;
#endif
        }
        else if (event == APP_SENSOR_GET_OTA_VERSION_EVENT)
        {
            s_data.CID = 0x2;
        }
        else if (event == APP_SENSOR_WAIT_OTA_EVENT)
        {
            s_data.CID = 0x3;
        }
        if (s_data.CommandLengths > 0 )
        {
            memset(s_data.CommandData, 0x00, SENSOR_COMMAND_LENS_MAX);
            memset(s_data.CommandData, 0xff, s_data.CommandLengths);
        }

        s_data.FrameLengths = 15 + s_data.CommandLengths;

        payload = mem_malloc(sizeof(app_sensor_data_t));
        if (payload)
        {
            thread_app_sensor_data_piece(payload, &payloadlens, &s_data);
            thread_app_sensor_data_queue_push(event, payload, payloadlens);
        }
    } while (0);

    if (payload)
    {
        mem_free(payload);
    }
}

int thread_app_sensor_data_received(uint8_t *data, uint16_t lens)
{
    app_sensor_data_t s_data;
    uint16_t i = 0;
    int ret = 1;
    do
    {
        if (thread_app_sensor_data_parse(data, lens, &s_data))
        {
            info("isn't sensor data\r\n");
            break;
        }
        SENSOR_SNED_TMR_STOP();
        info("==========================================\r\n");
        info("Preamble       : %04X \r\n", s_data.Preamble);
        info("FrameLengths   : %u \r\n", s_data.FrameLengths);
        info("FrameSN        : %u \r\n", s_data.FrameSN);
        info("FrameClass     : %u \r\n", s_data.FrameClass);
        info("NetWorkId      : ");
        for (i = 0; i < 8; i++)
        {
            info("%02X", s_data.NetWorkId[i]);
        }
        info("\r\n");
        info("PID            : %u \r\n", s_data.PID);
        info("CID            : %u \r\n", s_data.CID);
        info("CommandLengths : %u \r\n", s_data.CommandLengths);
        info("CommandData    : ");
        for (i = 0; i < s_data.CommandLengths; i++)
        {
            info("%02X", s_data.CommandData[i]);
        }
        info("\r\n");
        info("FrameCRC: %04X \r\n", s_data.FrameCRC);
        info("==========================================\r\n");
        if (s_data.FrameClass == 0x10) //ack
        {
            if (s_data.CID == 0x0 || s_data.CID == 0x1)
            {
#if APP_DOOR_SENSOR_USE
                if (gpio_pin_get(6) != gpio6_last_state)
                {
                    thread_app_sensor_data_generate(APP_SENSOR_CONTROL_EVENT);
                }
                else
#endif
                {
                    thread_go_deep_sleep();
                }
            }
            else if (s_data.CID == 0x02)
            {
                uint32_t ota_ver = ota_get_image_version();
                uint32_t rec_ota_ver = 0;
                memcpy(&rec_ota_ver, &s_data.CommandData, sizeof(uint32_t));
                if (rec_ota_ver == ota_ver)
                {
                    info("is same ota version\r\n");
                    thread_go_deep_sleep();
                }
                else if (rec_ota_ver == 0)
                {
                    info("ota version is null\r\n");
                    thread_go_deep_sleep();
                }
                else
                {
                    info("is different ota version %04X %04X\r\n", ota_ver, (uint32_t)&s_data.CommandData);
                    thread_app_sensor_data_generate(APP_SENSOR_WAIT_OTA_EVENT);
                }
            }
            else if (s_data.CID == 0x03)
            {
                WAIT_OTA_TMR_START();
                info("wait new ota\r\n");
                otError error = otLinkSetPollPeriod(otGetInstance(), 200);
                if (error != OT_ERROR_NONE)
                {
                    err("otLinkSetPollPeriod failed with %d %s\r\n", error, otThreadErrorToString(error));
                }
            }
        }
        ret = 0;
    } while (0);
    return ret;
}

/*IF Thread Netwoking attach attempt big than 1, go to sleep*/
static void thread_app_networking_timeout_handler()
{
    const otMleCounters *aMleCounters;
    aMleCounters = otThreadGetMleCounters(otGetInstance());
    if (aMleCounters->mAttachAttempts > 1)
    {
        thread_go_deep_sleep();
    }
    else
    {
        NETWORKING_TMR_START();
    }
}

static void thread_app_sensor_data_timeout_handler()
{
    thread_go_deep_sleep();
}

static void thread_app_wait_ota_timeout_handler()
{
    if (ota_get_state() == OTA_IDLE)
    {
        WAIT_OTA_TMR_STOP();
        info("not received new ota\r\n");
        thread_go_deep_sleep();
    }
    else
    {
        WAIT_OTA_TMR_START();
    }
}

static void thread_app_time_tick_handler()
{
    if (0 < networking_tmr)
    {
        networking_tmr % 2 == 0 ? gpio_pin_write(21, 0) : gpio_pin_write(21, 1);
        if (0 == --networking_tmr)
        {
            thread_app_networking_timeout_handler();
        }
    }

    if (0 < sensor_send_tmr && 0 == --sensor_send_tmr)
    {
        thread_app_sensor_data_timeout_handler();
    }

    if (0 < wait_ota_tmr)
    {
        wait_ota_tmr % 2 == 0 ? gpio_pin_write(22, 0) : gpio_pin_write(22, 1);
        if (0 == --wait_ota_tmr)
        {
            thread_app_wait_ota_timeout_handler();
        }
    }
#if !APP_DOOR_SENSOR_USE
    if (true == g_button1_hold)
    {
        g_button1_tmr = 1;
    }
    if (g_button1_tmr)
    {
        if (!bsp_button_state_get(BSP_BUTTON_1))
        {
            ++g_button1_tmr;
        }
        else
        {
            info("button hold for %u seconds \r\n", g_button1_tmr);
            g_button1_tmr = 0;
            if (0 == networking_tmr)
            {
                thread_go_deep_sleep();
            }
        }
        g_button1_hold = false;
    }
#endif
    xTimerStart(Thread_Time_Tick, 0);
}

void UdpReceiveCallBack(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aContext);

    OT_UNUSED_VARIABLE(aContext);

    uint8_t *buf = NULL;
    int length;
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    app_sensor_data_t s_data;

    otIp6AddressToString(&aMessageInfo->mPeerAddr, string, sizeof(string));
    length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    info("%d bytes from \r\n", length);
    info("ip : %s\r\n", string);
    info("port : %d \r\n", aMessageInfo->mSockPort);
    buf = mem_malloc(length);

    if (buf)
    {
        otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, length);

        /*sensor data Receiver queue*/
        if (!thread_app_sensor_data_parse(buf, length, &s_data))
        {
            thread_app_sensor_data_queue_push(APP_SENSOR_DATA_RECEIVED, buf, length);
        }
        else
        {
            info("Message Received : ");
            for (int i = 0; i < length; i++)
            {
                info("%02x", buf[i]);
            }
            info("\r\n");
        }
        mem_free(buf);
    }
}

void _Udp_Init()
{
    otError error;
    otSockAddr bindAddr;

    // Open the socket
    error = otUdpOpen(g_app_instance, &socket, UdpReceiveCallBack, NULL);
    if (error != OT_ERROR_NONE)
    {
        otCliOutputFormat("Failed to open udp socket with: %d, %s\r\n", error, otThreadErrorToString(error));
        return;
    }

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.mPort = UDP_PORT;

    // Bind to the socket. Close the socket if bind fails.
    error = otUdpBind(g_app_instance, &socket, &sockaddr, OT_NETIF_THREAD);
    if (error != OT_ERROR_NONE)
    {
        otCliOutputFormat("Failed to bind udp socket with: %d, %s\r\n", error, otThreadErrorToString(error));
        error = otUdpClose(g_app_instance, &socket);
        return;
    }
}

void app_sensor_event_process()
{
    app_sensor_event_queue_t event_data;
    memset(&event_data, 0x0, sizeof(app_sensor_event_queue_t));
    /*process ota event*/
    do
    {
        enter_critical_section();
        if (thread_dequeue(&app_sensor_event_queue, &event_data))
        {
            leave_critical_section();
            break;
        }
        leave_critical_section();

        switch (event_data.event)
        {
        case APP_SENSOR_CONTROL_EVENT:
        case APP_SENSOR_GET_OTA_VERSION_EVENT:
        case APP_SENSOR_WAIT_OTA_EVENT:
            thread_app_sensor_data_send(event_data.data, event_data.data_lens);
            break;
        case APP_SENSOR_DATA_RECEIVED:
            thread_app_sensor_data_received(event_data.data, event_data.data_lens);
            break;
        default:
            info("unknow event %u\r\n", event_data.event);
            break;
        }
    } while (0);
}

void _Network_Interface_State_Change(uint32_t aFlags, void *aContext)
{
    uint8_t show_ip = 0, is_child = 0;
    if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        otDeviceRole changeRole = otThreadGetDeviceRole(g_app_instance);
        switch (changeRole)
        {
        case OT_DEVICE_ROLE_DETACHED:
            otCliOutputFormat("Change to detached \r\n");
        case OT_DEVICE_ROLE_DISABLED:
            break;
        case OT_DEVICE_ROLE_LEADER:
            otCliOutputFormat("Change to leader \r\n");
            show_ip = 1;
            break;
        case OT_DEVICE_ROLE_ROUTER:
            otCliOutputFormat("Change to router \r\n");
            show_ip = 1;
            break;
        case OT_DEVICE_ROLE_CHILD:
            otCliOutputFormat("Change to child \r\n");
            show_ip = 1;
            is_child = 1;
            break;
        default:
            break;
        }

        if (show_ip)
        {
            const otNetifAddress *unicastAddress = otIp6GetUnicastAddresses(g_app_instance);

            for (const otNetifAddress *addr = unicastAddress; addr; addr = addr->mNext)
            {
                char string[OT_IP6_ADDRESS_STRING_SIZE];

                otIp6AddressToString(&addr->mAddress, string, sizeof(string));
                otCliOutputFormat("%s\r\n", string);
            }
        }
        if (is_child)
        {
            NETWORKING_TMR_STOP();
            if (g_sensor_event != 0xff)
            {
                thread_app_sensor_data_generate(g_sensor_event);
                g_sensor_event = 0xff;
            }
            else
            {
#if APP_DOOR_SENSOR_USE
                thread_app_sensor_data_generate(APP_SENSOR_CONTROL_EVENT);
#else
                thread_go_deep_sleep();
#endif
            }
        }
    }
}

static otError Processota(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aContext);
    otError error = OT_ERROR_NONE;

    if (0 == aArgsLength)
    {
        info("ota state : %s \r\n", OtaStateToString(ota_get_state()));
        info("ota image version : 0x%08x\r\n", ota_get_image_version());
        info("ota image size : 0x%08x \r\n", ota_get_image_size());
        info("ota image crc : 0x%08x \r\n", ota_get_image_crc());
    }
    else if (!strcmp(aArgs[0], "start"))
    {
        if (aArgsLength > 2)
        {
            do
            {
                uint64_t segments_size = 0;
                uint64_t intervel = 0;
                error = otParseAsUint64(aArgs[1], &segments_size);
                if (error != OT_ERROR_NONE)
                {
                    break;
                }
                error = otParseAsUint64(aArgs[2], &intervel);
                if (error != OT_ERROR_NONE)
                {
                    break;
                }
                info("segments_size %u ,intervel %u \r\n", (uint16_t)segments_size, (uint16_t)intervel);
                ota_start((uint16_t)segments_size, (uint16_t)intervel);
            } while (0);
        }
        else
        {
            error = OT_ERROR_INVALID_COMMAND;
        }
    }
    else if (!strcmp(aArgs[0], "send"))
    {
        if (aArgsLength > 1)
        {
            ota_send(aArgs[1]);
        }
    }
    else if (!strcmp(aArgs[0], "stop"))
    {
        ota_stop();
    }
    else if (!strcmp(aArgs[0], "debug"))
    {
        if (aArgsLength > 1)
        {
            uint64_t level = 0;
            error = otParseAsUint64(aArgs[1], &level);
            ota_debug_level((unsigned int)level);
        }
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

static otError Processmemory(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aContext);
    otError error = OT_ERROR_NONE;

    if (0 == aArgsLength)
    {
        mem_mgmt_show_info();
    }

exit:
    return error;
}

static const otCliCommand kCommands[] =
{
    {"ota", Processota},
    {"mem", Processmemory},
};

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

void _app_init(void)
{
#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    size_t otInstanceBufferLength = 0;
    uint8_t *otInstanceBuffer = NULL;

    // Call to query the buffer size
    (void)otInstanceInit(NULL, &otInstanceBufferLength);

    // Call to allocate the buffer
    otInstanceBuffer = (uint8_t *)malloc(otInstanceBufferLength);
    OT_ASSERT(otInstanceBuffer);

    // Initialize OpenThread with the buffer
    g_app_instance = otInstanceInit(otInstanceBuffer, &otInstanceBufferLength);
#else
    g_app_instance = otInstanceInitSingle();
#endif
    OT_ASSERT(g_app_instance);

    otAppCliInit(g_app_instance);

#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
    OT_ASSERT(otLoggingSetLevel(OT_LOG_LEVEL_NONE) == OT_ERROR_NONE);
#endif

    info("Toggle SubG Thread and BLE Init ability MTD \r\n");

    /* Initialize thread network paraneters*/
    _Set_Network_Configuration();
    /* Initialize thread sleep mode*/
    _Sleep_Init();
    /* Initialize udp setting*/
    _Udp_Init();

    /* Initialize ota */
    OT_ASSERT(ota_init(g_app_instance) == OT_ERROR_NONE);

    /* Brings ipv6 interface */
    OT_ASSERT(otIp6SetEnabled(g_app_instance, true) == OT_ERROR_NONE);
    /* start thread network */
    OT_ASSERT(otThreadSetEnabled(g_app_instance, true) == OT_ERROR_NONE);

    /* Initialize thread app tick*/
    Thread_Time_Tick = xTimerCreate("Thread_Time_Tick_T", pdMS_TO_TICKS(1 * 1000), false, NULL, thread_app_time_tick_handler);
    xTimerStart(Thread_Time_Tick, 0);
    NETWORKING_TMR_START();

    /* Registers a callback to indicate when certain configuration or state changes within OpenThread */
    OT_ASSERT(otSetStateChangedCallback(g_app_instance, _Network_Interface_State_Change, 0) == OT_ERROR_NONE);

    /* add user command */
    OT_ASSERT(otCliSetUserCommands(kCommands, OT_ARRAY_LENGTH(kCommands), g_app_instance) == OT_ERROR_NONE);

    info("Thread version     : %s \r\n", otGetVersionString());

    info("Link Mode           %d, %d, %d \r\n",
         otThreadGetLinkMode(g_app_instance).mRxOnWhenIdle,
         otThreadGetLinkMode(g_app_instance).mDeviceType,
         otThreadGetLinkMode(g_app_instance).mNetworkData);
    info("Network name        : %s \r\n", otThreadGetNetworkName(g_app_instance));
    info("PAN ID              : %x \r\n", otLinkGetPanId(g_app_instance));

    info("channel             : %d \r\n", otLinkGetChannel(g_app_instance));
    info("networkkey          : ");
    otNetworkKey networkKey;
    otThreadGetNetworkKey(g_app_instance, &networkKey);
    for (uint8_t i = 0; i < 16; i++)
    {
        info("%02x", networkKey.m8[i]);
    }
    info("\r\n");
}

static void app_main_loop(void *aContext)
{
    OT_UNUSED_VARIABLE(aContext);

    otSysInit(0, NULL);

    _app_init();

    thread_queue_init(&app_sensor_event_queue, 5, sizeof(app_sensor_event_queue_t));

    otSysProcessDrivers(otGetInstance());
    while (!otSysPseudoResetWasRequested())
    {
        otTaskletsProcess(otGetInstance());
        if (!otTaskletsArePending(otGetInstance()))
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
        /*openthread use*/
        otSysProcessDrivers(otGetInstance());
        /*app sensor data use*/
        app_sensor_event_process();
    }

    otInstanceFinalize(otGetInstance());
    vTaskDelete(NULL);
}

void thread_app_task_start()
{
    xTaskCreate(app_main_loop, "app", APP_TASK_STACK_SIZE, NULL, THREAD_APP_TASK_PRIORITY, &sAppTask);
}

void otTaskletsSignalPending(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    xTaskNotifyGive(sAppTask);
}

void otSysEventSignalPending(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(sAppTask, &xHigherPriorityTaskWoken);
    /* Context switch needed? */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}