#include <zlib.h>
#include "fileio/grf.h"
#include "std/std_sort.h"
#include <stdint.h>
#include <limits.h>

#include <stdlib.h>
#include <string.h>
#include "core/cstr.h"

static unsigned name_hash(const char *name)
{
    unsigned hash = 0x1505;
    size_t i, len = strlen(name);

    for (i = 0; i < len; i++)
        hash += (hash << 5) + (signed char)name[i];
    return hash;
}

void hash_set_string(struct Hash *self, const char *name)
{
    strcpy(self->name, name);
    cstr_lower(self->name);
    self->hash = name_hash(self->name);
}

struct Hash *hash_construct(struct Hash *self, const char *name)
{
    hash_set_string(self, name);
    return self;
}

BYTE hash_less(const struct Hash *self, const struct Hash *other)
{
    if (self->hash != other->hash)
        return self->hash < other->hash;
    return strcmp(self->name, other->name) < 0;
}

BYTE hash_not_equal(const struct Hash *self, const struct Hash *other)
{
    return self->hash != other->hash || strcmp(self->name, other->name) != 0;
}

static BYTE entry_less(const void *a, const void *b, void *context)
{
    (void)context;
    return hash_less(&((const struct GrfEntry *)a)->key, &((const struct GrfEntry *)b)->key);
}

static unsigned read32(const BYTE *p)
{
    unsigned value;
    memcpy(&value, p, sizeof value);
    return value;
}

static BYTE nibble_swap(BYTE b) { return (BYTE)((b << 4) | (b >> 4)); }

static int is_header_only_ext(const char *name)
{
    static const char extensions[4][5] = {".gnd", ".gat", ".act", ".str"};
    const char *ext = strrchr(name, '.');
    unsigned i;
    if (!ext) return 0;
    for (i = 0; i < 4; ++i) if (!cstr_compare_nocase(ext, extensions[i])) return 1;
    return 0;
}

static void grf_archive_clear(struct GrfArchive *archive)
{
    free(archive->entries);
    free(archive->scratch);
    grf_file_clear(&archive->file);
    archive->entries = NULL;
    archive->scratch = NULL;
    archive->entry_count = archive->scratch_size = 0;
    archive->version = 0;
}

static void grf_archive_finish(struct GrfArchive *archive)
{
    grf_archive_clear(archive);
    platform_mutex_destroy(archive->lock);
}

void grf_archive_destroy(struct GrfArchive *archive)
{
    if (!archive) return;
    grf_archive_finish(archive);
    free(archive);
}

static int allocate_entries(struct GrfArchive *archive, unsigned count, size_t bytes, size_t minimum)
{
    if (count > bytes / minimum || count > SIZE_MAX / sizeof *archive->entries)
        return 0;
    if (count) {
        archive->entries = calloc(count, sizeof *archive->entries);
        if (!archive->entries) return 0;
    }
    archive->entry_count = count;
    return 1;
}

static int entry_valid(const struct GrfArchive *archive, const struct GrfEntry *entry)
{
    if (archive->file.file_size < 0x2e || entry->offset > archive->file.file_size - 0x2e
            || entry->sizeAligned > archive->file.file_size - 0x2e - entry->offset
            || entry->sizeCompressed > entry->sizeAligned)
        return 0;
    if (!(entry->flags & GRF_FLAG_DEFLATED) && entry->size > entry->sizeAligned)
        return 0;
    if ((entry->flags & (GRF_FLAG_DES | GRF_FLAG_DES_HEADER))
            && (entry->sizeAligned > INT_MAX || entry->sizeAligned % 8))
        return 0;
    return 1;
}

static int finish_table(struct GrfArchive *archive)
{
    size_t i, kept = 0, largest = 0;
    for (i = 0; i < archive->entry_count; ++i) {
        const struct GrfEntry *entry = archive->entries + i;
        if (!entry_valid(archive, entry)) continue;
        if ((entry->flags & (GRF_FLAG_DES | GRF_FLAG_DES_HEADER)) && entry->sizeAligned > largest)
            largest = entry->sizeAligned;
        archive->entries[kept++] = *entry;
    }
    archive->entry_count = kept;
    if (largest) {
        archive->scratch = malloc(largest);
        if (!archive->scratch) return 0;
    }
    archive->scratch_size = largest;
    if (archive->entry_count > 1)
        std_sort(archive->entries, archive->entries + archive->entry_count,
                 sizeof *archive->entries, entry_less, NULL);
    return 1;
}

static int load_table_v1(struct GrfArchive *archive, unsigned offset, unsigned count)
{
    unsigned bytes = archive->file.file_size - offset;
    const BYTE *cursor = grf_file_read(&archive->file, offset, bytes);
    size_t left = bytes, i;
    if (!cursor || archive->version > 0x103 || !allocate_entries(archive, count, left, 22))
        return 0;
    for (i = 0; i < count; ++i) {
        char name[256];
        unsigned length, j;
        const BYTE *record;
        struct GrfEntry *entry = archive->entries + i;
        if (left < 4) return 0;
        length = read32(cursor); cursor += 4; left -= 4;
        if (length > left || left - length < 17) return 0;
        if (archive->version < 0x101) {
            const BYTE *end = memchr(cursor, 0, length);
            if (!end || (size_t)(end - cursor) >= sizeof entry->key.name) return 0;
            for (j = 0; j <= (unsigned)(end - cursor); ++j) name[j] = (char)nibble_swap(cursor[j]);
        } else {
            char plain[256];
            struct Dec cipher;
            BYTE key[8] = {0};
            int decoded;
            unsigned encoded;
            if (length < 14) return 0;
            encoded = length - 6;
            if (encoded > sizeof name || encoded % 8) return 0;
            for (j = 0; j < encoded; ++j) name[j] = (char)nibble_swap(cursor[j + 2]);
            dec_construct(&cipher, key);
            dec_decode(&cipher, (const BYTE *)name, (int)encoded, (BYTE *)plain, &decoded, 1);
            dec_destruct(&cipher);
            if (!memchr(plain, 0, encoded) || strlen(plain) >= sizeof entry->key.name) return 0;
            strcpy(name, plain);
        }
        record = cursor + length;
        entry->size = read32(record + 8);
        if (entry->size > UINT_MAX - 0x2cb || read32(record) < entry->size + 0x2cb
                || read32(record + 4) < 0x92cb)
            return 0;
        entry->sizeCompressed = read32(record) - entry->size - 0x2cb;
        entry->sizeAligned = read32(record + 4) - 0x92cb;
        entry->flags = record[12] | (is_header_only_ext(name) ? GRF_FLAG_DES_HEADER : GRF_FLAG_DES);
        entry->offset = read32(record + 13);
        hash_set_string(&entry->key, name);
        cursor = record + 17; left -= length + 17;
    }
    return finish_table(archive);
}

static int load_table_v2(struct GrfArchive *archive, unsigned offset, unsigned count)
{
    const BYTE *header = grf_file_read(&archive->file, offset, 8), *packed_data;
    BYTE *table;
    const BYTE *cursor;
    unsigned packed, unpacked;
    unsigned long length;
    size_t left, i;
    int ok = 0;
    if (!header) return 0;
    packed = read32(header); unpacked = read32(header + 4);
    if (unpacked > SIZE_MAX - 8 || count > ((size_t)unpacked + 8) / 18) return 0;
    packed_data = grf_file_read(&archive->file, offset + 8, packed);
    if (!packed_data) return 0;
    length = unpacked + 8;
    table = malloc(length);
    if (!table) return 0;
    if (uncompress(table, &length, packed_data, packed) != Z_OK
            || !allocate_entries(archive, count, length, 18))
        goto done;
    cursor = table; left = length;
    for (i = 0; i < count; ++i) {
        const BYTE *end = memchr(cursor, 0, left), *record;
        size_t name_length;
        struct GrfEntry *entry = archive->entries + i;
        if (!end) goto done;
        name_length = end - cursor;
        if (name_length >= sizeof entry->key.name || left - name_length - 1 < 17) goto done;
        record = end + 1;
        entry->sizeCompressed = read32(record);
        entry->sizeAligned = read32(record + 4);
        entry->size = read32(record + 8);
        entry->flags = record[12];
        entry->offset = read32(record + 13);
        hash_set_string(&entry->key, (const char *)cursor);
        cursor = record + 17; left -= name_length + 18;
    }
    ok = finish_table(archive);
done:
    free(table);
    return ok;
}

static int grf_archive_load(struct GrfArchive *archive)
{
    const BYTE *header = grf_file_read(&archive->file, 0, 0x2e);
    unsigned encoded_count, seed, offset, count;
    if (!header || memcmp(header, "Master of Magic", 16)) return 0;
    archive->version = read32(header + 0x2a);
    encoded_count = read32(header + 0x26); seed = read32(header + 0x22);
    offset = read32(header + 0x1e);
    if (seed > UINT_MAX - 7 || encoded_count < seed + 7
            || offset > archive->file.file_size - 0x2e)
        return 0;
    count = encoded_count - seed - 7; offset += 0x2e;
    switch (archive->version & 0xff00) {
    case 0x100: return load_table_v1(archive, offset, count);
    case 0x200: return load_table_v2(archive, offset, count);
    default: return 0;
    }
}

struct GrfArchive *grf_archive_create(const char *name)
{
    struct GrfArchive *archive = calloc(1, sizeof *archive);
    if (!archive) return NULL;
    grf_file_init(&archive->file);
    archive->lock = platform_mutex_create();
    if (!grf_file_open(&archive->file, name) || !grf_archive_load(archive)) {
        grf_archive_destroy(archive);
        return NULL;
    }
    return archive;
}

int grf_archive_find(const struct GrfArchive *archive, const struct Hash *key, struct GrfEntry *out)
{
    size_t first = 0, count = archive->entry_count;
    while (count) {
        size_t half = count / 2, middle = first + half;
        if (hash_less(&archive->entries[middle].key, key)) { first = middle + 1; count -= half + 1; }
        else count = half;
    }
    if (first == archive->entry_count || hash_not_equal(&archive->entries[first].key, key)) return 0;
    if (out) *out = archive->entries[first];
    return 1;
}

int grf_archive_read(struct GrfArchive *archive, const struct GrfEntry *entry, BYTE *out, size_t capacity)
{
    const BYTE *stored, *input;
    unsigned input_size = entry->sizeCompressed;
    int ok = 0;
    if ((!out && entry->size) || capacity < entry->size || !entry_valid(archive, entry)) return 0;
    platform_mutex_lock(archive->lock);
    stored = grf_file_read(&archive->file, entry->offset + 0x2e, entry->sizeAligned);
    if (!stored) goto done;
    input = stored;
    if ((entry->flags & (GRF_FLAG_DES | GRF_FLAG_DES_HEADER)) && entry->sizeAligned) {
        struct Dec cipher;
        BYTE key[8] = {0};
        int decoded;
        if (entry->sizeAligned > archive->scratch_size) goto done;
        dec_construct(&cipher, key);
        if (entry->flags & GRF_FLAG_DES) {
            unsigned n = entry->sizeCompressed;
            int digits = 0;
            do { ++digits; n /= 10; } while (n);
            dec_decode(&cipher, stored, (int)entry->sizeAligned, archive->scratch, &decoded, digits);
        } else dec_decode_header(&cipher, stored, (int)entry->sizeAligned, archive->scratch, &decoded);
        dec_destruct(&cipher);
        input = archive->scratch; input_size = (unsigned)decoded;
    }
    if (entry->flags & GRF_FLAG_DEFLATED) {
        unsigned long length = entry->size;
        uncompress(out, &length, input, input_size);
        if (length < entry->size)
            memset(out + length, 0, entry->size - length);
        ok = 1;
    } else {
        if (entry->size) memcpy(out, stored, entry->size);
        ok = 1;
    }
done:
    platform_mutex_unlock(archive->lock);
    return ok;
}
