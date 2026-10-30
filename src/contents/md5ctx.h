#ifndef CONTENTS_MD5CTX_H
#define CONTENTS_MD5CTX_H

#include "base.h"

struct MD5Context {
    unsigned bits[2];
    unsigned state[4];
    unsigned char block[64];
};
OFFSET_ASSERT(MD5Context, state, 0x08);
OFFSET_ASSERT(MD5Context, block, 0x18);
SIZE_ASSERT(MD5Context, 0x58);

void md5ctx_init(struct MD5Context *self);
void md5ctx_update(struct MD5Context *self, const void *data, int length);
void md5ctx_final(struct MD5Context *self, unsigned char digest[16]);

#endif
