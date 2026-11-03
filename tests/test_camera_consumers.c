#include "game/context.h"
#include "world/view_types.h"
#include "actor/actor.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct View *current;
struct GameMode *game_current_mode(void) { return (struct GameMode *)current; }
struct View *game_current_view(void) { return current; }

#include "../src/actor/position.c"

struct Session g_session1;
struct Renderer *g_renderer;
struct GameActor *world_get_game_actor_by_aid(struct World *world, unsigned aid) { abort(); }
float float_sqrt(float value) { abort(); }
int game_actor_get_job(struct GameActor *actor) { return actor->job; }
int session_is_pc_job(struct Session *session, int job) { assert(session == &g_session1);return job == 1; }
int session_is_mercenary_job(struct Session *session, int job) { assert(session == &g_session1);return job == 2000; }
float get_sin_deg(int degrees) { abort(); }
float get_cos_deg(int degrees) { abort(); }
void renderer_project_point(struct Renderer *renderer, const Vec3 *in, const float *matrix,
                            float *x, float *y, float *rhw) { abort(); }

int main(void)
{
    struct View view;
    struct RenderObjectOps ops = {0};
    struct GameActor actor = {0};
    struct ScreenBounds bounds = {100,100,0,0,0.5f};
    struct SqQuad quad = {0};
    Vec3 up = {0, -1, 0};
    memset(&view, 0, sizeof view);
    assert(render_object_camera_dir(0, 0) == 0);
    assert(render_object_camera_angle(0) == 0);
    assert(game_object_dir_of(NULL, 0) == 0);
    current = &view;
    view.longitude = 90;
    assert(render_object_camera_dir(0, 0) == 6);
    assert(render_object_camera_dir(0, 1) == 6);
    assert(render_object_camera_angle(0) == 270);
    assert(game_object_dir_of(NULL, 0) == 6);
    view.longitude = 350;
    assert(render_object_camera_dir(30, 0) == 7);
    assert(render_object_camera_dir(30, 1) == 0);
    assert(render_object_camera_angle(30) == 340);
    view_set_up(&view, &up);
    assert(view.up[0] == 0 && view.up[1] == -1 && view.up[2] == 0);
    assert(view.longitude == 350);
    actor.ops = &ops.base;
    ops.base.get_direction = game_actor_dir_of;
    view.longitude = 330;
    actor.job = 1;
    assert(render_object_direction((struct RenderObject *)&actor, 0) == 1);
    actor.base_action = 0x10;
    assert(render_object_direction((struct RenderObject *)&actor, 0) == 1);
    actor.base_action = 0x20;
    assert(render_object_direction((struct RenderObject *)&actor, 0) == 0);
    actor.job = 100;
    actor.base_action = 0;
    assert(render_object_direction((struct RenderObject *)&actor, 0) == 1);
    actor.base_action = 8;
    assert(render_object_direction((struct RenderObject *)&actor, 0) == 0);
    actor.job = 2000;
    assert(render_object_direction((struct RenderObject *)&actor, 0) == 1);
    actor.job = 3000;
    assert(render_object_direction((struct RenderObject *)&actor, 0) == 0);
    ops.base.get_direction = game_object_dir_of;
    assert(render_object_direction((struct RenderObject *)&actor, 0) == 0);
    current = NULL;
    ops.base.get_direction = game_actor_dir_of;
    assert(render_object_direction((struct RenderObject *)&actor, 0) == 0);
    quad.x0 = 20;quad.x1 = 10;quad.top = 30;quad.bottom = 40;
    bounds_add_quad(&bounds, &quad);
    assert(bounds.left == 10 && bounds.right == 20 && bounds.top == 30 && bounds.bottom == 40 && bounds.z == 0.5f);
    quad.x0 = 15;quad.x1 = 40;quad.top = 35;quad.bottom = 50;
    bounds_add_quad(&bounds, &quad);
    assert(bounds.left == 10 && bounds.right == 40 && bounds.top == 30 && bounds.bottom == 50 && bounds.z == 0.5f);
    puts("camera consumers: typed heading, wrapped sprite directions, absent mode and up-vector writes passed");
    return 0;
}
