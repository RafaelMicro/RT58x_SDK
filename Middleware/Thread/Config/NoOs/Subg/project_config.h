/**************************************************************************//**
 * @file     project_config.h
 * @version
 * @brief   define project configuration
 *
 * @copyright
 ******************************************************************************/

#include "chip_define.h"
#include "cm3_mcu.h"

#define MODULE_ENABLE(module) (module > 0)

/*System use UART0 */
#define SUPPORT_UART0                      1

/*System use UART1 */
#define SUPPORT_UART1                      1
#define SUPPORT_UART1_TX_DMA               1
#define SUPPORT_UART1_RX_DMA               1

#define SUPPORT_QSPI_DMA                   1

/*Support AES  */
#define CRYPTO_AES_ENABLE                  0

#define USE_BSP_UART_DRV                    1
#define DEBUG_CONSOLE_UART_ID               0

/*Support low voltage protection*/
#define LPWR_FLASH_PROTECT_ENABLE 1
#define LPWR_FLASH_CMP_PROTECT_ENABLE 1

#define SET_SYS_CLK    SYS_CLK_64MHZ
#define RF_FW_INCLUDE_PCI           (TRUE)
#define RF_FW_INCLUDE_BLE           (FALSE)
#define RF_FW_INCLUDE_MULTI_2P4G    (FALSE)

#define PLAFFORM_CONFIG_ENABLE_SUBG (TRUE)

#if PLAFFORM_CONFIG_ENABLE_SUBG == TRUE
#define RFB_SUBG_ENABLED            (TRUE)
#else
#define RFB_ZIGBEE_ENABLED          (TRUE)
#endif

#if RFB_SUBG_ENABLED == TRUE
#define RFB_15p4_MAC_ENABLED         (TRUE)
#define RFB_FIX_TX_POWER_SUPPORT            1
#define RFB_SUBG_FREQUENCY_BAND      (0) //0: 915M, 1: 868M, 2: 433M, 3: 470M
#endif //RFB_SUBG_ENABLED

#define __reloc __attribute__ ((used, section("reloc_text")))