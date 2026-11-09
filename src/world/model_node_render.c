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

static struct TexDrawVertex g_nodeV1TlVerts[30000] = { 0 };

static struct TexDrawVertex g_nodeV2TlVerts[30000] = { 0 };

static struct Once g_nodeV2TlVertsGuard;

static void face_colours(struct C3dNode *node, struct TexDrawVertex *out, const struct FaceInfo *info,
                         const struct FaceInfo *infoBase, BYTE alpha, const float *tint, const float *scale)
{
    int k;

    if (g_playerBlind != 0) {
        for (k = 0; k < 3; k++) {
            DWORD c = info->colour[k];
            out[k].diffuse = pack(alpha, SCALED(BYTE_R(c), tint[0]), SCALED(BYTE_G(c), tint[1]),
                                 SCALED(BYTE_B(c), tint[2]));
        }
    } else if (g_weather.ground_lighting) {
        const int *li = node->groundLight + (info - infoBase) * 9;

        for (k = 0; k < 3; k++) {
            DWORD c = info->colour[k];
            out[k].diffuse = pack(alpha, SCALED(ground_lit(li[3 * k], BYTE_R(c)), scale[0]),
                                 SCALED(ground_lit(li[3 * k + 1], BYTE_G(c)), scale[1]),
                                 SCALED(ground_lit(li[3 * k + 2], BYTE_B(c)), scale[2]));
        }
    } else {
        for (k = 0; k < 3; k++) {
            DWORD c = info->colour[k];
            out[k].diffuse = pack(alpha, SCALED(BYTE_R(c), scale[0]), SCALED(BYTE_G(c), scale[1]),
                                 SCALED(BYTE_B(c), scale[2]));
        }
    }
}

/* custom: whether a projected triangle has a corner on or behind the camera's own plane. Model
   triangles are drawn as pre-transformed vertices, so such a corner flips to the far side of the
   screen and the triangle smears its texture across the view. The original's camera stays outside
   the models; with the unlocked zoom and tilt (game/camera.h) the eye can stand inside a building,
   whose triangles around it are then left out instead. */
static int behind_eye(const struct TexDrawVertex *a, const struct TexDrawVertex *b, const struct TexDrawVertex *c)
{
    return !(a->rhw > 0.0f) || !(b->rhw > 0.0f) || !(c->rhw > 0.0f);
}

static int on_screen(const struct TexDrawVertex *a, const struct TexDrawVertex *b, const struct TexDrawVertex *c)
{
    struct Renderer *r = g_renderer;
    float minX, maxX, minY, maxY;

    minX = a->x < b->x ? a->x : b->x;
    if (!(c->x > minX))
        minX = c->x;
    if (minX >= (float)r->width)
        return 0;
    maxX = a->x > b->x ? a->x : b->x;
    if (!(maxX > c->x))
        maxX = c->x;
    if (0.0f > maxX)
        return 0;
    minY = a->y < b->y ? a->y : b->y;
    if (!(c->y > minY))
        minY = c->y;
    if (minY >= (float)r->height)
        return 0;
    maxY = a->y > b->y ? a->y : b->y;
    if (!(maxY > c->y))
        maxY = c->y;
    if (0.0f > maxY)
        return 0;
    if (g_isVoodooInUse != 0) {
        if (r->clipLeft >= minX || r->clipTop >= minY || maxX >= r->clipRight || maxY >= r->clipBottom)
            return 0;
    }
    return 1;
}

static void init_tl_buffer(struct TexDrawVertex *buf)
{
    int i;

    for (i = 0; i < MODEL_TL_VERTS; i++) {
        buf[i].x = buf[i].y = buf[i].z = buf[i].rhw = 0.0f;
        buf[i].diffuse = 0;
        buf[i].specular = 0xff000000;
        buf[i].u = buf[i].v = 0.0f;
    }
}

static void project(struct C3dNode *self, const float *m, struct TexDrawVertex *buf, float *tint, float *scale)
{
    struct AnimMesh *mesh = self->mesh;
    int n = mesh->vertexCount;
    const Vec3 *v = mesh->positions;

    scale[0] = g_weather.color[0];
    scale[1] = g_weather.color[1];
    scale[2] = g_weather.color[2];
    tint[0] = g_weather.tint[0];
    tint[1] = g_weather.tint[1];
    tint[2] = g_weather.tint[2];
    for (; n > 0; n--) {
        renderer_project_vertex(g_renderer, v, m, buf++);
        ++v;
    }
}

/* custom: a node's faces as one static mesh the device projects (render/gfx.h, world/model_types.h).
   A face's three corners keep their position, texture coordinates and what face_colours makes
   their colour from: diffuse the node's own lit colour, with the share of the light it was lit
   with as its alpha (128 or 255 by the corner's texture vertex, as c3d_node_light takes it, 0 for
   a model lit without its light), colour2 the ground's light under the corner. Each frame a draw
   goes where the loops below would send its faces' quads, with the colours, alpha and list they
   would give them: from the weather, the player's blindness, the ground lighting option, the
   actor's alpha, the texture's format and, on a version 2 node, its texture-coordinate animation,
   which turns the draw's coordinates as uv_turn does. In a shadow the device takes the light's
   direct part away from a corner's own colour (render/gfx.h). The device projects with the node's own matrix to view space, the
   one project() uses, and culls as the loops below do: a version 1 node keeps the faces that turn
   counter-clockwise on screen, a version 2 node the clockwise ones, the other way round when the
   actor is mirrored, and both sides of a twoSided face. Every draw also casts a shadow. */

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

/* the node's mesh, built on the first call; NULL where it cannot be */
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

/* Queues the node's draws and its casters, and answers whether it did; where its mesh cannot be
   made the caller draws the faces itself. A draw that goes into an alpha list is keyed there by
   the rhw of its first face's first corner, as a quad by its first vertex's. */
static int gpu_render(struct C3dNode *node, int version, const float *m, BYTE mirrored, BYTE layer)
{
    struct ModelGpu *g = gpu_ready(node);
    struct RPMeshDraw draw;
    BYTE alpha = node->actor->alpha;
    int i, k, cull, blind = g_playerBlind != 0;

    if (!g)
        return 0;
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
    gpu_cast(node, version, g, m);
    return 1;
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

void c3d_node_v1_render(struct C3dNode *base, const float *view, BYTE mirrored,
                                 BYTE clip, BYTE layer)
{
    struct C3dNodeV1 *self = (struct C3dNodeV1 *)base;
    static struct TexDrawVertex *const buf = g_nodeV1TlVerts;
    struct C3dNode *node = &self->base;
    struct AnimMesh *mesh;
    float m[12], tint[3], scale[3];
    struct TexDrawVertex *pool;
    struct AnimMeshFace *face, *end;
    struct FaceInfo *info, *infoBase;
    BYTE alpha;
    size_t child;

    mat_multiply(m, node->matrix, view);
    alpha = node->actor->alpha;
    if (gpu_render(node, 1, m, mirrored, layer))
        goto children;
    if (once_begin(&g_nodeV1TlVertsGuard)) {
        init_tl_buffer(buf);
        once_complete(&g_nodeV1TlVertsGuard);
    }
    project(node, m, buf, tint, scale);
    mesh = node->mesh;
    pool = renderer_new_tex_draw_vertices(g_renderer, mesh->faceCount * 3);
    face = mesh->faces;
    end = face ? face + mesh->faceCount : face;
    infoBase = node->faceInfo;
    info = infoBase;
    while (face != end) {
        WORD texture = face->texture;
        struct TexDrawVertex *out = pool;
        int count;

        for (; face != end; face++, info++) {
            struct TexDrawVertex *a, *b, *c;
            int front, k;

            if (face == NULL || face->texture != texture)
                break;
            a = &buf[face->v[0]];
            b = &buf[face->v[1]];
            c = &buf[face->v[2]];
            if (behind_eye(a, b, c))
                continue;
            if (clip && !on_screen(a, b, c))
                continue;
            front = (b->y - a->y) * (c->x - a->x) > (c->y - a->y) * (b->x - a->x);
            if (!(front ? mirrored == 0 : mirrored != 0) && face->twoSided == 0)
                continue;
            out[0] = *a;
            out[1] = *b;
            out[2] = *c;
            for (k = 0; k < 3; k++) {
                out[k].u = info->uv[k][0];
                out[k].v = info->uv[k][1];
            }
            face_colours(node, out, info, infoBase, alpha, tint, scale);
            out += 3;
        }
        count = (int)(out - pool);
        if (count != 0) {
            struct RPQuad *prim = renderer_new_quad(g_renderer);
            int list;

            prim->verts = pool;
            prim->nverts = count;
            prim->texture = run_texture(node, texture);
            if (alpha >= 0xff && prim->texture->format != 1)
                list = 0;
            else
                list = layer << 15 | 1;
            renderer_add_prim(g_renderer, prim, list);
            pool = out;
        }
    }
children:
    for (child = 0; child < node->childCount; ++child)
        c3d_node_v1_render(node->children[child], m, mirrored, clip, layer);
}

void c3d_node_v2_render(struct C3dNode *base, const float *view, BYTE mirrored,
                                 BYTE clip, BYTE layer)
{
    struct C3dNodeV2 *self = (struct C3dNodeV2 *)base;
    static struct TexDrawVertex *const buf = g_nodeV2TlVerts;
    struct C3dNode *node = &self->base;
    struct AnimMesh *mesh;
    float m[12], tint[3], scale[3];
    struct TexDrawVertex *pool;
    struct AnimMeshFace *face, *end;
    struct FaceInfo *info, *infoBase;
    BYTE alpha;
    size_t child;

    mat_multiply(m, node->matrix, view);
    alpha = node->actor->alpha;
    if (gpu_render(node, 2, m, mirrored, layer))
        goto children;
    if (once_begin(&g_nodeV2TlVertsGuard)) {
        init_tl_buffer(buf);
        once_complete(&g_nodeV2TlVertsGuard);
    }
    project(node, m, buf, tint, scale);
    mesh = node->mesh;
    pool = renderer_new_tex_draw_vertices(g_renderer, mesh->faceCount * 3);
    face = mesh->faces;
    end = face ? face + mesh->faceCount : face;
    infoBase = node->faceInfo;
    info = infoBase;
    while (face != end) {
        WORD texture = face->texture;
        struct TexDrawVertex *out = pool;
        int uvFlag = 0, count;

        for (; face != end; face++, info++) {
            struct TexDrawVertex *a, *b, *c;
            BYTE front;
            int k;

            if (face == NULL || face->texture != texture)
                break;
            a = &buf[face->v[0]];
            b = &buf[face->v[1]];
            c = &buf[face->v[2]];
            if (behind_eye(a, b, c))
                continue;
            if (clip && !on_screen(a, b, c))
                continue;
            front = (c->y - a->y) * (b->x - a->x) > (b->y - a->y) * (c->x - a->x);
            if (!(front == 1 ? mirrored == 0 : mirrored == 1) && face->twoSided != 1)
                continue;
            out[0] = *a;
            out[1] = *b;
            out[2] = *c;
            for (k = 0; k < 3; k++) {
                out[k].u = info->uv[k][0];
                out[k].v = info->uv[k][1];
            }
            for (k = 0; k < 3; k++) {
                const struct UvTransform *x = uv_xform_find(self, face->texture);

                if (x != NULL) {
                    uv_turn(x, &out[k].u, &out[k].v);
                    uvFlag = 0x20;
                }
            }
            face_colours(node, out, info, infoBase, alpha, tint, scale);
            out += 3;
        }
        count = (int)(out - pool);
        if (count != 0) {
            struct RPQuad *prim = renderer_new_quad(g_renderer);
            int list;

            prim->verts = pool;
            prim->nverts = count;
            prim->texture = run_texture(node, texture);
            list = uvFlag | layer << 15;
            if (alpha < 0xff || prim->texture->format == 1)
                list |= 1;
            renderer_add_prim(g_renderer, prim, list);
            pool = out;
        }
    }
children:
    for (child = 0; child < node->childCount; ++child)
        c3d_node_v2_render(node->children[child], m, mirrored, clip, layer);
}
