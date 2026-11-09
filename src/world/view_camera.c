#include "actor/render_ops.h"
#include "world/model_types.h"
#include "world/ground_api.h"
#include "world/hover_cell.h"
#include "session/state.h"
#include "world/camera.h"
#include "world/mode.h"
#include "game/context.h"

#include <math.h>
#include <stdlib.h>
#include "actor/types.h"
#include "game/talk_type.h"
#include "actor/sprite_quad.h"
#include "game/camera.h"
#include "game/mode_callbacks.h"
#include "platform/clock.h"

int g_drawnCellMaxX = 0;

int g_drawnCellMaxY = 0;

int g_drawnCellMinX = 0;

int g_drawnCellMinY = 0;

static struct Once g_quakeDistanceGuard;

static float g_quakeDistanceOffset = 0;

static int g_quakeFrameDrawn = 0;

static struct Once g_quakeLatitudeGuard;

static float g_quakeLatitudeOffset = 0;

static struct Once g_quakeSidewaysGuard;

static float g_quakeSidewaysOffset = 0;
static void view_process_quake(struct View *self);

void view_add_longitude(struct View *self, float d)
{
    self->destLongitude = d + self->destLongitude;
}

static void eye_offset(const struct View *v, const float *m, float *out)
{
    const float z = 0.0f;
    float nd = -v->distance;
    int i;

    for (i = 0; i < 3; i++)
        out[i] = m[3 + i] * z + m[i] * z + nd * m[6 + i] + m[9 + i];
}

static void clamp_eye_to_ground(float eye[3])
{
    struct World *world = game_current_world();
    float height;

    if (!world || !world->attr)
        return;
    /* custom: the unlocked zoom (game/camera.h) can put the eye far outside the map, where the
       plain lookup answers 0 and the eye could sink below the map's ground */
    height = c3d_attr_get_height_nearest(world->attr, eye[0], eye[2]);
    if (eye[1] > height)
        eye[1] = height;
}

static void view_build_view_matrix(struct View *self)
{
    float m[12], off[3];

    mat_set_rotate_x(m, VIEW_DEG_TO_RAD(self->latitude));
    mat_rotate_y(m, VIEW_DEG_TO_RAD(self->longitude));
    eye_offset(self, m, off);
    self->from[1] = off[1];
    self->from[2] = off[2];
    self->from[1] = self->at[1] + self->from[1];
    self->from[2] = self->at[2] + self->from[2];
    self->from[0] = off[0] + self->at[0];
    clamp_eye_to_ground(self->from);
    {
        Vec3 eye_point = {self->from[0], self->from[1], self->from[2]};
        Vec3 target_point = {self->at[0], self->at[1], self->at[2]};
        Vec3 up_direction = {self->up[0], self->up[1], self->up[2]};
        mat_look_at(self->viewMatrix, &eye_point, &target_point, &up_direction);
    }
    mat_invert(self->invViewMatrix, self->viewMatrix);
    g_viewGroundUp[0] = self->viewMatrix[3];
    g_viewGroundUp[1] = self->viewMatrix[4];
    g_viewGroundUp[2] = self->viewMatrix[5];
}

void view_get_view_dir(struct View *self, Vec3 *dir)
{
    float inv;

    dir->y = self->at[1] - self->from[1];
    dir->x = self->at[0] - self->from[0];
    dir->z = self->at[2] - self->from[2];
    inv = 1.0f / sqrtf(dir->y * dir->y + dir->x * dir->x + dir->z * dir->z);
    dir->x = dir->x * inv;
    dir->y = dir->y * inv;
    dir->z = dir->z * inv;
}

/* custom: what the camera divides the rest of its way by this call. The original moved it a tenth
   of the way once per game tick (16 ms, frame_timer_step_count) and ran as many ticks as each
   frame's time held, so on a 60 Hz display most frames moved it one step and every 24th frame two
   (more often where the frame's start wavered across a tick), and the whole view jerked a few
   times a second while the camera moved. It now moves by the time since the last call, a tenth
   per 16 ms (exactly 10 for 16 ms, as the scene harness's frames take), so a frame's further
   ticks move it by next to nothing; 0 where no time has passed. */
static float ease_divisor(struct View *self)
{
    int64_t now = platform_perf_counter(), freq = platform_perf_frequency();
    double ms = self->easedAt && freq > 0 ? (double)(now - self->easedAt) * 1000.0 / (double)freq : 16.0;

    self->easedAt = now;
    if (!(ms > 0.0))
        return 0.0f;
    return (float)(1.0 / (1.0 - pow(0.9, ms / 16.0)));
}

/* `from` moved towards `to` by the share ease_divisor gives, as the original moved it a tenth */
static float eased(float from, float to, float divisor)
{
    return divisor > 0.0f ? (to - from) / divisor + from : from;
}

void view_process_camera(struct View *self)
{
    float d, x, y, z, t, bias, k, r, depth_tilt, divisor = ease_divisor(self);

    while (self->destLongitude >= 360.0f)
        self->destLongitude -= 360.0f;
    while (0.0f > self->destLongitude)
        self->destLongitude += 360.0f;
    d = self->destLongitude - self->longitude;
    while (d >= 180.0f)
        d -= 360.0f;
    while (-180.0f > d)
        d += 360.0f;
    if (divisor > 0.0f)
        self->longitude = d / divisor + self->longitude;
    while (self->longitude >= 360.0f)
        self->longitude -= 360.0f;
    while (0.0f > self->longitude)
        self->longitude += 360.0f;

    self->latitude = eased(self->latitude, self->destLatitude, divisor);
    self->distance = eased(self->distance, self->destDistance, divisor);
    x = self->destAt[0];
    y = self->destAt[1];
    z = self->destAt[2];
    if (g_isFixedCamera == 0) {
        x = eased(self->at[0], x, divisor);
        y = eased(self->at[1], y, divisor);
        z = eased(self->at[2], z, divisor);
    }
    self->at[2] = z;
    self->at[1] = y;
    self->at[0] = x;

    /* custom: the upright sprites' depth per screen row models a figure foreshortened by the tilt
       (tan), but sprites keep their full height on screen. Past the original's tilt range that step
       grows without bound and pushes a sprite's top through the near plane, so it takes the tilt
       held to -65..-25; inside that range nothing changes. The flat sprites take the ground's
       own slope from the view matrix (g_viewGroundUp, sprite_flat_row_rhw). */
    depth_tilt = self->latitude < -65.0f ? -65.0f : self->latitude > -25.0f ? -25.0f : self->latitude;
    t = get_sin_deg((int)-depth_tilt) / get_cos_deg((int)-depth_tilt);
    k = g_spriteDepthScale * t;
    r = g_renderer->scaleY;
    g_spriteRowDepthStep = k / r;
    bias = get_sin_deg((int)-self->latitude) / get_sin_deg(45) * 9.0e-6f;
    g_spriteLayerDepthStep = g_fixedViewDistanceMax / self->distance * bias;
    g_spriteScaleDepthBias = g_fixedViewDistanceMax / self->distance * 3.6e-5f;
}

void view_on_calc_view_info(struct View *self)
{
    const struct RenderObject *player = (const struct RenderObject *)self->world->player;

    self->destAt[0] = player->pos.x;
    self->destAt[1] = player->pos.y;
    self->destAt[2] = player->pos.z;
    view_process_camera(self);
    view_process_quake(self);
    view_build_view_matrix(self);
}

void view_set_quake(struct View *self, int on, int duration, float sideways,
                               float distance, float latitude)
{
    DWORD now;

    self->quaking = on;
    now = platform_ticks_ms();
    self->quakeDistance = distance;
    self->quakeStart = now;
    self->quakeSideways = sideways;
    self->quakeLatitude = latitude;
    if (duration == 0)
        self->quakeDuration = 650;
    else if (duration == 1)
        self->quakeDuration = 200;
    else
        self->quakeDuration = duration;
}

void view_set_quake_amplitude(struct View *self, float sideways, float distance,
                                        float latitude)
{
    self->quakeSideways = sideways;
    self->quakeLatitude = latitude;
    self->quakeDistance = distance;
}

static float quake_step(float divisor)
{
    int r = rand() % 5;
    return (float)((r < 0 ? 0 : r) - 2) / divisor;
}

static void view_process_quake(struct View *self)
{
    float f, z0;

    if (!self->quaking)
        return;
    if (platform_ticks_ms() >= self->quakeDuration + self->quakeStart) {
        self->at[0] = self->destAt[0];
        self->at[1] = self->destAt[1];
        self->at[2] = self->destAt[2];
        self->distance = self->destDistance;
        self->quaking = 0;
        self->latitude = self->destLatitude;
        return;
    }
    if (!g_quakeFrameDrawn)
        return;
    g_quakeFrameDrawn = 0;

    if (once_begin(&g_quakeSidewaysGuard)) {
        g_quakeSidewaysOffset = self->quakeSideways;
        once_complete(&g_quakeSidewaysGuard);
    }
    {
        int r = rand() % 5;
        float s = get_cos_deg((int)self->longitude);
        f = (float)((r < 0 ? 0 : r) - 2) / 10.0f + g_quakeSidewaysOffset;
        self->at[0] = self->at[0] + s * f;
    }
    {
        int r = rand() % 5;
        float c;
        z0 = self->at[2];
        c = get_sin_deg((int)self->longitude);
        f = (float)((r < 0 ? 0 : r) - 2) / 10.0f + g_quakeSidewaysOffset;
        g_quakeSidewaysOffset = -g_quakeSidewaysOffset;
        self->at[2] = z0 - c * f;
    }

    if (once_begin(&g_quakeDistanceGuard)) {
        g_quakeDistanceOffset = self->quakeDistance;
        once_complete(&g_quakeDistanceGuard);
    }
    f = quake_step(10.0f) + g_quakeDistanceOffset;
    g_quakeDistanceOffset = -g_quakeDistanceOffset;
    self->distance = f + self->distance;

    if (once_begin(&g_quakeLatitudeGuard)) {
        g_quakeLatitudeOffset = self->quakeLatitude;
        once_complete(&g_quakeLatitudeGuard);
    }
    f = quake_step(15.0f) + g_quakeLatitudeOffset;
    g_quakeLatitudeOffset = -g_quakeLatitudeOffset;
    self->latitude = f + self->latitude;
}

static void cells_min(int *v, int c)
{
    if (*v > c)
        *v = c;
}

static void cells_max(int *v, int c)
{
    if (*v < c)
        *v = c;
}

/* custom: the models and ground the view left out whose shadows can fall into it
   (render/renderer.h): their casters only, the models animated as the view's are */
static void view_cast_shadows(struct SceneGraphNode *node, int level, const float *view)
{
    struct Renderer *r = g_renderer;
    size_t index;
    int i;

    if (!renderer_shadow_reaches(r, node->centre, node->half, NULL))
        return;
    if (level < RSW_TREE_DEPTH) {
        for (i = 0; i < 4; i++)
            if (node->child[i])
                view_cast_shadows(node->child[i], level + 1, view);
        return;
    }
    for (index = 0; index < node->objects.count; ++index) {
        struct C3dActor *a = node->objects.data[index];

        if (a->castFrame == r->frame || !renderer_shadow_reaches(r, a->obb.centre, a->obb.extent, a->obb.axis))
            continue;
        if (a->lastFrame != r->frame) {
            if (g_isSkipFrame > 0)
                a->ops->tick(a);
            a->lastFrame = r->frame;
        }
        a->ops->cast(a, view);
    }
    if (node->ground)
        c3d_ground15_cast(node->ground, view, node->groundCell);
}

void view_on_render(struct View *self)
{
    struct Renderer *r;
    struct ListNode *n;
    size_t leaf_index;
    float *view = self->viewMatrix;

    g_quakeFrameDrawn = 1;
    r = g_renderer;
    view_frustum_build(&self->frustum, r->proj[2], r->proj[3], view, &self->world->root);
    /* custom: the models' shadows, fitted to what the camera sees (render/renderer.h) */
    renderer_shadow_frame(r, view, self->at, self->distance);
    renderer_set_camera(r, view);
    g_drawnCellMinY = 999;
    g_drawnCellMinX = 999;
    g_drawnCellMaxY = 0;
    g_drawnCellMaxX = 0;

    for (leaf_index = 0; leaf_index < self->frustum.crossing_count; ++leaf_index) {
        struct SceneGraphNode *leaf = self->frustum.crossing[leaf_index];
        size_t index;

        for (index = 0; index < leaf->objects.count; ++index) {
            struct C3dActor *a = leaf->objects.data[index];
            int res;

            if (a->lastFrame == g_renderer->frame)
                continue;
            res = view_frustum_test_obb(&self->frustum, &a->obb);
            if (g_isSkipFrame > 0)
                a->ops->tick(a);
            if (res != FRUSTUM_OUTSIDE)
                a->ops->render(a, view);
            a->lastFrame = g_renderer->frame;
        }
        c3d_ground15_render(leaf->ground, view, leaf->groundCell);
        cells_min(&g_drawnCellMinX, leaf->groundCell[0]);
        cells_max(&g_drawnCellMaxX, leaf->groundCell[2]);
        cells_min(&g_drawnCellMinY, leaf->groundCell[1]);
        cells_max(&g_drawnCellMaxY, leaf->groundCell[3]);
    }

    for (leaf_index = 0; leaf_index < self->frustum.inside_count; ++leaf_index) {
        struct SceneGraphNode *leaf = self->frustum.inside[leaf_index];
        size_t index;

        for (index = 0; index < leaf->objects.count; ++index) {
            struct C3dActor *a = leaf->objects.data[index];

            if (g_isSkipFrame > 0)
                a->ops->tick(a);
            if (a->lastFrame != g_renderer->frame) {
                a->ops->render(a, view);
                a->lastFrame = g_renderer->frame;
            }
        }
        c3d_ground15_render(leaf->ground, view, leaf->groundCell);
        cells_min(&g_drawnCellMinY, leaf->groundCell[1]);
        cells_max(&g_drawnCellMaxY, leaf->groundCell[3]);
        cells_min(&g_drawnCellMinX, leaf->groundCell[0]);
        cells_max(&g_drawnCellMaxX, leaf->groundCell[2]);
    }

    view_cast_shadows(&self->world->root, 0, view);

    navi_manager_draw_path(&g_navi, view);
    if (g_hoverCellHidden == 0)
        c3d_ground15_render_attr_tile(self->world->ground, view,
                                                          g_hoverCellX, g_hoverCellY, g_hoverCellColor);
    if (g_session1.homun_ai_flags[0] && g_session1.homun_command[1] && g_session1.homun_command[2])
        c3d_ground15_render_attr_tile(self->world->ground, view,
                                                          g_session1.homun_command[1], g_session1.homun_command[2], (int)0xfeff0000);
    for (n = self->world->gameObjects.head->next; n != self->world->gameObjects.head; n = n->next) {
        struct GameObject *object = *LIST_VALUE(n, struct GameObject *);
        game_object_render(object, view);
    }
    effect_mgr_render(g_effectMgr);

    view_frustum_clear(&self->frustum);
}
