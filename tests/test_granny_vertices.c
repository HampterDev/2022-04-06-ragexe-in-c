/* A Granny model's meshes as the device takes them (resources/granny_mesh.c,
   granny_model_res_skin): every mesh with a triangle one behind the other, its vertices packed for
   the skeleton (a rigid mesh's on its bone), its whole triangles' indices with its first vertex
   added; a mesh without a triangle left out; nothing for a skeleton the device does not take; an
   index past its mesh, and every allocation that fails, failing it with nothing left behind. And
   the vertex an effect starts at (granny_mesh_vertex_position): where Granny's deformer put it in
   the pose, a rigid one where it lies, within bounds. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resources/granny_res.h"
#include "render/skin_layout.h"
#include "owned_storage_test_allocator.h"

#define malloc checked_malloc
#define free checked_free
#include "../src/resources/granny_mesh.c"
#undef malloc
#undef free

enum { MESHES = 4 };
static granny_mesh meshes[MESHES];
static struct GrannyResMesh resMeshes[MESHES];
static int vertexCounts[MESHES] = {5, 3, 4, 2}, indexCounts[MESHES] = {7, 2, 6, 3};
static unsigned short meshIndices[MESHES][8] = {{0, 1, 2, 4, 3, 2, 1}, {0, 1}, {3, 2, 1, 0, 1, 2}, {1, 0, 1}};
static float composite[3][16];
static granny_world_pose *const g_pose = (granny_world_pose *)7;
static int skins_live, fail_skin;
static struct SkinVertex made[16];
static uint32_t madeIndices[32], madeVertexCount, madeIndexCount;

static int mesh_number(const granny_mesh *mesh)
{
    int i = (int)(mesh - meshes);
    assert(i >= 0 && i < MESHES);
    return i;
}
int _GrannyGetMeshVertexCount(granny_mesh *mesh) { return vertexCounts[mesh_number(mesh)]; }
int _GrannyGetMeshIndexCount(granny_mesh *mesh) { return indexCounts[mesh_number(mesh)]; }
void _GrannyCopyMeshIndices(granny_mesh *mesh, int bytesPerIndex, void *out)
{
    int m = mesh_number(mesh);
    assert(bytesPerIndex == 2);
    memcpy(out, meshIndices[m], (size_t)indexCounts[m] * 2);
}
/* vertex i of mesh m: at (m, i, 1), its bones i, i + 1, i + 2, i + 3, weighted 0.5, 0.25, 0.25, 0 */
void _GrannyCopyMeshSkinVertices(granny_mesh *mesh, int first, int count, struct SkinSource *out)
{
    int m = mesh_number(mesh), i, k;
    assert(first >= 0 && count > 0 && first + count <= vertexCounts[m]);
    for (i = first; i < first + count; i++, out++) {
        memset(out, 0, sizeof *out);
        out->position[0] = (float)m;
        out->position[1] = (float)i;
        out->position[2] = 1.0f;
        out->normal[1] = 1.0f;
        out->uv[0] = (float)i / 8;
        for (k = 0; k < 4; k++)
            out->bones[k] = i + k;
        out->weights[0] = 0.5f;
        out->weights[1] = out->weights[2] = 0.25f;
    }
}
float *_GrannyGetWorldPoseComposite4x4Array(granny_world_pose *pose)
{
    assert(pose == g_pose);
    return &composite[0][0];
}
gfx_skin *gfx_create_skin(const struct SkinVertex *vertices, uint32_t vertexCount, const uint32_t *indices,
                          uint32_t indexCount)
{
    assert(vertexCount <= 16 && indexCount <= 32);
    if (fail_skin)
        return NULL;
    memcpy(made, vertices, vertexCount * sizeof *vertices);
    memcpy(madeIndices, indices, indexCount * sizeof *indices);
    madeVertexCount = vertexCount;
    madeIndexCount = indexCount;
    skins_live++;
    return (gfx_skin *)&skins_live;
}

static void model_init(struct C3dGrannyModelRes *res, struct GrannyResModel *model, granny_skeleton *skeleton)
{
    int i;
    memset(res, 0, sizeof *res);
    memset(model, 0, sizeof *model);
    memset(skeleton, 0, sizeof *skeleton);
    skeleton->BoneCount = 3;
    model->meshCount = MESHES;
    model->meshes = resMeshes;
    res->model = model;
    res->skeleton = skeleton;
    for (i = 0; i < MESHES; i++) {
        memset(&resMeshes[i], 0, sizeof resMeshes[i]);
        resMeshes[i].mesh = &meshes[i];
        resMeshes[i].rigidBone = i == 2 ? 2 : -1;
    }
}

static void skin_layout(void)
{
    struct C3dGrannyModelRes res;
    struct GrannyResModel model;
    granny_skeleton skeleton;
    unsigned start, calls, i;

    model_init(&res, &model, &skeleton);
    start = allocation_calls;
    assert(granny_model_res_skin(&res) && res.skin == (gfx_skin *)&skins_live && !outstanding);
    calls = allocation_calls - start;
    /* mesh 0: 5 vertices, two whole triangles of its 7 indices; mesh 1 has none and is left out;
       mesh 2 (rigid, bone 2) after mesh 0's vertices; mesh 3 after mesh 2's */
    assert(madeVertexCount == 5 + 4 + 2 && madeIndexCount == 6 + 6 + 3);
    assert(resMeshes[0].firstIndex == 0 && resMeshes[0].indexCount == 6);
    assert(!resMeshes[1].indexCount);
    assert(resMeshes[2].firstIndex == 6 && resMeshes[2].indexCount == 6);
    assert(resMeshes[3].firstIndex == 12 && resMeshes[3].indexCount == 3);
    for (i = 0; i < 6; i++) {
        assert(madeIndices[i] == meshIndices[0][i]);
        assert(madeIndices[6 + i] == 5u + meshIndices[2][i]);
    }
    assert(madeIndices[12] == 9 + 1 && madeIndices[13] == 9 && madeIndices[14] == 9 + 1);
    /* a skinned vertex keeps its weights, its bones past the skeleton's three taken as 0 */
    assert(made[1].position[0] == 0.0f && made[1].position[1] == 1.0f && !made[1].flags);
    assert(made[1].bones[0] == 1 && made[1].bones[1] == 2 && made[1].bones[2] == 0 && made[1].bones[3] == 0);
    assert(made[1].weights[0] == 0.5f && made[1].weights[2] == 0.25f && made[1].weights[3] == 0.0f);
    assert(made[1].uv[0] == 1.0f / 8 && made[1].normal[1] == 1.0f);
    /* the rigid mesh's on its bone alone */
    for (i = 5; i < 9; i++) {
        assert(made[i].flags == SKIN_RIGID && made[i].bones[0] == 2 && made[i].weights[0] == 1.0f);
        assert(made[i].weights[1] == 0.0f && made[i].position[0] == 2.0f && made[i].position[1] == (float)(i - 5));
    }
    assert(made[9].position[0] == 3.0f && !made[9].flags);
    skins_live = 0;

    /* every allocation that fails, and the device's model, fails it with nothing left */
    for (i = 1; i <= calls; i++) {
        model_init(&res, &model, &skeleton);
        fail_at = allocation_calls + i;
        assert(!granny_model_res_skin(&res) && !res.skin && !outstanding);
        fail_at = 0;
    }
    model_init(&res, &model, &skeleton);
    fail_skin = 1;
    assert(!granny_model_res_skin(&res) && !res.skin && !outstanding);
    fail_skin = 0;
    /* an index past its mesh's vertices */
    model_init(&res, &model, &skeleton);
    meshIndices[2][4] = 4;
    assert(!granny_model_res_skin(&res) && !res.skin && !outstanding && !skins_live);
    meshIndices[2][4] = 1;
    /* a skeleton the device does not take, no triangle at all, no model: nothing made, and the load
       goes on */
    model_init(&res, &model, &skeleton);
    skeleton.BoneCount = SKIN_BONES_MAX + 1;
    assert(granny_model_res_skin(&res) && !res.skin && !resMeshes[0].indexCount);
    model_init(&res, &model, &skeleton);
    skeleton.BoneCount = -1;
    assert(granny_model_res_skin(&res) && !res.skin);
    model_init(&res, &model, &skeleton);
    indexCounts[0] = indexCounts[2] = indexCounts[3] = 2;
    assert(granny_model_res_skin(&res) && !res.skin);
    indexCounts[0] = 7;
    indexCounts[2] = 6;
    indexCounts[3] = 3;
    model_init(&res, &model, &skeleton);
    res.model = NULL;
    assert(granny_model_res_skin(&res) && !res.skin);
    assert(!skins_live && !outstanding);
}

static void effect_anchor(void)
{
    struct C3dGrannyModelRes res;
    struct GrannyResModel model;
    granny_skeleton skeleton;
    Vec3 at = {-1, -1, -1};
    int b;

    model_init(&res, &model, &skeleton);
    memset(composite, 0, sizeof composite);
    for (b = 0; b < 3; b++) {
        composite[b][0] = composite[b][5] = composite[b][10] = composite[b][15] = 1.0f;
        composite[b][12] = (float)(10 * (b + 1));
        composite[b][13] = (float)(100 * (b + 1));
    }
    /* vertex 1 of mesh 0, at (0, 1, 1), on bones 1 (0.5), 2 (0.25) and 0 (0.25, bone 3 taken as 0) */
    assert(granny_mesh_vertex_position(&res, 0, g_pose, 1, &at));
    assert(at.x == 0.5f * 20.0f + 0.25f * 30.0f + 0.25f * 10.0f);
    assert(at.y == 0.5f * 201.0f + 0.25f * 301.0f + 0.25f * 101.0f && at.z == 1.0f);
    /* a rigid mesh's where it lies */
    assert(granny_mesh_vertex_position(&res, 2, g_pose, 3, &at));
    assert(at.x == 2.0f && at.y == 3.0f && at.z == 1.0f);
    /* nothing past a mesh's vertices, the model's meshes or without a pose */
    at.x = 99.0f;
    assert(!granny_mesh_vertex_position(&res, 0, g_pose, 5, &at) && !granny_mesh_vertex_position(&res, 0, g_pose, -1, &at));
    assert(!granny_mesh_vertex_position(&res, MESHES, g_pose, 0, &at) && !granny_mesh_vertex_position(&res, -1, g_pose, 0, &at));
    assert(!granny_mesh_vertex_position(&res, 0, NULL, 0, &at) && !granny_mesh_vertex_position(NULL, 0, g_pose, 0, &at));
    assert(!granny_mesh_vertex_position(&res, 0, g_pose, 0, NULL));
    skeleton.BoneCount = SKIN_BONES_MAX + 1;
    assert(!granny_mesh_vertex_position(&res, 0, g_pose, 0, &at) && at.x == 99.0f);
}

int main(void)
{
    skin_layout();
    effect_anchor();
    puts("Granny vertices: meshes one behind the other for the device, whole triangles, rigid meshes on their bone, "
         "bounded bones and indices, allocation failures, and the effect's vertex in the pose passed.");
    return 0;
}
