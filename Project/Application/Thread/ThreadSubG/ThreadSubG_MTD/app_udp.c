/**
 * @file app_udp.c
 * @author
 * @brief
 * @version 0.1
 * @date 2023-10-06
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "main.h"
#include "thread_queue.h"
#include "sw_timer.h"

#define THREAD_UDP_PORT 5678

typedef struct
{
    otIp6Address src_ip;
    uint8_t buf[OPENTHREAD_CONFIG_IP6_MAX_DATAGRAM_LENGTH + 1];
    uint16_t lens;
} app_udp_msg_t;

static otUdpSocket          appSock;
static thread_queue_t app_udp_msg_queue;
static app_udp_msg_t app_udp_msg;
otIp6Address app_udp_last_src_ip;

otError app_udp_send(otIp6Address dst_addr, uint8_t *data, uint16_t data_lens)
{
    otError error = OT_ERROR_NONE;
    otMessage *message = NULL;
    otMessageInfo messageInfo;
    otBufferInfo bufInfo;
    otMessageSettings messageSettings = {true, OT_MESSAGE_PRIORITY_NORMAL};

    do
    {
#if 0 //debug use
        char string[OT_IP6_ADDRESS_STRING_SIZE];
        otIp6AddressToString(&dst_addr, string, sizeof(string));
        info("ip : %s\n", string);
        info("Message send : \r\n");
        for (int i = 0 ; i < data_lens; i++)
        {
            info("%02x", data[i]);
        }
        info("\r\n");
#endif

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

static void app_udp_receive_handler(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    uint8_t *buf = NULL;
    uint8_t data_seq = 0, cmd = 0xFF, pid = 0xFF;

    int length;
    char string[OT_IP6_ADDRESS_STRING_SIZE];

    otIp6AddressToString(&aMessageInfo->mPeerAddr, string, sizeof(string));
    length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    buf = mem_malloc(length + 1);
    if (buf)
    {
        otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, length);
        buf[length] = '\0';
#if 0 //debug use
        info("%d bytes from \n", length);
        info("ip : %s\n", string);
        info("port : %d \n", aMessageInfo->mSockPort);
        info("Message Received : \r\n");
        for (int i = 0 ; i < length; i++)
        {
            info("%02x", buf[i]);
        }
        info("\r\n");
#endif
        memset((void *)&app_udp_msg, 0, sizeof(app_udp_msg_t));
        /*copy source ip*/
        memcpy((void *)&app_udp_msg.src_ip, (void *)&aMessageInfo->mPeerAddr, sizeof(otIp6Address));
        /*copy msg ip*/
        memcpy(app_udp_msg.buf, buf, length);
        /*msg lens*/
        app_udp_msg.lens = length;

        thread_enqueue(&app_udp_msg_queue, &app_udp_msg);

        mem_free(buf);
    } //if(buf)
}

void app_udp_message_queue_process()
{
    memset((void *)&app_udp_msg, 0, sizeof(app_udp_msg_t));

    if (!thread_dequeue(&app_udp_msg_queue, &app_udp_msg))
    {
        if (app_udp_comm_header_check(app_udp_msg.buf, app_udp_msg.lens) == true)
        {
            app_udp_comm_process(app_udp_msg.buf, app_udp_msg.lens, app_udp_msg.src_ip);
        }
        else
        {
            memcpy((void *)&app_udp_last_src_ip, (void *)&app_udp_msg.src_ip, sizeof(otIp6Address));
            app_udp_msg.buf[app_udp_msg.lens] = '\0';
            otCliInputLine((char * )&app_udp_msg.buf);
        }
    }
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
        ret = otUdpBind(instance, &appSock, &sockAddr, OT_NETIF_THREAD_HOST);
        if (OT_ERROR_NONE == ret)
        {
            info("App UDP Port        : 0x%x \r\n", THREAD_UDP_PORT);
        }
        /* Init rx done queue*/
        thread_queue_init(&app_udp_msg_queue, 5, sizeof(app_udp_msg_t));
    }

    return ret;
}