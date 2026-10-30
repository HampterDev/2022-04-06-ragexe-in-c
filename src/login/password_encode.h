#ifndef LOGINMODE_PASSWORD_ENCODE_H
#define LOGINMODE_PASSWORD_ENCODE_H

#include <stddef.h>

enum LoginPasswordEncoding {
    LOGIN_PASSWORD_SERVICE4,
    LOGIN_PASSWORD_ACF,
    LOGIN_PASSWORD_SERVICE7
};

int login_password_encode(enum LoginPasswordEncoding encoding,
                          const char *input, size_t input_size,
                          char *output, size_t output_size);

#endif
