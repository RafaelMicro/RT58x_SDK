/**
 * @file app_udp.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-10-06
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "main.h"
#include "thread_queue.h"

#define THREAD_UDP_PORT 5678

static otUdpSocket          appSock;

static void app_udp_receive_handler(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    uint8_t *buf = NULL;
    uint8_t data_seq = 0, cmd = 0xFF, pid = 0xFF;
    int length;
    char string[OT_IP6_ADDRESS_STRING_SIZE];
    app_sensor_data_t s_data;

    otIp6AddressToString(&aMessageInfo->mPeerAddr, string, sizeof(string));
    length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);

    info("%d bytes from \n", length);
    info("ip : %s\n", string);
    info("port : %d \n", aMessageInfo->mSockPort);
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
            info("\n");
        }
        mem_free(buf);
    }
}

otError app_udp_send(otIp6Address dst_addr, uint8_t *data, uint16_t data_lens)
{
    otError error = OT_ERROR_NONE;
    otMessage *message = NULL;
    otMessageInfo messageInfo;
    otBufferInfo bufInfo;
    otMessageSettings messageSettings = {true, OT_MESSAGE_PRIORITY_NORMAL};

    do
    {
        memset(&messageInfo, 0, sizeof(messageInfo));

        messageInfo.mPeerPort = THREAD_UDP_PORT;
        messageInfo.mPeerAddr = dst_addr;

        otMessageGetBufferInfo(otGetInstance(), &bufInfo);
        if (bufInfo.mIp6Queue.mNumMessages > 2)
        {
            error = OT_ERROR_BUSY;
            break;
        }

        message = otUdpNewMessage(otGetInstance(), &messageSettings);
        if (message == NULL)
        {
            error = OT_ERROR_NO_BUFS;
            break;
        }
        error = otMessageAppend(message, data, data_lens);
        if (error != OT_ERROR_NONE)
        {
            break;
        }

        error = otUdpSend(otGetInstance(), &appSock, message, &messageInfo);
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
    return error;
}

uint8_t app_sock_init(otInstance *instance)
{
    otSockAddr sockAddr;
    uint8_t ret;

    memset(&appSock, 0, sizeof(otUdpSocket));
    memset(&sockAddr, 0, sizeof(otSockAddr));

    ret = otUdpOpen(instance, &appSock, app_udp_receive_handler, instance);

    if (OT_ERROR_NONE == ret)
    {
        sockAddr.mPort = THREAD_UDP_PORT;
        ret = otUdpBind(instance, &appSock, &sockAddr, OT_NETIF_THREAD);
        if (OT_ERROR_NONE == ret)
        {
            info("App UDP Port        : 0x%x \r\n", THREAD_UDP_PORT);
        }
    }

    return ret;
}