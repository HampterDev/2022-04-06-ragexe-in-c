#include "login/password_encode.h"
#include "core/rijndael_api.h"
#include "password_vectors.h"
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check_aes128(void)
{
    const unsigned char key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    const unsigned char input[16] = {
        0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff
    };
    const unsigned char expected[16] = {
        0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a
    };
    unsigned char output[16];
    assert(rijndael_encrypt(key, 16, input, output));
    assert(memcmp(output, expected, sizeof output) == 0);
}

static void check_protocol_bytes(void)
{
    uint32_t state = 0x91e10da5u;
    uint64_t hash = UINT64_C(14695981039346656037);
    size_t bytes = LOGIN_PASSWORD_BYTES, i;
    int n;
    for (n = 0; n < 4096; ++n) {
        unsigned char input[32], output[33], overlap[64];
        for (i = 0; i < bytes; ++i) {
            state = state * 1664525u + 1013904223u;
            input[i] = n == 0 ? 0 : n == 1 ? 255 : n == 2 ? (unsigned char)i : (unsigned char)(state >> 24);
        }
        memset(output, 0xa5, sizeof output);
        assert(login_password_encode((char *)input, bytes, (char *)output, bytes));
        assert(output[bytes] == 0xa5);
        for (i = 0; i < bytes; ++i) {
            hash ^= output[i];
            hash *= UINT64_C(1099511628211);
        }
        if (n < 3) {
            assert(memcmp(output, password_vectors[n], bytes) == 0);
            memcpy(overlap, input, bytes);
            assert(login_password_encode((char *)overlap, bytes, (char *)overlap, bytes));
            assert(memcmp(overlap, output, bytes) == 0);
            memcpy(overlap, input, bytes);
            assert(login_password_encode((char *)overlap, bytes, (char *)overlap + 1, bytes));
            assert(memcmp(overlap + 1, output, bytes) == 0);
            memcpy(overlap + 1, input, bytes);
            assert(login_password_encode((char *)overlap + 1, bytes, (char *)overlap, bytes));
            assert(memcmp(overlap, output, bytes) == 0);
        }
    }
    assert(hash == password_hash);
}

static void check_boundaries(void)
{
    SYSTEM_INFO info;
    unsigned char *input, *output;
    DWORD old;
    size_t page, bytes = LOGIN_PASSWORD_BYTES, short_size, i;
    unsigned char *in, *out;
    GetSystemInfo(&info);
    page = info.dwPageSize;
    input = VirtualAlloc(NULL, page * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    output = VirtualAlloc(NULL, page * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(input && output);
    assert(VirtualProtect(input + page, page, PAGE_NOACCESS, &old));
    assert(VirtualProtect(output + page, page, PAGE_NOACCESS, &old));
    in = input + page - bytes;
    out = output + page - bytes;
    memset(in, 0, bytes);
    memset(out - 1, 0xa5, bytes + 1);
    assert(login_password_encode((char *)in, bytes, (char *)out, bytes));
    assert(out[-1] == 0xa5 && memcmp(out, password_vectors[0], bytes) == 0);
    for (short_size = 0; short_size < bytes; ++short_size) {
        memset(out, 0xa5, bytes);
        assert(!login_password_encode((char *)input + page - short_size, short_size, (char *)out, bytes));
        assert(!login_password_encode((char *)in, bytes, (char *)output + page - short_size, short_size));
        for (i = 0; i < bytes; ++i)
            assert(out[i] == 0xa5);
    }
    assert(!login_password_encode(NULL, 32, (char *)output, 32));
    assert(!login_password_encode((char *)input, 32, NULL, 32));
    memset(output, 0xa5, 32);
    assert(!rijndael_encrypt(NULL, 32, input, output));
    assert(!rijndael_encrypt(input, 32, NULL, output));
    assert(!rijndael_encrypt(input, 32, input, NULL));
    for (bytes = 0; bytes < 40; ++bytes)
        if (bytes != 16 && bytes != 24 && bytes != 32)
            assert(!rijndael_encrypt(input + page, bytes, input + page, output));
    for (bytes = 0; bytes < 32; ++bytes)
        assert(output[bytes] == 0xa5);
    assert(VirtualFree(input, 0, MEM_RELEASE) && VirtualFree(output, 0, MEM_RELEASE));
}

int main(void)
{
    check_aes128();
    check_protocol_bytes();
    check_boundaries();
    puts("PASS: 4096 0x0acf password vectors, overlapping buffers and guarded input/output boundaries");
    return 0;
}
