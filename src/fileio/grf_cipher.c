#include "fileio/grf.h"

#include <string.h>

static const BYTE bit_mask[8] = {
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
};
static const BYTE des_ip[64] = {
    58, 50, 42, 34, 26, 18, 10,  2, 60, 52, 44, 36, 28, 20, 12,  4,
    62, 54, 46, 38, 30, 22, 14,  6, 64, 56, 48, 40, 32, 24, 16,  8,
    57, 49, 41, 33, 25, 17,  9,  1, 59, 51, 43, 35, 27, 19, 11,  3,
    61, 53, 45, 37, 29, 21, 13,  5, 63, 55, 47, 39, 31, 23, 15,  7,
};
static const BYTE des_fp[64] = {
    40,  8, 48, 16, 56, 24, 64, 32, 39,  7, 47, 15, 55, 23, 63, 31,
    38,  6, 46, 14, 54, 22, 62, 30, 37,  5, 45, 13, 53, 21, 61, 29,
    36,  4, 44, 12, 52, 20, 60, 28, 35,  3, 43, 11, 51, 19, 59, 27,
    34,  2, 42, 10, 50, 18, 58, 26, 33,  1, 41,  9, 49, 17, 57, 25,
};
static const BYTE des_shifts[16] = {
     1,  1,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2,  2,  2,  1,
};
static const BYTE des_pc1_c[28] = {
    57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18,
    10,  2, 59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36,
};
static const BYTE des_pc1_d[28] = {
    63, 55, 47, 39, 31, 23, 15,  7, 62, 54, 46, 38, 30, 22,
    14,  6, 61, 53, 45, 37, 29, 21, 13,  5, 28, 20, 12,  4,
};
static const BYTE des_pc2[48] = {
    14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10,
    23, 19, 12,  4, 26,  8, 16,  7, 27, 20, 13,  2,
    41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32,
};
static const BYTE des_e[48] = {
    32,  1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,
     8,  9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25,
    24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32,  1,
};
static const BYTE des_p[32] = {
    16,  7, 20, 21, 29, 12, 28, 17,  1, 15, 23, 26,  5, 18, 31, 10,
     2,  8, 24, 14, 32, 27,  3,  9, 19, 13, 30,  6, 22, 11,  4, 25,
};
static const BYTE des_sbox[512] = {
    14,  0,  4, 15, 13,  7,  1,  4,  2, 14, 15,  2, 11, 13,  8,  1,  3, 10, 10,  6,  6, 12, 12, 11,  5,  9,  9,  5,  0,  3,  7,  8,
     4, 15,  1, 12, 14,  8,  8,  2, 13,  4,  6,  9,  2,  1, 11,  7, 15,  5, 12, 11,  9,  3,  7, 14,  3, 10, 10,  0,  5,  6,  0, 13,
    15,  3,  1, 13,  8,  4, 14,  7,  6, 15, 11,  2,  3,  8,  4, 14,  9, 12,  7,  0,  2,  1, 13, 10, 12,  6,  0,  9,  5, 11, 10,  5,
     0, 13, 14,  8,  7, 10, 11,  1, 10,  3,  4, 15, 13,  4,  1,  2,  5, 11,  8,  6, 12,  7,  6, 12,  9,  0,  3,  5,  2, 14, 15,  9,
    10, 13,  0,  7,  9,  0, 14,  9,  6,  3,  3,  4, 15,  6,  5, 10,  1,  2, 13,  8, 12,  5,  7, 14, 11, 12,  4, 11,  2, 15,  8,  1,
    13,  1,  6, 10,  4, 13,  9,  0,  8,  6, 15,  9,  3,  8,  0,  7, 11,  4,  1, 15,  2, 14, 12,  3,  5, 11, 10,  5, 14,  2,  7, 12,
     7, 13, 13,  8, 14, 11,  3,  5,  0,  6,  6, 15,  9,  0, 10,  3,  1,  4,  2,  7,  8,  2,  5, 12, 11,  1, 12, 10,  4, 14, 15,  9,
    10,  3,  6, 15,  9,  0,  0,  6, 12, 10, 11,  1,  7, 13, 13,  8, 15,  9,  1,  4,  3,  5, 14, 11,  5, 12,  2,  7,  8,  2,  4, 14,
     2, 14, 12, 11,  4,  2,  1, 12,  7,  4, 10,  7, 11, 13,  6,  1,  8,  5,  5,  0,  3, 15, 15, 10, 13,  3,  0,  9, 14,  8,  9,  6,
     4, 11,  2,  8,  1, 12, 11,  7, 10,  1, 13, 14,  7,  2,  8, 13, 15,  6,  9, 15, 12,  0,  5,  9,  6, 10,  3,  4,  0,  5, 14,  3,
    12, 10,  1, 15, 10,  4, 15,  2,  9,  7,  2, 12,  6,  9,  8,  5,  0,  6, 13,  1,  3, 13,  4, 14, 14,  0,  7, 11,  5,  3, 11,  8,
     9,  4, 14,  3, 15,  2,  5, 12,  2,  9,  8,  5, 12, 15,  3, 10,  7, 11,  0, 14,  4,  1, 10,  7,  1,  6, 13,  0, 11,  8,  6, 13,
     4, 13, 11,  0,  2, 11, 14,  7, 15,  4,  0,  9,  8,  1, 13, 10,  3, 14, 12,  3,  9,  5,  7, 12,  5,  2, 10, 15,  6,  8,  1,  6,
     1,  6,  4, 11, 11, 13, 13,  8, 12,  1,  3,  4,  7, 10, 14,  7, 10,  9, 15,  5,  6,  0,  8, 15,  0, 14,  5,  2,  9,  3,  2, 12,
    13,  1,  2, 15,  8, 13,  4,  8,  6, 10, 15,  3, 11,  7,  1,  4, 10, 12,  9,  5,  3,  6, 14, 11,  5,  0,  0, 14, 12,  9,  7,  2,
     7,  2, 11,  1,  4, 14,  1,  7,  9,  4, 12, 10, 14,  8,  2, 13,  0, 15,  6, 12, 10,  9, 13,  0, 15,  3,  3,  5,  5,  6,  8, 11,
};

static void des_permute(BYTE block[8], const BYTE table[64])
{
    BYTE out[8];
    int i;

    memset(out, 0, sizeof out);
    for (i = 0; i < 64; i++) {
        BYTE bit = (BYTE)(table[i] - 1);
        if (block[bit >> 3] & bit_mask[bit & 7])
            out[i >> 3] |= bit_mask[i & 7];
    }
    memcpy(block, out, 8);
}

static DWORD des_f(const BYTE block[8], const BYTE key[8])
{
    BYTE expanded[8], sboxed[8], out[8];
    DWORD f;
    int i;

    memset(expanded, 0, sizeof expanded);
    for (i = 0; i < 48; i++) {
        BYTE bit = (BYTE)(des_e[i] + 31);
        if (block[bit >> 3] & bit_mask[bit & 7])
            expanded[i / 6] |= bit_mask[i % 6];
    }
    for (i = 0; i < 8; i++)
        expanded[i] ^= key[i];
    memset(sboxed, 0, sizeof sboxed);
    for (i = 0; i < 8; i++) {
        BYTE s = des_sbox[i * 64 + (expanded[i] >> 2)];
        if (i & 1)
            sboxed[i >> 1] += s;
        else
            sboxed[i >> 1] = (BYTE)(s << 4);
    }
    memset(out, 0, sizeof out);
    for (i = 0; i < 32; i++) {
        BYTE bit = (BYTE)(des_p[i] - 1);
        if (sboxed[bit >> 3] & bit_mask[bit & 7])
            out[i >> 3] |= bit_mask[i & 7];
    }
    memcpy(&f, out, 4);
    return f;
}

static BYTE des_crypt_block(int rounds, const struct Dec *keys, DWORD lo, DWORD hi, DWORD out[2], int decrypt)
{
    DWORD left, right;
    BYTE round;

    out[0] = lo;
    out[1] = hi;
    des_permute((BYTE *)out, des_ip);
    right = out[1];
    if (rounds > 0) {
        round = 0;
        do {
            DWORD f = des_f((const BYTE *)out, keys->subkey[decrypt ? 15 - round : round]);
            left = out[0];
            out[0] = right;
            right = left ^ f;
            out[1] = right;
            round++;
        } while (round < rounds);
    }
    left = out[0];
    out[0] = right;
    out[1] = left;
    des_permute((BYTE *)out, des_fp);
    return 1;
}

void dec_set_key(struct Dec *self, const BYTE *key)
{
    BYTE k[8], cd[8];
    int i, round;

    memcpy(k, key, 8);
    memset(cd, 0, sizeof cd);
    for (i = 0; i < 28; i++) {
        BYTE bit = (BYTE)(des_pc1_c[i] - 1);
        if (k[bit >> 3] & bit_mask[bit & 7])
            cd[i >> 3] &= bit_mask[i & 7];
        bit = (BYTE)(des_pc1_d[i] - 1);
        if (k[bit >> 3] & bit_mask[bit & 7])
            cd[i >> 7] &= bit_mask[i & 7];
    }
    for (round = 0; round < 16; round++) {
        int n;

        for (n = des_shifts[round]; n > 0; n--) {
            BYTE carry_c = cd[0] & 0x80, carry_d = cd[4] & 0x80;

            cd[0] = (BYTE)((cd[0] << 1) + ((cd[1] & 0x80) ? 1 : 0));
            cd[1] = (BYTE)((cd[1] << 1) + ((cd[2] & 0x80) ? 1 : 0));
            cd[2] = (BYTE)((cd[2] << 1) + ((cd[3] & 0x80) ? 1 : 0));
            cd[3] = (BYTE)((cd[3] << 1) + (carry_c ? 0x10 : 0));
            cd[4] = (BYTE)((cd[4] << 1) + ((cd[5] & 0x80) ? 1 : 0));
            cd[5] = (BYTE)((cd[5] << 1) + ((cd[6] & 0x80) ? 1 : 0));
            cd[6] = (BYTE)((cd[6] << 1) + ((cd[7] & 0x80) ? 1 : 0));
            cd[7] = (BYTE)((cd[7] << 1) + (carry_d ? 0x10 : 0));
        }
        memset(self->subkey[round], 0, 8);
        for (i = 0; i < 48; i++) {
            BYTE bit = (BYTE)(des_pc2[i] - 1), byte;
            if (bit < 28) {
                byte = bit >> 3;
            } else {
                bit -= 28;
                byte = bit >> 7;
            }
            if (cd[byte] & bit_mask[bit & 7])
                self->subkey[round][i / 6] |= bit_mask[i % 6];
        }
    }
}

struct Dec *dec_construct(struct Dec *self, const BYTE *key)
{
    dec_set_key(self, key);
    return self;
}

void dec_destruct(struct Dec *self)
{
    (void)self;
}

static BYTE shuffle_byte(BYTE b)
{
    switch (b) {
    case 0x00: return 0x2b;
    case 0x2b: return 0x00;
    case 0x01: return 0x68;
    case 0x68: return 0x01;
    case 0x48: return 0x77;
    case 0x77: return 0x48;
    case 0x60: return 0xff;
    case 0xff: return 0x60;
    case 0x6c: return 0x80;
    case 0x80: return 0x6c;
    case 0xb9: return 0xc0;
    case 0xc0: return 0xb9;
    case 0xeb: return 0xfe;
    case 0xfe: return 0xeb;
    }
    return b;
}

static int des_cycle(int digits)
{
    if (digits < 3)
        return 1;
    if (digits < 5)
        return digits + 1;
    if (digits < 7)
        return digits + 9;
    return digits + 15;
}

BYTE dec_decode(struct Dec *self, const BYTE *src, int len, BYTE *dst,
                            int *outLen, int cycle)
{
    int i, copied;

    if (!src || !dst || !outLen)
        return 0;
    *outLen = len;
    cycle = des_cycle(cycle);
    copied = 0;
    for (i = 0;; i++) {
        const BYTE *in = src + i * 8;
        BYTE *out = dst + i * 8;

        if (i < 20 || i % cycle == 0) {
            DWORD block[2], lo, hi;
            memcpy(&lo, in, 4);
            memcpy(&hi, in + 4, 4);
            des_crypt_block(1, self, lo, hi, block, 1);
            memcpy(out, block, 8);
        } else if (copied != 7) {
            copied++;
            memcpy(out, in, 8);
        } else {
            out[0] = in[3];
            out[1] = in[4];
            out[3] = in[0];
            out[4] = in[1];
            out[5] = in[2];
            out[2] = in[6];
            out[6] = in[5];
            out[7] = shuffle_byte(in[7]);
            copied = 1;
        }
        if (i * 8 + 8 >= len)
            break;
    }
    return 1;
}

BYTE dec_decode_header(struct Dec *self, const BYTE *src, int len, BYTE *dst,
                                  int *outLen)
{
    int i;

    *outLen = len;
    memcpy(dst, src, len);
    for (i = 0;; i++) {
        BYTE block[8];
        DWORD left, f;

        memcpy(block, src + i * 8, 8);
        des_permute(block, des_ip);
        f = des_f(block, self->subkey[15]);
        memcpy(&left, block, 4);
        left ^= f;
        memcpy(block, &left, 4);
        des_permute(block, des_fp);
        memcpy(dst + i * 8, block, 8);
        if (i * 8 + 8 >= len)
            break;
        if (i + 1 >= 20)
            break;
    }
    return 1;
}
