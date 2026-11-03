#ifndef LOGINMODE_PASSWORD_ENCODE_H
#define LOGINMODE_PASSWORD_ENCODE_H

#include <stddef.h>

/* The 0x0acf login step sends the password as one 32-byte Rijndael block. */
#define LOGIN_PASSWORD_BYTES 32

int login_password_encode(const char *input, size_t input_size, char *output, size_t output_size);

#endif
