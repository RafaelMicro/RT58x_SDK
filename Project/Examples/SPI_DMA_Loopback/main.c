/** @file main.c
 *
 * @brief SPI_DMA_Loopback example main file.
 *
 *
 */
/**
* @defgroup SPI_DMA_Loopback_example_group  SPI_DMA_Loopback
* @ingroup examples_group
* @{
* @brief SPI_DMA_Loopback example demonstrate
*/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "project_config.h"

#include "uart_drv.h"
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
uint32_t console_drv_init(uart_baudrate_t baudrate);


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

    /*init SPI pin mux   -- for SPI1*/
    pin_set_mode(28, MODE_QSPI1);     /*SPI SCLK*/
    pin_set_mode(29, MODE_QSPI1);     /*SPI CS*/
    pin_set_mode(30, MODE_QSPI1);     /*SPI DATA0*/
    pin_set_mode(31, MODE_QSPI1);     /*SPI DATA1*/

    return;
}
void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}

/* Notice: In this simple demo example, it does NOT support OS task signal event
 * It use state polling...
 * This is demo code only.
 * If you have RTOS, you can use some functions like wait_signal_event/signal_event
 *
 */

volatile uint32_t     spi1_finish_state = 0;

void spi1_finish_callback(uint32_t channel_id, uint32_t status)
{
    spi1_finish_state = 1;
    return;
}

volatile uint32_t     spi0_finish_state = 0;

void spi0_finish_callback(uint32_t channel_id, uint32_t status)
{
    spi0_finish_state = 1;
    return;
}


#define TESTBLOCKSIZE   2048


static uint8_t    master_buffer_send[TESTBLOCKSIZE],  master_buffer_recv[TESTBLOCKSIZE];
static uint8_t    slave_buffer_send[TESTBLOCKSIZE], slave_buffer_recv[TESTBLOCKSIZE];


int main(void)
{
    uint32_t         i, j, temp;
    uint32_t         test_mode = 0;

    qspi_transfer_mode_t    spi0_config_mode, spi1_config_mode;

    spi_block_request_t     master_xfer_request, slave_xfer_request;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("Spi master/slave loopback test\n");


    /*init spi mode first.*/

    /*   To test this program,
     *
     *  Please hardware loopback the pin
     *
     *  SCLK  GPIO 6 --- GPIO28
     *  CS    GPIO 7 --- GPIO29
     *  D0    GPIO 8 --- GPIO30
     *  D1    GPIO 9 --- GPIO31
     */

    printf("Please hardware loopback the pin: \n");
    printf(" SCLK: GPIO 06 --- GPIO28 \n");
    printf(" CS  : GPIO 07 --- GPIO29 \n");
    printf(" MOSI: GPIO 08 --- GPIO30 \n");
    printf(" MISO: GPIO 09 --- GPIO31 \n");

    printf("In this example, SPI0 is master, SPI1 is slave \n");
    printf("SPI0 and SPI1 using MOSI and MISO to exchange data \n");
    printf("SPI using DMA to transfer data \n");


    /*generate some test pattern for test*/
    master_buffer_send[0] = 0xBE;
    master_buffer_send[1] = 0xEF;

    slave_buffer_send[0] = 0xDE;
    slave_buffer_send[1] = 0xAD;

    temp = 0;

    for (i = 2; i < TESTBLOCKSIZE; i++)
    {
        temp += (master_buffer_send[i - 2] * 29) + (master_buffer_send[i - 1] * 41) + 71;
        master_buffer_send[i] = temp % 0xFE;

        temp += (slave_buffer_send[i - 2] * 47) + (slave_buffer_send[i - 1] * 31) + 57;
        slave_buffer_send[i] = temp % 0xFE;

        if (i % 37)
        {
            temp = (((temp >> 13) & 0x0007FFFF) | ((temp & 0x1FFF) << 19));
        }

    }

    memset(slave_buffer_recv, 0xFF, TESTBLOCKSIZE);
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

    /*slave mode should the same as master mode.*/
    spi1_config_mode.SPI_BIT_ORDER =  SPI_MSB_ORDER;

    spi1_config_mode.SPI_CPOL  =  (test_mode >> 1);    /*MODE 0*/
    spi1_config_mode.SPI_CPHA  =  (test_mode & 0x1);

    spi1_config_mode.SPI_CS_POL = SPI_CHIPSEL_ACTIVE_LOW;    /*CS low active, idled in high */
    spi1_config_mode.SPI_MASTER = SPI_SLAVE_MODE;            /*controller spi1 as slave*/
    spi1_config_mode.SPI_CS = SPI_SELECT_SLAVE_0;            /*SPI1 only has one chip select */

    /* SPI slave mode ignored this SPI_CLK setting
     * set this value is to avoild SPI_CLK become strange value */
    spi1_config_mode.SPI_CLK = QSPI_CLK_1M;

    qspi_init(1, &spi1_config_mode);

    /*
     * In real application, slave must prepare data before master request!
     * So if using spi as slave in SOC, then it should have one mechaism to
     * let master knows data is available or not...
     * It means that SOC needs one interrupt pin to notify data available.
     *
     */


    for (i = 1; i < TESTBLOCKSIZE ; i++)
    {
        slave_xfer_request.write_buf = slave_buffer_send ;
        slave_xfer_request.read_buf = slave_buffer_recv ;
        slave_xfer_request.length      = i;

        spi_transfer(1, &slave_xfer_request, spi1_finish_callback);

        master_xfer_request.write_buf = master_buffer_send;
        master_xfer_request.read_buf = master_buffer_recv;
        master_xfer_request.length      = i;

        spi_transfer(0, &master_xfer_request, spi0_finish_callback);

        /*wait transfer finish... in multi-tasking environment,
          it can use wait_evnet function here.*/

        while ( spi0_finish_state == 0)
        {

        }

        while ( spi1_finish_state == 0)
        {

        }

        /*compare data*/

        for (j = 0; j < i; j++)
        {
            if (master_buffer_recv[j] != slave_buffer_send[j])
            {
                printf("data error X %ld %ld \n", i, j);

                while (1);
            }

            if (slave_buffer_recv[j] != master_buffer_send[j])
            {
                printf("data error Y %ld %ld  \n", i, j);
                while (1);
            }
        }

        spi0_finish_state = 0;
        spi1_finish_state = 0;
        printf(".");
    }

    printf("\n TEST SPI loopback OK! \n");

    while (1);
}

void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
