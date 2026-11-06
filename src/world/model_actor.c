#include "render/rp_pool.h"
#include <string.h>
#include <stdlib.h>
#include "core/numeric.h"
#include <math.h>
#include "base.h"
#include "world/model.h"

static const struct ModelActorOps model_actor_v1_ops = {
    .destroy = model_actor_v1_destroy,
    .create = c3d_actor_v1_create,
    .light = c3d_actor_v1_light,
    .animate = c3d_actor_v1_animate,
    .tick = c3d_actor_v1_tick,
    .render = c3d_actor_v1_render,
    .set_speed = c3d_actor_set_speed,
};

static const struct ModelActorOps model_actor_v2_ops = {
    .destroy = model_actor_v2_destroy,
    .create = c3d_actor_v2_create,
    .light = c3d_actor_v2_light,
    .animate = c3d_actor_v2_animate,
    .tick = c3d_actor_v2_tick,
    .render = c3d_actor_v2_render,
    .set_speed = c3d_actor_v2_set_speed,
};

#define MODEL_DEG_TO_RAD(d) ((d) / 180.0f * 3.141592f)
#define MODEL_BOX_HUGE      100000.0f

static double unsigned_to_double(DWORD v)
{
    return (double)v;
}

struct C3dActor *model_actor_create(int version)
{
    size_t size = version == 1 ? sizeof(struct C3dActorV1) :
                  version == 2 ? sizeof(struct C3dActorV2) : 0;
    struct C3dActor *actor;
    if (!size) return NULL;
    actor = calloc(1, size);
    if (!actor) return NULL;
    actor->ops = version == 1 ? &model_actor_v1_ops : &model_actor_v2_ops;
    actor->scale[0] = actor->scale[1] = actor->scale[2] = 1.0f;
    actor->dirty = 1;
    actor->alpha = 255;
    actor->animType = 2;
    if (version == 2) ((struct C3dActorV2 *)actor)->startTick = GetTickCount();
    return actor;
}

static void clear_model_roots(struct C3dActorV2 *actor)
{
    size_t i;
    for (i = 0; i < actor->nodeCount; ++i)
        if (actor->nodes[i]) model_node_destroy(actor->nodes[i], 2);
    free(actor->nodes);
    actor->nodes = NULL;
    actor->nodeCount = 0;
}

void model_actor_v1_destroy(struct C3dActor *base)
{
    struct C3dActorV1 *actor = (struct C3dActorV1 *)base;
    if (actor->root) model_node_destroy(actor->root, 1);
    free(base->volumeBoxes);
    free(actor);
}

void model_actor_v2_destroy(struct C3dActor *base)
{
    clear_model_roots((struct C3dActorV2 *)base);
    free(base->volumeBoxes);
    free(base);
}

int c3d_actor_add_volume_boxes(struct C3dActor *actor, const struct VolumeBox *boxes, size_t count)
{
    struct VolumeBox *copy;
    size_t total;
    if (!count) return 1;
    if (count > SIZE_MAX / sizeof *copy - actor->volumeBoxCount) return 0;
    total = actor->volumeBoxCount + count;
    copy = malloc(total * sizeof *copy);
    if (!copy) return 0;
    if (actor->volumeBoxCount) memcpy(copy, actor->volumeBoxes, actor->volumeBoxCount * sizeof *copy);
    memcpy(copy + actor->volumeBoxCount, boxes, count * sizeof *copy);
    free(actor->volumeBoxes);
    actor->volumeBoxes = copy;
    actor->volumeBoxCount = total;
    return 1;
}

int c3d_actor_v1_create(struct C3dActor *base, struct C3dModelRes *res)
{
    struct C3dActorV1 *self = (struct C3dActorV1 *)base;
    if (self->root) model_node_destroy(self->root, 1);
    self->root = NULL;
    if (!res->nodeCount) return 0;
    base->shadeType = res->shadeType;
    self->frameCount = res->frameCount;
    self->root = model_node_create(1, base, NULL, res->nodes[0]);
    if (!self->root) return 0;
    c3d_actor_v1_animate(base, 0.0);
    c3d_actor_v1_update_bound(base);
    return 1;
}

int c3d_actor_v2_create(struct C3dActor *base, struct C3dModelRes *res)
{
    struct C3dActorV2 *self = (struct C3dActorV2 *)base;
    size_t i;
    clear_model_roots(self);
    base->shadeType = res->shadeType;
    self->fps = res->fps;
    self->duration = (double)((float)(res->frameCount * 1000) * (1.0f / self->fps));
    if (res->mainNodes.size) {
        if (res->mainNodes.size > SIZE_MAX / sizeof *self->nodes) return 0;
        self->nodes = calloc(res->mainNodes.size, sizeof *self->nodes);
        if (!self->nodes) return 0;
        self->nodeCount = res->mainNodes.size;
    }
    for (i = 0; i < self->nodeCount; ++i) {
        struct ModelResNode *node = model_res_find_node(res, owned_text_cstr(&res->mainNodes.items[i]));
        self->nodes[i] = model_node_create(2, base, NULL, node);
        if (!self->nodes[i]) { clear_model_roots(self); return 0; }
    }
    c3d_actor_v2_animate(base, 0.0);
    c3d_actor_v2_update_bound(base);
    return 1;
}

static void place(struct C3dActor *self)
{
    float *m = self->matrix;

    mat_scale(m, self->scale[0], self->scale[1], self->scale[2]);
    mat_rotate_y(m, MODEL_DEG_TO_RAD(self->rot[1]));
    mat_rotate_x(m, MODEL_DEG_TO_RAD(self->rot[0]));
    mat_rotate_z(m, MODEL_DEG_TO_RAD(self->rot[2]));
    {
        Vec3 translation = {self->pos[0], self->pos[1], self->pos[2]};
        mat_translate(m, &translation);
    }
}

void c3d_actor_build_matrix(struct C3dActor *self)
{
    mat_set_translation(self->matrix, self->offset[0], self->offset[1], self->offset[2]);
    place(self);
    mat_invert(self->inverse, self->matrix);
    self->dirty = 0;
}

void c3d_actor_v2_build_matrix(struct C3dActor *base)
{
    struct C3dActorV2 *self = (struct C3dActorV2 *)base;
    float *m = self->base.matrix;

    memcpy(m, g_identityMatrix.m, sizeof(self->base.matrix));
    m[4] = -1.0f;
    place(&self->base);
    mat_invert(self->base.inverse, m);
    self->base.dirty = 0;
}

static void xform(const float *m, const float *p, float *out)
{
    out[0] = p[0] * m[0] + p[1] * m[3] + p[2] * m[6] + m[9];
    out[1] = p[0] * m[1] + p[1] * m[4] + p[2] * m[7] + m[10];
    out[2] = p[0] * m[2] + p[1] * m[5] + p[2] * m[8] + m[11];
}

static void normalize(float *v)
{
    float inv = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    v[0] *= inv;
    v[1] *= inv;
    v[2] *= inv;
}

static void place_volume_box(struct VolumeBox *b, const float *actorMatrix)
{
    static const signed char sign[8][3] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1},
    };
    float w[12];
    int i, k;

    mat_set_rotate_x(b->matrix, MODEL_DEG_TO_RAD(b->rot[0]));
    mat_rotate_y(b->matrix, MODEL_DEG_TO_RAD(b->rot[1]));
    mat_rotate_z(b->matrix, MODEL_DEG_TO_RAD(b->rot[2]));
    {
        Vec3 translation = {b->pos[0], b->pos[1], b->pos[2]};
        mat_translate(b->matrix, &translation);
    }
    mat_invert(b->inverse, b->matrix);
    mat_multiply(w, b->matrix, actorMatrix);
    for (i = 0; i < 8; i++) {
        float p[3];

        for (k = 0; k < 3; k++)
            p[k] = sign[i][k] < 0 ? -b->size[k] : b->size[k];
        xform(w, p, b->corners[i]);
    }
}

static void bound_from_box(struct C3dActor *self, const float *box)
{
    float *m = self->matrix;
    float rm[12], lo[3], hi[3];
    size_t nodeIndex;
    int i, k;

    for (k = 0; k < 3; k++) {
        self->bboxMin[k] = MODEL_BOX_HUGE;
        self->bboxMax[k] = -MODEL_BOX_HUGE;
    }
    for (i = 0; i < 8; i++) {
        float p[3];

        p[0] = box[i & 1 ? 3 : 0];
        p[1] = box[i & 2 ? 4 : 1];
        p[2] = box[i & 4 ? 5 : 2];
        xform(m, p, self->corners[i]);
        for (k = 0; k < 3; k++) {
            if (self->bboxMin[k] > self->corners[i][k])
                self->bboxMin[k] = self->corners[i][k];
        }
        for (k = 0; k < 3; k++) {
            if (self->corners[i][k] > self->bboxMax[k])
                self->bboxMax[k] = self->corners[i][k];
        }
    }
    xform(m, box, lo);
    xform(m, box + 3, hi);
    for (k = 0; k < 3; k++) {
        self->obb.centre[k] = (lo[k] + hi[k]) * 0.5f;
        self->obb.extent[k] = fabsf((box[3 + k] - box[k]) * self->scale[k]) * 0.5f;
    }

    mat_set_scale(rm, self->scale[0], self->scale[1], self->scale[2]);
    mat_rotate_y(rm, MODEL_DEG_TO_RAD(self->rot[1]));
    mat_rotate_x(rm, MODEL_DEG_TO_RAD(self->rot[0]));
    mat_rotate_z(rm, MODEL_DEG_TO_RAD(self->rot[2]));
    for (i = 0; i < 3; i++) {
        for (k = 0; k < 3; k++)
            self->obb.axis[i][k] = rm[3 * i + k];
        normalize(self->obb.axis[i]);
    }

    for (nodeIndex = 0; nodeIndex < self->volumeBoxCount; ++nodeIndex)
        place_volume_box(&self->volumeBoxes[nodeIndex], self->matrix);
    self->dirty = 1;
}

static void huge_box(float *box)
{
    box[0] = box[1] = box[2] = MODEL_BOX_HUGE;
    box[3] = box[4] = box[5] = -MODEL_BOX_HUGE;
}

void c3d_actor_v1_update_bound(struct C3dActor *base)
{
    struct C3dActorV1 *self = (struct C3dActorV1 *)base;
    struct C3dActor *a = &self->base;
    float box[6], ident[12];

    huge_box(box);
    memcpy(ident, g_identityMatrix.m, sizeof(ident));
    c3d_node_get_bound(self->root, box, ident);
    a->offset[0] = (box[3] + box[0]) * -0.5f;
    a->offset[1] = -box[4];
    a->offset[2] = (box[5] + box[2]) * -0.5f;
    mat_set_translation(a->matrix, a->offset[0], a->offset[1], a->offset[2]);
    place(a);
    bound_from_box(a, box);
}

void c3d_actor_v2_update_bound(struct C3dActor *base)
{
    struct C3dActorV2 *self = (struct C3dActorV2 *)base;
    float box[6], ident[12];
    size_t nodeIndex;

    huge_box(box);
    memcpy(ident, g_identityMatrix.m, sizeof(ident));
    for (nodeIndex = 0; nodeIndex < self->nodeCount; ++nodeIndex)
        c3d_node_get_bound(self->nodes[nodeIndex], box, ident);
    c3d_actor_v2_build_matrix(&self->base);
    bound_from_box(&self->base, box);
}

void c3d_actor_v1_light(struct C3dActor *base, const float *dir, const float *diffuse,
                                 const float *ambient)
{
    struct C3dActorV1 *self = (struct C3dActorV1 *)base;
    if (self->base.dirty == 1)
        c3d_actor_build_matrix(&self->base);
    c3d_node_light(self->root, self->base.matrix, dir, diffuse, ambient, self->base.shadeType);
}

void c3d_actor_v2_light(struct C3dActor *base, const float *dir, const float *diffuse,
                                 const float *ambient)
{
    struct C3dActorV2 *self = (struct C3dActorV2 *)base;
    size_t nodeIndex;

    if (self->base.dirty == 1)
        c3d_actor_v2_build_matrix(&self->base);
    for (nodeIndex = 0; nodeIndex < self->nodeCount; ++nodeIndex)
        c3d_node_light(self->nodes[nodeIndex], self->base.matrix, dir, diffuse, ambient,
                      self->base.shadeType);
}

void c3d_actor_v1_animate(struct C3dActor *base, double t)
{
    struct C3dActorV1 *self = (struct C3dActorV1 *)base;
    c3d_node_v1_animate(self->root, t);
    self->base.dirty = 1;
}

void c3d_actor_v2_animate(struct C3dActor *base, double t)
{
    struct C3dActorV2 *self = (struct C3dActorV2 *)base;
    float frame = (float)((double)self->fps * t / 1000.0);
    size_t nodeIndex;

    for (nodeIndex = 0; nodeIndex < self->nodeCount; ++nodeIndex)
        c3d_node_v2_animate(self->nodes[nodeIndex], (double)frame);
    self->base.dirty = 1;
}

void c3d_actor_v1_tick(struct C3dActor *base)
{
    struct C3dActorV1 *self = (struct C3dActorV1 *)base;
    DWORD now;
    int frame;

    switch (self->base.animType) {
    case 1:
        now = GetTickCount();
        if (now == self->lastTick)
            return;
        frame = (int)(self->base.speed * 100.0f + (float)self->frame);
        self->frame = frame;
        if (frame >= self->frameCount) {
            frame = self->frameCount - 1;
            self->frame = frame;
        }
        break;
    case 2:
        now = GetTickCount();
        if (now == self->lastTick)
            return;
        frame = (int)(self->base.speed * 100.0f + (float)self->frame);
        self->frame = frame;
        if (frame >= self->frameCount) {
            self->frame = 0;
            frame = 0;
        }
        break;
    default:
        return;
    }
    c3d_actor_v1_animate(&self->base, (double)frame);
    self->lastTick = now;
}

void c3d_actor_v2_tick(struct C3dActor *base)
{
    struct C3dActorV2 *self = (struct C3dActorV2 *)base;
    DWORD now, t;

    switch (self->base.animType) {
    case 1:
        t = number_truncate_low32((float)unsigned_to_double(GetTickCount() - self->startTick) * self->base.speed);
        if (!(unsigned_to_double(t) < self->duration))
            t = number_truncate_low32(self->duration - 1.0);
        c3d_actor_v2_animate(&self->base, unsigned_to_double(t));
        break;
    case 2:
        now = GetTickCount();
        t = number_truncate_low32((float)unsigned_to_double(now - self->startTick) * self->base.speed);
        if (!(unsigned_to_double(t) < self->duration)) {
            self->startTick = now;
            t = 0;
        }
        c3d_actor_v2_animate(&self->base, unsigned_to_double(t));
        break;
    }
}

static BYTE mirrored(const struct C3dActor *a)
{
    BYTE m = 0.0f > a->scale[0];

    if (0.0f > a->scale[1])
        m = m == 0;
    if (0.0f > a->scale[2])
        m = m == 0;
    return m;
}

static void fade(struct C3dNode *node)
{
    struct C3dActor *a = node->actor;

    if (a->picked == 1) {
        if (a->alpha > 0x5a)
            a->alpha -= 5;
        else
            a->picked = 0;
    } else if (a->alpha < 0xff) {
        a->alpha += 5;
    }
}

void c3d_actor_v1_render(struct C3dActor *base, const float *view, BYTE clip)
{
    struct C3dActorV1 *self = (struct C3dActorV1 *)base;
    float m[12];
    BYTE mir;

    if (self->base.dirty == 1)
        c3d_actor_build_matrix(&self->base);
    mir = mirrored(&self->base);
    mat_multiply(m, self->base.matrix, view);
    c3d_node_v1_render(self->root, m, mir, clip, self->base.layer);
    fade(self->root);
}

void c3d_actor_v2_render(struct C3dActor *base, const float *view, BYTE clip)
{
    struct C3dActorV2 *self = (struct C3dActorV2 *)base;
    float m[12];
    BYTE mir;
    size_t nodeIndex;

    if (self->base.dirty == 1)
        c3d_actor_v2_build_matrix(&self->base);
    mir = mirrored(&self->base);
    mat_multiply(m, self->base.matrix, view);
    for (nodeIndex = 0; nodeIndex < self->nodeCount; ++nodeIndex) {
        c3d_node_v2_render(self->nodes[nodeIndex], m, mir, clip, self->base.layer);
        fade(self->nodes[nodeIndex]);
    }
}

void c3d_actor_pick(struct C3dActor *self, const struct Ray3 *ray)
{
    struct OrientedBox box;
    int i;

    box.centre = (Vec3){self->obb.centre[0], self->obb.centre[1], self->obb.centre[2]};
    for (i = 0; i < 3; ++i) {
        box.half[i] = self->obb.extent[i];
        box.axis[i] = (Vec3){self->obb.axis[i][0], self->obb.axis[i][1], self->obb.axis[i][2]};
    }
    if (ray_hits_obb(ray, &box))
        self->picked = 1;
}

void c3d_actor_set_speed(struct C3dActor *self, float speed)
{
    self->speed = speed;
    if (speed != 0.0f)
        self->speed = speed / 3.0f;
}

void c3d_actor_v2_set_speed(struct C3dActor *base, float speed)
{
    struct C3dActorV2 *self = (struct C3dActorV2 *)base;
    self->base.speed = speed;
}
