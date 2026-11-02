#include "fileio/grf.h"
#include "platform/filesystem.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

void grf_file_init(struct GrfFile *file)
{
    memset(file, 0, sizeof *file);
}

void grf_file_clear(struct GrfFile *file)
{
    if (file->view) platform_file_unmap_view(file->view, file->view_size);
    if (file->mapping) platform_file_unmap(file->mapping);
    if (file->file) platform_file_close(file->file);
    free(file->buffer);
    memset(file, 0, sizeof *file);
}

int grf_file_open(struct GrfFile *file, const char *name)
{
    int64_t size;
    grf_file_clear(file);
    if (!name) return 0;
    file->file = platform_file_open_read(name);
    if (!file->file) return 0;
    if (!platform_file_size(file->file, &size) || size < 0 || size > UINT_MAX) {
        grf_file_clear(file);
        return 0;
    }
    file->file_size = (unsigned)size;
    file->granularity = platform_file_map_granularity();
    if (file->file_size)
        file->mapping = platform_file_map(file->file);
    file->map_chunk = file->mapping ? 0x20000000u : 0;
    return 1;
}

const BYTE *grf_file_read(struct GrfFile *file, unsigned offset, unsigned size)
{
    static const BYTE empty;
    unsigned start, needed, chunk;
    if (!file->file || offset > file->file_size || size > file->file_size - offset)
        return NULL;
    if (!size) return &empty;
    if (file->view && offset >= file->view_offset
            && offset - file->view_offset <= file->view_size
            && size <= file->view_size - (offset - file->view_offset))
        return file->view + (offset - file->view_offset);
    if (file->mapping) {
        if (file->view) platform_file_unmap_view(file->view, file->view_size);
        file->view = NULL;
        file->view_size = 0;
        start = offset - offset % file->granularity;
        needed = offset - start + size;
        chunk = file->map_chunk;
        while (chunk >= needed) {
            unsigned length = chunk < file->file_size - start ? chunk : file->file_size - start;
            file->view = platform_file_map_view(file->mapping, start, length);
            if (file->view) {
                file->map_chunk = chunk;
                file->view_offset = start;
                file->view_size = length;
                return file->view + (offset - start);
            }
            chunk >>= 1;
        }
        platform_file_unmap(file->mapping);
        file->mapping = NULL;
        file->map_chunk = 0;
    }
    if (size > file->buffer_capacity) {
        BYTE *next = realloc(file->buffer, size);
        if (!next) return NULL;
        file->buffer = next;
        file->buffer_capacity = size;
    }
    if (!platform_file_seek(file->file, offset, SEEK_SET)
            || !platform_file_read(file->file, file->buffer, size))
        return NULL;
    return file->buffer;
}
