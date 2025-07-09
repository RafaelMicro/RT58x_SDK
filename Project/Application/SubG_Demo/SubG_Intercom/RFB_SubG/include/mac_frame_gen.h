/**************************************************************************//**
 * @file     mac_frame_gen.h
 * @version
 * $Revision:
 * $Date:
 * @brief
 * @note
 * Copyright (C) 2019 Rafael Microelectronics Inc. All rights reserved.
 *
 ******************************************************************************/
#ifndef _MAC_FRAME_GEN_H_
#define _MAC_FRAME_GEN_H_

#include "radio.h"

#define FSK_MAX_DATA_SIZE 2045
#define OQPSK_MAX_DATA_SIZE 125
#define MAX_DATA_SIZE FSK_MAX_DATA_SIZE

typedef enum _transmission_algorithm_e
{
    DIRECT_TRANSMISSION                   = 0x00,
    NONBEACON_MODE_CSMACA                 = 0x01,
    BEACON_MODE_CSMACA                    = 0x02
} TransmissionAlgorithm_e;
typedef enum _mac_sec_level_e
{
    // security levels enums
    SEC_LEVEL_NONE = 0,      // No sec is used
    SEC_LEVEL_MIC32,         // Authentication MIC32 is used.
    SEC_LEVEL_MIC64,         // Authentication MIC64 is used.
    SEC_LEVEL_NMIC128,       // Authentication MIC128 is used.
    SEC_LEVEL_ENC,           // AES encryption is used.
    SEC_LEVEL_ENC32,         // AES encryption and authentication MIC32 are used.
    SEC_LEVEL_ENC64,         // AES encryption and authentication MIC64 are used.
    SEC_LEVEL_ENC128         // AES encryption and authentication MIC128 are used.
} MacSecLevel_e;

typedef enum _mac_keyid_mode_e
{
    KEY_ID_MODE_IMPLICITY = 0, // key is determined implicity
    KEY_ID_MODE_1,            // key is determined from the Key Index field in conjunction with macDefault-KeySource.
    KEY_ID_MODE_4,            // key is determined explicitly from the 4-octet Key Source field and the Key Index field.
    KEY_ID_MODE_8             // key is determined explicitly from the 9-octet Key Source field and the Key Index field.
} MacKeyidMode_e;

typedef enum _mac_frm_ctrl_e
{
    // MAC frame control field bit offset
    MAC_SEC_ENAB_OFFSET                 = 3,     // Security Enabled flag bit offset
    MAC_FRM_PEND_OFFSET                 = 4,     // Data pending flag bit offset
    MAC_ACK_REQ_OFFSET                  = 5,     // Ack request flag bit offset
    MAC_PAN_ID_COMPR_OFFSET             = 6,     // Pan ID compression bit offset
    MAC_DEST_ADDR_MODE_OFFSET           = 10,    // Destination address mode bit offset
    MAC_SRC_ADDR_MODE_OFFSET            = 14,    // Source address bit offset

    // MAC frame type
    MAC_BEACON                          = 0,      // Beacon
    MAC_DATA                            = 1,      // Data
    MAC_ACK                             = 2,      // ACK
    MAC_COMMAND                         = 3,      // Command

    // Address mode
    NO_PAN_ID_ADDR                      = 0,      // No address
    SHORT_ADDR                          = 2,      // Short address
    LONG_ADDR                           = 3,      // Long address

    // MAC security control field bit offset
    MAC_KEYID_MODE_OFFSET               = 3      // Key identifier mode offset in SCF
} MacFrmCtrl_e;

typedef enum
{
    ENERGY_DETECTION                       = 0,
    CHARRIER_SENSING                       = 1,
    ENERGY_DETECTION_AND_CHARRIER_SENSING  = 2,
    ENERGY_DETECTION_OR_CHARRIER_SENSING   = 3
} cca_mode_t;


typedef struct _mac_sec_ctrl_t
{
    uint8_t      secLevel;        // Security Level
    uint8_t      keyIdMode;       // Key Identifier Mode
} MacSecCtrl_t;

typedef struct _mac_frm_ctrl_t
{
    uint8_t frameType;            // Frame type
    bool    secEnab;              // Security enable
    bool    framePending;         // Pending frame
    bool    ackReq;               // ACK requested
    bool    panidCompr;           // PAN ID compression
    uint8_t frameVer;             // Frame version
} MacFrmCtrl_t;

typedef struct _mac_addr_t
{
    uint8_t mode;          // Address mode: short or long
    uint16_t shortAddr;    // Short address field
    uint32_t longAddr[2]; // Long address field

} MacAddr_t;

typedef struct _mac_keyid_t
{
    uint8_t  keyIndex;       // Key Index
    uint8_t  keySource1;     // Key Source: 1byte
    uint32_t keySource4;     // Key Source: 4 bytes
    uint32_t  keySource8[2]; // Key Source: 8 bytes

} MacKeyid_t;

typedef struct _mac_buffer_t
{
    uint8_t  *dptr;                          // Data pointer
    uint16_t len;                            // Data length
    uint8_t  buf[MAX_DATA_SIZE];      // Data buffer
} MacBuffer_t;

typedef struct _mac_hdr_t
{
    MacSecCtrl_t    macSecCtrl;     // Security control field structure
    MacFrmCtrl_t    macFrmCtrl;     // Frame control field structure
    uint16_t        macFcf;         // Frame control field hex value (value after bit fields are assembled)
    uint8_t         dsn;            // Data sequence number
    uint16_t        destPanid;      // Destination network ID
    MacAddr_t       destAddr;       // Destination address
    uint16_t        srcPanid;       // Source network ID
    MacAddr_t       srcAddr;        // Source address
    uint8_t         macSef;         // Security control field
    uint32_t        frameCounter;   // Frame counter
    MacKeyid_t      keyIdentifier;  // Key identifier
} MacHdr_t;

void Rfb_MacFrameGen(MacBuffer_t *pbuf, uint8_t *InitialCwAckRequest, uint8_t Dsn, uint16_t MacDataLength);
void mac_genAck(MacBuffer_t *pbuf, bool framePending, uint8_t dsn);
#ifdef RT569_P2P_Example
void Rfb_MacFrameGen_example(MacBuffer_t *pbuf, uint8_t *InitialCwAckRequest, uint8_t Dsn, uint8_t MacDataLength, uint16_t src_panid, uint16_t src_short_addr, uint16_t dst_panid, uint16_t dst_short_addr, uint16_t *data);
#endif
#endif
