#ifndef FILEIO_GRF_H
#define FILEIO_GRF_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "base.h"

struct Dec {
    BYTE subkey[16][8];
};
SIZE_ASSERT(Dec, 0x80);

struct Dec *dec_construct(struct Dec *self, const BYTE *key);
void dec_destruct(struct Dec *self);
void dec_set_key(struct Dec *self, const BYTE *key);
BYTE dec_decode(struct Dec *self, const BYTE *src, int len, BYTE *dst,
                            int *outLen, int cycle);
BYTE dec_decode_header(struct Dec *self, const BYTE *src, int len, BYTE *dst,
                                  int *outLen);

struct GrfFile {
    HANDLE file, mapping;
    BYTE *view;
    unsigned file_size, view_offset, view_size, map_chunk, granularity;
    BYTE *buffer;
    size_t buffer_capacity;
};
void grf_file_init(struct GrfFile *file);
int grf_file_open(struct GrfFile *file, const char *name);
void grf_file_clear(struct GrfFile *file);
const BYTE *grf_file_read(struct GrfFile *file, unsigned offset, unsigned size);

#include "fileio/hash_types.h"

#define GRF_FLAG_DEFLATED    0x01
#define GRF_FLAG_DES         0x02
#define GRF_FLAG_DES_HEADER  0x04

struct GrfEntry {
    unsigned sizeAligned;
    unsigned sizeCompressed;
    unsigned size;
    unsigned offset;
    BYTE flags;
    struct Hash key;
};

struct GrfArchive {
    struct GrfFile file;
    struct GrfEntry *entries;
    size_t entry_count;
    BYTE *scratch;
    size_t scratch_size;
    unsigned version;
    CRITICAL_SECTION lock;
};
struct GrfArchive *grf_archive_create(const char *name);
void grf_archive_destroy(struct GrfArchive *archive);
int grf_archive_find(const struct GrfArchive *archive, const struct Hash *key, struct GrfEntry *out);
int grf_archive_read(struct GrfArchive *archive, const struct GrfEntry *entry, BYTE *out, size_t capacity);

void hash_set_string(struct Hash *self, const char *name);
struct Hash *hash_construct(struct Hash *self, const char *name);
BYTE hash_less(const struct Hash *self, const struct Hash *other);

#endif
