#include "actor/actor_ops.h"
#include "contents/sound.h"
#include "game/context.h"
#include <limits.h>
#include <string.h>
#include "effects/rag/rag_process_moves.h"
#include "effects/effect_mgr.h"
#include "render/render_api.h"
#include "actor/actor_common.h"

void rag_quake(void)
{
    game_send_message(GAME_MSG_QUAKE, 0, 0, 0, 0);
}

void rag_wave_at(struct RagEffect *self, int frame, const char *wave)
{
    if (self->frame == frame) {
        rag_follow_master(self);
        rag_play_wave_here(self, wave);
    }
}

void rag_wave_plain(const char *wave)
{
    sound_play_wave(wave, 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
}

unsigned char rag_str_step_up80(struct RagEffect *self)
{
    return rag_str_step(self, 0.0f, -80.0f, 0.0f);
}

void rag_str_or_rewind(struct RagEffect *self)
{
    if (!rag_str_step_up80(self))
        rag_str_rewind(self);
}

void rag_flicker_tint(struct RagEffect *self, int first, int last, int always, int even, int r, int g, int b)
{
    if (self->frame < first || self->frame > last)
        return;
    self->master->overlay_flags |= always;
    if (self->frame & 1) {
        render_object_set_argb(self->master, -1, 255, 255, 255);
    } else {
        self->master->overlay_flags |= even;
        render_object_set_argb(self->master, -1, r, g, b);
    }
}

void rag_tint_mark(struct RagEffect *self)
{
    if (self->frame >= 0) {
        self->master->overlay_flags |= 0x1000;
        self->master->overlay_step = (short)self->frame;
    }
}

void rag_tint_early(struct RagEffect *self, int r, int g, int b)
{
    if (self->frame <= 30) {
        self->master->overlay_flags |= 1;
        self->master->overlay_flags |= 8;
        render_object_set_argb(self->master, -1, r, g, b);
    }
}

void rag_tinted_rings(struct RagEffect *self, const char *texture, char style, int r, int g, int b)
{
    rag_tint_mark(self);
    rag_ground_rings(self, texture, style);
    rag_tint_early(self, r, g, b);
}

void rag_master_spin(struct RagEffect *self)
{
    if (self->frame <= 15)
        self->master->roty += 12.0f;
    else if (self->frame <= 30)
        self->master->roty += 18.0f;
    else if (self->frame <= 40)
        self->master->roty += 24.0f;
    else
        self->master->roty += 48.0f;
    if (self->master->roty > 360.0f)
        self->master->roty -= 360.0f;
}

void rag_master_flash(struct RagEffect *self, unsigned argb)
{
    render_object_send(self->master, NULL, 0x58, 1, argb, 0x72, 0);
}

void rag_flying_kick_wave(struct RagEffect *self)
{
    int f = self->frame;

    if (f != 5 && f != 20 && f != 35 && f != 50 && f != 65)
        return;
    rag_follow_master(self);
    sound_play_wave("effect\\T_날라차기.wav",
             0.0f, f <= 50 ? (float)(5 - f) : -50.0f, 0.0f, 250, 40, 1.0f);
}

void rag_streams_tint(struct RagEffect *self)
{
    int v;

    if ((unsigned)(self->frame - 50) > 30)
        return;
    self->master->overlay_flags |= 8;
    v = (125 - self->frame) * 2;
    render_object_set_argb(self->master, -1, v, v, 250);
}

void rag_spell_circles(struct RagEffect *self, const char *texture, char style)
{
    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_BeginSpell.wav");
    rag_casting_circle(self, 45, texture, style);
    rag_casting_circle(self, 25, texture, style);
}

void rag_spell_circles_low(struct RagEffect *self, const char *texture, char style45, char style25)
{
    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave(self, "effect\\EF_BeginSpell.wav", -100.0f);
    rag_casting_circle(self, 45, texture, style45);
    rag_casting_circle(self, 25, texture, style25);
}

void rag_spell_rings(struct RagEffect *self, const char *texture, char style)
{
    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_BeginSpell.wav");
    rag_casting_rings(self, texture, style);
}

void rag_plant_actor(struct RagEffect *self)
{
    struct World *world = game_current_world();
    struct RenderObject *plant;
    void *mem;

    if (!world)
        return;
    mem = mem_alloc(sizeof(struct HalluPlant));
    plant = mem ? (struct RenderObject *)hallu_plant_construct(mem) : NULL;
    if (!plant)
        return;
    plant->pos = self->base.pos;
    switch (self->effect_id) {
    case 1602:
        render_object_load_act_res(plant, MONSTER_DIR "green_plant.act");
        game_actor_set_job((struct GameActor *)plant, 0x438);
        break;
    case 1603:
        render_object_load_act_res(plant, MONSTER_DIR "blue_plant.act");
        game_actor_set_job((struct GameActor *)plant, 0x437);
        break;
    case 1604:
        render_object_load_act_res(plant, MONSTER_DIR "yellow_plant.act");
        game_actor_set_job((struct GameActor *)plant, 0x439);
        break;
    case 1605:
        render_object_load_act_res(plant, MONSTER_DIR "white_plant.act");
        game_actor_set_job((struct GameActor *)plant, 0x43a);
        break;
    case 1606:
        render_object_load_act_res(plant, MONSTER_DIR "red_plant.act");
        game_actor_set_job((struct GameActor *)plant, 0x436);
        break;
    }
    actor_set_job_model(plant, game_actor_get_job((struct GameActor *)plant), ((struct GameActor *)plant)->sex);
    effect_add_world_object(world, plant);
}
