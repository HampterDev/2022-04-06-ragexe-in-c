#include "platform/filesystem.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include <direct.h>
#include <io.h>
#include <wchar.h>

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
#else
#include <dirent.h>
#include <fcntl.h>
#include "platform/iconv_port.h"
#include <pthread.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#define LISTING_SLOTS 32
#define CURRENT_DIRECTORY_MAX 4096

struct PlatformFile {
    int descriptor;
};

struct PlatformFileMapping {
    int descriptor;
};

struct PlatformFind {
    DIR *directory;
    char *path;
    size_t prefix_length;
    const char *pattern;
    char *short_pattern;
    char *name;
    int is_directory;
};

struct Listing {
    char *directory;
    char **names;
    size_t count;
    dev_t device;
    ino_t inode;
    time_t modified;
    int stable;
};

static struct Listing g_listings[LISTING_SLOTS];
static unsigned g_nextListing;
static pthread_mutex_t g_listingLock = PTHREAD_MUTEX_INITIALIZER;

static char *copy_text(const char *text)
{
    size_t size = strlen(text) + 1;
    char *result = malloc(size);
    if (!result) {
        errno = ENOMEM;
        return NULL;
    }
    return memcpy(result, text, size);
}

static char *recode(const char *text, const char *to, const char *from)
{
    char *result, *input, *output;
    size_t input_size, output_size, converted;
    const char *at;
    iconv_t converter;
    int error;
    for (at = text; *at && !((unsigned char)*at & 0x80); ++at)
        ;
    if (!*at) return copy_text(text);
    input_size = strlen(text);
    if (input_size > (SIZE_MAX - 1) / 3) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    converter = iconv_open(to, from);
    if (converter == (iconv_t)-1) return NULL;
    output_size = input_size * 3;
    result = malloc(output_size + 1);
    if (!result) {
        iconv_close(converter);
        errno = ENOMEM;
        return NULL;
    }
    input = (char *)text;
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
    return result;
}

static int fold(int c)
{
    return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

static int fold_compare(const char *a, const char *b)
{
    for (; *a && fold((unsigned char)*a) == fold((unsigned char)*b); ++a, ++b)
        ;
    return fold((unsigned char)*a) - fold((unsigned char)*b);
}

static int compare_names(const void *a, const void *b)
{
    const char *left = *(char *const *)a, *right = *(char *const *)b;
    int result = fold_compare(left, right);
    return result ? result : strcmp(left, right);
}

static void listing_clear(struct Listing *listing)
{
    size_t i;
    for (i = 0; i < listing->count; ++i)
        free(listing->names[i]);
    free(listing->names);
    free(listing->directory);
    memset(listing, 0, sizeof *listing);
}

static int listing_load(struct Listing *listing, const char *directory, const struct stat *status)
{
    DIR *handle = opendir(directory);
    struct dirent *entry = NULL;
    size_t capacity = 0;
    if (!handle) return 0;
    listing->directory = copy_text(directory);
    while (listing->directory && (entry = readdir(handle)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        if (listing->count == capacity) {
            size_t next = capacity ? capacity * 2 : 64;
            char **names = next <= SIZE_MAX / sizeof *names ? realloc(listing->names, next * sizeof *names) : NULL;
            if (!names) break;
            listing->names = names;
            capacity = next;
        }
        listing->names[listing->count] = copy_text(entry->d_name);
        if (!listing->names[listing->count]) break;
        ++listing->count;
    }
    if (!listing->directory || entry) {
        closedir(handle);
        listing_clear(listing);
        return 0;
    }
    closedir(handle);
    if (listing->count) qsort(listing->names, listing->count, sizeof *listing->names, compare_names);
    listing->device = status->st_dev;
    listing->inode = status->st_ino;
    listing->modified = status->st_mtime;
    listing->stable = time(NULL) > status->st_mtime + 1;
    return 1;
}

static int listing_lookup(const char *directory, char *name)
{
    struct stat status;
    struct Listing *listing = NULL;
    size_t i, low, high;
    int found = 0;
    if (stat(directory, &status) != 0 || !S_ISDIR(status.st_mode)) return 0;
    pthread_mutex_lock(&g_listingLock);
    for (i = 0; i < LISTING_SLOTS && !listing; ++i)
        if (g_listings[i].directory && !strcmp(g_listings[i].directory, directory))
            listing = &g_listings[i];
    if (listing && (!listing->stable || listing->device != status.st_dev ||
                    listing->inode != status.st_ino || listing->modified != status.st_mtime))
        listing_clear(listing);
    if (!listing || !listing->directory) {
        if (!listing) {
            listing = &g_listings[g_nextListing];
            g_nextListing = (g_nextListing + 1) % LISTING_SLOTS;
            listing_clear(listing);
        }
        if (!listing_load(listing, directory, &status)) {
            pthread_mutex_unlock(&g_listingLock);
            return 0;
        }
    }
    low = 0;
    high = listing->count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (fold_compare(listing->names[middle], name) < 0) low = middle + 1;
        else high = middle;
    }
    if (low < listing->count && !fold_compare(listing->names[low], name)) {
        memcpy(name, listing->names[low], strlen(name));
        found = 1;
    }
    pthread_mutex_unlock(&g_listingLock);
    return found;
}

static int find_component(char *path, char *component)
{
    char saved;
    int found;
    if (component == path) return listing_lookup(".", component);
    if (component == path + 1) return listing_lookup("/", component);
    saved = component[-1];
    component[-1] = 0;
    found = listing_lookup(path, component);
    component[-1] = saved;
    return found;
}

static void resolve_case(char *path, size_t start)
{
    struct stat status;
    char *component = path + start, *end, saved;
    if (stat(path, &status) == 0 || (errno != ENOENT && errno != ENOTDIR)) return;
    for (;;) {
        while (*component == '/') ++component;
        if (!*component) return;
        end = component + strcspn(component, "/");
        saved = *end;
        *end = 0;
        if (stat(path, &status) != 0 && !find_component(path, component)) {
            *end = saved;
            return;
        }
        *end = saved;
        component = end;
    }
}

static void normalize(char *path)
{
    char *read = path, *write = path;
    size_t size = strlen(path);
    int directory = size && path[size - 1] == '/';
    while (*read) {
        size_t length = strcspn(read, "/");
        char *last = write;
        while (last > path && last[-1] != '/') --last;
        if (length == 2 && read[0] == '.' && read[1] == '.' && write > path &&
            !(write - last == 2 && last[0] == '.' && last[1] == '.')) {
            write = last > path ? last - 1 : path;
        } else if (length && !(length == 1 && read[0] == '.')) {
            if (write > path) *write++ = '/';
            memmove(write, read, length);
            write += length;
        }
        read += length;
        if (*read) ++read;
    }
    if (write == path) *write++ = '.';
    else if (directory) *write++ = '/';
    *write = 0;
}

static char *disk_name(const char *legacy_path)
{
    char *result = recode(legacy_path, "UTF-8", "CP949"), *at;
    if (!result) return NULL;
    for (at = result; *at; ++at)
        if (*at == '\\') *at = '/';
    normalize(result);
    return result;
}

static char *disk_path(const char *path)
{
    char *result;
    if (!path || !*path) {
        errno = EINVAL;
        return NULL;
    }
    if (platform_path_is_native(path)) return copy_text(path);
    result = disk_name(path);
    if (result) resolve_case(result, 0);
    return result;
}

static void free_path(char *path)
{
    int error = errno;
    free(path);
    errno = error;
}

static struct PlatformFile *open_descriptor(const char *path, int flags)
{
    char *disk = disk_path(path);
    struct PlatformFile *file;
    struct stat status;
    int descriptor;
    if (!disk) return NULL;
    descriptor = open(disk, flags | O_CLOEXEC, 0666);
    free_path(disk);
    if (descriptor < 0) {
        if (errno == ENOTDIR) errno = ENOENT;
        return NULL;
    }
    if (fstat(descriptor, &status) != 0 || S_ISDIR(status.st_mode)) {
        close(descriptor);
        errno = EISDIR;
        return NULL;
    }
    file = malloc(sizeof *file);
    if (!file) {
        close(descriptor);
        errno = ENOMEM;
        return NULL;
    }
    file->descriptor = descriptor;
    return file;
}

int platform_path_is_native(const char *path)
{
    return path && path[0] == '/';
}

int platform_path_join(char *out, size_t size, const char *directory, const char *legacy_path)
{
    char *tail;
    int ok;
    if (!platform_path_is_native(directory))
        return join_text(out, size, directory, legacy_path, '\\');
    tail = disk_name(legacy_path);
    if (!tail) {
        if (size) out[0] = 0;
        return 0;
    }
    ok = join_text(out, size, directory, tail, '/');
    free(tail);
    if (ok) resolve_case(out, strlen(directory));
    return ok;
}

size_t platform_path_full(const char *path, char *out, size_t size, char **name)
{
    char directory[CURRENT_DIRECTORY_MAX], *last;
    *name = NULL;
    if (!path || !*path) return 0;
    if (platform_path_is_native(path)) {
        if (strlen(path) >= size) return 0;
        strcpy(out, path);
    } else if (!getcwd(directory, sizeof directory) || !platform_path_join(out, size, directory, path)) {
        return 0;
    }
    last = strrchr(out, '/');
    *name = last && last[1] ? last + 1 : NULL;
    return strlen(out);
}

int platform_current_directory(char *out, size_t size)
{
    if (size && getcwd(out, size)) return 1;
    if (size) out[0] = 0;
    return 0;
}

int platform_module_path(char *out, size_t size)
{
#ifdef __APPLE__
    uint32_t capacity = size > UINT32_MAX ? UINT32_MAX : (uint32_t)size;
    if (size && _NSGetExecutablePath(out, &capacity) == 0) return 1;
#else
    ssize_t length = size ? readlink("/proc/self/exe", out, size - 1) : -1;
    if (length > 0 && (size_t)length < size - 1) {
        out[length] = 0;
        return 1;
    }
#endif
    if (size) out[0] = 0;
    return 0;
}

int platform_path_exists(const char *path)
{
    char *disk = disk_path(path);
    int result;
    if (!disk) return 0;
    result = access(disk, F_OK) == 0;
    free_path(disk);
    return result;
}

int platform_path_is_directory(const char *path)
{
    char *disk = disk_path(path);
    struct stat status;
    int result;
    if (!disk) return 0;
    result = stat(disk, &status) == 0 && S_ISDIR(status.st_mode);
    free_path(disk);
    return result;
}

int platform_directory_create(const char *path)
{
    char *disk = disk_path(path);
    int result;
    if (!disk) return -1;
    result = mkdir(disk, 0777);
    free_path(disk);
    return result;
}

int platform_directory_create_all(char *directory)
{
    char *disk = disk_path(directory), *p;
    int result = 1;
    if (!disk) return 0;
    for (p = disk + 1; *p && result; ++p) {
        if (*p != '/') continue;
        *p = 0;
        result = mkdir(disk, 0777) == 0 || errno == EEXIST;
        *p = '/';
    }
    if (result) result = mkdir(disk, 0777) == 0 || errno == EEXIST;
    free_path(disk);
    return result;
}

FILE *platform_file_open(const char *path, const char *mode)
{
    char *disk;
    FILE *file;
    int error;
    if (!mode || !*mode) {
        errno = EINVAL;
        return NULL;
    }
    disk = disk_path(path);
    if (!disk) return NULL;
    file = fopen(disk, mode);
    error = errno;
    free(disk);
    errno = error;
    return file;
}

int platform_file_remove(const char *path)
{
    char *disk = disk_path(path);
    int result;
    if (!disk) return -1;
    result = remove(disk);
    free_path(disk);
    return result;
}

int platform_file_make_writable(const char *path)
{
    char *disk = disk_path(path);
    struct stat status;
    int result;
    if (!disk) return -1;
    result = stat(disk, &status) == 0 ? chmod(disk, status.st_mode | S_IWUSR) : -1;
    free_path(disk);
    return result;
}

int platform_file_copy(const char *from, const char *to)
{
    struct PlatformFile *source = open_descriptor(from, O_RDONLY), *target;
    char buffer[0x4000];
    ssize_t count;
    int ok = 1, error;
    if (!source) return 0;
    target = open_descriptor(to, O_WRONLY | O_CREAT | O_EXCL);
    if (!target) {
        platform_file_close(source);
        return 0;
    }
    while (ok && (count = read(source->descriptor, buffer, sizeof buffer)) != 0) {
        if (count < 0) {
            ok = errno == EINTR;
            continue;
        }
        ok = platform_file_write(target, buffer, (size_t)count);
    }
    error = errno;
    platform_file_close(source);
    if (!platform_file_close(target)) ok = 0;
    if (!ok) {
        platform_file_remove(to);
        errno = error;
    }
    return ok;
}

int platform_file_move(const char *from, const char *to)
{
    char *source = disk_path(from), *target = disk_path(to);
    int result = source && target && rename(source, target) == 0;
    free_path(source);
    free_path(target);
    return result;
}

struct PlatformFile *platform_file_open_read(const char *path)
{
    return open_descriptor(path, O_RDONLY);
}

struct PlatformFile *platform_file_create(const char *path)
{
    return open_descriptor(path, O_RDWR | O_CREAT | O_TRUNC);
}

struct PlatformFile *platform_file_create_temporary(const char *directory, const char *prefix,
                                                   char *name, size_t size)
{
    char *disk = disk_path(directory), *pattern, *base;
    struct PlatformFile *file;
    size_t length;
    int descriptor;
    if (!disk) return NULL;
    length = strlen(disk) + strlen(prefix) + 8;
    pattern = malloc(length);
    if (pattern) snprintf(pattern, length, "%s/%sXXXXXX", disk, prefix);
    free(disk);
    if (!pattern) {
        errno = ENOMEM;
        return NULL;
    }
    descriptor = mkstemp(pattern);
    if (descriptor < 0) {
        free_path(pattern);
        return NULL;
    }
    base = strrchr(pattern, '/') + 1;
    file = malloc(sizeof *file);
    if (!file || !join_text(name, size, directory, base, platform_path_is_native(directory) ? '/' : '\\')) {
        free(file);
        unlink(pattern);
        close(descriptor);
        free(pattern);
        errno = ENAMETOOLONG;
        return NULL;
    }
    free(pattern);
    file->descriptor = descriptor;
    return file;
}

int platform_file_size(struct PlatformFile *file, int64_t *size)
{
    struct stat status;
    if (fstat(file->descriptor, &status) != 0) return 0;
    *size = (int64_t)status.st_size;
    return 1;
}

int platform_file_seek(struct PlatformFile *file, int64_t offset, int origin)
{
    return lseek(file->descriptor, (off_t)offset, origin) != (off_t)-1;
}

int platform_file_read(struct PlatformFile *file, void *data, size_t size)
{
    size_t done = 0;
    while (done < size) {
        ssize_t count = read(file->descriptor, (char *)data + done, size - done);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return 0;
        done += (size_t)count;
    }
    return 1;
}

int platform_file_write(struct PlatformFile *file, const void *data, size_t size)
{
    size_t done = 0;
    while (done < size) {
        ssize_t count = write(file->descriptor, (const char *)data + done, size - done);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return 0;
        done += (size_t)count;
    }
    return 1;
}

int platform_file_close(struct PlatformFile *file)
{
    int result = close(file->descriptor);
    free(file);
    return result == 0;
}

int platform_file_replace(struct PlatformFile *file, const char *temporary, const char *target)
{
    char *source = disk_path(temporary), *destination = disk_path(target), *slash;
    int ok = source && destination && fsync(file->descriptor) == 0, directory;
    if (!platform_file_close(file)) ok = 0;
    if (ok) ok = rename(source, destination) == 0;
    if (ok && (slash = strrchr(destination, '/')) != NULL) {
        *slash = 0;
        directory = open(slash == destination ? "/" : destination, O_RDONLY | O_CLOEXEC);
        if (directory >= 0) {
            fsync(directory);
            close(directory);
        }
    }
    if (!ok && source) unlink(source);
    free(source);
    free(destination);
    return ok;
}

void platform_file_discard(struct PlatformFile *file, const char *temporary)
{
    platform_file_close(file);
    platform_file_remove(temporary);
}

struct PlatformFileMapping *platform_file_map(struct PlatformFile *file)
{
    struct PlatformFileMapping *mapping = malloc(sizeof *mapping);
    if (mapping) mapping->descriptor = file->descriptor;
    return mapping;
}

const unsigned char *platform_file_map_view(struct PlatformFileMapping *mapping, uint64_t offset,
                                            size_t size)
{
    void *view;
    if (!size || offset > (uint64_t)INT64_MAX) return NULL;
    view = mmap(NULL, size, PROT_READ, MAP_SHARED, mapping->descriptor, (off_t)offset);
    return view == MAP_FAILED ? NULL : view;
}

void platform_file_unmap_view(const unsigned char *view, size_t size)
{
    munmap((void *)view, size);
}

void platform_file_unmap(struct PlatformFileMapping *mapping)
{
    free(mapping);
}

unsigned platform_file_map_granularity(void)
{
    long size = sysconf(_SC_PAGESIZE);
    return size > 0 ? (unsigned)size : 4096u;
}

void *platform_file_buffer_alloc(size_t size)
{
    return calloc(1, size);
}

void platform_file_buffer_free(void *data)
{
    free(data);
}

static const char *next_character(const char *text)
{
    ++text;
    while (((unsigned char)*text & 0xc0) == 0x80) ++text;
    return text;
}

static int wildcard(const char *name, const char *pattern)
{
    for (; *pattern; ++pattern) {
        if (*pattern == '*') {
            while (pattern[1] == '*') ++pattern;
            if (!pattern[1]) return 1;
            for (; *name; name = next_character(name))
                if (wildcard(name, pattern + 1)) return 1;
            return 0;
        }
        if (!*name) return 0;
        if (*pattern == '?') {
            name = next_character(name);
            continue;
        }
        if (fold((unsigned char)*name) != fold((unsigned char)*pattern)) return 0;
        ++name;
    }
    return !*name;
}

static int find_advance(struct PlatformFind *find)
{
    struct dirent *entry;
    struct stat status;
    while ((entry = readdir(find->directory)) != NULL) {
        char *name, *full;
        size_t length;
        if (!wildcard(entry->d_name, find->pattern) &&
            !(find->short_pattern && !strchr(entry->d_name, '.') && wildcard(entry->d_name, find->short_pattern)))
            continue;
        name = recode(entry->d_name, "CP949", "UTF-8");
        if (!name) continue;
        length = find->prefix_length + strlen(entry->d_name) + 1;
        full = malloc(length);
        if (!full) {
            free(name);
            return 0;
        }
        snprintf(full, length, "%.*s%s", (int)find->prefix_length, find->path, entry->d_name);
        find->is_directory = stat(full, &status) == 0 && S_ISDIR(status.st_mode);
        free(full);
        free(find->name);
        find->name = name;
        return 1;
    }
    return 0;
}

/* The spelling a directory stores for name: on a case-insensitive volume the requested
   spelling already exists, but a Win32 find reports the stored one. */
static const char *stored_spelling(const char *directory, const char *name, char *out, size_t size)
{
    DIR *listing = opendir(directory);
    struct dirent *entry;
    const char *result = name;
    if (!listing)
        return name;
    while ((entry = readdir(listing)) != NULL) {
        if (strcmp(entry->d_name, name) == 0) {
            result = name;
            break;
        }
        if (fold_compare(entry->d_name, name) == 0 && strlen(entry->d_name) < size) {
            strcpy(out, entry->d_name);
            result = out;
        }
    }
    closedir(listing);
    return result;
}

struct PlatformFind *platform_find_first(const char *pattern)
{
    struct PlatformFind *find;
    struct stat status;
    char *slash;
    size_t length;
    find = calloc(1, sizeof *find);
    if (!find) return NULL;
    find->path = disk_path(pattern);
    if (!find->path) {
        free(find);
        return NULL;
    }
    slash = strrchr(find->path, '/');
    find->pattern = slash ? slash + 1 : find->path;
    find->prefix_length = (size_t)(find->pattern - find->path);
    if (!*find->pattern) {
        platform_find_close(find);
        errno = ENOENT;
        return NULL;
    }
    if (!strpbrk(find->pattern, "*?")) {
        char stored[256];
        const char *name;
        if (stat(find->path, &status) == 0) {
            if (slash == find->path) {
                name = stored_spelling("/", find->pattern, stored, sizeof stored);
            } else if (slash) {
                *slash = 0;
                name = stored_spelling(find->path, find->pattern, stored, sizeof stored);
                *slash = '/';
            } else {
                name = stored_spelling(".", find->pattern, stored, sizeof stored);
            }
            find->name = recode(name, "CP949", "UTF-8");
        }
        if (!find->name) {
            platform_find_close(find);
            return NULL;
        }
        find->is_directory = S_ISDIR(status.st_mode);
        return find;
    }
    length = strlen(find->pattern);
    if (length >= 2 && !strcmp(find->pattern + length - 2, ".*")) {
        find->short_pattern = copy_text(find->pattern);
        if (find->short_pattern) find->short_pattern[length - 2] = 0;
    }
    if (slash == find->path) {
        find->directory = opendir("/");
    } else if (slash) {
        *slash = 0;
        find->directory = opendir(find->path);
        *slash = '/';
    } else {
        find->directory = opendir(".");
    }
    if (!find->directory || !find_advance(find)) {
        platform_find_close(find);
        errno = ENOENT;
        return NULL;
    }
    return find;
}

int platform_find_next(struct PlatformFind *find)
{
    return find->directory && find_advance(find);
}

const char *platform_find_name(const struct PlatformFind *find)
{
    return find->name;
}

int platform_find_is_directory(const struct PlatformFind *find)
{
    return find->is_directory;
}

void platform_find_close(struct PlatformFind *find)
{
    if (!find) return;
    if (find->directory) closedir(find->directory);
    free(find->name);
    free(find->short_pattern);
    free(find->path);
    free(find);
}

int platform_native_list(const char *folder, PlatformListEntry entry, void *context)
{
    DIR *dir = opendir(folder);
    struct dirent *item;
    if (!dir)
        return 0;
    while ((item = readdir(dir))) {
        char path[4096];
        struct stat info;
        if (!strcmp(item->d_name, ".") || !strcmp(item->d_name, ".."))
            continue;
        if ((size_t)snprintf(path, sizeof path, "%s/%s", folder, item->d_name) >= sizeof path || stat(path, &info))
            continue;
        if (S_ISDIR(info.st_mode) || S_ISREG(info.st_mode))
            entry(context, item->d_name, S_ISDIR(info.st_mode));
    }
    closedir(dir);
    return 1;
}
#endif
