#include "render/texture_cache.h"
#include "effects/rag/rag_prim_setup.h"
#include "core/fatal.h"
#include "std/list.h"
#include "std/list.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static char worlds[2];
static struct Texture borrowed_texture;
static int checking_texture_array;
static size_t requested_texture_bytes;
static struct World *world;
static struct List objects;
static struct ListNode object_head;
static struct RagEffect effect;
static struct ListNode prim_head;
static struct {
    struct ListNode links;
    struct EffectPrim *prim;
} prim_node;
static struct RenderObjectOps methods;
static struct EffectPrim *owned;
static int list_present, fail_allocation, replace_world, allocations, deletes, inserts, textures;
struct TexCache g_textureCache;

struct World *game_current_world(void) { return world; }
struct List *game_current_object_list(void) { return list_present ? &objects : NULL; }

void *mem_alloc(size_t bytes)
{
    if (checking_texture_array) {
        unsigned char *block;
        requested_texture_bytes = bytes;
        if (bytes > 4096) return NULL;
        block = malloc(bytes + 16);
        assert(block);
        memset(block, 0x5a, bytes + 16);
        return block;
    }
    if (bytes == sizeof(void *)) return calloc(1, bytes);
    assert(bytes == sizeof(struct EffectPrim));
    ++allocations;
    return fail_allocation ? NULL : calloc(1, bytes);
}

static void destroy(struct GameObject *self)
{
    assert( inserts == 0 && effect.prims.size == 0);
    ++deletes;
    free(self);
}

struct EffectPrim *effect_prim_construct(struct EffectPrim *self)
{
    self->base.ops = &methods.base;
    self->motion_count = 1;
    if (replace_world)
        world = (struct World *)&worlds[1];
    return self;
}

void effect_prim_init(struct EffectPrim *self, struct RagEffect *master, int kind, Vec3 offset)
{
    assert(master == &effect && kind == 12);
    self->master = master;
    self->delta = offset;
}

void *list_insert(struct List *list, struct ListNode *where, const void *value, size_t size)
{
    struct EffectPrim *const *prim = value;

    if (list == &objects) {
        assert(where == &object_head && world == (struct World *)&worlds[0]);
        assert(size == sizeof(void *) && effect.prims.size == 0 && deletes == 0);
        owned = *prim;
        ++inserts;
        return NULL;
    }
    assert(list == &effect.prims && where == &prim_head && prim_head.prev == &prim_head);
    assert(size == sizeof *prim && *prim == owned && inserts == 1 && deletes == 0);
    prim_node.links.next = where;
    prim_node.links.prev = where->prev;
    where->prev->next = &prim_node.links;
    where->prev = &prim_node.links;
    list->size++;
    prim_node.prim = *prim;
    return &prim_node.prim;
}

void fatal_error(const char *message) { abort(); }

struct Texture *texture_cache_get(struct TexCache *cache,
                                           const char *name, int flag1, int flag2)
{
    assert(cache == &g_textureCache && strcmp(name, "test.tga") == 0);
    ++textures;
    return &borrowed_texture;
}

static void reset(void)
{
    if (owned) {
        free(owned->textures);
        free(owned);
        owned = NULL;
    }
    memset(&effect, 0, sizeof effect);
    prim_head.next = prim_head.prev = &prim_head;
    effect.prims.head = &prim_head;
    effect.duration = 77;
    world = (struct World *)&worlds[0];
    objects.head = &object_head;
    list_present = 1;
    fail_allocation = replace_world = allocations = deletes = inserts = textures = 0;
}

static void test_texture_arrays(void)
{
    static const int counts[] = {0, 1, 2, 17};
    static const size_t byte_counts[] = {0, 4, 8, 68};
    static const int limits[] = {0x3fffffff, 0x40000000, INT_MAX, -1};
    struct EffectPrim prim = {0};
    unsigned path, i, j;

    checking_texture_array = 1;
    for (path = 0; path < 3; ++path) {
        for (i = 0; i < sizeof counts / sizeof counts[0]; ++i) {
            unsigned char *bytes;
            prim.motion_count = counts[i];
            if (path == 0) prim.textures = rag_prim_new_textures(&prim);
            else if (path == 1) prim_new_textures(&prim);
            else RAG_NEW_TEXTURES(&prim);
            assert(prim.textures && requested_texture_bytes == byte_counts[i]);
            for (j = 0; j < (unsigned)counts[i]; ++j) prim.textures[j] = &borrowed_texture;
            for (j = 0; j < (unsigned)counts[i]; ++j) assert(prim.textures[j] == &borrowed_texture);
            bytes = (unsigned char *)prim.textures;
            for (j = 0; j < 16; ++j) assert(bytes[byte_counts[i] + j] == 0x5a);
            free(prim.textures);
            prim.textures = NULL;
        }
    }
    for (i = 0; i < sizeof limits / sizeof limits[0]; ++i) {
        prim.motion_count = limits[i];
        assert(!rag_prim_new_textures(&prim));
        assert(requested_texture_bytes == (i == 0 ? 0xfffffffcu : SIZE_MAX));
    }
    checking_texture_array = 0;
    puts("primitive textures: typed borrowed values, empty arrays, guarded motion slots and overflow requests passed");
}

int main(void)
{
    struct EffectPrim *prim;
    test_texture_arrays();
    methods.base.destroy = destroy;
    reset();
    world = NULL;
    assert(rag_launch_textured(&effect, 12, "test.tga") == NULL);
    assert(!allocations && !inserts && !textures && effect.last_prim == NULL);

    reset();
    list_present = 0;
    assert(rag_launch_prim(&effect, 12, 1, 2, 3) == NULL && !allocations);
    reset();
    objects.head = NULL;
    assert(rag_launch_prim(&effect, 12, 1, 2, 3) == NULL && !allocations);

    reset();
    fail_allocation = 1;
    effect.last_prim = (struct EffectPrim *)&prim_head;
    assert(rag_launch_textured(&effect, 12, "test.tga") == NULL);
    assert(allocations == 1 && !deletes && !inserts && !textures && effect.last_prim == NULL);

    reset();
    replace_world = 1;
    assert(rag_launch_textured(&effect, 12, "test.tga") == NULL);
    assert(allocations == 1 && deletes == 1 && !inserts && !textures && !effect.prims.size);

    reset();
    prim = rag_launch_prim(&effect, 12, 1, 2, 3);
    assert(prim == owned && inserts == 1 && !deletes && effect.prims.size == 1);
    assert(prim->delta.x == 1 && prim->delta.y == 2 && prim->delta.z == 3);
    assert(prim_head.next == &prim_node.links && prim_head.prev == &prim_node.links && prim_node.prim == prim);

    reset();
    prim = rag_launch_textured(&effect, 12, "test.tga");
    assert(prim == owned && effect.last_prim == prim && textures == 1);
    assert(prim->list_flags == 5 && prim->duration == 77 && prim->textures[0] == &borrowed_texture);
    reset();
    puts("particle creation: missing context, allocation failure, changed world, ownership and setup passed");
    return 0;
}
