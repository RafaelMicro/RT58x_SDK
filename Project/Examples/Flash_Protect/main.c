/** @file main.c
 *
 * @brief flash protect example main file.
 *
 *
 */
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "project_config.h"
#include "retarget.h"
#include "comm_subsystem_drv.h"
#include "rf_mcu_ahb.h"
#include "flash_protect.h"



int main(void);

void SetClockFreq(void);

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200



/************************************************************/

/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    /*uart0 pinmux*/
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 TX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 RX*/

    return;
}
void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}

int main(void)
{

    uint8_t rbuf[256];
    uint32_t i;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();

    printf(" Comparator Protect Example Build %s %s\r\n", __DATE__, __TIME__);

#if (LPWR_FLASH_PROTECT_ENABLE==1)

#if  (LPWR_FLASH_CMP_PROTECT_ENABLE==1)
    //when vbat voltage low than 2.0v the flash_protect variable will be set true
    //the flash contrl can't operation read/write function.
    flash_cmp_protect_init();
#endif

#endif


    while (1)
    {
        flash_read_page_syncmode((uint32_t)rbuf, 0x9000);

        printf("Read Page\r\n");
        printf("\r\n==================================================\r\n");
        printf("   ");
        for (i = 0; i < 16; i++)
        {
            printf("%02X ", i);
        }
        printf("\r\n==================================================\r\n");

        for (i = 0; i < 256; i++)
        {
            if ((i % 16 == 0) && i != 0)
            {
                printf("\r\n");
            }
            if ((i % 16 == 0))
            {
                printf("%02X ", ((i / 16) << 4));
            }
            printf("%02X ", rbuf[i] );
        }
        printf("\r\n==================================================\r\n");
        printf("\r\n");

        memset(rbuf, 0xFF, sizeof(rbuf));
    }
}



/** @} */ /* end of examples group */
