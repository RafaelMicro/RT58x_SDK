/** @file main.c
 *
 * @brief Crypto example main file.
 *
 *
 */
/**
* @defgroup Crypto_example_group  Crypto
* @ingroup examples_group
* @{
* @brief Crypto example demonstrate
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

/*below test vector is RFC 4493 Test (Appendix A.) data*/
const uint8_t tk[AES_BLOCKLEN] =
{
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

const uint8_t plaintext_data[64] =
{
    0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
    0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
    0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
    0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
    0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
    0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
    0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
};

const uint8_t expected_encryption_text[64] =
{
    0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
    0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97,
    0xf5, 0xd3, 0xd5, 0x85, 0x03, 0xb9, 0x69, 0x9d,
    0xe7, 0x85, 0x89, 0x5a, 0x96, 0xfd, 0xba, 0xaf,
    0x43, 0xb1, 0xcd, 0x7f, 0x59, 0x8e, 0xce, 0x23,
    0x88, 0x1b, 0x00, 0xe3, 0xed, 0x03, 0x06, 0x88,
    0x7b, 0x0c, 0x78, 0x5e, 0x27, 0xe8, 0xad, 0x3f,
    0x82, 0x23, 0x20, 0x71, 0x04, 0x72, 0x5d, 0xd4
};

/*
 *   g_encryption_text_buf/g_decryption_text_buf buffer
 *  used as "temp buffer" for below function used.
 *
 */

static uint8_t g_encryption_text_buf[128];

static uint8_t g_decryption_text_buf[128];

void aes_128_test(void)
{
    //AES-128, AES-128 test pattern
    uint8_t *in_buf;
    uint8_t *out_buf;

    uint32_t i;

    struct aes_ctx ctx;

    /*in fact, aes_acquire function is reserved for future multi-tasking case*/
    aes_acquire(&ctx);

    /*set encryption key*/
    aes_key_init(&ctx, tk, AES_KEY128);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    in_buf = (uint8_t *) plaintext_data;
    out_buf = g_encryption_text_buf;

    for (i = 0; i < 4; i++)
    {
        aes_ecb_encrypt( &ctx, in_buf, out_buf);

        in_buf  += AES_BLOCKLEN;
        out_buf += AES_BLOCKLEN;
    }

    /*in fact, aes_release function is reserved for future multi-tasking case*/
    aes_release(&ctx);

    printf("AES_128_test hardware encryption  ECB:");

    if (memcmp((char *) expected_encryption_text, (char *) g_encryption_text_buf, 64) == 0)
    {
        printf(" SUCCESS!\n");      /*encryption correct */
    }
    else
    {
        printf(" FAILURE!\n");
    }

}

void  aes_128_decrypt_test(void)
{
    //AES-128, AES-128 test pattern

    uint8_t *out_buf, *in_buf;

    uint32_t i;
    struct aes_ctx ctx;

    aes_acquire(&ctx);

    aes_key_init(&ctx, tk, AES_KEY128);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    in_buf =  (uint8_t *) expected_encryption_text;
    out_buf = g_decryption_text_buf;

    for (i = 0; i < 4; i++)
    {
        aes_ecb_decrypt(&ctx, in_buf, out_buf);

        in_buf  += AES_BLOCKLEN;
        out_buf += AES_BLOCKLEN;
    }

    aes_release(&ctx);

    printf("AES_128_test hardware decryption  ECB:");
    if (memcmp((char *) plaintext_data, (char *) g_decryption_text_buf, 64) == 0)
    {
        printf(" SUCCESS!\n");      /*decryption correct */
    }
    else
    {
        printf(" FAILURE!\n");
    }

}

void aes_256_test(void)
{

    //AES-256, AES-256 test pattern
    uint8_t key[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    /*cleartext buffer is data for encryption */
    uint8_t cleartext_buf[]  =
    {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };

    /*this is expected encryption buffer data*/
    uint8_t expect_out_buf[] =
    {
        0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
        0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89
    };

    uint8_t hw_out_buf[16];

    struct  aes_ctx ctx;

    aes_acquire(&ctx);

    aes_key_init(&ctx, key, AES_KEY256);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    aes_ecb_encrypt(&ctx, cleartext_buf, hw_out_buf);

    aes_release(&ctx);

    printf("AES_256_test hardware encryption  ECB: ");
    if (memcmp((char *) expect_out_buf, (char *) hw_out_buf, 16) == 0)
    {
        printf("SUCCESS!\n");
    }
    else
    {
        printf("FAILURE!\n");
    }

}


void aes_256_decrypt_test(void)
{
    //AES-256, AES-256 test pattern
    uint8_t key[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
    };

    /*expected decryption clear text buffer*/
    uint8_t expect_cleartext_buf[] =
    {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };

    /*cryption buffer is data to decryption*/
    uint8_t cryption_buf[] =
    {
        0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
        0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89
    };

    uint8_t hw_decryptio_cleartext_buf[16];

    struct aes_ctx ctx;

    aes_acquire(&ctx);

    aes_key_init(&ctx, key, AES_KEY256);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    aes_ecb_decrypt( &ctx, cryption_buf, hw_decryptio_cleartext_buf);

    aes_release(&ctx);

    printf("AES_256_test hardware decryption  ECB: ");

    if (memcmp((char *) expect_cleartext_buf, (char *) hw_decryptio_cleartext_buf, 16) == 0)
    {
        printf("SUCCESS!\n");
    }
    else
    {
        printf("FAILURE!\n");
    }

}

void left_shift(int len, uint8_t *add, uint8_t *des)
{
    int i;

    for (i = 0; i < len - 1; i++)
    {
        des[i] = (add[i] << 1) + ((add[i + 1] >= 0x80) ? 1 : 0);
    }

    des[len - 1] = add[len - 1] << 1;

}

void load_mac_key(uint8_t *key, uint8_t *subkey_k1, uint8_t *subkey_k2)
{

    uint8_t const_zero[16] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };                              /*Const_Zero in RFC4493*/
    uint8_t L[16];
    uint8_t const_rb = 0x87;        /*Const_Rb in RFC4493 */

    struct  aes_ctx ctx;

    /*see RFC4493 for detail algorithm. It is based on AES KEY 128.*/

    aes_acquire(&ctx);

    aes_key_init(&ctx, key, AES_KEY128);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    /*Step 1. AES_128(K, const_Zero) */
    aes_ecb_encrypt( &ctx, const_zero, L);

    aes_release(&ctx);

    left_shift(AES_BLOCKLEN, L, subkey_k1);

    /* Step 2. generate k1, if MSB(L) is 0, then L is just shift left one bit */

    if (L[0] >= 0x80)
    {
        /* generate k1, if  MSB(L) is 1, it means  = (L << 1) xor Rb */
        subkey_k1[(AES_BLOCKLEN - 1)] ^= const_rb;
    }

    /* Step 3. generate k2, if MSB(K1) is 0, K2 = K1 << 1 */
    left_shift(AES_BLOCKLEN, subkey_k1, subkey_k2);

    if (subkey_k1[0] >= 0x80)
    {
        /* generate k1, if  MSB(K1) is 1, it means  = (K1 << 1) xor Rb */
        subkey_k2[(AES_BLOCKLEN - 1)] ^= const_rb;
    }

}

const uint8_t cmac_expect_64_data[AES_BLOCKLEN] =
{
    /*This is 64 bytes return mac*/
    0x51, 0xF0, 0xBE, 0xBF,
    0x7E, 0x3B, 0x9D, 0x92,
    0xFC, 0x49, 0x74, 0x17,
    0x79, 0x36, 0x3C, 0xFE
};

const uint8_t cmac_expect_16_data[AES_BLOCKLEN] =
{
    /*This is 16 bytes return mac*/
    0x07, 0x0A, 0x16, 0xB4,
    0x6B, 0x4D, 0x41, 0x44,
    0xF7, 0x9B, 0xDD, 0x9D,
    0xD0, 0x4A, 0x28, 0x7C
};


void aes_128_test_cmac_compare(uint8_t *data, uint32_t length, uint8_t *expect_result)
{
    uint32_t   n, remain, i;

    uint8_t    temp_buf[AES_BLOCKLEN];
    uint8_t    iv[AES_BLOCKLEN];
    uint8_t    *p_lastblock, *p_orig_last;

    /*subkey k1 k2 defined in RFC4493 */
    uint8_t    k1[AES_BLOCKLEN];
    uint8_t    k2[AES_BLOCKLEN];

    struct aes_ctx ctx;

    /*Please refer RFC4493 for these operation.*/

    /* if datra length is not 16N
     * data should add padding "10000..." according to RFC4493*/

    load_mac_key((uint8_t *)tk, k1, k2);     /*generate k1, k2*/

    p_lastblock = temp_buf;

    if (length == 0)
    {
        /*this is specail case in RFC4493.*/
        /*flag is false*/
        n = 1;

        memset(p_lastblock, 0, AES_BLOCKLEN);

        temp_buf[0] = 0x80;    /*padding*/
        aes_xorwith((uint32_t *)p_lastblock, (uint32_t *)p_lastblock, (uint32_t *)k2);
    }
    else
    {
        n = (length + (AES_BLOCKLEN - 1)) >> 4;

        remain = length & (AES_BLOCKLEN - 1);

        if (remain)
        {
            /*length is not 16N*/
            /*flag is flase*/

            /*we should add padding for last block*/
            p_orig_last = data + (length & ~(AES_BLOCKLEN - 1));

            /*copy the last block for add padding to temp_buffer*/
            for (i = 0; i < remain; i++)
            {
                temp_buf[i] = p_orig_last[i];
            }

            temp_buf[i] = 0x80;           /*RFC4493 defined padding */
            i++;

            while (i < AES_BLOCKLEN)
            {
                temp_buf[i] = 0x00;
                i++;
            }

            aes_xorwith((uint32_t *)p_lastblock, (uint32_t *)p_lastblock, (uint32_t *) k2);
        }
        else
        {
            /*flag is true*/
            /*because we can not corrupt the last block, so we need temp_buf to help us*/
            p_lastblock = (uint8_t *) data + (length - AES_BLOCKLEN);
            aes_xorwith((uint32_t *)temp_buf, (uint32_t *)p_lastblock, (uint32_t *)k1);
            p_lastblock = temp_buf;
        }

    }

    memset(iv, 0, AES_BLOCKLEN);

    aes_acquire(&ctx);

    aes_key_init(&ctx, tk, AES_KEY128);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    aes_iv_set(&ctx, iv);

    /*RFC4493: step 5. for i:=1 to n-1 do... so if n=1 it will NOT do the cbc mode */
    n--;
    if (n > 0)
    {
        /*  CBC only count block1 ~ block (n-1), (block1 is the first block)
         *  REMARK: because we use the same buffer for in_buf/out_buf
         *  IMPORTANT: So original data will lose here.
         *  Or you can allocate a "temp" buffer to save cbc data...
         */
        aes_cbc_buffer_encrypt(&ctx, data, data, (n << 4));
    }

    aes_iv_get(&ctx, iv);        /*get IV*/

    aes_xorwith((uint32_t *)iv, (uint32_t *)iv, (uint32_t *)p_lastblock);

    aes_ecb_encrypt(&ctx, iv, temp_buf);     /*temp_buf is the finial mac message */

    aes_release(&ctx);

    printf("AES_128_test CMAC length(%ld)  : ", length);

    if ( memcmp((char *) expect_result, (char *) temp_buf, AES_BLOCKLEN) == 0 )
    {
        printf("SUCCESS!\n");
    }
    else
    {
        printf("FAILURE!\n");
    }

}

void aes_128_test_cmac(void)
{
    uint8_t  msg_buf[128];

    memcpy(msg_buf, plaintext_data, 64);
    aes_128_test_cmac_compare(msg_buf, 64, (uint8_t *) cmac_expect_64_data);

    /*because msg_buf will be modified, so we need reinitial msg_buf*/
    memcpy(msg_buf, plaintext_data, 16);
    aes_128_test_cmac_compare(msg_buf, 16, (uint8_t *) cmac_expect_16_data);

}

void aes_128_cbc_test(void)
{
    uint8_t   iv[16];
    struct    aes_ctx ctx;

    uint32_t  length, i;
    uint8_t   padding_data;
    uint8_t   *p_string_end;

    /*
     *   Please notice: plain_text_buf is only 85 bytes. It is not 64N...
     * so we need add padding in the end of plain_text. In this example we
     * will add 11 bytes. Add padding has many format, like all zero padding,
     * ISO-7816-4 padding,  PKCS#7, or ....
     * In this example, we use PKCS#7 padding.
     */
    uint8_t   plain_text_buf[128] =
    {
        "Example string to demonstrate AES CBC mode with padding. This text has 85 characters."
    };

    /*    AES CBC only support length is 16N. Data suggest to  add pkcs#7 padding
     *  or "0x80 0x00 ...."  for not 16N data.
     * Here we use PKCS7 padding for not 16N data.
     */
    memset(iv, 0, AES_BLOCKLEN);

    length = strlen((char *) plain_text_buf);

    p_string_end = (uint8_t *) plain_text_buf + length;
    /*padding data will equal (16-length%16) */
    padding_data = AES_BLOCKLEN - (length & (AES_BLOCKLEN - 1));

    /*add PKCS#7 padding*/
    for (i = 0; i < padding_data; i++)
    {
        *p_string_end++ = padding_data;
    }

    length = strlen((char *) plain_text_buf);

    aes_acquire(&ctx);

    aes_key_init(&ctx, tk, AES_KEY128);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    aes_iv_set(&ctx, iv);
    aes_cbc_buffer_encrypt( &ctx, plain_text_buf, g_encryption_text_buf, length);
    aes_release(&ctx);

    /*******************************************/

    aes_acquire(&ctx);

    aes_key_init(&ctx, tk, AES_KEY128);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    aes_iv_set(&ctx, iv);
    aes_cbc_buffer_decrypt(&ctx, g_encryption_text_buf, g_decryption_text_buf, length);
    aes_release(&ctx);

    if ( memcmp((char *) plain_text_buf, (char *) g_decryption_text_buf, length) == 0 )
    {
        printf("AES_128_CBC_test : SUCCESS!\n");
    }
    else
    {
        printf("AES_128_CBC_test : FAILURE!\n");
    }

}

void  aes_128_ctr_test(void)
{
    /*Please see RFC 3686 Test Vector #3 for this example*/

    uint8_t  in[36] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x20, 0x21, 0x22, 0x23
    };

    uint8_t  expected_cipher[36] =
    {
        0xC1, 0xCF, 0x48, 0xA8, 0x9F, 0x2F, 0xFD, 0xD9,
        0xCF, 0x46, 0x52, 0xE9, 0xEF, 0xDB, 0x72, 0xD7,
        0x45, 0x40, 0xA4, 0x2B, 0xDE, 0x6D, 0x78, 0x36,
        0xD5, 0x9A, 0x5C, 0xEA, 0xAE, 0xF3, 0x10, 0x53,
        0x25, 0xB2, 0x07, 0x2F
    };

    uint8_t  key[16] =
    {
        0x76, 0x91, 0xBE, 0x03, 0x5E, 0x50, 0x20, 0xA8,
        0xAC, 0x6E, 0x61, 0x85, 0x29, 0xF9, 0xA0, 0xDC
    };

    uint8_t  iv[16]  =
    {
        0x00, 0xE0, 0x01, 0x7B, 0x27, 0x77, 0x7F, 0x3F,
        0x4A, 0x17, 0x86, 0xF0, 0x00, 0x00, 0x00, 0x01
    };

    uint8_t  stream_block[16];

    uint32_t   offset;

    struct aes_ctx ctx;


    offset = 0;

    aes_acquire(&ctx);

    aes_key_init(&ctx, key, AES_KEY128);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    aes_iv_set(&ctx, iv);

    /* this is sample to demo "streaming character" */
    aes_ctr_buffer_crypt(&ctx, in, g_encryption_text_buf, stream_block, &offset, 15);

    aes_ctr_buffer_crypt(&ctx, in + 15, g_encryption_text_buf + 15, stream_block, &offset, 3);

    aes_ctr_buffer_crypt(&ctx, in + 18, g_encryption_text_buf + 18, stream_block, &offset, 18);

    aes_release(&ctx);

    printf("\nAES_128_CTR_test encryption:");

    if (memcmp((char *) g_encryption_text_buf, (char *) expected_cipher, 36) == 0)
    {
        printf("SUCCESS!\n");
    }
    else
    {
        printf("FAILURE!\n");
    }

    /*Remark: CTR use AES Encryption only!!*/

    offset = 0;

    aes_acquire(&ctx);

    aes_key_init(&ctx, key, AES_KEY128);
    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    aes_iv_set(&ctx, iv);

    /* this is sample to demo "streaming character" */
    aes_ctr_buffer_crypt(&ctx, g_encryption_text_buf, g_decryption_text_buf,
                         stream_block, &offset, 10);

    aes_ctr_buffer_crypt(&ctx, g_encryption_text_buf + 10, g_decryption_text_buf + 10,
                         stream_block, &offset, 7);

    aes_ctr_buffer_crypt(&ctx, g_encryption_text_buf + 17, g_decryption_text_buf + 17,
                         stream_block, &offset, 19);

    aes_release(&ctx);

    printf("\nAES_128_CTR_test decryption:");
    if (memcmp((char *) g_decryption_text_buf, (char *) in, 36) == 0)
    {
        printf("SUCCESS!\n");
    }
    else
    {
        printf("FAILURE!\n");
    }

}


void aes_192_ctr_test(void)
{
    /*Please see RFC 3686 Test Vector #6 for this example*/

    uint8_t  in[36] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x20, 0x21, 0x22, 0x23
    };


    uint8_t  expected_cipher[36] =
    {
        0x96, 0x89, 0x3F, 0xC5, 0x5E, 0x5C, 0x72, 0x2F,
        0x54, 0x0B, 0x7D, 0xD1, 0xDD, 0xF7, 0xE7, 0x58,
        0xD2, 0x88, 0xBC, 0x95, 0xC6, 0x91, 0x65, 0x88,
        0x45, 0x36, 0xC8, 0x11, 0x66, 0x2F, 0x21, 0x88,
        0xAB, 0xEE, 0x09, 0x35,
    };

    uint8_t  key[24] =
    {
        0x02, 0xBF, 0x39, 0x1E, 0xE8, 0xEC, 0xB1, 0x59,
        0xB9, 0x59, 0x61, 0x7B, 0x09, 0x65, 0x27, 0x9B,
        0xF5, 0x9B, 0x60, 0xA7, 0x86, 0xD3, 0xE0, 0xFE
    };


    uint8_t  iv[16]  =
    {
        0x00, 0x07, 0xBD, 0xFD, 0x5C, 0xBD, 0x60, 0x27,
        0x8D, 0xCC, 0x09, 0x12, 0x00, 0x00, 0x00, 0x01
    };

    uint8_t  stream_block[16];

    uint32_t   offset;

    struct aes_ctx ctx;


    offset = 0;

    aes_acquire(&ctx);

    aes_key_init(&ctx, key, AES_KEY192);
    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    aes_iv_set(&ctx, iv);

    /* this is sample to demo "streaming character" */
    aes_ctr_buffer_crypt(&ctx, in, g_encryption_text_buf, stream_block, &offset, 5);

    aes_ctr_buffer_crypt(&ctx, in + 5, g_encryption_text_buf + 5, stream_block, &offset, 11);

    aes_ctr_buffer_crypt(&ctx, in + 16, g_encryption_text_buf + 16, stream_block, &offset, 20);

    aes_release(&ctx);

    printf("\nAES_192_CTR_test encryption:");

    if (memcmp((char *) g_encryption_text_buf, (char *) expected_cipher, 36) == 0)
    {
        printf("SUCCESS!\n");
    }
    else
    {
        printf("FAILURE!\n");
    }

    /*Remark: CTR use AES Encryption only!!*/

    offset = 0;

    aes_acquire(&ctx);

    aes_key_init(&ctx, key, AES_KEY192);

    /*load key to secure engine*/
    aes_load_round_key(&ctx);

    aes_iv_set(&ctx, iv);

    /* this is sample to demo "streaming character" */
    aes_ctr_buffer_crypt(&ctx, g_encryption_text_buf, g_decryption_text_buf,
                         stream_block, &offset, 10);

    aes_ctr_buffer_crypt(&ctx, g_encryption_text_buf + 10, g_decryption_text_buf + 10,
                         stream_block, &offset, 7);

    aes_ctr_buffer_crypt(&ctx, g_encryption_text_buf + 17, g_decryption_text_buf + 17,
                         stream_block, &offset, 19);

    aes_release(&ctx);

    printf("\nAES_192_CTR_test decryption:");
    if ( memcmp((char *) g_decryption_text_buf, (char *) in, 36) == 0 )
    {
        printf("SUCCESS!\n");
    }
    else
    {
        printf("FAILURE!\n");
    }

}
#endif

//------------------ AES ------------------

//++++++++++++++++++ ECC GF(P) ++++++++++++++++++
#if MODULE_ENABLE(CRYPTO_SECP192R1_ENABLE)

void ecc_gfp_p192_test(void)
{
    //private key
    uint8_t prva[24] =
    {
        0xff, 0x18, 0xa5, 0xf4, 0xef, 0xd2, 0x5e, 0x62,
        0x2b, 0x14, 0x0c, 0xcf, 0xd6, 0xf1, 0x5d, 0x00,
        0x27, 0xdc, 0x8d, 0x91, 0x86, 0x5f, 0x91, 0x07
    };

    /* Curve P192 Gx & Gy parameters and used for the
      hardware public key caculation result */
    uint32_t p_x[secp192r1_op_num] =
    {
        0x82FF1012, 0xF4FF0AFD, 0x43A18800,
        0x7CBF20EB, 0xB03090F6, 0x188DA80E
    };
    uint32_t p_y[secp192r1_op_num] =
    {
        0x1E794811, 0x73F977A1, 0x6B24CDD5,
        0x631011ED, 0xFFC8DA78, 0x07192B95
    };

    //caculation result verify
    uint32_t result_x[secp192r1_op_num] =
    {
        0x1125f8ed, 0xd2809ea5, 0xfe7e4329,
        0x586f9fc3, 0x984421a6, 0x15207009
    };
    uint32_t result_y[secp192r1_op_num] =
    {
        0xb9f7ea25, 0x7fca856f, 0x9dbbaa85,
        0x9f79e4b5, 0x1bc5bd00, 0xb09d42b8
    };

    gfp_ecc_curve_p192_init();
    gfp_point_p192_mult(p_x, p_y, p_x, p_y, (uint32_t *)prva);

    //compare HW result with pattern
    if ((memcmp((char *) p_x, (char *) result_x, 24) == 0) &&
            (memcmp((char *) p_y, (char *) result_y, 24) == 0) )
    {
        printf("SECP192R1 point multiply SUCCESS!\n");
    }
    else
    {
        /*almost impossible to goto here.*/
        printf("Oops... SECP192R1 point multiply FAILURE!\n");
        printf("check why.....");
    }

}

/*Notice Data is in Little Endian*/
const uint32_t exp_x[][secp192r1_op_num] =
{
    { 0x1CF8AD1B, 0x774FC97A, 0x38D72688, 0x07FD8392, 0x247DE5D3,  0xC0626BCF  },
    { 0xD72591A3, 0xB6513365, 0x28024E40, 0xA6CA15C4, 0x27210613,  0xC1B4DB02  },
    { 0x0510FF8A, 0x8BB44593, 0xC2B4213B, 0xF9F57636, 0xE1F46DC3,  0x44275CD2  },
    { 0x3048AFDF, 0xD9947373, 0xA5355BFC, 0x29005092, 0x48FBFDBD,  0xB7310B45  }

};

const uint32_t exp_y[][secp192r1_op_num] =
{
    { 0x8E055DF5, 0x13E594B1, 0x17D637FA, 0x681ED887, 0xAC68C23E, 0x63236628 },
    { 0x3C0B80AC, 0x887D13C1, 0xD9820601, 0xB01707FE, 0x435E2F70, 0xE1D94D79 },
    { 0x2DA2202F, 0x64895985, 0x57E71671, 0x21BC79ED, 0x021CF378, 0x10527CB7 },
    { 0xD630DD6F, 0xE7BA3A99, 0xBAB926B5, 0xBD27A7A3, 0x232D83E1, 0x00615161 }
};


void ecc_gfp_p192_test2(void)
{

    int i, j;

    /*
     * Notice: p192  order "N" is
     *      0xFFFFFFFF FFFFFFFF  FFFFFFFF 99DEF836 146BC9B1 B4D22831
     *  NG = 0. "0" is not in ECC curve. It's illegal value.
     *  private Key should not be equal or greater than this value.
     *
     */

    uint32_t key[secp192r1_op_num] =
    {
        0xB4D2281E, 0x146BC9B1, 0x99DEF836, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
    };

    /* Curve P192 Gx & Gy parameters and used for the
      hardware public key caculation result */
    uint32_t p_x[secp192r1_op_num] =
    {
        0x82FF1012, 0xF4FF0AFD, 0x43A18800,
        0x7CBF20EB, 0xB03090F6, 0x188DA80E
    };
    uint32_t p_y[secp192r1_op_num] =
    {
        0x1E794811, 0x73F977A1, 0x6B24CDD5,
        0x631011ED, 0xFFC8DA78, 0x07192B95
    };

    //caculation result verify
    uint32_t result_x[secp192r1_op_num] ;
    uint32_t result_y[secp192r1_op_num] ;

    for (i = 0; i < 4; i++)
    {

        gfp_ecc_curve_p192_init();
        gfp_point_p192_mult(result_x, result_y, p_x, p_y, key);


        for (j = 0; j <= 5; j++)
        {
            if ((result_x[j] != exp_x[i][j]) || (result_y[j] != exp_y[i][j]))
            {
                printf("compare error \n");
                while (1);
            }
        }

        key[0]++;

    }

    printf("ecc_gfp_p192_test2 success \n");

}


void ecc_gfp_p192_ecdh_test()
{

    /*
     *  secp192r1
     *  Alice private key = 0x7092e5fd43a17f6a3375325989284eba093564e1944e176d
     *
     *  Bob private key = 0xd6185566ec0b1f52cc56276560907cb1a8683d8449b882ce
     *
     *  Remark: This is demo sample only.
     *  HERE PRIVATE KEY IS TEST ONLY. Private Key should be random generated.
     *  It can NOT be hardcode, otherwise key will be extracted from binary or hex file.
     *
     *     Please Notice: Share key is in little endian format...
     *  So if you want to use the share key for some cryption, please notice
     *  key format is little-endian or big-endian
     */

    /*Remark: data is little endian format. that is MSB is the last byte. */

    uint32_t Alice_privK[secp192r1_op_num] = { 0x944e176d, 0x093564e1, 0x89284eba, 0x33753259, 0x43a17f6a, 0x7092e5fd};

    //Curve P192 Gx & Gy parameters and used for the hardware public key caculation result
    uint32_t p_x[secp192r1_op_num] = {0x82FF1012, 0xF4FF0AFD, 0x43A18800, 0x7CBF20EB, 0xB03090F6, 0x188DA80E};
    uint32_t p_y[secp192r1_op_num] = {0x1E794811, 0x73F977A1, 0x6B24CDD5, 0x631011ED, 0xFFC8DA78, 0x07192B95};

    //Public key for Alice.
    uint32_t Alice_Public_x[secp192r1_op_num] ;
    uint32_t Alice_Public_y[secp192r1_op_num] ;

    uint32_t Bob_privK[secp192r1_op_num] = { 0x49b882ce, 0xa8683d84, 0x60907cb1, 0xcc562765, 0xec0b1f52, 0xd6185566};

    //Public key for Bob.
    uint32_t Bob_Public_x[secp192r1_op_num];
    uint32_t Bob_Public_y[secp192r1_op_num];

    /*
     *Share key for Alice
     */
    uint32_t Alice_Share_x[secp192r1_op_num];
    uint32_t Alice_Share_y[secp192r1_op_num];

    //Share key for Bob.
    uint32_t Bob_Share_x[secp192r1_op_num];
    uint32_t Bob_Share_y[secp192r1_op_num];


    printf("This is simple example for ECDH based ECC SECP192R1 curve. \n");

    gfp_ecc_curve_p192_init();
    /*Alice uses private key to generate Alice's public key*/
    gfp_point_p192_mult(Alice_Public_x, Alice_Public_y, p_x, p_y, Alice_privK);

    gfp_ecc_curve_p192_init();
    /*Bob uses private key to generate Bob's public key*/
    gfp_point_p192_mult(Bob_Public_x, Bob_Public_y, p_x, p_y, Bob_privK);

    /*Alice sends her public key to Bob, and Bob sends his public key to Alice*/

    gfp_ecc_curve_p192_init();
    /*Alice caculate the secert shared key*/
    gfp_point_p192_mult(Alice_Share_x, Alice_Share_y, Bob_Public_x, Bob_Public_y, Alice_privK);

    gfp_ecc_curve_p192_init();
    /*Bob caculate the secert shared key*/
    gfp_point_p192_mult(Bob_Share_x, Bob_Share_y, Alice_Public_x, Alice_Public_y, Bob_privK);


    if ( (memcmp((char *) Alice_Share_x, (char *) Bob_Share_x, 24) == 0) &&
            (memcmp((char *) Alice_Share_y, (char *) Bob_Share_y, 24) == 0) )
    {
        printf("ECDH test for SECP192R1: SUCCESS!\n");
    }
    else
    {
        /*almost impossible to goto here.*/
        printf("Oops, ECDH test or SECP192R1: FAILURE!\n");
    }

}

#endif

#if  MODULE_ENABLE(CRYPTO_SECP256R1_ENABLE)

void ecc_gfp_p256_ecdh_test(void)
{
    /*
     *  secp256r1
     *
     *  Remark: This is demo sample only.
     *  HERE PRIVATE KEY IS TEST ONLY. Private Key should be random generated.
     *  It can NOT be hardcode, otherwise key will be extracted from binary or hex file.
     *
     *     Please Notice: Share key is in little endian format...
     *  So if you want to use the share key for some cryption, please notice
     *  key format is little-endian or big-endian
     */

    /*Remark: data is little endian format. that is MSB is the last byte. */

    uint32_t Alice_privK[8] =
    {
        0x944e176d, 0x093564e1, 0x89284eba, 0x33753259,
        0x43a17f6a, 0x7092e5fd, 0xc486e0f6, 0x5aeb8123
    };

    uint32_t Bob_privK[8] =
    {
        0x49b882ce, 0xa8683d84, 0x60907cb1, 0xcc562765,
        0xec0b1f52, 0xd6185566, 0xf975a3bf, 0x711c0eef
    };

    ECPoint_P256   Alice_Public_key;
    ECPoint_P256   Bob_Public_key;

    ECPoint_P256   Alice_Share_key;
    ECPoint_P256   Bob_Share_key;

    gfp_ecc_curve_p256_init();

    /*Alice uses private key to generate Alice's public key*/
    gfp_point_p256_mult((ECPoint_P256 *) &Alice_Public_key, (ECPoint_P256 *) &Curve_Gx_p256, Alice_privK);

    gfp_ecc_curve_p256_init();
    /*Bob uses private key to generate Bob's public key*/
    gfp_point_p256_mult((ECPoint_P256 *) &Bob_Public_key, (ECPoint_P256 *) &Curve_Gx_p256, Bob_privK);

    /*Alice sends her public key to Bob, and Bob sends his public key to Alice*/

    gfp_ecc_curve_p256_init();
    /*Alice caculate the secert shared key*/
    gfp_point_p256_mult((ECPoint_P256 *) &Alice_Share_key, (ECPoint_P256 *) &Bob_Public_key, Alice_privK);


    gfp_ecc_curve_p256_init();
    /*Bob caculate the secert shared key*/
    gfp_point_p256_mult((ECPoint_P256 *) &Bob_Share_key, (ECPoint_P256 *) &Alice_Public_key, Bob_privK);


    if ( (0 == memcmp((char *) (Alice_Share_key.x), (char *) (Bob_Share_key.x), 32)) &&
            (0 == memcmp((char *) (Alice_Share_key.y), (char *) (Bob_Share_key.y), 32)) )
    {
        printf("ECDH256 test: SUCCESS!\n");
    }
    else
    {
        printf("ECDH256 test: FAILURE!\n");
    }

}


#endif

//------------------ ECC GF(P) ------------------

//++++++++++++++++++ ECC GF(2m) ++++++++++++++++++

#if MODULE_ENABLE(CRYPTO_SECT163R2_ENABLE)

void ecc_gf2m_b163_test(void)
{

    /* GF2m_ECC_B163's key should be NOT greater than (<=) 21 bytes */
    /*Here we for 4-bytes alignment, so we use 24 bytes.*/

    /* Remark: Data is in Little-Endian */

    //Curve B163 Gx & Gy parameters and used for the hardware public key caculation result
    uint32_t p_x[6] =
    {
        0xE8343E36, 0xD4994637, 0xA0991168,
        0x86A2D57E, 0xF0EBA162, 0x00000003
    };
    uint32_t p_y[6] =
    {
        0x797324f1, 0xb11c5c0c, 0xa2cdd545,
        0x71a0094f, 0xd51fbc6c, 0x00000000
    };


    /* This Key is for demo use only. It's not a good choice for private key.
      Key = 5846006549323611672814742442876390689256843201577 (decimal)
      Key = 04 00000000 00000000 000292FE 77E70C12 A4234C29   (hex)
      x =   05 07E54141 0F581B0D 6914C218 3C9313E7 CAA10915
      y =   06 0423939F 668BF4D6 BB1DA3A3 AB714CFC E8D58A54
    */
    uint32_t key[6] =
    {
        0xA4234C29, 0x77E70C12, 0x000292FE,
        0x00000000, 0x00000000, 0x00000004
    };

    uint32_t public_key_x[6];
    uint32_t public_key_y[6];

    //expected result for verify
    uint32_t exp_x[6] =
    {
        0xCAA10915, 0x3C9313E7, 0x6914C218,
        0x0F581B0D, 0x07E54141, 0x00000005
    };

    uint32_t exp_y[6] =
    {
        0xE8D58A54, 0xAB714CFC, 0xBB1DA3A3,
        0x668BF4D6, 0x0423939F, 0x00000006
    };

    //Crypto engine init
    gf2m_ecc_curve_b163_init();

    //start the crypto engine caculation
    gf2m_point_b163_mult(public_key_x,  public_key_y,  p_x,  p_y, (uint32_t *)key);

    if ((memcmp((char *) exp_x, (char *) public_key_x, 21) == 0) &&
            (memcmp((char *) exp_y, (char *) public_key_y, 21) == 0) )
    {
        printf("ECC SECT163R2 point multiplication SUCCESS!\n");
    }
    else
    {
        printf("Oops ECC SECT163R2 point multiplication  FAILURE!\n");
        printf("check why \n");
    }

}


void ecc_gf2m_b163_test2(void)
{

    int i, j;
    /* GF2m_ECC_B163's key should be NOT greater than (<=) 21 bytes */
    /*Here we for 4-bytes alignment, so we use 24 bytes.*/

    /* Remark: Data is in Little-Endian */

    //Curve B163 Gx & Gy parameters and used for the hardware public key caculation result
    uint32_t p_x[6] =
    {
        0xE8343E36, 0xD4994637, 0xA0991168,
        0x86A2D57E, 0xF0EBA162, 0x00000003
    };
    uint32_t p_y[6] =
    {
        0x797324f1, 0xb11c5c0c, 0xa2cdd545,
        0x71a0094f, 0xd51fbc6c, 0x00000000
    };

    uint32_t key[6] =
    {
        0xA4234C1F,  0x77E70C12, 0x000292FE,
        0x00000000, 0x00000000,  0x00000004
    };

    uint32_t public_key_x[6];
    uint32_t public_key_y[6];


    for (i = 1; i < 20; i++)
    {
        //Crypto engine init
        gf2m_ecc_curve_b163_init();

        //start the crypto engine caculation
        gf2m_point_b163_mult(public_key_x,  public_key_y,  p_x,  p_y, (uint32_t *)key);


        for (j = 0; j <= 5; j++)
        {
            printf("%08lX ", public_key_x[5 - j]);
        }

        printf("\n");


        for (j = 0; j <= 5; j++)
        {
            printf("%08lX ", public_key_y[5 - j]);
        }

        printf("\n\n");


        key[0]++;
    }

}

#endif


#if  MODULE_ENABLE(CRYPTO_C25519_ENABLE)

void lecp_print_hex_bytes(const char *name, const uint8_t *data, uint32_t size)
{
    uint32_t i = 0;

    printf("%s = 0x", name);

    while (i < size)
    {
        printf("%02X", data[i++]);
    }

    printf("\n");
}

#define curve25519_in_bytes       32

void curve_c25519_test1( void)
{

    unsigned char alice_public_key[curve25519_in_bytes], alice_shared_key[curve25519_in_bytes];
    unsigned char bruce_public_key[curve25519_in_bytes], bruce_shared_key[curve25519_in_bytes];

    /*this is rfc7748 example... Notice: RFC7748 test vector is little endian format*/

    unsigned char alice_secret_key[curve25519_in_bytes] =
    {
        0x77, 0x07, 0x6d, 0x0a, 0x73, 0x18, 0xa5, 0x7d,
        0x3c, 0x16, 0xc1, 0x72, 0x51, 0xb2, 0x66, 0x45,
        0xdf, 0x4c, 0x2f, 0x87, 0xeb, 0xc0, 0x99, 0x2a,
        0xb1, 0x77, 0xfb, 0xa5, 0x1d, 0xb9, 0x2c, 0x2a

    };

    unsigned char bruce_secret_key[curve25519_in_bytes] =
    {
        0x5d, 0xab, 0x08, 0x7e, 0x62, 0x4a, 0x8a, 0x4b,
        0x79, 0xe1, 0x7f, 0x8b, 0x83, 0x80, 0x0e, 0xe6,
        0x6f, 0x3b, 0xb1, 0x29, 0x26, 0x18, 0xb6, 0xfd,
        0x1c, 0x2f, 0x8b, 0x27, 0xff, 0x88, 0xe0, 0xeb

    };

    unsigned char BasePoint[curve25519_in_bytes] = {0x09};


    printf("\n-- curve25519 -- key exchange test -----------------------------\n");

    /* Step 1. init to load curve25519 firmware  */
    curve_c25519_init();

    /* Step 2. Alice and Bruce generate their own random secret keys */
    /* Remark: secret key should keep secret... here we just show it for deom.*/
    lecp_print_hex_bytes("Alice_secret_key", alice_secret_key, curve25519_in_bytes);
    lecp_print_hex_bytes("Bruce_secret_key", bruce_secret_key, curve25519_in_bytes);


    /* Step 3. Alice and Bruce create public keys associated with their secret keys */
    /*         and exchange their public keys by some method... */

    /*Here we set blind_zr to NULL, so the curve25516_point_mul will use default blind_zr.*/

    curve25519_point_mul(NULL, (uint32_t *)alice_public_key, (uint32_t *)alice_secret_key, (uint32_t *)BasePoint);
    curve25519_point_mul(NULL, (uint32_t *)bruce_public_key, (uint32_t *)bruce_secret_key, (uint32_t *)BasePoint);

    lecp_print_hex_bytes("Alice_public_key", alice_public_key, curve25519_in_bytes);
    lecp_print_hex_bytes("Bruce_public_key", bruce_public_key, curve25519_in_bytes);

    /* Step 4. Alice and Bruce create their shared key */

    curve25519_point_mul( NULL, (uint32_t *)alice_shared_key,  (uint32_t *)alice_secret_key, (uint32_t *)bruce_public_key);
    curve25519_point_mul( NULL, (uint32_t *)bruce_shared_key, (uint32_t *)bruce_secret_key, (uint32_t *)alice_public_key);

    /*Now Alice and Bruce have share private key*/
    lecp_print_hex_bytes("Alice_shared", alice_shared_key, curve25519_in_bytes);
    lecp_print_hex_bytes("Bruce_shared", bruce_shared_key, curve25519_in_bytes);

    /*Release crypto accelator*/
    curve25519_release();

    /* Alice and Bruce should end up with idetntical keys */
    if ( memcmp(alice_shared_key, bruce_shared_key, curve25519_in_bytes) != 0)
    {
        printf("DH key exchange FAILED!!\n");
        while (1);
    }
    else
    {
        printf("Curve25519 DH key exchange Success!!\n");
    }

    return;
}

void curve_c25519_test2()
{

    /*this is rfc7748 example... Notice: RFC7748 test vector is little endian format*/


    int i;
    unsigned char k[curve25519_in_bytes] = {0x09, 0x00};
    unsigned char u[curve25519_in_bytes] = {0x09, 0x00};
    unsigned char temp[curve25519_in_bytes] = {0x00};

    unsigned char expect_result[curve25519_in_bytes] =
    {
        0x68, 0x4C, 0xF5, 0x9B, 0xA8, 0x33, 0x09, 0x55,
        0x28, 0x00, 0xEF, 0x56, 0x6F, 0x2F, 0x4D, 0x3C,
        0x1C, 0x38, 0x87, 0xC4, 0x93, 0x60, 0xE3, 0x87,
        0x5F, 0x2E, 0xB9, 0x4D, 0x99, 0x53, 0x2C, 0x51
    };

    printf("\n-- curve25519 --RFC 7748 test vector example------------\n");

    printf("\n ");
    printf("This test could be running more than 2 mins... because it do point multiplication 1000 times \n");

    curve_c25519_init();


    for (i = 0; i < 1000; i++)
    {
        //u is basepoint, k is key. Please see  RFC7748 test vector for this setting.
        /*Here we set blind_zr to NULL, so the curve25516_point_mul will use default blind_zr.*/
        curve25519_point_mul(NULL, (uint32_t *)temp,  (uint32_t *)k, (uint32_t *)u);

        memcpy(u, k, curve25519_in_bytes);
        memcpy(k, temp, curve25519_in_bytes);
        printf(".");
    }

    printf("\n");

    /*Release crypto accelator*/
    curve25519_release();

    /*compare data*/
    for (i = 0; i < curve25519_in_bytes; i++)
    {
        if (temp[i] != expect_result[i])
        {
            printf("rfc7748 TEST LOOP 1000 FAILS \n");
            while (1);
        }
    }

    printf("GOOD NEWS rfc7748 TEST LOOP 1000 OK!! \n");

    return ;

}

#endif



//------------------ ECC GF(2m) ------------------

int main(void)
{
    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.

    printf("Hello Crypto %s %s\n", __DATE__, __TIME__);


#if MODULE_ENABLE(CRYPTO_AES_ENABLE)

    /* NOTICE:
     *   2021/03/04 Only load AES firmware when AES firmware changed.
     * If crypto engine always do AES operation, it just need to load ONCE
     * firmware.
     */
    printf("test AES basic ECB operation for encryption and decryption: \n");

    aes_fw_init();

    aes_128_test();

    aes_128_decrypt_test();

    aes_256_test();

    aes_256_decrypt_test();

    aes_128_cbc_test();

    aes_128_test_cmac();

    /*some test for AES CTR*/

    aes_128_ctr_test();
    aes_192_ctr_test();

#endif

#if MODULE_ENABLE(CRYPTO_SECP192R1_ENABLE)

    printf("\n");
    printf("Test for ECC SECP192R1 test: \n");

    ecc_gfp_p192_test();

    //ecc_gfp_p192_test2();

    ecc_gfp_p192_ecdh_test();
#endif


#if MODULE_ENABLE(CRYPTO_SECP256R1_ENABLE)

    printf("\n");

    ecc_gfp_p256_ecdh_test();
#endif


#if  MODULE_ENABLE(CRYPTO_SECT163R2_ENABLE)

    printf("\n");

    ecc_gf2m_b163_test();

    //ecc_gf2m_b163_test2();

#endif


#if  MODULE_ENABLE(CRYPTO_C25519_ENABLE)

    printf("\n");

    printf("This is simple example for curve c25519 test: \n");

    curve_c25519_test1();
    curve_c25519_test2();

#endif

    printf("finish basic crypto test\n");

    while (1);
}


void SetClockFreq(void)
{
    return;
}
/** @} */ /* end of examples group */
