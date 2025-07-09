/** @file main.c
 *
 * @brief Comparator example main file.
 *        Demonstrate Compare the voltage comparator initial and configuration that to get output result
 *        Compare input voltage by Comparator, and trigger Comparator interrupt.
 */
/**
* @defgroup Comparator_example_group  Comparator
* @ingroup examples_group
* @{
* @brief Comparator example demonstrate
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
#define COMP_VBAT_DEMO           0
#define COMP_PCH_IOLDO_DEMO      1
#define COMP_DCDCLLDO_NCH_DEMO   2
#define COMP_PCH_NCH_DEMO        3

#define COMP_DEMO_CASE           COMP_VBAT_DEMO


#define GPIO0   0
#define GPIO1   1
#define GPIO2   2
#define GPIO3   3

#define RX_BUF_SIZE   128
#define TX_BUF_SIZE   128

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */
#define PRINTF_BAUDRATE      UART_BAUDRATE_115200
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/


/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
comp_config_t p_comp_config;


/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/


/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/

/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    int i;

    /*set all pin to gpio, except GPIO16, GPIO17 */
    for (i = 0; i < 32; i++)
    {
        if ((i != 16) && (i != 17))
        {
            pin_set_mode(i, MODE_GPIO);
        }
    }

    /*uart0 pinmux*/
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/

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
 * @brief Comparator Interrupt callback function
 *        Enters the function is meant the comparator interrupt event is triggered.
 * @return None
 */

void Comp_Int_Callback_Handler(void)
{
    printf("Comparator interrupt is triggered, ");

    gpio_pin_clear(GPIO0);

    if (COMP_OUT_GET())
    {
        printf("output high. \n");
        gpio_pin_set(GPIO1);
    }
    else
    {
        printf("output low. \n");
        gpio_pin_clear(GPIO1);
    }
    COMP_INT_CLEAR();
    gpio_pin_set(GPIO0);
}

/**
 * @brief Comparator Config function
 * @param[in] p_config
 *          \arg comp_selref        Comparator selref
 *          \arg comp_selinput      Comparator input P source select
 *          \arg comp_ref_sel       Comparator input N source select
 *          \arg comp_pw            Comparator current select
 *          \arg comp_hys_sel       Comparator hys window select
 *          \arg comp_swdiv         Comparator vdd div type switch
 *          \arg comp_ds_wakeup     Comparator wakeup in DeepSleep
 *          \arg comp_ds_inv        Comparator output invert for waking up from DeepSleep
 *          \arg comp_psrr          Comparator PSRR select
 *          \arg comp_tc            Comparator temperature coefficient select
 *          \arg comp_en_start      Comparator en start select
 *          \arg comp_int_pol       Comparator interrupt polarity
 *          \arg comp_ch_sel        Comparator AIO channel
 *          \arg comp_v_sel         Comparator internal vdd div voltage
 * @retval STATUS_SUCCESS config comparator registers is vaild
 * @retval STATUS_INVALID_PARAM p_config value is invaild
 */
uint32_t Comp_Config(comp_config_t *p_config)
{
    if (p_config == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

#if (COMP_DEMO_CASE == COMP_VBAT_DEMO)
    p_config->comp_selref = COMP_CONFIG_SELREF_INTERNAL;          /*Select the Comparator input N source: BG 0.6V*/
    p_config->comp_ref_sel = COMP_CONFIG_REFSEL_CHANNEL_0;        /*Select the Comparator N input for comparision: Channel 0*/
    p_config->comp_selinput = COMP_CONFIG_SELINPUT_INTERNAL;      /*Select the Comparator input P source: Internal vdd div*/
    p_config->comp_ch_sel = COMP_CONFIG_CHSEL_CHANNEL_0;          /*Select the Comparator P input for comparision: Channel 0*/
#elif (COMP_DEMO_CASE == COMP_PCH_IOLDO_DEMO)
    p_config->comp_selref = COMP_CONFIG_SELREF_EXTERNAL;          /*Select the Comparator input N source: external*/
    p_config->comp_ref_sel = COMP_CONFIG_REFSEL_IOLDO;            /*Select the Comparator N input for comparision: IOLDO*/
    p_config->comp_selinput = COMP_CONFIG_SELINPUT_EXTERNAL;      /*Select the Comparator input P source: external*/
    p_config->comp_ch_sel = COMP_CONFIG_CHSEL_CHANNEL_7;          /*Select the Comparator P input for comparision: Channel 7*/
#elif (COMP_DEMO_CASE == COMP_DCDCLLDO_NCH_DEMO)
    p_config->comp_selref = COMP_CONFIG_SELREF_EXTERNAL;          /*Select the Comparator input N source: external*/
    p_config->comp_ref_sel = COMP_CONFIG_REFSEL_CHANNEL_7;        /*Select the Comparator N input for comparision: Channel 7*/
    p_config->comp_selinput = COMP_CONFIG_SELINPUT_EXTERNAL;      /*Select the Comparator input P source: external*/
    p_config->comp_ch_sel = COMP_CONFIG_CHSEL_AVDD_1V;            /*Select the Comparator P input for comparision: DCDC/LLDO AVDD 1.2V*/
#elif (COMP_DEMO_CASE == COMP_PCH_NCH_DEMO)
    p_config->comp_selref = COMP_CONFIG_SELREF_EXTERNAL;          /*Select the Comparator input N source: external*/
    p_config->comp_ref_sel = COMP_CONFIG_REFSEL_CHANNEL_5;        /*Select the Comparator N input for comparision: Channel 5*/
    p_config->comp_selinput = COMP_CONFIG_SELINPUT_EXTERNAL;      /*Select the Comparator input P source: external*/
    p_config->comp_ch_sel = COMP_CONFIG_CHSEL_CHANNEL_7;          /*Select the Comparator P input for comparision: Channel 7*/
#endif

    p_config->comp_swdiv = COMP_CONFIG_SWDIV_RES;                 /*Switch the Comparator vdd div type: RES div*/
    p_config->comp_v_sel = (comp_config_v_sel_t)COMP_V_SEL_GET(); /*Select the Comparator internal vdd div voltage for Power Fail: By MP setting*/
    p_config->comp_int_pol = COMP_CONFIG_INT_POL_BOTH;            /*Set the Comparator interrupt polarity: Both edge*/
    p_config->comp_ds_wakeup = COMP_CONFIG_DS_WAKEUP_ENABLE;      /*Select the Comparator wakeup in DeepSleep: Enable*/
    p_config->comp_ds_inv = COMP_CONFIG_DS_INVERT_DISABLE;        /*Select the invert of the Comparator output for waking up from DeepSleep: Disable*/

    p_config->comp_pw = COMP_CONFIG_PW_LARGEST;                   /*Select the Comparator current: Largest*/
    p_config->comp_hys_sel = COMP_CONFIG_HYS_130;                 /*Select the Comparator hys window: 130mV*/

    return STATUS_SUCCESS;
}


int main(void)
{
    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.
    /*debug used gpio*/
    gpio_pin_set(GPIO0);
    gpio_cfg_output(GPIO0);
    gpio_pin_set(GPIO1);
    gpio_cfg_output(GPIO1);
    gpio_pin_set(GPIO2);
    gpio_cfg_output(GPIO2);
    gpio_pin_set(GPIO3);
    gpio_cfg_output(GPIO3);

    printf("\nComparator enable!!!\n");

    Comp_Config(&p_comp_config);
    Comp_Init(&p_comp_config, Comp_Int_Callback_Handler);    /*Init Comparator*/

    Comp_Enable();                                           /*Enable Comparator*/
    Comp_Enable_At_Sleep();                                  /*Enable Comparator at Sleep*/
    Comp_Enable_At_DeepSleep();                              /*Enable Comparator at DeepSleep*/

    printf("Comparator Vsel: %d\n", COMP_V_SEL_GET());

    while (1)
    {
        gpio_pin_toggle(GPIO3);

        if (COMP_OUT_GET())
        {
            gpio_pin_set(GPIO2);
        }
        else
        {
            gpio_pin_clear(GPIO2);
        }
    }
}
/** @} */ /* end of examples group */
