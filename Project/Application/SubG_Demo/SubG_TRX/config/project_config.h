/**************************************************************************//**
 * @file     project_config.h
 * @version
 * @brief   define project config
 *
 * @copyright
 ******************************************************************************/

#include "chip_define.h"


#ifndef ___PROJECT_CONFIG_H__
#define ___PROJECT_CONFIG_H__


#define MODULE_ENABLE(module)     (module > 0)

/*
 * If system support multi-tasking, some hardware need mutex to protect
 */
#define SUPPORT_MULTITASKING               0

/* This define is temp used, it will be removed in production IC finially.
 * in production IC, the uart will use native DMA.
 * For shutter ASIC, it should define UART_USE_PDMA    1
 * For product ASIC in future, it should remove the following define.
 */
#if (CHIP_VERSION == RT58X_SHUTTLE)
#define UART_USE_PDMA                      1
#else
#define UART_USE_PDMA                      0
#endif

/*System use UART0 */
#define SUPPORT_UART0                      1

/*System use UART1 */
#define SUPPORT_UART1                      1

#define SUPPORT_UART1_TX_DMA               1

#define SUPPORT_UART1_RX_DMA               1

/*enable this option, UART1 will support CTS/RTS pin for flow control*/
#define SUPPORT_UART1_FLOWCNTL             0

/*System use UART2 */
#define SUPPORT_UART2                      1

#define SUPPORT_UART2_TX_DMA               1

#define SUPPORT_UART2_RX_DMA               1


#define SUPPORT_QSPI_DMA                   1

/*Support AES  */
#define CRYPTO_AES_ENABLE                  1

/*Support ECC SECP192R1/P192 curve */
#define CRYPTO_SECP192R1_ENABLE            1

/*Support ECC SECP256R1/P256 curve */
#define CRYPTO_SECP256R1_ENABLE            1

/*Support ECC SECT163R2/B163 curve */
#define CRYPTO_SECT163R2_ENABLE            1

#define SUPPORT_FREERTOS_PORT              1
#define SUPPORT_FREERTOS_SLEEP             0
#define SUPPORT_FREERTOS_NEST_CRITICAL     0
#define RFB_SUBG_ENABLED                   TRUE
#define RFB_15p4_MAC_ENABLED               TRUE
#define RFB_FIX_TX_POWER_SUPPORT           TRUE

#endif

//==========================================================
// SubG Test Plan
//==========================================================
#define SUBG_TEST_PLAN_BER                 FALSE

//==========================================================
// <h> CLI Console
//==========================================================
#define USE_BSP_UART_DRV                   1
#define DEBUG_CONSOLE_UART_ID              0
#define SUPPORT_SHELL_CMD_HISTORY          1

#if (defined SUBG_TEST_PLAN_BER && SUBG_TEST_PLAN_BER == TRUE)
#define SUPPORT_DEBUG_CONSOLE_CLI          1
#endif

#ifndef SUPPORT_DEBUG_CONSOLE_CLI
#define SUPPORT_DEBUG_CONSOLE_CLI          0
#endif
