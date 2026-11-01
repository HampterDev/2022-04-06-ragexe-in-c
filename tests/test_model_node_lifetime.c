#include "world/model.h"
#include "render/texture_cache.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"

struct TexCache g_textureCache;
static struct Texture texture;
static unsigned unlocks, releases, meshes, nodes_deleted;
static void *node_addresses[2];
static size_t node_sizes[2];
static unsigned char node_alive[2];

static void release_bytes(void *memory, size_t size)
{
    unsigned i;
    if (!memory) return;
    assert(((struct Allocation *)memory - 1)->size == size);
    for (i = 0; i < 2; ++i) {
        if (memory != node_addresses[i]) continue;
        assert(node_alive[i] && size == node_sizes[i]);
        node_alive[i] = 0;
        ++nodes_deleted;
    }
    checked_free(memory);
}

static void tracked_free(void *memory)
{ if (memory) release_bytes(memory, ((struct Allocation *)memory - 1)->size); }
#define TEST_RENDER_NODES
#define MODEL_TEST_FREE tracked_free
#include "model_owner_test_support.h"

void *mem_alloc(size_t size) { return checked_malloc(size); }
void mem_free(void *memory)
{ tracked_free(memory); }
void *std_allocate_bytes(size_t size) { return checked_malloc(size); }

void fatal_error(const char *message) { (void)message; abort(); }
void texture_unlock(struct Texture *value)
{
    assert(value == &texture && unlocks == releases);
    ++unlocks;
}
void ui_tex_cache_release(struct TexCache *cache, struct Texture *value)
{
    assert(cache == &g_textureCache && value == &texture && unlocks == releases + 1);
    ++releases;
}
void anim_mesh_destroy(struct AnimMesh *mesh)
{
    if (!mesh) return;
    assert(((struct Allocation *)mesh - 1)->size == sizeof *mesh);
    ++meshes;
    checked_free(mesh);
}

static void fill_common(struct ModelResNode *node, int populated)
{
    assert(node->destroy && !node->model && !node->parent && !node->mesh);
    assert(node->alpha == 255 && !node->childCount && !node->children);
    if (!populated) return;
    assert(owned_text_assign(&node->name, text_view_cstr("아주_긴_노드_이름")));
    assert(owned_text_assign(&node->parentName, text_view_cstr("borrowed_parent_node")));
    node->textures = model_test_calloc(3, sizeof *node->textures);
    node->textureCount = 3;
    node->textures[0] = node->textures[2] = &texture;
    node->mesh = model_test_calloc(1, sizeof *node->mesh);
}

static void fill_uv_keys(struct UvAnimation *animation, int populated)
{
    if (!populated) return;
    animation->tracks = model_test_calloc(2, sizeof *animation->tracks);
    animation->count = 2;
    animation->tracks[0].texture = 0;
    animation->tracks[1].texture = 2;
    animation->tracks[0].channels[0].keys = model_test_calloc(3, sizeof(struct KeyFloat));
    animation->tracks[0].channels[0].count = 3;
    animation->tracks[1].channels[4].keys = model_test_calloc(2, sizeof(struct KeyFloat));
    animation->tracks[1].channels[4].count = 2;
}

static void check_reset(int populated, int reverse)
{
    struct C3dModelRes model = {0};
    struct ModelResNodeV1 *a = (struct ModelResNodeV1 *)model_res_node_create(1);
    struct ModelResNodeV2 *b = (struct ModelResNodeV2 *)model_res_node_create(2);
    unsigned before_releases = releases, before_meshes = meshes;
    unsigned before_deleted = nodes_deleted;
    node_addresses[0] = a; node_sizes[0] = sizeof *a; node_alive[0] = 1;
    node_addresses[1] = b; node_sizes[1] = sizeof *b; node_alive[1] = 1;
    fill_common(&a->base, populated);
    fill_common(&b->base, populated);
    a->base.destroy = model_res_node_v1_destroy;
    b->base.destroy = model_res_node_v2_destroy;
    a->base.model = b->base.model = &model;
    fill_uv_keys(&b->uvKeys, populated);
    if (populated) {
        a->rotKeys = model_test_calloc(2, sizeof *a->rotKeys); a->rotKeyCount = 2;
        b->scaleKeys = model_test_calloc(4, sizeof *b->scaleKeys); b->scaleKeyCount = 4;
        b->rotKeys = model_test_calloc(5, sizeof *b->rotKeys); b->rotKeyCount = 5;
        b->posKeys = model_test_calloc(6, sizeof *b->posKeys); b->posKeyCount = 6;
        a->base.parent = &b->base;
        b->base.parent = &a->base;
        a->base.children = model_test_calloc(1, sizeof *a->base.children);
        b->base.children = model_test_calloc(1, sizeof *b->base.children);
        a->base.childCount = b->base.childCount = 1;
        a->base.children[0] = &b->base; b->base.children[0] = &a->base;
    }
    model.nodes = model_test_calloc(3, sizeof *model.nodes); model.nodeCount = 3;
    model.nodes[0] = reverse ? &b->base : &a->base;
    model.nodes[2] = reverse ? &a->base : &b->base;
    if (populated) {
        model.blocks = model_test_calloc(1, sizeof *model.blocks); model.blockCount = 1;
        assert(text_array_resize(&model.mainNodes, 1));
        assert(owned_text_assign(&model.mainNodes.items[0], text_view_cstr("root")));
        assert(owned_text_assign(&model.fileName, text_view_cstr("model.rsm")));
    }
    model_res_clear(&model);
    assert(nodes_deleted == before_deleted + 2 && !node_alive[0] && !node_alive[1]);
    assert(releases == before_releases + (populated ? 4 : 0));
    assert(meshes == before_meshes + (populated ? 2 : 0));
    assert(!model.nodeCount && !model.nodes && !model.blockCount && !model.blocks);
    assert(!model.mainNodes.size && !model.fileName.data);
    assert(!outstanding);
    model_res_clear(&model);
    assert(!outstanding && nodes_deleted == before_deleted + 2);
    node_addresses[0] = node_addresses[1] = NULL;
    assert(!outstanding);
}

static void check_render_nodes(int version)
{
    struct C3dActor owner = {0};
    struct ModelResNode *a = model_res_node_create(version), *b = model_res_node_create(version);
    unsigned before_meshes, before_releases, before_deleted;
    unsigned before, failure = 0, total = 1;
    fill_common(a, 1); fill_common(b, 1);
    a->children = model_test_calloc(1, sizeof *a->children); a->childCount = 1;
    a->children[0] = b; b->parent = a;
    if (version == 1) {
        struct ModelResNodeV1 *v1a = (struct ModelResNodeV1 *)a, *v1b = (struct ModelResNodeV1 *)b;
        v1a->scale[0] = v1a->scale[1] = v1a->scale[2] = 1;
        v1b->scale[0] = v1b->scale[1] = v1b->scale[2] = 1;
    } else {
        struct ModelResNodeV2 *v2a = (struct ModelResNodeV2 *)a, *v2b = (struct ModelResNodeV2 *)b;
        memcpy(v2a->matrix, g_identityMatrix.m, sizeof v2a->matrix);
        memcpy(v2b->matrix, g_identityMatrix.m, sizeof v2b->matrix);
        fill_uv_keys(&v2a->uvKeys, 1); fill_uv_keys(&v2b->uvKeys, 1);
    }
    before_meshes = meshes; before_releases = releases; before = outstanding;
    do {
        struct C3dNode *root;
        allocation_calls = 0; fail_at = failure;
        root = model_node_create(version, &owner, NULL, a);
        fail_at = 0;
        if (!failure) {
            size_t size = version == 1 ? sizeof(struct C3dNodeV1) : sizeof(struct C3dNodeV2);
            assert(root && root->childCount == 1 && root->children[0]->parent == root);
            assert(root->name == a->name.data && root->mesh == a->mesh && root->textures == a->textures);
            assert(root->resource == a && root->children[0]->resource == b);
            node_addresses[0] = root; node_addresses[1] = root->children[0];
            node_sizes[0] = node_sizes[1] = size; node_alive[0] = node_alive[1] = 1;
            before_deleted = nodes_deleted;
            total = allocation_calls;
            if (version == 1) c3d_node_v1_animate(root, 0);
            else c3d_node_v2_animate(root, 0);
            model_node_destroy(root, version);
            assert(nodes_deleted == before_deleted + 2 && !node_alive[0] && !node_alive[1]);
            node_addresses[0] = node_addresses[1] = NULL;
        } else assert(!root);
        assert(outstanding == before && meshes == before_meshes && releases == before_releases);
    } while (++failure <= total);
    a->destroy(a); b->destroy(b);
    assert(!outstanding && meshes == before_meshes + 2 && releases == before_releases + 4);
}

int main(void)
{
    check_reset(0, 0);
    check_reset(0, 1);
    check_reset(1, 0);
    check_reset(1, 1);
    check_render_nodes(1);
    check_render_nodes(2);
    puts("model node lifetime: typed V1/V2 destruction, borrowed child cycles, shared textures, C animation arrays and repeated reset passed");
    puts("render node lifetime: typed V1/V2 operations, owned child trees, borrowed actor/resource storage and every creation allocation failure passed");
    return 0;
}
