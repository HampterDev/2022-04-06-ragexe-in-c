#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "fileio/file_api.h"
#include "render/anim_mesh.h"
#include "resources/model_mesh.h"
#include "std/std.h"

static int read_count(struct File *file, int *count, size_t elementSize)
{
    return file_read(file, count, sizeof *count) && *count >= 0 &&
           (size_t)*count <= (size_t)INT_MAX / elementSize;
}

static int read_face(struct File *file, struct AnimMeshFace *face, int major, int minor)
{
    unsigned char record[20];
    int bytes = 24;
    if (major == 1 && minor == 1) bytes = 20;
    if (major == 2 && minor >= 2 &&
        (!file_read(file, &bytes, sizeof bytes) || bytes < 20)) return 0;
    if (!file_read(file, record, sizeof record)) return 0;
    memcpy(face->v, record, sizeof face->v);
    memcpy(face->tv, record + 6, sizeof face->tv);
    memcpy(&face->texture, record + 12, sizeof face->texture);
    memcpy(&face->twoSided, record + 16, sizeof face->twoSided);
    face->groupCount = (size_t)(bytes - 20) / sizeof *face->groups;
    if (major == 1 && minor == 1) face->groupCount = 1;
    if (face->groupCount) {
        face->groups = calloc(face->groupCount, sizeof *face->groups);
        if (!face->groups) return 0;
        if (!(major == 1 && minor == 1) &&
            !file_read(file, face->groups, (int)(face->groupCount * sizeof *face->groups)))
            return 0;
    }
    return file_read(file, record, (bytes - 20) % 4);
}

static unsigned char face_texture_less(const void *a, const void *b, void *unused)
{
    (void)unused;
    return ((const struct AnimMeshFace *)a)->texture < ((const struct AnimMeshFace *)b)->texture;
}

struct AnimMesh *model_mesh_read(struct File *file, int major, int minor,
                                const float *matrix)
{
    struct AnimMesh *mesh = calloc(1, sizeof *mesh);
    int i, k;
    if (!mesh) return NULL;
    if (!read_count(file, &mesh->vertexCount, sizeof *mesh->positions)) goto fail;
    if (mesh->vertexCount) {
        mesh->positions = calloc((size_t)mesh->vertexCount, sizeof *mesh->positions);
        if (!mesh->positions || !file_read(file, mesh->positions,
                mesh->vertexCount * (int)sizeof *mesh->positions)) goto fail;
    }
    if (matrix) {
        for (i = 0; i < mesh->vertexCount; ++i) {
            Vec3 *v = &mesh->positions[i];
            float x = v->x, y = v->y, z = v->z;
            v->x = x * matrix[0] + y * matrix[3] + z * matrix[6] + matrix[9];
            v->y = x * matrix[1] + y * matrix[4] + z * matrix[7] + matrix[10];
            v->z = x * matrix[2] + y * matrix[5] + z * matrix[8] + matrix[11];
        }
    }
    if (!read_count(file, &mesh->texVertCount, sizeof *mesh->texVerts)) goto fail;
    if (mesh->texVertCount) {
        mesh->texVerts = calloc((size_t)mesh->texVertCount, sizeof *mesh->texVerts);
        if (!mesh->texVerts) goto fail;
        if (major == 1 && minor == 1) {
            for (i = 0; i < mesh->texVertCount; ++i) {
                float uv[2];
                if (!file_read(file, uv, sizeof uv)) goto fail;
                mesh->texVerts[i].colour = 0xffffffffu;
                mesh->texVerts[i].u = uv[0];
                mesh->texVerts[i].v = uv[1];
            }
        } else if (!file_read(file, mesh->texVerts,
                       mesh->texVertCount * (int)sizeof *mesh->texVerts)) goto fail;
    }
    if (!read_count(file, &mesh->faceCount, sizeof *mesh->faces) ||
        mesh->faceCount > INT_MAX / 3) goto fail;
    if (mesh->faceCount) {
        mesh->faces = calloc((size_t)mesh->faceCount, sizeof *mesh->faces);
        if (!mesh->faces) goto fail;
        for (i = 0; i < mesh->faceCount; ++i) {
            struct AnimMeshFace *face = &mesh->faces[i];
            if (!read_face(file, face, major, minor)) goto fail;
            for (k = 0; k < 3; ++k)
                if (face->v[k] >= mesh->vertexCount ||
                    (mesh->texVertCount && face->tv[k] >= mesh->texVertCount)) goto fail;
        }
        std_sort(mesh->faces, mesh->faces + mesh->faceCount, sizeof *mesh->faces,
                 face_texture_less, NULL);
    }
    if (!anim_mesh_smooth_normals(mesh)) goto fail;
    return mesh;
fail:
    anim_mesh_destroy(mesh);
    return NULL;
}
