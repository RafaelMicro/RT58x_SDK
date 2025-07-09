/** @file main.c
 *
 * @brief SPI_Master_PIO example main file.
 *
 *
 */
/**
* @defgroup SPI_Master_PIO_example_group  SPI_Master_PIO
* @ingroup examples_group
* @{
* @brief SPI_Master_PIO example demonstrate
*/

#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "project_config.h"

#include "uart_drv.h"
#include "retarget.h"
#include "rf_mcu_ahb.h"

int main(void);

void SetClockFreq(void);

/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{

    /*uart0 pinmux, This is default setting,
      we set it for safety. */
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/

    /*init SPI pin mux   -- for SPI0*/
    pin_set_mode( 6, MODE_QSPI0);     /*SPI SCLK*/
    pin_set_mode( 7, MODE_QSPI0);     /*SPI CS*/
    pin_set_mode( 8, MODE_QSPI0);     /*SPI DATA0*/
    pin_set_mode( 9, MODE_QSPI0);     /*SPI DATA1*/

    pin_set_mode( 0, MODE_QSPI0_CSN1);      /*GPIO0 as CS1 of SPI0*/
    pin_set_mode( 1, MODE_QSPI0_CSN2);      /*GPIO1 as CS2 of SPI0*/

    return;
}

void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}

#define TESTBLOCKSIZE   32


static uint8_t    master_buffer_send[TESTBLOCKSIZE],  master_buffer_recv[TESTBLOCKSIZE];


int main(void)
{
    uint32_t         i,  temp;
    uint32_t         test_mode = 0;

    qspi_transfer_mode_t    spi0_config_mode;

    spi_block_request_t     master_xfer_request;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("SPI master pio send example \n");

    printf("This example use SPI0 as master \n");
    printf("If you enable SUPPORT_QSPI0_MULTI_CS then the SOC can send data to CS0 and CS1 device\n");

    printf("In this example:\n ");
    printf("    SLCK is  GPIO6 \n");
    printf("    DATA0 is GPIO8 \n");
    printf("    DATA1 is GPIO9 \n");
    printf("    CSN0 is  GPIO7 \n");
    printf("    CSN1 is  GPIO70\n");


    /*init spi mode first.*/

    /*   To test this program,
     *
     *  SCLK  GPIO 6
     *  CS    GPIO 7
     *  D0    GPIO 8
     *  D1    GPIO 9
     */

    /*generate some test pattern for test*/
    master_buffer_send[0] = 0xBE;
    master_buffer_send[1] = 0xEF;

    temp = 0;

    for (i = 2; i < TESTBLOCKSIZE; i++)
    {
        temp += (master_buffer_send[i - 2] * 29) + (master_buffer_send[i - 1] * 41) + 71;
        master_buffer_send[i] = temp % 0xFE;

        if (i % 37)
        {
            temp = (((temp >> 13) & 0x0007FFFF) | ((temp & 0x1FFF) << 19));
        }

    }

    memset(master_buffer_recv, 0xFF, TESTBLOCKSIZE);

    /*In this example, we test mode0, SPI clock is 16MHz */

    spi0_config_mode.SPI_BIT_ORDER =  SPI_MSB_ORDER;

    spi0_config_mode.SPI_CPOL  =  (test_mode >> 1);
    spi0_config_mode.SPI_CPHA  =  (test_mode & 0x1);


    spi0_config_mode.SPI_CS_POL = SPI_CHIPSEL_ACTIVE_LOW;    /*CS low active, idled in high */
    spi0_config_mode.SPI_MASTER = SPI_MASTER_MODE;           /*controller spi0 as SPI host*/
    spi0_config_mode.SPI_CS = SPI_SELECT_SLAVE_0;            /*Select SS0 */

    spi0_config_mode.SPI_CLK = QSPI_CLK_16M;

    qspi_init(0, &spi0_config_mode);


#if (MODULE_ENABLE(SUPPORT_QSPI0_MULTI_CS))
    /* set other polarity of spi0,
     * Please call qspi0_set_device_polarity immediately after qspi_init,
     * and before any spi transfer
     */
    qspi0_set_device_polarity(SPI_SELECT_SLAVE_1, SPI_CHIPSEL_ACTIVE_HIGH);
    qspi0_set_device_polarity(SPI_SELECT_SLAVE_2, SPI_CHIPSEL_ACTIVE_LOW);

#endif

    for (i = 1; i < TESTBLOCKSIZE ; i++)
    {

        master_xfer_request.write_buf = master_buffer_send;
        master_xfer_request.read_buf = master_buffer_recv;      /*if this pointer is NULL, it is just send data to slave.*/
        master_xfer_request.length      = i;

#if (MODULE_ENABLE(SUPPORT_QSPI0_MULTI_CS))
        if ((i & 1) == 0)
        {
            master_xfer_request.select_slave_device = 0;    /*transfer to CS0 device*/
        }
        else
        {
            master_xfer_request.select_slave_device = 1;    /*transfer to CS1 device*/
        }
#endif

        /* Notice: if you want to send large data, please use DMA.
         * PIO transfer is better for short bytes...
         */
        spi_transfer_pio(0, &master_xfer_request);


        printf(".");
    }



    while (1);
}

void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
