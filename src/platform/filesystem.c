#include "platform/filesystem.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include <wchar.h>

static wchar_t *native_path(const char *path)
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

FILE *platform_file_open(const char *legacy_path, const char *mode)
{
    wchar_t wide_mode[16], *path;
    FILE *file;
    size_t i;
    int error;
    if (!mode || !*mode || strlen(mode) >= sizeof wide_mode / sizeof *wide_mode) {
        errno = EINVAL;
        return NULL;
    }
    for (i = 0; mode[i]; ++i)
        wide_mode[i] = (unsigned char)mode[i];
    wide_mode[i] = 0;
    path = native_path(legacy_path);
    if (!path) return NULL;
    file = _wfopen(path, wide_mode);
    error = errno;
    free(path);
    errno = error;
    return file;
}

int platform_file_remove(const char *legacy_path)
{
    wchar_t *path = native_path(legacy_path);
    int result, error;
    if (!path) return -1;
    result = _wremove(path);
    error = errno;
    free(path);
    errno = error;
    return result;
}

int platform_path_is_directory(const char *legacy_path)
{
    wchar_t *path = native_path(legacy_path);
    struct _stat64 status;
    int result;
    if (!path) return 0;
    result = _wstat64(path, &status) == 0 && (status.st_mode & _S_IFDIR) != 0;
    free(path);
    return result;
}
#else
#include <iconv.h>

static char *native_path(const char *path)
{
    char *result, *input, *output, *at;
    size_t input_size, output_size, converted;
    iconv_t converter;
    int error;
    if (!path || !*path) {
        errno = EINVAL;
        return NULL;
    }
    input_size = strlen(path);
    if (input_size > (SIZE_MAX - 1) / 3) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    converter = iconv_open("UTF-8", "CP949");
    if (converter == (iconv_t)-1) return NULL;
    output_size = input_size * 3;
    result = malloc(output_size + 1);
    if (!result) {
        iconv_close(converter);
        errno = ENOMEM;
        return NULL;
    }
    input = (char *)path;
    output = result;
    converted = iconv(converter, &input, &input_size, &output, &output_size);
    error = errno;
    iconv_close(converter);
    if (converted == (size_t)-1 || converted != 0 || input_size) {
        free(result);
        errno = converted == (size_t)-1 ? error : EILSEQ;
        return NULL;
    }
    *output = 0;
    for (at = result; *at; ++at)
        if (*at == '\\') *at = '/';
    return result;
}

FILE *platform_file_open(const char *legacy_path, const char *mode)
{
    char *path;
    FILE *file;
    int error;
    if (!mode || !*mode) {
        errno = EINVAL;
        return NULL;
    }
    path = native_path(legacy_path);
    if (!path) return NULL;
    file = fopen(path, mode);
    error = errno;
    free(path);
    errno = error;
    return file;
}

int platform_file_remove(const char *legacy_path)
{
    char *path = native_path(legacy_path);
    int result, error;
    if (!path) return -1;
    result = remove(path);
    error = errno;
    free(path);
    errno = error;
    return result;
}

int platform_path_is_directory(const char *legacy_path)
{
    char *path = native_path(legacy_path);
    struct stat status;
    int result;
    if (!path) return 0;
    result = stat(path, &status) == 0 && S_ISDIR(status.st_mode);
    free(path);
    return result;
}
#endif
