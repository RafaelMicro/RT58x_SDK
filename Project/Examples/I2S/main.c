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
// ---------- D E F I N I T I O N S -------------------- //
#define DAC_BRD_USE         0

#define I2S_IO_BCK          0       //  0 or 10, default: 0
#define I2S_IO_WS           1       //  1 or 11, default: 1
#define I2S_IO_SDO          2       //  2 or 12, default: 2
#define I2S_IO_SDI          3       //  3 or 13, default: 3
#define I2S_IO_MCLK         8


#define I2S_IMCK_RATE       I2S_IMCLK_12P288M;
#define I2S_MCK_DIV         I2S_MCLK_DIV_1;
#define I2S_BCK_OSC         I2S_CFG_BCK_OSR_8;
#define I2S_BCK_RATIO       I2S_BCK_RATIO_32
#define I2S_SR              I2S_SR_48K
#define I2S_FMT             I2S_FMT_I2S
#define I2S_CH              I2S_CH_STEREO
#define I2S_TRX_MODE        I2S_TRX_MODE_TXRX
#define I2S_CFG_WID         I2S_CFG_WID_16

#define I2S_RD_BUFF         0x20002000
#define I2S_WR_BUFF         0x20003000

#define I2S_RING_BUF_RATIO  (3)
#define I2S_RDMA_SEG_ZIZE   (96*I2S_RING_BUF_RATIO)
#define I2S_WRMA_SEG_ZIZE   (96*I2S_RING_BUF_RATIO)
#define I2S_RDMA_BLK_ZIZE   (96)
#define I2S_WRMA_BLK_ZIZE   (96)

#define GPIO16              16
#define GPIO17              17

#define COMPARE_HEAD_DATA        0x5AA50001
/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */
#define PRINTF_BAUDRATE      UART_BAUDRATE_115200
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
// ---------- D E C L A I R A T I O N S ---------------- //
/*this is pin mux setting*/
extern void I2C_Pin_Config(void);
extern void I2c_Finish(uint32_t status);
extern void Tas2505_I2c_Data_Convert(uint16_t reg_addr, uint8_t data);
extern void Init_Dac_Board_I2c(i2s_para_set_t *i2s_para);

i2s_rdma_ctrl_ptr_t i2s_rdma_buf;
i2s_wdma_ctrl_ptr_t i2s_wdma_buf;
i2s_buff_ptr_t      i2s_w, i2s_r;

const i2s_sample_rate_t i2s_sr = I2S_SR;
/**************************************************************************************************
*    GLOBAL FUNCTIONS
*************************************************************************************************/
/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    /*set GPIO00 as I2S_BCK, GPIO01 as I2S_MCK, GPIO02 as I2S0_SD, GPIO03 as I2S_SDI ,and GPIO08 as I2S_MCLK*/
    pin_set_mode(I2S_IO_BCK,    MODE_I2S);
    pin_set_mode(I2S_IO_WS,     MODE_I2S);
    pin_set_mode(I2S_IO_SDO,    MODE_I2S);
    pin_set_mode(I2S_IO_SDI,    MODE_I2S);
    pin_set_mode(I2S_IO_MCLK,   MODE_I2S);


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
void I2S_Fill_LoopBack_Data_To_Memory(i2s_rdma_ctrl_ptr_t *i2s_rdma_para)
{
    uint16_t loop = 0;

    for (loop = 0; loop < I2S_RDMA_SEG_ZIZE; loop++)
    {

        *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop )  = (COMPARE_HEAD_DATA + loop);
    }
}

void I2s_Fill_Data_To_Memory(i2s_rdma_ctrl_ptr_t *i2s_rdma_para)
{
    uint16_t loop;

    if (i2s_rdma_para->i2s_xdma_blk_size == 0)
    {
        //One-shot mode case. Fill one-shot data here
    }
    else     //continuous mode
    {
        //In the sample below, the data in left channel is 1k tone, and the data in the right channel is 2k tone
        if (i2s_sr == I2S_SR_48K)
        {
            for (loop = 0; loop < 2 * 3; loop++)
            {
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 0)  = 0x7FFF0000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 1)  = 0x7BA310B5;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 2)  = 0x6EDA2121;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 3)  = 0x5A8230FC;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 4)  = 0x40004000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 5)  = 0x21214DEC;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 6)  = 0x00005A82;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 7)  = 0xDEDF658D;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 8)  = 0xC0006EDA;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 9)  = 0xA57E7642;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 10) = 0x91267BA3;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 11) = 0x845D7EE8;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 12) = 0x80007FFF;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 13) = 0x845D7EE8;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 14) = 0x91267BA3;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 15) = 0xA57E7642;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 16) = 0xC0006EDA;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 17) = 0xDEDF658D;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 18) = 0x00005A82;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 19) = 0x21214DEC;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 20) = 0x40004000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 21) = 0x5A8230FC;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 22) = 0x6EDA2121;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 23) = 0x7BA310B5;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 24) = 0x7FFF0000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 25) = 0x7BA3EF4B;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 26) = 0x6EDADEDF;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 27) = 0x5A82CF04;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 28) = 0x4000C000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 29) = 0x2121B214;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 30) = 0x0000A57E;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 31) = 0xDEDF9A73;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 32) = 0xC0009126;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 33) = 0xA57E89BE;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 34) = 0x9126845D;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 35) = 0x845D8118;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 36) = 0x80008000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 37) = 0x845D8118;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 38) = 0x9126845D;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 39) = 0xA57E89BE;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 40) = 0xC0009126;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 41) = 0xDEDF9A73;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 42) = 0x0000A57E;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 43) = 0x2121B214;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 44) = 0x4000C000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 45) = 0x5A82CF04;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 46) = 0x6EDADEDF;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 48 + 47) = 0x7BA3EF4B;
            }
        }
        else if (I2S_SR == I2S_SR_32K)
        {
            for (loop = 0; loop < 3 * 3; loop++)
            {
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 0) = 0x7FFF0000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 1) = 0x764218F9;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 2) = 0x5A8230FC;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 3) = 0x30FC471D;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 4) = 0x00005A82;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 5) = 0xCF046A6E;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 6) = 0xA57E7642;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 7) = 0x89BE7D8A;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 8) = 0x80007FFF;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 9) = 0x89BE7D8A;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 10) = 0xA57E7642;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 11) = 0xCF046A6E;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 12) = 0x00005A82;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 13) = 0x30FC471D;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 14) = 0x5A8230FC;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 15) = 0x764218F9;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 16) = 0x7FFF0000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 17) = 0x7642E707;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 18) = 0x5A82CF04;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 19) = 0x30FCB8E3;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 20) = 0x0000A57E;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 21) = 0xCF049592;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 22) = 0xA57E89BE;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 23) = 0x89BE8276;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 24) = 0x80008000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 25) = 0x89BE8276;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 26) = 0xA57E89BE;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 27) = 0xCF049592;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 28) = 0x0000A57E;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 29) = 0x30FCB8E3;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 30) = 0x5A82CF04;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 32 + 31) = 0x7642E707;
            }
        }
        else if (I2S_SR == I2S_SR_16K)
        {
            for (loop = 0; loop < 6 * 3; loop++)
            {
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 0) = 0x7FFF0000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 1) = 0x5A8230FC;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 2) = 0x00005A82;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 3) = 0xA57E7642;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 4) = 0x80007FFF;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 5) = 0xA57E7642;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 6) = 0x00005A82;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 7) = 0x5A8230FC;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 8) = 0x7FFF0000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 9) = 0x5A82CF04;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 10) = 0x0000A57E;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 11) = 0xA57E89BE;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 12) = 0x80008000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 13) = 0xA57E89BE;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 14) = 0x0000A57E;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 16 + 15) = 0x5A82CF04;
            }
        }
        else if (I2S_SR == I2S_SR_8K)
        {
            for (loop = 0; loop < 12 * 3; loop++)
            {
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 8 + 0) = 0x7FFF0000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 8 + 1) = 0x00005A82;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 8 + 2) = 0x80007FFF;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 8 + 3) = 0x00005A82;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 8 + 4) = 0x7FFF0000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 8 + 5) = 0x0000A57E;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 8 + 6) = 0x80008000;
                *((uint32_t *)(i2s_rdma_para->i2s_fw_access_addr) + loop * 8 + 7) = 0x0000A57E;
            }
        }
    }
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
    uint16_t loop;
    //Config I2S Rdma
    i2s_rdma_buf.i2s_xdma_start_addr = I2S_RD_BUFF;
    i2s_rdma_buf.i2s_fw_access_addr = I2S_RD_BUFF;
    i2s_rdma_buf.i2s_xdma_seg_size = I2S_RDMA_SEG_ZIZE;
    i2s_rdma_buf.i2s_xdma_blk_size = I2S_RDMA_BLK_ZIZE;
    i2s_rdma_buf.i2s_xdma_seg_blk_ratio = I2S_RING_BUF_RATIO;
    //Config I2S wdma
    i2s_wdma_buf.i2s_xdma_start_addr = I2S_WR_BUFF;
    i2s_wdma_buf.i2s_fw_access_addr = I2S_WR_BUFF;
    i2s_wdma_buf.i2s_xdma_seg_size = I2S_WRMA_SEG_ZIZE;
    i2s_wdma_buf.i2s_xdma_blk_size = I2S_WRMA_BLK_ZIZE;
    i2s_wdma_buf.i2s_xdma_seg_blk_ratio = I2S_RING_BUF_RATIO;

    i2s_para->rdma_config = (i2s_rdma_ctrl_ptr_t *)&i2s_rdma_buf;
    i2s_para->wdma_config = (i2s_rdma_ctrl_ptr_t *)&i2s_wdma_buf;

    i2s_para->sr = I2S_SR;
    i2s_para->fmt = I2S_FMT;
    i2s_para->ch = I2S_CH;
    i2s_para->trx_mode = I2S_TRX_MODE;
    i2s_para->width = I2S_CFG_WID;
    i2s_para->bck_ratio = I2S_BCK_RATIO;
    i2s_para->mck_div = I2S_MCK_DIV;
    i2s_para->bck_osr = I2S_BCK_OSC;
    i2s_para->imck_rate = I2S_IMCK_RATE;



    for (loop = 0; loop < i2s_wdma_buf.i2s_xdma_seg_size; loop++)
    {
        *((uint32_t *)i2s_wdma_buf.i2s_xdma_start_addr + loop) = 0x00;
    }
    for (loop = 0; loop < i2s_rdma_buf.i2s_xdma_seg_size; loop++)
    {
        *((uint32_t *)i2s_rdma_buf.i2s_xdma_start_addr + loop) = 0x00;
    }

}

void I2s_Int_Callback_Handler(i2s_cb_t *p_cb)
{

    if (p_cb->type == I2S_CB_WDMA)
    {

        //gpio_pin_toggle(4);
        i2s_w.flag = 1;
        if (i2s_w.offset == p_cb->seg_size)
        {
            i2s_w.offset = 0;
        }
        i2s_w.size = p_cb->blk_size;
        i2s_w.offset += p_cb->blk_size;

    }

    if (p_cb->type == I2S_CB_RDMA)
    {
        //gpio_pin_toggle(5);
        i2s_r.flag = 1;
        if (i2s_r.offset == p_cb->seg_size)
        {
            i2s_r.offset = 0;
        }
        i2s_r.offset += p_cb->blk_size;
        i2s_r.size = p_cb->blk_size;
    }
}

int main(void)
{
    i2s_para_set_t i2s_config_para;

    uint32_t wdma_value = 0;
    uint32_t wdma_index = 0, cmpvalue = 0, count = 0;
    uint32_t buf_offset;

    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();
    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    /* fill the parameters will be used. */
    Init_I2s_Parameter(&i2s_config_para);

    /* Initial I2S */
    I2s_Init(&i2s_config_para);

    /* Fill the initial loop back data into memory */
    I2S_Fill_LoopBack_Data_To_Memory(&i2s_rdma_buf);

    /* Reigster Interrupt callback function */
    i2s_Register_Int_Callback(I2s_Int_Callback_Handler);

    printf("I2S Start\n");

    I2s_Start(&i2s_config_para);

    while (1)
    {
        if (i2s_w.flag == 1) //I2S wdma Interrupt
        {
            if (i2s_w.offset == i2s_w.size)
            {
                buf_offset = 0;
                count = 0;
            }
            else
            {
                buf_offset = i2s_w.offset - i2s_w.size ;
            }

            for (wdma_index = 0; wdma_index < i2s_w.size; wdma_index++)
            {
                wdma_value = *((uint32_t *)(i2s_wdma_buf.i2s_fw_access_addr) + buf_offset + wdma_index); //read wdam data

                if (wdma_value == COMPARE_HEAD_DATA)
                {
                    cmpvalue =  COMPARE_HEAD_DATA;
                }
                else
                {
                    cmpvalue += 1;
                    if (wdma_value == cmpvalue)
                    {
                        count += 1;
                        if ((wdma_index > 10) && (wdma_index < 12))
                        {
                            printf("%ld,%.8lx\r\n", count, wdma_value);    //for loop check
                        }
                    }
                    else
                    {
                        printf("%ld,%.8lx\r\n", count, wdma_value); //
                    }
                }
            }

            i2s_w.flag = 0;
        }
    }
}
/** @} */ /* end of examples group */
