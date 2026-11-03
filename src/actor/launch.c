#include "actor/actor_ops.h"
#include "session/state.h"
#include "actor/actor.h"
#include "game/context.h"
#include "platform/language.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_names.h"

void game_actor_set_hat_effect_by_aid(struct GameActor *self, int aid, int item, unsigned char on)
{
    struct GameActor *actor = session_get_actor_by_aid(&g_session1, aid);

    if (aid == self->aid)
        actor = self;
    if (actor)
        game_actor_set_hat_effect(actor, item, on);
}

void game_actor_make_corpse(struct GameActor * self)
{
    struct GameMode *mode = game_current_mode();
    const char *spr = self->spr ? res_get_name((struct Res *)self->spr) : NULL;
    const char *act = self->act ? res_get_name((struct Res *)self->act) : NULL;
    struct Corpse *copy;
    struct Effect *effect;

    if (!mode)
        return;
    copy = game_mode_make_corpse(mode, self->pos, act, spr, self->base_action, self->motion, self->roty);
    if (!copy)
        return;
    if (self->monster_job > 0)
        copy->monster_job = self->monster_job;
    effect = effect_mgr_find_effect(g_effectMgr, self->aid, 0x946);
    if (effect)
        copy->extra_scale = effect->params.scale;
}

void game_actor_show_siege_emblem(struct GameActor *self)
{
    void *emblems;
    struct GameMode *mode;
    int version, guild;

    if (!session_is_siege_mode(&g_session1))
        return;
    emblems = g_guildMgr;
    version = guild_mgr_emblem_version(emblems, game_actor_get_guild_id(self));
    guild = game_actor_get_guild_id(self);
    mode = game_current_mode();
    if (mode)
        game_mode_make_emblem_wnd(mode, self, guild);
}

void game_actor_show_party_mark(struct GameActor *self, int id)
{
    struct PartyMember member;
    int i;

    if (!game_map_has_party_marks())
        return;
    for (i = 0; i < session_get_party_member_count(&g_session1); i++) {
        session_party_member_at(&g_session1, &member, i);
        if (member.aid == id) {
            if (!able_to_make_effect_is_effect_exist(self, 0xa0)) {
                Vec3 zero = { 0, 0, 0 };

                game_actor_launch_effect(self, 0xa0, zero, 0.0f);
            }
            string_free(&member.map_name);
            string_free(&member.name);
            return;
        }
        string_free(&member.map_name);
        string_free(&member.name);
    }
}
