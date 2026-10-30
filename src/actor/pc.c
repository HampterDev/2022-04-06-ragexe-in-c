#include "actor/actor_ops.h"
#include "contents/client_lua_calls.h"
#include "core/object_type.h"
#include "session/state.h"
#include "actor/actor.h"
#include "actor/pc.h"
#include "session/session_jobs.h"

void game_actor_set_hat_effect(struct GameActor *self, int item, unsigned char on)
{
    struct Pc *pc = object_as_type(self, OBJECT_PC);
    int hat = item - 0x950;
    int effect = 0;
    unsigned char pair = 0;
    int launch;
    Vec3 zero = { 0, 0, 0 };

    client_lua_call(g_session1.luaState, "GetHatEffectID", "d>d", hat, &effect);
    client_lua_call(g_session1.luaState, "GetIsEffectPair", "d>b", hat, &pair);
    if (effect != -1) {
        if (effect_mgr_has_effect(g_effectMgr, self->aid, effect) == 1)
            effect_mgr_remove_type(g_effectMgr, self->aid, effect);
        else if (able_to_make_effect_is_effect_exist(self, effect) == 1)
            render_object_send(self, NULL, 0x6d, effect, 0, 0, 0);

        if (effect == 0xfe) {
            if (pc && (self->aid == g_session1.ownGid || session_is_pc_job(&g_session1, game_actor_get_job(self)))) {
                if (on == 1)
                    pc->hat_flags |= 0x10;
                else
                    pc->hat_flags ^= 0x10;
                launch = (pc->virtue & 0x10) || (pc->hat_flags & 0x10);
            } else {
                launch = on == 1;
            }
        } else {
            launch = on != 0;
        }
        if (launch && !effect_mgr_spawn(g_effectMgr, self->aid, effect, zero, 0, -1, 0, 0))
            game_actor_launch_effect(self, effect, zero, 0);
        if (!pair)
            return;
    }
    if (!g_effectMgr)
        return;
    if (on)
        effect_mgr_spawn(g_effectMgr, self->aid, item, zero, 0, -1, 0, 0);
    else
        effect_mgr_remove_type(g_effectMgr, self->aid, item);
}

unsigned char pc_get_skip_arrow(struct GameActor * base_actor)
{
    struct Pc * self = (struct Pc *)base_actor;

    return self->skip_arrow;
}

int pc_get_sprite_variant(struct GameActor * base_actor)
{
    struct Pc * self = (struct Pc *)base_actor;

    return self->madogear_type;
}

int pc_get_char_id(struct GameActor * base_actor)
{
    struct Pc * self = (struct Pc *)base_actor;

    return self->char_id;
}

int64_t pc_get_weapon(struct GameActor * base_actor)
{
    struct Pc * self = (struct Pc *)base_actor;

    return self->weapon;
}
