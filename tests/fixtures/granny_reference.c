/* The CPU code Granny's actors were drawn with before the device skinned them, as it was (see
   granny_reference.h): kept as the reference the skinning shader is compared with. */
#include "granny_reference.h"
#include "render/math3d.h"
#include "render/projection.h"
#include "render/renderer.h"
#include <string.h>

/* resources/granny_gltf.c, _GrannyDeformVertices: position and normal skinned by up to four
   composite bone matrices (the mesh binding's bones are the skeleton's own); texture coordinates
   copied */
static void deform(const struct SkinSource *v, int count, const float *composite, struct GrannyReferenceVertex *out)
{
    float *o = (float *)out;
    int i, k;
    for (i = 0; i < count; i++, v++, o += 8) {
        float p[3] = {0, 0, 0}, n[3] = {0, 0, 0};
        for (k = 0; k < 4; k++) {
            const float *m;
            float w = v->weights[k];
            if (w == 0.0f)
                continue;
            m = composite + v->bones[k] * 16;
            p[0] += w * (v->position[0] * m[0] + v->position[1] * m[4] + v->position[2] * m[8] + m[12]);
            p[1] += w * (v->position[0] * m[1] + v->position[1] * m[5] + v->position[2] * m[9] + m[13]);
            p[2] += w * (v->position[0] * m[2] + v->position[1] * m[6] + v->position[2] * m[10] + m[14]);
            n[0] += w * (v->normal[0] * m[0] + v->normal[1] * m[4] + v->normal[2] * m[8]);
            n[1] += w * (v->normal[0] * m[1] + v->normal[1] * m[5] + v->normal[2] * m[9]);
            n[2] += w * (v->normal[0] * m[2] + v->normal[1] * m[6] + v->normal[2] * m[10]);
        }
        memcpy(o, p, sizeof p);
        memcpy(o + 3, n, sizeof n);
        memcpy(o + 6, v->uv, sizeof v->uv);
    }
}

void granny_reference_mesh(const struct SkinSource *src, int count, int rigidBone, const float *composite,
                           struct GrannyReferenceVertex *out, float *matrix)
{
    int i, row;
    if (rigidBone >= 0) {
        /* render_mesh's rigid branch: the bone's composite as the mesh's matrix, the vertices as
           _GrannyCopyMeshVertices copied them (PNT332) */
        const float *c = composite + rigidBone * 16;
        for (row = 0; row < 4; row++) {
            matrix[row * 3] = c[row * 4];
            matrix[row * 3 + 1] = c[row * 4 + 1];
            matrix[row * 3 + 2] = c[row * 4 + 2];
        }
        for (i = 0; i < count; i++) {
            out[i].pos.x = src[i].position[0];
            out[i].pos.y = src[i].position[1];
            out[i].pos.z = src[i].position[2];
            out[i].nx = src[i].normal[0];
            out[i].ny = src[i].normal[1];
            out[i].nz = src[i].normal[2];
            out[i].u = src[i].uv[0];
            out[i].v = src[i].uv[1];
        }
        return;
    }
    deform(src, count, composite, out);
    memcpy(matrix, &g_identityMatrix, sizeof g_identityMatrix.m);
}

static __inline float light_term(float d)
{
    float t = 1.0f < d ? 1.0f : d;

    return 0.0f > t ? 0.0f : t;
}

static void world_corner(float *dst, const Vec3 *pt, const float *toWorld)
{
    dst[0] = pt->x * toWorld[0] + pt->y * toWorld[3] + pt->z * toWorld[6] + toWorld[9];
    dst[1] = pt->x * toWorld[1] + pt->y * toWorld[4] + pt->z * toWorld[7] + toWorld[10];
    dst[2] = pt->x * toWorld[2] + pt->y * toWorld[5] + pt->z * toWorld[8] + toWorld[11];
}

void granny_reference_corners(struct Renderer *renderer, int kind, const struct GrannyReferenceVertex *v,
                              const unsigned short *indices, int tris, const float *mesh, const float *r,
                              const float *view, const struct GrannyReferenceLight *light, unsigned char alpha,
                              struct TexDrawVertex *out, float (*world)[3], float *light_out)
{
    float m1[12], m2[12], inv[12], lx, ly, lz, k;
    int t, j, n = 0;
    if (kind == GRANNY_REFERENCE_EMBLEM) {
        /* the guild flag: projected through its model matrix alone, lit by the view's direction
           taken through the view */
        memcpy(m1, r, sizeof m1);
        mat_multiply(m2, r, view);
        lx = ((view[0] * light->viewDir.x + view[3] * light->viewDir.y) + view[6] * light->viewDir.z) + view[9];
        ly = ((view[1] * light->viewDir.x + view[4] * light->viewDir.y) + view[7] * light->viewDir.z) + view[10];
        lz = ((view[2] * light->viewDir.x + view[5] * light->viewDir.y) + view[8] * light->viewDir.z) + view[11];
        k = 1.0f / float_sqrt((ly * ly + lx * lx) + lz * lz);
        lx = lx * k;
        ly = ly * k;
        lz = lz * k;
    } else {
        mat_multiply(m1, mesh, r);
        mat_multiply(m2, m1, view);
        mat_invert(inv, view);
        if (kind == GRANNY_REFERENCE_CORPSE) {
            lx = ((inv[0] * 0.0f - inv[3] * 300.0f) + inv[6] * 0.0f) + inv[9];
            ly = ((inv[1] * 0.0f - inv[4] * 300.0f) + inv[7] * 0.0f) + inv[10];
            lz = ((inv[2] * 0.0f - inv[5] * 300.0f) + inv[8] * 0.0f) + inv[11];
            k = 1.0f / float_sqrt((ly * ly + lx * lx) + lz * lz);
            lx = lx * k;
            ly = ly * k;
            lz = lz * k;
        } else {
            lx = ((light->sun.x * inv[0] + light->sun.y * inv[3]) + light->sun.z * inv[6]) + inv[9];
            ly = ((light->sun.x * inv[1] + inv[4] * light->sun.y) + light->sun.z * inv[7]) + inv[10];
            lz = ((inv[2] * light->sun.x + inv[5] * light->sun.y) + light->sun.z * inv[8]) + inv[11];
            k = 1.0f / float_sqrt((ly * ly + lx * lx) + lz * lz);
            lx = k * lx;
            ly = k * ly;
            lz = k * lz;
        }
    }
    light_out[0] = lx;
    light_out[1] = ly;
    light_out[2] = lz;
    for (t = 0; t < tris; t++) {
        for (j = 0; j < 3; j++) {
            struct TexDrawVertex *dst = out + n;
            const struct GrannyReferenceVertex *src = &v[indices[n]];
            Vec3 pt;
            unsigned b;

            pt = src->pos;
            renderer_project_vertex(renderer, &pt, m2, dst);
            world_corner(world[n], &pt, m1);
            n++;
            memcpy(&dst->u, &src->u, sizeof dst->u);
            memcpy(&dst->v, &src->v, sizeof dst->v);
            if (kind == GRANNY_REFERENCE_LIT) {
                int a = light->intensity;
                b = (unsigned)(light_term((src->nx * lx + src->ny * ly) + src->nz * lz) * (float)a + (float)(0xff - a));
                dst->diffuse = (((light->g + 0x38 + b) << 8) & 0xff00) + (((light->r + 0x38 + b) << 16) & 0xff0000) +
                               (unsigned char)(light->b + 0x38 + b) + ((unsigned)alpha << 24);
            } else {
                b = (unsigned)(light_term((src->nx * lx + src->ny * ly) + src->nz * lz) * 100.0f + 155.0f);
                dst->diffuse = (unsigned char)b * 0x10101u + ((unsigned)alpha << 24);
            }
        }
    }
}
