#ifdef _WIN32
#include "platform/filesystem.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include <direct.h>
#include <io.h>
#include <wchar.h>

static int join_text(char *out, size_t size, const char *directory, const char *name, char separator)
{
    size_t length = strlen(directory);
    int count;
    if (length && (directory[length - 1] == separator || directory[length - 1] == '/'))
        count = snprintf(out, size, "%s%s", directory, name);
    else
        count = snprintf(out, size, "%s%c%s", directory, separator, name);
    if (count >= 0 && (size_t)count < size) return 1;
    if (size) out[0] = 0;
    return 0;
}

struct PlatformFind {
    HANDLE handle;
    WIN32_FIND_DATAA data;
};

static wchar_t *wide_path(const char *path)
{
    wchar_t *result;
    int count, i;
    if (!path || !*path) {
        errno = EINVAL;
        return NULL;
    }
    count = MultiByteToWideChar(949, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (!count) {
        errno = EILSEQ;
        return NULL;
    }
    result = malloc((size_t)count * sizeof *result);
    if (!result) {
        errno = ENOMEM;
        return NULL;
    }
    if (!MultiByteToWideChar(949, MB_ERR_INVALID_CHARS, path, -1, result, count)) {
        free(result);
        errno = EILSEQ;
        return NULL;
    }
    for (i = 0; result[i]; ++i)
        if (result[i] == L'\\') result[i] = L'/';
    return result;
}

static struct PlatformFile *file_from_handle(HANDLE handle)
{
    return handle == INVALID_HANDLE_VALUE ? NULL : (struct PlatformFile *)handle;
}

int platform_path_is_native(const char *path)
{
    if (!path) return 0;
    if (((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':')
        return 1;
    return (path[0] == '\\' || path[0] == '/') && (path[1] == '\\' || path[1] == '/');
}

int platform_path_join(char *out, size_t size, const char *directory, const char *legacy_path)
{
    return join_text(out, size, directory, legacy_path, '\\');
}

size_t platform_path_full(const char *path, char *out, size_t size, char **name)
{
    return GetFullPathNameA(path, (DWORD)size, out, name);
}

int platform_current_directory(char *out, size_t size)
{
    DWORD length = GetCurrentDirectoryA((DWORD)size, out);
    return length && length < size;
}

int platform_module_path(char *out, size_t size)
{
    DWORD length = GetModuleFileNameA(NULL, out, (DWORD)size);
    return length && length < size;
}

int platform_path_exists(const char *path)
{
    return _access(path, 0) == 0;
}

int platform_path_is_directory(const char *path)
{
    wchar_t *wide;
    struct _stat64 status;
    int result;
    if (platform_path_is_native(path)) {
        struct _stat native;
        return _stat(path, &native) == 0 && (native.st_mode & _S_IFDIR) != 0;
    }
    wide = wide_path(path);
    if (!wide) return 0;
    result = _wstat64(wide, &status) == 0 && (status.st_mode & _S_IFDIR) != 0;
    free(wide);
    return result;
}

int platform_directory_create(const char *path)
{
    return _mkdir(path);
}

int platform_directory_create_all(char *directory)
{
    char *p;
    if (strlen(directory) > 3) {
        for (p = directory + 3; *p; ++p) {
            if (*p != '\\') continue;
            *p = 0;
            if (_mkdir(directory) != 0 && errno != EEXIST)
                return 0;
            *p = '\\';
        }
    }
    return _mkdir(directory) == 0 || errno == EEXIST;
}

FILE *platform_file_open(const char *path, const char *mode)
{
    wchar_t wide_mode[16], *wide;
    FILE *file;
    size_t i;
    int error;
    if (!mode || !*mode || strlen(mode) >= sizeof wide_mode / sizeof *wide_mode) {
        errno = EINVAL;
        return NULL;
    }
    if (platform_path_is_native(path))
        return fopen(path, mode);
    for (i = 0; mode[i]; ++i)
        wide_mode[i] = (unsigned char)mode[i];
    wide_mode[i] = 0;
    wide = wide_path(path);
    if (!wide) return NULL;
    file = _wfopen(wide, wide_mode);
    error = errno;
    free(wide);
    errno = error;
    return file;
}

int platform_file_remove(const char *path)
{
    wchar_t *wide;
    int result, error;
    if (platform_path_is_native(path))
        return _unlink(path);
    wide = wide_path(path);
    if (!wide) return -1;
    result = _wremove(wide);
    error = errno;
    free(wide);
    errno = error;
    return result;
}

int platform_file_make_writable(const char *path)
{
    return _chmod(path, _S_IWRITE);
}

int platform_file_copy(const char *from, const char *to)
{
    return CopyFileA(from, to, TRUE) != 0;
}

int platform_file_move(const char *from, const char *to)
{
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING) != 0;
}

struct PlatformFile *platform_file_open_read(const char *path)
{
    HANDLE handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        errno = error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? ENOENT : EIO;
    }
    return file_from_handle(handle);
}

struct PlatformFile *platform_file_create(const char *path)
{
    return file_from_handle(CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
}

struct PlatformFile *platform_file_create_temporary(const char *directory, const char *prefix,
                                                   char *name, size_t size)
{
    HANDLE handle;
    if (size < MAX_PATH || !GetTempFileNameA(directory, prefix, 0, name))
        return NULL;
    handle = CreateFileA(name, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
        DeleteFileA(name);
    return file_from_handle(handle);
}

int platform_file_size(struct PlatformFile *file, int64_t *size)
{
    LARGE_INTEGER length;
    if (!GetFileSizeEx((HANDLE)file, &length)) return 0;
    *size = length.QuadPart;
    return 1;
}

int platform_file_seek(struct PlatformFile *file, int64_t offset, int origin)
{
    LARGE_INTEGER to;
    DWORD method = origin == SEEK_SET ? FILE_BEGIN : origin == SEEK_CUR ? FILE_CURRENT : FILE_END;
    to.QuadPart = offset;
    return SetFilePointerEx((HANDLE)file, to, NULL, method) != 0;
}

int platform_file_read(struct PlatformFile *file, void *data, size_t size)
{
    DWORD done = 0;
    if (size > MAXDWORD) return 0;
    return ReadFile((HANDLE)file, data, (DWORD)size, &done, NULL) && done == size;
}

int platform_file_write(struct PlatformFile *file, const void *data, size_t size)
{
    DWORD done = 0;
    if (size > MAXDWORD) return 0;
    return WriteFile((HANDLE)file, data, (DWORD)size, &done, NULL) && done == size;
}

int platform_file_close(struct PlatformFile *file)
{
    return CloseHandle((HANDLE)file) != 0;
}

int platform_file_replace(struct PlatformFile *file, const char *temporary, const char *target)
{
    int ok = FlushFileBuffers((HANDLE)file) != 0;
    if (!CloseHandle((HANDLE)file)) ok = 0;
    if (ok)
        ok = MoveFileExA(temporary, target, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    if (!ok) DeleteFileA(temporary);
    return ok;
}

void platform_file_discard(struct PlatformFile *file, const char *temporary)
{
    CloseHandle((HANDLE)file);
    DeleteFileA(temporary);
}

struct PlatformFileMapping *platform_file_map(struct PlatformFile *file)
{
    return (struct PlatformFileMapping *)CreateFileMappingA((HANDLE)file, NULL, PAGE_READONLY, 0, 0, NULL);
}

const unsigned char *platform_file_map_view(struct PlatformFileMapping *mapping, uint64_t offset,
                                            size_t size)
{
    return MapViewOfFile((HANDLE)mapping, FILE_MAP_READ, (DWORD)(offset >> 32), (DWORD)offset, size);
}

void platform_file_unmap_view(const unsigned char *view, size_t size)
{
    (void)size;
    UnmapViewOfFile(view);
}

void platform_file_unmap(struct PlatformFileMapping *mapping)
{
    CloseHandle((HANDLE)mapping);
}

unsigned platform_file_map_granularity(void)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwAllocationGranularity;
}

void *platform_file_buffer_alloc(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}

void platform_file_buffer_free(void *data)
{
    VirtualFree(data, 0, MEM_RELEASE);
}

struct PlatformFind *platform_find_first(const char *pattern)
{
    struct PlatformFind *find = malloc(sizeof *find);
    if (!find) return NULL;
    find->handle = FindFirstFileA(pattern, &find->data);
    if (find->handle == INVALID_HANDLE_VALUE) {
        free(find);
        return NULL;
    }
    return find;
}

int platform_find_next(struct PlatformFind *find)
{
    return FindNextFileA(find->handle, &find->data) != 0;
}

const char *platform_find_name(const struct PlatformFind *find)
{
    return find->data.cFileName;
}

int platform_find_is_directory(const struct PlatformFind *find)
{
    return (find->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

void platform_find_close(struct PlatformFind *find)
{
    if (!find) return;
    FindClose(find->handle);
    free(find);
}

int platform_native_list(const char *folder, PlatformListEntry entry, void *context)
{
    char pattern[MAX_PATH];
    WIN32_FIND_DATAA data;
    HANDLE handle;
    if ((size_t)snprintf(pattern, sizeof pattern, "%s\\*", folder) >= sizeof pattern)
        return 0;
    handle = FindFirstFileA(pattern, &data);
    if (handle == INVALID_HANDLE_VALUE)
        return 0;
    do {
        if (strcmp(data.cFileName, ".") && strcmp(data.cFileName, ".."))
            entry(context, data.cFileName, (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
    } while (FindNextFileA(handle, &data));
    FindClose(handle);
    return 1;
}
#endif
