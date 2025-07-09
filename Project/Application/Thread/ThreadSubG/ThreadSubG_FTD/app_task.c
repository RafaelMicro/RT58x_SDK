#include "main.h"
#include "project_config.h"
#include "ota_handler.h"
#include "platform-rt58x.h"
#include "thread_queue.h"
#include "gpio.h"
#include "sw_timer.h"
#include <openthread/random_noncrypto.h>
#include <openthread/netdiag.h>
#include "bin_version.h"

static otInstance *g_app_instance = NULL;
extern void otAppCliInit(otInstance *aInstance);
static sw_timer_t *app_led_timer = NULL;
static sw_timer_t *app_led_ack_timer = NULL;
static sw_timer_t *app_runtime_record_timer = NULL;
static uint32_t runtimeCount = 0;
static char tmp_ip_string[OT_IP6_ADDRESS_STRING_SIZE];

static uint8_t ip_null[OT_IP6_ADDRESS_SIZE] = {0x0, 0x0, 0x0, 0x0, \
                                               0x0, 0x0, 0x0, 0x0, \
                                               0x0, 0x0, 0x0, 0x0, \
                                               0x0, 0x0, 0x0, 0x0
                                              }; //check use
extern otIp6Address app_udp_last_src_ip;

void app_set_led0_on(void)
{
    gpio_pin_write(20, 0);
    gpio_pin_write(15, 1);
}

void app_set_led0_off(void)
{
    gpio_pin_write(20, 1);
    gpio_pin_write(15, 0);
}

void app_set_led0_toggle(void)
{
    gpio_pin_toggle(20);
    gpio_pin_toggle(15);
}

static void app_led_timer_handler(void *p_param)
{
    app_set_led0_off();
    //clear timer point
    sw_timer_delete(app_led_timer);
    app_led_timer = NULL;
}

void app_set_led0_flash(void)
{
    app_set_led0_on();
    if (NULL == app_led_timer)
    {
        app_led_timer = sw_timer_create("app_udp_led_ack_timer",
                                        100,
                                        false,
                                        SW_TIMER_EXECUTE_ONCE_FOR_EACH_TIMEOUT,
                                        NULL,
                                        app_led_timer_handler);
        sw_timer_start(app_led_timer);
    }
}

void app_set_led1_on(void)
{
    gpio_pin_write(21, 0);
    gpio_pin_write(14, 1);
}

void app_set_led1_off(void)
{
    gpio_pin_write(21, 1);
    gpio_pin_write(14, 0);
}

void app_set_led1_toggle(void)
{
    gpio_pin_toggle(21);
    gpio_pin_toggle(14);
}

#if 0  //EVK hardware not support GPIO-22 (need wire connection)
void app_set_led2_on(void)
{
    gpio_pin_write(22, 0);
}

void app_set_led2_off(void)
{
    gpio_pin_write(22, 1);
}
#endif

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
    uint8_t nwkkey[OT_NETWORK_KEY_SIZE] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                                           0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
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
    aDataset.mPanId = (otPanId)0xabcd;
    aDataset.mComponents.mIsPanIdPresent = true;

    /* Set Extended Pan ID */
    memcpy(aDataset.mExtendedPanId.m8, extPanId, OT_EXT_PAN_ID_SIZE);
    aDataset.mComponents.mIsExtendedPanIdPresent = true;

    /* Set network key */
    memcpy(aDataset.mNetworkKey.m8, nwkkey, OT_NETWORK_KEY_SIZE);
    aDataset.mComponents.mIsNetworkKeyPresent = true;

    /* Set pskc */
    memcpy(aDataset.mPskc.m8, aPSKc, OT_PSKC_MAX_SIZE);
    aDataset.mComponents.mIsPskcPresent = true;

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
        info("current bin version : 0x%08x \n", GET_BIN_VERSION(systeminfo.sysinfo));
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
    else if (!strcmp(aArgs[0], "reset"))
    {
        ota_reset();
    }
    else if (!strcmp(aArgs[0], "wakeup"))
    {
        ota_send_wakeup();
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

#if CFG_USE_CENTRAK_CONFIG
static otError Processnwkmem(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aContext);
    otError error = OT_ERROR_NONE;

    if (0 == aArgsLength)
    {
        net_mgm_node_table_display();
    }
    else if (!strcmp(aArgs[0], "debug"))
    {
        if (aArgsLength > 1)
        {
            uint64_t level = 0;
            error = otParseAsUint64(aArgs[1], &level);
            net_mgm_debug_level((unsigned int)level);
        }
    }

exit:
    return error;
}

static otError Processnodereset(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    do
    {
        if (!strcmp(aArgs[0], "erase"))
        {
            info("broadcast node factory reset... \r\n");
            net_mgm_node_reset_send(true);
        }
        else
        {
            info("broadcast node reset... \r\n");
            net_mgm_node_reset_send(false);
        }
    } while (0);

exit:
    return error;
}

#endif
static otError ProcessLedAck(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    //Do nothing, just get peer device status response
    if (aArgsLength > 0)
    {
        if (memcmp(&ip_null, &app_udp_last_src_ip.mFields.m8, OT_IP6_ADDRESS_SIZE) != 0)
        {
            otIp6AddressToString(&app_udp_last_src_ip, tmp_ip_string, sizeof(tmp_ip_string));
            info("[led ack] << %s: %s \r\n", tmp_ip_string, aArgs[0]);
            memset(&app_udp_last_src_ip.mFields.m8, 0x0, OT_IP6_ADDRESS_SIZE);
        }
        else
        {
            info("is not form app udp \r\n");
            return OT_ERROR_FAILED;
        }
    }
    return error;
}

static void app_udp_led_ack_timer_handler(void *p_param)
{
    char led_on_ok[20] = {"ledack On:+OK"};
    char led_off_ok[20] = {"ledack Off:+OK"};
    char led_toggle_ok[20] = {"ledack Toggle:+OK"};
    app_led_ack_timer_para *para = (app_led_ack_timer_para *) p_param;

    if (para)
    {
        char *ack_str = NULL;
        otIp6AddressToString(&para->src_ip, tmp_ip_string, sizeof(tmp_ip_string));
        switch (para->led_status)
        {
        case 0:
            info("[led ack] >> %s: Led Off! \r\n", tmp_ip_string);
            ack_str = led_off_ok;
            break;
        case 1:
            info("[led ack] >> %s: Led On!\r\n", tmp_ip_string);
            ack_str = led_on_ok;
            break;
        case 2:
            info("[led ack] >> %s: Led Toggle! \r\n", tmp_ip_string);
            ack_str = led_toggle_ok;
            break;
        }

        if (app_udp_send(para->src_ip, (uint8_t *)ack_str, strlen(ack_str)) != OT_ERROR_NONE)
        {
            info("led %d ack send fail \r\n", para->led_status);
        }
        mem_free(para);
    }
    //clear timer point
    sw_timer_delete(app_led_ack_timer);
    app_led_ack_timer = NULL;
}

static otError ProcessLed(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_FAILED;
    char led_on_ok[20] = {"ledack On:+OK"};
    char led_off_ok[20] = {"ledack Off:+OK"};
    char led_toggle_ok[20] = {"ledack Toggle:+OK"};
    uint8_t data_lens, led_status = 0xff;
    // e.g: set led_ack_delay_ms = 1200 ms
    uint64_t led_ack_delay_ms = 100;
    if (aArgsLength > 0)
    {
        if (memcmp(&ip_null, &app_udp_last_src_ip.mFields.m8, OT_IP6_ADDRESS_SIZE) != 0)
        {
            otIp6AddressToString(&app_udp_last_src_ip, tmp_ip_string, sizeof(tmp_ip_string));
        }
        else
        {
            info("is not form app udp \r\n");
            return error;
        }
        if (strcmp(aArgs[0], "on") == 0)
        {
            led_status = 1;
            info("[led] << %s: Led On! \r\n", tmp_ip_string);
            app_set_led0_on();
            error = OT_ERROR_NONE;
        }
        else if (strcmp(aArgs[0], "off") == 0)
        {
            led_status = 0;
            info("[led] << %s: Led Off! \r\n", tmp_ip_string);
            app_set_led0_off();
            error = OT_ERROR_NONE;

        }
        else if (strcmp(aArgs[0], "toggle") == 0)
        {
            led_status = 2;
            info("[led] << %s: Led Toggle! \r\n", tmp_ip_string);
            app_set_led0_toggle();
            error = OT_ERROR_NONE;
        }
        if (aArgsLength > 1)
        {
            if (aArgsLength > 2)
            {
                if (otParseAsUint64(aArgs[2], &led_ack_delay_ms) != OT_ERROR_NONE)
                {
                    info("delay ms parameters error! \r\n");
                    error = OT_ERROR_FAILED;
                    return error;
                }
                // delay number range limitation  100ms ~ 5000ms
                if (led_ack_delay_ms > 5000)
                {
                    led_ack_delay_ms = 5000;
                }
                else if (led_ack_delay_ms < 100)
                {
                    led_ack_delay_ms = 100;
                }
                app_led_ack_timer_para *para = NULL;
                para = mem_malloc(sizeof(app_led_ack_timer_para));
                if (para)
                {
                    para->led_status = led_status;
                    para->src_ip = app_udp_last_src_ip;
                    if (NULL == app_led_ack_timer)
                    {
                        app_led_ack_timer = sw_timer_create("app_udp_led_ack_timer",
                                                            led_ack_delay_ms,
                                                            false,
                                                            SW_TIMER_EXECUTE_ONCE_FOR_EACH_TIMEOUT,
                                                            (void *)para,
                                                            app_udp_led_ack_timer_handler);
                        sw_timer_start(app_led_ack_timer);
                    }
                    else
                    {
                        mem_free(para);
                    }
                }
                else
                {
                    error = OT_ERROR_FAILED;
                    return error;
                }
                error = OT_ERROR_NONE;
            }
            else
            {
                if (strcmp(aArgs[1], "ack") == 0)
                {
                    char *ack_str = NULL;
                    switch (led_status)
                    {
                    case 0:
                        info("[led ack] >> %s: Led Off! \r\n", tmp_ip_string);
                        ack_str = led_off_ok;
                        break;
                    case 1:
                        info("[led ack] >> %s: Led On!\r\n", tmp_ip_string);
                        ack_str = led_on_ok;
                        break;
                    case 2:
                        info("[led ack] >> %s: Led Toggle! \r\n", tmp_ip_string);
                        ack_str = led_toggle_ok;
                        break;
                    }

                    if (app_udp_send(app_udp_last_src_ip, (uint8_t *)ack_str, strlen(ack_str)) != OT_ERROR_NONE)
                    {
                        info("led %d ack send fail \r\n", led_status);
                        error = OT_ERROR_FAILED;
                        return error;
                    }
                    error = OT_ERROR_NONE;
                }
            }
        }
        memset(&app_udp_last_src_ip.mFields.m8, 0x0, OT_IP6_ADDRESS_SIZE);
    }

    return error;
}


static otError ProcessMcu(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_FAILED;
    char buff[50] = {};
    if (aArgsLength > 0)
    {
        if (memcmp(&ip_null, &app_udp_last_src_ip.mFields.m8, OT_IP6_ADDRESS_SIZE) != 0)
        {
            otIp6AddressToString(&app_udp_last_src_ip, tmp_ip_string, sizeof(tmp_ip_string));
        }
        else
        {
            info("is not form app udp \r\n");
            return error;
        }
        if (strcmp(aArgs[0], "read") == 0)
        {
            if (strcmp(aArgs[1], "runtime") == 0)
            {
                sprintf(buff, "mcu runtime %08dsec.:+OK", (runtimeCount * 5));
                info("[mcu time] >> %s: %08d sec \r\n", tmp_ip_string, (runtimeCount * 5));
                if (app_udp_send(app_udp_last_src_ip, (uint8_t *)buff, strlen(buff)) != OT_ERROR_NONE)
                {
                    info("udp send fail! \r\n");
                    return error;
                }
                error = OT_ERROR_NONE;
            }
            if (strcmp(aArgs[1], "version") == 0)
            {
                sprintf(buff, "mcu version 0x%08x:+OK", GET_BIN_VERSION(systeminfo.sysinfo));
                info("[mcu version] >> %s: Thread-SubG-%08x \r\n", tmp_ip_string, GET_BIN_VERSION(systeminfo.sysinfo));
                if (app_udp_send(app_udp_last_src_ip, (uint8_t *)buff, strlen(buff)) != OT_ERROR_NONE)
                {
                    info("udp send fail! \r\n");
                    return error;
                }
                error = OT_ERROR_NONE;
            }
        }
        else if (strcmp(aArgs[0], "runtime") == 0)
        {
            info("[mcu time] << %s: %s \r\n", tmp_ip_string, aArgs[1]);
            error = OT_ERROR_NONE;
        }
        else if (strcmp(aArgs[0], "version") == 0)
        {
            info("[mcu version] << %s: Thread-SubG-%s \r\n", tmp_ip_string, aArgs[1]);
            error = OT_ERROR_NONE;
        }
        memset(&app_udp_last_src_ip.mFields.m8, 0x0, OT_IP6_ADDRESS_SIZE);
    }
    return error;
}

static void app_runtime_record_timer_handler(void *p_param)
{
    runtimeCount++;
    if ((runtimeCount % 16) == 0)
    {
        info("[Timer] MCU Running Time = %d sec! \r\n", runtimeCount * 5);
    }
    //repeat timer, 5000ms timeout callback;
    sw_timer_start(app_runtime_record_timer);

}
// udpapp <ip_addr> <para0> <para1> <para2> ... <para(N-1)>
static otError Processappudp(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aContext);
    otError error = OT_ERROR_FAILED;
    otIp6Address dst_addr;
    uint16_t data_lens = 0;
    uint16_t cmd_lens = 0;
    uint8_t *data = NULL;
    uint16_t paraCount = 0;
    uint16_t offset = 0;
    uint8_t ch = ' ';

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

        //info("aArgs[1] = %s \r\n", aArgs[1]);
        if (strcmp("-x", aArgs[1]) == 0)  // "-t" :  hex mode
        {
            data_lens = (strlen(aArgs[2]) + 1) / 2;
            data = mem_malloc(data_lens);
            if (NULL == data)
            {
                break;
            }

            if (otParseHexString(aArgs[2], data, data_lens) != OT_ERROR_NONE)
            {
                info("otParseHexString fail \r\n");
                break;
            }

            if (app_udp_send(dst_addr, data, data_lens) != OT_ERROR_NONE)
            {
                info("app_udp_send fail \r\n");
                break;
            }

        }
        else if (strcmp("-c", aArgs[1]) == 0)   // "-c" cli mode
        {
            data = mem_malloc(1000);
            offset = 0;
            data_lens = 0;
            for (paraCount = 2; paraCount < aArgsLength; paraCount++)
            {
                cmd_lens = (strlen(aArgs[paraCount]));
                mem_memcpy(&data[offset], aArgs[paraCount], cmd_lens);
                offset += cmd_lens;
                mem_memcpy(&data[offset], &ch, 1);  //append char ' '
                offset ++;

                data_lens += (cmd_lens + 1);
            }

            if (app_udp_send(dst_addr, data, data_lens) != OT_ERROR_NONE)
            {
                info("app_udp_send fail \r\n");
                break;
            }

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

static otError Processpathreq(void *aContext, uint8_t aArgsLength, char *aArgs[])
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

        uint64_t timeout = 0;
        error = otParseAsUint64(aArgs[1], &timeout);
        if (error != OT_ERROR_NONE)
        {
            break;
        }

        if (path_req_send(dst_addr, (uint32_t)timeout) != OT_ERROR_NONE)
        {
            info("path req fail \r\n");
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

static otError Processmacsend(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    otIp6Address PeerAddr;
    uint64_t payload_len;
    uint64_t count;
    uint8_t *payload = NULL;
    uint16_t i = 0;
    do
    {
        if (aArgsLength < 2)
        {
            break;
        }
        error = otParseAsUint64(aArgs[0], &payload_len);
        if (error != OT_ERROR_NONE)
        {
            break;
        }
        error = otParseAsUint64(aArgs[1], &count);
        if (error != OT_ERROR_NONE)
        {
            break;
        }
        payload = mem_malloc(payload_len);
        if (payload)
        {
            memset(payload, 0xfe, payload_len);
            info("send %d %ld \r\n", payload_len, (uint16_t)count);
            for (i = 0; i < (uint16_t)count; i++)
            {
                radio_mac_broadcast_send(g_app_instance, payload, payload_len);
                Delay_ms(5);
            }

            mem_free(payload);
        }
    } while (0);
exit:
    return error;
}

static const otCliCommand kCommands[] =
{
    {"ota", Processota},
    {"mem", Processmemory},
#if CFG_USE_CENTRAK_CONFIG
    {"nwk", Processnwkmem},
    {"nodereset", Processnodereset},
#endif
    {"appudp", Processappudp},
    {"pathreq", Processpathreq},
    {"led", ProcessLed},   //single endpoint P2P control
    {"mcu", ProcessMcu},   // single endpoint information reading
    //Ack process, null function
    {"ledack", ProcessLedAck},//
    {"macsend", Processmacsend},
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


void app_mcu_runtime_record_init(void)
{
    app_runtime_record_timer = sw_timer_create("app_runtime_record_timer",
                               (5000),
                               false,
                               SW_TIMER_EXECUTE_ONCE_FOR_EACH_TIMEOUT,
                               NULL,
                               app_runtime_record_timer_handler);
    sw_timer_start(app_runtime_record_timer);

}

void mac_received_cb(uint8_t *data, uint16_t lens, int8_t rssi, uint8_t *src_addr)
{
    bool rep = true;
    for (uint16_t i = 0 ; i < lens; i++)
    {
        if (data[i] != 0xfe)
        {
            rep = false;
        }
    }
    info("lens(%d), src(%02x%02x%02x%02x%02x%02x%02x%02x), rssi(%d) \r\n",
         lens,
         src_addr[0], src_addr[1], src_addr[2], src_addr[3],
         src_addr[4], src_addr[5], src_addr[6], src_addr[7],
         rssi);
    if (rep)
    {
        char ack[3] = "ack";
        const otExtAddress *extaddr = otLinkGetExtendedAddress(g_app_instance);
        uint8_t random_index;
        random_index = otRandomNonCryptoGetUint8InRange(1, 255);
        Delay_ms(random_index + extaddr->m8[7]);
        radio_mac_unicast_send(g_app_instance, src_addr, (uint8_t *)ack, 3);
    }
}
#if !CFG_USE_CENTRAK_CONFIG
void ot_state_change_interface(uint32_t aFlags, void *aContext)
{
    uint8_t show_ip = 0, is_child = 0;
    otOperationalDataset ds;
    if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        otDeviceRole changeRole = otThreadGetDeviceRole(otGetInstance());
        switch (changeRole)
        {
        case OT_DEVICE_ROLE_DETACHED:
            info("Change to detached \r\n");
            break;
        case OT_DEVICE_ROLE_DISABLED:
            info("Change to disabled \r\n");
            break;
        case OT_DEVICE_ROLE_LEADER:
            info("Change to leader \r\n");
            show_ip = 1;
            break;
        case OT_DEVICE_ROLE_ROUTER:
            info("Change to router \r\n");
            show_ip = 1;
            break;
        case OT_DEVICE_ROLE_CHILD:
            info("Change to child \r\n");
            show_ip = 1;
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
#endif

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
    bool netinit = false;
    do
    {
        info("\r\n");
        /*udp init*/
        if (app_sock_init(g_app_instance) != OT_ERROR_NONE)
        {
            info("sock init fail \r\n");
            break;
        }

        radio_mac_received_callback(mac_received_cb);

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
#if !CFG_USE_CENTRAK_CONFIG
        if (otSetStateChangedCallback(otGetInstance(), ot_state_change_interface, 0) != OT_ERROR_NONE)
        {
            info("otSetStateChangedCallback fail \r\n");
        }
        if (otThreadSetEnabled(otGetInstance(), true) != OT_ERROR_NONE)
        {
            info("otThreadSetEnabled fail \r\n");
        }
#endif
        if (otCliSetUserCommands(kCommands, OT_ARRAY_LENGTH(kCommands), g_app_instance) != OT_ERROR_NONE)
        {
            info("otCliSetUserCommands fail \r\n");
            break;
        }
        netinit = true;
    } while (0);

    if (netinit)
    {
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

        app_udp_comm_init(g_app_instance);
    }
    else
    {
        info("thread init fail \r\n");
    }
}

void app_task_process_action()
{
    otTaskletsProcess(otGetInstance());
    otSysProcessDrivers(otGetInstance());
    app_udp_message_queue_process(); //udp parser main loop
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
