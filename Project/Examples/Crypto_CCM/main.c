/** @file main.c
 *
 * @brief Crypto_CCM example main file.
 *
 *
 */
/**
* @defgroup Crypto_CCM_example_group  Crypto_CCM
* @ingroup examples_group
* @{
* @brief Crypto_CCM example demonstrate
*/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"

#include "project_config.h"

#include "uart_drv.h"
#include "retarget.h"

#include "crypto.h"

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

#define GPIO20  20
#define GPIO21  21



/************************************************************/

/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    /*set gpio 20 21 for debug used*/
    pin_set_mode(GPIO20, MODE_GPIO);
    pin_set_mode(GPIO21, MODE_GPIO);


    gpio_cfg_output(GPIO20);
    /*default output low.*/
    gpio_pin_clear(GPIO20);
    gpio_pin_set(GPIO20);

    gpio_cfg_output(GPIO21);
    /*default output low.*/
    gpio_pin_clear(GPIO21);

    /*uart2 pinmux*/
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 TX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 RX*/

    pin_set_mode(6, MODE_UART);      /*GPIO6 as UART2 TX*/
    pin_set_mode(31, MODE_UART);     /*GPIO31 as UART2 RX*/

    return;
}
void Comm_Subsystem_Disable_LDO_Mode(void)
{
    uint8_t reg_buf[4];

    RfMcu_MemoryGetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
    reg_buf[0] &= ~SUBSYSTEM_CFG_LDO_MODE_DISABLE;
    RfMcu_MemorySetAhb(SUBSYSTEM_CFG_PMU_MODE, reg_buf, 4);
}

//++++++++++++++++++ AES ++++++++++++++++++

#if MODULE_ENABLE(CRYPTO_AES_ENABLE)


#define NUM_NIST_KEYS 16
#define NONCE_LEN 13
#define HEADER_LEN 8

#define M_LEN8 8
#define M_LEN10 10

#define DATA_BUF_LEN23 23
#define DATA_BUF_LEN24 24
#define DATA_BUF_LEN25 25

#define EXPECTED_BUF_LEN31 31
#define EXPECTED_BUF_LEN32 32
#define EXPECTED_BUF_LEN33 33
#define EXPECTED_BUF_LEN34 34
#define EXPECTED_BUF_LEN35 35

#define EXPECTED_BUF_LEN39 39
#define EXPECTED_BUF_LEN40 40
#define EXPECTED_BUF_LEN41 41
#define EXPECTED_BUF_LEN42 42
#define EXPECTED_BUF_LEN43 43
#define EXPECTED_BUF_LEN44 44
#define EXPECTED_BUF_LEN45 45
#define EXPECTED_BUF_LEN46 46


/*This test case is RFC3610 test vector #1*/
void ccm_test_vector_1(void)
{

    /* RFC 3610 test vector #1 */
    const uint8_t key[NUM_NIST_KEYS] =
    {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
    };
    uint8_t nonce[NONCE_LEN] =
    {
        0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0xa0,
        0xa1, 0xa2, 0xa3, 0xa4, 0xa5
    };
    const uint8_t hdr[HEADER_LEN] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };
    const uint8_t data[DATA_BUF_LEN23] =
    {
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e
    };
    const uint8_t expected[EXPECTED_BUF_LEN39] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x58, 0x8c, 0x97, 0x9a, 0x61, 0xc6, 0x63, 0xd2,
        0xf0, 0x66, 0xd0, 0xc2, 0xc0, 0xf9, 0x89, 0x80,
        0x6d, 0x5f, 0x6b, 0x61, 0xda, 0xc3, 0x84, 0x17,
        0xe8, 0xd1, 0x2c, 0xfd, 0xf9, 0x26, 0xe0
    };

    uint32_t   outbuf_length, status;
    uint8_t    outbuf[64], decrypt_buf[64];

    struct aes_ccm_encryption_packet  ccm_input_packet;

    struct aes_ccm_decryption_packet  ccm_payload_packet;

    struct aes_ctx ctx;

    /*in fact, aes_acquire function is reserved for future multi-tasking case*/
    aes_acquire(&ctx);

    /*set ccm key --- ccm key must be 128 bits*/
    aes_key_init(&ctx, key, AES_KEY128);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    ccm_input_packet.nonce = (uint8_t *)nonce;

    ccm_input_packet.hdr = (uint8_t *) hdr;
    ccm_input_packet.hdr_len = HEADER_LEN;

    ccm_input_packet.data = (uint8_t *) data;
    ccm_input_packet.data_len = DATA_BUF_LEN23;

    ccm_input_packet.mlen = M_LEN8;


    outbuf_length = sizeof(outbuf);          /*this is the maximum buffer for ouput buffer */
    ccm_input_packet.out_buf = outbuf;
    ccm_input_packet.out_buf_len = &outbuf_length;

    status = aes_ccm_encryption( &ccm_input_packet);

    if (status != STATUS_SUCCESS)
    {
        printf("\n Please Check input ccm config setting. \n");
    }
    else
    {
        /* check result*/
        /* after function aes_ccm_encryption,  outbuf_length will be the real data length
         * of ouput buffer in bytes.
         */
        if ((outbuf_length != EXPECTED_BUF_LEN39) ||
                (memcmp(expected, outbuf, EXPECTED_BUF_LEN39) != 0))
        {
            printf("\nccm_encryption produced wrong data !! \n");
        }
        else
        {
            printf("\nccm_encryption test vector #1 correct !\n");
        }

    }

    /*decryption and verification*/

    ccm_payload_packet.payload_buf = outbuf;        /*payload for decryption and verification*/

    ccm_payload_packet.nonce = (uint8_t *) nonce;

    ccm_payload_packet.hdr_len = HEADER_LEN;

    ccm_payload_packet.data_len = DATA_BUF_LEN23;   /*Notice: we don't count mlen for data length!*/

    ccm_payload_packet.mlen = M_LEN8;

    outbuf_length = sizeof (decrypt_buf);          /*this is the maximum buffer for ouput buffer */
    ccm_payload_packet.out_buf = decrypt_buf;
    ccm_payload_packet.out_buf_len = &outbuf_length;


    status = aes_ccm_decryption_verification( &ccm_payload_packet);

    if (status == STATUS_SUCCESS)
    {

        if ((outbuf_length != DATA_BUF_LEN23) ||
                (memcmp(data, decrypt_buf, DATA_BUF_LEN23) != 0))
        {
            printf("\nccm_decryption for test vector #1 error ?1 \n");
        }
        else
        {
            printf("\nccm_decryption and verification for test vector #1 correct !\n");
        }

    }
    else
    {
        printf("\nccm_dncryption wrong ?! \n");
    }


    /*in fact, aes_release function is reserved for future multi-tasking case*/
    aes_release(&ctx);

}

/*This test case is RFC3610 test vector #2*/
void ccm_test_vector_2(void)
{

    /* RFC 3610 test vector #1 */
    const uint8_t key[NUM_NIST_KEYS] =
    {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
    };
    uint8_t nonce[NONCE_LEN] =
    {
        0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01, 0xa0,
        0xa1, 0xa2, 0xa3, 0xa4, 0xa5

    };
    const uint8_t hdr[HEADER_LEN] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };
    const uint8_t data[DATA_BUF_LEN24] =
    {
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };
    const uint8_t expected[EXPECTED_BUF_LEN40] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x72, 0xc9, 0x1a, 0x36, 0xe1, 0x35, 0xf8, 0xcf,
        0x29, 0x1c, 0xa8, 0x94, 0x08, 0x5c, 0x87, 0xe3,
        0xcc, 0x15, 0xc4, 0x39, 0xc9, 0xe4, 0x3a, 0x3b,
        0xa0, 0x91, 0xd5, 0x6e, 0x10, 0x40, 0x09, 0x16
    };

    uint32_t   outbuf_length, status;
    uint8_t    outbuf[64], decrypt_buf[64];

    struct aes_ccm_encryption_packet  ccm_input_packet;

    struct aes_ccm_decryption_packet  ccm_payload_packet;

    struct aes_ctx ctx;

    /*in fact, aes_acquire function is reserved for future multi-tasking case*/
    aes_acquire(&ctx);

    /*set ccm key --- ccm key must be 128 bits*/
    aes_key_init(&ctx, key, AES_KEY128);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    ccm_input_packet.nonce = (uint8_t *) nonce;

    ccm_input_packet.hdr = (uint8_t *) hdr;
    ccm_input_packet.hdr_len = sizeof(hdr);

    ccm_input_packet.data = (uint8_t *) data;
    ccm_input_packet.data_len = sizeof(data);

    ccm_input_packet.mlen = M_LEN8;


    outbuf_length = sizeof(outbuf);
    ccm_input_packet.out_buf = outbuf;
    ccm_input_packet.out_buf_len = &outbuf_length;

    status = aes_ccm_encryption( &ccm_input_packet);

    if (status != STATUS_SUCCESS)
    {
        printf("\n Please Check input ccm config setting. \n");
    }
    else
    {
        /*check result*/

        if ((outbuf_length != sizeof(expected)) ||
                (memcmp(expected, outbuf, sizeof(expected)) != 0))
        {
            printf("\nccm_encryption produced wrong data !! \n");
        }
        else
        {
            printf("\nccm_encryption test vector #2 correct !\n");
        }

    }


    /*decryption and verification*/

    ccm_payload_packet.payload_buf = outbuf;        /*payload for decryption and verification*/

    ccm_payload_packet.nonce = (uint8_t *)nonce;

    ccm_payload_packet.hdr_len = HEADER_LEN;

    ccm_payload_packet.data_len = DATA_BUF_LEN24;   /*Notice: we don't count mlen for data length!*/

    ccm_payload_packet.mlen = M_LEN8;

    outbuf_length = sizeof(decrypt_buf);
    ccm_payload_packet.out_buf = decrypt_buf;
    ccm_payload_packet.out_buf_len = &outbuf_length;

    status = aes_ccm_decryption_verification( &ccm_payload_packet);

    if (status == STATUS_SUCCESS)
    {

        if ((outbuf_length != DATA_BUF_LEN24) ||
                (memcmp(data, decrypt_buf, DATA_BUF_LEN24) != 0))
        {
            printf("\nccm_decryption for test vector #2 error ?1 \n");
        }
        else
        {
            printf("\nccm_decryption and verification for test vector #2 correct !\n");
        }

    }
    else
    {
        printf("\nccm_dncryption wrong ?! \n");
    }

    /*in fact, aes_release function is reserved for future multi-tasking case*/
    aes_release(&ctx);

}

/*This test case is RFC3610 test vector #3*/
void ccm_test_vector_3(void)
{
    /* RFC 3610 test vector #3  */
    const uint8_t key[NUM_NIST_KEYS] =
    {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
    };
    uint8_t nonce[NONCE_LEN] =
    {
        0x00, 0x00, 0x00, 0x05, 0x04, 0x03, 0x02, 0xa0,
        0xa1, 0xa2, 0xa3, 0xa4, 0xa5
    };
    const uint8_t hdr[HEADER_LEN] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };
    const uint8_t data[DATA_BUF_LEN25] =
    {
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20
    };
    const uint8_t expected[EXPECTED_BUF_LEN41] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x51, 0xb1, 0xe5, 0xf4, 0x4a, 0x19, 0x7d, 0x1d,
        0xa4, 0x6b, 0x0f, 0x8e, 0x2d, 0x28, 0x2a, 0xe8,
        0x71, 0xe8, 0x38, 0xbb, 0x64, 0xda, 0x85, 0x96,
        0x57, 0x4a, 0xda, 0xa7, 0x6f, 0xbd, 0x9f, 0xb0,
        0xc5
    };

    uint32_t   outbuf_length, status;
    uint8_t    outbuf[64], decrypt_buf[64];

    struct aes_ccm_encryption_packet  ccm_input_packet;

    struct aes_ccm_decryption_packet  ccm_payload_packet;

    struct aes_ctx ctx;

    /*in fact, aes_acquire function is reserved for future multi-tasking case*/
    aes_acquire(&ctx);

    /*set ccm key --- ccm key must be 128 bits*/
    aes_key_init(&ctx, key, AES_KEY128);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    ccm_input_packet.nonce = (uint8_t *)nonce;

    ccm_input_packet.hdr = (uint8_t *) hdr;
    ccm_input_packet.hdr_len = sizeof(hdr);

    ccm_input_packet.data = (uint8_t *) data;
    ccm_input_packet.data_len = sizeof(data);

    ccm_input_packet.mlen = M_LEN8;

    outbuf_length = sizeof(outbuf);
    ccm_input_packet.out_buf = outbuf;
    ccm_input_packet.out_buf_len = &outbuf_length;

    status = aes_ccm_encryption( &ccm_input_packet);

    if (status != STATUS_SUCCESS)
    {
        printf("\n Please Check input ccm config setting. \n");
    }
    else
    {
        /*check result*/
        if ((outbuf_length != sizeof(expected)) ||
                (memcmp(expected, outbuf, sizeof(expected)) != 0))
        {
            printf("\nccm_encryption produced wrong data !! \n");
        }
        else
        {
            printf("\nccm_encryption test vector #3 correct !\n");
        }

    }

    /*decryption and verification*/

    ccm_payload_packet.payload_buf = outbuf;        /*payload for decryption and verification*/

    ccm_payload_packet.nonce = (uint8_t *)nonce;

    ccm_payload_packet.hdr_len = HEADER_LEN;

    ccm_payload_packet.data_len = DATA_BUF_LEN25;   /*Notice: we don't count mlen for data length!*/

    ccm_payload_packet.mlen = M_LEN8;

    outbuf_length = sizeof(decrypt_buf);
    ccm_payload_packet.out_buf = decrypt_buf;
    ccm_payload_packet.out_buf_len = &outbuf_length;

    status = aes_ccm_decryption_verification(&ccm_payload_packet);

    if (status == STATUS_SUCCESS)
    {

        if ((outbuf_length != DATA_BUF_LEN25) ||
                (memcmp(data, decrypt_buf, DATA_BUF_LEN25) != 0))
        {
            printf("\nccm_decryption for test vector #3 error ?1 \n");
        }
        else
        {
            printf("\nccm_decryption and verification for test vector #3 correct !\n");
        }

    }
    else
    {
        printf("\nccm_dncryption wrong ?! \n");
    }

    /*in fact, aes_release function is reserved for future multi-tasking case*/
    aes_release(&ctx);

}


/*This test case is RFC3610 test vector #7*/
void ccm_test_vector_7(void)
{

    /* RFC 3610 test vector #7  */
    const uint8_t key[NUM_NIST_KEYS] =
    {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
    };
    uint8_t nonce[NONCE_LEN] =
    {
        0x00, 0x00, 0x00, 0x09, 0x08, 0x07, 0x06, 0xa0,
        0xa1, 0xa2, 0xa3, 0xa4, 0xa5
    };
    const uint8_t hdr[HEADER_LEN] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };
    const uint8_t data[DATA_BUF_LEN23] =
    {
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e
    };

    /*expeceted is the result of test pattern after CCM*/
    const uint8_t expected[EXPECTED_BUF_LEN41] =
    {
        0x00, 0x01, 0x02, 0x03,  0x04, 0x05, 0x06, 0x07,
        0x01, 0x35, 0xD1, 0xB2,  0xC9, 0x5F, 0x41, 0xD5,
        0xD1, 0xD4, 0xFE, 0xC1,  0x85, 0xD1, 0x66, 0xB8,
        0x09, 0x4E, 0x99, 0x9D,  0xFE, 0xD9, 0x6C, 0x04,
        0x8C, 0x56, 0x60, 0x2C,  0x97, 0xAC, 0xBB, 0x74,
        0x90
    };

    uint32_t   outbuf_length, status;
    uint8_t    outbuf[64], decrypt_buf[64];

    struct aes_ccm_encryption_packet  ccm_input_packet;

    struct aes_ccm_decryption_packet  ccm_payload_packet;

    struct aes_ctx ctx;

    /*in fact, aes_acquire function is reserved for future multi-tasking case*/
    aes_acquire(&ctx);

    /*set ccm key --- ccm key must be 128 bits*/
    aes_key_init(&ctx, key, AES_KEY128);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    ccm_input_packet.nonce = (uint8_t *)nonce;

    ccm_input_packet.hdr = (uint8_t *) hdr;
    ccm_input_packet.hdr_len = sizeof(hdr);

    ccm_input_packet.data = (uint8_t *) data;
    ccm_input_packet.data_len = sizeof(data);

    ccm_input_packet.mlen = M_LEN10;


    outbuf_length = sizeof(outbuf);
    ccm_input_packet.out_buf = outbuf;
    ccm_input_packet.out_buf_len = &outbuf_length;

    status = aes_ccm_encryption( &ccm_input_packet);

    if (status != STATUS_SUCCESS)
    {
        printf("\n Please Check input ccm config setting. \n");
    }
    else
    {
        /*check result*/

        if ((outbuf_length != sizeof(expected)) ||
                (memcmp(expected, outbuf, sizeof(expected)) != 0))
        {
            printf("\nccm_encryption produced wrong data !! \n");
        }
        else
        {
            printf("\nccm_encryption test vector #7 correct !\n");
        }

    }

    /*decryption and verification*/

    ccm_payload_packet.payload_buf = outbuf;        /*payload for decryption and verification*/

    ccm_payload_packet.nonce = (uint8_t *)nonce;

    ccm_payload_packet.hdr_len = HEADER_LEN;

    ccm_payload_packet.data_len = DATA_BUF_LEN23;   /*Notice: we don't count mlen for data length!*/

    ccm_payload_packet.mlen = M_LEN10;

    outbuf_length = sizeof(decrypt_buf);
    ccm_payload_packet.out_buf = decrypt_buf;
    ccm_payload_packet.out_buf_len = &outbuf_length;


    status = aes_ccm_decryption_verification( &ccm_payload_packet);

    if (status == STATUS_SUCCESS)
    {

        if ((outbuf_length != DATA_BUF_LEN23) ||
                (memcmp(data, decrypt_buf, DATA_BUF_LEN23) != 0))
        {
            printf("\nccm_decryption for test vector #7 error ?1 \n");
        }
        else
        {
            printf("\nccm_decryption and verification for test vector #7 correct !\n");
        }

    }
    else
    {
        printf("\nccm_dncryption wrong ?! \n");
    }

    /*in fact, aes_release function is reserved for future multi-tasking case*/
    aes_release(&ctx);

}


#endif

//------------------ AES ------------------

int main(void)
{

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("Crypto CCM Test %s %s\n", __DATE__, __TIME__);

#if MODULE_ENABLE(CRYPTO_AES_ENABLE)

    /* NOTICE:
     *   Only load AES firmware when AES firmware changed.
     * If crypto engine always do AES operation, it just need to load ONCE
     * firmware.
     *
     *   If it is possible for crypto engine to used in ECC, we can add
     * some help/check code in aes_acquire function to load firmware again.
     *
     */
    aes_fw_init();

    ccm_test_vector_1();
    ccm_test_vector_2();
    ccm_test_vector_3();

    ccm_test_vector_7();

#endif

    printf("finish crypto test\n");

    while (1);
}


void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */

