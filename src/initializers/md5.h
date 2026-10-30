#ifndef INITIALIZERS_MD5_H
#define INITIALIZERS_MD5_H

#include <stddef.h>
#include "base.h"

struct Md5 {
    union {
        unsigned char bytes[64];
        unsigned int words[16];
    } block;
    unsigned int state[4];
    unsigned int bytes_lo;
    unsigned int bytes_hi;
};
OFFSET_ASSERT(Md5, state, 0x40);
OFFSET_ASSERT(Md5, bytes_lo, 0x50);
SIZE_ASSERT(Md5, 0x58);

struct Md5 *md5_construct(struct Md5 *self);
void md5_destruct(struct Md5 *self);

void md5_init(struct Md5 *self);
void md5_update(struct Md5 *self, const void *data, unsigned len);
const unsigned char *md5_final(struct Md5 *self);
void md5_digest(struct Md5 *self, const void *data, unsigned len, unsigned char digest[16]);

unsigned int md5_hash32(struct Md5 *self, const void *data, unsigned len);
unsigned int md5_hash32_string(struct Md5 *self, const char *text);
unsigned int md5_hash32_wstring(struct Md5 *self, const wchar_t *text);
unsigned int md5_hash32_wstring_lower(struct Md5 *self, const wchar_t *text);
unsigned int md5_hash32_file(struct Md5 *self, const wchar_t *path);

#endif
