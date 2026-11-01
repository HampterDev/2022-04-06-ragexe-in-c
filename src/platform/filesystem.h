#ifndef PLATFORM_FILESYSTEM_H
#define PLATFORM_FILESYSTEM_H

#include <stdio.h>

FILE *platform_file_open(const char *legacy_path, const char *mode);
int platform_file_remove(const char *legacy_path);
int platform_path_is_directory(const char *legacy_path);

#endif
