#include "render/screen_bounds.h"
#define GRANNY_IMPORT
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resources/granny_res.h"
#include "owned_storage_test_allocator.h"

void *model_test_calloc(size_t count, size_t size)
{
    void *p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
void model_test_free(void *memory) { checked_free(memory); }
#define malloc checked_malloc
#define calloc model_test_calloc
#define free checked_free
#include "../src/resources/granny_playback.c"
#include "../src/actor/granny_projection.c"
#include "../src/actor/granny_model_storage.c"
#undef malloc
#undef calloc
#undef free
#include "../src/actor/granny_model.c"
#include "granny_sdk_test_support.h"

enum { TEST_MESH_COUNT = 65 };
struct MeshData {
    int vertex_count, index_count, rigid, groups[3];
    struct GrannyMeshVertex vertices[8];
    unsigned short indices[3];
};
struct ModelFixture {
    struct C3dGrannyModelRes resource;
    struct GrannyResModel model;
    granny_model sdk_model;
    granny_skeleton skeleton;
    granny_mesh meshes[TEST_MESH_COUNT];
    struct GrannyResMesh mesh_resources[TEST_MESH_COUNT];
    struct MeshData data[TEST_MESH_COUNT];
    struct GrannyResTexture material;
    struct GrannyResTexture *materials[1];
};
static struct ModelFixture fixtures[5];
static struct C3dGrannyModelRes *loaded_resource;

static void fixture_init(struct ModelFixture *f, int count)
{
    int i, j;
    memset(f, 0, sizeof *f);
    f->resource.model = &f->model; f->resource.grannyModel = &f->sdk_model;
    f->resource.skeleton = &f->skeleton; f->skeleton.BoneCount = 3;
    f->resource.bounds.half_width = f->resource.bounds.half_height = 10.0f;
    f->sdk_model.Skeleton = &f->skeleton;
    f->model.meshCount = count; f->model.meshes = f->mesh_resources;
    f->material.texture = (struct Texture *)&f->material; f->materials[0] = &f->material;
    for (i = 0; i < count; i++) {
        struct GrannyResMesh *m = &f->mesh_resources[i];
        struct MeshData *d = &f->data[i];
        f->meshes[i].Name = i == 0 ? "hair_pivot" : "mesh";
        f->meshes[i].PrimaryVertexData = d;
        d->vertex_count = 8; d->index_count = 3; d->rigid = !(i & 1);
        d->indices[0] = 0; d->indices[1] = 2; d->indices[2] = 7;
        for (j = 0; j < 8; j++) {
            d->vertices[j].pos.x = (float)(i + j);
            d->vertices[j].pos.y = (float)j;
            d->vertices[j].pos.z = (float)(2 * j);
            d->vertices[j].ny = 1.0f; d->vertices[j].u = 0.5f; d->vertices[j].v = 0.25f;
        }
        m->mesh = &f->meshes[i]; m->indices = d->indices;
        m->deformer = (granny_mesh_deformer *)1; m->rigidVerts = d->vertices;
        m->materialCount = 1; m->textures = f->materials;
    }
}
int __stdcall _GrannyGetMeshVertexCount(granny_mesh *mesh)
{ return ((struct MeshData *)mesh->PrimaryVertexData)->vertex_count; }
int __stdcall _GrannyGetMeshIndexCount(granny_mesh *mesh)
{ return ((struct MeshData *)mesh->PrimaryVertexData)->index_count; }
BYTE __stdcall _GrannyMeshIsRigid(granny_mesh *mesh)
{ return (BYTE)((struct MeshData *)mesh->PrimaryVertexData)->rigid; }
int *__stdcall _GrannyGetMeshTriangleGroups(granny_mesh *mesh)
{ return ((struct MeshData *)mesh->PrimaryVertexData)->groups; }
void *__stdcall _GrannyGetMeshVertices(granny_mesh *mesh)
{ return ((struct MeshData *)mesh->PrimaryVertexData)->vertices; }
int __stdcall _GrannyGetMeshBytesPerIndex(granny_mesh *mesh) { (void)mesh; return 2; }
void *__stdcall _GrannyGetMeshIndices(granny_mesh *mesh)
{ return ((struct MeshData *)mesh->PrimaryVertexData)->indices; }
void __stdcall _GrannyDeformVertices(granny_mesh_deformer *deformer, const int *bones,
                                    const float *matrix, int count, const void *source, void *out)
{
    struct GrannyMeshVertex *vertices = out;
    int i;
    assert(deformer == (granny_mesh_deformer *)1 && bones[0] == 0 && count == 8);
    memcpy(out, source, (size_t)count * sizeof *vertices);
    for (i = 0; i < count; i++) vertices[i].pos.x += matrix[12];
}
void res_lock(struct Res *res) { res->lockCount++; }
void res_unlock(struct Res *res) { assert(res->lockCount > 0); res->lockCount--; }
struct ResMgr *g_resMgr(void) { return (struct ResMgr *)1; }
void *res_mgr_get(struct ResMgr *mgr, const char *name)
{
    assert(mgr == (struct ResMgr *)1 && name);
    return loaded_resource;
}
void game_actor_set_job(struct GameActor *actor, int job) { actor->job = job; }
void game_actor_set_sex(struct GameActor *actor, int sex) { actor->sex = sex; }

static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
static struct TexDrawVertex captured[TEST_MESH_COUNT * 4][3], frame_vertices[3];
static struct RPQuad frame_quad;
static int submissions;
struct TexDrawVertex *tex_draw_vertex_construct(struct TexDrawVertex *v)
{ memset(v, 0, sizeof *v); return v; }
struct RPQuad *rp_quad_construct(struct RPQuad *q) { memset(q, 0, sizeof *q); return q; }
struct RPQuad *renderer_new_quad(struct Renderer *r) { assert(r == &renderer); return &frame_quad; }
struct TexDrawVertex *renderer_new_tex_draw_vertices(struct Renderer *r, int count)
{ assert(r == &renderer && count == 3); return frame_vertices; }
void renderer_add_prim(struct Renderer *r, struct RPQuad *q, int flags)
{
    assert(r == &renderer && !flags && q->nverts == 3 && q->texture);
    assert(submissions < TEST_MESH_COUNT * 4);
    memcpy(captured[submissions++], q->verts, sizeof frame_vertices);
}

static void assert_clean(void)
{
    int i;
    assert(!outstanding); sdk_assert_empty();
    for (i = 0; i < 5; i++) assert(!fixtures[i].resource.res.lockCount);
}

static void test_create_failures(void)
{
    struct GrannyModelState *model;
    unsigned i, count, before;
    int meshes;
    assert(!granny_model_state_create(NULL, NULL));
    for (meshes = 0; meshes <= TEST_MESH_COUNT; meshes += TEST_MESH_COUNT) {
        fixture_init(&fixtures[0], meshes);
        before = allocation_calls;
        model = granny_model_state_create(&fixtures[0].resource, NULL); assert(model);
        count = allocation_calls - before;
        assert(model->mesh_count == meshes && fixtures[0].resource.res.lockCount == 1);
        granny_model_state_destroy(model); assert_clean();
        for (i = 1; i <= count; i++) {
            fail_at = allocation_calls + i;
            assert(!granny_model_state_create(&fixtures[0].resource, NULL));
            fail_at = 0; assert_clean();
        }
        before = sdk_calls;
        model = granny_model_state_create(&fixtures[0].resource, NULL); assert(model);
        count = sdk_calls - before;
        granny_model_state_destroy(model); assert_clean();
        for (i = 1; i <= count; i++) {
            sdk_fail_at = sdk_calls + i;
            assert(!granny_model_state_create(&fixtures[0].resource, NULL));
            sdk_fail_at = 0; assert_clean();
        }
    }
    fixtures[0].model.meshCount = -1;
    assert(!granny_model_state_create(&fixtures[0].resource, NULL));
    fixtures[0].model.meshCount = INT_MAX;
    assert(!granny_model_state_create(&fixtures[0].resource, NULL));
    fixtures[0].model.meshCount = 1;
    fixtures[0].data[0].index_count = -1;
    assert(!granny_model_state_create(&fixtures[0].resource, NULL));
    fixtures[0].data[0].index_count = 1;
    model = granny_model_state_create(&fixtures[0].resource, NULL);
    assert(model && !model->meshes[0].projection && model->meshes[0].binding);
    granny_model_state_destroy(model); assert_clean();
    fixtures[0].data[0].index_count = 5;
    model = granny_model_state_create(&fixtures[0].resource, NULL);
    assert(model && model->meshes[0].projection);
    granny_model_state_destroy(model); assert_clean();
    fixtures[0].data[0].index_count = 0; fixtures[0].data[0].vertex_count = 0;
    model = granny_model_state_create(&fixtures[0].resource, NULL); assert(model);
    assert(!model->meshes[0].projection && !model->meshes[0].vertices && !model->meshes[0].binding);
    granny_model_state_destroy(model); assert_clean();
}

static void test_replacement(void)
{
    struct C3dGrannyGameActor actor = {0};
    struct GrannyCorpse3d corpse = {0};
    struct GrannyModelState *old;
    unsigned i, calls, before, old_allocations;
    int j;
    for (j = 0; j < 5; j++) fixture_init(&fixtures[j], TEST_MESH_COUNT);
    loaded_resource = &fixtures[0].resource;
    c3d_granny_game_actor_set_job_model((struct GameActor *)&actor, 1, 0); assert(actor.model);
    old = actor.model; before = allocation_calls;
    c3d_granny_game_actor_set_job_model((struct GameActor *)&actor, 2, 1); assert(actor.model != old);
    calls = allocation_calls - before; old = actor.model; old_allocations = outstanding;
    for (i = 1; i <= calls; i++) {
        fail_at = allocation_calls + i;
        c3d_granny_game_actor_set_job_model((struct GameActor *)&actor, 3, 0); fail_at = 0;
        assert(actor.model == old && actor.job == 2 && actor.sex == 1 && outstanding == old_allocations);
    }
    c3d_granny_game_actor_set_job_model((struct GameActor *)&actor, 0x49d, 0);
    assert(actor.model->bounds.half_width == 30.0f && fixtures[0].resource.bounds.half_width == 10.0f);
    c3d_granny_game_actor_set_job_model((struct GameActor *)&actor, 2, 0);
    assert(actor.model->bounds.half_width == 10.0f);
    granny_model_state_destroy(actor.model); assert_clean();
    granny_corpse3d_load_model(&corpse); assert(corpse.model);
    granny_corpse3d_load_model(&corpse);
    granny_corpse3d_free_model(&corpse); granny_corpse3d_free_model(&corpse); assert_clean();

}

static void test_pose(void)
{
    struct GrannyModelState *model, *other;
    granny_world_pose *pose;
    fixture_init(&fixtures[0], TEST_MESH_COUNT);
    model = granny_model_state_create(&fixtures[0].resource, NULL); assert(model);
    assert(granny_playback_play(model->playback, 2, 0));
    pose = granny_playback_pose(model->playback, 0.25f);
    assert(pose == granny_playback_world_pose(model->playback));
    other = granny_model_state_create(&fixtures[0].resource, NULL); assert(other);
    granny_playback_pose(other->playback, 90.0f);
    assert(pose->time == 0.25f);
    granny_model_state_destroy(other);
    fixture_init(&fixtures[2], TEST_MESH_COUNT);
    other = granny_model_state_create(&fixtures[2].resource, model->resource); assert(other);
    assert(!other->playback);
    assert(granny_model_prepare_mesh(other, 0, pose, NULL));
    assert(other->meshes[0].matrix[9] == 0.25f);
    assert(granny_model_prepare_mesh(other, 1, pose, NULL));
    assert(other->meshes[1].vertices->vertices[0].pos.x == 1.25f);
    assert(!granny_model_prepare_mesh(other, 0, NULL, NULL));
    assert(!other->meshes[0].texture && !other->meshes[0].triangles);
    granny_model_state_destroy(other);
    granny_model_state_destroy(model); assert_clean();
}

int main(void)
{
    test_create_failures();
    test_replacement();
    test_pose();
    puts("Granny models: 65 meshes, complete allocation/SDK rollback, atomic actor model replacement, owned bindings and poses passed");
    return 0;
}
