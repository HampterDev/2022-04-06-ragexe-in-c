#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "resources/granny_res.h"
#include "render/gfx.h"
#include "render/skin_layout.h"

/* custom: the meshes that draw triangles, one behind the other: each mesh's vertices packed for its
   skeleton, its triangles' indices (whole triangles, as the original drew them) with its first
   vertex added, so that a draw may run on from one mesh into the next */
int granny_model_res_skin(struct C3dGrannyModelRes *self)
{
    struct SkinSource *sources = NULL;
    struct SkinVertex *vertices = NULL;
    uint32_t *indices = NULL;
    uint16_t *meshIndices = NULL;
    size_t vertexCount = 0, indexCount = 0, most = 0;
    int bones, i, ok = 0;

    if (!self->model || !self->skeleton)
        return 1;
    bones = self->skeleton->BoneCount;
    if (bones < 0 || bones > SKIN_BONES_MAX)
        return 1;
    for (i = 0; i < self->model->meshCount; i++) {
        struct GrannyResMesh *m = &self->model->meshes[i];
        size_t v = (size_t)_GrannyGetMeshVertexCount(m->mesh), all = (size_t)_GrannyGetMeshIndexCount(m->mesh);
        size_t n = all / 3 * 3;

        if (!n)
            continue;
        if (vertexCount > 0xffffffffu - v || indexCount > 0xffffffffu - n)
            return 1;
        vertexCount += v;
        indexCount += n;
        if (v > most)
            most = v;
        if (all > most)
            most = all;
    }
    if (!indexCount)
        return 1;
    if (vertexCount > SIZE_MAX / sizeof *vertices || indexCount > SIZE_MAX / sizeof *indices
        || most > SIZE_MAX / sizeof *sources)
        return 0;
    vertices = malloc(vertexCount * sizeof *vertices);
    indices = malloc(indexCount * sizeof *indices);
    sources = malloc(most * sizeof *sources);
    meshIndices = malloc(most * sizeof *meshIndices);
    if (!vertices || !indices || !sources || !meshIndices)
        goto done;
    vertexCount = indexCount = 0;
    for (i = 0; i < self->model->meshCount; i++) {
        struct GrannyResMesh *m = &self->model->meshes[i];
        int v = _GrannyGetMeshVertexCount(m->mesh), n = _GrannyGetMeshIndexCount(m->mesh), k;

        if (n < 3)
            continue;
        _GrannyCopyMeshSkinVertices(m->mesh, 0, v, sources);
        for (k = 0; k < v; k++)
            skin_pack_vertex(&sources[k], m->rigidBone, bones, &vertices[vertexCount + (size_t)k]);
        _GrannyCopyMeshIndices(m->mesh, 2, meshIndices);
        if (!skin_pack_indices(meshIndices, (uint32_t)(n / 3 * 3), (uint32_t)v, (uint32_t)vertexCount,
                               indices + indexCount))
            goto done;
        m->firstIndex = (uint32_t)indexCount;
        m->indexCount = (uint32_t)(n / 3 * 3);
        vertexCount += (size_t)v;
        indexCount += m->indexCount;
    }
    self->skin = gfx_create_skin(vertices, (uint32_t)vertexCount, indices, (uint32_t)indexCount);
    ok = self->skin != NULL;
done:
    free(vertices);
    free(indices);
    free(sources);
    free(meshIndices);
    return ok;
}

int granny_mesh_vertex_position(const struct C3dGrannyModelRes *model, int mesh, granny_world_pose *pose,
                               int vertex, Vec3 *out)
{
    const struct GrannyResMesh *m;
    struct SkinSource source;
    struct SkinVertex v;
    float at[3];
    int bones;

    if (!model || !model->model || !model->skeleton || !pose || mesh < 0 || mesh >= model->model->meshCount
        || vertex < 0 || !out)
        return 0;
    m = &model->model->meshes[mesh];
    bones = model->skeleton->BoneCount;
    if (vertex >= _GrannyGetMeshVertexCount(m->mesh) || bones < 0 || bones > SKIN_BONES_MAX)
        return 0;
    _GrannyCopyMeshSkinVertices(m->mesh, vertex, 1, &source);
    skin_pack_vertex(&source, m->rigidBone, bones, &v);
    skin_vertex_position(&v, _GrannyGetWorldPoseComposite4x4Array(pose), (uint32_t)bones, at);
    out->x = at[0];
    out->y = at[1];
    out->z = at[2];
    return 1;
}
