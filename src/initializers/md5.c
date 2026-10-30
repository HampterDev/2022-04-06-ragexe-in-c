#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "initializers/md5.h"

#define MD5_FILE_LIMIT 500000000u

struct Md5 *md5_construct(struct Md5 *self)
{
    return self;
}

void md5_destruct(struct Md5 *self)
{
    (void)self;
}

static void md5_decode(unsigned int *dst, const unsigned char *src, unsigned words)
{
    while (words--) {
        *dst++ = (unsigned int)src[0] | (unsigned int)src[1] << 8 |
                 (unsigned int)src[2] << 16 | (unsigned int)src[3] << 24;
        src += 4;
    }
}

#define ROL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define F(x, y, z) ((((y) ^ (z)) & (x)) ^ (z))
#define G(x, y, z) ((((x) ^ (y)) & (z)) ^ (y))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))
#define STEP(f, a, b, c, d, x, t, s) ((a) += f((b), (c), (d)) + (x) + (t), (a) = ROL((a), (s)) + (b))

static void md5_transform(unsigned int state[4], const unsigned int x[16])
{
    unsigned int a = state[0], b = state[1], c = state[2], d = state[3];

    STEP(F, a, b, c, d, x[ 0], 0xd76aa478,  7); STEP(F, d, a, b, c, x[ 1], 0xe8c7b756, 12);
    STEP(F, c, d, a, b, x[ 2], 0x242070db, 17); STEP(F, b, c, d, a, x[ 3], 0xc1bdceee, 22);
    STEP(F, a, b, c, d, x[ 4], 0xf57c0faf,  7); STEP(F, d, a, b, c, x[ 5], 0x4787c62a, 12);
    STEP(F, c, d, a, b, x[ 6], 0xa8304613, 17); STEP(F, b, c, d, a, x[ 7], 0xfd469501, 22);
    STEP(F, a, b, c, d, x[ 8], 0x698098d8,  7); STEP(F, d, a, b, c, x[ 9], 0x8b44f7af, 12);
    STEP(F, c, d, a, b, x[10], 0xffff5bb1, 17); STEP(F, b, c, d, a, x[11], 0x895cd7be, 22);
    STEP(F, a, b, c, d, x[12], 0x6b901122,  7); STEP(F, d, a, b, c, x[13], 0xfd987193, 12);
    STEP(F, c, d, a, b, x[14], 0xa679438e, 17); STEP(F, b, c, d, a, x[15], 0x49b40821, 22);

    STEP(G, a, b, c, d, x[ 1], 0xf61e2562,  5); STEP(G, d, a, b, c, x[ 6], 0xc040b340,  9);
    STEP(G, c, d, a, b, x[11], 0x265e5a51, 14); STEP(G, b, c, d, a, x[ 0], 0xe9b6c7aa, 20);
    STEP(G, a, b, c, d, x[ 5], 0xd62f105d,  5); STEP(G, d, a, b, c, x[10], 0x02441453,  9);
    STEP(G, c, d, a, b, x[15], 0xd8a1e681, 14); STEP(G, b, c, d, a, x[ 4], 0xe7d3fbc8, 20);
    STEP(G, a, b, c, d, x[ 9], 0x21e1cde6,  5); STEP(G, d, a, b, c, x[14], 0xc33707d6,  9);
    STEP(G, c, d, a, b, x[ 3], 0xf4d50d87, 14); STEP(G, b, c, d, a, x[ 8], 0x455a14ed, 20);
    STEP(G, a, b, c, d, x[13], 0xa9e3e905,  5); STEP(G, d, a, b, c, x[ 2], 0xfcefa3f8,  9);
    STEP(G, c, d, a, b, x[ 7], 0x676f02d9, 14); STEP(G, b, c, d, a, x[12], 0x8d2a4c8a, 20);

    STEP(H, a, b, c, d, x[ 5], 0xfffa3942,  4); STEP(H, d, a, b, c, x[ 8], 0x8771f681, 11);
    STEP(H, c, d, a, b, x[11], 0x6d9d6122, 16); STEP(H, b, c, d, a, x[14], 0xfde5380c, 23);
    STEP(H, a, b, c, d, x[ 1], 0xa4beea44,  4); STEP(H, d, a, b, c, x[ 4], 0x4bdecfa9, 11);
    STEP(H, c, d, a, b, x[ 7], 0xf6bb4b60, 16); STEP(H, b, c, d, a, x[10], 0xbebfbc70, 23);
    STEP(H, a, b, c, d, x[13], 0x289b7ec6,  4); STEP(H, d, a, b, c, x[ 0], 0xeaa127fa, 11);
    STEP(H, c, d, a, b, x[ 3], 0xd4ef3085, 16); STEP(H, b, c, d, a, x[ 6], 0x04881d05, 23);
    STEP(H, a, b, c, d, x[ 9], 0xd9d4d039,  4); STEP(H, d, a, b, c, x[12], 0xe6db99e5, 11);
    STEP(H, c, d, a, b, x[15], 0x1fa27cf8, 16); STEP(H, b, c, d, a, x[ 2], 0xc4ac5665, 23);

    STEP(I, a, b, c, d, x[ 0], 0xf4292244,  6); STEP(I, d, a, b, c, x[ 7], 0x432aff97, 10);
    STEP(I, c, d, a, b, x[14], 0xab9423a7, 15); STEP(I, b, c, d, a, x[ 5], 0xfc93a039, 21);
    STEP(I, a, b, c, d, x[12], 0x655b59c3,  6); STEP(I, d, a, b, c, x[ 3], 0x8f0ccc92, 10);
    STEP(I, c, d, a, b, x[10], 0xffeff47d, 15); STEP(I, b, c, d, a, x[ 1], 0x85845dd1, 21);
    STEP(I, a, b, c, d, x[ 8], 0x6fa87e4f,  6); STEP(I, d, a, b, c, x[15], 0xfe2ce6e0, 10);
    STEP(I, c, d, a, b, x[ 6], 0xa3014314, 15); STEP(I, b, c, d, a, x[13], 0x4e0811a1, 21);
    STEP(I, a, b, c, d, x[ 4], 0xf7537e82,  6); STEP(I, d, a, b, c, x[11], 0xbd3af235, 10);
    STEP(I, c, d, a, b, x[ 2], 0x2ad7d2bb, 15); STEP(I, b, c, d, a, x[ 9], 0xeb86d391, 21);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

void md5_init(struct Md5 *self)
{
    self->state[0] = 0x67452301;
    self->state[1] = 0xefcdab89;
    self->state[2] = 0x98badcfe;
    self->state[3] = 0x10325476;
    self->bytes_lo = 0;
    self->bytes_hi = 0;
}

void md5_update(struct Md5 *self, const void *data, unsigned len)
{
    const unsigned char *in = (const unsigned char *)data;
    unsigned used = self->bytes_lo & 0x3f, space = 64 - used;

    self->bytes_lo += len;
    if (self->bytes_lo < len)
        self->bytes_hi++;

    if (len < space) {
        memcpy(self->block.bytes + used, in, len);
        return;
    }
    if (used) {
        memcpy(self->block.bytes + used, in, space);
        md5_decode(self->block.words, self->block.bytes, 16);
        md5_transform(self->state, self->block.words);
        in += space;
        len -= space;
    }
    for (; len >= 64; in += 64, len -= 64) {
        md5_decode(self->block.words, in, 16);
        md5_transform(self->state, self->block.words);
    }
    if (len)
        memcpy(self->block.bytes, in, len);
}

const unsigned char *md5_final(struct Md5 *self)
{
    unsigned used = self->bytes_lo & 0x3f, i;
    unsigned char *pad = self->block.bytes + used;
    unsigned char *digest = (unsigned char *)self->state;
    unsigned pad_len = 63 - used;

    *pad++ = 0x80;
    if (pad_len < 8) {
        memset(pad, 0, pad_len);
        md5_decode(self->block.words, self->block.bytes, 16);
        md5_transform(self->state, self->block.words);
        pad = self->block.bytes;
        pad_len = 64;
    }
    memset(pad, 0, pad_len - 8);
    md5_decode(self->block.words, self->block.bytes, 14);
    self->block.words[14] = self->bytes_lo << 3;
    self->block.words[15] = self->bytes_hi << 3 | self->bytes_lo >> 29;
    md5_transform(self->state, self->block.words);

    for (i = 0; i < 4; i++) {
        unsigned int word = self->state[i];
        digest[4 * i + 0] = (unsigned char)word;
        digest[4 * i + 1] = (unsigned char)(word >> 8);
        digest[4 * i + 2] = (unsigned char)(word >> 16);
        digest[4 * i + 3] = (unsigned char)(word >> 24);
    }
    return digest;
}

void md5_digest(struct Md5 *self, const void *data, unsigned len, unsigned char digest[16])
{
    md5_init(self);
    md5_update(self, data, len);
    md5_final(self);
    memcpy(digest, self->state, 16);
}

static unsigned int fold_digest(const unsigned char d[16])
{
    return (unsigned int)(d[0] ^ d[1] ^ d[2] ^ d[3]) |
           (unsigned int)(d[4] ^ d[5] ^ d[6] ^ d[7]) << 8 |
           (unsigned int)(d[8] ^ d[9] ^ d[10] ^ d[11]) << 16 |
           (unsigned int)(d[12] ^ d[13] ^ d[14] ^ d[15]) << 24;
}

unsigned int md5_hash32(struct Md5 *self, const void *data, unsigned len)
{
    unsigned char digest[16] = { 0 };

    md5_digest(self, data, len, digest);
    return fold_digest(digest);
}

unsigned int md5_hash32_string(struct Md5 *self, const char *text)
{
    return md5_hash32(self, text, (unsigned)strlen(text));
}

unsigned int md5_hash32_wstring(struct Md5 *self, const wchar_t *text)
{
    char ansi[MAX_PATH];

    WideCharToMultiByte(CP_ACP, 0, text, -1, ansi, MAX_PATH, 0, 0);
    return md5_hash32_string(self, ansi);
}

unsigned int md5_hash32_wstring_lower(struct Md5 *self, const wchar_t *text)
{
    wchar_t lower[MAX_PATH];

    wcscpy_s(lower, MAX_PATH, text);
    _wcslwr_s(lower, MAX_PATH);
    return md5_hash32_wstring(self, lower);
}

unsigned int md5_hash32_file(struct Md5 *self, const wchar_t *path)
{
    LARGE_INTEGER size;
    HANDLE file, mapping;
    const void *view;
    unsigned char digest[16] = { 0 };
    unsigned int hash = 0;

    if (!path || !path[0])
        return 0;
    file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return 0;
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0) {
        CloseHandle(file);
        return 0;
    }
    if (size.QuadPart > MD5_FILE_LIMIT)
        size.QuadPart = MD5_FILE_LIMIT;

    mapping = CreateFileMappingW(file, 0, PAGE_READONLY, size.HighPart, size.LowPart, 0);
    if (mapping) {
        view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, size.LowPart);
        if (view) {
            md5_digest(self, view, size.LowPart, digest);
            hash = fold_digest(digest);
            UnmapViewOfFile(view);
        }
        CloseHandle(mapping);
    }
    CloseHandle(file);
    return hash;
}
