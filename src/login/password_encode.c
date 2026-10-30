#include "login/password_encode.h"
#include "core/rijndael_api.h"

static const unsigned char repeated_key[32] = {
    0x06,0xa9,0x21,0x40,0x36,0xb8,0xa1,0x5b,0x51,0x2e,0x03,0xd5,0x34,0x12,0x00,0x06,
    0x06,0xa9,0x21,0x40,0x36,0xb8,0xa1,0x5b,0x51,0x2e,0x03,0xd5,0x34,0x12,0x00,0x06
};
static const unsigned char service7_key[24] = {
    0x06,0xa9,0x21,0x40,0x36,0xb8,0xa1,0x5b,0x51,0x2e,0x03,0xd5,0x34,0x12,0x00,0x06,
    0x3d,0xaf,0xba,0x42,0x9d,0x9e,0xb4,0x30
};

int login_password_encode(enum LoginPasswordEncoding encoding,
                          const char *input, size_t input_size,
                          char *output, size_t output_size)
{
    const unsigned char *key = repeated_key;
    size_t bytes;

    switch (encoding) {
    case LOGIN_PASSWORD_SERVICE4: bytes = 24; break;
    case LOGIN_PASSWORD_ACF: bytes = 32; break;
    case LOGIN_PASSWORD_SERVICE7: bytes = 24; key = service7_key; break;
    default: return 0;
    }
    if (!input || !output || input_size < bytes || output_size < bytes)
        return 0;
    return rijndael_encrypt(key, bytes, (const unsigned char *)input, (unsigned char *)output);
}
