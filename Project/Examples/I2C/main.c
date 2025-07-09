/** @file main.c
 *
 * @brief I2C_Read_Write example main file.
 *
 *
 */
/**
* @defgroup I2C_Read_Write_example_group  I2C
* @ingroup examples_group
* @{
* @brief I2C example demonstrate
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

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

#define GPIO22_SCL           22
#define GPIO23_SDA           23

#define TIMER0_ID             0
#define END_TICK           5000         /*Write ATEML I2C flash in 3.3V needs 5ms. */

#define I2C_WRITE_SECTOR     64         /*according to AT24C... the maximum write page size is 64 bytes only!*/
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
int main(void);

void SetClockFreq(void);

/************************************************************/

volatile uint32_t   g_transfer_done;
volatile uint32_t   g_i2c_status;

void i2c_finish(uint32_t status)
{
    g_i2c_status = status;
    g_transfer_done = TRUE;
}

/************************************************************/


void do_i2c_master_test(void)
{
    i2c_slave_data_t  dev_cfg;
    uint32_t   status;
    uint32_t   i, temp, read_bytes;

    /*Please notice: I2C flash maximum write sector is 64 bytes...*/
    uint8_t    data[I2C_WRITE_SECTOR] = {0x55, 0xAA, 0xC4, 0x37, 0x12, 0x46, 0xC9, 0x33, 0x18, 0xFF, 0x38};
    uint8_t    rdata[I2C_WRITE_SECTOR];


    printf("In following example test, we try to send data to I2C address 0x47 device \n");
    printf("But we do NOT have this I2C device outside, so we will get NOACK error! \n");
    printf("It is correct behavior!!\n");

    i2c_init(I2C_CLOCK_400K);           /*I2C run in 400KHz...*/

    g_transfer_done = FALSE;

    dev_cfg.dev_addr = 0x47;          /*Device 0x47 does NOT exist.*/
    dev_cfg.reg_addr = 0x00;
    dev_cfg.bFlag_16bits = 0x00;      /*register address is 8 bit mode*/
    status = i2c_write(&dev_cfg, data, 11, i2c_finish);

    /* Notice: Because i2c_write is non-block function,
     * it means that data still in progress for tranfer..
     * So you can NOT modify data buffer before tranfer_done!!
     * Before I2C transfer complete, we should not transfer another I2C command
     */

    while (g_transfer_done == FALSE)      /*Busy waiting I2C complete*/
        ;

    if (g_i2c_status == I2C_STATUS_ERR_NOACK)
    {
        /*This return error status is correct, because we don't have 0x47 I2C device in bus*/
        printf("No I2C device %x \n", dev_cfg.dev_addr);
    }

    printf("next i2c test \n");

    /*generate some test pattern to write.. this is simple loop to generate random number */
    for (i = 11; i < I2C_WRITE_SECTOR; i++)
    {
        temp = ((data[i - 10] * data[i - 9]) - data[i - 7]);
        temp = ((temp & 0x00FF0000) >> 16) + ((temp & 0x0000FF00) >> 8) + (temp & 0xFF);
        data[i] = temp & 0xFF;
    }

    printf("Now we will try to write 64 bytes data to ATMEL EEPROM \n");

    /*this is I2C write for ATMEL I2C */
    dev_cfg.dev_addr = 0x50;      /*ATEML I2C flash address.*/
    dev_cfg.reg_addr = 0x0000;
    dev_cfg.bFlag_16bits = 0x01;      /*register address is 16 bit mode*/

    g_transfer_done = FALSE;

    status = i2c_write( &dev_cfg, data, I2C_WRITE_SECTOR, i2c_finish);

    /*wait write finish.... when write finish, an interrupt will generated to notify the ISR callback function.*/
    while (g_transfer_done == FALSE)
        ;

    if (g_i2c_status == I2C_STATUS_ERR_NOACK)
    {
        /*Almost impossible for code here.*/
        printf("Why I2C NO ACK ERROR?\n");
    }

    /* According to spec, ATEML I2C EEPROM
     * write should wait a period for write*/

    Timer_Start(TIMER0_ID, END_TICK);

    while (Timer_Current_Get(TIMER0_ID) <= END_TICK)
    {
    }

    Timer_Stop(TIMER0_ID);


    printf("Now, we will try to read 1~64 bytes from ATMEL I2C EEPROM \n");

    dev_cfg.dev_addr = 0x50;      /*ATEML I2C flash address.*/
    dev_cfg.reg_addr = 0x0000;
    dev_cfg.bFlag_16bits = 0x01;


    for (i = 1; i < I2C_WRITE_SECTOR; i++)
    {

        memset(rdata, 0, I2C_WRITE_SECTOR);     /*reset buffer to zero.*/
        read_bytes = i;

        g_transfer_done = FALSE;

        /*read_bytes can NOT be zero!!*/
        status = i2c_read( &dev_cfg, rdata, read_bytes, i2c_finish);

        if (status != STATUS_SUCCESS)
        {
            printf("i2c_read error why \n");
        }

        while (g_transfer_done == FALSE)
        {
            /*wait I2C finish transfer*/
        }

        if (g_i2c_status == I2C_STATUS_ERR_NOACK)
        {
            /*should not happened... almost impossible...*/
            printf("Why I2C NO ACK ERROR?\n");

            while (1);
        }

        /*compare data*/
        for (temp = 0; temp < read_bytes; temp++)
        {
            if (rdata[temp] != data[temp])
            {
                printf("data mismatch \n");     /*Oops.... are you sure ATMEL flash is good IC??*/
                while (1);
            }
        }

        printf(".");
    }

    printf("\n");

    printf("I2C read write verify OK \n");

}



/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    /*uart0 pinmux is default*/

    /*
     * call i2c_preinit function to set SCL and SDA pin mode,
     * This function also set SCL and SDA as open drian mode.
     */
    i2c_preinit(GPIO22_SCL, GPIO23_SDA);

    return;
}
void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}
void init_timer(void)
{
    timer_config_mode_t cfg;

    cfg.mode = TIMER_FREERUN_MODE;
    /*the input clock is 32M/s, so it will become 1M ticks per second */
    cfg.prescale = TIMER_PRESCALE_32;

    Timer_Open(TIMER0_ID, cfg, NULL);
}

int main(void)
{
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("Hello World RT58x I2C \n");
    printf("To test this I2C example, you need an extra ATEML I2C EEPROM, like AT24C128 or AT24C256 \n");
    printf("Please connect EEPROM SCL to GPIO22, SDA to GPIO23 \n");
    printf("Please remember to connect VCC and GROUND of the EPROM, too\n");
    printf("We use AT24C EEPROM I2C address is 0x50 ...A1 A0 is zero \n");


    init_timer();

    /*test I2C master*/
    do_i2c_master_test();

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
