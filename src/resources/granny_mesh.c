#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "resources/granny_res.h"
#include "render/math3d.h"

struct GrannyVertexBuffer *granny_vertex_buffer_create(const struct C3dGrannyModelRes *model, int mesh)
{
    struct GrannyVertexBuffer *buffer;
    int count;

    if (!model || !model->model || mesh < 0 || mesh >= model->model->meshCount)
        return NULL;
    count = _GrannyGetMeshVertexCount(model->model->meshes[mesh].mesh);
    if (count <= 0 || (size_t)count > (SIZE_MAX - sizeof *buffer) / sizeof buffer->vertices[0])
        return NULL;
    buffer = calloc(1, sizeof *buffer + (size_t)count * sizeof buffer->vertices[0]);
    if (buffer)
        buffer->count = (size_t)count;
    return buffer;
}

void granny_vertex_buffer_destroy(struct GrannyVertexBuffer *buffer)
{
    free(buffer);
}

int granny_mesh_vertex_position(const struct C3dGrannyModelRes *model, int mesh,
                               const struct GrannyMeshVertex *vertices, int vertex, Vec3 *out)
{
    if (!model || !model->model || mesh < 0 || mesh >= model->model->meshCount ||
        !vertices || !out || vertex < 0 ||
        vertex >= _GrannyGetMeshVertexCount(model->model->meshes[mesh].mesh))
        return 0;
    *out = vertices[vertex].pos;
    return 1;
}

const struct GrannyMeshVertex *c3d_granny_model_res_rigid_mesh_vertices(struct C3dGrannyModelRes *self, int mesh)
{
    struct GrannyResMesh *m;

    if (!self || !self->model || mesh < 0 || mesh >= self->model->meshCount)
        return NULL;
    m = &self->model->meshes[mesh];
    return _GrannyMeshIsRigid(m->mesh) ? m->rigidVerts : NULL;
}

static const struct GrannyMeshVertex *render_mesh(struct GrannyResModel *model, int mesh,
                         struct Texture **tex, unsigned short **indices, int *tris, float *matrix,
                         const struct GrannyMeshPose *pose, struct GrannyVertexBuffer *buffer)
{
    struct GrannyResMesh *m;
    const float *composite;
    const int *bones, *groups;
    int count, material;
    struct GrannyResTexture *t = NULL;

    *tex = NULL;
    *indices = NULL;
    *tris = 0;
    if (!model || mesh < 0 || mesh >= model->meshCount ||
        !pose || !pose->world || !pose->binding)
        return NULL;
    m = &model->meshes[mesh];
    count = _GrannyGetMeshVertexCount(m->mesh);
    if (count <= 0)
        return NULL;
    groups = _GrannyGetMeshTriangleGroups(m->mesh);
    if (groups) {
        material = groups[0];
        if (material >= 0 && material < m->materialCount && m->textures)
            t = m->textures[material];
    }
    composite = _GrannyGetWorldPoseComposite4x4Array(pose->world);
    bones = _GrannyGetMeshBindingToBoneIndices(pose->binding);
    if (_GrannyMeshIsRigid(m->mesh)) {
        const float *c = _GrannyGetWorldPoseComposite4x4(pose->world, bones[0]);
        const struct GrannyMeshVertex *verts;
        int row;

        for (row = 0; row < 4; row++) {
            matrix[row * 3] = c[row * 4];
            matrix[row * 3 + 1] = c[row * 4 + 1];
            matrix[row * 3 + 2] = c[row * 4 + 2];
        }
        verts = m->rigidVerts;
        if (!verts)
            return NULL;
        *tex = t ? t->texture : NULL;
        *indices = m->indices;
        *tris = _GrannyGetMeshIndexCount(m->mesh) / 3;
        if (m->mesh->Name && strstr(m->mesh->Name, "hair_pivot") && pose->hair)
            memcpy(pose->hair->matrix, matrix, sizeof pose->hair->matrix);
        return verts;
    }
    if (!m->deformer || !buffer || buffer->count < (size_t)count)
        return NULL;
    _GrannyDeformVertices(m->deformer, bones, composite, count, _GrannyGetMeshVertices(m->mesh), buffer->vertices);
    _GrannyGetMeshBytesPerIndex(m->mesh);
    _GrannyGetMeshIndices(m->mesh);
    *indices = m->indices;
    *tris = _GrannyGetMeshIndexCount(m->mesh) / 3;
    *tex = t ? t->texture : NULL;
    memcpy(matrix, &g_identityMatrix, sizeof g_identityMatrix.m);
    return buffer->vertices;
}

const struct GrannyMeshVertex *c3d_granny_model_res_mesh_to_draw(struct C3dGrannyModelRes *self, int mesh,
                                              struct Texture **tex, unsigned short **indices, int *tris,
                                              float *matrix, const struct GrannyMeshPose *pose,
                                              struct GrannyVertexBuffer *buffer)
{
    return render_mesh(self ? self->model : NULL, mesh, tex, indices, tris, matrix, pose, buffer);
}
