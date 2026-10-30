#ifndef MODEL_MESH_TEST_SUPPORT_H
#define MODEL_MESH_TEST_SUPPORT_H

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileio/file.h"
#include "render/anim_mesh.h"
#include "resources/model_mesh.h"
#include "owned_storage_test_allocator.h"

static void *checked_calloc(size_t count, size_t size)
{
    void *p;
    assert(!size || count <= SIZE_MAX / size);
    p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}

#define malloc checked_malloc
#define calloc checked_calloc
#define free checked_free
#include "../src/render/anim_mesh.c"
#include "../src/resources/model_mesh.c"
#undef malloc
#undef calloc
#undef free

#ifndef TEST_RESOURCE_LOAD
void *mem_alloc(size_t bytes) { (void)bytes; abort(); }
void mem_free(void *memory)
{ (void)memory; abort(); }
#endif

BYTE file_read(struct File *file, void *out, int bytes)
{
    assert(bytes >= 0 && (out || !bytes));
    if (bytes > file->size - file->position) return 0;
    if (bytes) memcpy(out, file->buffer + file->position, (size_t)bytes);
    file->position += bytes;
    return 1;
}

static unsigned char input[8192];
static int used, face_offset, count_offset;

static void put(const void *data, int bytes)
{
    assert(bytes <= (int)sizeof input - used);
    memcpy(input + used, data, (size_t)bytes);
    used += bytes;
}

static void integer(int value) { put(&value, sizeof value); }

static struct File stream(int size)
{
    struct File file = {0};
    file.buffer = input;
    file.size = size;
    return file;
}

static void fixture(int major, int minor, int textures, int groups)
{
    const Vec3 positions[] = {{0,0,0}, {1,0,0}, {0,1,0}, {0,0,1}};
    const struct AnimMeshTexVertex uv[] = {{0x12345678, .25f, .5f}};
    const uint16_t indices[3][3] = {{0,1,2}, {0,3,1}, {0,2,3}};
    int i, g;
    used = 0;
    integer(4); put(positions, sizeof positions);
    integer(textures);
    if (textures) {
        if (major == 1 && minor == 1) put(&uv[0].u, 8);
        else put(uv, sizeof uv);
    }
    count_offset = used;
    integer(3);
    face_offset = used;
    for (i = 0; i < 3; ++i) {
        uint16_t tv[3] = {0,0,0}, texture = (uint16_t)(2-i), reserved = 37;
        int n = (major == 2 && minor >= 2) ? groups : 1;
        if (major == 2 && minor >= 2) integer(20 + 4*n + 1);
        put(indices[i], sizeof indices[i]); put(tv, sizeof tv);
        put(&texture, 2); put(&reserved, 2); integer(i & 1);
        if (!(major == 1 && minor == 1))
            for (g = 0; g < n; ++g) integer(g ? 9 : 7);
        if (major == 2 && minor >= 2) put("x", 1);
    }
}

#endif
