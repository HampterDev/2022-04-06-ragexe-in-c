#include "fileio/file.h"
#include "platform/filesystem.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct File *file_construct(struct File *self)
{
    memset(self, 0, sizeof *self);
    return self;
}

void file_close(struct File *self)
{
    if (self->handle) {
        platform_file_close(self->handle);
        self->handle = NULL;
    }
    if (self->buffer) {
        game_file_mgr_release_data(&g_fileMgr, self->buffer);
        self->buffer = NULL;
    }
}

void file_destruct(struct File *self)
{
    file_close(self);
    free(self->name);
    self->name = NULL;
}

static char *make_file_name(const char *name, int resolve)
{
    size_t length = strlen(name), prefix;
    char *result, *colon, *read, *write;
    if (platform_path_is_native(name))
        resolve = 0;
    prefix = resolve ? 5 : 0;
    if (length > SIZE_MAX - prefix - 1)
        abort();
    result = malloc(length + prefix + 1);
    if (!result)
        abort();
    if (prefix)
        memcpy(result, "data\\", prefix);
    memcpy(result + prefix, name, length + 1);
    if (!resolve)
        return result;
    colon = strchr(result, ':');
    if (colon && colon != result)
        memmove(result, colon - 1, strlen(colon - 1) + 1);
    for (read = write = result; *read; ++read) {
        if (*read == '/' && write != result && write[-1] == '/')
            continue;
        *write++ = *read;
    }
    *write = 0;
    return result;
}

BYTE file_make_file_name(const char *name, int flags)
{
    char *key;
    BYTE found;
    if (!name)
        return 0;
    key = make_file_name(name, 1);
    found = game_file_mgr_is_file_exist(&g_fileMgr, key, (BYTE)flags);
    free(key);
    return found;
}

BYTE file_open(struct File *self, const char *name, int mode, BYTE resolve, BYTE diskOnly)
{
    char *copy;
    if (!name || (mode != CFILE_MODE_READ && !(mode & CFILE_MODE_WRITE)))
        return 0;
    copy = make_file_name(name, mode == CFILE_MODE_READ && resolve);
    file_close(self);
    free(self->name);
    self->name = copy;
    self->position = self->size = 0;
    if (mode == CFILE_MODE_READ) {
        self->buffer = game_file_mgr_get_data(&g_fileMgr, copy, &self->size, diskOnly);
        if (self->buffer && self->size >= 0)
            return 1;
        file_close(self);
        self->size = 0;
        return 0;
    }
    self->handle = platform_file_create(copy);
    if (self->handle)
        return 1;
    error_msg(copy, 0);
    return 0;
}

BYTE file_read(struct File *self, void *out, int size)
{
    if (size < 0 || (!out && size))
        return 0;
    if (self->handle)
        return !size || platform_file_read(self->handle, out, (size_t)size);
    if (!self->buffer || self->position < 0 || self->position > self->size
            || size > self->size - self->position)
        return 0;
    if (size)
        memcpy(out, self->buffer + self->position, (size_t)size);
    self->position += size;
    return 1;
}

BYTE file_seek(struct File *self, int offset, int origin)
{
    int64_t position;
    if (origin != SEEK_SET && origin != SEEK_CUR && origin != SEEK_END)
        return 0;
    if (self->handle)
        return platform_file_seek(self->handle, offset, origin);
    if (!self->buffer || self->size < 0)
        return 0;
    position = origin == SEEK_SET ? 0 : origin == SEEK_CUR ? self->position : self->size;
    position += offset;
    if (position < 0 || position > self->size)
        return 0;
    self->position = (int)position;
    return 1;
}

BYTE file_write(struct File *self, const void *data, int size)
{
    if (!self->handle || size < 0 || (!data && size))
        return 0;
    return !size || platform_file_write(self->handle, data, (size_t)size);
}

const BYTE *file_consume(struct File *self, size_t count, size_t size)
{
    const BYTE *bytes;
    if (self->handle || !self->buffer || !size
            || self->position < 0 || self->position > self->size
            || count > (size_t)(self->size - self->position) / size)
        return NULL;
    bytes = self->buffer + self->position;
    self->position += (int)(count * size);
    return bytes;
}
