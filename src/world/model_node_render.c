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
    } else if (node->highlight) {
        for (k = 0; k < 3; k++)
            out[k].diffuse = (info->colour[k] & 0x00ffffff) | 0x80000000;
    } else if (g_weather.ground_lighting) {
        const int *li = (const int *)node->lightInts + (info - infoBase) * 9;

        for (k = 0; k < 3; k++)
            out[k].diffuse = pack(alpha, SCALED(li[3 * k], scale[0]), SCALED(li[3 * k + 1], scale[1]),
                                 SCALED(li[3 * k + 2], scale[2]));
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

/* custom: a node's faces as a static mesh the device projects (render/gfx.h, world/model_types.h).
   A face's three corners keep the position, texture coordinates and colour face_colours gives
   them before the weather colour, which the device multiplies in as face_colours does, with the
   actor opaque, the player not blind and the node not highlighted; only then is the mesh drawn,
   where those faces would go into RENDER_OPAQUE_CLAMP.
   The device projects with the node's own matrix to view space, the one project() uses, and culls
   as the loops below do: a version 1 node keeps the faces that turn counter-clockwise on screen, a
   version 2 node the clockwise ones, the other way round when the actor is mirrored, and both
   sides of a twoSided face. */

struct GpuFace {
    int face;
    uintptr_t key[2];
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

static int gpu_texture_in_mesh(struct C3dNode *node, int version, int texture)
{
    if (run_texture(node, (WORD)texture)->format == 1)
        return 0;
    return version != 2 || !uv_xform_find((const struct C3dNodeV2 *)node, texture);
}

static DWORD gpu_corner_colour(const struct C3dNode *node, int face, int corner)
{
    const struct FaceInfo *info = &node->faceInfo[face];

    if (g_weather.ground_lighting) {
        const int *li = node->lightInts + face * 9 + corner * 3;

        return pack(0xff, (BYTE)li[0], (BYTE)li[1], (BYTE)li[2]);
    }
    return (info->colour[corner] & 0x00ffffff) | 0xff000000;
}

/* Fills a mesh with three vertices per face of `list` (n faces, sorted) and a draw per run of
   equal keys: key[0] the twoSided flag, key[1] the draw's texture. */
static int gpu_mesh_from(struct C3dNode *node, struct GpuFace *list, int n, struct GfxMeshVertex *verts,
                         WORD *indices, struct ModelGpuDraw *draws, int *drawCount, struct gfx_mesh **out)
{
    struct AnimMesh *mesh = node->mesh;
    int i, k;

    if (!n)
        return 1;
    qsort(list, (size_t)n, sizeof *list, gpu_face_less);
    for (i = 0; i < n; i++) {
        const struct AnimMeshFace *face = &mesh->faces[list[i].face];
        const struct FaceInfo *info = &node->faceInfo[list[i].face];
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
            v->diffuse = gpu_corner_colour(node, list[i].face, k);
            v->u0 = info->uv[k][0];
            v->v0 = info->uv[k][1];
            v->u1 = v->v1 = 0.0f;
            indices[i * 3 + k] = (WORD)(i * 3 + k);
        }
        if (i == 0 || list[i - 1].key[0] != list[i].key[0] || list[i - 1].key[1] != list[i].key[1]) {
            struct ModelGpuDraw *d = &draws[(*drawCount)++];

            d->texture = (struct Texture *)list[i].key[1];
            d->twoSided = (BYTE)list[i].key[0];
            d->first = i * 3;
        }
        draws[*drawCount - 1].count += 3;
    }
    *out = gfx_mesh_create(verts, n * 3, indices, n * 3);
    return *out != NULL;
}

static int gpu_build(struct C3dNode *node, int version)
{
    struct AnimMesh *mesh = node->mesh;
    struct ModelGpu *g;
    struct GpuFace *list;
    struct GfxMeshVertex *verts;
    WORD *indices;
    int f, n = 0, i, k, textures = 0, ok = 0;

    if (!mesh || !mesh->faces || !mesh->positions || mesh->faceCount <= 0 || mesh->faceCount > 0x10000 / 3
        || !node->faceInfo || !node->lightInts)
        return 0;
    for (f = 0; f < mesh->faceCount; f++) {
        const struct AnimMeshFace *face = &mesh->faces[f];

        for (k = 0; k < 3; k++)
            if (face->v[k] >= mesh->vertexCount)
                return 0;
        if (face->texture >= textures)
            textures = face->texture + 1;
    }
    g = calloc(1, sizeof *g);
    if (!g)
        return 0;
    node->gpu = g;
    g->groundLighting = g_weather.ground_lighting;
    g->inMesh = calloc((size_t)textures, 1);
    list = malloc((size_t)mesh->faceCount * sizeof *list);
    verts = malloc((size_t)mesh->faceCount * 3 * sizeof *verts);
    indices = malloc((size_t)mesh->faceCount * 3 * sizeof *indices);
    g->draws = calloc((size_t)mesh->faceCount, sizeof *g->draws);
    g->casterDraws = calloc((size_t)mesh->faceCount, sizeof *g->casterDraws);
    if (!g->inMesh || !list || !verts || !indices || !g->draws || !g->casterDraws)
        goto done;
    g->inMeshCount = textures;
    g->allInMesh = 1;
    for (i = 0; i < textures; i++)
        g->inMesh[i] = (BYTE)gpu_texture_in_mesh(node, version, i);
    for (f = 0; f < mesh->faceCount; f++) {
        const struct AnimMeshFace *face = &mesh->faces[f];

        if (!g->inMesh[face->texture]) {
            g->allInMesh = 0;
            continue;
        }
        list[n].face = f;
        list[n].key[0] = face->twoSided != 0;
        list[n].key[1] = (uintptr_t)run_texture(node, face->texture);
        n++;
    }
    if (!gpu_mesh_from(node, list, n, verts, indices, g->draws, &g->drawCount, &g->mesh))
        goto done;
    /* the faces left out cast from casterMesh: an alpha-keyed face with its texture, the rest
       (texture-coordinate animations, opaque) without */
    for (f = 0, n = 0; f < mesh->faceCount; f++) {
        const struct AnimMeshFace *face = &mesh->faces[f];
        struct Texture *t = run_texture(node, face->texture);

        if (g->inMesh[face->texture])
            continue;
        list[n].face = f;
        list[n].key[0] = 0;
        list[n].key[1] = t->format == 1 ? (uintptr_t)t : 0;
        n++;
    }
    ok = gpu_mesh_from(node, list, n, verts, indices, g->casterDraws, &g->casterDrawCount, &g->casterMesh);
done:
    free(list);
    free(verts);
    free(indices);
    return ok;
}

/* Queues the node's meshes: the mesh into the opaque list when its faces would be drawn with their
   own colours there (the actor opaque, the player not blind, the node not highlighted), and answers
   whether it did, so that the caller leaves the faces inMesh marks to it; otherwise the mesh only
   casts shadows this frame, as casterMesh always does. */
static int gpu_render(struct C3dNode *node, int version, const float *m, BYTE mirrored, BYTE alpha)
{
    struct ModelGpu *g;
    int i, cull, colour;

    if (node->gpu && node->gpu->groundLighting != g_weather.ground_lighting)
        model_node_gpu_free(node);
    if (node->gpuState == 0) {
        int built = gpu_build(node, version);

        if (!built)
            model_node_gpu_free(node);
        node->gpuState = built ? 1 : -1;
    }
    g = node->gpu;
    if (node->gpuState != 1 || !g)
        return 0;
    colour = alpha == 0xff && !g_playerBlind && !node->highlight;
    cull = (version == 1) == (mirrored == 0) ? GFX_CULL_CW : GFX_CULL_CCW;
    for (i = 0; i < g->drawCount; i++)
        renderer_add_mesh_draw(g_renderer, colour ? RENDER_MODEL_MESH : RENDER_SHADOW_ONLY, g->mesh, g->draws[i].first,
                               g->draws[i].count, m, g_weather.color, colour ? g->draws[i].texture : NULL, NULL,
                               g->draws[i].twoSided ? GFX_CULL_NONE : cull);
    for (i = 0; i < g->casterDrawCount; i++)
        renderer_add_mesh_draw(g_renderer, RENDER_SHADOW_ONLY, g->casterMesh, g->casterDraws[i].first,
                               g->casterDraws[i].count, m, NULL, g->casterDraws[i].texture, NULL, 0);
    return colour;
}

static int gpu_has(const struct C3dNode *node, WORD texture)
{
    return texture < node->gpu->inMeshCount && node->gpu->inMesh[texture];
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
    int gpu;

    mat_multiply(m, node->matrix, view);
    alpha = node->actor->alpha;
    gpu = gpu_render(node, 1, m, mirrored, alpha);
    if (gpu && node->gpu->allInMesh)
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

        if (gpu && gpu_has(node, texture)) {
            for (; face != end && face->texture == texture; face++, info++) {}
            continue;
        }

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
    node->highlight = 0;
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
    int gpu;

    mat_multiply(m, node->matrix, view);
    alpha = node->actor->alpha;
    gpu = gpu_render(node, 2, m, mirrored, alpha);
    if (gpu && node->gpu->allInMesh)
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

        if (gpu && gpu_has(node, texture)) {
            for (; face != end && face->texture == texture; face++, info++) {}
            continue;
        }

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
    node->highlight = 0;
    for (child = 0; child < node->childCount; ++child)
        c3d_node_v2_render(node->children[child], m, mirrored, clip, layer);
}
