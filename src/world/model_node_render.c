#include "render/rp_pool.h"
#include "actor/weather.h"
#include "game/context.h"
#include "base.h"
#include "world/model.h"
#include "world/model_node_util.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "actor/attr_state.h"

/* custom: a node's faces as one static mesh the device projects (render/gfx.h, world/model_types.h),
   in place of the original's triangles, which the CPU projected each frame. A face's three
   corners keep their position, texture coordinates and what the original made their colour from:
   diffuse the node's own lit colour, with the share of the light it was lit with as its alpha
   (128 or 255 by the corner's texture vertex, as c3d_node_light takes it, 0 for a model lit
   without its light), colour2 the ground's light under the corner. Each frame a draw goes where
   the original sent its faces, with the colours, alpha and list it gave them: from the weather,
   the player's blindness, the ground lighting option, the actor's alpha, the texture's format
   and, on a version 2 node, its texture-coordinate animation, which turns the draw's coordinates
   as uv_turn_matrix puts it. In a shadow the device takes the light's direct part away from a
   corner's own colour (render/gfx.h). The device projects with the node's own matrix to view
   space and culls as the original did: a version 1 node keeps the faces that turn
   counter-clockwise on screen, a version 2 node the clockwise ones, the other way round when the
   actor is mirrored, and both sides of a twoSided face. Every draw also casts a shadow while the
   frame draws them. */

struct GpuFace {
    int face;
    int key[2];
};

static int gpu_face_less(const void *a, const void *b)
{
    const struct GpuFace *x = a, *y = b;
    int i;

    for (i = 0; i < 2; i++)
        if (x->key[i] != y->key[i])
            return x->key[i] < y->key[i] ? -1 : 1;
    return x->face < y->face ? -1 : x->face > y->face;
}

/* the share of the light c3d_node_light lit a corner with */
static BYTE light_share(const struct C3dNode *node, const struct AnimMeshFace *face, int corner)
{
    const struct AnimMesh *mesh = node->mesh;
    int shadeType = node->actor ? node->actor->shadeType : 0;

    if (shadeType != 1 && shadeType != 2)
        return 0;
    return mesh->texVertCount && face->tv[corner] < mesh->texVertCount && mesh->texVerts[face->tv[corner]].colour != 0
               ? 128
               : 255;
}

static int gpu_build(struct C3dNode *node)
{
    struct AnimMesh *mesh = node->mesh;
    struct ModelGpu *g;
    struct GpuFace *list = NULL;
    struct GfxMeshVertex *verts = NULL;
    WORD *indices = NULL;
    int f, i, k, n, ok = 0;

    if (!mesh || !mesh->faces || !mesh->positions || mesh->faceCount <= 0 || mesh->faceCount > 0x10000 / 3
        || !node->faceInfo || !node->groundLight)
        return 0;
    n = mesh->faceCount;
    for (f = 0; f < n; f++)
        for (k = 0; k < 3; k++)
            if (mesh->faces[f].v[k] >= mesh->vertexCount)
                return 0;
    g = calloc(1, sizeof *g);
    if (!g)
        return 0;
    node->gpu = g;
    list = malloc((size_t)n * sizeof *list);
    verts = malloc((size_t)n * 3 * sizeof *verts);
    indices = malloc((size_t)n * 3 * sizeof *indices);
    g->draws = calloc((size_t)n, sizeof *g->draws);
    if (!list || !verts || !indices || !g->draws)
        goto done;
    for (f = 0; f < n; f++) {
        list[f].face = f;
        list[f].key[0] = mesh->faces[f].texture;
        list[f].key[1] = mesh->faces[f].twoSided != 0;
    }
    qsort(list, (size_t)n, sizeof *list, gpu_face_less);
    for (i = 0; i < n; i++) {
        const struct AnimMeshFace *face = &mesh->faces[list[i].face];
        const struct FaceInfo *info = &node->faceInfo[list[i].face];
        const int *li = node->groundLight + list[i].face * 9;
        const Vec3 *a = &mesh->positions[face->v[0]], *b = &mesh->positions[face->v[1]];
        const Vec3 *c = &mesh->positions[face->v[2]];
        float e1[3] = {b->x - a->x, b->y - a->y, b->z - a->z}, e2[3] = {c->x - a->x, c->y - a->y, c->z - a->z};
        float nrm[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
        float len = sqrtf(nrm[0] * nrm[0] + nrm[1] * nrm[1] + nrm[2] * nrm[2]);

        for (k = 0; k < 3 && len > 0.0f; k++)
            nrm[k] /= len;
        for (k = 0; k < 3; k++) {
            struct GfxMeshVertex *v = &verts[i * 3 + k];
            const Vec3 *p = &mesh->positions[face->v[k]];

            v->x = p->x;
            v->y = p->y;
            v->z = p->z;
            v->nx = nrm[0];
            v->ny = nrm[1];
            v->nz = nrm[2];
            v->diffuse = (info->colour[k] & 0x00ffffff) | (DWORD)light_share(node, face, k) << 24;
            v->colour2 = pack(0xff, (BYTE)li[k * 3], (BYTE)li[k * 3 + 1], (BYTE)li[k * 3 + 2]);
            v->u0 = info->uv[k][0];
            v->v0 = info->uv[k][1];
            v->u1 = v->v1 = 0.0f;
            indices[i * 3 + k] = (WORD)(i * 3 + k);
        }
        if (i == 0 || list[i - 1].key[0] != list[i].key[0] || list[i - 1].key[1] != list[i].key[1]) {
            struct ModelGpuDraw *d = &g->draws[g->drawCount++];

            d->texture = face->texture;
            d->twoSided = (BYTE)list[i].key[1];
            d->first = i * 3;
            d->face = list[i].face;
        }
        g->draws[g->drawCount - 1].count += 3;
    }
    g->mesh = gfx_mesh_create(verts, n * 3, indices, n * 3);
    ok = g->mesh != NULL;
done:
    free(list);
    free(verts);
    free(indices);
    return ok;
}

/* the node's mesh, built on the first call; NULL where it cannot be, and the node is then not
   drawn */
static struct ModelGpu *gpu_ready(struct C3dNode *node)
{
    if (node->gpuState == 0) {
        int built = gpu_build(node);

        if (!built)
            model_node_gpu_free(node);
        node->gpuState = built ? 1 : -1;
    }
    return node->gpuState == 1 ? node->gpu : NULL;
}

/* Queues every draw of the mesh as a shadow caster, with its texture and texture-coordinate
   animation, which the shadow pass tests for alpha. */
static void gpu_cast(struct C3dNode *node, int version, const struct ModelGpu *g, const float *m)
{
    struct RPMeshDraw draw;
    int i;

    memset(&draw, 0, sizeof draw);
    draw.mesh = g->mesh;
    memcpy(draw.view, m, sizeof draw.view);
    draw.draw.kind = GFX_MESH_MODEL;
    for (i = 0; i < 4; i++)
        draw.draw.colour[i] = 1.0f;
    for (i = 0; i < g->drawCount; i++) {
        const struct ModelGpuDraw *d = &g->draws[i];

        uv_turn_matrix(version == 2 ? uv_xform_find((const struct C3dNodeV2 *)node, d->texture) : NULL, draw.draw.uv);
        draw.draw.first = d->first;
        draw.draw.count = d->count;
        renderer_add_mesh_draw(g_renderer, RENDER_SHADOW_CASTERS, &draw, run_texture(node, d->texture), NULL, 0.0f);
    }
}

/* Queues the node's draws and, while the frame draws shadows, its casters. A draw that goes into
   an alpha list is keyed there by the rhw of its first face's first corner, as a quad by its
   first vertex's. */
static void gpu_render(struct C3dNode *node, int version, const float *m, BYTE mirrored, BYTE layer)
{
    struct ModelGpu *g = gpu_ready(node);
    struct RPMeshDraw draw;
    BYTE alpha = node->actor->alpha;
    int i, k, cull, blind = g_playerBlind != 0;

    if (!g)
        return;
    memset(&draw, 0, sizeof draw);
    draw.mesh = g->mesh;
    memcpy(draw.view, m, sizeof draw.view);
    draw.draw.kind = GFX_MESH_MODEL;
    draw.draw.lightGround = !blind && g_weather.ground_lighting;
    for (k = 0; k < 3; k++)
        draw.draw.colour[k] = blind ? g_weather.tint[k] : g_weather.color[k];
    draw.draw.colour[3] = (float)alpha / 255.0f;
    cull = (version == 1) == (mirrored == 0) ? GFX_CULL_CW : GFX_CULL_CCW;
    for (i = 0; i < g->drawCount; i++) {
        const struct ModelGpuDraw *d = &g->draws[i];
        struct Texture *t = run_texture(node, d->texture);
        const struct UvTransform *x = version == 2 ? uv_xform_find((const struct C3dNodeV2 *)node, d->texture) : NULL;
        float key = 0.0f;
        int flags;

        if (version == 1)
            flags = alpha >= 0xff && t->format != 1 ? 0 : layer << 15 | 1;
        else
            flags = (x ? 0x20 : 0) | layer << 15 | (alpha < 0xff || t->format == 1);
        if (flags & 1) {
            struct TexDrawVertex corner;

            renderer_project_vertex(g_renderer, &node->mesh->positions[node->mesh->faces[d->face].v[0]], m, &corner);
            key = corner.rhw;
        }
        draw.draw.first = d->first;
        draw.draw.count = d->count;
        draw.cull = d->twoSided ? GFX_CULL_NONE : cull;
        uv_turn_matrix(x, draw.draw.uv);
        renderer_add_mesh_draw(g_renderer, renderer_mesh_pass(flags), &draw, t, NULL, key);
    }
    if (g_renderer->shadowPending)
        gpu_cast(node, version, g, m);
}

/* custom: only the node's shadow casters, and its children's (render/rp_pool.h) */
void c3d_node_v1_cast(struct C3dNode *base, const float *view)
{
    struct ModelGpu *g = gpu_ready(base);
    float m[12];
    size_t child;

    mat_multiply(m, base->matrix, view);
    if (g)
        gpu_cast(base, 1, g, m);
    for (child = 0; child < base->childCount; ++child)
        c3d_node_v1_cast(base->children[child], m);
}

void c3d_node_v2_cast(struct C3dNode *base, const float *view)
{
    struct ModelGpu *g = gpu_ready(base);
    float m[12];
    size_t child;

    mat_multiply(m, base->matrix, view);
    if (g)
        gpu_cast(base, 2, g, m);
    for (child = 0; child < base->childCount; ++child)
        c3d_node_v2_cast(base->children[child], m);
}

void c3d_node_v1_render(struct C3dNode *base, const float *view, BYTE mirrored, BYTE layer)
{
    float m[12];
    size_t child;

    mat_multiply(m, base->matrix, view);
    gpu_render(base, 1, m, mirrored, layer);
    for (child = 0; child < base->childCount; ++child)
        c3d_node_v1_render(base->children[child], m, mirrored, layer);
}

void c3d_node_v2_render(struct C3dNode *base, const float *view, BYTE mirrored, BYTE layer)
{
    float m[12];
    size_t child;

    mat_multiply(m, base->matrix, view);
    gpu_render(base, 2, m, mirrored, layer);
    for (child = 0; child < base->childCount; ++child)
        c3d_node_v2_render(base->children[child], m, mirrored, layer);
}
