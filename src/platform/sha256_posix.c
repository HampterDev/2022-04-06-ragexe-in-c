#ifndef _WIN32
#include "platform/sha256.h"
#include <string.h>

struct Sha256 {
    uint32_t state[8];
    uint64_t length;
    uint8_t block[64];
    size_t used;
};

static const uint32_t round_constants[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static uint32_t rotate_right(uint32_t value, unsigned count)
{
    return value >> count | value << (32 - count);
}

static void compress(uint32_t state[8], const uint8_t block[64])
{
    uint32_t schedule[64], a, b, c, d, e, f, g, h;
    unsigned i;

    for (i = 0; i < 16; ++i)
        schedule[i] = (uint32_t)block[i * 4] << 24 | (uint32_t)block[i * 4 + 1] << 16
                      | (uint32_t)block[i * 4 + 2] << 8 | block[i * 4 + 3];
    for (i = 16; i < 64; ++i) {
        uint32_t s0 = rotate_right(schedule[i - 15], 7) ^ rotate_right(schedule[i - 15], 18) ^ schedule[i - 15] >> 3;
        uint32_t s1 = rotate_right(schedule[i - 2], 17) ^ rotate_right(schedule[i - 2], 19) ^ schedule[i - 2] >> 10;
        schedule[i] = schedule[i - 16] + s0 + schedule[i - 7] + s1;
    }
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];
    for (i = 0; i < 64; ++i) {
        uint32_t s1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
        uint32_t choice = (e & f) ^ (~e & g);
        uint32_t first = h + s1 + choice + round_constants[i] + schedule[i];
        uint32_t s0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t second = s0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + first;
        d = c;
        c = b;
        b = a;
        a = first + second;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

static void sha256_update(struct Sha256 *hash, const uint8_t *data, size_t size)
{
    hash->length += size;
    while (size) {
        size_t take = 64 - hash->used < size ? 64 - hash->used : size;
        memcpy(hash->block + hash->used, data, take);
        hash->used += take;
        data += take;
        size -= take;
        if (hash->used == 64) {
            compress(hash->state, hash->block);
            hash->used = 0;
        }
    }
}

int platform_sha256(const void *const *parts, const size_t *sizes, size_t count,
                    uint8_t digest[PLATFORM_SHA256_SIZE])
{
    static const uint8_t padding[64] = {0x80};
    struct Sha256 hash = {
        {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19}, 0, {0}, 0
    };
    uint8_t length[8];
    uint64_t bits;
    size_t i;

    for (i = 0; i < count; ++i)
        sha256_update(&hash, parts[i], sizes[i]);
    bits = hash.length * 8;
    for (i = 0; i < 8; ++i)
        length[i] = (uint8_t)(bits >> (56 - i * 8));
    sha256_update(&hash, padding, hash.used < 56 ? 56 - hash.used : 120 - hash.used);
    sha256_update(&hash, length, 8);
    for (i = 0; i < 8; ++i) {
        digest[i * 4] = (uint8_t)(hash.state[i] >> 24);
        digest[i * 4 + 1] = (uint8_t)(hash.state[i] >> 16);
        digest[i * 4 + 2] = (uint8_t)(hash.state[i] >> 8);
        digest[i * 4 + 3] = (uint8_t)hash.state[i];
    }
    return 1;
}
#endif
