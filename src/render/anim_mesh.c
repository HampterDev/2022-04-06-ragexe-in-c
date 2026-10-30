#include <limits.h>
#include <stdlib.h>
#include "render/anim_mesh.h"
#include "render/math3d.h"

void anim_mesh_destroy(struct AnimMesh *mesh)
{
    int i;
    if (!mesh) return;
    if (mesh->faces)
        for (i = mesh->faceCount; i-- > 0;)
            free(mesh->faces[i].groups);
    free(mesh->faces);
    free(mesh->positions);
    free(mesh->texVerts);
    free(mesh->faceNormals);
    free(mesh->vertexNormals);
    free(mesh);
}

static void vec3_normalise(Vec3 *v)
{
    float inv = (float)(1.0 / (double)float_sqrt(v->x * v->x + v->y * v->y + v->z * v->z));
    v->x *= inv;
    v->y *= inv;
    v->z *= inv;
}

struct VertexFaces {
    size_t begin, count;
};

int anim_mesh_smooth_normals(struct AnimMesh *mesh)
{
    struct VertexFaces *vertices = NULL;
    int *adjacent = NULL;
    Vec3 *faceNormals = NULL, *vertexNormals = NULL;
    size_t corners, offset = 0;
    int f, k, i;

    if (!mesh || mesh->vertexCount < 0 || mesh->faceCount < 0 ||
        mesh->faceCount > INT_MAX / 3 ||
        (mesh->vertexCount && !mesh->positions) || (mesh->faceCount && !mesh->faces))
        return 0;
    corners = (size_t)mesh->faceCount * 3;
    if (corners > SIZE_MAX / sizeof *vertexNormals ||
        (size_t)mesh->vertexCount > SIZE_MAX / sizeof *vertices)
        return 0;
    for (f = 0; f < mesh->faceCount; ++f) {
        if (mesh->faces[f].groupCount && !mesh->faces[f].groups) return 0;
        for (k = 0; k < 3; ++k)
            if (mesh->faces[f].v[k] >= mesh->vertexCount) return 0;
    }
    if (mesh->faceCount) {
        faceNormals = calloc((size_t)mesh->faceCount, sizeof *faceNormals);
        vertexNormals = calloc(corners, sizeof *vertexNormals);
        vertices = calloc((size_t)mesh->vertexCount, sizeof *vertices);
        adjacent = malloc(corners * sizeof *adjacent);
        if (!faceNormals || !vertexNormals || !vertices || !adjacent) goto fail;
    }

    for (f = 0; f < mesh->faceCount; ++f) {
        const struct AnimMeshFace *face = &mesh->faces[f];
        const Vec3 *v0 = &mesh->positions[face->v[0]];
        const Vec3 *v1 = &mesh->positions[face->v[1]];
        const Vec3 *v2 = &mesh->positions[face->v[2]];
        Vec3 *n = &faceNormals[f];
        float ax = v0->x - v2->x, ay = v0->y - v2->y, az = v0->z - v2->z;
        float bx = v1->x - v2->x, by = v1->y - v2->y, bz = v1->z - v2->z;

        n->x = by * az - bz * ay;
        n->y = ax * bz - bx * az;
        n->z = bx * ay - ax * by;
        vec3_normalise(n);
        for (k = 0; k < 3; ++k) ++vertices[face->v[k]].count;
    }
    if (mesh->faceCount) {
        for (i = 0; i < mesh->vertexCount; ++i) {
            vertices[i].begin = offset;
            offset += vertices[i].count;
            vertices[i].count = 0;
        }
    }
    for (f = 0; f < mesh->faceCount; ++f)
        for (k = 0; k < 3; ++k) {
            struct VertexFaces *list = &vertices[mesh->faces[f].v[k]];
            adjacent[list->begin + list->count++] = f;
        }

    for (f = 0; f < mesh->faceCount; ++f) {
        const struct AnimMeshFace *face = &mesh->faces[f];
        for (k = 0; k < 3; ++k) {
            const struct VertexFaces *list = &vertices[face->v[k]];
            size_t g, a, b;
            for (g = list->begin; g < list->begin + list->count; ++g) {
                const struct AnimMeshFace *other = &mesh->faces[adjacent[g]];
                for (a = 0; a < face->groupCount; ++a) {
                    for (b = 0; b < other->groupCount; ++b)
                        if (other->groups[b] == face->groups[a]) break;
                    if (b != other->groupCount) {
                        Vec3 *vn = &vertexNormals[3 * f + k];
                        const Vec3 *fn = &faceNormals[adjacent[g]];
                        vn->x = vn->x + fn->x;
                        vn->y = fn->y + vn->y;
                        vn->z = fn->z + vn->z;
                    }
                }
            }
        }
    }
    for (i = 0; i < mesh->faceCount * 3; ++i) vec3_normalise(&vertexNormals[i]);
    free(vertices);
    free(adjacent);
    free(mesh->faceNormals);
    free(mesh->vertexNormals);
    mesh->faceNormals = faceNormals;
    mesh->vertexNormals = vertexNormals;
    return 1;

fail:
    free(vertices);
    free(adjacent);
    free(faceNormals);
    free(vertexNormals);
    return 0;
}
