#include "actor/sprite_render.h"
#include "actor/render_ops.h"
#include "session/state.h"
#include "world/view_types.h"
#include "game/context.h"
#include "actor/actor.h"
#include "game/gamemode.h"
#include "actor/game_actor.h"
#include "session/session_jobs.h"

static float facing_of(float dx, float dz)
{
    float deg = (float)atan2((double)dx, (double)-dz) / 3.141592f * 180.0f;

    if (deg >= 360.0f)
        deg -= 360.0f;
    if (deg < 0.0f)
        deg += 360.0f;
    return deg;
}

float render_object_calc_dir(float x1, float z1,
                                        float x2, float z2)
{
    return facing_of(x2 - x1, z2 - z1);
}

void render_object_turn_toward(struct RenderObject *self, float x, float z)
{
    float dx = x - self->pos.x;
    float dz = z - self->pos.z;

    if (dx == 0.0f && dz == 0.0f)
        return;
    self->roty = facing_of(dx, dz);
}

float render_object_calc_ground_dist(struct RenderObject *self, float x, float z)
{
    float dx = x - self->pos.x;
    float dz = z - self->pos.z;

    return float_sqrt(dz * dz + dx * dx);
}

float render_object_calc_dist(struct RenderObject *self, float x, float y, float z)
{
    float dx = x - self->pos.x;
    float dz = z - self->pos.z;
    float dy = y - self->pos.y;
    float ground = float_sqrt(dz * dz + dx * dx);

    return float_sqrt(ground * ground + dy * dy);
}

static float view_facing(float roty)
{
    float deg = -game_current_view()->longitude - roty;

    while (0.0f > deg)
        deg += 360.0f;
    while (deg >= 360.0f)
        deg -= 360.0f;
    return deg;
}

static int dir_rounded(float deg)
{
    if (22.5f > deg || deg > 337.5f)
        return 0;
    return (int)(((double)deg + 22.5) / 45.0);
}

int render_object_camera_dir(float roty, int rounded)
{
    struct GameMode *mode = game_current_mode();
    float deg;

    if (!mode)
        return 0;
    deg = view_facing(roty);
    if (rounded)
        return dir_rounded(deg);
    return (int)(deg / 45.0f);
}

int game_object_dir_of(struct GameObject *self, float roty)
{
    struct GameMode *mode = game_current_mode();

    if (!mode)
        return 0;
    return (int)(view_facing(roty) / 45.0f);
}

int game_actor_dir_of(struct GameObject *object, float roty)
{
    struct GameActor *self = (struct GameActor *)object;
    struct GameMode *mode = game_current_mode();
    float deg;
    int action;

    if (!mode)
        return 0;
    deg = view_facing(roty);
    if (session_is_pc_job(&g_session1, game_actor_get_job(self))) {
        action = self->base_action;
        if (action == 0 || action == 8 || action == 0x10)
            return dir_rounded(deg);
    }
    if (game_actor_get_job(self) > 0x2d && game_actor_get_job(self) < 0x3e8 && self->base_action == 0)
        return dir_rounded(deg);
    if (session_is_mercenary_or_homun_s_job(&g_session1, game_actor_get_job(self))) {
        action = self->base_action;
        if (action == 0 || action == 8 || action == 0x10)
            return dir_rounded(deg);
    }
    return (int)(deg / 45.0f);
}

int render_object_camera_angle(float roty)
{
    struct GameMode *mode = game_current_mode();
    int deg;

    if (!mode)
        return 0;
    deg = (int)(-game_current_view()->longitude - roty);
    if (deg < 0)
        deg = (int)((unsigned)deg + 360 + ((unsigned)(-1 - deg) / 360) * 360);
    if (deg >= 360)
        return (int)((unsigned)deg % 360);
    return deg;
}

unsigned char render_object_get_actor_pos(struct RenderObject *self, int aid, Vec3 *out)
{
    struct GameMode *mode = game_current_mode();
    struct World *world = mode->world;
    struct RenderObject *actor;

    if (aid == g_session1.ownAid) {
        *out = ((struct RenderObject *)(struct GameActor *)world->player)->pos;
        return 1;
    }
    actor = (struct RenderObject *)world_get_game_actor_by_aid(world, aid);
    if (actor) {
        *out = actor->pos;
        return 1;
    }
    actor = (struct RenderObject *)(struct GameActor *)mode->world->player;
    out->x = self->pos.x - (actor->pos.x - self->pos.x);
    out->y = self->pos.y - (actor->pos.y - self->pos.y);
    out->z = self->pos.z - (actor->pos.z - self->pos.z);
    return 0;
}

void render_object_set_screen_position(struct RenderObject *self, float x, float y)
{
    self->screen_x = (int)x;
    self->screen_y = (int)y;
}

void view_set_up(struct View *view, const Vec3 *v)
{
    view->up[0] = v->x;
    view->up[1] = v->y;
    view->up[2] = v->z;
}

void render_object_render_projected(struct RenderObject *self, const void *view)
{
    float x, y, rhw;

    renderer_project_point(g_renderer, &self->pos, view, &x, &y, &rhw);
    render_object_set_screen_position(self, x, y);
}

void bounds_add_quad(struct ScreenBounds *box, const struct SqQuad *r)
{
    float lo = r->x0 < r->x1 ? r->x0 : r->x1;
    float hi = r->x0 > r->x1 ? r->x0 : r->x1;

    if (box->left > lo)
        box->left = lo;
    if (box->top > r->top)
        box->top = r->top;
    if (hi > box->right)
        box->right = hi;
    if (r->bottom > box->bottom)
        box->bottom = r->bottom;
}
