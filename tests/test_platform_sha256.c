#include "platform/sha256.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void expect(const void *const *parts, const size_t *sizes, size_t count, const char *hex)
{
    uint8_t digest[PLATFORM_SHA256_SIZE];
    char text[PLATFORM_SHA256_SIZE * 2 + 1];
    size_t i;

    memset(digest, 0, sizeof digest);
    assert(platform_sha256(parts, sizes, count, digest));
    for (i = 0; i < sizeof digest; ++i)
        sprintf(text + i * 2, "%02x", digest[i]);
    assert(!strcmp(text, hex));
}

static void expect_text(const char *message, const char *hex)
{
    const void *parts[1] = {message};
    size_t sizes[1] = {strlen(message)};
    expect(parts, sizes, 1, hex);
}

static void boundaries(void)
{
    static char bytes[130];
    size_t length, split;

    memset(bytes, 'x', sizeof bytes);
    for (length = 0; length <= sizeof bytes; ++length) {
        uint8_t whole[PLATFORM_SHA256_SIZE], pieces[PLATFORM_SHA256_SIZE];
        const void *one[1] = {bytes};
        size_t one_size[1] = {length};
        assert(platform_sha256(one, one_size, 1, whole));
        for (split = 0; split <= length; split += 7) {
            const void *two[3] = {bytes, bytes + split, bytes};
            size_t two_sizes[3] = {split, length - split, 0};
            assert(platform_sha256(two, two_sizes, 3, pieces));
            assert(!memcmp(whole, pieces, sizeof whole));
        }
    }
}

int main(void)
{
    static char block[1000];
    const void *million[1000];
    size_t million_sizes[1000], i;
    const void *fields[3] = {"one.example", "6900", "World"};
    const size_t field_sizes[3] = {12, 5, 6};
    const void *joined[1] = {"one.example\0" "6900\0" "World"};
    const size_t joined_size[1] = {23};

    expect(NULL, NULL, 0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    expect_text("abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    expect_text("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    expect_text("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrs"
                "mnopqrstnopqrstu",
                "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1");
    memset(block, 'a', sizeof block);
    for (i = 0; i < 1000; ++i) {
        million[i] = block;
        million_sizes[i] = sizeof block;
    }
    expect(million, million_sizes, 1000, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    {
        uint8_t a[PLATFORM_SHA256_SIZE], b[PLATFORM_SHA256_SIZE];
        assert(platform_sha256(fields, field_sizes, 3, a));
        assert(platform_sha256(joined, joined_size, 1, b));
        assert(!memcmp(a, b, sizeof a));
    }
    boundaries();
    puts("Platform SHA-256: FIPS 180 vectors, a million bytes in parts, block boundaries and split input passed");
    return 0;
}
