#ifndef INITIALIZERS_GG_HASH_H
#define INITIALIZERS_GG_HASH_H

#include <stddef.h>

enum GGHashError {
    GG_HASH_OK            = 0,
    GG_HASH_NO_FILE       = 1,
    GG_HASH_NO_DATA       = 2,
    GG_HASH_EMPTY         = 3,
    GG_HASH_BAD_CHECKSUM  = 4,
    GG_HASH_BAD_SIGNATURE = 5,
};

enum GGHashMatch {
    GG_HASH_MISMATCH = 0,
    GG_HASH_MATCH    = 1,
    GG_HASH_UNLISTED = 2,
};

struct GGHash {
    int last_error;
    unsigned count;
    unsigned *keys;
};

void gg_hash_init(struct GGHash *self);
void gg_hash_clear(struct GGHash *self);

int gg_hash_is_intact(struct GGHash *self);
int gg_hash_last_error(struct GGHash *self);

int gg_hash_lookup(struct GGHash *self, const wchar_t *name, unsigned contents_hash);
int gg_hash_lookup_data(struct GGHash *self, const wchar_t *name, const void *data, unsigned len);
int gg_hash_lookup_data_ansi(struct GGHash *self, const char *name, const void *data, unsigned len);
int gg_hash_lookup_file(struct GGHash *self, const wchar_t *path);
int gg_hash_lookup_file_ansi(struct GGHash *self, const char *path);

#endif
