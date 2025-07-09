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
* @brief SPI_DMA_Master example demonstrate
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
#define SPI_CLK_PIN     6
#define SPI_CS_PIN      7
#define SPI_MOSI_PIN    8
#define SPI_MISO_PIN    9
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

static uint8_t    master_buffer_send[TESTBLOCKSIZE],  master_buffer_recv[TESTBLOCKSIZE];
volatile uint32_t gpio0_interrupt_flag = 0;
volatile uint32_t     spi0_finish_state = 0;

void gpio0_isr_callback(uint32_t pin, void *isr_param)
{
    gpio0_interrupt_flag = 1;
    return;
}

void spi0_finish_callback(uint32_t channel_id, uint32_t status)
{
    spi0_finish_state = 1;
    return;
}


/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{

    /*uart0 pinmux, This is default setting,
      we set it for safety. */
    pin_set_mode(UART0_RX_PIN, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(UART0_TX_PIN, MODE_UART);     /*GPIO17 as UART0 TX*/


    /*init SPI pin mux   -- for SPI0*/
    pin_set_mode( SPI_CLK_PIN, MODE_QSPI0);     /*SPI SCLK*/
    pin_set_mode( SPI_CS_PIN, MODE_QSPI0);     /*SPI CS*/
    pin_set_mode( SPI_MOSI_PIN, MODE_QSPI0);     /*SPI DATA0*/
    pin_set_mode( SPI_MISO_PIN, MODE_QSPI0);     /*SPI DATA1*/

    pin_set_mode( GPIO_0, MODE_GPIO);     /*SPI DATA1*/
    gpio_cfg_input(GPIO_0, GPIO_PIN_INT_BOTH_EDGE);
    gpio_register_isr(GPIO_0, gpio0_isr_callback, NULL);
    gpio_int_enable(GPIO_0);
    gpio_debounce_enable(GPIO_0);
    gpio_set_debounce_time(DEBOUNCE_SLOWCLOCKS_1024);

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




int main(void)
{
    uint32_t         i;

    qspi_transfer_mode_t    spi0_config_mode, spi1_config_mode;

    spi_block_request_t     master_xfer_request, slave_xfer_request;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf(" Spi master dma example\r\n");

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
    printf(" SPI Master --- SPI Slave \r\n");
    printf(" SCLK: GPIO 06 --- GPIO28 \r\n");
    printf(" CS  : GPIO 07 --- GPIO29 \r\n");
    printf(" MOSI: GPIO 08 --- GPIO30 \r\n");
    printf(" MISO: GPIO 09 --- GPIO31 \r\n");

    printf(" In this example, SPI0 is master\r\n");
    printf(" SPI using DMA to transfer data \r\n");


    /*generate some test pattern for test*/

    memset(master_buffer_recv, 0xFF, TESTBLOCKSIZE);

    for (i = 0; i < TESTBLOCKSIZE; i++)
    {
        master_buffer_send[i] = (i + 0x01);
    }

    /*In this example, we test mode0, SPI clock is 16MHz */
    spi0_config_mode.SPI_BIT_ORDER =  SPI_MSB_ORDER;
    spi0_config_mode.SPI_CPOL  =  0;
    spi0_config_mode.SPI_CPHA  =  0;
    spi0_config_mode.SPI_CS_POL = SPI_CHIPSEL_ACTIVE_LOW;    /*CS low active, idled in high */
    spi0_config_mode.SPI_MASTER = SPI_MASTER_MODE;           /*controller spi0 as SPI host*/
    spi0_config_mode.SPI_CS = SPI_SELECT_SLAVE_0;            /*Select SS0 */
    spi0_config_mode.SPI_CLK = QSPI_CLK_16M;
    qspi_init(0, &spi0_config_mode);

    while (1)
    {
        printf(" Press button 0 to send data\r\n");
        while (gpio_pin_get(0)) {;}
        gpio0_interrupt_flag = 0;
        master_xfer_request.write_buf = master_buffer_send;
        master_xfer_request.read_buf = master_buffer_recv;
        master_xfer_request.length      = 10;
        spi_transfer(0, &master_xfer_request, spi0_finish_callback);
        while ( spi0_finish_state == 0) {;}
        spi0_finish_state = 0;

        printf(" SPI Master Send 10 bytes Finish OK! \r\n");
        printf(" Receive Slave 10 bytes data :");
        for (i = 0; i < 10; i++)
        {
            printf(" 0x%.2X ", master_buffer_recv[i]);
            master_buffer_recv[i] = 0x00;
        }
        printf("\r\n");
        Delay_ms(200);
    }
}

void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
