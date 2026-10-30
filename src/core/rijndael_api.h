#ifndef CORE_RIJNDAEL_API_H
#define CORE_RIJNDAEL_API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int rijndael_encrypt(const unsigned char *key, size_t bytes,
                     const unsigned char *input, unsigned char *output);

#ifdef __cplusplus
}
#endif
#endif
