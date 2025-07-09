
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


#define  ECDSA_P256_LENGTH             32

/*
 * Remark: UART_BAUDRATE_115200 is not 115200...Please don't use 115200 directly
 * Please use macro define  UART_BAUDRATE_XXXXXX
 */

#define PRINTF_BAUDRATE      UART_BAUDRATE_115200

#define GPIO20  20
#define GPIO21  21
#define SUBSYSTEM_CFG_PMU_MODE              0x4B0
#define SUBSYSTEM_CFG_LDO_MODE_DISABLE      0x02
/* WARNING: This is just example.. it should NOT hardcode private_key in code!
 * This private key is example in　RFC6979
 * Because we should caculate hmac_drbg, this private key is BIG endian..
 * then we will transfer it to little endian.
 */
const uint8_t      private_key[32] =
{
    0xC9, 0xAF, 0xA9, 0xD8, 0x45, 0xBA, 0x75, 0x16, 0x6B, 0x5C, 0x21, 0x57, 0x67, 0xB1, 0xD6, 0x93,
    0x4E, 0x50, 0xC3, 0xDB, 0x36, 0xE8, 0x9B, 0x12, 0x7B, 0x8A, 0x62, 0x2B, 0x12, 0x0F, 0x67, 0x21
};


const uint8_t  public_key_x[ECDSA_P256_LENGTH] =
{
    0x60, 0xFE, 0xD4, 0xBA, 0x25, 0x5A, 0x9D, 0x31,
    0xC9, 0x61, 0xEB, 0x74, 0xC6, 0x35, 0x6D, 0x68,
    0xC0, 0x49, 0xB8, 0x92, 0x3B, 0x61, 0xFA, 0x6C,
    0xE6, 0x69, 0x62, 0x2E, 0x60, 0xF2, 0x9F, 0xB6
};

const uint8_t  public_key_y[ECDSA_P256_LENGTH] =
{
    0x79, 0x03, 0xFE, 0x10, 0x08, 0xB8, 0xBC, 0x99,
    0xA4, 0x1A, 0xE9, 0xE9, 0x56, 0x28, 0xBC, 0x64,
    0xF2, 0xF1, 0xB2, 0x0C, 0x2D, 0x7E, 0x9F, 0x51,
    0x77, 0xA3, 0xC2, 0x94, 0xD4, 0x46, 0x22, 0x99
};

/*Noice: the following parameter is big endian.. because hmac_drbg is big-endian*/
const uint8_t      nist_p256_order[32] =
{
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84, 0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51
};


/*The following vector is big endian format...*/

/*
 * RFC 6979
 *
 *     A.2.5.  ECDSA, 256 Bits (Prime Field)
 *
 *　　　With SHA-256, message = "sample":
 *  k = A6E3C57DD01ABE90086538398355DD4C3B17AA873382B0F24D6129493D8AAD60
 *  r = EFD48B2AACB6A8FD1140DD9CD45E81D69D2C877B56AAF991C34D0EA84EAF3716
 *  s = F7CB1C942D657C41D436C7A1B6E29F65F3E900DBB9AFF4064DC4AB2F843ACDA8
 *
 *      With SHA-256, message = "test":
 *  k = D16B6AE827F17175E040871A1C7EC3500192C4C92677336EC2537ACAEE0008E0
 *  r = F1ABB023518351CD71D881567B1EA663ED3EFCF6C5132B354F28D3B0B7D38367
 *  s = 019F4113742A2B14BD25926B49C649155F267E60D3814B4C0CC84250E46F0083
 *
 */

const uint8_t  sample_expect_vector_r[ECDSA_P256_LENGTH] =
{
    0xEF, 0xD4, 0x8B, 0x2A, 0xAC, 0xB6, 0xA8, 0xFD,
    0x11, 0x40, 0xDD, 0x9C, 0xD4, 0x5E, 0x81, 0xD6,
    0x9D, 0x2C, 0x87, 0x7B, 0x56, 0xAA, 0xF9, 0x91,
    0xC3, 0x4D, 0x0E, 0xA8, 0x4E, 0xAF, 0x37, 0x16
};

const uint8_t  sample_expect_vector_s[ECDSA_P256_LENGTH] =
{
    0xF7, 0xCB, 0x1C, 0x94, 0x2D, 0x65, 0x7C, 0x41,
    0xD4, 0x36, 0xC7, 0xA1, 0xB6, 0xE2, 0x9F, 0x65,
    0xF3, 0xE9, 0x00, 0xDB, 0xB9, 0xAF, 0xF4, 0x06,
    0x4D, 0xC4, 0xAB, 0x2F, 0x84, 0x3A, 0xCD, 0xA8
};

const uint8_t  test_expect_vector_r[ECDSA_P256_LENGTH] =
{
    0xF1, 0xAB, 0xB0, 0x23, 0x51, 0x83, 0x51, 0xCD,
    0x71, 0xD8, 0x81, 0x56, 0x7B, 0x1E, 0xA6, 0x63,
    0xED, 0x3E, 0xFC, 0xF6, 0xC5, 0x13, 0x2B, 0x35,
    0x4F, 0x28, 0xD3, 0xB0, 0xB7, 0xD3, 0x83, 0x67
};

const uint8_t  test_expect_vector_s[ECDSA_P256_LENGTH] =
{
    0x01, 0x9F, 0x41, 0x13, 0x74, 0x2A, 0x2B, 0x14,
    0xBD, 0x25, 0x92, 0x6B, 0x49, 0xC6, 0x49, 0x15,
    0x5F, 0x26, 0x7E, 0x60, 0xD3, 0x81, 0x4B, 0x4C,
    0x0C, 0xC8, 0x42, 0x50, 0xE4, 0x6F, 0x00, 0x83
};

/*print in big endian format  for big endian data.*/
void be_print_hex_bytes(const char *name, const uint8_t *data, uint32_t size)
{
    uint32_t i = 0;

    printf("%s = 0x", name);

    while (i < size)
    {
        printf("%02X", data[i++]);
    }

    printf("\n");
}

/*print in little endian format for little endian data. */
void le_print_hex_bytes(const char *name, const uint8_t *data, uint32_t size)
{
    uint32_t i = 0;

    printf("%s = 0x", name);

    while (i < size)
    {
        printf("%02X", data[(size - 1) - i]);
        i++;
    }

    printf("\n");
}

/************************************************************/

/*this is pin mux setting*/
void Init_Default_Pin_Mux(void)
{
    /*set gpio 20 21 for debug used*/
    pin_set_mode(GPIO20, MODE_GPIO);
    pin_set_mode(GPIO21, MODE_GPIO);

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
/*
 * using hmac_drbg to generate random K...
 *
 */
void hmac_drbg_generate_k(uint8_t *msg, uint32_t msg_length, uint8_t  *k)
{
    hmac_drbg_state    hmac_drbg_ctx;
    uint32_t           i;
    uint8_t            *ptr_k, *ptr_order;


    hmac_drbg_instantiate(&hmac_drbg_ctx, msg, msg_length);       /*RFC6979 section 3.2 step d~ step g*/

    while (1)
    {

        hmac_drbg_generate(k, &hmac_drbg_ctx, ECDSA_P256_LENGTH,
                           NULL, 0);                                      /*we need 32 bytes random k*/

        be_print_hex_bytes("random_k:", k, ECDSA_P256_LENGTH);

        /* we should compare random value k with order of NIST-P256 "q"
         * if k must be within [1, q-1]
         * remember: k and order is big endian.
         */

        ptr_order = (uint8_t *) nist_p256_order;
        ptr_k = k;

        for (i = 0; i < 32; i++)
        {
            if (*k < *ptr_order)
            {
                return;     /*ok we find k now*/
            }
            else
            {
                ptr_k++;
                ptr_order++;
            }
        }


        /*if code goto here.. it means k > q, we should regenerate k... should we call this??
          it seems that hmac_drbg_generate alreday update it...???? */
        hmac_drbg_update(&hmac_drbg_ctx, NULL, 0);

    }

}

void  test_ecdsa_signature(uint8_t *msg, uint32_t msg_len, uint8_t *exp_r, uint8_t *exp_s)
{
    uint8_t    random_k[32];
    sha256_context     sha256_ctx;

    uint8_t    h1_hash_value[SHA256_DIGEST_SIZE];
    uint8_t    seed_material[(ECDSA_P256_LENGTH) * 2];

    Signature_P256  signature;

    /*
     * of course, if you want to save stack space, you can only use one temp buffer.
     * convert big endian to little endian in temp buffer,
     * then memory move the temp buffer to the big data buffer...
     */
    /*In this case  SHA256_DIGEST_SIZE = ECDSA_P256_LENGTH , but it is not always equal!*/
    uint8_t    little_endian_hash[SHA256_DIGEST_SIZE], little_endian_k[ECDSA_P256_LENGTH];
    uint8_t    little_endian_private[ECDSA_P256_LENGTH];


    /*caculate h1 of RFC6979*/
    sha256_init(&sha256_ctx);
    sha256_update(&sha256_ctx, (uint8_t *) msg, msg_len);

    sha256_finish(&sha256_ctx, h1_hash_value);

    be_print_hex_bytes("h1 value:", h1_hash_value, SHA256_DIGEST_SIZE);

    /*
     *  according to RFC6979 Section 3.6.  Variants
     *   It is possible to use H(m) directly, instead of bits2octets(H(m)),
     * as part of the HMAC input.  As explained in Section 3.5, we use
     * bits2octets(H(m)) in order to ease integration into systems that
     * already use an (EC)DSA signature engine by sending it an already-
     * truncated hash value.  Using the whole H(m) does not introduce any
     * vulnerability.
     */

    memcpy(seed_material, private_key, ECDSA_P256_LENGTH);             /*because NIST P-256 is 32 bytes*/
    memcpy(seed_material + 32, h1_hash_value, ECDSA_P256_LENGTH);

    hmac_drbg_generate_k(seed_material, (ECDSA_P256_LENGTH * 2), random_k);

    /*
     *  because ECC algorithm is based little endain, so we must convert the format from BE to LE
     *  This is fast convert, based arm special instruction.
     *  Notice: the following pointer must be 4 bytes alignment!!
     *
     */

    buffer_endian_exchange( (uint32_t *) little_endian_private, (uint32_t *) private_key, (ECDSA_P256_LENGTH >> 2));
    buffer_endian_exchange( (uint32_t *) little_endian_k, (uint32_t *) random_k, (ECDSA_P256_LENGTH >> 2));
    buffer_endian_exchange( (uint32_t *) little_endian_hash, (uint32_t *) h1_hash_value, (SHA256_DIGEST_SIZE >> 2));


    /*ecdsa signature.*/
    gfp_ecc_curve_p256_init();

    gfp_ecdsa_p256_signature( &signature,
                              (uint32_t *) little_endian_hash,
                              (uint32_t *)little_endian_private, (uint32_t *) little_endian_k);


    le_print_hex_bytes("r:", (signature.r),  ECDSA_P256_LENGTH);
    le_print_hex_bytes("s:", (signature.s),  ECDSA_P256_LENGTH);

    /*compare the result.*/
    /*transfer little endian to big endian back, here we use seed_material for temp buffer*/
    buffer_endian_exchange( (uint32_t *) seed_material, (uint32_t *) (signature.r), (ECDSA_P256_LENGTH >> 2));
    buffer_endian_exchange( (uint32_t *) (seed_material + ECDSA_P256_LENGTH), (uint32_t *) (signature.s), (ECDSA_P256_LENGTH >> 2));

    /*compare result r*/
    if (( memcmp(seed_material, exp_r, ECDSA_P256_LENGTH) != 0) ||
            ( memcmp((seed_material + ECDSA_P256_LENGTH), exp_s, ECDSA_P256_LENGTH) != 0))
    {
        printf("OOPS, error \n");
        while (1);
    }

    printf("Success for test vector \n");

    printf("\n");

}

void test_ecdsa_verify(uint8_t  *msg, uint32_t  msg_length, uint8_t *vector_r, uint8_t *vector_s)
{
    sha256_context     sha256_ctx;

    uint8_t    hash_value[SHA256_DIGEST_SIZE];

    Signature_P256  signature;

    uint8_t    little_endian_hash[SHA256_DIGEST_SIZE];

    ECPoint_P256  public_key_LE;

    uint32_t   status;

    /*caculate hash*/
    sha256_init(&sha256_ctx);
    sha256_update(&sha256_ctx, (uint8_t *) msg, msg_length);
    sha256_finish(&sha256_ctx, hash_value);

    /*conver big endian data to little endian*/

    /*
     *  because ECC algorithm is based little endain, so we must convert the format from BE to LE
     *  This is fast convert, based arm special instruction.
     *  Notice: the following pointer must be 4 bytes alignment!!
     *  Sometime, vector_r or vector_s may not 4 bytes alignment...
     *  Because some protocol will add 0x41 0x04 in key... for this case, we should use convert_endian function
     *
     */

    buffer_endian_exchange( (uint32_t *) (signature.r), (uint32_t *) vector_r, (ECDSA_P256_LENGTH >> 2));
    buffer_endian_exchange( (uint32_t *) (signature.s), (uint32_t *) vector_s, (ECDSA_P256_LENGTH >> 2));

    buffer_endian_exchange( (uint32_t *) little_endian_hash, (uint32_t *) hash_value, (SHA256_DIGEST_SIZE >> 2));

    buffer_endian_exchange( (uint32_t *) (public_key_LE.x), (uint32_t *) public_key_x, (ECDSA_P256_LENGTH >> 2));
    buffer_endian_exchange( (uint32_t *) (public_key_LE.y), (uint32_t *) public_key_y, (ECDSA_P256_LENGTH >> 2));

    gfp_ecdsa_p256_verify_init();

    status = gfp_ecdsa_p256_verify(
                 &signature,
                 (uint32_t *) little_endian_hash,
                 &public_key_LE);

    if (status != STATUS_SUCCESS)
    {
        printf("Oops, Error in ECDSA p256 verify \n ");
        while (1);
    }

    printf("ECDSA p256 verification success \n");
    printf("\n");

}


/* Curve P256 Gx & Gy parameters and used for the
 *    hardware public key caculation result
 * Notice: this data is little endian.
 */
const uint32_t nist_p256_Gx[secp256r1_op_num] =
{
    0xd898c296, 0xf4a13945, 0x2deb33a0, 0x77037d81,
    0x63a440f2, 0xf8bce6e5, 0xe12c4247, 0x6b17d1f2
};

const uint32_t nist_p256_Gy[secp256r1_op_num] =
{
    0x37bf51f5, 0xcbb64068, 0x6b315ece, 0x2bce3357,
    0x7c0f9e16, 0x8ee7eb4a, 0xfe1a7f9b, 0x4fe342e2
};

void test_ecdsa_signature_verify(uint32_t length)
{
    /* This function is generate
     * 1. NIST- P256 ECC key pair
     * 2. a hash value some message.
     * 3. random k
     * 4. ecdsa signature     r,s
     * 5. ecdsa verification  r,s
     */

    sha256_context    sha256_ctx;

    /*the following static is avoid stack overflow*/

    static uint32_t   test_message[1024];

    static uint8_t    hash_value[SHA256_DIGEST_SIZE], le_hash_value[SHA256_DIGEST_SIZE];

    /*init random number */
    uint8_t    device_id[64] =
    {
        0x4F, 0xAA, 0x6C, 0xF0, 0x09, 0x05, 0x89, 0xF7, 0x56, 0x3B, 0x23, 0xE9, 0x2B, 0x19, 0x7D, 0x38,
        0x24, 0xD2, 0xAD, 0x4D, 0x3C, 0x9F, 0x20, 0xBA, 0x25, 0xDA, 0x20, 0xC3, 0xDE, 0x57, 0xEC, 0xE6,
        0x37, 0xA7, 0x80, 0x28, 0x34, 0xC0, 0x50, 0x66, 0xF1, 0x02, 0xF6, 0xB3, 0x9A, 0xF2, 0xE6, 0x2B,
        0xB3, 0x71, 0xFC, 0xCE, 0x04, 0x87, 0x51, 0xAD, 0x19, 0xE8, 0x00, 0x33, 0xA8, 0xBF, 0xA3, 0x6F
    };

    uint32_t   i, j, count, temp, status;

    uint8_t    *test_ptr;

    static uint8_t    test_private_key[ECDSA_P256_LENGTH];
    static uint8_t    random_k[ECDSA_P256_LENGTH];

    static uint8_t    le_test_private_key[ECDSA_P256_LENGTH];

    ECPoint_P256      public_key_point_LE;

    static uint8_t    le_random_k[ECDSA_P256_LENGTH];

    static uint8_t    seed_material[(ECDSA_P256_LENGTH * 2)];

    Signature_P256      caculate_signature_LE;

    test_message[0] = 0xDEADBEEF;
    test_message[1] = 0xC0FFEE43;
    test_message[2] = 0x54023098;

    /*generate key pair, using device id as personal seed*/
    if (length <= 16)
    {
        flash_get_unique_id( (uint32_t) device_id, length);   /*length bytes personal data*/
    }
    else
    {
        flash_get_unique_id( (uint32_t) device_id, 16);       /*maximum 16 bytes personal data*/
    }

    be_print_hex_bytes("id value:", device_id, 16);

    sha256_init(&sha256_ctx);
    sha256_update(&sha256_ctx, (uint8_t *) device_id, length);

    sha256_finish(&sha256_ctx, test_private_key);           /*so different flash has different private key.*/

    be_print_hex_bytes("test privatge value:", test_private_key, ECDSA_P256_LENGTH);

    /*convert big endian private key to little endian key format*/
#if 0
    convert_endian(le_test_private_key, test_private_key,  ECDSA_P256_LENGTH);
#else
    buffer_endian_exchange( (uint32_t *) le_test_private_key, (uint32_t *) test_private_key, (ECDSA_P256_LENGTH >> 2));
#endif

    /*TODO: k must be less than NIST P256 order... we don't check it here..*/

    /*key generate --- in fact, key generate is ecc point multiplication*/

    gfp_ecc_curve_p256_init();

    /*in fact, this is key generator function too... Notice: little endian format*/
    status = gfp_point_p256_mult( &public_key_point_LE,
                                  (ECPoint_P256 *) &Curve_Gx_p256, (uint32_t *) le_test_private_key);

    /*display little endian public key information. display in big endian format*/

    le_print_hex_bytes("public_key_x:", (public_key_point_LE.x), ECDSA_P256_LENGTH);
    le_print_hex_bytes("public_key_y:", (public_key_point_LE.y), ECDSA_P256_LENGTH);

    printf("\n");

    test_ptr = (uint8_t *) test_message;

    temp = 0;

    {
        /*init test pattern*/
        for (i = 3; i < 1024; i++)
        {
            temp += (test_message[i - 3] * 257) + (test_message[i - 2] * 617) + (test_message[i - 1] * 947);

            if ((i % 47) == 0)
            {
                __ROR(temp, 17);
            }

            if ((i % 29) == 0)
            {
                __ROR(temp, 5);
            }

            test_message[i] = temp;
        }



        for (j = 0; j < 4096; j++)
        {
            count = 1;

            for (i = j; i < 4096; i++)
            {
                sha256_init(&sha256_ctx);
                sha256_update(&sha256_ctx, test_ptr, count);

                sha256_finish(&sha256_ctx, hash_value);

                be_print_hex_bytes("hash_value:",  hash_value,  SHA256_DIGEST_SIZE);

                memcpy(seed_material, test_private_key, ECDSA_P256_LENGTH);             /*because NIST P-256 is 32 bytes*/
                memcpy(seed_material + 32, hash_value, ECDSA_P256_LENGTH);

                /*using hamc_drbg to generte random k*/
                hmac_drbg_generate_k(seed_material, (ECDSA_P256_LENGTH * 2), random_k);

#if 0
                convert_endian(le_hash_value, hash_value, ECDSA_P256_LENGTH);
                convert_endian(le_random_k, random_k, ECDSA_P256_LENGTH);       /*generate little endian format random key*/
#else
                buffer_endian_exchange( (uint32_t *) le_hash_value, (uint32_t *) hash_value, (ECDSA_P256_LENGTH >> 2));
                /*in fact, we don't care random_k is little endian or big endian... because it is just a random number... */
                buffer_endian_exchange( (uint32_t *) le_random_k, (uint32_t *) random_k, (ECDSA_P256_LENGTH >> 2));
#endif

                /*ecdsa signature.*/
                gfp_ecc_curve_p256_init();


                gfp_ecdsa_p256_signature( &caculate_signature_LE,
                                          (uint32_t *) le_hash_value,
                                          (uint32_t *) le_test_private_key, (uint32_t *) le_random_k);

                le_print_hex_bytes("r:", (uint8_t *) & (caculate_signature_LE.r),  ECDSA_P256_LENGTH);
                le_print_hex_bytes("s:", (uint8_t *) & (caculate_signature_LE.s),  ECDSA_P256_LENGTH);

                /*verification --- we skip caculate hash value again..*/

                gfp_ecdsa_p256_verify_init();

                status = gfp_ecdsa_p256_verify(
                             &caculate_signature_LE,
                             (uint32_t *) le_hash_value,
                             &public_key_point_LE);


                if (status != STATUS_SUCCESS)
                {
                    printf("Oops, signature is not equal verification \n ");

                    le_print_hex_bytes("correct r:",  (uint8_t *) & (caculate_signature_LE.r),  ECDSA_P256_LENGTH);

                    printf("error %ld %ld %ld   %lx \n", length, j,  i,  (uint32_t) (test_ptr));

                    while (1);
                }

                printf("success  %ld %ld   %lx \n", j,  i,  (uint32_t) (test_ptr));
                printf("\n\n");

                count++;

            }

            test_ptr++;

        }

    }

}



int main(void)
{
    const uint8_t      message[] = "sample";
    const uint8_t      message2[] = "test";

    /*we should set pinmux here or in SystemInit */
    Change_Ahb_System_Clk(SYS_48MHZ_CLK);

    Init_Default_Pin_Mux();

    /*init debug uart port for printf*/
    console_drv_init(PRINTF_BAUDRATE);

    Comm_Subsystem_Disable_LDO_Mode();//if don't load 569 FW, need to call the function.
    /*init software sha256 vector*/
    sha256_vector_init();

    printf("Hello Crypto ECDSA %s %s\n", __DATE__, __TIME__);

    /*following test pattern is in RFC 6979, A.2.5.*/

    test_ecdsa_signature((uint8_t *)  message,  6, (uint8_t *) sample_expect_vector_r, (uint8_t *) sample_expect_vector_s);
    test_ecdsa_signature((uint8_t *) message2, 4, (uint8_t *) test_expect_vector_r, (uint8_t *) test_expect_vector_s);

    test_ecdsa_verify((uint8_t *)  message,  6, (uint8_t *) sample_expect_vector_r, (uint8_t *) sample_expect_vector_s);
    test_ecdsa_verify((uint8_t *)  message2,  4, (uint8_t *) test_expect_vector_r, (uint8_t *) test_expect_vector_s);



    printf("finish crypto ecdsa  test \n");

    printf("\n\n");

    while (1);
}


void SetClockFreq(void)
{
    return;
}


