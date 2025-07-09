/** @file main.c
 *
 * @brief DMA_Memory example main file.
 *
 *
 */
/**
* @defgroup DMA_Memory_example_group  DMA_Memory
* @ingroup examples_group
* @{
* @brief DMA_Memory example demonstrate
*/

#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"

#include "project_config.h"

#include "uart_drv.h"
#include "retarget.h"
#include "rf_mcu_ahb.h"
/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02

#define  PRINTF_BAUDRATE      UART_BAUDRATE_115200

#define  DMA_MEMTOMEM         0
#define  DMA_BLOCK_SIZE       2032

int main(void);

void SetClockFreq(void);


/************************************************************/
static volatile uint32_t     g_dma_finish_state = 0;

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
/*
 * This ISR is for dma transfer done
 *
 *
 */
static void dma_user_complete(uint32_t channel_id, uint32_t status)
{
    g_dma_finish_state = 1;
    return;
}

/*
 * We allocate "static" memory for DMA test..
 * the memory will always exist. (it will not use stack space.)
 *
 *   If using stack memory for DMA operation address. it is
 * possibile that data corrupted when function call returned (stack
 * data changed).
 *
 *   If you use stack memory for DMA operation, you should be careful
 * to design the memory of DMA without corrupted by other functions
 * when dma is in operation. Of course, you can declare a
 * static "local buffer" that will not use stack. But that buffer
 * can not share by other function!
 *
 */
static  uint8_t  dma_src_mem_buf[DMA_BLOCK_SIZE];
static  uint8_t  dma_dest_mem_buf[DMA_BLOCK_SIZE];
static  uint8_t  dma_dest_mem_buf2[DMA_BLOCK_SIZE];

int main(void)
{

    /* In this example, it use DMA0 Channel0 to DMA
     *
     * If you want to use another channel for DMA, please change
     * the define DMA_MEMTOMEM
     */
    uint32_t   dma_channel;
    uint32_t   temp, i, dma_status = 0;

    dma_config_t     dma_mode_config;



    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("Hello World, DMA MEMORY to MEMORY TEST \n");



    /*generated some test pattern for read/write*/
    dma_src_mem_buf[0] = 0x6D;
    dma_src_mem_buf[1] = 0x2F;

    for (i = 2; i < DMA_BLOCK_SIZE; i++)
    {
        temp = (dma_src_mem_buf[i - 2] * 13 + dma_src_mem_buf[i - 1] * 31 + 67);
        dma_src_mem_buf[i] =   temp % 251;
        dma_dest_mem_buf[i] = 0x0;
    }

    printf("using DMA interrupt mode: \n");

    dma_channel = DMA_MEMTOMEM;

    /*using DMA interrupt mode need register a callback for task finish */
    dma_register_isr(dma_channel, dma_user_complete);

    /*This is DMA interrupt mode.*/

    dma_mode_config.dma_control_setting = DMA_CNTL_SRC_INC |
                                          DMA_CNTL_SRC_WIDTH_32BIT |  DMA_CNTL_SRC_BURST4 | DMA_CNTL_SRC_PORT |
                                          DMA_CNTL_DEST_INC | DMA_CNTL_DEST_WIDTH_32BIT | DMA_CNTL_DEST_BURST4
                                          |  DMA_CNTL_DEST_PORT | DMA_CNTL_MODE_MEMTOMEM;

    dma_mode_config.dma_src_addr = (uint32_t) dma_src_mem_buf;
    dma_mode_config.dma_dest_addr = (uint32_t) dma_dest_mem_buf;
    dma_mode_config.dma_xfer_length = DMA_BLOCK_SIZE;
    dma_mode_config.dma_repeat_setting = 0;           /*We don't use DMA repeat mode in memory to memory*/
    dma_mode_config.dma_int_enable  = DMA_INT_ALL;    /*We use interrupt to notify Finish*/

    dma_config(dma_channel, &dma_mode_config);
    g_dma_finish_state = 0;

    dma_enable(dma_channel);

    while (g_dma_finish_state != 0)
    {
        /*
         * You can do other FSM job here....or just
         * busy waiting for CPU complete here.
         */

    }

    g_dma_finish_state = 0;

    /*simple compare data */

    for (i = 0; i < DMA_BLOCK_SIZE; i++)
    {
        if (dma_src_mem_buf[i] != dma_dest_mem_buf[i])
        {
            printf("Error \n");
            while (1);
        }

        dma_dest_mem_buf[i] = 0;    /*because next we want to use polling mode test*/
    }

    printf("DMA interrupt mode move successful! \n\n");

    printf("using DMA polling mode: \n");
    /*This is polling mode*/

    dma_mode_config.dma_control_setting = DMA_CNTL_SRC_INC |
                                          DMA_CNTL_SRC_WIDTH_32BIT |  DMA_CNTL_SRC_BURST4 | DMA_CNTL_SRC_PORT |
                                          DMA_CNTL_DEST_INC | DMA_CNTL_DEST_WIDTH_32BIT | DMA_CNTL_DEST_BURST4
                                          |  DMA_CNTL_DEST_PORT | DMA_CNTL_MODE_MEMTOMEM;

    dma_mode_config.dma_src_addr = (uint32_t) dma_src_mem_buf;
    dma_mode_config.dma_dest_addr = (uint32_t) dma_dest_mem_buf2;
    dma_mode_config.dma_xfer_length = DMA_BLOCK_SIZE;
    dma_mode_config.dma_repeat_setting = 0;           /*We don't use DMA repeat mode in memory to memory*/
    dma_mode_config.dma_int_enable  = 0;              /*polled mode.. not enable interrupt*/

    dma_config(dma_channel, &dma_mode_config);

    dma_enable(dma_channel);

    /*You should polled DMA finish state for dma_int_enable = 0 */
    while (1)
    {
        dma_get_status(dma_channel, &dma_status);
        if ( (dma_status & DMA_INT_XFER_DONE) == DMA_INT_XFER_DONE)
        {
            break;
        }
    }

    /* If we use polling mode, we should clear dma state
       by calling finish_dma(channel) */
    dma_finish(dma_channel);

    /*simple compare data */

    for (i = 0; i < DMA_BLOCK_SIZE; i++)
    {
        if ( dma_src_mem_buf[i] != dma_dest_mem_buf2[i])
        {
            printf("Error \n");
            while (1);
        }
    }

    printf("DMA polling mode move successful! \n\n");

    printf("DMA for memory to memory move success \n");

    while (1)
    {
        __WFI();
    }
}

void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
