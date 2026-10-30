#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "base.h"
#include "world/model.h"
#include "world/model_node_util.h"

DWORD g_nodeCornerColours[0x15f90];
INIT_ONCE g_nodeV1TlVertsGuard;
int g_isVoodooInUse;

static struct C3dNode *model_node_allocate(int version)
{
    size_t size = version == 1 ? sizeof(struct C3dNodeV1) :
                  version == 2 ? sizeof(struct C3dNodeV2) : 0;
    struct C3dNode *node;
    if (!size) return NULL;
    node = calloc(1, size);
    if (!node) return NULL;
    node->name = "";
    node->alpha = 1.0f;
    return node;
}

void model_node_destroy(struct C3dNode *node, int version)
{
    size_t i;
    if (version == 2) free(((struct C3dNodeV2 *)node)->uvXforms);
    for (i = 0; i < node->childCount; ++i)
        if (node->children[i]) model_node_destroy(node->children[i], version);
    free(node->children);
    free(node->faceInfo);
    free(node->lightInts);
    free(node);
}

static int model_node_load(struct C3dNode *node, struct ModelResNode *resource, int version)
{
    return version == 1 ? c3d_node_v1_load(node, resource) : c3d_node_v2_load(node, resource);
}

struct C3dNode *model_node_create(int version, struct C3dActor *actor,
                                struct C3dNode *parent, struct ModelResNode *resource)
{
    struct C3dNode *node = model_node_allocate(version);
    if (!node) return NULL;
    node->actor = actor;
    node->parent = parent;
    if (!resource || !model_node_load(node, resource, version)) {
        model_node_destroy(node, version);
        return NULL;
    }
    return node;
}

static int prepare_node(struct C3dNode *node, struct ModelResNode *res)
{
    size_t faces;
    if (!res->mesh || res->mesh->faceCount < 0) return 0;
    faces = (size_t)res->mesh->faceCount;
    if (faces > SIZE_MAX / sizeof *node->faceInfo || faces > SIZE_MAX / (9 * sizeof *node->lightInts)) return 0;
    node->resource = res;
    node->name = owned_text_cstr(&res->name);
    node->mesh = res->mesh;
    node->textures = res->textures;
    node->textureCount = res->textureCount;
    node->alpha = (float)res->alpha / 255.0f;
    if (faces) {
        node->faceInfo = calloc(faces, sizeof *node->faceInfo);
        node->lightInts = calloc(faces, 9 * sizeof *node->lightInts);
        if (!node->faceInfo || !node->lightInts) return 0;
    }
    return 1;
}

static int load_children(struct C3dNode *node, struct ModelResNode *res, int version)
{
    size_t i;
    if (!res->childCount) return 1;
    if (res->childCount > SIZE_MAX / sizeof *node->children) return 0;
    node->children = calloc(res->childCount, sizeof *node->children);
    if (!node->children) return 0;
    node->childCount = res->childCount;
    for (i = 0; i < res->childCount; ++i) {
        struct C3dNode *child = model_node_allocate(version);
        if (!child) return 0;
        child->actor = node->actor;
        child->parent = node;
        if (version == 1) node->children[i] = child;
        if (!model_node_load(child, res->children[i], version)) {
            if (version == 2) model_node_destroy(child, 2);
            return 0;
        }
        if (version == 2) node->children[i] = child;
    }
    return 1;
}

int c3d_node_v1_load(struct C3dNode *base, struct ModelResNode *resource)
{
    struct C3dNodeV1 *self = (struct C3dNodeV1 *)base;
    struct ModelResNodeV1 *source = (struct ModelResNodeV1 *)resource;
    if (!prepare_node(base, resource)) return 0;
    self->restPos = (struct KeyPos){0, source->pos[0], source->pos[1], source->pos[2]};
    self->restScale = (struct KeyScaleAxis){0, source->scale[0], source->scale[1], source->scale[2], 0, 0, 0, 1};
    key_quat_from_axis_angle(&self->restRot, source->axis, source->angle);
    return load_children(base, resource, 1);
}

int c3d_node_v2_load(struct C3dNode *base, struct ModelResNode *resource)
{
    struct C3dNodeV2 *self = (struct C3dNodeV2 *)base;
    struct ModelResNodeV2 *source = (struct ModelResNodeV2 *)resource;
    if (!prepare_node(base, resource)) return 0;
    memcpy(self->offset, source->matrix, sizeof self->offset);
    if (!base->parent) memcpy(self->local, source->matrix, sizeof self->local);
    else {
        float m[12], parentOffset[12], inv[12];
        memcpy(m, source->matrix, sizeof m);
        memcpy(parentOffset, ((struct C3dNodeV2 *)base->parent)->offset, sizeof parentOffset);
        mat_invert(inv, parentOffset);
        mat_multiply(self->local, m, inv);
    }
    if (source->uvKeys.count) {
        if (source->uvKeys.count > SIZE_MAX / sizeof *self->uvXforms) return 0;
        self->uvXforms = calloc(source->uvKeys.count, sizeof *self->uvXforms);
        if (!self->uvXforms) return 0;
    }
    return load_children(base, resource, 2);
}

void c3d_node_v1_animate(struct C3dNode *base, double t)
{
    struct C3dNodeV1 *self = (struct C3dNodeV1 *)base;
    float pos[12], scale[12], rot[12], m[12];
    int frame = (int)t;
    size_t i;

    memcpy(pos, g_identityMatrix.m, sizeof(pos));
    memcpy(scale, g_identityMatrix.m, sizeof(scale));
    memcpy(rot, g_identityMatrix.m, sizeof(rot));
    {
        const struct ModelResNodeV1 *res = (const struct ModelResNodeV1 *)base->resource;
        key_pos_matrix(&self->restPos, 1, frame, pos);
        key_quat_matrix(res->rotKeyCount ? res->rotKeys : &self->restRot,
                        res->rotKeyCount ? res->rotKeyCount : 1, frame, rot);
        key_scale_matrix(&self->restScale, 1, scale);
    }
    mat_multiply(m, scale, rot);
    mat_multiply(self->base.matrix, m, pos);
    for (i = 0; i < base->childCount; ++i)
        c3d_node_v1_animate(base->children[i], t);
}

const struct UvTransform *uv_xform_find(const struct C3dNodeV2 *node, int texture)
{
    size_t first = 0, last = node->uvXformCount;
    while (first < last) {
        size_t mid = first + (last - first) / 2;
        if (node->uvXforms[mid].texture < texture) first = mid + 1;
        else last = mid;
    }
    return first < node->uvXformCount && node->uvXforms[first].texture == texture ? &node->uvXforms[first] : NULL;
}

void c3d_node_v2_animate(struct C3dNode *base, double t)
{
    struct C3dNodeV2 *self = (struct C3dNodeV2 *)base;
    const struct ModelResNodeV2 *res = (const struct ModelResNodeV2 *)base->resource;
    float *m = base->matrix;
    size_t i;
    memcpy(m, g_identityMatrix.m, sizeof base->matrix);
    key_vec_scale(res->scaleKeys, res->scaleKeyCount, (float)t, m);
    key_quat_apply(res->rotKeys, res->rotKeyCount, (float)t, self->local, m);
    key_vec_translation(res->posKeys, res->posKeyCount, (float)t, self->local, m);
    self->uvXformCount = res->uvKeys.count;
    for (i = 0; i < self->uvXformCount; ++i) {
        const struct UvKeyTrack *track = &res->uvKeys.tracks[i];
        struct UvTransform *out = &self->uvXforms[i];
        float values[5] = {0, 0, 1, 1, 0};
        key_uv_channels_eval(track->channels, (float)t, values);
        out->texture = track->texture;
        out->offset[0] = values[0]; out->offset[1] = values[1];
        out->scale[0] = values[2]; out->scale[1] = values[3]; out->angle = values[4];
    }
    for (i = 0; i < base->childCount; ++i)
        c3d_node_v2_animate(base->children[i], t);
}

void uv_turn(const struct UvTransform *x, float *u, float *v)
{
    float a, b, nu, nv;

    a = float_cos(x->angle) * (*u - 0.5f);
    b = (float)((double)a - (double)float_sin(x->angle) * (double)(*v - 0.5f));
    nu = x->scale[0] * b + 0.5f + x->offset[0];
    a = float_sin(x->angle) * (*u - 0.5f);
    b = (float)((double)float_cos(x->angle) * (double)(*v - 0.5f) + (double)a);
    nv = x->scale[1] * b + 0.5f + x->offset[1];
    *u = nu;
    *v = nv;
}

void c3d_node_get_bound(struct C3dNode *self, float *box, const float *parent)
{
    float m[12], p[3];
    size_t child;
    int i, k;

    mat_multiply(m, self->matrix, parent);
    for (i = 0; i < self->mesh->vertexCount; i++) {
        xform(m, &self->mesh->positions[i], p);
        for (k = 0; k < 3; k++) {
            if (box[k] > p[k])
                box[k] = p[k];
            if (p[k] > box[3 + k])
                box[3 + k] = p[k];
        }
    }
    for (child = 0; child < self->childCount; ++child)
        c3d_node_get_bound(self->children[child], box, m);
}
