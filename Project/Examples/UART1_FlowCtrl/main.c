/** @file main.c
 *
 * @brief UART1_FlowCtrl example main file.
 *
 *
 */
/**
* @defgroup UART1_FlowCtrl_example_group  UART1_Flow_Control
* @ingroup examples_group
* @{
* @brief UART1_Flow_Control example demonstrate
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

#define GPIO28  28
#define GPIO29  29

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/************************************************************
 * In this example, we use uart0 as loop test port
 * To test loopback test, please do NOT call any printf function
 ************************************************************/

/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{


    /*uart0 pinmux, This is default setting,
      we set it for safety. */
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/

    /*uart1 pinmux*/

    pin_set_mode(GPIO28, MODE_UART);     /*GPIO28 as UART1 TX */
    pin_set_mode(GPIO29, MODE_UART);     /*GPIO29 as UART1 RX*/

    pin_set_mode(20, MODE_UART);     /*GPIO20 as UART1 RTS*/
    pin_set_mode(21, MODE_UART);     /*GPIO21 as UART1 CTS*/

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
 * So it use state polling...
 * This is demo code only
 */


volatile uint32_t tx_finish = 0, rx_finish = 0;

void uart1_callback(uint32_t event, void *p_context)
{
    /*Notice:
        UART_EVENT_TX_DONE  is for asynchronous mode send
        UART_EVENT_RX_DONE  is for synchronous  mode receive

        if system wants to use p_context as parameter, it can cast
        the type of p_context to original type.  like

        uint32_t  phandle;

        phandle = (uint32_t *) p_context;

     */


    if (event & UART_EVENT_TX_DONE)
    {
        /*if you use multi-tasking, signal the waiting task here.*/
        tx_finish = 1;
    }

    if (event & UART_EVENT_RX_DONE)
    {
        /*if you use multi-tasking, signal the waiting task here.*/
        rx_finish = 1;
    }

    if (event & (UART_EVENT_RX_OVERFLOW | UART_EVENT_RX_BREAK |
                 UART_EVENT_RX_FRAMING_ERROR | UART_EVENT_RX_PARITY_ERROR ))
    {

        //it's almost impossible for those error case.
        //do something ...

    }

}

volatile uint32_t  test_count = 0;
void timer_handler(uint32_t timer_id)
{
    /*
     *SET  state=1, host side can send data to us.
     *Clear state=0, host side SHOULD STOP to send data!
     */
    uart_set_modem_status(1, (test_count & 1));

    test_count++;
}

void init_timer(void)
{
    timer_config_mode_t cfg;

    cfg.mode = TIMER_PERIODIC_MODE;
    /*the input clock is 32M/s, so it will become 1M ticks per second */
    cfg.prescale = TIMER_PRESCALE_32;
    cfg.int_en = 1;

    Timer_Open(0, cfg, timer_handler);
    Timer_Start(0, 122343);         /*just a random number to set RTS*/

}

#define TESTBLOCKSIZE   512

int main(void)
{
    /* Here we assume max loop data is 512 bytes
     * Please notice: decarle a large array inside one function is BAD/STUPID
     * design... it will occupy stack resource. So if you want to defined an array
     * for internal used in compiler time, please declare the array as "static", it
     * will NOT occupy the stack. Or you can dynamic allocate the buffer in
     * run-time method...(heap or some directly pointer assign)
     */
    static uint8_t   sendbuf[TESTBLOCKSIZE], recvbuf[TESTBLOCKSIZE];

    uint32_t  i, length, temp;

    uart_config_t  uart1_drv_config;

    /* if we want uart1_callback to process parameter p_context, then this parameter
     * can NOT be allocated in stack... so we allocate it in "local global" space.
     */
    static uint32_t  handle;


    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);
    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/

    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("uart1 Flow control receive/send demo \n");
    printf("Please loopback uart1 TX GPIO28 with uart1 RX GPIO29 for this test \n");
    printf("In this example RTS is GPIO21, CTS is GPIO20 \n");
    printf("Please read readme.txt for this example behavior\n");

    init_timer();

    handle = 0;

    /*init uart1, 115200, 8bits 1 stopbit, none parity, no flow control.*/
    uart1_drv_config.baudrate = UART_BAUDRATE_115200;
    uart1_drv_config.databits = UART_DATA_BITS_8;
    uart1_drv_config.hwfc     = UART_HWFC_ENABLED;          /*Only UART1 support hardware flow control.*/
    uart1_drv_config.parity   = UART_PARITY_NONE;

    /* Important: p_contex will be the second parameter in uart callback.
     * In this example, we do NOT use p_context, (So we just use handle for sample)
     * but you can use it for whaterever you want. (It can be NULL, too)
     */
    uart1_drv_config.p_context = (void *) &handle;

    uart1_drv_config.stopbit  = UART_STOPBIT_ONE;
    uart1_drv_config.interrupt_priority = IRQ_PRIORITY_NORMAL;

    uart_init(1, &uart1_drv_config, uart1_callback);

    /*generate some random pattern for test*/
    sendbuf[0] = 0xBE;
    sendbuf[1] = 0xEF;
    recvbuf[0] = 0;
    recvbuf[1] = 0;

    for (i = 2; i < TESTBLOCKSIZE; i++)
    {
        temp = (sendbuf[i - 2] * 97) + (sendbuf[i - 1] * 127) + 46;
        sendbuf[i] = temp & 0xFF;
        recvbuf[i] = 0;
    }


    while (1)
    {

        for (length = 1; length < TESTBLOCKSIZE; length++)
        {
            printf(".");

            uart_rx(1, recvbuf, length);

            uart_tx(1, sendbuf, length);

            /*  for multi-tasking, you can wait_event function here,
             *  interrupt callback can signal_event to wakeup this task.
             *  In this example, we just use simple busy-waiting polling
             */
            while (tx_finish == 0)
                ;

            while (rx_finish == 0)
                ;

            tx_finish = 0;
            rx_finish = 0;

            for (i = 0; i < length; i++)
            {

                if (sendbuf[i] != recvbuf[i])
                {
                    printf("what's wrong !\n");
                    while (1);
                }

                recvbuf[i] = 0;
            }
        }

    }//end for while(1)

}

void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
