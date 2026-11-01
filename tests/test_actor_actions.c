#include "actor/actor_ops.h"
#include "actor/granny_pc.h"
#include "resources/animation.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static float delay;
static unsigned expected_action;
static size_t action_count;
static int delay_reads, stops, plays, played_action, played_once;
static int pc_plays, pc_loop;
static struct GrannyPc *expected_pc;
static const struct ActRes *resource = (const struct ActRes *)1;
static struct GrannyPlayback *playback = (struct GrannyPlayback *)2;

float act_res_get_delay(const struct ActRes *r, unsigned action)
{
    assert(r == resource && action == expected_action);
    ++delay_reads;
    return delay;
}

size_t animation_action_count(const struct ActRes *r)
{
    assert(r == resource);
    return action_count;
}

void granny_playback_stop(struct GrannyPlayback *p)
{
    assert(p == playback);
    ++stops;
}

int granny_playback_play(struct GrannyPlayback *p, int action, int once)
{
    assert(p == playback && stops == plays + 1);
    ++plays;
    played_action = action;
    played_once = once;
    return 1;
}

void granny_pc_play_state_anim(struct GrannyPc *pc, int loop)
{
    assert(pc == expected_pc);
    ++pc_plays;
    pc_loop = loop;
}

static void sprite_actions(void)
{
    struct RenderObjectOps ops = {0};
    struct GameActor actor = {0};
    struct RenderObject *object = (struct RenderObject *)&actor;
    struct GameActor before;

    actor.ops = &ops.base;
    actor.act = (struct ActRes *)resource;
    ops.set_action = render_object_set_action_base;
    expected_action = 8;
    delay = 3;
    actor.delay_factor = 2;
    actor.motion = 9;
    actor.motion_finished = 1;
    actor.script_range_on = actor.script_steps_on = actor.script_pingpong_on = 1;
    render_object_set_action(object, 8, 1);
    assert(delay_reads == 1 && actor.motion_delay == 6);
    assert(actor.base_action == 8 && actor.action == 8 && actor.motion == 0);
    assert(actor.play_mode == 1 && actor.motion_finished == 0);
    assert(actor.plays == 1 && actor.delay_factor == 1);
    assert(!actor.script_range_on && !actor.script_steps_on && !actor.script_pingpong_on);

    delay = 0;
    actor.motion_delay = 0.25f;
    render_object_set_action(object, 8, 0);
    assert(actor.motion_delay == 1 && actor.play_mode == 0);
    actor.motion_delay = 5;
    render_object_set_action(object, 8, 0);
    assert(actor.motion_delay == 5);

    ops.set_action = game_actor_set_action;
    actor.object_type = 7;
    actor.glide = 1;
    actor.attack_motion_count = 3;
    action_count = 8;
    before = actor;
    delay_reads = 0;
    render_object_set_action(object, 8, 0);
    assert(!memcmp(&before, &actor, sizeof actor) && delay_reads == 0);
    action_count = 9;
    delay = 2;
    render_object_set_action(object, 8, 0);
    assert(delay_reads == 1 && !actor.glide && actor.attack_motion_count == -1);
    assert(actor.motion_delay == 2 && actor.play_mode == 0);
}

static void model_actions(void)
{
    static const int actions[] = {0,8,0x10,0x18,0x20,0x28,0x30,0x40,0x50,0x58,0x60};
    static const int states[] = {0,1,0,5,11,12,4,3,2,2,0};
    struct RenderObjectOps ops = {0};
    struct C3dGrannyGameActor actor = {0};
    struct GrannyPc pc = {0};
    struct GrannyModelState model = {0};
    size_t i;

    model.playback = playback;
    actor.ops = &ops.base;
    ops.set_action = c3d_granny_game_actor_set_action;
    actor.state = 99;
    actor.anim_time = 7;
    render_object_set_action((struct RenderObject *)&actor, 0x50, 1);
    assert(actor.state == 2 && actor.anim == 2 && actor.anim_time == 7 && stops == 0);
    actor.model = &model;
    for (i = 0; i < sizeof actions / sizeof *actions; ++i) {
        actor.anim_time = 7;
        render_object_set_action((struct RenderObject *)&actor, actions[i], (int)i % 3);
        assert(actor.state == states[i] && actor.anim == states[i] && actor.anim_time == 0);
        assert(played_action == states[i] && played_once == ((int)i % 3 == 1));
    }
    assert(stops == 11 && plays == 11);

    pc.ops = &ops.base;
    expected_pc = &pc;
    ops.set_action = granny_pc_set_action;
    pc.state = 99;
    render_object_set_action((struct RenderObject *)&pc, 0x10, 1);
    assert(pc.state == 99 && pc_plays == 0);
    pc.model = &model;
    for (i = 0; i < sizeof actions / sizeof *actions; ++i) {
        render_object_set_action((struct RenderObject *)&pc, actions[i], (int)i % 3);
        assert(pc.state == (actions[i] == 0x10 ? 6 : states[i]));
        assert(pc_loop == (int)i % 3);
    }
    assert(pc_plays == 11);
}

static void head_extents(void)
{
    struct GameActorOps ops = {0};
    struct Pc pc = {0}, before;
    struct ActorHeadExtent extent;

    pc.ops = &ops.base.base;
    pc.head_clip[0] = -7.5f;
    pc.head_clip[1] = 12.25f;
    pc.head_clip[2] = 20.0f;
    pc.head_clip[3] = -3.5f;
    before = pc;
    ops.get_head_extent = pc_get_head_extent;
    extent = actor_get_head_extent(&pc);
    assert(extent.width == 27.5f && extent.height == -15.75f);
    assert(!memcmp(&pc, &before, sizeof pc));

    pc.head_clip[0] = NAN;
    extent = actor_get_head_extent(&pc);
    assert(isnan(extent.width) && extent.height == -15.75f);
    ops.get_head_extent = game_actor_get_head_extent;
    extent = actor_get_head_extent(&pc);
    assert(extent.width == 0.0f && extent.height == 0.0f);
}

int main(void)
{
    sprite_actions();
    model_actions();
    head_extents();
    puts("Actor actions: sprite timing, pet guards and both model mappings and head extents passed");
    return 0;
}
