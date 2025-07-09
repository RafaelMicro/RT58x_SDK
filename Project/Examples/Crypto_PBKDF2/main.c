
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"

#include "project_config.h"

#include "uart_drv.h"
#include "retarget.h"

#include "crypto.h"
#include "sha256.h"

#include "rf_mcu_ahb.h"
int main(void);

void SetClockFreq(void);

/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

#define GPIO20  20
#define GPIO21  21

#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/************************************************************/

/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    /*uart0 pinmux*/
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 TX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 RX*/

    return;
}
void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}
/*In this test case, we limit generate test key length less than 64 bytes.*/
#define maximum_test_key_length          64

/*temp use buffer*/
uint8_t               key_buffer[maximum_test_key_length];

struct pbkdf2_struct t1 =
{ (uint8_t *)"password", 8, (uint8_t *)"salt", 4, 1, 32, key_buffer};


struct pbkdf2_struct t2 =
{ (uint8_t *)"password", 8, (uint8_t *)"salt", 4, 2, 32, key_buffer};


struct pbkdf2_struct t3 =
{ (uint8_t *)"password", 8, (uint8_t *)"salt", 4, 4096, 32, key_buffer};

struct pbkdf2_struct t4 =
{ (uint8_t *)"password", 8, (uint8_t *)"salt", 4, 20000, 32, key_buffer};

struct pbkdf2_struct t5 =
{
    (uint8_t *)"passwordPASSWORDpassword", 24,
    (uint8_t *)"saltSALTsaltSALTsaltSALTsaltSALTsalt", 36, 4096, 40, key_buffer
};

struct pbkdf2_struct t6 =
{
    (uint8_t *)"pass\0word", 9, (uint8_t *)"sa\0lt", 5, 4096, 16, key_buffer
};

/*test for matter pbkdf2*/

struct pbkdf2_struct mt1 =
{
    (uint8_t *)"password", 8, (uint8_t *)"saltSALTsaltSALT", 16, 1, 20, key_buffer
};

struct pbkdf2_struct mt2 =
{
    (uint8_t *)"password", 8, (uint8_t *)"saltSALTsaltSALT", 16, 2, 20, key_buffer
};

struct pbkdf2_struct mt3 =
{
    (uint8_t *)"password", 8, (uint8_t *)"saltSALTsaltSALT", 16, 5, 20, key_buffer
};

struct pbkdf2_struct mt4 =
{
    (uint8_t *)"password", 8, (uint8_t *)"saltSALTsaltSALT", 16, 3, 20, key_buffer
};

struct pbkdf2_struct mt5 =
{
    (uint8_t *)"passwordPASSWORDpassword", 24, (uint8_t *)"saltSALTsaltSALTsaltSALTsaltSALT", 32, 10, 25, key_buffer
};

const uint8_t expect_t1[] =
{
    0x12, 0x0f, 0xb6, 0xcf, 0xfc, 0xf8, 0xb3, 0x2c,
    0x43, 0xe7, 0x22, 0x52, 0x56, 0xc4, 0xf8, 0x37,
    0xa8, 0x65, 0x48, 0xc9, 0x2c, 0xcc, 0x35, 0x48,
    0x08, 0x05, 0x98, 0x7c, 0xb7, 0x0b, 0xe1, 0x7b
};

const uint8_t expect_t2[] =
{
    0xae, 0x4d, 0x0c, 0x95, 0xaf, 0x6b, 0x46, 0xd3,
    0x2d, 0x0a, 0xdf, 0xf9, 0x28, 0xf0, 0x6d, 0xd0,
    0x2a, 0x30, 0x3f, 0x8e, 0xf3, 0xc2, 0x51, 0xdf,
    0xd6, 0xe2, 0xd8, 0x5a, 0x95, 0x47, 0x4c, 0x43
};

const uint8_t expect_t3[] =
{
    0xc5, 0xe4, 0x78, 0xd5, 0x92, 0x88, 0xc8, 0x41,
    0xaa, 0x53, 0x0d, 0xb6, 0x84, 0x5c, 0x4c, 0x8d,
    0x96, 0x28, 0x93, 0xa0, 0x01, 0xce, 0x4e, 0x11,
    0xa4, 0x96, 0x38, 0x73, 0xaa, 0x98, 0x13, 0x4a
};


const uint8_t expect_t4[] =
{
    0x2A, 0x6A, 0x4F, 0x08, 0x32, 0xD0, 0x46, 0x83,
    0x8F, 0x4A, 0x22, 0xCB, 0xAD, 0xC9, 0x4D, 0xFF,
    0x08, 0xA5, 0xBC, 0xC3, 0x28, 0x46, 0x7E, 0xE3,
    0xCA, 0xDE, 0x8D, 0xB8, 0xC5, 0xD9, 0x3B, 0x87
};

const uint8_t expect_t5[] =
{
    0x34, 0x8c, 0x89, 0xdb, 0xcb, 0xd3, 0x2b, 0x2f,
    0x32, 0xd8, 0x14, 0xb8, 0x11, 0x6e, 0x84, 0xcf,
    0x2b, 0x17, 0x34, 0x7e, 0xbc, 0x18, 0x00, 0x18,
    0x1c, 0x4e, 0x2a, 0x1f, 0xb8, 0xdd, 0x53, 0xe1,
    0xc6, 0x35, 0x51, 0x8c, 0x7d, 0xac, 0x47, 0xe9
};

const uint8_t expect_t6[] =
{
    0x89, 0xb6, 0x9d, 0x05, 0x16, 0xf8, 0x29, 0x89,
    0x3c, 0x69, 0x62, 0x26, 0x65, 0x0a, 0x86, 0x87
};


const uint8_t expect_mt1[] =
{
    0xf2, 0xe3, 0x4b, 0xd9, 0x50, 0xe9, 0x1c, 0xf3,
    0x7d, 0x22, 0xe1, 0x13, 0x5a, 0x39, 0x9b, 0x02,
    0xa1, 0x7c, 0xb1, 0x93
};

const uint8_t expect_mt2[] =
{
    0x2b, 0x77, 0x27, 0x5c, 0xc3, 0x12, 0x0b, 0x15,
    0x13, 0xf6, 0xf3, 0xe0, 0x36, 0x49, 0xfd, 0x49,
    0x33, 0x76, 0x52, 0x60
};


const uint8_t expect_mt3[] =
{
    0x82, 0xff, 0xaa, 0xfc, 0x0b, 0x04, 0x91, 0x80,
    0xee, 0xa7, 0x9a, 0x04, 0x10, 0x31, 0x58, 0x87,
    0xb6, 0x60, 0xac, 0x7e
};

const uint8_t expect_mt4[] =
{
    0xf8, 0x8a, 0xfb, 0xb7, 0x9d, 0xda, 0x3f, 0x28,
    0x2e, 0x21, 0xad, 0xf2, 0x53, 0xd0, 0xe9, 0xf1,
    0x70, 0x82, 0x3a, 0x9f
};

const uint8_t expect_mt5[] =
{
    0x0d, 0xbf, 0x87, 0x38, 0xd2, 0x30, 0xbf, 0x28,
    0xba, 0xe0, 0xfb, 0x4d, 0x8f, 0x07, 0x34, 0x98,
    0x24, 0xb5, 0xe0, 0xb1, 0xa7, 0x0b, 0xa2, 0x19,
    0x3b
};

const uint32_t  *test_vector_case[]  =
{
    (uint32_t *) &t1, (uint32_t *) &t2, (uint32_t *) &t3, (uint32_t *) &t4,
    (uint32_t *) &t5, (uint32_t *) &t6,
    (uint32_t *) &mt1, (uint32_t *) &mt2, (uint32_t *) &mt3, (uint32_t *) &mt4,
    (uint32_t *) &mt5
};

const uint8_t *expect_result[] =
{
    (uint8_t *) &expect_t1[0], (uint8_t *) &expect_t2[0], (uint8_t *) &expect_t3[0], (uint8_t *) &expect_t4[0],
    (uint8_t *) &expect_t5[0], (uint8_t *) &expect_t6[0],
    (uint8_t *) &expect_mt1[0], (uint8_t *) &expect_mt2[0],  (uint8_t *) &expect_mt3[0], (uint8_t *) &expect_mt4[0],
    (uint8_t *) &expect_mt5[0]
};

#define test_item   (sizeof(test_vector_case)/sizeof(uint32_t))

void test_pbkdf2_case(uint32_t i, struct pbkdf2_struct  *test_vector)
{
    uint32_t result;

    result = pbkdf2_hmac(test_vector);

    if (result != STATUS_SUCCESS)
    {
        printf("Oops, PBKDF2 case %ld input errorr \n", i);
        while (1);
    }

    if ( memcmp( (uint8_t *) (test_vector->key_output), (expect_result[i]), test_vector->key_length) != 0)
    {
        printf("Oops, PBKDF2 case %ld errorr \n", i);
        while (1);
    }

    printf("PBKDF2 case %ld success \n", i);
}


int main(void)
{
    uint32_t              i;

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    sha256_vector_init();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("Hello Crypto PBKDF2 test %s %s\n", __DATE__, __TIME__);

    printf("test item is %d \n", test_item);

    for (i = 0; i < test_item; i++)
    {
        test_pbkdf2_case(i, (struct pbkdf2_struct *) (test_vector_case[i]));
    }

    printf("test PBKDF2 all successful \n");

    while (1);
}


void SetClockFreq(void)
{
    return;
}


