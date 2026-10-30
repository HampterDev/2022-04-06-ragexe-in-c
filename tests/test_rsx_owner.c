#include "resources/anim_model_res.h"
#include "render/texture_cache.h"
#include "fileio/file_api.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rsx_fixture.h"

#ifndef CAPTURE_RSX_REFERENCE
#include "owned_storage_test_allocator.h"
static void *rsx_calloc(size_t count, size_t size)
{
    void *p;
    assert(!size || count <= (size_t)-1 / size);
    p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
#define malloc checked_malloc
#define calloc rsx_calloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/resources/anim_model_res.c"
#undef malloc
#undef calloc
#undef free
#else
void *mem_alloc(size_t size) { void *p = malloc(size); assert(p); return p; }
void *mem_alloc(size_t size) { return mem_alloc(size); }
void mem_free(void *p) { free(p); }
void mem_free(void *p) { free(p); }
void mem_free(void *p) { (void)size; free(p); }
void mem_free(void *p) { (void)size; free(p); }
void fatal_error(const char *text) { (void)text; abort(); }
void fatal_error(const char *text) { (void)text; abort(); }
struct ListNode *std_list_ptr_buy_node(struct List *list, struct ListNode *next,
                                         struct ListNode *prev, void *const *value)
{ return list_ptr_buy_node(next, prev, value); }
#endif

struct TexCache g_textureCache;
static struct Texture textures[2];
static unsigned texture_calls[16], calls, locks, releases, closes, destructs, errors;
static int open_failure, missing_texture;
struct Res *res_construct(struct Res *res) { memset(res, 0, sizeof *res); res->typeIndex = -1; return res; }
void res_destruct(struct Res *res) { res->ops = NULL; }
void error_msg(const char *text, int flags) { assert(text && !flags); ++errors; }
struct File *file_construct(struct File *file) { memset(file, 0, sizeof *file); return file; }
unsigned char file_open(struct File *file, const char *name, int mode, unsigned char resolve, unsigned char disk)
{
    assert(name && !mode && resolve == 1 && !disk);
    if (open_failure) return 0;
    file->buffer = input; file->size = used; return 1;
}
unsigned char file_read(struct File *file, void *out, int size)
{
    assert(size >= 0 && (out || !size));
    if (size > file->size - file->position) return 0;
    if (size) memcpy(out, file->buffer + file->position, (size_t)size);
    file->position += size; return 1;
}
void file_close(struct File *file) { (void)file; ++closes; }
void file_destruct(struct File *file) { (void)file; ++destructs; }
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int keyed, int full)
{
    unsigned index = strcmp(name, texture_names[0]) == 0 ? 0 : 1;
    assert(cache == &g_textureCache && !keyed && !full && !strcmp(name, texture_names[index]));
    assert(calls < 16); texture_calls[calls++] = index;
    return missing_texture ? NULL : &textures[index];
}
void texture_lock(struct Texture *texture) { assert(texture == &textures[0] || texture == &textures[1]); ++locks; }
void texture_unlock(struct Texture *texture) { assert(texture && locks); --locks; }
void ui_tex_cache_release(struct TexCache *cache, struct Texture *texture)
{ assert(cache == &g_textureCache && texture); ++releases; }

static FILE *reference;
static unsigned samples;
static void record(const void *data, size_t size)
{
#ifdef CAPTURE_RSX_REFERENCE
    assert(fwrite(data, 1, size, reference) == size);
#else
    unsigned char expected[256];
    assert(size <= sizeof expected && fread(expected, 1, size, reference) == size);
    assert(!memcmp(data, expected, size));
#endif
    ++samples;
}
static void record_int(int value) { record(&value, sizeof value); }
static void record_text(const char *text) { size_t n = strlen(text); record_int((int)n); record(text, n); }

static void check_loaded(struct Res *base)
{
    unsigned n, k, i;
#ifdef CAPTURE_RSX_REFERENCE
    struct C3dAnimModelRes *model = (struct C3dAnimModelRes *)base;
    struct ListNode *it = model->nodes.head->next;
    record_int(model->frameCount); record_int(model->headerValue0); record_int(model->headerValue1);
    record(&model->version12Scalar, 4); record(model->version12Values, 12); record_int(model->version13Value);
    record_int((int)model->nodes.size);
    assert(model->mainNodes.size == 1);
    record_text(string_cstr(LIST_VALUE(model->mainNodes.head->next, struct String)));
    for (n = 0; n < model->nodes.size; ++n, it = it->next) {
        struct AnimNode *node = STD_LIST_POINTER(it);
        record_text(string_cstr(&node->name)); record_text(string_cstr(&node->parentName));
        record_int(node->vertCount); record_int(node->texVertCount); record_int(node->faceCount);
        record_int(node->textureCount); record_int(node->sharedTexVerts);
        for (k = 0; k < (unsigned)model->frameCount; ++k) {
            record(node->frames[k].verts, (size_t)node->vertCount * 12);
            if (!node->sharedTexVerts || !k) record(node->frames[k].texVerts, (size_t)node->texVertCount * 12);
            else assert(!node->frames[k].texVerts);
        }
        record(node->faces, (size_t)node->faceCount * 24);
        for (i = 0; i < (unsigned)node->textureCount; ++i) record_int(node->textures[i] == &textures[1]);
        record_text(node->parent ? string_cstr(&node->parent->name) : "");
        record_int((int)node->children.size);
        if (node->children.size) {
            struct AnimNode *child = STD_LIST_POINTER(node->children.head->next);
            assert(node->children.size == 1); record_text(string_cstr(&child->name));
        }
    }
#else
    struct AnimatedModelResource *model = (struct AnimatedModelResource *)base;
    record_int((int)model->frameCount); record_int(model->headerValue0); record_int(model->headerValue1);
    record(&model->version12Scalar, 4); record(model->version12Values, 12); record_int(model->version13Value);
    record_int((int)model->nodeCount); record_text(owned_text_cstr(&model->mainNode));
    for (n = 0; n < model->nodeCount; ++n) {
        struct AnimatedModelNode *node = model->nodes[n];
        assert(node->frameCount == model->frameCount);
        record_text(owned_text_cstr(&node->name)); record_text(owned_text_cstr(&node->parentName));
        record_int((int)node->vertexCount); record_int((int)node->textureVertexCount); record_int((int)node->faceCount);
        record_int((int)node->textureCount); record_int(node->sharedTextureVertices);
        for (k = 0; k < node->frameCount; ++k) {
            record(node->frames[k].vertices, node->vertexCount * 12);
            if (!node->sharedTextureVertices || !k) record(node->frames[k].textureVertices, node->textureVertexCount * 12);
            else assert(!node->frames[k].textureVertices);
        }
        record(node->faces, node->faceCount * 24);
        for (i = 0; i < node->textureCount; ++i) record_int(node->textures[i] == &textures[1]);
        record_text(node->parent ? owned_text_cstr(&node->parent->name) : "");
        record_int((int)node->childCount);
        if (node->childCount) {
            assert(node->childCount == 1); record_text(owned_text_cstr(&node->children[0]->name));
        }
    }
#endif
    assert(locks == 6 && calls == 8);
    record(texture_calls, calls * sizeof *texture_calls);
}

#ifndef CAPTURE_RSX_REFERENCE
static void expect_failed_reload(struct Res *res, unsigned resources)
{
    unsigned char before[sizeof(struct AnimatedModelResource)];
    unsigned before_closes = closes, before_destructs = destructs;
    memcpy(before, res, sizeof before);
    calls = 0;
    assert(!res->ops->load(res, "모델.rsx"));
    assert(!memcmp(before, res, sizeof before));
    assert(outstanding == resources && locks == 6);
    assert(closes == before_closes + (open_failure ? 0 : 1) && destructs == before_destructs + 1);
}

static void failure_and_reload(void)
{
    int shared;
    allocation_calls = 0; fail_at = 1;
    assert(!animated_model_resource_create() && !outstanding);
    fail_at = 0;
    for (shared = 0; shared <= 1; ++shared) {
        struct Res *res = animated_model_resource_create();
        struct AnimatedModelResource *model = (struct AnimatedModelResource *)res;
        unsigned failure, total, resources;
        int cut, complete, i, kind;
        int offsets[8];
        assert(res);
        res->lockCount = 5; res->typeIndex = 14; res->lastUse = 123;
        strcpy(res->name.name, "cache key");
        fixture(1, 3, shared); calls = 0; allocation_calls = 0;
        assert(res->ops->load(res, "모델.rsx"));
        total = allocation_calls; resources = outstanding;
        for (failure = 1; failure <= total; ++failure) {
            allocation_calls = 0; fail_at = failure;
            expect_failed_reload(res, resources);
            fail_at = 0;
        }
        complete = used;
        for (cut = 0; cut < complete; ++cut) {
            used = cut;
            expect_failed_reload(res, resources);
        }
        used = complete;
        open_failure = 1; expect_failed_reload(res, resources); open_failure = 0;
        missing_texture = 1; expect_failed_reload(res, resources); missing_texture = 0;
        offsets[0] = frame_offset; offsets[1] = node_count_offset;
        offsets[2] = texture_offset[0] - 4; offsets[3] = vertex_offset[0];
        offsets[4] = uv_offset[0]; offsets[5] = face_offset[0];
        offsets[6] = vertex_offset[1]; offsets[7] = frame_offset + 24;
        for (i = 0; i < 8; ++i) {
            for (kind = 0; kind < 2; ++kind) {
                int value = kind ? INT_MAX : -1;
                fixture(1, 3, shared);
                memcpy(input + offsets[i], &value, sizeof value);
                expect_failed_reload(res, resources);
            }
        }
        for (i = 0; i < 2; ++i) {
            fixture(1, 3, shared); input[start_offset[i]] ^= 1;
            expect_failed_reload(res, resources);
            fixture(1, 3, shared); input[end_offset[i]] ^= 1;
            expect_failed_reload(res, resources);
            fixture(1, 3, shared); kind = i ? -1 : 2;
            memcpy(input + texture_offset[0], &kind, 4);
            expect_failed_reload(res, resources);
        }
        fixture(1, 3, shared);
        memset(input + parent_offset[0], 0, 40); memcpy(input + parent_offset[0], "absent", 6);
        expect_failed_reload(res, resources);
        fixture(1, 3, shared);
        memcpy(input + parent_offset[1], node_names[0], strlen(node_names[0]));
        expect_failed_reload(res, resources);
        fixture(1, 3, shared);
        memcpy(input + parent_offset[1], node_names[1], strlen(node_names[1]));
        expect_failed_reload(res, resources);

        fixture(1, 3, shared); calls = 0;
        assert(res->ops->load(res, owned_text_cstr(&model->mainNode)));
        assert(outstanding == resources && locks == 6 && model->nodeCount == 2);
        assert(res->lockCount == 5 && res->typeIndex == 14 && res->lastUse == 123 && !strcmp(res->name.name, "cache key"));
        assert(model->nodes[1]->children[0] == model->nodes[0]);
        memcpy(input, "TEST", 4); calls = 0;
        assert(res->ops->load(res, "모델.rsx"));
        fixture(1, 3, shared);
        memset(input + start_offset[0] + 4, 'x', 40); calls = 0;
        assert(res->ops->load(res, "모델.rsx"));
        assert(model->nodes[0]->name.size == 40 && strlen(owned_text_cstr(&model->nodes[0]->name)) == 40);
        animated_model_clear(model); animated_model_clear(model);
        assert(outstanding == 1 && !locks && !model->nodes && !model->nodeCount);
        res->ops->destroy(res); assert(!outstanding);
    }
}

static void empty_models(void)
{
    int frames, nodes;
    for (frames = 0; frames <= 1; ++frames) {
        for (nodes = 0; nodes <= 1; ++nodes) {
            const unsigned char version[] = {1,1}, skipped[16] = {0};
            struct Res *res = animated_model_resource_create();
            struct AnimatedModelResource *model = (struct AnimatedModelResource *)res;
            int shared_at = 0;
            used = 0; put("GRSX", 4); put(version, sizeof version);
            integer(2); integer(frames); integer(0); put(skipped, sizeof skipped);
            integer(0); name_record(""); integer(nodes);
            if (nodes) {
                integer(0x12345678); name_record(""); name_record("");
                integer(0); integer(0); integer(0); shared_at = used;
                integer(0); integer(0); integer(0x12345678);
            }
            calls = 0;
            assert(res && res->ops->load(res, "empty.rsx"));
            assert(model->frameCount == (size_t)frames && model->nodeCount == (size_t)nodes && !locks);
            if (nodes) {
                int shared = 1;
                memcpy(input + shared_at, &shared, 4);
                assert(res->ops->load(res, "empty.rsx") == (frames != 0));
            }
            res->ops->destroy(res);
            assert(!outstanding && !locks);
        }
    }
}
#endif

int main(int argc, char **argv)
{
    static const unsigned char versions[][2] = {{1,0},{1,1},{1,2},{1,3},{1,4},{2,1}};
    unsigned v;
    int shared;
    assert(argc == 2);
#ifdef CAPTURE_RSX_REFERENCE
    reference = fopen(argv[1], "wb");
#else
    reference = fopen(argv[1], "rb");
#endif
    assert(reference);
    for (v = 0; v < sizeof versions / sizeof versions[0]; ++v) {
        for (shared = 0; shared <= 1; ++shared) {
            struct Res *res;
            calls = 0; fixture(versions[v][0], versions[v][1], shared);
#ifdef CAPTURE_RSX_REFERENCE
            res = C3dAnimModelRes_Clone();
#else
            res = animated_model_resource_create();
#endif
            assert(res && res->ops->load(res, "모델.rsx")); check_loaded(res);
            res->ops->destroy(res); assert(!locks);
        }
    }
#ifndef CAPTURE_RSX_REFERENCE
    assert(fgetc(reference) == EOF && !outstanding);
#endif
    assert(!fclose(reference));
#ifndef CAPTURE_RSX_REFERENCE
    failure_and_reload();
    empty_models();
    puts("RSX ownership: every allocation/read failure, atomic reload, bounded names/counts, parent validation and empty models pass");
#endif
    printf("RSX ownership: %u archived decoded records, texture order and hierarchy match\n", samples);
    return 0;
}
