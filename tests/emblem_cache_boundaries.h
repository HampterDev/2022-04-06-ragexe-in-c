#include <stdio.h>
#include <io.h>
#include <direct.h>
int emblem_test_access(const char *path, int mode);
FILE *emblem_test_open(const char *path, const char *mode);
size_t emblem_test_write(const void *data, size_t size, size_t count, FILE *file);
int emblem_test_close(FILE *file);
#define _access emblem_test_access
#define fopen emblem_test_open
#define fwrite emblem_test_write
#define fclose emblem_test_close
