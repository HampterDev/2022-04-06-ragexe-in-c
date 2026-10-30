#include "fileio/grf.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

void grf_file_init(struct GrfFile *file)
{
    memset(file, 0, sizeof *file);
}

void grf_file_clear(struct GrfFile *file)
{
    if (file->view) UnmapViewOfFile(file->view);
    if (file->mapping) CloseHandle(file->mapping);
    if (file->file) CloseHandle(file->file);
    free(file->buffer);
    memset(file, 0, sizeof *file);
}

int grf_file_open(struct GrfFile *file, const char *name)
{
    LARGE_INTEGER size;
    SYSTEM_INFO info;
    grf_file_clear(file);
    if (!name) return 0;
    file->file = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file->file == INVALID_HANDLE_VALUE) { file->file = NULL; return 0; }
    if (!GetFileSizeEx(file->file, &size) || size.QuadPart < 0 || size.QuadPart > UINT_MAX) {
        grf_file_clear(file);
        return 0;
    }
    file->file_size = (unsigned)size.QuadPart;
    GetSystemInfo(&info);
    file->granularity = info.dwAllocationGranularity;
    if (file->file_size)
        file->mapping = CreateFileMappingA(file->file, NULL, PAGE_READONLY, 0, 0, NULL);
    file->map_chunk = file->mapping ? 0x20000000u : 0;
    return 1;
}

const BYTE *grf_file_read(struct GrfFile *file, unsigned offset, unsigned size)
{
    static const BYTE empty;
    unsigned start, needed, chunk;
    DWORD transferred = 0;
    LARGE_INTEGER position;
    if (!file->file || offset > file->file_size || size > file->file_size - offset)
        return NULL;
    if (!size) return &empty;
    if (file->view && offset >= file->view_offset
            && offset - file->view_offset <= file->view_size
            && size <= file->view_size - (offset - file->view_offset))
        return file->view + (offset - file->view_offset);
    if (file->mapping) {
        if (file->view) UnmapViewOfFile(file->view);
        file->view = NULL;
        file->view_size = 0;
        start = offset - offset % file->granularity;
        needed = offset - start + size;
        chunk = file->map_chunk;
        while (chunk >= needed) {
            unsigned length = chunk < file->file_size - start ? chunk : file->file_size - start;
            file->view = MapViewOfFile(file->mapping, FILE_MAP_READ, 0, start, length);
            if (file->view) {
                file->map_chunk = chunk;
                file->view_offset = start;
                file->view_size = length;
                return file->view + (offset - start);
            }
            chunk >>= 1;
        }
        CloseHandle(file->mapping);
        file->mapping = NULL;
        file->map_chunk = 0;
    }
    if (size > file->buffer_capacity) {
        BYTE *next = realloc(file->buffer, size);
        if (!next) return NULL;
        file->buffer = next;
        file->buffer_capacity = size;
    }
    position.QuadPart = offset;
    if (!SetFilePointerEx(file->file, position, NULL, FILE_BEGIN)
            || !ReadFile(file->file, file->buffer, size, &transferred, NULL)
            || transferred != size)
        return NULL;
    return file->buffer;
}
