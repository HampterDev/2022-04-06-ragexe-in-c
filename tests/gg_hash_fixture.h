#ifndef TEST_GG_HASH_FIXTURE_H
#define TEST_GG_HASH_FIXTURE_H
#include <wchar.h>
#include "initializers/md5.h"

static unsigned hash_words[128], hash_word_count;
static const unsigned hash_counts[] = {1, 2, 3, 16, 31};
static int compare_unsigned(const void *a, const void *b)
{
    unsigned left = *(const unsigned *)a, right = *(const unsigned *)b;
    return (left > right) - (left < right);
}
static void hash_name(wchar_t *name, unsigned index)
{
    if (!index) wcscpy(name, L"data\\first.lua");
    else swprintf(name, MAX_PATH, L"data\\file%u.lua", index);
}
static unsigned hash_content(unsigned index)
{
    struct Md5 md5;
    unsigned value = index * 73 + 19;
    md5_construct(&md5);
    value = md5_hash32(&md5, &value, sizeof value);
    md5_destruct(&md5);
    return value;
}
static void hash_fixture(unsigned seed, unsigned count)
{
    struct Md5 md5;
    wchar_t name[MAX_PATH];
    unsigned names[31], data[31], keys[16], i, value = seed;
    unsigned checksum = ~0u, key_count = seed % 10 + 6;
    assert(count && count <= 31 && key_count > 6);
    md5_construct(&md5);
    for (i = 0; i < count; ++i) {
        hash_name(name, i);
        names[i] = md5_hash32_wstring_lower(&md5, name);
        data[i] = names[i] ^ hash_content(i);
        checksum ^= data[i];
    }
    qsort(names, count, sizeof *names, compare_unsigned);
    qsort(data, count, sizeof *data, compare_unsigned);
    for (i = 0; i < key_count; ++i) value = md5_hash32(&md5, &value, sizeof value);
    for (i = 0; i < key_count; ++i) keys[i] = value = md5_hash32(&md5, &value, sizeof value);
    hash_words[0] = seed;
    hash_words[1] = count ^ keys[4];
    hash_words[2] = checksum ^ keys[6];
    for (i = 0; i < count; ++i) {
        hash_words[3 + i] = data[i] ^ keys[i % key_count];
        hash_words[3 + count + i] = names[i] ^ keys[(i + 4) % key_count];
    }
    hash_word_count = 3 + 2 * count;
    md5_destruct(&md5);
}
#endif
