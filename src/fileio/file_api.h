#ifndef FILEIO_FILE_API_H
#define FILEIO_FILE_API_H

#include <stddef.h>

struct File {
    void *handle;
    unsigned char *buffer;
    int position;
    int size;
    char *name;
};

struct File *file_construct(struct File *self);
unsigned char file_open(struct File *self, const char *name, int mode,
                        unsigned char resolve, unsigned char diskOnly);
unsigned char file_read(struct File *self, void *out, int size);
unsigned char file_write(struct File *self, const void *data, int size);
unsigned char file_seek(struct File *self, int offset, int origin);
const unsigned char *file_consume(struct File *self, size_t count, size_t size);
void file_close(struct File *self);
void file_destruct(struct File *self);
unsigned char file_make_file_name(const char *name, int diskOnly);

#endif
