#include "platform/filesystem.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define remove_directory _rmdir
#else
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#define remove_directory rmdir
#endif

static const char *const scratch_files[] = {
    "port-dir\\Alpha.TXT", "port-dir\\beta.txt", "port-dir\\gamma", "port-dir\\copy.txt",
    "port-dir\\Sub\\Deep.bin", "port-dir\\replaced.txt", "port-dir\\\307\321\261\333.txt",
};
static const char *const scratch_directories[] = {
    "port-dir\\a\\b\\c", "port-dir\\a\\b", "port-dir\\a", "port-dir\\Sub", "port-dir",
};

static void clean_scratch(void)
{
    size_t i;
    for (i = 0; i < sizeof scratch_files / sizeof *scratch_files; ++i)
        platform_file_remove(scratch_files[i]);
    for (i = 0; i < sizeof scratch_directories / sizeof *scratch_directories; ++i) {
        char path[256];
        size_t j;
        strcpy(path, scratch_directories[i]);
        for (j = 0; path[j]; ++j)
            if (path[j] == '\\') path[j] = '/';
        remove_directory(path);
    }
}

static void write_file(const char *path, const char *text)
{
    struct PlatformFile *file = platform_file_create(path);
    assert(file);
    assert(platform_file_write(file, text, strlen(text)));
    assert(platform_file_close(file));
}

static void expect_content(const char *path, const char *text)
{
    struct PlatformFile *file = platform_file_open_read(path);
    char buffer[64] = {0};
    int64_t size = -1;
    assert(file);
    assert(platform_file_size(file, &size) && size == (int64_t)strlen(text));
    assert(platform_file_read(file, buffer, (size_t)size) && !strcmp(buffer, text));
    assert(!platform_file_read(file, buffer, 1));
    assert(platform_file_close(file));
}

static void legacy_streams(void)
{
    const char *name = ".\\port-\307\321\261\333.tmp";
    FILE *file;
    char content[16] = {0};
    assert(platform_path_is_directory("."));
    assert(!platform_file_open(NULL, "rb") && errno == EINVAL);
    assert(!platform_file_open("", "rb") && errno == EINVAL);
    assert(!platform_file_open("\xff", "rb"));
    assert(!platform_file_open(name, NULL) && errno == EINVAL);
    file = platform_file_open(name, "wbx");
    assert(file);
    assert(fwrite("original", 1, 8, file) == 8);
    assert(!fclose(file));
    assert(!platform_path_is_directory(name));
    assert(!platform_file_open(name, "wbx") && errno == EEXIST);
    file = platform_file_open("./port-\307\321\261\333.tmp", "rb");
    assert(file && fread(content, 1, sizeof content, file) == 8);
    assert(!memcmp(content, "original", 8));
    assert(!fclose(file));
#ifndef _WIN32
    file = fopen("./port-\355\225\234\352\270\200.tmp", "rb");
    assert(file);
    assert(!fclose(file));
#endif
    assert(!platform_file_remove(name));
    assert(!platform_file_open(name, "rb") && errno == ENOENT);
}

static void directories_and_handles(void)
{
    struct PlatformFile *file;
    int64_t size = -1;
    char bytes[4] = {0};
    assert(platform_directory_create("port-dir") == 0);
    assert(platform_directory_create("port-dir") != 0 && errno == EEXIST);
    assert(platform_path_exists("port-dir") && platform_path_is_directory("port-dir"));
    assert(platform_directory_create("port-dir\\Sub") == 0);
    write_file("port-dir\\Alpha.TXT", "alpha");
    write_file("port-dir\\beta.txt", "beta");
    write_file("port-dir\\Sub\\Deep.bin", "deep");
    assert(!platform_file_open_read("port-dir\\missing.txt") && errno == ENOENT);
    assert(!platform_file_open_read("port-dir\\missing\\file.txt") && errno == ENOENT);
    assert(!platform_file_open_read("port-dir\\Sub"));
    file = platform_file_create("port-dir\\gamma");
    assert(file && platform_file_write(file, "0123456789", 10));
    assert(platform_file_seek(file, -4, SEEK_END) && platform_file_read(file, bytes, 4));
    assert(!memcmp(bytes, "6789", 4));
    assert(platform_file_seek(file, 2, SEEK_SET) && platform_file_seek(file, 1, SEEK_CUR));
    assert(platform_file_read(file, bytes, 2) && !memcmp(bytes, "34", 2));
    assert(platform_file_size(file, &size) && size == 10);
    assert(platform_file_close(file));
}

static void case_and_lexical_lookup(void)
{
    char directory[512], joined[600], full[600], *name = NULL;
    FILE *file;
    expect_content("PORT-DIR\\alpha.txt", "alpha");
    expect_content("Port-Dir/SUB/deep.BIN", "deep");
    expect_content("port-dir\\missing\\..\\.\\Alpha.TXT", "alpha");
    assert(platform_path_exists("PORT-DIR\\SUB"));
    assert(platform_path_is_directory("port-DIR\\sub"));
    file = platform_file_open("port-dir\\ALPHA.txt", "rb");
    assert(file && !fclose(file));
    assert(platform_current_directory(directory, sizeof directory));
    assert(platform_path_is_native(directory) && !platform_path_is_native("port-dir"));
    assert(platform_path_join(joined, sizeof joined, directory, "PORT-DIR\\alpha.txt"));
    assert(!strncmp(joined, directory, strlen(directory)));
    assert(!platform_path_join(joined, 8, directory, "PORT-DIR\\alpha.txt") && !joined[0]);
    assert(platform_path_join(joined, sizeof joined, "port-dir", "alpha.txt") &&
           !strcmp(joined, "port-dir\\alpha.txt"));
    assert(platform_path_join(joined, sizeof joined, "port-dir\\", "alpha.txt") &&
           !strcmp(joined, "port-dir\\alpha.txt"));
    assert(platform_path_join(joined, sizeof joined, directory, "PORT-DIR\\alpha.txt"));
    expect_content(joined, "alpha");
    assert(platform_path_full("port-dir\\Alpha.TXT", full, sizeof full, &name));
    assert(name && !strcmp(name, "Alpha.TXT") && platform_path_is_native(full));
    assert(platform_module_path(full, sizeof full) && platform_path_is_native(full));
#ifndef _WIN32
    {
        /* A case-insensitive volume (the macOS default) finds the requested spelling directly. */
        struct stat status;
        if (stat("PORT-DIR/ALPHA.TXT", &status) == 0)
            assert(!strcasecmp(joined + strlen(directory), "/port-dir/Alpha.TXT"));
        else
            assert(!strcmp(joined + strlen(directory), "/port-dir/Alpha.TXT"));
    }
    assert(!platform_path_join(joined, sizeof joined, directory, "\xff"));
#else
    assert(!strcmp(joined + strlen(directory), "\\PORT-DIR\\alpha.txt"));
#endif
}

static int listed(const char *pattern, const char *wanted, int *is_directory)
{
    struct PlatformFind *find = platform_find_first(pattern);
    int found = 0;
    if (!find) return 0;
    do {
        if (!strcmp(platform_find_name(find), wanted)) {
            ++found;
            *is_directory = platform_find_is_directory(find);
        }
    } while (platform_find_next(find));
    platform_find_close(find);
    return found;
}

static void listing(void)
{
    int is_directory = -1;
    assert(listed("port-dir\\*.txt", "Alpha.TXT", &is_directory) == 1 && !is_directory);
    assert(listed("PORT-DIR\\*.TXT", "beta.txt", &is_directory) == 1);
    assert(!listed("port-dir\\*.txt", "gamma", &is_directory));
    assert(listed("port-dir\\*.*", "gamma", &is_directory) == 1 && !is_directory);
    assert(listed("port-dir\\*.*", "Sub", &is_directory) == 1 && is_directory);
    assert(listed("port-dir\\?eta.*", "beta.txt", &is_directory) == 1);
    assert(listed("port-dir\\*", ".", &is_directory) == 1 && is_directory);
    assert(listed("port-dir\\sub", "Sub", &is_directory) == 1 && is_directory);
    assert(!platform_find_first("port-dir\\*.none"));
    assert(!platform_find_first("port-dir\\absent"));
    platform_find_close(NULL);
#ifndef _WIN32
    {
        FILE *file;
        write_file("port-dir\\\307\321\261\333.txt", "hangul");
        file = fopen("port-dir/\355\225\234\352\270\200.txt", "rb");
        assert(file && !fclose(file));
        assert(listed("port-dir\\*.txt", "\307\321\261\333.txt", &is_directory) == 1);
    }
#endif
}

static void copies_moves_and_temporaries(void)
{
    char directory[512], native[600], temporary[600], target[600];
    struct PlatformFile *file;
    assert(platform_file_copy("port-dir\\alpha.txt", "port-dir\\copy.txt"));
    expect_content("port-dir\\copy.txt", "alpha");
    assert(!platform_file_copy("port-dir\\beta.txt", "port-dir\\copy.txt"));
    expect_content("port-dir\\copy.txt", "alpha");
    assert(!platform_file_copy("port-dir\\absent.txt", "port-dir\\other.txt"));
    assert(!platform_path_exists("port-dir\\other.txt"));
    assert(platform_file_move("port-dir\\beta.txt", "port-dir\\copy.txt"));
    assert(!platform_path_exists("port-dir\\beta.txt"));
    expect_content("port-dir\\copy.txt", "beta");
    assert(!platform_file_move("port-dir\\absent.txt", "port-dir\\copy.txt"));
    assert(platform_current_directory(directory, sizeof directory));
    assert(platform_path_join(native, sizeof native, directory, "port-dir"));
    file = platform_file_create_temporary(native, "tst", temporary, sizeof temporary);
    assert(file && platform_path_is_native(temporary) && platform_path_exists(temporary));
    assert(platform_file_write(file, "first", 5));
    assert(platform_path_join(target, sizeof target, native, "replaced.txt"));
    assert(platform_file_replace(file, temporary, target));
    assert(!platform_path_exists(temporary));
    expect_content(target, "first");
    file = platform_file_create_temporary(native, "tst", temporary, sizeof temporary);
    assert(file && platform_file_write(file, "second", 6));
    assert(platform_file_replace(file, temporary, target));
    expect_content(target, "second");
    file = platform_file_create_temporary(native, "tst", temporary, sizeof temporary);
    assert(file);
    platform_file_discard(file, temporary);
    assert(!platform_path_exists(temporary));
    expect_content(target, "second");
    assert(!platform_file_create_temporary("port-dir\\absent", "tst", temporary, sizeof temporary));
    assert(platform_path_join(native, sizeof native, directory, "port-dir\\a\\b\\c"));
    assert(platform_directory_create_all(native));
    assert(platform_directory_create_all(native));
    assert(platform_path_is_directory("port-dir\\A\\B\\C"));
    assert(platform_file_make_writable("port-dir\\copy.txt") == 0);
    assert(platform_file_remove("port-dir\\copy.txt") == 0);
    assert(!platform_path_exists("port-dir\\copy.txt"));
}

static void mappings_and_buffers(void)
{
    struct PlatformFile *file = platform_file_open_read("port-dir\\gamma");
    struct PlatformFileMapping *mapping;
    const unsigned char *view;
    unsigned char *buffer;
    size_t i;
    assert(file);
    assert(platform_file_map_granularity() >= 4096);
    mapping = platform_file_map(file);
    assert(mapping);
    view = platform_file_map_view(mapping, 0, 10);
    assert(view && !memcmp(view, "0123456789", 10));
    platform_file_unmap_view(view, 10);
    platform_file_unmap(mapping);
    assert(platform_file_close(file));
    buffer = platform_file_buffer_alloc(4096);
    assert(buffer);
    for (i = 0; i < 4096; ++i)
        assert(!buffer[i]);
    platform_file_buffer_free(buffer);
}

int main(void)
{
    clean_scratch();
    legacy_streams();
    directories_and_handles();
    case_and_lexical_lookup();
    listing();
    copies_moves_and_temporaries();
    mappings_and_buffers();
    clean_scratch();
    assert(!platform_path_exists("port-dir"));
    puts("Filesystem: CP949 names, separators, case lookup, listings, handles, temporaries and mappings passed");
    return 0;
}
