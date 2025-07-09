#include "project_config.h"
#include "bsp.h"
#include "bsp_button.h"
#include "rfb.h"
#include "main.h"
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

bool g_button1_hold = false;

static TaskHandle_t sAppTask = NULL;
static otInstance *g_app_instance = NULL;
extern void otAppCliInit(otInstance *aInstance);

static TimerHandle_t Thread_Time_Tick;
static uint16_t networking_tmr = 0;
static uint16_t wait_ota_tmr = 0;
static uint16_t g_button1_tmr = 0;

/*timeout */
#define NETWORKING_TIMEOUT 10
#define WAIT_OTA_TIMEOUT 60

#define NETWORKING_TMR_START() (networking_tmr = NETWORKING_TIMEOUT)
#define WAIT_OTA_TMR_START() (wait_ota_tmr = WAIT_OTA_TIMEOUT)

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
        if (bsp_button_state_get(BSP_BUTTON_1))
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

static void app_task_network_configuration_setting()
{
    static char aNetworkName[] = "Rafael Thread";

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
            thread_go_deep_sleep();
        }
    }
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
    {"mem", Processmemory},
};

void app_sleep_init()
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

void ota_state_change_cb(uint8_t state)
{
    switch (state)
    {
    case OTA_IDLE:
        info("change to ota idle state \r\n");
        break;
    case OTA_DATA_SENDING:
        info("change to ota sending state \r\n");
        break;
    case OTA_DATA_RECEIVING:
        info("change to ota receiving state \r\n");
        break;
    case OTA_UNICASE_RECEIVING:
        info("change to ota unicase receiving state \r\n");
        break;
    case OTA_REQUEST_SENDING:
        info("change to ota request sending state \r\n");
        break;
    case OTA_WAKEUP_RECEIVING:
        info("change to ota wakeup state \r\n");
        break;
    case OTA_DONE:
        info("change to ota done state \r\n");
        break;
    case OTA_REBOOT:
        info("change to ota reboot state \r\n");
        break;
    default:
        break;
    }
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

    /*bootup network config setting*/
    app_task_network_configuration_setting();

    /*sleep_init*/
    app_sleep_init();

    do
    {
        info("\r\n");
        /* Initialize thread udp sock */
        if (app_sock_init(g_app_instance) != OT_ERROR_NONE)
        {
            info("sock init fail \r\n");
            break;
        }

        if (ota_init(g_app_instance, ota_state_change_cb) != OT_ERROR_NONE)
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

        /* Registers a callback to indicate when certain configuration or state changes within OpenThread */
        if (otSetStateChangedCallback(g_app_instance, _Network_Interface_State_Change, 0) != OT_ERROR_NONE)
        {
            info("otSetStateChangedCallback fail \r\n");
            break;
        }

        /* add user command */
        if (otCliSetUserCommands(kCommands, OT_ARRAY_LENGTH(kCommands), g_app_instance) != OT_ERROR_NONE)
        {
            info("otCliSetUserCommands fail \r\n");
            break;
        }

        /* Initialize thread app tick*/
        Thread_Time_Tick = xTimerCreate("Thread_Time_Tick_T", pdMS_TO_TICKS(1 * 1000), false, NULL, thread_app_time_tick_handler);
        xTimerStart(Thread_Time_Tick, 0);
        NETWORKING_TMR_START();

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

static void app_main_loop(void *aContext)
{
    OT_UNUSED_VARIABLE(aContext);

    otSysInit(0, NULL);

    _app_init();

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