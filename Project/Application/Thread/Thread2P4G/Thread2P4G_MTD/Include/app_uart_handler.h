/**
 * @file uart_handler.h
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2022-03-24
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef __APP_UART_HANDLER_H__
#define __APP_UART_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
//                Include (Better to prevent)
//=============================================================================

//=============================================================================
//                Public Definitions of const value
//=============================================================================
#define UART_HANDLER_PARSER_CB_NUM  3
//=============================================================================
//                Public ENUM
//=============================================================================
typedef enum
{
    UART_DATA_VALID = 0,
    UART_DATA_VALID_CRC_OK,
    UART_DATA_INVALID,
    UART_DATA_CS_ERROR,
} uart_handler_data_sts_t;

typedef uart_handler_data_sts_t (*uart_parser_cb)(uint8_t *pBuf, uint16_t plen, uint16_t *datalen, uint16_t *offset);
typedef void (*uart_recv_cb)(uint8_t *pBuf, uint16_t plen);
//=============================================================================
//                Public Struct
//=============================================================================
typedef struct UART_HANDLER_PARM_T
{
    uart_parser_cb UartParserCB[UART_HANDLER_PARSER_CB_NUM];
    uart_recv_cb UartRecvCB[UART_HANDLER_PARSER_CB_NUM];
} uart_handler_parm_t;
//=============================================================================
//                Public Function Declaration
//=============================================================================

void app_uart_handler_init();
void app_uart_handler_recv();
void app_uart_handler_send(uint8_t *pdata, uint32_t len);

#ifdef __cplusplus
};
#endif
#endif /* __APP_UART_HANDLER_H__ */
