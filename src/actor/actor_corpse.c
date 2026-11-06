#include "actor/actor_ops.h"
#include "actor/actor_spawns.h"
#include "actor/actor.h"
#include "game/context.h"

/* The make_corpse method of the actors that leave a plain corpse. Apart from
   actor/launch.c, whose other launches reach the session and the windows, so that the
   actor method tables naming it do not. */

void game_actor_make_corpse(struct GameActor * self)
{
    struct GameMode *mode = game_current_mode();
    const char *spr = self->spr ? res_get_name((struct Res *)self->spr) : NULL;
    const char *act = self->act ? res_get_name((struct Res *)self->act) : NULL;
    struct Corpse *copy;
    struct Effect *effect;

    if (!mode)
        return;
    copy = actor_spawn_corpse(mode, self->pos, act, spr, self->base_action, self->motion, self->roty);
    if (!copy)
        return;
    if (self->monster_job > 0)
        copy->monster_job = self->monster_job;
    effect = effect_mgr_find_effect(g_effectMgr, self->aid, 0x946);
    if (effect)
        copy->extra_scale = effect->params.scale;
}
