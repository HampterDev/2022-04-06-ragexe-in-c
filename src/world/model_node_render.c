#include "render/rp_pool.h"
#include "actor/weather.h"
#include "game/context.h"
#include "base.h"
#include "world/model.h"
#include "world/model_node_util.h"

#include <math.h>
#include <string.h>
#include "actor/attr_state.h"

static struct TexDrawVertex g_nodeV1TlVerts[30000] = { 0 };

static struct TexDrawVertex g_nodeV2TlVerts[30000] = { 0 };

static INIT_ONCE g_nodeV2TlVertsGuard = INIT_ONCE_STATIC_INIT;

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
    if (once_begin(&g_nodeV1TlVertsGuard)) {
        init_tl_buffer(buf);
        once_complete(&g_nodeV1TlVertsGuard);
    }
    project(node, m, buf, tint, scale);
    alpha = node->actor->alpha;
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

    mat_multiply(m, node->matrix, view);
    if (once_begin(&g_nodeV2TlVertsGuard)) {
        init_tl_buffer(buf);
        once_complete(&g_nodeV2TlVertsGuard);
    }
    project(node, m, buf, tint, scale);
    alpha = node->actor->alpha;
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
    node->highlight = 0;
    for (child = 0; child < node->childCount; ++child)
        c3d_node_v2_render(node->children[child], m, mirrored, clip, layer);
}
