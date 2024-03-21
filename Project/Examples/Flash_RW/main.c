/** @file main.c
 *
 * @brief Flash example main file.
 *
 *
 */
/**
* @defgroup Flash_example_group  Flash
* @ingroup examples_group
* @{
* @brief Flash example demonstrate
*/
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include "cm3_mcu.h"
#include "project_config.h"

#include "uart_drv.h"
#include "retarget.h"
#include "rf_mcu_ahb.h"


#define PRINTF_BAUDRATE      UART_BAUDRATE_115200
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/************************************************************/
uint32_t Flash_Address = 0;
/*this is pin mux setting*/
void init_default_pin_mux(void)
{
    uint32_t i;

    for (i = 0; i < 32; i++)
    {
        pin_set_mode(i, MODE_GPIO);     /*GPIO16 as UART0 TX*/
    }

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


uint32_t flash_size_check()
{
    uint32_t status = STATUS_SUCCESS;

    if (flash_size() == FLASH_512K)
    {
        Flash_Address =     0x3C000;
        printf("Flash Size 512K \r\n");
    }
    else if (flash_size() == FLASH_1024K)
    {
        Flash_Address =     0x7C000;
        printf("Flash Size 1MB\r\n");
    }
    else if (flash_size() == FLASH_2048K)
    {
        Flash_Address =     0xFC000;
        printf("Flash Size 2MB\r\n");
    }
    else
    {
        status = STATUS_ERROR;
    }

    return status;
}

int main(void)
{
    uint32_t i = 0;
    uint8_t     WbufAry[256], RbufAry[256];

    init_default_pin_mux();
    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//No use Commincution SubSystem FW, need to call the function.

    printf("Flash Read, Write, Erase, Demo Build %s %s\r\n", __DATE__, __TIME__);

    //Initinal Buf
    for (i = 0; i < 256; i++)
    {
        WbufAry[i] = (i + 1);
        RbufAry[i] = 0;
    }

    if (flash_size_check() != STATUS_SUCCESS)
    {
        printf("Unkonw Flash Size\r\n");
        while (1);
    }


    //Sector Erase e 4K
    if (flash_erase(FLASH_ERASE_SECTOR, Flash_Address) != STATUS_SUCCESS)
    {
        printf("Flash Sector Erase %8x Fail \r\n", Flash_Address);
    }
    else
    {
        printf("Flash Sector Erase Finish \r\n");
    }



    //Write Page (256 Bytes)
    if (flash_write_page((uint32_t)WbufAry, Flash_Address) != STATUS_SUCCESS)
    {
        printf("Flash Write Page Fail \r\n");
    }
    else
    {
        printf("Flash Write Page Finish \r\n");
    }


    //Read Page (256 Bytes)
    if (flash_read_page_syncmode((uint32_t)RbufAry, Flash_Address) != STATUS_SUCCESS)
    {
        printf("Flash Read Page Fail \r\n");
    }
    else
    {
        printf("Flash Read Page Finish \r\n");
    }

    //Compare Read/Wirte Data
    for (i = 0; i < 256; i++)
    {
        if (WbufAry[i] != RbufAry[i])
        {
            printf("Flash Address %8x Error \r\n", (Flash_Address + i));
        }
    }

    printf("Flash Demo End \r\n");


    while (1) {;}
}
/** @} */ /* end of examples group */
