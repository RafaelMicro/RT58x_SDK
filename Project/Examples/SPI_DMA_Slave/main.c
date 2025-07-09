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


#define GPIO_0          0
#define SPI_CLK_PIN     28
#define SPI_CS_PIN      29
#define SPI_MOSI_PIN    30
#define SPI_MISO_PIN    31
#define UART0_RX_PIN    16
#define UART0_TX_PIN    17
#define TESTBLOCKSIZE   10

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02

/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */
uint32_t console_drv_init(uart_baudrate_t baudrate);

static uint8_t    slave_buffer_send[TESTBLOCKSIZE], slave_buffer_recv[TESTBLOCKSIZE];
volatile uint32_t     spi1_finish_state = 0;


void spi1_finish_callback(uint32_t channel_id, uint32_t status)
{
    spi1_finish_state = 1;
    return;
}
/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    /*uart0 pinmux, This is default setting,
      we set it for safety. */
    pin_set_mode(UART0_RX_PIN, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(UART0_TX_PIN, MODE_UART);     /*GPIO17 as UART0 TX*/

    /*init SPI pin mux   -- for SPI1*/
    pin_set_mode(SPI_CLK_PIN, MODE_QSPI1);     /*SPI SCLK*/
    pin_set_mode(SPI_CS_PIN, MODE_QSPI1);     /*SPI CS*/
    pin_set_mode(SPI_MOSI_PIN, MODE_QSPI1);     /*SPI DATA0*/
    pin_set_mode(SPI_MISO_PIN, MODE_QSPI1);     /*SPI DATA1*/

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
    uint32_t         i;

    qspi_transfer_mode_t    spi1_config_mode;

    spi_block_request_t     slave_xfer_request;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf(" Spi slave dma example\n");


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

    printf(" hardware connect  pin: \r\n");
    printf(" SPI Slave --- SPI Master \r\n");
    printf(" SCLK: GPIO 28 --- GPIO06 \r\n");
    printf(" CS  : GPIO 29 --- GPIO07 \r\n");
    printf(" MOSI: GPIO 30 --- GPIO08 \r\n");
    printf(" MISO: GPIO 31 --- GPIO09 \r\n");

    printf(" In this example, SPI1 is slave \n");
    printf(" SPI using DMA to transfer data \n");

    for (i = 0; i < TESTBLOCKSIZE; i++)
    {
        slave_buffer_send[i] = 0x0A - i;
    }

    memset(slave_buffer_recv, 0xFF, TESTBLOCKSIZE);

    /*slave mode should the same as master mode.*/
    spi1_config_mode.SPI_BIT_ORDER =  SPI_MSB_ORDER;
    spi1_config_mode.SPI_CPOL  =  0;    /*MODE 0*/
    spi1_config_mode.SPI_CPHA  =  0;
    spi1_config_mode.SPI_CS_POL = SPI_CHIPSEL_ACTIVE_LOW;    /*CS low active, idled in high */
    spi1_config_mode.SPI_MASTER = SPI_SLAVE_MODE;            /*controller spi1 as slave*/
    spi1_config_mode.SPI_CS = SPI_SELECT_SLAVE_0;            /*SPI1 only has one chip select */

    /* SPI slave mode ignored this SPI_CLK setting
     * set this value is to avoild SPI_CLK become strange value */
    spi1_config_mode.SPI_CLK = QSPI_CLK_16M;

    qspi_init(1, &spi1_config_mode);

    /*
     * In real application, slave must prepare data before master request!
     * So if using spi as slave in SOC, then it should have one mechaism to
     * let master knows data is available or not...
     * It means that SOC needs one interrupt pin to notify data available.
     *
     */
    while (1)
    {
        printf(" Wait SPI Master send data\r\n");
        slave_xfer_request.write_buf = slave_buffer_send ;
        slave_xfer_request.read_buf = slave_buffer_recv ;
        slave_xfer_request.length      = 10;

        spi_transfer(1, &slave_xfer_request, spi1_finish_callback);
        while (spi1_finish_state == 0) {;}
        spi1_finish_state = 0;
        printf(" SPI Slave Receive 10 bytes Finish OK! \r\n");
        printf(" Receive Master 10 bytes data :");
        for (i = 0; i < 10; i++)
        {
            printf(" 0x%.2X ", slave_buffer_recv[i]);
            slave_buffer_recv[i] = 0x00;
        }

        printf("\r\n");
    }

}

void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
