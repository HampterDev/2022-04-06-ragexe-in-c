#include "base.h"
#include "world/model.h"
#include "world/model_node_util.h"
#include "game/context.h"

#include <math.h>
#include <string.h>

static void c3d_node_light_from_ground(struct C3dNode *self, const float *m, struct C3dGround15 *ground)
{
    struct AnimMesh *mesh = self->mesh;
    int f, k;

    for (f = 0; f < mesh->faceCount; f++) {
        const struct AnimMeshFace *face = &mesh->faces[f];
        const struct FaceInfo *info = (const struct FaceInfo *)self->faceInfo + f;

        for (k = 0; k < 3; k++) {
            int rgb[3] = {0, 0, 0};
            int *li = self->groundLight + f * 9 + k * 3;

            if (ground) {
                const Vec3 *v = &mesh->positions[face->v[k]];
                const float *pos = self->actor->pos;
                float x = v->x * m[0] + v->y * m[3] + v->z * m[6] + m[9] + pos[0];
                float z = v->x * m[2] + v->y * m[5] + v->z * m[8] + m[11] + pos[2];

                c3d_ground15_get_cell_colour(ground, x, z, rgb);
            }
            li[0] = rgb[0];
            li[1] = rgb[1];
            li[2] = rgb[2];
        }
    }
}

static float clamp1(float x)
{
    return x < 1.0f ? x : 1.0f;
}

static float lit(const float *r, const Vec3 *n, const float *dir)
{
    float x = n->x * r[0] + n->y * r[3] + n->z * r[6] + 0.0f;
    float y = n->x * r[1] + n->y * r[4] + n->z * r[7] + 0.0f;
    float z = n->x * r[2] + n->y * r[5] + n->z * r[8] + 0.0f;
    float inv = 1.0f / sqrtf(x * x + y * y + z * z);
    float d = x * inv * dir[0] + y * inv * dir[1] + z * inv * dir[2];

    return d > 0.0f ? d : 0.0f;
}

void c3d_node_light(struct C3dNode *self, const float *parent, const float *dir,
                              const float *diffuse, const float *ambient, int shadeType)
{
    float m[12], r[12];
    struct AnimMesh *mesh = self->mesh;
    DWORD *buf = g_nodeCornerColours;
    struct AnimMeshFace *face, *end;
    struct FaceInfo *info;
    size_t i;
    struct World *world;
    int f, k;

    mat_multiply(m, self->matrix, parent);
    memcpy(r, m, sizeof(r));
    switch (shadeType) {
    case 0:
        for (f = 0; f < mesh->faceCount; f++)
            for (k = 0; k < 3; k++)
                *buf++ = 0x00ffffff | (DWORD)(BYTE)(int)(self->alpha * 255.0f) << 24;
        break;
    case 1:
        for (f = 0; f < mesh->faceCount; f++) {
            const struct AnimMeshFace *fc = &mesh->faces[f];
            float d = lit(r, &mesh->faceNormals[f], dir);
            float cr = clamp1(diffuse[0] * d + ambient[0]);
            float cg = clamp1(d * diffuse[1] + ambient[1]);
            float cb = clamp1(diffuse[2] * d + ambient[2]);

            for (k = 0; k < 3; k++) {
                BYTE a = (BYTE)(int)(self->alpha * 255.0f);
                float s = mesh->texVertCount && mesh->texVerts[fc->tv[k]].colour != 0 ? 128.0f : 255.0f;

                *buf++ = pack(a, (BYTE)(int)(cr * s), (BYTE)(int)(cg * s), (BYTE)(int)(cb * s));
            }
        }
        break;
    case 2:
        for (f = 0; f < mesh->faceCount; f++) {
            const struct AnimMeshFace *fc = &mesh->faces[f];

            for (k = 0; k < 3; k++) {
                float d = lit(r, &mesh->vertexNormals[f * 3 + k], dir);
                BYTE a = (BYTE)(int)(self->alpha * 255.0f);
                float s = (float)(mesh->texVertCount && mesh->texVerts[fc->tv[k]].colour != 0 ? 128 : 255);

                *buf++ = pack(a, (BYTE)(int)(clamp1(diffuse[0] * d + ambient[0]) * s),
                              (BYTE)(int)(clamp1(d * diffuse[1] + ambient[1]) * s),
                              (BYTE)(int)(clamp1(diffuse[2] * d + ambient[2]) * s));
            }
        }
        break;
    }

    face = mesh->faces;
    end = face ? face + mesh->faceCount : face;
    info = self->faceInfo;
    buf = g_nodeCornerColours;
    while (face != end) {
        WORD texture = face->texture;
        struct Texture *t = run_texture(self, texture);
        float su = unsigned_to_float(t->reqWidth) / unsigned_to_float(t->base.width);
        float sv = unsigned_to_float(t->reqHeight) / unsigned_to_float(t->base.height);

        for (; face != end && face->texture == texture; face++, info++, buf += 3) {
            info->colour[0] = buf[0];
            info->colour[1] = buf[1];
            info->colour[2] = buf[2];
            if (self->mesh->texVertCount == 0) {
                for (k = 0; k < 3; k++) {
                    info->uv[k][0] = su;
                    info->uv[k][1] = sv;
                }
            } else {
                for (k = 0; k < 3; k++) {
                    const struct AnimMeshTexVertex *tv = &mesh->texVerts[face->tv[k]];
                    info->uv[k][0] = tv->u * su;
                    info->uv[k][1] = tv->v * sv;
                }
            }
        }
    }

    world = game_current_world();
    c3d_node_light_from_ground(self, m, world ? world->ground : NULL);
    for (i = 0; i < self->childCount; ++i)
        c3d_node_light(self->children[i], m, dir, diffuse, ambient, shadeType);
}
