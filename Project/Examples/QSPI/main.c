/** @file main.c
 *
 * @brief QSPI example main file.
 *
 *
 */
/**
* @defgroup QSPI_example_group  QSPI
* @ingroup examples_group
* @{
* @brief QSPI example demonstrate
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


#define FLASH_QSPI_ID           0           /*select slave 0... here assume QSPI Flash is connect in CS0*/

#define FLASH_SECTOR_SIZE       256


#define QSPI_FLASH_SLAVE_ID     0


#define QSPI0_DATA2             4
#define QSPI0_DATA3             5
#define QSPI0_SCLK              6
#define QSPI0_CS0               7
#define QSPI0_DATA0             8
#define QSPI0_DATA1             9

#define QSPI1_DATA2            14
#define QSPI1_DATA3            15
#define QSPI1_SCLK             28
#define QSPI1_CS0              29
#define QSPI1_DATA0            30
#define QSPI1_DATA1            31

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{

    /*uart0 pinmux, This is default setting,
      we set it for safety. */
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/


#if (FLASH_QSPI_ID==0)
    /*init QSPI pin mux   -- for QSPI0*/
    pin_set_mode( QSPI0_DATA2, MODE_QSPI0);     /*SPI DATA2*/
    pin_set_mode( QSPI0_DATA3, MODE_QSPI0);     /*SPI DATA3*/
    pin_set_mode( QSPI0_SCLK,  MODE_QSPI0);     /*SPI SCLK*/
    pin_set_mode( QSPI0_CS0,   MODE_QSPI0);     /*SPI CS*/
    pin_set_mode( QSPI0_DATA0, MODE_QSPI0);     /*SPI DATA0*/
    pin_set_mode( QSPI0_DATA1, MODE_QSPI0);     /*SPI DATA1*/
#endif

#if (FLASH_QSPI_ID==1)
    /*init QSPI pin mux   -- for QSPI1*/
    pin_set_mode(QSPI1_DATA2, MODE_QSPI1);     /*SPI DATA2*/
    pin_set_mode(QSPI1_DATA3, MODE_QSPI1);     /*SPI DATA3*/
    pin_set_mode(QSPI1_SCLK,  MODE_QSPI1);     /*SPI SCLK*/
    pin_set_mode(QSPI1_CS0,   MODE_QSPI1);     /*SPI CS*/
    pin_set_mode(QSPI1_DATA0, MODE_QSPI1);     /*SPI DATA0*/
    pin_set_mode(QSPI1_DATA1, MODE_QSPI1);     /*SPI DATA1*/
#endif

    return;
}
void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}

void get_flash_status0(uint8_t *status)
{
    qspi_block_request_t   qspi_req;

    uint8_t    command[4], read_buf[4];

    command[0] = 0x05;              /*winbond flash can not use 0x05 to get S1 ?*/

    qspi_req.cmd_buf = command;
    qspi_req.cmd_length = 1;        /*only 1 bytes command*/

    qspi_req.write_buf = NULL;
    qspi_req.write_length = 0;

    qspi_req.read_buf = read_buf;
    qspi_req.read_length = 1;

    qspi_req.data_transfer_mode = QSPI_NORMAL_SPI;

#if (MODULE_ENABLE(SUPPORT_QSPI0_MULTI_CS))
    qspi_req.select_slave_device = QSPI_FLASH_SLAVE_ID;
#endif

    qspi_transfer(FLASH_QSPI_ID, &qspi_req);      /*this command is too short, so use block mode is better.*/

    *status = read_buf[0];

    return;
}

void get_flash_status1(uint8_t *status)
{
    qspi_block_request_t   qspi_req;

    uint8_t    command[4], read_buf[4];

    command[0] = 0x35;

    qspi_req.cmd_buf = command;
    qspi_req.cmd_length = 1;        /*only 1 bytes command*/

    qspi_req.write_buf = NULL;
    qspi_req.write_length = 0;

    qspi_req.read_buf = read_buf;
    qspi_req.read_length = 1;

    qspi_req.data_transfer_mode = QSPI_NORMAL_SPI;

#if (MODULE_ENABLE(SUPPORT_QSPI0_MULTI_CS))
    qspi_req.select_slave_device = QSPI_FLASH_SLAVE_ID;
#endif

    qspi_transfer(FLASH_QSPI_ID, &qspi_req);      /*this command is too short, so use block mode is better.*/

    *status = read_buf[0];

    return;

}

void wait_wip_finish(void)
{
    uint8_t  status;

    while (1)
    {
        get_flash_status0(&status);

        if ((status & 1) == 0)
        {
            break;    /*WIP is 0 */
        }
    }

    return;
}


void write_enable(void)
{
    qspi_block_request_t   qspi_req;
    uint8_t    command[4];

    command[0] = 0x06;              /*write enable*/

    qspi_req.cmd_buf = command;
    qspi_req.cmd_length = 1;        /*1 bytes command*/

    /*write enable*/
    qspi_req.write_buf = NULL;
    qspi_req.write_length = 0;

    qspi_req.read_buf = NULL;
    qspi_req.read_length = 0;

    qspi_req.data_transfer_mode = QSPI_NORMAL_SPI;

#if (MODULE_ENABLE(SUPPORT_QSPI0_MULTI_CS))
    qspi_req.select_slave_device = QSPI_FLASH_SLAVE_ID;
#endif

    qspi_transfer(FLASH_QSPI_ID, &qspi_req);

}

void erase_sector(uint32_t address)
{
    qspi_block_request_t   qspi_req;

    uint8_t     command[4];

    write_enable();

    command[0] = 0x20;              /*sector erase*/

    command[1] = (address >> 16) & 0xFF;
    command[2] = (address >> 8) & 0xFF;
    command[3] = 0x00;              /*page must be 4K alignment, so it must be zero*/

    qspi_req.cmd_buf = command;
    qspi_req.cmd_length = 4;        /*4 bytes command*/

    /*erase sector command do not have any write data or read data*/
    qspi_req.write_buf = NULL;
    qspi_req.write_length = 0;

    qspi_req.read_buf = NULL;
    qspi_req.read_length = 0;

    qspi_req.data_transfer_mode = QSPI_NORMAL_SPI;

#if (MODULE_ENABLE(SUPPORT_QSPI0_MULTI_CS))
    qspi_req.select_slave_device = QSPI_FLASH_SLAVE_ID;
#endif

    qspi_transfer(FLASH_QSPI_ID, &qspi_req);

    /*erase sector will take 40ms typical, so maybe you can use timer to sleep 30ms...*/
    /*Here we use busy polling*/
    wait_wip_finish();

    return;
}

void program_sector(uint32_t flash_address, uint8_t *data_buf, int mode)
{
    qspi_block_request_t   qspi_req;
    uint8_t    command[4];

    write_enable();

    /*program page --- in QFlash, each page is 256 bytes!!*/

    /*Here, we will assume, each time write is write page size
      and the flash address is page-size alignment,*/
    switch (mode)
    {
    case QSPI_QUAD_SPI:
        command[0] = 0x32;              /*Quad read */
        qspi_req.data_transfer_mode = QSPI_QUAD_SPI;    /*QUAD mode*/
        break;

    case QSPI_NORMAL_SPI:
    case QSPI_DUAL_SPI:
    /* Notice: Flash does NOT support dual write command,
     * So we change this dual to single mode.
     */
    default:    /*use one bit */
        command[0] = 0x02;              /*default single read*/
        qspi_req.data_transfer_mode = QSPI_NORMAL_SPI;

    }

    command[1] = (flash_address >> 16) & 0xFF;     /*Here we assume data address in 256 bytes alignment*/
    command[2] = (flash_address >> 8) & 0xFF;      /*so flash_address last 8 bytes is zero!*/
    command[3] = 0x00;

    qspi_req.cmd_buf = command;
    qspi_req.cmd_length = 4;        /*command is 4 bytes command*/

    qspi_req.write_buf = data_buf;
    qspi_req.write_length = FLASH_SECTOR_SIZE;

    qspi_req.read_buf = NULL;
    qspi_req.read_length = 0;

#if (MODULE_ENABLE(SUPPORT_QSPI0_MULTI_CS))
    qspi_req.select_slave_device = QSPI_FLASH_SLAVE_ID;
#endif

    qspi_transfer(FLASH_QSPI_ID, &qspi_req);

    /*Here we use busy polling*/
    wait_wip_finish();

    return;
}


void read_sector(uint32_t flash_address, uint8_t *data_buf, int mode)
{
    qspi_block_request_t   qspi_req;
    uint8_t    command[8];

    switch (mode)
    {
    case QSPI_NORMAL_SPI:
        command[0] = 0x0B;              /*single read */
        qspi_req.data_transfer_mode = QSPI_NORMAL_SPI;    /*single one bit mode*/
        break;
    case QSPI_DUAL_SPI:
        command[0] = 0x3B;              /*Dual real */
        qspi_req.data_transfer_mode = QSPI_DUAL_SPI;    /*dual mode*/
        break;
    case QSPI_QUAD_SPI:
        command[0] = 0x6B;              /*Quad read */
        qspi_req.data_transfer_mode = QSPI_QUAD_SPI;    /*QUAD mode*/
        break;

    default:    /*use one bit */
        command[0] = 0x0B;              /*default single read*/
        qspi_req.data_transfer_mode = QSPI_NORMAL_SPI;

    }

    /*Here we assume data is page size alignment.*/

    command[1] = (flash_address >> 16) & 0xFF;
    command[2] = (flash_address >> 8) & 0xFF;
    command[3] = 0x00;
    command[4] = 0x00;      /*this byte is dummy byte*/

    qspi_req.cmd_buf = command;
    qspi_req.cmd_length = 5;        /*command is 5 bytes command*/

    qspi_req.write_buf = NULL;
    qspi_req.write_length = 0;

    qspi_req.read_buf = data_buf;
    qspi_req.read_length = FLASH_SECTOR_SIZE;

#if (MODULE_ENABLE(SUPPORT_QSPI0_MULTI_CS))
    qspi_req.select_slave_device = QSPI_FLASH_SLAVE_ID;
#endif

    qspi_transfer(FLASH_QSPI_ID, &qspi_req);

    return;
}

void write_status(uint8_t status0, uint8_t status1)
{
    qspi_block_request_t   qspi_req;
    uint8_t    command[4];

    write_enable();

    /*write status*/
    command[0] = 0x01;
    command[1] = status0;
    command[2] = status1;

    qspi_req.cmd_buf = command;
    qspi_req.cmd_length = 3;        /*3 bytes command*/
    qspi_req.data_transfer_mode = QSPI_NORMAL_SPI;

    qspi_req.write_buf = NULL;
    qspi_req.write_length = 0;

    qspi_req.read_buf = NULL;
    qspi_req.read_length = 0;

#if (MODULE_ENABLE(SUPPORT_QSPI0_MULTI_CS))
    qspi_req.select_slave_device = QSPI_FLASH_SLAVE_ID;
#endif

    qspi_transfer(FLASH_QSPI_ID, &qspi_req);

    wait_wip_finish();

    return;
}

volatile uint32_t     dma_finish_state = 0;

void qspi_dma_finish_cb(uint32_t channel_id, uint32_t status)
{
    dma_finish_state = 1;
    return;
}

void program_sector_dma(uint32_t flash_address, uint8_t *data_buf, int mode, uint16_t length)
{
    qspi_block_request_t   qspi_req;
    uint8_t    command[4];

    write_enable();

    /*program page using dma*/

    /* TODO:  we don't check length here. but the length should be <= page size (default page size is 256)
     *
     */

    /*Here, we will assume, each time write is write page size
      and the flash address is page-size alignment,*/
    switch (mode)
    {
    case QSPI_QUAD_SPI:
        command[0] = 0x32;              /*Quad read */
        qspi_req.data_transfer_mode = QSPI_QUAD_SPI;    /*QUAD mode*/
        break;

    case QSPI_NORMAL_SPI:
    case QSPI_DUAL_SPI:
    /* Notice: Flash does NOT support dual write command,
     * So we change this dual to single mode.
     */
    default:    /*use one bit */
        command[0] = 0x02;              /*default single read*/
        qspi_req.data_transfer_mode = QSPI_NORMAL_SPI;

    }

    command[1] = (flash_address >> 16) & 0xFF;
    command[2] = (flash_address >> 8) & 0xFF;
    command[3] = 0x00;

    qspi_req.cmd_buf = command;
    qspi_req.cmd_length = 4;        /*command is 4 bytes command*/

    qspi_req.write_buf = data_buf;
    qspi_req.write_length = length;

    /*qspi_write_dma don't care read_buf.. so it can ignore read_buf and read_length*/
    qspi_req.read_buf = NULL;
    qspi_req.read_length = 0;

#if (MODULE_ENABLE(SUPPORT_QSPI0_MULTI_CS))
    qspi_req.select_slave_device = QSPI_FLASH_SLAVE_ID;
#endif

    /*this is just an example to show we should wait dma finish...*/
    dma_finish_state = 0;

    /*
     * the following function is using dma to send data to spi device.
     * the function is asynchronous function. So we need a callback function
     * to notify the job finish.
     */
    qspi_write_dma(FLASH_QSPI_ID, &qspi_req, qspi_dma_finish_cb);

    while (dma_finish_state == 0)
    {
        /*
         * You can do other FSM job here....
         */

    }

    /*Here we use busy polling*/
    wait_wip_finish();

    return;
}




void read_sector_dma(uint32_t flash_address, uint8_t *data_buf, int mode, uint16_t length)
{
    qspi_block_request_t   qspi_req;
    uint8_t    command[8];

    /*QE bit already enable*/

    switch (mode)
    {
    case QSPI_NORMAL_SPI:
        command[0] = 0x0B;              /*single read */
        qspi_req.data_transfer_mode = QSPI_NORMAL_SPI;    /*single one bit mode*/
        break;

    case QSPI_DUAL_SPI:
        command[0] = 0x3B;              /*Dual real */
        qspi_req.data_transfer_mode = QSPI_DUAL_SPI;    /*DUAL mode*/
        break;

    case QSPI_QUAD_SPI:
        command[0] = 0x6B;              /*Quad read */
        qspi_req.data_transfer_mode = QSPI_QUAD_SPI;    /*QUAD mode*/
        break;

    default:    /*use one bit */
        command[0] = 0x0B;              /*default single read*/
        qspi_req.data_transfer_mode = QSPI_NORMAL_SPI;
    }

    command[1] = (flash_address >> 16) & 0xFF;
    command[2] = (flash_address >> 8) & 0xFF;
    command[3] = 0x00;
    command[4] = 0x00;      /*dummy clocks*/

    qspi_req.cmd_buf = command;
    qspi_req.cmd_length = 5;        /*5 bytes command*/

    /* If we want to use dma to read, data size must be 16*N */
    qspi_req.read_buf = data_buf;
    qspi_req.read_length = length;

    /*qspi_read_dma will ignore write_buf and write_length.*/
    qspi_req.write_buf = NULL;
    qspi_req.write_length = 0;

#if (MODULE_ENABLE(SUPPORT_QSPI0_MULTI_CS))
    qspi_req.select_slave_device = QSPI_FLASH_SLAVE_ID;
#endif

    dma_finish_state = 0;

    /*
     * the following function is using dma to send data to spi device.
     * the function is asynchronous function. So we need a callback function
     * to notify the job finish.
     */
    qspi_read_dma(FLASH_QSPI_ID, &qspi_req, qspi_dma_finish_cb);

    while (dma_finish_state == 0)
    {
        /*
         * You can do other FSM job here....
         *
         */

    }

}

/*************************************************************************************************************
 ***
 ***    This example demostrates how to use RT58x QSPI API to erase/read/write "external QSPI flash".
 ***  External QSPI flash is NOT RT58x internal XIP(execute-in-place) flash.
 ***  External QSPI flash is an additional flash outside RT58x SOC. It can be used a storage device.
 ***  If you want to erase/program/read data in internal flash, please refer to example code in Flash.
 ***
 ***    Until now, what we know for the device using QSPI interface is only QSPI flash. So we use
 ***  QSPI flash to demostrate RT58x QSPI API. If you have other device using QSPI interface, you can
 ***  easily using these functions to control the device.
 ***
 ***    In this example, we use GigaDevice(GD) GDL25LQ80C as external QSPI flash.
 ***  PLEASE read GD25LQ80C datasheet to know the protocol and command set using in this example.
 ***  It will help you to understand this example.
 ***
 *************************************************************************************************************/

int main(void)
{
    qspi_transfer_mode_t      mode;

    uint32_t  i, temp;

    /*test pattern1*/
    uint8_t   write_buf[FLASH_SECTOR_SIZE], read_buf[FLASH_SECTOR_SIZE];
    /*test pattern2*/
    uint8_t   write_buf2[FLASH_SECTOR_SIZE], read_buf2[FLASH_SECTOR_SIZE];

    uint8_t  status0, status1;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("Hello, QSPI TEST \n");
    printf("This QSPI flash is an external flash outside RT58X \n");
    printf("For this test, you need an QSPI flash to connect GPIO4~GPIO9!! \n");
    printf("GPIO4 is QSPI DATA2 \n");
    printf("GPIO5 is QSPI DATA3 \n");
    printf("GPIO6 is QSPI SCLK \n");
    printf("GPIO7 is QSPI CSN0 \n");
    printf("GPIO8 is QSPI DATA2 \n");
    printf("GPIO9 is QSPI DATA3 \n");

    printf("Please read flash document for more instructions command \n");
    printf("\n");


    /* set QSPI tranfer mode. flash support mode 0 or mode 3
     * but rtl simulation only accept mode 3. so we set flash mode 3.
     */
    mode.SPI_CPOL = 1;
    mode.SPI_CPHA = 1 ;

    mode.SPI_BIT_ORDER = SPI_MSB_ORDER;

    mode.SPI_MASTER = SPI_MASTER_MODE;    /*master mode*/

    mode.SPI_CS = QSPI_FLASH_SLAVE_ID;         /*QSPI device is SS0*/
    mode.SPI_CS_POL = SPI_CHIPSEL_ACTIVE_LOW;  /*QSPI device is CS low active*/

    mode.SPI_CLK = QSPI_CLK_32M;        /*We want QSPI device running at 32M*/

    qspi_init(FLASH_QSPI_ID, &mode);

    get_flash_status0(&status0);
    get_flash_status1(&status1);        /*Check QE bit*/

    /*before Quad program, you must check QE bit in status bit is 1*/
    if ((status1 & 0x2) == 0)
    {
        /*QE is 0, we should set it*/
        status1 |= 0x2;      /*set QE bit*/

        write_status(status0, status1);
    }


    erase_sector(0);            /*erase the first sector*/

    /*generated some test pattern for read/write*/

    write_buf[0] = 0x2D;
    write_buf[1] = 0xAB;

    write_buf2[0] = 0xDD;
    write_buf2[1] = 0x7F;

    for (i = 2; i < FLASH_SECTOR_SIZE; i++)
    {
        temp = (write_buf[i - 2] * 17 + write_buf[i - 1] * 53 + 173);
        write_buf[i] =   temp % 251;

        temp = (write_buf2[i - 2] * 17 + write_buf2[i - 1] * 53 + 173);
        write_buf2[i] =   temp % 251;

    }

    /*program data in CPU polling mode. default write 256 bytes.*/
    /*
     *   Please notice: QFLASH each sector is 256 bytes. so you can not program
     * a sector more than 256 bytes. If your data is more than 256 bytes,
     * you should separate the data to several write.
     */

    printf("PIO write/read QSPI by normal mode \n");

    program_sector(0x600, write_buf2, QSPI_NORMAL_SPI);
    /*program data in CPU polling mode. default read 256 bytes.*/
    read_sector(0x600, read_buf2, QSPI_NORMAL_SPI);

    /*compare data*/
    for (i = 0; i < FLASH_SECTOR_SIZE; i++)
    {
        if (write_buf2[i] != read_buf2[i])
        {
            printf("error i:%ld %02x %02x \n", i, write_buf[i], read_buf[i] );
            while (1);
        }

        read_buf2[i] = 0x00; /*clear buffer*/
    }

    printf("data match \n");

    printf("PIO write/read QSPI by QAUD mode.\n");

    /*program data in CPU polling mode. default write 256 bytes. in 4 bits write*/
    program_sector(0x500, write_buf2, QSPI_QUAD_SPI);
    /*program data in CPU polling mode. default read 256 bytes. in 4 bit read*/
    read_sector(0x500, read_buf2, QSPI_QUAD_SPI);

    /*compare data*/
    for (i = 0; i < FLASH_SECTOR_SIZE; i++)
    {
        if (write_buf2[i] != read_buf2[i])
        {
            printf("error i:%ld %02x %02x \n", i, write_buf[i], read_buf[i] );
            while (1);
        }

        read_buf2[i] = 0x00; /*clear buffer*/
    }

    printf("data match \n");

    /*
     * QSPI flash only support write in 1-bit mode or 4-bits mode. !!
     * so you will not see the function for 2-bit write.
     */

    printf("PIO read QSPI by DUAL mode.\n");
    read_sector(0x500, read_buf2, QSPI_DUAL_SPI);

    /*compare data*/
    for (i = 0; i < FLASH_SECTOR_SIZE; i++)
    {
        if (write_buf2[i] != read_buf2[i])
        {
            printf("error i:%ld %02x %02x \n", i, write_buf[i], read_buf[i] );
            while (1);
        }

        read_buf2[i] = 0x00; /*clear buffer*/
    }

    printf("data match \n");


    /*DMA write, normal SPI mode*/
    /*
     *   Please notice: QFLASH each sector is 256 bytes. so you can not program
     * a sector more than 256 bytes. If your data is more than 256 bytes,
     * you should separate the data to several write.
     */

    printf("DMA write/read QSPI by NORMAL mode.\n");

    program_sector_dma(0x300, write_buf, QSPI_NORMAL_SPI, FLASH_SECTOR_SIZE);

    /*DMA read, normal SPI mode*/
    read_sector_dma(0x300, read_buf, QSPI_NORMAL_SPI, FLASH_SECTOR_SIZE);

    /*compare data*/
    for (i = 0; i < FLASH_SECTOR_SIZE; i++)
    {
        if (write_buf[i] != read_buf[i])
        {
            printf("Error \n");
            while (1);
        }
        read_buf[i] = 0x00; /*clear buffer*/
    }

    printf("data match \n");


    printf("DMA write/read QSPI by QUAD mode.\n");

    /*4bit write dma*/
    program_sector_dma(0x100, write_buf, QSPI_QUAD_SPI, FLASH_SECTOR_SIZE);
    /*4bit read dma*/
    read_sector_dma(0x100, read_buf, QSPI_QUAD_SPI, FLASH_SECTOR_SIZE);

    /*compare data*/
    for (i = 0; i < FLASH_SECTOR_SIZE; i++)
    {
        if (write_buf[i] != read_buf[i])
        {
            printf("Error \n");
            while (1);
        }
        read_buf[i] = 0x00; /*clear buffer*/
    }

    printf("data match \n");


    printf("DMA read QSPI by DUAL mode.\n");

    /*2bit read dma*/
    read_sector_dma(0x100, read_buf, QSPI_DUAL_SPI, FLASH_SECTOR_SIZE);

    /*compare data*/
    for (i = 0; i < FLASH_SECTOR_SIZE; i++)
    {
        if (write_buf[i] != read_buf[i])
        {
            printf("Error \n");
            while (1);
        }
        read_buf[i] = 0x00; /*clear buffer*/
    }

    printf("data match \n");

    printf("QSPI verify ok \n");

    while (1)
        ;


}

void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
