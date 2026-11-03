#include "fileio/file.h"
#include "fileio/resource_path.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct File *file_construct(struct File *self)
{
    memset(self, 0, sizeof *self);
    self->handle = INVALID_HANDLE_VALUE;
    return self;
}

void file_close(struct File *self)
{
    if (self->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(self->handle);
        self->handle = INVALID_HANDLE_VALUE;
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
    size_t length = strlen(name), prefix = resolve ? 5 : 0;
    char *result, *colon, *read, *write;
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

/* The CP949 spelling of a UTF-8 name, to free, or NULL. Resource names are CP949, so a
   name that misses is looked up again in CP949; the name as it is goes first because
   short CP949 names can be valid UTF-8 as well. */
static char *cp949_name(const char *name)
{
    size_t capacity = strlen(name) + 1;
    char *converted = malloc(capacity);
    if (!converted)
        abort();
    if (resource_name_to_cp949(converted, capacity, name))
        return converted;
    free(converted);
    return NULL;
}

BYTE file_make_file_name(const char *name, int flags)
{
    char *key, *converted;
    BYTE found;
    if (!name)
        return 0;
    key = make_file_name(name, 1);
    found = game_file_mgr_is_file_exist(&g_fileMgr, key, (BYTE)flags);
    free(key);
    if (!found && (converted = cp949_name(name)) != NULL) {
        key = make_file_name(converted, 1);
        free(converted);
        found = game_file_mgr_is_file_exist(&g_fileMgr, key, (BYTE)flags);
        free(key);
    }
    return found;
}

BYTE file_open(struct File *self, const char *name, int mode, BYTE resolve, BYTE diskOnly)
{
    char *copy, *converted;
    if (!name || (mode != CFILE_MODE_READ && !(mode & CFILE_MODE_WRITE)))
        return 0;
    copy = make_file_name(name, mode == CFILE_MODE_READ && resolve);
    file_close(self);
    free(self->name);
    self->name = copy;
    self->position = self->size = 0;
    if (mode == CFILE_MODE_READ) {
        self->buffer = game_file_mgr_get_data(&g_fileMgr, copy, &self->size, diskOnly);
        if (!self->buffer && (converted = cp949_name(name)) != NULL) {
            char *retry = make_file_name(converted, resolve);
            free(converted);
            self->buffer = game_file_mgr_get_data(&g_fileMgr, retry, &self->size, diskOnly);
            if (self->buffer) {
                free(self->name);
                self->name = retry;
            } else {
                free(retry);
            }
        }
        if (self->buffer && self->size >= 0)
            return 1;
        file_close(self);
        self->size = 0;
        return 0;
    }
    self->handle = CreateFileA(copy, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                              NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (self->handle != INVALID_HANDLE_VALUE)
        return 1;
    error_msg(copy, 0);
    return 0;
}

BYTE file_read(struct File *self, void *out, int size)
{
    DWORD read = 0;
    if (size < 0 || (!out && size))
        return 0;
    if (self->handle != INVALID_HANDLE_VALUE)
        return !size || (ReadFile(self->handle, out, (DWORD)size, &read, NULL) && read == (DWORD)size);
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
    if (origin != FILE_BEGIN && origin != FILE_CURRENT && origin != FILE_END)
        return 0;
    if (self->handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER to;
        to.QuadPart = offset;
        return SetFilePointerEx(self->handle, to, NULL, (DWORD)origin) != 0;
    }
    if (!self->buffer || self->size < 0)
        return 0;
    position = origin == FILE_BEGIN ? 0 : origin == FILE_CURRENT ? self->position : self->size;
    position += offset;
    if (position < 0 || position > self->size)
        return 0;
    self->position = (int)position;
    return 1;
}

BYTE file_write(struct File *self, const void *data, int size)
{
    DWORD written = 0;
    if (self->handle == INVALID_HANDLE_VALUE || size < 0 || (!data && size))
        return 0;
    return !size || (WriteFile(self->handle, data, (DWORD)size, &written, NULL) && written == (DWORD)size);
}

const BYTE *file_consume(struct File *self, size_t count, size_t size)
{
    const BYTE *bytes;
    if (self->handle != INVALID_HANDLE_VALUE || !self->buffer || !size
            || self->position < 0 || self->position > self->size
            || count > (size_t)(self->size - self->position) / size)
        return NULL;
    bytes = self->buffer + self->position;
    self->position += (int)(count * size);
    return bytes;
}
