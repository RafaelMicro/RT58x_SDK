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
#include "util_log.h"
#include "rfb.h"
#include "app.h"
#include "mem_mgmt.h"
#include "ota_handler.h"
#include "sw_timer.h"
#include "platform-rt58x.h"
#include "app_uart_handler.h"

static otUdpSocket socket;
static otSockAddr sockaddr;
static otInstance *g_app_instance = NULL;
extern void otAppCliInit(otInstance *aInstance);
extern otError ota_init(otInstance *aInstance);
void _Sleep_Init(void);
void UdpReceiveCallBack(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);

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

static void _Set_Network_Configuration()
{
    static char aNetworkName[] = "Thread_RT58X";

    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x00, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00};

#if 0 // for certification
    uint8_t nwkkey[OT_NETWORK_KEY_SIZE] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
                                          };
#else
    uint8_t nwkkey[OT_NETWORK_KEY_SIZE] = {0xfe, 0x83, 0x44, 0x8a, 0x67, 0x29, 0xfe, 0xab,
                                           0xab, 0xfe, 0x29, 0x67, 0x8a, 0x44, 0x83, 0xff
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

    memset(&aDataset, 0, sizeof(otOperationalDataset));

    aDataset.mActiveTimestamp.mSeconds = 1;
    aDataset.mComponents.mIsActiveTimestampPresent = true;

    /* Set Channel */
    aDataset.mChannel = DEF_CHANNEL;
    aDataset.mComponents.mIsChannelPresent = true;

    /* Set Pan ID */
    aDataset.mPanId = (otPanId)0xbee0;
    aDataset.mComponents.mIsPanIdPresent = true;

    /* Set Extended Pan ID */
    memcpy(aDataset.mExtendedPanId.m8, extPanId, OT_EXT_PAN_ID_SIZE);
    aDataset.mComponents.mIsExtendedPanIdPresent = true;

    /* Set network key */
    memcpy(aDataset.mNetworkKey.m8, nwkkey, OT_NETWORK_KEY_SIZE);
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

void _Network_Interface_State_Change(uint32_t aFlags, void *aContext)
{
    uint8_t show_ip = 0;
    if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        otDeviceRole changeRole = otThreadGetDeviceRole(g_app_instance);
        switch (changeRole)
        {
        case OT_DEVICE_ROLE_DETACHED:
            otCliOutputFormat("Change to detached \r\n");
        case OT_DEVICE_ROLE_DISABLED:
            gpio_pin_write(20, 1);
            gpio_pin_write(21, 1);
            gpio_pin_write(22, 1);
            break;
        case OT_DEVICE_ROLE_LEADER:
            otCliOutputFormat("Change to leader \r\n");
            gpio_pin_write(20, 0);
            gpio_pin_write(21, 0);
            gpio_pin_write(22, 0);
            show_ip = 1;
            break;
        case OT_DEVICE_ROLE_ROUTER:
            otCliOutputFormat("Change to router \r\n");
            show_ip = 1;
            gpio_pin_write(20, 1);
            gpio_pin_write(21, 0);
            gpio_pin_write(22, 1);
            break;
        case OT_DEVICE_ROLE_CHILD:
            otCliOutputFormat("Change to child \r\n");
            show_ip = 1;
            gpio_pin_write(20, 1);
            gpio_pin_write(21, 1);
            gpio_pin_write(22, 0);
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
                otCliOutputFormat("%s\n", string);
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
        info("ota state : %s \n", OtaStateToString(ota_get_state()));
        info("ota image version : 0x%08x\n", ota_get_image_version());
        info("ota image size : 0x%08x \n", ota_get_image_size());
        info("ota image crc : 0x%08x \n", ota_get_image_crc());
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
                info("segments_size %u ,intervel %u \n", (uint16_t)segments_size, (uint16_t)intervel);
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

    //not use freertos
    sw_timer_init();

    _Sleep_Init();
    _Set_Network_Configuration();
    _Udp_Init();

    /*bin download will use uart 1 */
    app_uart_handler_init();

    OT_ASSERT(ota_init(g_app_instance) == OT_ERROR_NONE);

    OT_ASSERT(otIp6SetEnabled(g_app_instance, true) == OT_ERROR_NONE);
    OT_ASSERT(otThreadSetEnabled(g_app_instance, true) == OT_ERROR_NONE);

    OT_ASSERT(otSetStateChangedCallback(g_app_instance, _Network_Interface_State_Change, 0) == OT_ERROR_NONE);

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
    for (uint8_t i = 0; i < OT_NETWORK_KEY_SIZE; i++)
    {
        info("%02x", networkKey.m8[i]);
    }
    info("\r\n");
}

void _app_process_action()
{
    /*sw timer use*/
    sw_timer_proc();
    /*bin download will use uart 1 */
    app_uart_handler_recv();
    /*openthread use*/
    otTaskletsProcess(g_app_instance);
    otSysProcessDrivers(g_app_instance);
}

void _app_exit(void)
{
    otInstanceFinalize(g_app_instance);
}

void otTaskletsSignalPending(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

void otSysEventSignalPending(void)
{

}