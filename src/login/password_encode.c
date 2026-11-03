#include "login/password_encode.h"
#include "core/rijndael_api.h"

static const unsigned char repeated_key[32] = {
    0x06,0xa9,0x21,0x40,0x36,0xb8,0xa1,0x5b,0x51,0x2e,0x03,0xd5,0x34,0x12,0x00,0x06,
    0x06,0xa9,0x21,0x40,0x36,0xb8,0xa1,0x5b,0x51,0x2e,0x03,0xd5,0x34,0x12,0x00,0x06
};

int login_password_encode(const char *input, size_t input_size, char *output, size_t output_size)
{
    if (!input || !output || input_size < LOGIN_PASSWORD_BYTES || output_size < LOGIN_PASSWORD_BYTES)
        return 0;
    return rijndael_encrypt(repeated_key, LOGIN_PASSWORD_BYTES, (const unsigned char *)input,
                            (unsigned char *)output);
}
