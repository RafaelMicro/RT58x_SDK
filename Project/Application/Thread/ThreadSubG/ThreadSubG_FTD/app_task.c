#include "main.h"
#include "project_config.h"
#include "ota_handler.h"
#include "platform-rt58x.h"

static otInstance *g_app_instance = NULL;
extern void otAppCliInit(otInstance *aInstance);
extern otError ota_init(otInstance *aInstance);

bool app_task_check_leader_pin_state()
{
    return (gpio_pin_get(23) == 0);
}

otInstance *otGetInstance()
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

static void app_task_network_configuration_setting()
{
    static char aNetworkName[] = "Rafael Thread";

    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x00, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00};

#if 0 // for certification
    uint8_t nwkkey[OT_NETWORK_KEY_SIZE] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
                                          };
#else
    uint8_t nwkkey[OT_NETWORK_KEY_SIZE] = {0xfe, 0x83, 0x44, 0x8a, 0x67, 0x29, 0xfe, 0xab,
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
    memset(&aDataset, 0, sizeof(otOperationalDataset));

    error = otDatasetGetActive(otGetInstance(), &aDataset);
    if (error != OT_ERROR_NONE)
    {
        info("otDatasetGetActive failed with %d %s\r\n", error, otThreadErrorToString(error));
    }

    if (app_task_check_leader_pin_state() == true)
    {
        aDataset.mActiveTimestamp.mSeconds = 1;
        aDataset.mComponents.mIsActiveTimestampPresent = true;
    }
    else
    {
        aDataset.mActiveTimestamp.mSeconds = 0;
        aDataset.mComponents.mIsActiveTimestampPresent = false;
    }
    /* Set Channel */
    aDataset.mChannel = DEF_CHANNEL;
    aDataset.mComponents.mIsChannelPresent = true;

    /* Set Pan ID */
    aDataset.mPanId = (otPanId)0xabcd;
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
    error = otDatasetSetActive(otGetInstance(), &aDataset);
    if (error != OT_ERROR_NONE)
    {
        info("otDatasetSetActive failed with %d %s\r\n", error, otThreadErrorToString(error));
    }

    /* set extaddr to equal eui64*/
    otExtAddress extAddress;
    otLinkGetFactoryAssignedIeeeEui64(otGetInstance(), &extAddress);
    error = otLinkSetExtendedAddress(otGetInstance(), &extAddress);
    if (error != OT_ERROR_NONE)
    {
        info("set extaddr fail\r\n");
    }

    /* set mle eid to equal eui64*/
    otIp6InterfaceIdentifier iid;
    memcpy(iid.mFields.m8, extAddress.m8, OT_EXT_ADDRESS_SIZE);
    error = otIp6SetMeshLocalIid(otGetInstance(), &iid);
    if (error != OT_ERROR_NONE)
    {
        info("set mle eid fail\r\n");
    }
}

void _Network_Interface_State_Change(uint32_t aFlags, void *aContext)
{
    uint8_t show_ip = 0;
    otOperationalDataset ds;
    if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        otDeviceRole changeRole = otThreadGetDeviceRole(otGetInstance());
        if (app_task_check_leader_pin_state() == false)
        {
            if (changeRole == OT_DEVICE_ROLE_LEADER)
            {
                memset(&ds, 0, sizeof(otOperationalDataset));
                if (otDatasetGetActive(otGetInstance(), &ds) == OT_ERROR_NONE)
                {
                    ds.mActiveTimestamp.mSeconds = 0;
                    ds.mComponents.mIsActiveTimestampPresent = false;
                    if (otDatasetSetActive(otGetInstance(), &ds) != OT_ERROR_NONE)
                    {
                        info("otDatasetSetActive fail \r\n");
                    }
                }
                if (otThreadBecomeDetached(otGetInstance()) != OT_ERROR_NONE)
                {
                    info("otThreadBecomeDetached fail \r\n");
                }
                return;
            }
        }
        else
        {
            if (changeRole != OT_DEVICE_ROLE_LEADER)
            {
                memset(&ds, 0, sizeof(otOperationalDataset));
                if (otDatasetGetActive(otGetInstance(), &ds) == OT_ERROR_NONE)
                {
                    ds.mActiveTimestamp.mSeconds = 1;
                    ds.mComponents.mIsActiveTimestampPresent = true;
                    if (otDatasetSetActive(otGetInstance(), &ds) != OT_ERROR_NONE)
                    {
                        info("otDatasetSetActive fail \r\n");
                    }
                }
                if (otThreadBecomeLeader(otGetInstance()) != OT_ERROR_NONE)
                {
                    info("otThreadBecomeLeader fail \r\n");
                }
                return;
            }
        }
        switch (changeRole)
        {
        case OT_DEVICE_ROLE_DETACHED:
            info("Change to detached \r\n");
        case OT_DEVICE_ROLE_DISABLED:
            gpio_pin_write(20, 1);
            gpio_pin_write(21, 1);
            gpio_pin_write(22, 1);
            break;
        case OT_DEVICE_ROLE_LEADER:
            info("Change to leader \r\n");
            gpio_pin_write(20, 0);
            gpio_pin_write(21, 0);
            gpio_pin_write(22, 0);
            show_ip = 1;
            break;
        case OT_DEVICE_ROLE_ROUTER:
            info("Change to router \r\n");
            nwk_mgm_child_register_post();
            show_ip = 1;
            gpio_pin_write(20, 1);
            gpio_pin_write(21, 0);
            gpio_pin_write(22, 1);
            break;
        case OT_DEVICE_ROLE_CHILD:
            info("Change to child \r\n");
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
            const otNetifAddress *unicastAddress = otIp6GetUnicastAddresses(otGetInstance());

            for (const otNetifAddress *addr = unicastAddress; addr; addr = addr->mNext)
            {
                char string[OT_IP6_ADDRESS_STRING_SIZE];

                otIp6AddressToString(&addr->mAddress, string, sizeof(string));
                info("%s\n", string);
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

static otError Processnwkmem(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aContext);
    otError error = OT_ERROR_NONE;

    if (0 == aArgsLength)
    {
        nwk_mgm_child_reg_table_display();
    }
    else if (!strcmp(aArgs[0], "debug"))
    {
        if (aArgsLength > 1)
        {
            uint64_t level = 0;
            error = otParseAsUint64(aArgs[1], &level);
            nwk_mgm_debug_level((unsigned int)level);
        }
    }

exit:
    return error;
}

static otError Procesappudp(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aContext);
    otError error = OT_ERROR_FAILED;
    otIp6Address dst_addr;
    uint16_t data_lens = 0;
    uint8_t *data = NULL;
    do
    {
        if (aArgsLength < 2)
        {
            break;
        }

        if (otIp6AddressFromString(aArgs[0], &dst_addr) != OT_ERROR_NONE)
        {
            info("otIp6AddressFromString fail \r\n");
            break;
        }
        data_lens = (strlen(aArgs[1]) + 1) / 2;
        data = mem_malloc(data_lens);
        if (NULL == data)
        {
            break;
        }
        if (otParseHexString(aArgs[1], data, data_lens) != OT_ERROR_NONE)
        {
            info("otParseHexString fail \r\n");
            break;
        }

        if (app_udp_send(dst_addr, data, data_lens) != OT_ERROR_NONE)
        {
            info("app_udp_send fail \r\n");
            break;
        }
        error = OT_ERROR_NONE;
    } while (0);

    if (data)
    {
        mem_free(data);
    }
exit:
    return error;
}

static const otCliCommand kCommands[] =
{
    {"ota", Processota},
    {"mem", Processmemory},
    {"nwk", Processnwkmem},
    {"appudp", Procesappudp},
};

void app_sleep_init()
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

void app_task_init()
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

    /*bootup network config setting*/
    app_task_network_configuration_setting();

    /*sleep_init*/
    app_sleep_init();

    do
    {
        info("\r\n");
        /*udp init*/
        if (app_sock_init(g_app_instance) != OT_ERROR_NONE)
        {
            info("sock init fail \r\n");
            break;
        }

        if (ota_init(g_app_instance) != OT_ERROR_NONE)
        {
            info("ota init fail \r\n");
            break;
        }

        if (otIp6SetEnabled(g_app_instance, true) != OT_ERROR_NONE)
        {
            info("otIp6SetEnabled fail \r\n");
            break;
        }

        if (otThreadSetEnabled(g_app_instance, true) != OT_ERROR_NONE)
        {
            info("otThreadSetEnabled fail \r\n");
            break;
        }

        otThreadRegisterNeighborTableCallback(g_app_instance, _Network_Interface_Neighbor_Table_change);

        if (otSetStateChangedCallback(g_app_instance, _Network_Interface_State_Change, 0) != OT_ERROR_NONE)
        {
            info("otSetStateChangedCallback fail \r\n");
            break;
        }

        if (otCliSetUserCommands(kCommands, OT_ARRAY_LENGTH(kCommands), g_app_instance) != OT_ERROR_NONE)
        {
            info("otCliSetUserCommands fail \r\n");
            break;
        }

        if (app_task_check_leader_pin_state() == true)
        {
            if (otThreadBecomeLeader(g_app_instance) != OT_ERROR_NONE)
            {
                info("otThreadBecomeLeader fail \r\n");
                break;
            }
            otThreadSetLocalLeaderWeight(g_app_instance, 128);
            nwk_mgm_init(g_app_instance, true);
        }
        else
        {
            nwk_mgm_init(g_app_instance, false);
        }
    } while (0);

    info("Thread version      : %s \r\n", otGetVersionString());

    info("Link Mode           : %d, %d, %d \r\n",
         otThreadGetLinkMode(g_app_instance).mRxOnWhenIdle,
         otThreadGetLinkMode(g_app_instance).mDeviceType,
         otThreadGetLinkMode(g_app_instance).mNetworkData);
    const otMeshLocalPrefix *MeshPrefix = otThreadGetMeshLocalPrefix(g_app_instance);
    info("Mesh IP Prefix      : %02X%02X:%02X%02X:%02X%02X:%02X%02X: \r\n", \
         MeshPrefix->m8[0], MeshPrefix->m8[1], MeshPrefix->m8[2], MeshPrefix->m8[3],
         MeshPrefix->m8[4], MeshPrefix->m8[5], MeshPrefix->m8[6], MeshPrefix->m8[7]);
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
    const otExtAddress *extaddr = otLinkGetExtendedAddress(g_app_instance);
    info("Extaddr             : %02X%02X%02X%02X%02X%02X%02X%02X \r\n", \
         extaddr->m8[0], extaddr->m8[1], extaddr->m8[2], extaddr->m8[3], \
         extaddr->m8[4], extaddr->m8[5], extaddr->m8[6], extaddr->m8[7]);
    info("=================================\r\n");
}

void app_task_process_action()
{
    otTaskletsProcess(otGetInstance());
    otSysProcessDrivers(otGetInstance());
}

void app_task_exit()
{
    otInstanceFinalize(otGetInstance());
}

void otTaskletsSignalPending(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

void otSysEventSignalPending()
{

}