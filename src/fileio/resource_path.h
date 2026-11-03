#ifndef FILEIO_RESOURCE_PATH_H
#define FILEIO_RESOURCE_PATH_H

#include <stddef.h>

int resource_normalize_name(char *out, size_t capacity, const char *name);
int resource_build_path(char *out, size_t capacity, const char *name, const char *directory);

/* The CP949 spelling of a resource name given in UTF-8: resource names are CP949 in the
   GRF and on disk, the client's text is UTF-8. Returns 1 with the name in out when name
   has non-ASCII characters, is valid UTF-8 and CP949 holds every character; otherwise 0
   with out empty. The CP949 spelling is never longer than the UTF-8 one. */
int resource_name_to_cp949(char *out, size_t capacity, const char *name);

#endif
