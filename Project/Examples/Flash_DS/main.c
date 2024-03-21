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
/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/************************************************************/

/*this is pin mux setting*/
void init_default_pin_mux(void)
{
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
    ds_config_t ds_init_cfg;
    uint32_t ds_ret = 0;
    uint8_t rd_buf[0xFF], wr_buf[0xFF];
    int i, j, ds_count = 0xFF;
    ds_rw_t t_ds_r, t_ds_w;

    /*we should set pinmux here or in SystemInit */
    init_default_pin_mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("Flash DataSet Demo build %s %s\r\n", __DATE__, __TIME__);


    for (i = 0x84000; i < 0xA4000; i += 0x1000)
    {
        flash_erase(FLASH_ERASE_SECTOR, i);
        flush_cache();
    }

    //config record flash address range
    ds_init_cfg.start_address = 0x84000;
    ds_init_cfg.end_address = 0xA4000;
    ds_ret = ds_initinal(ds_init_cfg);

    if (ds_ret != STATUS_SUCCESS)
    {
        printf("ds_initial fail %d\r\n", ds_ret);
        while (1);
    }

    //write data buffer init
    for (i = 0; i < ds_count; i++)
    {
        wr_buf[i] = i;
    }

    //DS_TABLE_TYPE_MAX default is 255, that can adjust the define value. in flashds.h
    for (i = 1; i < ds_count; i++)
    {

        t_ds_w.type = i;
        t_ds_w.len = i;
        t_ds_w.address = (uint32_t)wr_buf;

        ds_ret = ds_write(&t_ds_w);

        printf("type %d len %d address %x\r\n", t_ds_w.type, t_ds_w.len, t_ds_w.address);
        if (ds_ret != STATUS_SUCCESS)
        {
            printf("ds_write failed %d\r\n", ds_ret);
        }
        else
        {
            printf("ds type %d write done!\r\n", i);
        }

    }

    //read data buffer
    for (i = 1; i < ds_count; i++)
    {
        t_ds_r.type = i;
        t_ds_r.len = 0;
        t_ds_r.address = 0;
        ds_ret = ds_read(&t_ds_r);

        if (ds_ret != STATUS_SUCCESS)
        {
            printf("ds_read failed %d\r\n", ds_ret);
        }

        if (t_ds_r.address != 0 && t_ds_r.len != 0)
        {
            for (j = 0; j < t_ds_r.len; j++)
            {
                rd_buf[j] = flash_read_byte(t_ds_r.address + j);
            }

            if (memcmp(rd_buf, wr_buf, t_ds_r.len) != 0)
            {
                printf("ds %d verifiy fail!\r\n", i);
            }
            else
            {
                printf("ds read type %d success!\r\n", i);
            }
        }
        else
        {
            printf("ds type %d not found!\r\n", i);
        }

    }

    //delete type
    for (i = 1; i < ds_count; i++)
    {
        ds_ret = ds_delete_type(i);

        if (ds_ret != STATUS_SUCCESS)
        {
            printf("ds_delete_type fail %d\r\n", ds_ret);
        }
        else
        {
            printf("ds delete type %d success!\r\n", i);
        }
    }

    while (1)
    {;}
}
/** @} */ /* end of examples group */
