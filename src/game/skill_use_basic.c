#include "game/actor_names.h"
#include "session/state.h"
#include "game/context.h"
#include "game/skill_use.h"
#include "platform/language.h"
#include "actor/game_actor.h"
#include "core/msg_str.h"
#include "session/session_jobs.h"

static const char WAVE_STONE_CURSE[] = "_stonecurse.wav";

enum {
    SKILL_MG_SRECOVERY = 0x9,
    SKILL_STONE_CURSE = 0x10,
    SKILL_HEAL = 0x1c,
    SKILL_ALL_RESURRECTION = 0x36,
    SKILL_KN_TWOHANDQUICKEN = 0x3c,
    SKILL_KN_AUTOCOUNTER = 0x3d,
    SKILL_PR_SLOWPOISON = 0x47,
    SKILL_BS_OVERTHRUST = 0x71,
    SKILL_HT_REMOVETRAP = 0x7c,
    SKILL_HT_SPRINGTRAP = 0x83,
    SKILL_NPC_SUICIDE = 0xaf,
    SKILL_RG_STEALCOIN = 0xd3,
    SKILL_AM_POTIONPITCHER = 0xe7,
    SKILL_CR_AUTOGUARD = 0xf9,
    SKILL_CR_SPEARQUICKEN = 0x102,
    SKILL_MO_STEELBODY = 0x10c,
    SKILL_MO_BLADESTOP = 0x10d,
    SKILL_EXPLOSION_SPIRITS = 0x10e,
    MSG_COINS_STOLEN = 0x651,
    ACTOR_MSG_PROGRESS = 0x52,
    ACTOR_MSG_PROGRESS_STOP = 0x53,
    ACTOR_MSG_GUARD_EFFECT = 0x7e,
    MSG_EFFECT_MOVE_TO = 0x1d,
};

static const struct SimpleLook basic_looks[] = {
    { 0x006, 0x043, ON_TARGET }, { 0x007, 0x011, ON_CASTER }, { 0x008, 0x00b, ON_CASTER },
    { 0x01d, 0x025, ON_TARGET }, { 0x01e, 0x026, ON_TARGET }, { 0x01f, 0x027, ON_CASTER },
    { 0x020, 0x028, ON_CASTER }, { 0x021, 0x029, ON_CASTER }, { 0x022, 0x02a, ON_TARGET },
    { 0x023, 0x042, ON_TARGET }, { 0x02d, 0x099, ON_CASTER }, { 0x032, 0x012, ON_TARGET },
    { 0x035, 0x015, ON_TARGET }, { 0x042, 0x054, ON_TARGET }, { 0x043, 0x058, ON_TARGET },
    { 0x044, 0x056, ON_TARGET }, { 0x048, 0x04e, ON_TARGET }, { 0x049, 0x070, ON_CASTER },
    { 0x04a, 0x04c, ON_CASTER }, { 0x04b, 0x04b, ON_CASTER }, { 0x04c, 0x057, ON_TARGET },
    { 0x04e, 0x055, ON_TARGET }, { 0x051, 0x03e, ON_CASTER }, { 0x058, 0x01c, ON_TARGET },
    { 0x06c, 0x065, ON_TARGET }, { 0x06f, 0x062, ON_CASTER }, { 0x070, 0x067, ON_TARGET },
    { 0x072, 0x068, ON_CASTER }, { 0x08a, 0x07d, ON_TARGET }, { 0x08b, 0x07e, ON_TARGET },
    { 0x08d, 0x07d, ON_TARGET }, { 0x08e, 0x135, ON_CASTER }, { 0x09b, 0x137, ON_CASTER },
    { 0x09d, 0x0a9, ON_CASTER }, { 0x0a2, 0x0ae, ON_CASTER }, { 0x0a3, 0x0b1, ON_CASTER },
    { 0x0a4, 0x0ad, ON_CASTER }, { 0x0a5, 0x0af, ON_CASTER }, { 0x0a6, 0x0b3, ON_CASTER },
    { 0x0a7, 0x0b2, ON_CASTER }, { 0x0a8, 0x0ac, ON_CASTER }, { 0x0a9, 0x0b0, ON_CASTER },
    { 0x0ad, 0x0b7, ON_CASTER }, { 0x0c4, 0x0d7, ON_CASTER }, { 0x0c9, 0x0d6, ON_CASTER },
    { 0x0cd, 0x0d5, ON_CASTER }, { 0x0d6, 0x114, ON_CASTER }, { 0x0d7, 0x10d, ON_TARGET },
    { 0x0d8, 0x10e, ON_TARGET }, { 0x0d9, 0x10f, ON_TARGET }, { 0x0da, 0x110, ON_TARGET },
    { 0x0ea, 0x12c, ON_TARGET }, { 0x0eb, 0x12c, ON_TARGET }, { 0x0ec, 0x12c, ON_TARGET },
    { 0x0ed, 0x12c, ON_TARGET }, { 0x0fc, 0x0fc, ON_CASTER }, { 0x0ff, 0x0fb, ON_CASTER },
    { 0x100, 0x0f8, ON_TARGET }, { 0x101, 0x0de, ON_CASTER }, { 0x106, 0x0fd, ON_CASTER },
    { 0x114, 0x0f4, ON_CASTER }, { 0x115, 0x0ea, ON_TARGET }, { 0x118, 0x0ff, ON_TARGET },
    { 0x119, 0x100, ON_TARGET }, { 0x11a, 0x101, ON_TARGET }, { 0x11b, 0x102, ON_TARGET },
    { 0x121, 0x0eb, ON_TARGET }, { 0x130, 0x350, ON_CASTER },
};

static void steal_coin_look(const struct SkillCast *cast, struct GameActor *target)
{
    struct RagEffect *effect;

    if ((unsigned)cast->src_aid == (unsigned)g_session1.ownAid) {
        struct String name;
        struct String *got;
        char line[500];

        got = game_mode_get_actor_name(game_current_mode(), &name, target->aid);
        sprintf(line, msg_str(MSG_COINS_STOLEN), string_cstr(got), cast->level);
        string_free(&name);
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)line, 0xc8c8ff, 0xf, 0);
    }
    effect = actor_launch_effect(target, 0x112);
    if (effect) {
        Vec3 at = actor_pos(target);
        effect_set_pos(effect, &at);
    }
    actor_launch_effect(target, 0x10c);
}

int skill_use_basic(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster, *target = cast->target;

    if (play_simple_look(cast, basic_looks, SIMPLE_LOOK_COUNT(basic_looks)))
        return 1;

    switch (cast->skill_id) {
    case 0xc: case 0xf: case 0x12: case 0x18: case 0x108:
        return 1;

    case SKILL_MG_SRECOVERY:
        if (target)
            actor_show_number(target, cast->effect_level, 0xff0000ff);
        return 1;

    case SKILL_STONE_CURSE:
        if (!target || cast->skill_id != SKILL_STONE_CURSE)
            return 1;
        actor_launch_effect(target, 0x17);
        {
            struct Player *player = cast->mode->world->player;

            play_skill_wave_at(WAVE_STONE_CURSE, target->pos.x - player->pos.x, 0.0f,
                               target->pos.z - player->pos.z);
        }
        return 1;

    case SKILL_HEAL:
        skill_use_heal(cast);
        return 1;

    case SKILL_ALL_RESURRECTION:
        if (!target)
            return 1;
        actor_launch_effect(target, 0x4d);
        if (session_is_pc_job(&g_session1, game_actor_get_job(target)))
            actor_launch_effect(target, 0x8c);
        return 1;

    case SKILL_KN_TWOHANDQUICKEN:
        if (caster)
            actor_restart_effect(caster, 0x82);
        return 1;

    case SKILL_KN_AUTOCOUNTER:
        if (!caster)
            return 1;
        render_object_send(caster, NULL, ACTOR_MSG_PROGRESS, cast->level * 400, 0, 0, 0);
        actor_set_state(caster, 0xd, 1);
        if ((unsigned)cast->src_aid != (unsigned)g_session1.ownAid)
            actor_send(caster, ACTOR_MSG_PROGRESS_STOP);
        return 1;

    case SKILL_PR_SLOWPOISON:
        return 1;

    case SKILL_BS_OVERTHRUST:
        if (caster)
            actor_restart_effect(caster, 0x80);
        return 1;

    case SKILL_HT_REMOVETRAP: {
        struct GameActor *unit = (struct GameActor *)world_get_skill_by_aid(cast->mode->world, cast->target_aid);

        if (unit)
            actor_launch_effect(unit, 0x64);
        return 1;
    }

    case SKILL_HT_SPRINGTRAP: {
        struct GameActor *unit;

        if (!caster)
            return 1;
        unit = (struct GameActor *)world_get_skill_by_aid(cast->mode->world, cast->target_aid);
        if (!unit)
            return 1;
        actor_launch_effect(unit, 0x6f);
        if (caster->falcon)
            render_object_send(caster->falcon, NULL, MSG_EFFECT_MOVE_TO, (intptr_t)&unit->pos.x, 0, 0, 0);
        return 1;
    }

    case SKILL_NPC_SUICIDE: {
        struct RagEffect *effect;

        if (!caster)
            return 1;
        effect = actor_launch_effect((struct GameActor *)cast->mode->world->player, 0xb9);
        if (effect)
            effect_set_pos(effect, (const Vec3 *)&caster->pos.x);
        return 1;
    }

    case SKILL_RG_STEALCOIN:
        if (target)
            steal_coin_look(cast, target);
        return 1;

    case SKILL_AM_POTIONPITCHER:
        game_mode_launch_effect_at(cast->mode, 0x12b, caster, target, cast->effect_level);
        return 1;

    case SKILL_CR_AUTOGUARD:
        if (caster)
            actor_send(caster, ACTOR_MSG_GUARD_EFFECT);
        return 1;

    case SKILL_CR_SPEARQUICKEN:
        if (caster)
            actor_restart_effect(caster, 0xfa);
        return 1;

    case SKILL_MO_STEELBODY:
        if (!caster)
            return 1;
        play_skill_wave("effect\\mon_맹룡과강.wav");
        send_quake();
        actor_launch_effect(caster, 0x105);
        render_object_play_motion_range((struct RenderObject *)caster, 0x60, 0, 0, 0x60, 0, 1, 20, 0);
        return 1;

    case SKILL_MO_BLADESTOP:
        if (caster)
            render_object_play_motion_range((struct RenderObject *)caster, 0x60, 3, 3, 0x60, 3, 1, 3000, 0);
        return 1;

    case SKILL_EXPLOSION_SPIRITS:
        if (!caster)
            return 1;
        send_quake();
        play_skill_wave("effect\\mon_폭기.wav");
        actor_restart_effect(caster, 0xcb);
        actor_launch_effect(caster, 0x105);
        render_object_set_script_step_frames((struct RenderObject *)caster, 0x1e);
        render_object_play_motion_range((struct RenderObject *)caster, 0x60, 1, 2, 0x60, 2, 1, 1000, 0);
        return 1;
    }
    return 0;
}
