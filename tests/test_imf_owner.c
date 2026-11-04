#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "fileio/file.h"
#include "fileio/file_api.h"
#include "actor/layer_order.h"
#include "owned_storage_test_allocator.h"

static void *checked_calloc(size_t count, size_t size)
{
    void *data;
    assert(!size || count <= SIZE_MAX / size);
    data = checked_malloc(count * size);
    if (data) memset(data, 0, count * size);
    return data;
}
#define calloc checked_calloc
#define free checked_free
#include "../src/resources/imf.c"
#undef calloc
#undef free

struct GameFileMgr g_fileMgr;
static BYTE *input;
static int input_size, present = 1, borrowed;
static unsigned prefixes;

struct Res *res_construct(struct Res *base)
{ memset(base, 0, sizeof *base); base->typeIndex = -1; return base; }
void error_msg(const char *text, int flags) { (void)text; (void)flags; abort(); }
BYTE *game_file_mgr_get_data(struct GameFileMgr *manager, const char *name, int *size, BYTE disk)
{
    assert(manager == &g_fileMgr && name && !disk && !borrowed);
    if (!present) { *size = 0; return NULL; }
    *size = input_size; ++borrowed; return input;
}
void game_file_mgr_release_data(struct GameFileMgr *manager, void *data)
{ assert(manager == &g_fileMgr && data == input && borrowed == 1); --borrowed; }
BYTE game_file_mgr_is_file_exist(struct GameFileMgr *manager, const char *name, BYTE disk)
{ (void)manager; (void)name; (void)disk; return (BYTE)present; }
static void set_input(const void *data, size_t size)
{ assert(size <= INT_MAX && !borrowed); input = (BYTE *)data; input_size = (int)size; }
static void clean(void) { assert(!outstanding && !borrowed); }
static void put32(BYTE *data, int32_t value) { memcpy(data, &value, 4); }
static int32_t get32(const BYTE *data) { int32_t value; memcpy(&value, data, 4); return value; }
static void point_is(const struct ImfRes *resource, int layer, int action, int frame, int x, int y)
{
    POINT point = imf_resource_position(resource, layer, action, frame);
    assert(point.x == x && point.y == y);
}
static size_t fixture(BYTE *data)
{
    float version = 1.01f;
    static const int32_t values[] = {
        123, 1,
        2,
        2, 1,10,-20, 0,30,40,
        1, 0,100,200,
        1,
        3, 0,50,60, 1,70,80, 0,90,100,
    };
    memcpy(data, &version, 4); memcpy(data + 4, values, sizeof values);
    return 4 + sizeof values;
}
static void verify_fixture(const struct ImfRes *resource)
{
    assert(resource->count == 2 && resource->layers[0].count == 2 && resource->layers[1].count == 1);
    point_is(resource,0,0,0,10,-20); point_is(resource,0,0,1,30,40);
    point_is(resource,0,1,0,100,200); point_is(resource,1,0,0,50,60);
    point_is(resource,1,0,1,70,80); point_is(resource,1,0,2,90,100);
    assert(imf_resource_layer(resource,0,0,0) == 1 && imf_resource_layer(resource,1,0,0) == 0);
    assert(imf_resource_layer(resource,0,0,1) == 0 && imf_resource_layer(resource,1,0,1) == 1);
    assert(imf_resource_layer(resource,0,0,2) == -1 && imf_resource_layer(resource,1,0,2) == -1);
    assert(imf_resource_layer(resource,0,1,0) == 0 && imf_resource_layer(resource,1,1,0) == -1);
    assert(imf_resource_layer(resource,0,0,3) == -1 && imf_resource_layer(resource,0,2,0) == -1);
    assert(imf_resource_layer(resource,-1,0,0) == -1 && imf_resource_layer(resource,0,-1,0) == -1);
    assert(imf_resource_layer(resource,0,0,-1) == -1 && imf_resource_layer(NULL,0,0,0) == -1);
    point_is(resource,-1,0,0,0,0); point_is(resource,0,-1,0,0,0); point_is(resource,0,0,-1,0,0);
    point_is(resource,2,0,0,0,0); point_is(resource,1,1,0,0,0); point_is(resource,0,0,2,0,0);
    point_is(NULL,0,0,0,0,0); point_is(resource,INT_MAX,INT_MAX,INT_MAX,0,0);
    assert(imf_layer_at(resource,8,0,0,0) == 0 && imf_layer_at(resource,7,0,0,0) == 1);
    assert(imf_layer_at(resource,8,0,0,2) == 9 && imf_layer_at(resource,7,0,0,2) == 9);
    assert(imf_layer_at(NULL,8,0,0,0) == 9 && imf_layer_at(NULL,7,0,0,0) == 9);
    assert(imf_layer_at(NULL,6,0,0,0) == 2 && imf_layer_at(NULL,8,0,2,0) == 7);
    assert(imf_layer_at(resource,-1,0,0,0) == 9 && imf_layer_at(resource,9,0,0,0) == 9);
    assert(imf_layer_at(resource,INT_MIN,INT_MAX,INT_MIN,0) == 9);
}
static void test_replacement(void)
{
    BYTE data[160], bad[160]; size_t size = fixture(data); unsigned i, before;
    struct Res *base = imf_resource_create(); struct ImfRes *resource = (struct ImfRes *)base, saved;
    const unsigned offsets[] = {0,0,8,12,12,16,16};
    const int32_t values[] = {0x3f800000,(int32_t)0xff800000,INT_MAX,-1,INT_MAX,-1,INT_MAX};
    assert(base && base->ops == &imf_resource_ops); set_input(data,size);
    assert(resource_load(base,"layers.imf")); saved = *resource; verify_fixture(resource); before = outstanding;
    for (i = 0; i < sizeof offsets/sizeof *offsets; ++i) {
        memcpy(bad,data,size); put32(bad+offsets[i],values[i]); set_input(bad,size);
        assert(!resource_load(base,"bad.imf") && !memcmp(resource,&saved,sizeof saved));
        assert(outstanding == before && !borrowed); verify_fixture(resource);
    }
    present = 0; assert(!resource_load(base,"missing.imf")); present = 1;
    assert(!resource_load(base,NULL)); set_input(data,size); input_size = -1;
    assert(!resource_load(base,"negative-size.imf")); set_input(data,size);
    assert(!memcmp(resource,&saved,sizeof saved));
    assert(resource_load(base,"replacement.imf") && resource->layers != saved.layers); verify_fixture(resource);
    imf_resource_reset(resource); imf_resource_reset(resource);
    assert(!resource->layers && !resource->count && outstanding == 1);
    assert(resource_load(base,"reload.imf")); verify_fixture(resource);
    memcpy(bad,data,size); put32(bad,0x7fc00000); set_input(bad,size);
    assert(resource_load(base,"nan-version.imf")); verify_fixture(resource);
    memcpy(bad,data,size); put32(bad,0x7f800000); set_input(bad,size);
    assert(resource_load(base,"infinite-version.imf")); verify_fixture(resource);
    memcpy(bad,data,size); put32(bad+8,-2); set_input(bad,size);
    assert(resource_load(base,"no-layers.imf") && !resource->layers && !resource->count);
    memcpy(bad,data,size); put32(bad+60,-1); set_input(bad,size);
    assert(resource_load(base,"no-actions.imf") && resource->count == 2 && !resource->layers[1].count);
    assert(resource->layers[0].count == 2);
    memcpy(bad,data,size); put32(bad+64,-1); set_input(bad,size);
    assert(resource_load(base,"no-frames.imf") && resource->layers[1].count == 1);
    assert(!resource->layers[1].actions[0].count);
    resource_destroy(base); clean();
}
static void test_prefixes(void)
{
    BYTE data[160]; size_t size = fixture(data), n;
    SYSTEM_INFO sys; DWORD old; BYTE *pages, *guard;
    struct Res *base = imf_resource_create(); struct ImfRes saved; unsigned before;
    set_input(data,size); assert(resource_load(base,"old.imf")); saved = *(struct ImfRes *)base; before = outstanding;
    GetSystemInfo(&sys); pages = VirtualAlloc(NULL,sys.dwPageSize*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    assert(pages); guard = pages + sys.dwPageSize; assert(VirtualProtect(guard,sys.dwPageSize,PAGE_NOACCESS,&old));
    for (n = 0; n < size; ++n) {
        memcpy(guard-n,data,n); set_input(guard-n,n);
        assert(!resource_load(base,"prefix.imf") && !memcmp(base,&saved,sizeof saved));
        assert(outstanding == before && !borrowed); ++prefixes;
    }
    resource_destroy(base); assert(VirtualFree(pages,0,MEM_RELEASE)); clean();
}
static void test_allocation_failures(void)
{
    BYTE data[160]; size_t size = fixture(data); unsigned count, start, point, held;
    struct Res *base; struct ImfRes saved;
    fail_at = allocation_calls + 1; assert(!imf_resource_create()); fail_at = 0; clean();
    base = imf_resource_create(); set_input(data,size); start = allocation_calls;
    assert(resource_load(base,"count.imf")); count = allocation_calls-start; assert(count == 6);
    resource_destroy(base); clean();
    for (point = 1; point <= count; ++point) {
        base = imf_resource_create(); assert(resource_load(base,"old.imf")); saved = *(struct ImfRes *)base;
        held = outstanding; fail_at = allocation_calls + point;
        assert(!resource_load(base,"replace.imf")); fail_at = 0;
        assert(!memcmp(base,&saved,sizeof saved) && outstanding == held && !borrowed);
        verify_fixture((struct ImfRes *)base); resource_destroy(base); clean();
    }
}
static void test_empty_and_variable_counts(void)
{
    BYTE data[512]; struct Res *base = imf_resource_create(); struct ImfRes *resource = (struct ImfRes *)base;
    unsigned i;
    fixture(data); put32(data+8,-1); set_input(data,12); assert(resource_load(base,"empty.imf"));
    assert(!resource->layers && !resource->count); point_is(resource,0,0,0,0,0);
    put32(data+8,0); put32(data+12,0); set_input(data,16); assert(resource_load(base,"empty-layer.imf"));
    assert(resource->count == 1 && !resource->layers[0].count);
    put32(data+12,105); memset(data+16,0,105*4); set_input(data,16+105*4);
    assert(resource_load(base,"many-actions.imf") && resource->layers[0].count == 105);
    put32(data+8,15); for (i = 0; i < 16; ++i) put32(data+12+i*4,0);
    set_input(data,12+16*4); assert(resource_load(base,"many-layers.imf") && resource->count == 16);
    resource_destroy(base); clean();
}

struct ExpectedAction { size_t count; const BYTE *records; };
struct ExpectedLayer { size_t count; struct ExpectedAction *actions; };
static unsigned compare_source_records(struct ImfRes *resource, const BYTE *data, size_t size)
{
    size_t layers = (size_t)get32(data+8)+1, offset = 12, layer, action, frame, max_actions = 0;
    unsigned images = 0;
    struct ExpectedLayer *expected = calloc(layers,sizeof *expected); assert(expected);
    assert(resource->count == layers);
    for (layer = 0; layer < layers; ++layer) {
        struct ExpectedLayer *item = expected+layer;
        assert(offset+4 <= size); item->count = (size_t)get32(data+offset); offset += 4;
        assert(item->count == resource->layers[layer].count); if (item->count > max_actions) max_actions = item->count;
        item->actions = calloc(item->count,sizeof *item->actions); assert(item->actions);
        for (action = 0; action < item->count; ++action) {
            struct ExpectedAction *clip = item->actions+action;
            assert(offset+4 <= size); clip->count = (size_t)get32(data+offset); offset += 4;
            clip->records = data+offset; offset += clip->count*12; assert(offset <= size);
            assert(resource->layers[layer].actions[action].count == clip->count);
            for (frame = 0; frame < clip->count; ++frame) {
                const BYTE *record = clip->records+frame*12;
                assert(resource->layers[layer].actions[action].frames[frame].order == get32(record));
                point_is(resource,(int)layer,(int)action,(int)frame,get32(record+4),get32(record+8)); ++images;
            }
            point_is(resource,(int)layer,(int)action,(int)clip->count,0,0);
        }
    }
    assert(offset == size);
    for (action = 0; action < max_actions; ++action) {
        size_t most = 0, order;
        for (layer = 0; layer < layers; ++layer)
            if (action < expected[layer].count && expected[layer].actions[action].count > most)
                most = expected[layer].actions[action].count;
        for (frame = 0; frame <= most; ++frame) for (order = 0; order <= layers; ++order) {
            int found = -1;
            for (layer = 0; layer < layers && order < layers && action < expected[order].count
                    && frame < expected[order].actions[action].count; ++layer) if (action < expected[layer].count) {
                struct ExpectedAction *clip = expected[layer].actions+action;
                if (frame < clip->count && get32(clip->records+frame*12) == (int)order) { found = (int)layer; break; }
            }
            assert(imf_resource_layer(resource,(int)order,(int)action,(int)frame) == found);
        }
    }
    for (layer = 0; layer < layers; ++layer) free(expected[layer].actions);
    free(expected); return images;
}
static int corpus(const char *path)
{
    FILE *file = fopen(path,"rb"); uint32_t size; unsigned files = 0, frames = 0;
    struct Res *base = imf_resource_create(); assert(file && base);
    while (fread(&size,4,1,file) == 1) {
        BYTE *data = malloc(size); assert(data && size >= 12 && size <= INT_MAX);
        assert(fread(data,size,1,file) == 1); set_input(data,size); assert(resource_load(base,"asset.imf"));
        frames += compare_source_records((struct ImfRes *)base,data,size); ++files; free(data);
    }
    assert(feof(file) && !ferror(file)); fclose(file); resource_destroy(base); clean();
    printf("IMF corpus: %u files, %u frame records and all layer/position queries match source data.\n",files,frames);
    return 0;
}
int main(int argc, char **argv)
{
    if (argc == 2) return corpus(argv[1]);
    test_replacement(); test_prefixes(); test_allocation_failures(); test_empty_and_variable_counts();
    printf("IMF owner: %u guarded prefixes, malformed counts, unequal layers, missing queries, actor ordering, replacement and allocation failures pass.\n",prefixes);
    return 0;
}
