#ifndef PLATFORM_FILESYSTEM_H
#define PLATFORM_FILESYSTEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct PlatformFile;
struct PlatformFileMapping;
struct PlatformFind;

int platform_path_is_native(const char *path);
int platform_path_join(char *out, size_t size, const char *directory, const char *legacy_path);
size_t platform_path_full(const char *path, char *out, size_t size, char **name);
int platform_current_directory(char *out, size_t size);
int platform_module_path(char *out, size_t size);
int platform_path_exists(const char *path);
int platform_path_is_directory(const char *path);
int platform_directory_create(const char *path);
int platform_directory_create_all(char *directory);

FILE *platform_file_open(const char *path, const char *mode);
int platform_file_remove(const char *path);
int platform_file_make_writable(const char *path);
int platform_file_copy(const char *from, const char *to);
int platform_file_move(const char *from, const char *to);

struct PlatformFile *platform_file_open_read(const char *path);
struct PlatformFile *platform_file_create(const char *path);
struct PlatformFile *platform_file_create_temporary(const char *directory, const char *prefix,
                                                   char *name, size_t size);
int platform_file_size(struct PlatformFile *file, int64_t *size);
int platform_file_seek(struct PlatformFile *file, int64_t offset, int origin);
int platform_file_read(struct PlatformFile *file, void *data, size_t size);
int platform_file_write(struct PlatformFile *file, const void *data, size_t size);
int platform_file_close(struct PlatformFile *file);
int platform_file_replace(struct PlatformFile *file, const char *temporary, const char *target);
void platform_file_discard(struct PlatformFile *file, const char *temporary);

struct PlatformFileMapping *platform_file_map(struct PlatformFile *file);
const unsigned char *platform_file_map_view(struct PlatformFileMapping *mapping, uint64_t offset,
                                            size_t size);
void platform_file_unmap_view(const unsigned char *view, size_t size);
void platform_file_unmap(struct PlatformFileMapping *mapping);
unsigned platform_file_map_granularity(void);

void *platform_file_buffer_alloc(size_t size);
void platform_file_buffer_free(void *data);

struct PlatformFind *platform_find_first(const char *pattern);
int platform_find_next(struct PlatformFind *find);
const char *platform_find_name(const struct PlatformFind *find);
int platform_find_is_directory(const struct PlatformFind *find);
void platform_find_close(struct PlatformFind *find);

/* The files and folders in a folder named as the system names it (UTF-8 off Windows), not as the
   client does: for files outside the game, such as the system's fonts. "." and ".." are left out. */
typedef void (*PlatformListEntry)(void *context, const char *name, int is_directory);
int platform_native_list(const char *folder, PlatformListEntry entry, void *context);

#endif
