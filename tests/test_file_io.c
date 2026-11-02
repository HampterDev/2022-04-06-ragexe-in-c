#include "fileio/file.h"
#include "platform/filesystem.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned errors, archive_queries, archive_reads;
static int archive_found, archive_read_ok = 1;
static unsigned archive_size = 3;
static struct GrfArchive archive;
static struct GrfArchive *archives[] = {&archive};

void error_msg(const char *text, int flags) { assert(text && !flags); ++errors; }

struct Hash *hash_construct(struct Hash *key, const char *name)
{
    assert(strlen(name) < sizeof key->name);
    strcpy(key->name, name);
    _strlwr(key->name);
    key->hash = 0;
    return key;
}
int grf_archive_find(const struct GrfArchive *pak, const struct Hash *key, struct GrfEntry *out)
{
    assert(pak == &archive && key);
    ++archive_queries;
    if (archive_found && out) {
        memset(out, 0, sizeof *out);
        out->size = archive_size;
    }
    return (BYTE)archive_found;
}
int grf_archive_read(struct GrfArchive *pak, const struct GrfEntry *entry, BYTE *out, size_t capacity)
{
    assert(pak == &archive && out && entry->size <= 3 && capacity >= entry->size);
    ++archive_reads;
    if (archive_read_ok)
        memcpy(out, "pak", entry->size);
    return (BYTE)archive_read_ok;
}

struct GrfArchive *grf_archive_create(const char *name) { (void)name; abort(); }
void grf_archive_destroy(struct GrfArchive *pak) { (void)pak; abort(); }

static void bounded_spans(void)
{
    struct File file;
    unsigned char data[6] = {1, 2, 3, 4, 5, 6};
    file_construct(&file);
    assert(!file_consume(&file, 0, 1));
    file.buffer = data;
    file.size = sizeof data;
    assert(file_consume(&file, 2, 2) == data && file.position == 4);
    assert(!file_consume(&file, 2, 2) && file.position == 4);
    assert(!file_consume(&file, (size_t)-1, 2) && file.position == 4);
    assert(!file_consume(&file, 2, (size_t)-1) && file.position == 4);
    assert(!file_consume(&file, 0, 0) && file.position == 4);
    assert(file_consume(&file, 1, 2) == data + 4 && file.position == 6);
    assert(file_consume(&file, 0, (size_t)-1) == data + 6);
    assert(!file_consume(&file, 1, 1) && file.position == 6);
    file.position = INT_MAX - 1;
    file.size = INT_MAX;
    assert(!file_consume(&file, 2, 1) && file.position == INT_MAX - 1);
    file.position = -1;
    assert(!file_consume(&file, 0, 1) && file.position == -1);
    file.position = 0;
    file.size = -1;
    assert(!file_consume(&file, 0, 1));
    file.size = sizeof data;
    file.handle = (struct PlatformFile *)&file;
    assert(!file_consume(&file, 1, 1));
    file.buffer = NULL;
    file.handle = NULL;
    file_destruct(&file);
}

static void memory_file(const char *path)
{
    struct File file;
    char bytes[8] = {0};
    char *name;
    file_construct(&file);
    assert(!file.name && !file.buffer && !file.size && !file.position);
    assert(!file_read(&file, bytes, 1));
    assert(file_open(&file, path, CFILE_MODE_READ, 1, 1));
    assert(file.size == 6 && file.buffer && !file.position);
    assert(strcmp(file.name, path) == 0);
    name = file.name;
    assert(!file_open(&file, NULL, CFILE_MODE_READ, 0, 1));
    assert(!file_open(&file, path, 2, 0, 1));
    assert(file.name == name && file.size == 6);
    assert(!file_read(&file, bytes, -1));
    assert(!file_read(&file, NULL, 1));
    assert(file_read(&file, NULL, 0));
    assert(file_read(&file, bytes, 3) && memcmp(bytes, "abc", 3) == 0);
    assert(!file_read(&file, bytes, 4) && file.position == 3);
    assert(!file_write(&file, bytes, 1));
    assert(!file_seek(&file, INT_MAX, SEEK_CUR) && file.position == 3);
    assert(!file_seek(&file, -1, SEEK_SET) && file.position == 3);
    assert(!file_seek(&file, 0, 99) && file.position == 3);
    assert(file_seek(&file, -2, SEEK_END) && file.position == 4);
    assert(file_read(&file, bytes, 2) && memcmp(bytes, "ef", 2) == 0);
    assert(!file_read(&file, bytes, 1) && file.position == 6);
    file.position = INT_MAX - 1;
    file.size = INT_MAX;
    assert(!file_read(&file, bytes, 4) && file.position == INT_MAX - 1);
    file.position = INT_MIN;
    assert(!file_read(&file, bytes, 1));
    file.position = 0;
    file.size = -1;
    assert(!file_read(&file, bytes, 1));
    assert(!file_seek(&file, 0, SEEK_SET));
    file.size = 6;
    assert(file_open(&file, file.name, CFILE_MODE_READ, 0, 1));
    assert(file.buffer && file.size == 6 && file.position == 0);
    file_close(&file);
    assert(!file.buffer && file.size == 6 && strcmp(file.name, path) == 0);
    assert(!file_read(&file, bytes, 1));
    file_destruct(&file);
    file_destruct(&file);
    assert(!file.name);
}

static void disk_file(const char *path)
{
    struct File file;
    char bytes[8] = {0};
    file_construct(&file);
    assert(file_open(&file, path, CFILE_MODE_WRITE, 0, 1));
    assert(!file_write(&file, "x", -1));
    assert(!file_write(&file, NULL, 1));
    assert(file_write(&file, NULL, 0));
    assert(file_write(&file, "abcdef", 6));
    assert(file_seek(&file, -2, SEEK_END));
    assert(!file_read(&file, bytes, 4));
    assert(memcmp(bytes, "ef", 2) == 0);
    assert(!file_seek(&file, -1, SEEK_SET));
    assert(file_seek(&file, 0, SEEK_SET));
    assert(file_read(&file, bytes, 6) && memcmp(bytes, "abcdef", 6) == 0);
    file_close(&file);
    assert(!file_write(&file, bytes, 1));
    file.handle = platform_file_open_read(path);
    assert(file.handle);
    memset(bytes, 0, sizeof bytes);
    assert(!file_read(&file, bytes, 8) && memcmp(bytes, "abcdef", 6) == 0);
    file_destruct(&file);
    assert(!file.handle && !file_seek(&file, 0, SEEK_SET));
}

static void empty_and_missing(const char *empty, const char *missing)
{
    struct File file;
    char invalid[512];
    file_construct(&file);
    assert(file_open(&file, empty, CFILE_MODE_READ, 0, 1));
    assert(file.buffer && file.size == 0);
    assert(file_read(&file, NULL, 0));
    assert(!file_read(&file, invalid, 1));
    assert(file_make_file_name(empty, 1));
    assert(!file_open(&file, missing, CFILE_MODE_READ, 0, 1));
    assert(!file.buffer && file.size == 0 && file.position == 0);
    sprintf(invalid, "%s\\child", missing);
    assert(!file_open(&file, invalid, CFILE_MODE_WRITE, 0, 1));
    assert(errors == 1 && !file.size);
    assert(!file_open(&file, "missing///nested//file", CFILE_MODE_READ, 1, 1));
    assert(strcmp(file.name, "data\\missing/nested/file") == 0);
    assert(!file_open(&file, ":unlikely", CFILE_MODE_READ, 1, 1));
    assert(strcmp(file.name, "\\:unlikely") == 0);
    file_destruct(&file);
}

static void archive_priority(const char *path, const char *empty, const char *missing)
{
    struct File file;
    unsigned queries, reads;
    char long_name[300];
    g_fileMgr.archives = archives;
    g_fileMgr.archive_count = 1;
    archive_found = 1;
    file_construct(&file);
    g_readFolderFirst = 0;
    assert(file_open(&file, path, CFILE_MODE_READ, 0, 0));
    assert(file.size == 3 && memcmp(file.buffer, "pak", 3) == 0);
    queries = archive_queries;
    g_readFolderFirst = 1;
    assert(file_open(&file, path, CFILE_MODE_READ, 0, 0));
    assert(file.size == 6 && archive_queries == queries);
    assert(file_open(&file, empty, CFILE_MODE_READ, 0, 0));
    assert(file.size == 0 && archive_queries == queries);
    g_readFolderFirst = 0;
    archive_read_ok = 0;
    assert(file_open(&file, path, CFILE_MODE_READ, 0, 0) && file.size == 6);
    archive_read_ok = 1;
    archive_size = (unsigned)INT_MAX + 1;
    reads = archive_reads;
    assert(file_open(&file, path, CFILE_MODE_READ, 0, 0) && file.size == 6);
    assert(archive_reads == reads);
    archive_size = 0;
    assert(file_open(&file, path, CFILE_MODE_READ, 0, 0) && file.size == 0);
    queries = archive_queries;
    memset(long_name, 'x', sizeof long_name - 1);
    long_name[sizeof long_name - 1] = 0;
    assert(!file_open(&file, long_name, CFILE_MODE_READ, 0, 0));
    assert(!game_file_mgr_is_file_exist(&g_fileMgr, long_name, 0));
    assert(archive_queries == queries);
    archive_found = 0;
    assert(!file_open(&file, missing, CFILE_MODE_READ, 0, 0));
    assert(file.size == 0);
    file_destruct(&file);
    g_fileMgr.archives = NULL;
    g_fileMgr.archive_count = 0;
}

int main(void)
{
    bounded_spans();
    char directory[MAX_PATH], path[MAX_PATH], empty[MAX_PATH], missing[MAX_PATH + 16];
    assert(GetTempPathA(sizeof directory, directory));
    assert(GetTempFileNameA(directory, "hcf", 0, path));
    assert(GetTempFileNameA(directory, "hcf", 0, empty));
    sprintf(missing, "%s.absent", empty);
    assert(GetFileAttributesA(missing) == INVALID_FILE_ATTRIBUTES);
    g_fileMgr.archives = NULL;
    g_fileMgr.archive_count = 0;
    disk_file(path);
    memory_file(path);
    empty_and_missing(empty, missing);
    archive_priority(path, empty, missing);
    assert(DeleteFileA(path));
    assert(DeleteFileA(empty));
    puts("file I/O: C ownership, reopen, exact transfers, signed seeks, bounds, empty files and archive fallback passed");
    return 0;
}
