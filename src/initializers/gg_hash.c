#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "initializers/gg_hash.h"
#include <stddef.h>
#include "initializers/md5.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void gg_hash_init(struct GGHash *self)
{
    memset(self, 0, sizeof *self);
}

void gg_hash_clear(struct GGHash *self)
{
    free(self->keys);
    gg_hash_init(self);
}

int gg_hash_last_error(struct GGHash *self)
{
    return self->last_error;
}

int gg_hash_is_intact(struct GGHash *self)
{
    unsigned running = 0xffffffff, i;

    if (!self->count) {
        self->last_error = GG_HASH_EMPTY;
        return 0;
    }
    for (i = 0; i < self->count; i++)
        running ^= self->keys[i];
    return self->keys[self->count] == (~self->keys[(self->count - 1) / 2] ^ running);
}

static const unsigned *find_key(const unsigned *table, unsigned count, unsigned key)
{
    while (count) {
        const unsigned *probe = &table[count >> 1];
        if (*probe == key)
            return probe;
        if (key > *probe) {
            table = probe + 1;
            count--;
        }
        count >>= 1;
    }
    return 0;
}

int gg_hash_lookup(struct GGHash *self, const wchar_t *name, unsigned contents_hash)
{
    struct Md5 md5;
    unsigned name_key;
    char ansi[MAX_PATH];
    int match;

    if (!name) {
        self->last_error = GG_HASH_NO_FILE;
        return 0;
    }
    if (!self->count) {
        self->last_error = GG_HASH_EMPTY;
        return 0;
    }
    if (wcslen(name) >= MAX_PATH ||
        !WideCharToMultiByte(CP_ACP, 0, name, -1, ansi, MAX_PATH, NULL, NULL)) {
        self->last_error = GG_HASH_NO_FILE;
        return 0;
    }
    md5_construct(&md5);
    name_key = md5_hash32_wstring_lower(&md5, name);
    if (find_key(self->keys, self->count, name_key ^ contents_hash))
        match = GG_HASH_MATCH;
    else if (find_key(self->keys + self->count + 1, self->count, name_key))
        match = GG_HASH_MISMATCH;
    else
        match = GG_HASH_UNLISTED;
    md5_destruct(&md5);
    return match;
}

int gg_hash_lookup_data(struct GGHash *self, const wchar_t *name, const void *data, unsigned len)
{
    struct Md5 md5;
    int match;

    self->last_error = 0;
    if (!name) {
        self->last_error = GG_HASH_NO_FILE;
        return 0;
    }
    if (!data || !len) {
        self->last_error = GG_HASH_NO_DATA;
        return 0;
    }
    md5_construct(&md5);
    match = gg_hash_lookup(self, name, md5_hash32(&md5, data, len));
    md5_destruct(&md5);
    return match;
}

int gg_hash_lookup_file(struct GGHash *self, const wchar_t *path)
{
    struct Md5 md5;
    unsigned contents_hash;
    int match = 0;

    self->last_error = 0;
    if (!path) {
        self->last_error = GG_HASH_NO_FILE;
        return 0;
    }
    md5_construct(&md5);
    contents_hash = md5_hash32_file(&md5, path);
    if (contents_hash)
        match = gg_hash_lookup(self, path, contents_hash);
    else
        self->last_error = GG_HASH_NO_DATA;
    md5_destruct(&md5);
    return match;
}

int gg_hash_lookup_data_ansi(struct GGHash *self, const char *name, const void *data, unsigned len)
{
    wchar_t wide[MAX_PATH];

    self->last_error = 0;
    if (!name) {
        self->last_error = GG_HASH_NO_FILE;
        return 0;
    }
    if (!data || !len) {
        self->last_error = GG_HASH_NO_DATA;
        return 0;
    }
    if (!MultiByteToWideChar(CP_ACP, 0, name, -1, wide, MAX_PATH)) {
        self->last_error = GG_HASH_NO_FILE;
        return 0;
    }
    return gg_hash_lookup_data(self, wide, data, len);
}

int gg_hash_lookup_file_ansi(struct GGHash *self, const char *path)
{
    wchar_t wide[MAX_PATH];

    self->last_error = 0;
    if (!path) {
        self->last_error = GG_HASH_NO_FILE;
        return 0;
    }
    if (!path || !MultiByteToWideChar(CP_ACP, 0, path, -1, wide, MAX_PATH)) {
        self->last_error = GG_HASH_NO_FILE;
        return 0;
    }
    return gg_hash_lookup_file(self, wide);
}
