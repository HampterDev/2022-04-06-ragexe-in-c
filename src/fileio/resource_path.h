#ifndef FILEIO_RESOURCE_PATH_H
#define FILEIO_RESOURCE_PATH_H

#include <stddef.h>

int resource_normalize_name(char *out, size_t capacity, const char *name);
int resource_build_path(char *out, size_t capacity, const char *name, const char *directory);

#endif
