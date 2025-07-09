/** @file main.c
 *
 * @brief I2S example main file.
 * @details Demonstrate configuration the I2s parameter and control Tas2505 for output
 *
 *
 */
/**
* @defgroup I2S_example_group  I2S
* @ingroup examples_group
* @{
* @brief I2S example demonstrate
*
*
*/
/**************************************************************************************************
*    INCLUDES
*************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "project_config.h"
#include "uart_drv.h"
#include "retarget.h"
#include "rf_mcu_ahb.h"
/**************************************************************************************************
*    MACROS
*************************************************************************************************/


/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
/************************************************************
* definitions for this sample code project
************************************************************/
#define I2S_IO_BCK          0       //  0 or 10, default: 0
#define I2S_IO_WS           1       //  1 or 11, default: 1
#define I2S_IO_SDO          2       //  2 or 12, default: 2
#define I2S_IO_SDI          3       //  3 or 13, default: 3
#define I2S_IO_MCLK         8
#define GPIO16              16
#define GPIO17              17

#define I2S_IMCK_RATE       I2S_IMCLK_8P192M        //iMCLK
#define I2S_MCK_DIV         I2S_MCLK_DIV_2          //iMCLK_DIV_2 -->8192/2-->I2S_MCLK is 4096
#define I2S_BCK_RATIO       I2S_BCK_RATIO_64        //Bck_Ratio_64
#define I2S_BCK_OSC         I2S_CFG_BCK_OSR_4       //Ratio (MCLK/BCK) 4096/4-->1024
#define I2S_SR              I2S_SR_48K
#define I2S_FMT             I2S_FMT_I2S             //I2S Format
#define I2S_TRX_MODE        I2S_TRX_MODE_RX         //I2S Rx Mode
#define I2S_CFG_WID         I2S_CFG_WID_32          //width 32bit
#define I2S_CH              I2S_CH_MONO_L           //Mono L

#define I2S_RING_BUF_RATIO  (16)
#define I2S_RDMA_SEG_SIZE   (256*I2S_RING_BUF_RATIO)
#define I2S_WRMA_SEG_SIZE   (256*I2S_RING_BUF_RATIO)
#define I2S_RDMA_BLK_SIZE   (256)
#define I2S_WRMA_BLK_SIZE   (256)

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02

#define I2S_WR_BUFF         0x20003000
/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
// ---------- D E C L A I R A T I O N S ---------------- //
i2s_rdma_ctrl_ptr_t i2s_rdma_buf, i2s_wdma_buf;
i2s_buff_ptr_t      i2s_w, i2s_r;
/**************************************************************************************************
*    GLOBAL FUNCTIONS
*************************************************************************************************/
/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    /*set GPIO00 as I2S_BCK, GPIO01 as I2S_MCK, GPIO02 as I2S0_SD, GPIO03 as I2S_SDI ,and GPIO08 as I2S_MCLK*/
    pin_set_mode(I2S_IO_BCK,    MODE_I2S);  //GPIO0 or GPIO10  BCK
    pin_set_mode(I2S_IO_WS,     MODE_I2S);  //GPIO1 or GPIO11  WS
    pin_set_mode(I2S_IO_SDO,    MODE_I2S);  //GPIO2 or GPIO12  SDO
    pin_set_mode(I2S_IO_SDI,    MODE_I2S);  //GPIO3 or GPIO13  SDI
    pin_set_mode(I2S_IO_MCLK,   MODE_I2S);  //GPIO08           MCLK

    /*uart0 pinmux*/
    pin_set_mode(GPIO16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(GPIO17, MODE_UART);     /*GPIO17 as UART0 TX*/

    return;
}
void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}
/**
 * @ingroup I2S_example_group
 * @brief Functio initinal the i2s parameter set
 * @param[in] i2s_para
                \arg i2s_rdma_ctrl_ptr_t* I2S RDMA configurarions
                \arg i2s_wdma_ctrl_ptr_t* I2S WDMA configurarions
                \arg i2s_sample_rate_t    I2S Simple rate configurarions
                \arg i2s_ch_format_t      I2S Channel mode configurarions ( Stereo / Left channel only / Right channel only)
                \arg i2s_trx_mode_t       I2C TRX mode select 0: TxRx 1: Tx only 2: Rx only 3: TxRx
                \arg i2s_fmt_t            I2S Format (LJ/RJ/I2S Mode)
                \arg i2s_cfg_txd_wid_t    I2S TRX width  0: 16-bit;1: 24-bit; 2: 32-bit 3: 32-bit
                \arg i2s_cfg_bck_ratio_t  I2S BLCK/WRCK ratio
                \arg i2s_mclk_div_t       I2S Output MCLK divider
                \arg i2s_cfg_bck_osr_t    I2S Ratio of MCLK and BCLK (I2S_MCLK = I2S_BCK * Ratio)
                \arg i2s_cfg_imck_rate_t  I2S Internal MCLK rate
 * @return None
 */
void Init_I2s_Parameter(i2s_para_set_t *i2s_para)
{
    uint32_t loop;

    //Config I2S wdma
    i2s_wdma_buf.i2s_xdma_start_addr = I2S_WR_BUFF;
    i2s_wdma_buf.i2s_fw_access_addr = I2S_WR_BUFF;
    i2s_wdma_buf.i2s_xdma_seg_size = I2S_WRMA_SEG_SIZE;
    i2s_wdma_buf.i2s_xdma_blk_size = I2S_WRMA_BLK_SIZE;
    i2s_wdma_buf.i2s_xdma_seg_blk_ratio = I2S_RING_BUF_RATIO;

    i2s_para->rdma_config = NULL;//(i2s_rdma_ctrl_ptr_t *)&i2s_rdma_buf;
    i2s_para->wdma_config = (i2s_wdma_ctrl_ptr_t *)&i2s_wdma_buf;

    i2s_para->imck_rate = I2S_IMCK_RATE;    //i2s imclk rate
    i2s_para->mck_div = I2S_MCK_DIV;        //imclk div
    i2s_para->bck_osr = I2S_BCK_OSC;        //MCLK/BCK = 4
    i2s_para->trx_mode = I2S_TRX_MODE;      //
    i2s_para->fmt = I2S_FMT;                //
    i2s_para->bck_ratio = I2S_BCK_RATIO;    //
    i2s_para->width = I2S_CFG_WID;          //32 bits
    i2s_para->ch = I2S_CH;                  //mono, L
    i2s_para->sr = I2S_SR;
    //Clear Buffer

    for (loop = 0; loop < i2s_wdma_buf.i2s_xdma_seg_size; loop++)
    {
        *((uint32_t *)i2s_wdma_buf.i2s_xdma_start_addr + loop) = 0x11;
    }

    for (loop = 0; loop < i2s_wdma_buf.i2s_xdma_seg_size; loop++)
    {
        *((uint32_t *)i2s_wdma_buf.i2s_xdma_start_addr + loop) = 0x00;
    }


    i2s_w.address = i2s_wdma_buf.i2s_xdma_start_addr;
    i2s_w.size = I2S_WRMA_BLK_SIZE;
}

void I2s_Int_Callback_Handler(i2s_cb_t *p_cb)
{

    if (p_cb->type == I2S_CB_WDMA)
    {

        i2s_w.flag = 1;

        if (i2s_w.index == p_cb->seg_size)
        {
            i2s_w.index = 0;
        }

        i2s_w.index += p_cb->blk_size;

        if (i2s_w.index == i2s_w.size)
        {
            i2s_w.offset = 0;
        }
        else
        {
            i2s_w.offset = i2s_w.index - i2s_w.size;
        }
    }

    if (p_cb->type == I2S_CB_RDMA)
    {
        i2s_r.flag = 1;

        if (i2s_r.index == p_cb->seg_size)
        {
            i2s_r.index = 0;
        }

        i2s_r.index += p_cb->blk_size;

        if (i2s_r.index == i2s_r.size)
        {
            i2s_r.offset = 0;
        }
        else
        {
            i2s_r.offset = i2s_r.index - i2s_r.size ;
        }
    }
}

int main(void)
{
    i2s_para_set_t i2s_config_para;

    uint32_t buf_index;
    uint32_t wdma_value;

    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();
    /*init debug uart port for printf
     *Please use macro define  UART_BAUDRATE_XXXXXX
    */
    console_drv_init(UART_BAUDRATE_115200);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    /* fill the parameters will be used. */
    Init_I2s_Parameter(&i2s_config_para);

    /* Reigster Interrupt callback function */
    i2s_Register_Int_Callback(I2s_Int_Callback_Handler);

    /* Initial I2S */
    I2s_Init(&i2s_config_para);

    printf("Digital Microphone SPH6405 Pin connect\n");
    printf("SEL  Pin <----> EVK_GND(Left channel)/EVK_VCC(Right channel)\n");
    printf("LRCL Pin <----> EVK_WCK(GPIO1/GPIO11)\n");
    printf("BCK  Pin <----> EVK_BCK(GPIO0/GPIO10)\n");
    printf("DOUT Pin <----> EVK_SDI(GPIO3/GPIO13)\n");
    printf("I2S Start\n");

    I2s_Start(&i2s_config_para);

    while (1)
    {
        if (i2s_w.flag == 1)
        {
            for (buf_index = 0; buf_index < i2s_w.size; buf_index++)
            {
                wdma_value = *((uint32_t *)(i2s_wdma_buf.i2s_fw_access_addr) + i2s_w.offset + buf_index); //read wdam data
                printf("%ld,%lx\r\n", buf_index, wdma_value);
            }

            i2s_w.flag = 0;
        }
    }
}
/** @} */ /* end of examples group */
