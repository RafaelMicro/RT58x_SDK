/**************************************************************************//**
 * @file     mac_frame_gen.c
 * @version
 * $Revision:
 * $Date:
 * @brief
 * @note
 * Copyright (C) 2019 Rafael Microelectronics Inc. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "mac_frame_gen.h"
#include "rfb_port.h"

extern rfb_t g_rfb;
/*!************************************************************************************************
 * \fn          uint8_t mac_genSecCtrl(MacHdr_t *hdr)
 * \brief
 *              MAC security control field generator
 * \return
 *              uint8_t
 *************************************************************************************************/

uint8_t mac_genSecCtrl(MacHdr_t *hdr)
{
    uint8_t tmp = 0;
    tmp |= (uint8_t)((hdr->macSecCtrl.keyIdMode              & 0x3) << MAC_KEYID_MODE_OFFSET);
    tmp |= (uint8_t)(hdr->macSecCtrl.secLevel                & 0x7);
    return tmp;

}

/*!************************************************************************************************
 * \fn          uint16_t mac_genFrmCtrl(MacHdr_t *hdr)
 * \brief
 *              MAC frame control field generator
 * \return
 *              uint8_t
 *************************************************************************************************/

uint16_t mac_genFrmCtrl(MacHdr_t *hdr)
{
    uint16_t tmp = 0;
    tmp |= (uint16_t)((hdr->srcAddr.mode                     & 0x3) << MAC_SRC_ADDR_MODE_OFFSET);
    tmp |= (uint16_t)((hdr->destAddr.mode                    & 0x3) << MAC_DEST_ADDR_MODE_OFFSET);
    tmp |= (uint16_t)((hdr->macFrmCtrl.panidCompr            & 0x1) << MAC_PAN_ID_COMPR_OFFSET);
    tmp |= (uint16_t)((hdr->macFrmCtrl.ackReq                & 0x1) << MAC_ACK_REQ_OFFSET);
    tmp |= (uint16_t)((hdr->macFrmCtrl.framePending          & 0x1) << MAC_FRM_PEND_OFFSET);
    tmp |= (uint16_t)((hdr->macFrmCtrl.secEnab               & 0x1) << MAC_SEC_ENAB_OFFSET);
    tmp |= (uint16_t)(hdr->macFrmCtrl.frameType              & 0x3);
    return tmp;
}


/*!************************************************************************************************
 * \fn          void mac_genHeader(MacBuffer_t *buf, MacHdr_t *hdr)
 * \brief
 *              MAC Header generator
 * \return
 *              uint8_t
 *************************************************************************************************/


void mac_genHeader(MacBuffer_t *buf, MacHdr_t *hdr)
{
    uint8_t  hdrSize;
    hdr->macFcf = mac_genFrmCtrl(hdr);

    // Calculate the size of the header
    hdrSize = 3;                                               // fcf (2) + dsn (1) = 3 bytes
    if (hdr->destAddr.mode > 0)
    {
        hdrSize += ((hdr->destAddr.mode == SHORT_ADDR) ? 4 : 10);     // destPanid (2) + destAddr (2/8) = 4/10 bytes
    }

    if (hdr->srcAddr.mode > 0)
    {
        hdrSize += (hdr->macFrmCtrl.panidCompr == TRUE) ?  0 : 2;  // if panidCompr, then we don't have srcPanid
        hdrSize += (hdr->srcAddr.mode == SHORT_ADDR) ? 2 : 8;
    }

    if (hdr->macFrmCtrl.secEnab > 0)
    {
        hdrSize += 5;  // Security Control(1) + Frame Counter (4)
        hdrSize += (((hdr->macSecCtrl.keyIdMode - 1) << 2) + 1);
    }
    buf->len += hdrSize;


    // Start filling in the frame header. Write in the data, then advance the data pointer.
    *(uint16_t *)buf->dptr         = hdr->macFcf;
    buf->dptr                 += sizeof(uint16_t);
    *(uint8_t *)buf->dptr          = hdr->dsn;
    buf->dptr ++;

    // pan_id and addresses
    if (hdr->destAddr.mode > 0)
    {
        // dst_pan_id
        *(uint16_t *)buf->dptr = hdr->destPanid;
        buf->dptr += sizeof(uint16_t);

        // dst_addr
        if (hdr->destAddr.mode == SHORT_ADDR)
        {
            *(uint16_t *)buf->dptr = hdr->destAddr.shortAddr;
            buf->dptr += sizeof(uint16_t);
        }
        else
        {

            *(uint32_t *)buf->dptr = hdr->destAddr.longAddr[0];
            buf->dptr += sizeof(uint32_t);
            *(uint32_t *)buf->dptr = hdr->destAddr.longAddr[1];
            buf->dptr += sizeof(uint32_t);

        }
    }


    if (hdr->srcAddr.mode > 0)
    {
        // srcPanid
        // if panidCompr is used, then we won't have a srcPanid
        if (!hdr->macFrmCtrl.panidCompr)
        {
            *(uint16_t *)buf->dptr = hdr->srcPanid;
            buf->dptr += sizeof(uint16_t);
        }

        // srcAddr
        if (hdr->srcAddr.mode == SHORT_ADDR)
        {
            *(uint16_t *)buf->dptr = hdr->srcAddr.shortAddr;
            buf->dptr += sizeof(uint16_t);
        }
        else
        {

            *(uint32_t *)buf->dptr = hdr->srcAddr.longAddr[0];
            buf->dptr += sizeof(uint32_t);
            *(uint32_t *)buf->dptr = hdr->srcAddr.longAddr[1];
            buf->dptr += sizeof(uint32_t);

        }
    }
    if (hdr->macFrmCtrl.secEnab > 0)
    {

        hdr->macSef = mac_genSecCtrl(hdr);
        *(uint8_t *)buf->dptr        = hdr->macSef;
        buf->dptr ++;

        *(uint32_t *)buf->dptr = hdr->frameCounter;
        buf->dptr += sizeof(uint32_t);

        switch (hdr->macSecCtrl.keyIdMode)
        {
        case KEY_ID_MODE_1:
            *(uint8_t *)buf->dptr = hdr->keyIdentifier.keySource1;
            buf->dptr ++;
            break;
        case KEY_ID_MODE_4:
            *(uint32_t *)buf->dptr = hdr->keyIdentifier.keySource4;
            buf->dptr += sizeof(uint32_t);
            *(uint8_t *)buf->dptr = hdr->keyIdentifier.keyIndex;
            buf->dptr ++;
            break;
        case KEY_ID_MODE_8:
            *(uint32_t *)buf->dptr = hdr->keyIdentifier.keySource8[0];
            buf->dptr += sizeof(uint32_t);
            *(uint32_t *)buf->dptr = hdr->keyIdentifier.keySource8[1];
            buf->dptr += sizeof(uint32_t);
            break;
        default:
            break;
        }

    }

    // Move the data pointer to the first byte of this frame
    buf->dptr -= hdrSize;
}


/*!************************************************************************************************
 * \fn          void mac_genAck(MacBuffer_t *buf, MacHdr_t *hdr)
 * \brief
 *              MAC ACK generator
 * \return
 *              uint8_t
 *************************************************************************************************/

void mac_genAck(MacBuffer_t *pbuf, bool framePending, uint8_t dsn)
{
    uint16_t fcf = (framePending) ? 0x0012 : 0x0002;

    *(uint16_t *)pbuf->dptr         = fcf;
    *(uint8_t *)(pbuf->dptr + 2)          = dsn;
    pbuf->len = 3; // FCF(2)+DSN(1) The size of ACK is fixed.
}


void Rfb_MacFrameGen(MacBuffer_t *pbuf, uint8_t *tx_control, uint8_t Dsn, uint16_t MacDataLength)
{
    uint16_t idx;
    uint16_t payloadLength = MacDataLength;
    uint8_t initialCW;
    uint8_t ackRequest;
    uint16_t max_data_size = ((g_rfb.modem_type == RFB_MODEM_FSK) ? FSK_MAX_DATA_SIZE : OQPSK_MAX_DATA_SIZE);

    MacHdr_t MacHdr;
    memset(&pbuf->buf[0], 0x0, max_data_size);
    pbuf->dptr = &pbuf->buf[0];
    pbuf->len = 0;

    MacHdr.macFrmCtrl.secEnab         = false;
    MacHdr.macSecCtrl.secLevel        = SEC_LEVEL_NONE;
    MacHdr.macSecCtrl.keyIdMode       = KEY_ID_MODE_IMPLICITY;


    MacHdr.destPanid                   = 0x1AAA;

    MacHdr.destAddr.mode               = SHORT_ADDR; //LONG_ADDR;
    MacHdr.destAddr.shortAddr          = 0x1234;
    MacHdr.destAddr.longAddr[0]        = 0x11223333;
    MacHdr.destAddr.longAddr[1]        = 0x55667788;

    MacHdr.macFrmCtrl.panidCompr       = true;

    MacHdr.srcPanid                    = 0x1aaa;

    MacHdr.srcAddr.mode                = SHORT_ADDR; //LONG_ADDR;
    MacHdr.srcAddr.shortAddr           = 0x1111;
    MacHdr.srcAddr.longAddr[0]        = 0x00000001;
    MacHdr.srcAddr.longAddr[1]        = 0xacde4800;

    MacHdr.macFrmCtrl.framePending     = true;
    MacHdr.macFrmCtrl.frameType        = MAC_COMMAND;

    MacHdr.macFrmCtrl.ackReq           = true;


    MacHdr.dsn = Dsn;
    initialCW = DIRECT_TRANSMISSION;
    ackRequest = MacHdr.macFrmCtrl.ackReq;
    *tx_control = (((initialCW << 1) & 0x02) | ((ackRequest) & 0x01));


    switch (MacHdr.macFrmCtrl.frameType)
    {
    case MAC_COMMAND:
    case MAC_BEACON:
    case MAC_DATA:
        mac_genHeader(pbuf, &MacHdr);
        break;
    case MAC_ACK:
        mac_genAck(pbuf, MacHdr.macFrmCtrl.framePending, MacHdr.dsn);
        break;
    default:
        break;
    }

    // add payload
    for (idx = pbuf->len; idx < (payloadLength + pbuf->len); idx++)
    {
        pbuf->buf[idx] = idx;
    }
    if ((pbuf->len + payloadLength) > max_data_size)
    {
        pbuf->len = max_data_size;
    }
    else
    {
        pbuf->len += payloadLength;
    }
}

