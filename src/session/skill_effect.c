#include "effects/rag/rag_effect.h"
#include "contents/client_lua_calls.h"
#include "contents/sound.h"
#include "session/state.h"
#include "game/context.h"
#include "session/csession.h"
#include "session/session_skills.h"

#include <string.h>

#include "actor/render_ops.h"

BYTE session_play_ground_skill_effect(struct Session *self, int skill, struct Vec3 pos)
{
    BYTE have = 0;
    const char *wave;
    int count, i;

    if (!client_lua_call(g_session1.luaState, "HaveSkillEffectInfo", "d>b", skill, &have) || !have)
        return 0;
    wave = "";
    if (client_lua_call(g_session1.luaState, "GetEffectWaveFileName", "d>s", skill, &wave)
        && strcmp(wave, "") != 0) {
        struct Vec3 origin = { 0.0f, 0.0f, 0.0f };

        sound_play_wave(wave, origin.x, origin.y, origin.z, 0xfa, 0x28, 1.0f);
    }
    count = 0;
    if (client_lua_call(g_session1.luaState, "GetGroundEffectNum", "d>d", skill, &count)) {
        for (i = 0; i < count;) {
            int id = -1;

            i++;
            client_lua_call(g_session1.luaState, "GetGroundEffectID", "dd>d", skill, i, &id);
            if (id == -1)
                continue;
            if (effect_mgr_is_registered(g_effectMgr, id))
                effect_mgr_spawn(g_effectMgr, 0, id, pos, 0, -1, 0, 0);
            else
                game_mode_launch_dummy_effect(game_current_mode(), &pos, id, g_effectDurations[id] + 10, 0);
        }
    }
    return 1;
}

BYTE skill_effect_on_caster(int skill, struct GameActor *caster, struct GameActor *target)
{
    BYTE have = 0;
    const char *wave;
    struct Vec3 pos;
    struct Vec3 *at;
    int caster_aid, target_aid, count, i;

    caster_aid = caster ? caster->aid : 0;
    if (caster) {
        pos = caster->pos;
    } else {
        pos.x = 0.0f;
        pos.y = 0.0f;
        pos.z = 0.0f;
    }
    at = &pos;
    target_aid = target ? target->aid : 0;
    if (!client_lua_call(g_session1.luaState, "HaveSkillEffectInfo", "d>b", skill, &have) || !have)
        return 0;
    wave = "";
    if (client_lua_call(g_session1.luaState, "GetEffectWaveFileName", "d>s", skill, &wave)
        && strcmp(wave, "") != 0) {
        struct Vec3 origin = { 0.0f, 0.0f, 0.0f };

        sound_play_wave(wave, origin.x, origin.y, origin.z, 0xfa, 0x28, 1.0f);
    }
    count = 0;
    if (!client_lua_call(g_session1.luaState, "GetEffectNum", "d>d", skill, &count))
        return 0;
    for (i = 0; i < count;) {
        int id = -1;

        i++;
        if (!client_lua_call(g_session1.luaState, "GetEffectID", "dd>d", skill, i, &id) || id == -1)
            continue;
        if (effect_mgr_is_registered(g_effectMgr, id)) {
            effect_mgr_spawn(g_effectMgr, caster_aid, id, *at, caster_aid != target_aid ? target_aid : 0, -1, 0,
                              0);
        } else if (caster) {
            struct Vec3 offset = { 0.0f, 0.0f, 0.0f };
            void *effect = game_actor_launch_effect(caster, id, offset, 0.0f);

            if (effect) {
                BYTE on_target = 0;

                client_lua_call(g_session1.luaState, "IsOnTarget", "d>b", skill, &on_target);
                if (on_target && target)
                    rag_effect_send_msg(effect, NULL, 0xe, MSG_ARG_PTR(&target->pos), 0, 0);
            }
        }
    }
    return 1;
}

BYTE skill_effect_on_target(int skill, struct GameActor *target)
{
    BYTE have = 0;
    const char *wave;
    int count, i;

    if (!client_lua_call(g_session1.luaState, "HaveSkillEffectInfo", "d>b", skill, &have) || !have)
        return 0;
    wave = "";
    if (client_lua_call(g_session1.luaState, "GetTargetEffectWaveFileName", "d>s", skill, &wave)
        && strcmp(wave, "") != 0) {
        struct Vec3 origin = { 0.0f, 0.0f, 0.0f };

        sound_play_wave(wave, origin.x, origin.y, origin.z, 0xfa, 0x28, 1.0f);
    }
    count = 0;
    if (!client_lua_call(g_session1.luaState, "GetTargetEffectNum", "d>d", skill, &count))
        return 1;
    for (i = 0; i < count;) {
        int id = -1;

        i++;
        client_lua_call(g_session1.luaState, "GetTargetEffectID", "dd>d", skill, i, &id);
        if (id == -1)
            continue;
        if (effect_mgr_is_registered(g_effectMgr, id)) {
            effect_mgr_spawn(g_effectMgr, target->aid, id, target->pos, 0, -1, 0, 0);
        } else {
            struct Vec3 offset = { 0.0f, 0.0f, 0.0f };

            game_actor_launch_effect(target, id, offset, 0.0f);
        }
    }
    return 1;
}
