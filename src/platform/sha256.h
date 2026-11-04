#ifndef PLATFORM_SHA256_H
#define PLATFORM_SHA256_H

#include <stddef.h>
#include <stdint.h>

#define PLATFORM_SHA256_SIZE 32

int platform_sha256(const void *const *parts, const size_t *sizes, size_t count,
                    uint8_t digest[PLATFORM_SHA256_SIZE]);

#endif
