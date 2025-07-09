/** @file main.c
 *
 * @brief DMA_LinkList example main file.
 *
 *
 */
/**
* @defgroup DMA_LinkList_example_group  DMA_LinkList
* @ingroup examples_group
* @{
* @brief DMA_LinkList example demonstrate
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
#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

#define DMA_BLOCK_SIZE         1024


/*
 * The more LINKLIST ITEM you use, the more LINKLIST "pointer entry memory" required...
 */
#define DMA_LINKLIST_ITEM         8          /*Here we assume there are 8 LINKLIST ITEM*/

#define TEST_DMA_CHANNEL          0          /*use DMA channel 0 for test*/


/*
 * We allocate "static" memory for DMA test..
 * the memory will always exist. (it will not use stack space.)
 *
 * It does not suggest to use stack memory for DMA operation address.
 * Because it is possibile for data corrupted when function call return...
 *
 */
static  uint8_t  dma_src_mem_buf[DMA_BLOCK_SIZE];

static  uint8_t  dma_dest_mem_buf[DMA_BLOCK_SIZE];

static volatile uint32_t     g_dma_finish_state = 0;

int main(void);

void SetClockFreq(void);


/************************************************************/

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
 */
static void dma_user_complete(uint32_t channel_id, uint32_t status)
{
    /*this ISR will not set dma_finish_state bit, if DMA_INT_LL_ELEMENT_Done..*/
    if (status & DMA_INT_XFER_DONE)
    {
        if (channel_id == TEST_DMA_CHANNEL)
        {
            g_dma_finish_state = 1;
        }
    }

    return;
}


void dma_memory_move(uint32_t channel_id, uint8_t *test_src_addr, uint8_t *test_dest_addr, uint32_t length)
{
    dma_config_t   dma_mode_config;
    uint32_t       dma_channel, control_reg;

    dma_channel = channel_id;

    /*default SRC 32bits DEST 32Bits and Burst8*/
    control_reg = (DMA_CNTL_SRC_WIDTH_32BIT | DMA_CNTL_DEST_WIDTH_32BIT |
                   DMA_CNTL_SRC_BURST8 | DMA_CNTL_DEST_BURST8) ;

    dma_mode_config.dma_control_setting = control_reg |
                                          DMA_CNTL_SRC_INC  | DMA_CNTL_SRC_PORT |
                                          DMA_CNTL_DEST_INC | DMA_CNTL_DEST_PORT |
                                          DMA_CNTL_MODE_MEMTOMEM;

    dma_mode_config.dma_src_addr = (uint32_t) test_src_addr;
    dma_mode_config.dma_dest_addr = (uint32_t) test_dest_addr;
    dma_mode_config.dma_xfer_length = length;
    dma_mode_config.dma_repeat_setting = 0;         /*We don't use DMA repeat mode in memory to memory*/
    dma_mode_config.dma_int_enable = DMA_INT_ALL;   /*We use interrupt to notify DMA job finish.*/

    dma_config(dma_channel, &dma_mode_config);

    dma_enable(dma_channel);

    /*NOTICE: we don't wait DMA finish here.*/

    return;
}

uint32_t mem_cmp(uint8_t channel, uint8_t *test_src_addr, uint8_t *test_dest_addr, uint32_t length)
{
    uint32_t  i;
    uint8_t   *src8_addr, *dest8_addr;

    src8_addr = (uint8_t *) test_src_addr;
    dest8_addr = (uint8_t *) test_dest_addr;

    for (i = 0; i < length ; i++)
    {
        if (src8_addr[i] != dest8_addr[i])
        {
            printf("Error index: %ld channel: %d\n", i, channel);
            printf("compare error  src: %lx  dest: %lx \n", (uint32_t)(src8_addr + i), (uint32_t)(dest8_addr + i));
            return STATUS_ERROR;
        }
    }

    return STATUS_SUCCESS;
}

#define   DMA_LL_ITEM0_SIZE           6
#define   DMA_LL_ITEM1_SIZE           6
#define   DMA_LL_ITEM2_SIZE           2
#define   DMA_LL_ITEM3_SIZE         256

#define   DMA_TEST_ITEM               4

int main(void)
{
    uint32_t             i, ret_status, dest_offset;

    /*dma_linklist_request_t used for software to describe memory relation*/
    dma_linklist_request_t   dma_link_request[DMA_LINKLIST_ITEM];

    /*dma_linklist_entry_t is used by hardware to describe memory relation*/
    dma_linklist_entry_t     dma_link_list[DMA_LINKLIST_ITEM];

    uint32_t             dma_control_mode;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("Hello World, DMA memory Link List TEST \n");

    /* generated some test pattern for read/write
     * so dma_src_mem_buf will be 0 1 2 3 .... 254 255 0 1 2 ... 255
     */
    for (i = 0; i < DMA_BLOCK_SIZE; i++)
    {
        dma_src_mem_buf[i] = (i & 0xFF);        /*because dma_src_mem_buf[i] is uint8_t */
    }

    memset(dma_dest_mem_buf, 0, sizeof(dma_dest_mem_buf));

    dma_register_isr(TEST_DMA_CHANNEL, dma_user_complete);     /*register callback notify for channel TEST_DMA_CHANNEL*/

    g_dma_finish_state = 0;

    dma_memory_move(TEST_DMA_CHANNEL, dma_src_mem_buf, dma_dest_mem_buf, DMA_BLOCK_SIZE);

    /*wait memory move complete. */
    while (g_dma_finish_state != 0)
    {
        /*busy wait DMA finish...if you use RTOS, you can use event wait function here. */

    }

    /*compare data*/
    ret_status = mem_cmp(TEST_DMA_CHANNEL, dma_src_mem_buf, dma_dest_mem_buf, DMA_BLOCK_SIZE);

    if (ret_status == STATUS_ERROR)
    {
        while (1);
    }

    /*create linklist for memory move.*/
    /*
     * In this example, we will move
     *
     *   dma_src_mem_buf[4] ~ dma_src_mem_buf[9]     6 bytes
     *   dma_src_mem_buf[24] ~ dma_src_mem_buf[29]   6 bytes
     *   dma_src_mem_buf[38] ~ dma_src_mem_buf[39]   2 bytes
     *   dma_src_mem_buf[62] ~ dma_src_mem_buf[317]   256 bytes
     *   to
     *   dma_dest_mem_buf[0] ~ dma_dest_mem_buf[269]
     *
     *   So after the dma linklist move finish, the dma_dest_mem_buf will become:
     *    4 5 6 7 8 9 24 25 26 27 28 29 38 39 62 63 64 .....  0xFE 0xFF 0 ... 61
     */

    dest_offset = 0;

    memset(dma_dest_mem_buf, 0, sizeof(dma_dest_mem_buf));

    dma_link_request[0].src_ptr  = dma_src_mem_buf + 4;              /*src address*/
    dma_link_request[0].dest_ptr =  (dma_dest_mem_buf + dest_offset);   /*destinatio address*/
    dma_link_request[0].size = DMA_LL_ITEM0_SIZE;                /*move 6 bytes*/

    dest_offset +=  DMA_LL_ITEM0_SIZE;

    dma_link_request[1].src_ptr  = dma_src_mem_buf + 24;
    dma_link_request[1].dest_ptr = dma_dest_mem_buf + dest_offset;
    dma_link_request[1].size = DMA_LL_ITEM1_SIZE;

    dest_offset +=  DMA_LL_ITEM1_SIZE;

    dma_link_request[2].src_ptr  = dma_src_mem_buf + 38;
    dma_link_request[2].dest_ptr = dma_dest_mem_buf + dest_offset;
    dma_link_request[2].size = DMA_LL_ITEM2_SIZE;

    dest_offset +=  DMA_LL_ITEM2_SIZE;

    dma_link_request[3].src_ptr  = dma_src_mem_buf + 62;
    dma_link_request[3].dest_ptr = dma_dest_mem_buf + dest_offset;
    dma_link_request[3].size = DMA_LL_ITEM3_SIZE;

    dest_offset +=  DMA_LL_ITEM3_SIZE;

    /*the following setting is linkListEnable, meory to memory. */
    dma_control_mode =  DMA_CNTL_SRC_WIDTH_32BIT | DMA_CNTL_DEST_WIDTH_32BIT |
                        DMA_CNTL_SRC_INC  | DMA_CNTL_SRC_BURST8 | DMA_CNTL_SRC_PORT |
                        DMA_CNTL_DEST_INC | DMA_CNTL_DEST_BURST8 |  DMA_CNTL_DEST_PORT |
                        DMA_CNTL_MODE_MEMTOMEM  | DMA_CNTL_LINKLIST_ENABLE;

    /*create a LinkList reqest for hardware..*/
    create_dma_link_list(
        (dma_linklist_request_t *) (dma_link_request), DMA_TEST_ITEM,
        (dma_linklist_entry_t *) (dma_link_list), dma_control_mode );

    g_dma_finish_state = 0;

    /*set dma to link list mode*/
    dma_set_link_list(TEST_DMA_CHANNEL, (dma_linklist_entry_t *) dma_link_list,
                      (DMA_INT_LL_ELEMENT_Done | DMA_INT_XFER_DONE));

    /*start DMA */
    dma_enable(TEST_DMA_CHANNEL);

    /*wait memory move complete. */
    while (g_dma_finish_state != 0)
    {
        /*busy wait DMA finish...if you use RTOS, you can use event wait function here. */

    }

    /*compare data here.*/
    for (i = 0; i < 4; i++)
    {
        ret_status = mem_cmp(TEST_DMA_CHANNEL, dma_link_request[i].src_ptr,
                             dma_link_request[i].dest_ptr, dma_link_request[i].size);

        if (ret_status == STATUS_ERROR)
        {
            while (1);
        }

    }

    printf("\n DMA for memory link list test success \n");
    printf("Good News!! \n");

    while (1)
    {

    }
}

void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
