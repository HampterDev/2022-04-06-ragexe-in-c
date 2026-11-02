#include "fileio/file.h"
#include "platform/filesystem.h"
#include <stdint.h>
#include <stdlib.h>

#include <string.h>
#include <limits.h>

struct GameFileMgr g_fileMgr;
BYTE g_readFolderFirst;

void game_file_manager_clear(struct GameFileMgr *self)
{
    size_t i;
    for (i = self->archive_count; i-- > 0;) grf_archive_destroy(self->archives[i]);
    free(self->archives);
    self->archives = NULL;
    self->archive_count = 0;
}

int game_file_manager_add_archive(struct GameFileMgr *self, const char *name)
{
    struct GrfArchive *archive;
    struct GrfArchive **next;
    if (self->archive_count >= SIZE_MAX / sizeof *next) return 0;
    archive = grf_archive_create(name);
    if (!archive) return 0;
    next = realloc(self->archives, (self->archive_count + 1) * sizeof *next);
    if (!next) { grf_archive_destroy(archive); return 0; }
    self->archives = next;
    self->archives[self->archive_count++] = archive;
    return 1;
}

BYTE *game_file_mgr_read_loose_file(struct GameFileMgr *self, const char *name, int *size)
{
    struct PlatformFile *file;
    int64_t length;
    BYTE *data = NULL;
    (void)self;
    if (!size)
        return NULL;
    *size = 0;
    if (!name)
        return NULL;
    file = platform_file_open_read(name);
    if (!file)
        return NULL;
    if (platform_file_size(file, &length) && length >= 0 && length <= INT_MAX) {
        size_t count = (size_t)length;
        data = platform_file_buffer_alloc(count ? count : 1);
        if (data && count && !platform_file_read(file, data, count)) {
            platform_file_buffer_free(data);
            data = NULL;
        }
        if (data)
            *size = (int)count;
    }
    platform_file_close(file);
    return data;
}

BYTE *game_file_mgr_read_from_archives(struct GameFileMgr *self, const char *name,
                                               int *size)
{
    size_t i;
    if (!size)
        return NULL;
    *size = 0;
    if (!name || strlen(name) >= sizeof ((struct Hash *)0)->name)
        return NULL;

    for (i = self->archive_count; i-- > 0;) {
        struct GrfArchive *pak = self->archives[i];
        struct GrfEntry entry;
        struct Hash key;

        hash_construct(&entry.key, "");
        hash_construct(&key, name);
        if (grf_archive_find(pak, &key, &entry)) {
            BYTE *data;

            if (entry.size > INT_MAX)
                continue;
            data = platform_file_buffer_alloc((size_t)entry.size + 8);
            if (data && grf_archive_read(pak, &entry, data, entry.size)) {
                *size = (int)entry.size;

                return data;
            }
            platform_file_buffer_free(data);
        }
    }
    return NULL;
}

BYTE *game_file_mgr_get_data(struct GameFileMgr *self, const char *name,
                                      int *size, BYTE diskOnly)
{
    BYTE *data;
    if (!size)
        return NULL;
    *size = 0;
    if (!name)
        return NULL;

    if (diskOnly)
        return game_file_mgr_read_loose_file(self, name, size);
    if (g_readFolderFirst) {
        data = game_file_mgr_read_loose_file(self, name, size);
        if (!data)
            data = game_file_mgr_read_from_archives(self, name, size);
    } else {
        data = game_file_mgr_read_from_archives(self, name, size);
        if (!data)
            data = game_file_mgr_read_loose_file(self, name, size);
    }
    return data;
}

static BYTE exists_on_disk(const char *name)
{
    struct PlatformFind *find = platform_find_first(name);

    if (!find)
        return 0;
    platform_find_close(find);
    return 1;
}

BYTE game_file_mgr_is_file_exist(struct GameFileMgr *self, const char *name,
                                         BYTE diskOnly)
{
    if (!name)
        return 0;
    if (!diskOnly && strlen(name) < sizeof ((struct Hash *)0)->name
            && self->archive_count) {
        size_t i;
        struct Hash key;

        for (i = self->archive_count; i-- > 0;) {
            hash_construct(&key, name);
            if (grf_archive_find(self->archives[i], &key, NULL))
                return 1;
        }
    }
    return exists_on_disk(name);
}

void game_file_mgr_release_data(struct GameFileMgr *self, void *data)
{
    (void)self;

    platform_file_buffer_free(data);
}
