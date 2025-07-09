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
#define  GD25WQ80C_ID          0x1465C8

/************************************************************/
/*
#define flash_write_protect
 */

#define GPIO28  28
#define GPIO29  29

#define page_writesize                (256)       /*For GD flash, the minimum write sector is 256 bytes*/
#define erase_sector               (4*1024)       /*For GD flash, the minimum erase sector is 4K*/

/* The following timeout tick is very large, but this is for
 * multiple threads consider...If some task has high priority, the timeout
 * could be large...
 */
#define    read_tick                  50000
#define    write_page_tick           100000       /*program page typical 0.7ms */

#define    erase_sector_tick         500000       /*erase sector us 40ms typical*/
#define    erase_block_tick         1000000       /*erase block typical 0.18s*/

#define    erase_chip_tick          3000000       /*If erase whole chip, */


#define  GD_ID       0xC8
#define  WB_ID       0xEF

#define  FLASH_SIZE_BIT_OFFSET      16
#define  BYTE_MASK                  0xFF

#define  MAGIC_TEST_PATTERN         0x5AA5FF00

#define  FLASH_ID_NUMBER            16

#define  FLASH_INIT_PATTERN         0xFFFFFFFF

#define  BLOCK_64KB                 (64*1024)


/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
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
#define TIMER0_ID           0

void init_timer(void)
{
    timer_config_mode_t cfg;

    cfg.mode = TIMER_FREERUN_MODE;
    /*the input clock is 32M/s, so it will become 1M ticks per second */
    cfg.prescale = TIMER_PRESCALE_32;

    Timer_Open(TIMER0_ID, cfg, NULL);
}

uint32_t wait_flash_finish_or_timeout(uint32_t us_tick)
{
    /* In fact, timeout is not necessary.
     * So you can wait until flash_check_busy to 0
     * without timeout... If flash_check_busy is always 1,
     * it means you can not run any code correct....
     *
     */
    Timer_Start(TIMER0_ID, us_tick);

    do
    {
        if (flash_check_busy() == FLASH_IDLE)
        {
            /*flash done*/
            Timer_Stop(TIMER0_ID);
            return STATUS_SUCCESS;
        }
    } while (Timer_Current_Get(TIMER0_ID) <= us_tick);

    /* It is almost IMPOSSIBLE to running here...if code go to here
     * it means  1. tick is too small  or 2. check your timer setting.
     * 3. Flash broken
     */
    printf("Error: you should never see flash timeout\n");
    Timer_Stop(TIMER0_ID);
    return STATUS_TIMEOUT;
}

int main(void)
{
    uint32_t  flash_model;
    uint32_t  operation_base_addr;

    /*
     *   Allocate a large buffer in stack is not a good idea,
     * so we add "static" to declare this buffer as "local global" buffer,
     * not in stack.
     *
     */
    static uint8_t   temp_buffer[page_writesize];

    uint32_t  buf_addr = (uint32_t)  temp_buffer;
    uint32_t  *read_ptr;

    uint32_t  flash_size;        /*This is the flash size*/


    flash_status_t  flash_status;

    uint8_t     flash_id[FLASH_ID_NUMBER];

    int i;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();
    init_timer();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("\nHello World, Flash TEST\n");

    printf("This flash is internal Flash running code and saving data storage chip.\n");
    printf("This flash is not QSPI external flash \n");

    /*Here we default read page is 256 bytes.*/
    flash_set_read_pagesize();

    flash_model = flash_get_deviceinfo();

    printf("Manuf: ");

    switch (flash_model & BYTE_MASK)
    {

    case  GD_ID:
        printf("GD\n");
        break;

    case  WB_ID:
        printf("WinBond\n");
        break;

    default:
        printf("Unknow\n");
    }

    flash_size = (1 << ((flash_model >> FLASH_SIZE_BIT_OFFSET)&BYTE_MASK));
    printf("Size: %ld KB\n", (flash_size >> 10));


    /*
     * get flash id, each GD flash has one unique 128bits id.
     * You can use this id to become some software random number seed.
     */

    printf("Each internal flash has one unique flash ID number. \n");
    printf("You can use this unique number as random seed or something \n");

    flash_get_unique_id( (uint32_t) flash_id, FLASH_ID_NUMBER);

    printf("Flash ID: ");
    for (i = 0; i < 16; i++)
    {
        printf("%02x ", flash_id[i] );
    }
    printf("\n\n");

    flash_status.require_mode = (FLASH_STATUS_RW1);
    flash_get_status_reg(&flash_status);

    printf("Get Flash status byte1: 0x%2X \n", flash_status.status1);

#ifdef flash_write_protect
    /*
     * set status1 0x68, protect the first 8K bytes --
     * This setting is based GD25LQ80
     * different flash has different value setting! Please refer related document
     */
    flash_status.require_mode = (FLASH_STATUS_RW1);
    flash_status.status1 = 0x68;              /*depends on flash, please check flash document for correct setting.*/
    flash_set_status_reg(&flash_status);
#endif

    /*   This example will erase/write/read the last erase sector of the (N-2)th block.
     *   Because the last block block has some IC hardware setting information sector, it should be keep.
     *   It erases the last sector of "the second last" block.
     *
     * Note: if you want to use CPU to direct read flash memory,
     * you can use the address in (0x90000000 + offset), or hardward remap address (0x00000000 + offset)
     * Here we use address  (0x00000000 + offset).
     */

    operation_base_addr =  (flash_size - erase_sector - BLOCK_64KB);
    flash_erase(FLASH_ERASE_SECTOR, operation_base_addr);

    printf("erase the 4K page  in address 0x%08lX \n", operation_base_addr);

    /*we use polling to check busy state.. */
    wait_flash_finish_or_timeout(erase_sector_tick);


    printf("read the 4K data in address 0x%08lX \n", operation_base_addr);
    printf("those data should be all 0xFF \n");

    /*  this read is using CPU acess flash address */
    /*  using CPU read is very slow,  compared with flash read API.
     *  And using CPU read need to clean cache first...
     *  so please AVOID to using CPU access flash data....
     *  In fact, we suggest to use flash control API to read data in flash...
     *  not directly using CPU mapping. Only using CPU read for few data (not more than 256 bytes)!
     *  Here is just an example to show how to use CPU access flash.
     */
    read_ptr = (uint32_t *)operation_base_addr;

    /*
     * Remark: For CPU read, it is possible that some old data still in cache
     * For safety, we need clear cache before using CPU read.
     */
    flush_cache();          /*ONLY call this API after erase or write API... do NOT call it in normal case!*/

    /*one erase sector minimum size is 4K byte, we use 4 bytes(32bits) read */
    for (i = 0; i < (erase_sector >> 2); i++)
    {
        if (read_ptr[i] != FLASH_INIT_PATTERN)
        {
            /*OOPS... error?!*/
            printf("Sector erase error?\n");
            goto main_end;
        }
    }
    printf("Sector erase success\n");

    read_ptr = (uint32_t *) temp_buffer;

    /*generate test pattern to write*/
    for (i = 0; i < (page_writesize >> 2); i++)
    {
        read_ptr[i] = MAGIC_TEST_PATTERN + i;
    }

    printf("write the one page (256 bytes) data in address 0x%08lX \n", operation_base_addr);

    flash_write_page(buf_addr, operation_base_addr);        /*write must use flash controller*/
    wait_flash_finish_or_timeout(write_page_tick);

    printf("Page write success\n");

    for (i = 0; i < (page_writesize >> 2); i++)
    {
        read_ptr[i] = FLASH_INIT_PATTERN;           /*initial read_ptr to default 0xFF value.*/
    }

    printf("Verify write data \n");

    /*check verify what we write*/
    flash_read_page(buf_addr, operation_base_addr);     /*This read is using flash controller read*/
    wait_flash_finish_or_timeout(read_tick);

    for (i = 0; i < (page_writesize >> 2); i++)
    {
        if (read_ptr[i] != (MAGIC_TEST_PATTERN + i))
        {
            /*OOPS... error?!*/
            printf("Page verify error\n");
            goto main_end;
        }
    }
    printf("Page verify success\n");

    printf("flash demo finish \n");

main_end:

#ifdef flash_write_protect
    /*
     * set status1 0x2C, protect the 0~256KB bytes
     * This setting is based GD25LQ80
     */
    flash_status.require_mode = (FLASH_STATUS_RW1);
    flash_status.status1 = 0x2C;
    flash_set_status_reg(&flash_status);
#endif


    while (1)
    {

    }
}
/** @} */ /* end of examples group */
