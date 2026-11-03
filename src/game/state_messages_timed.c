#include "actor/actor_ops.h"
#include "resources/state_icon_sheet.h"
#include "contents/sound.h"
#include "session/state.h"
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "game/packet_handlers.h"
#include "game/actor_packets.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"
#include "game/own_states.h"
#include "game/skill_cooldowns.h"
#include "game/rodex_mail.h"
#include "game/state_change_helpers.h"
#include "actor/game_actor.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "session/session_party.h"
#include "world/world.h"

enum {
    EFST_ATTACK_COOLDOWN = 0x40,
    EFST_TIMER_KIND_FIRST = 0x16b,
    STATE_NO_TIME = 9999,
    STATE_END_NEVER = 999999,
    MSG_STATE_CHANGE = 0x6e,
    ACTOR_MSG_SLEEP_MARK_ON = 0x45,
};

static void actor_state_msg(void *actor, int index, unsigned char state, int val1, int val2)
{
    render_object_send(actor, NULL, MSG_STATE_CHANGE, index, state, val1, val2);
}

static struct RagEffect *launch(void *actor, int effect_id)
{
    Vec3 here = { 0.0f, 0.0f, 0.0f };

    return game_actor_launch_effect((struct GameActor *)actor, effect_id, here, 0.0f);
}

static void spawn(int aid, int effect_id)
{
    Vec3 at = { 0.0f, 0.0f, 0.0f };

    effect_mgr_spawn(g_effectMgr, aid, effect_id, at, 0, -1, 0, 0);
}

static void spell_fist_effects(int aid, int kind)
{
    switch (kind) {
    case 0x13:
        spawn(aid, 0x681);
        spawn(aid, 0x682);
        break;
    case 0x14:
        spawn(aid, 0x685);
        break;
    case 0x0e:
        spawn(aid, 0x683);
        spawn(aid, 0x684);
        break;
    }
}

static void cooldown_update(struct SkillCooldown *cooldown, int skill_id, int remain_ms, int total_ms)
{
    struct SkillCooldownNode *it;

    for (it = g_session1.skillDelays.head; it != NULL; it = it->next)
        if (it->value.skill_id == skill_id)
            break;
    cooldown->skill_id = skill_id;
    cooldown->end_time = cooldown_clock_now() + remain_ms;
    cooldown->total_ms = total_ms;
    if (it == NULL)
        skill_cooldowns_append(&g_session1.skillDelays, cooldown);
    else
        it->value.end_time = cooldown->end_time;
}

static void cooldown_add(struct SkillCooldown *cooldown, int skill_id, int remain_ms, int total_ms)
{
    cooldown->skill_id = skill_id;
    cooldown->end_time = cooldown_clock_now() + remain_ms;
    cooldown->total_ms = total_ms;
    skill_cooldowns_append(&g_session1.skillDelays, cooldown);
}

static int own_entry_matches(const struct OwnStateEntry *it, int index, int value)
{
    return it->index == index && (index != EFST_EQUIPMENT_EFFECT || it->value == value);
}

static int state_known(int index)
{
    return skill_action_mgr_has_efst_action(skill_action_mgr_instance(), index) == 1;
}

static void own_state_list(int index, int remain_ms, int val1, int val2, int total_ms)
{
    struct SkillCooldown cooldown = {0};
    struct OwnStateEntry entry = {0};
    size_t i;
    unsigned end;
    int skill_id;

    if (index == 0x2e || index == 0x2ac) {
        cooldown_add(&cooldown, index == 0x2e ? 0x241 : 0x270f, remain_ms, total_ms);
        skill_cooldown_clear(&cooldown);
    } else if (index == 0xaf) {
        for (skill_id = 0x2710; skill_id < 0x2724; skill_id++)
            cooldown_update(&cooldown, skill_id, remain_ms, total_ms);
        skill_cooldown_clear(&cooldown);
    }

    entry.index = index;
    if (remain_ms == STATE_NO_TIME)
        end = STATE_END_NEVER;
    else end = timeGetTime() + remain_ms;
    entry.end_tick = end;

    if ((unsigned)(index - EFST_TIMER_KIND_FIRST) <= 4)
        entry.value = val1 & 3;
    else if (index == 0x153 || index == EFST_EQUIPMENT_EFFECT)
        entry.value = val1;
    if (state_known(index))
        entry.value = val1;

    for (i = 0; i < own_states_count(); ) {
        const struct OwnStateEntry *it = own_states_at(i);
        if (own_entry_matches(it, index, entry.value)) {
            icon_sheet_remove_timer(g_stateIconSheet, (unsigned)it->index);
            own_states_erase_at(i);
        } else {
            ++i;
        }
    }
    entry.total_ms = total_ms;
    entry.remain_ms = remain_ms;
    if (end != STATE_END_NEVER)
        icon_sheet_set_timer(g_stateIconSheet, index, total_ms, end - (unsigned)remain_ms, (unsigned)total_ms - (unsigned)remain_ms);
    own_states_push(&entry);
}

static unsigned minutes_left(int remain_ms)
{
    return (unsigned)remain_ms / 60000;
}

static void say_string(char *text, struct String *s)
{
    strcpy(text, string_cstr(s));
    string_free(s);
}

static int change_begins_without_line(int index)
{
    switch (index) {
    case 0x00: case 0x01: case 0x0b: case 0x1b: case 0x1c: case 0x22: case 0x3d: case 0x56:
    case 0x57: case 0x5f: case 0x61: case 0x62: case 0x6b: case 0x6e: case 0x82: case 0x89:
    case 0x8b: case 0x8d: case 0x8f: case 0x91: case 0xb5: case 0xb8: case 0xba: case 0xbf:
    case 0x11a: case 0x11e: case 0x139: case 0x13a: case 0x13c: case 0x148: case 0x14a: case 0x14d:
    case 0x14e: case 0x150: case 0x151: case 0x163: case 0x165: case 0x176: case 0x177: case 0x179:
    case 0x17b: case 0x189: case 0x18a: case 0x1ad: case 0x1ae: case 0x1b8: case 0x1c2: case 0x1cd:
    case 0x1cf: case 0x1d0: case 0x1d8: case 0x1d9: case 0x1dc: case 0x23b: case 0x241: case 0x244:
    case 0x245: case 0x248: case 0x263: case 0x265: case 0x26a: case 0x288: case 0x289: case 0x29e:
    case 0x365: case 0x461:
        return 1;
    }
    return (index >= 0x03 && index <= 0x05) || (index >= 0x36 && index <= 0x39) ||
        (index >= 0x67 && index <= 0x69) || (index >= 0x70 && index <= 0x76) ||
        (index >= 0x78 && index <= 0x7d) || (index >= 0x84 && index <= 0x87) ||
        (index >= 0x95 && index <= 0x99) || (index >= 0x9c && index <= 0xa0) ||
        (index >= 0xa5 && index <= 0xa7) || (index >= 0xa9 && index <= 0xab) ||
        (index >= 0xc5 && index <= 0xc9) || (index >= 0xcb && index <= 0xec) ||
        (index >= 0xee && index <= 0xf0) || (index >= 0xfe && index <= 0x103) ||
        (index >= 0x105 && index <= 0x10e) || (index >= 0x121 && index <= 0x12e) ||
        (index >= 0x13e && index <= 0x143) || (index >= 0x155 && index <= 0x161) ||
        (index >= 0x168 && index <= 0x16a) || (index >= 0x17d && index <= 0x186) ||
        (index >= 0x18c && index <= 0x18f) || (index >= 0x191 && index <= 0x195) ||
        (index >= 0x197 && index <= 0x1a3) || (index >= 0x1a5 && index <= 0x1a7) ||
        (index >= 0x1a9 && index <= 0x1ab) || (index >= 0x1b4 && index <= 0x1b6) ||
        (index >= 0x1ba && index <= 0x1bf) || (index >= 0x1eb && index <= 0x1f0) ||
        (index >= 0x25d && index <= 0x260) || (index >= 0x420 && index <= 0x423);
}

static int change_begins_quietly(int index)
{
    switch (index) {
    case 0x1c0: case 0x285: case 0x286: case 0x293: case 0x294: case 0x2a2: case 0x2d8: case 0x2da:
    case 0x2f3: case 0x2f8: case 0x30b: case 0x336: case 0x337: case 0x339: case 0x33a: case 0x351:
    case 0x358: case 0x359: case 0x35b: case 0x35f: case 0x360: case 0x368: case 0x37f: case 0x380:
    case 0x395: case 0x397: case 0x3a9: case 0x3d6: case 0x3f6: case 0x413: case 0x41a: case 0x41f:
    case 0x466: case 0x473: case 0x475: case 0x476: case 0x47c: case 0x47e:
        return 1;
    }
    return (index >= 0x46 && index <= 0x54) || (index >= 0x16b && index <= 0x16f) ||
        (index >= 0x1c4 && index <= 0x1c6) || (index >= 0x28f && index <= 0x291) ||
        (index >= 0x2cc && index <= 0x2d2) || (index >= 0x2de && index <= 0x2e1) ||
        (index >= 0x3f8 && index <= 0x401) || (index >= 0x40e && index <= 0x411) ||
        (index >= 0x48d && index <= 0x48f) || (index >= 0x539 && index <= 0x53b);
}

static int change_ends_without_line(int index)
{
    switch (index) {
    case 0x00: case 0x01: case 0x0b: case 0x1b: case 0x1c: case 0x56: case 0x57: case 0x5f:
    case 0x61: case 0x62: case 0x6b: case 0x6e: case 0x82: case 0x89: case 0x8b: case 0x8d:
    case 0x8f: case 0x91: case 0xb5: case 0xb6: case 0xb8: case 0xba: case 0xbf: case 0x11a:
    case 0x11e: case 0x13c: case 0x148: case 0x14a: case 0x14d: case 0x14e: case 0x150: case 0x151:
    case 0x165: case 0x179: case 0x17b: case 0x189: case 0x18a: case 0x1ad: case 0x1ae: case 0x1c2:
    case 0x1c3: case 0x1cd: case 0x1cf: case 0x1d0: case 0x1d8: case 0x1d9: case 0x1dc: case 0x23b:
    case 0x241: case 0x244: case 0x245: case 0x248: case 0x263: case 0x265: case 0x26a: case 0x27c:
    case 0x288: case 0x289: case 0x29e: case 0x365: case 0x461:
        return 1;
    }
    return (index >= 0x03 && index <= 0x05) || (index >= 0x20 && index <= 0x22) ||
        (index >= 0x36 && index <= 0x39) || (index >= 0x67 && index <= 0x69) ||
        (index >= 0x70 && index <= 0x76) || (index >= 0x78 && index <= 0x7d) ||
        (index >= 0x84 && index <= 0x87) || (index >= 0x95 && index <= 0x99) ||
        (index >= 0x9c && index <= 0xa0) || (index >= 0xa5 && index <= 0xa7) ||
        (index >= 0xa9 && index <= 0xab) || (index >= 0xc5 && index <= 0xc9) ||
        (index >= 0xcb && index <= 0xec) || (index >= 0xee && index <= 0xf0) ||
        (index >= 0xfa && index <= 0xfc) || (index >= 0xfe && index <= 0x103) ||
        (index >= 0x105 && index <= 0x10e) || (index >= 0x121 && index <= 0x12e) ||
        (index >= 0x138 && index <= 0x13a) || (index >= 0x13e && index <= 0x143) ||
        (index >= 0x155 && index <= 0x163) || (index >= 0x168 && index <= 0x16a) ||
        (index >= 0x175 && index <= 0x177) || (index >= 0x17d && index <= 0x186) ||
        (index >= 0x18c && index <= 0x18f) || (index >= 0x191 && index <= 0x195) ||
        (index >= 0x197 && index <= 0x1a3) || (index >= 0x1a5 && index <= 0x1a7) ||
        (index >= 0x1a9 && index <= 0x1ab) || (index >= 0x1b4 && index <= 0x1bf) ||
        (index >= 0x1eb && index <= 0x1f0) || (index >= 0x25d && index <= 0x260) ||
        (index >= 0x420 && index <= 0x423);
}

static int change_ends_quietly(int index)
{
    switch (index) {
    case 0x1c0: case 0x2a1: case 0x2a2: case 0x2d8: case 0x2da: case 0x30b: case 0x336: case 0x337:
    case 0x339: case 0x33a: case 0x351: case 0x358: case 0x359: case 0x35b: case 0x35f: case 0x360:
    case 0x368: case 0x37f: case 0x380: case 0x395: case 0x397: case 0x3a9: case 0x3d6: case 0x3f6:
    case 0x413: case 0x41a: case 0x41f: case 0x466: case 0x473: case 0x475: case 0x476:
        return 1;
    }
    return (index >= 0x46 && index <= 0x54) || (index >= 0x1c4 && index <= 0x1c6) ||
        (index >= 0x2cc && index <= 0x2d2) || (index >= 0x2de && index <= 0x2e1) ||
        (index >= 0x3f8 && index <= 0x401) || (index >= 0x40e && index <= 0x411) ||
        (index >= 0x48d && index <= 0x48f);
}

enum { CHANGE_TAIL, CHANGE_OWN_AID, CHANGE_NOTHING };

static int change_began(struct GameMode *mode, int aid, unsigned char state, int index, int remain_ms, int val1,
                        int val2, int val3, unsigned char from_entry, char *text, int *color)
{
    int own = aid == g_session1.ownAid;
    struct Player *player = mode->world->player;
    struct GameActor *actor;
    struct String s;
    unsigned minutes;
    int i;

    *color = STATE_COLOR_BEGIN;
    if ((unsigned)index > 0x53b)
        return state_known(index) ? CHANGE_TAIL : CHANGE_OWN_AID;
    switch (index) {
    case 0x02: case 0x25: case 0x26: case 0x27: case 0x28: case 0x44: case 0xa1:
        say(text, 0x1cf); break;
    case 0x06: say(text, 0x1d5); break;
    case 0x07: say(text, 0x26c); break;
    case 0x08: say(text, 0x26e); break;
    case 0x09: say(text, 0xd6b); break;
    case 0x0a: say(text, 0xd6a); break;
    case 0x0c: case 0x29: case 0x2a: say(text, 0xd1); break;
    case 0x0d: say(text, 0xd2); break;
    case 0x0e: say(text, 0x276); break;
    case 0x0f: say(text, 0xd87); break;
    case 0x10: say(text, 0x1d3); break;
    case 0x11: case 0x40: say(text, 0x1d6); break;
    case 0x12: say(text, 0x1d8); break;
    case 0x13: say(text, 0x1da); break;
    case 0x14: say(text, 0x278); break;
    case 0x15: say(text, 0x27a); break;
    case 0x16: say(text, 0x27c); break;
    case 0x17: case 0x93: say(text, 0x27e); break;
    case 0x18: say(text, 0x1dc); break;
    case 0x19: case 0xbc: say(text, 0x1d1); break;
    case 0x1a: say(text, 0x1e0); break;
    case 0x1d:
        no_line(text);
        if (own) {
            render_object_set_action((struct RenderObject *)player, 0x40, 1);
            mode->world->player->motion_held = 1;
        }
        break;
    case 0x1e: say(text, 0xd69); break;
    case 0x1f: say(text, 0x288); break;
    case 0x20: say(text, 0x28a); break;
    case 0x21: say(text, 0x28c); break;
    case 0xb6:
        if (own)
            sound_play_wave("lava_golem_move.wav", 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
        no_line(text);
        break;
    case 0xf1: case 0x10f: say(text, 0x4cf); break;
    case 0xf2: case 0x110: say(text, 0x4d1); break;
    case 0xf3: case 0x111: say(text, 0x4d3); break;
    case 0xf4: case 0x112: say(text, 0x4d7); break;
    case 0xf5: case 0x113: say(text, 0x4d5); break;
    case 0xf6: case 0x114: say(text, 0x4d9); break;
    case 0xf7: say(text, 0x4db); break;
    case 0xf8: say(text, 0x4dd); break;
    case 0xf9: say(text, 0x4df); break;
    case 0xfa:
        minutes = minutes_left(remain_ms);
        if ((int)minutes <= 0) {
            text[0] = 0;
            break;
        }
        string_init(&s);
        string_format(&s, msg_str(0x7ef), minutes, val1);
        say_string(text, &s);
        break;
    case 0xfb: say(text, 0x4e2); break;
    case 0xfc:
        minutes = minutes_left(remain_ms);
        if ((int)minutes <= 0) {
            text[0] = 0;
            break;
        }
        string_init(&s);
        if ((int)minutes / 60 > 0)
            string_format(&s, msg_str(0x9a8), (int)minutes / 60, (int)minutes % 60, val1);
        else
            string_format(&s, msg_str(0x9a7), minutes, val1);
        say_string(text, &s);
        break;
    case 0xfd:
        if (own)
            mode->boss_shown = 1;
        say(text, 0x4e4);
        break;
    case 0x104:
        if (own)
            g_session1.ownIllusion = 1;
        no_line(text);
        break;
    case 0x138:
        minutes = minutes_left(remain_ms);
        if ((int)minutes <= 0) {
            text[0] = 0;
            break;
        }
        string_init(&s);
        string_format(&s, msg_str(0x99e), minutes, val1);
        say_string(text, &s);
        break;
    case 0x153:
        if (own) {
            for (i = 0; i < 10; i++) {
                effect_mgr_remove_type(g_effectMgr, player->aid, 0x514 + i);
                effect_mgr_remove_type(g_effectMgr, player->aid, 0x5bb + i);
            }
            spawn(player->aid, g_session1.map_rule_flags[0] == 1 ? val1 + 0x5ba : val1 + 0x513);
            no_line(text);
        }
        break;
    case 0x162:
        if (own)
            launch(player, 0x2d9);
        printf(text, "");
        break;
    case 0x166:
        if (own) {
            launch(player, 0x2f0);
            no_line(text);
        }
        break;
    case 0x175:
        g_session1.overheatStart = timeGetTime();
        g_session1.overheatValue = val2 == 0 ? val1 : 0;
        if (val1 == 0 && own_states_count() != 0)
            own_states_remove(0x175, 0x175);
        break;
    case 0x1b0:
        actor = world_get_actor_or_player(mode->world, aid);
        if (actor)
            spell_fist_effects(actor->aid, (unsigned char)val3);
        break;
    case 0x1b1:
        if (own) {
            actor_msg(player, ACTOR_MSG_REMOVE_EFFECT, 0x1c5);
            no_line(text);
        }
        break;
    case 0x1b2: case 0x1b7: case 0x1b9: case 0x1c3: {
        static const short effects[] = { 0x36f, 0x361, 0x363, 0x367 };
        int effect_id = effects[index == 0x1b2 ? 0 : index == 0x1b7 ? 1 : index == 0x1b9 ? 2 : 3];
        if (own) {
            actor_msg(player, ACTOR_MSG_REMOVE_EFFECT, effect_id);
            launch(player, effect_id);
            no_line(text);
        }
        break;
    }
    case 0x1b3:
        if (own) {
            actor_msg(player, ACTOR_MSG_SLEEP_MARK_ON, 0);
            no_line(text);
        }
        break;
    case 0x26d: case 0x28b: case 0x299: case 0x2a3:
        if (!own)
            return CHANGE_NOTHING;
        actor_state_msg(player, index, state, val1, 0);
        return CHANGE_OWN_AID;
    case 0x27c:
        string_init(&s);
        string_format(&s, msg_str(0x784), val2, val1 / 100.0 + 1.0);
        _snprintf_s(text, 0x100, _TRUNCATE, "%s", string_cstr(&s));
        string_free(&s);
        break;
    case 0x2a1:
        if (own)
            session_set_my_exchange_money(&g_session1, val1);
        break;
    case 0x2a4: case 0x2c6:
        if (own) {
            actor_state_msg(player, index, state, val1, 0);
            game_mode_on_own_aid(mode, aid);
        }
        break;
    case 0x37d:
        if (own) {
            world_get_game_actor_by_aid(mode->world, aid);
            actor_set_state(player, 0x2f, 1);
        }
        break;
    case 0x3a5:
        if (own) {
            actor_set_state(player, 0x30, 1);
            mode->world->player->motion_held = 1;
        }
        break;
    case 0x3f5:
        *color = STATE_COLOR_END;
        say(text, 0xcaf);
        break;
    case 0x465:
        spawn(aid, 0x552);
        sound_play_wave("effect\\s포이즌버스터.wav", 0.0f, 0.0f,
                        0.0f, 0xfa, 0x28, 1.0f);
        break;
    default:
        if (change_begins_without_line(index))
            no_line(text);
        else if (!change_begins_quietly(index) && !state_known(index))
            return CHANGE_OWN_AID;
        break;
    }
    return CHANGE_TAIL;
}

static int change_ended(struct GameMode *mode, int aid, int index, int val1, char *text, int *color)
{
    int own = aid == g_session1.ownAid;
    struct Player *player = mode->world->player;
    struct Effect *effect;
    int i;

    *color = STATE_COLOR_END;
    switch (index) {
    case 0x02: case 0x25: case 0x26: case 0x27: case 0x44: case 0xa1:
        say(text, 0x1d0); break;
    case 0x06: case 0x11: case 0x40: say(text, 0x1d7); break;
    case 0x07: say(text, 0x26d); break;
    case 0x08: say(text, 0x26f); break;
    case 0x09: say(text, 0x271); break;
    case 0x0a: say(text, 0x273); break;
    case 0x0c: case 0x28: case 0x2a:
        if (own && own_states_count() != 0)
            own_states_remove(index, index);
        say(text, 0xd2);
        break;
    case 0x0d: say(text, 0xd1); break;
    case 0x0e: say(text, 0x277); break;
    case 0x0f: say(text, 0xd88); break;
    case 0x10: say(text, 0x1d4); break;
    case 0x12: say(text, 0x1d9); break;
    case 0x13: say(text, 0x1db); break;
    case 0x14: say(text, 0x279); break;
    case 0x15: say(text, 0x27b); break;
    case 0x16: say(text, 0x27d); break;
    case 0x17: case 0x93: say(text, 0x27f); break;
    case 0x18: say(text, 0x1dd); break;
    case 0x19: case 0xbc: say(text, 0x1d2); break;
    case 0x1a: say(text, 0x1e1); break;
    case 0x1d:
        no_line(text);
        if (own) {
            actor_set_state(player, 0, 1);
            mode->world->player->motion_held = 0;
        }
        break;
    case 0x1e: say(text, 0x287); break;
    case 0x1f: say(text, 0x289); break;
    case 0x29: say(text, 0xd2); break;
    case 0xf1: case 0x10f: say(text, 0x4d0); break;
    case 0xf2: case 0x110: say(text, 0x4d2); break;
    case 0xf3: case 0x111: say(text, 0x4d4); break;
    case 0xf4: case 0x112: say(text, 0x4d8); break;
    case 0xf5: case 0x113: say(text, 0x4d6); break;
    case 0xf6: case 0x114: say(text, 0x4da); break;
    case 0xf7: say(text, 0x4dc); break;
    case 0xf8: say(text, 0x4de); break;
    case 0xf9: say(text, 0x4e0); break;
    case 0xfd:
        if (own)
            mode->boss_shown = 0;
        break;
    case 0x104:
        if (own)
            g_session1.ownIllusion = 0;
        no_line(text);
        break;
    case 0x153:
        if (own) {
            for (i = 0; i < 10; i++) {
                effect_mgr_remove_type(g_effectMgr, player->aid, 0x514 + i);
                effect_mgr_remove_type(g_effectMgr, player->aid, 0x5bb + i);
            }
        }
        no_line(text);
        break;
    case 0x166:
        if (own) {
            remove_all_effects((struct GameActor *)player, 0x2f0);
            no_line(text);
        }
        break;
    case 0x16b: case 0x16c: case 0x16d: case 0x16e: case 0x16f:
        if (own) {
            switch (val1 & 3) {
            case 0: actor_msg(player, ACTOR_MSG_REMOVE_EFFECT, 0x2df); break;
            case 1: actor_msg(player, ACTOR_MSG_REMOVE_EFFECT, 0x2e0); break;
            case 2: actor_msg(player, ACTOR_MSG_REMOVE_EFFECT, 0x2e1); break;
            case 3: actor_msg(player, ACTOR_MSG_REMOVE_EFFECT, 0x2e2); break;
            }
            no_line(text);
        }
        break;
    case 0x1b1:
        if (own)
            actor_msg(player, ACTOR_MSG_REMOVE_EFFECT, 0x1c5);
        no_line(text);
        break;
    case 0x1b2:
        if (own)
            actor_msg(player, ACTOR_MSG_REMOVE_EFFECT, 0x36f);
        no_line(text);
        break;
    case 0x1b3:
        if (own)
            actor_msg(player, ACTOR_MSG_SLEEP_MARK_OFF, 0);
        no_line(text);
        break;
    case 0x37d:
        if (own)
            actor_set_state(player, 0, 1);
        break;
    case 0x3a5:
        if (own) {
            actor_set_state(player, 0, 1);
            mode->world->player->motion_held = 0;
        }
        break;
    case 0x3f5:
        *color = STATE_COLOR_BEGIN;
        say(text, 0xcaf);
        break;
    case 0x465:
        effect = effect_mgr_find_effect(g_effectMgr, aid, 0x552);
        if (effect)
            effect_mgr_remove(g_effectMgr, effect, 1);
        break;
    default:
        if (change_ends_without_line(index))
            no_line(text);
        else if (!change_ends_quietly(index))
            return CHANGE_OWN_AID;
        break;
    }
    return CHANGE_TAIL;
}

static void own_states_drop(int index, int val1)
{
    size_t i = 0;
    while (i < own_states_count()) {
        if (own_entry_matches(own_states_at(i), index, val1))
            own_states_erase_at(i);
        else
            ++i;
    }
}

void game_mode_zc_msg_state_change(struct GameMode *mode, int aid, int state,
                                               int index, int remain_ms, int val1, int val2, int val3,
                                               int from_entry, int total_ms)
{
    struct PartyMember member;
    struct GameActor *actor;
    char prefix[0x100], text[0x100], line[0x200];
    int color, how;

    if (index == EFST_ATTACK_COOLDOWN)
        index = val1 + 0x381;
    if (aid != g_session1.ownAid) {
        actor = world_get_game_actor_by_aid(mode->world, aid);
        if (actor)
            actor_state_msg(actor, index, (unsigned char)state, val1, val2);
        if (index == 0x1b0 && actor)
            spell_fist_effects(actor->aid, (unsigned char)val3);
    }

    session_get_party_member(&g_session1, &member, aid);
    if (aid == g_session1.ownAid || (member.found != 0 && member.state == 0)) {
        sprintf(prefix, "");
        if (aid != g_session1.ownAid)
            sprintf(prefix, msg_str(MSG_PARTY_MEMBER_PREFIX), string_cstr(&member.name));
        memset(text, 0, sizeof text);

        if ((unsigned char)state) {
            if (aid == g_session1.ownAid) {
                if (index == 0x338)
                    g_rodex->mail_count = val2;
                else
                    own_state_list(index, remain_ms, val1, val2, total_ms);
            }
            how = change_began(mode, aid, (unsigned char)state, index, remain_ms, val1, val2, val3,
                               (unsigned char)from_entry, text, &color);
        } else {
            if (aid == g_session1.ownAid && own_states_count() != 0)
                own_states_drop(index, val1);
            how = change_ended(mode, aid, index, val1, text, &color);
        }

        if (how != CHANGE_NOTHING)
            game_mode_on_own_aid(mode, aid);
        if (how == CHANGE_TAIL) {
            if (strlen(text) > 1) {
                sprintf(line, "%s%s", prefix, text);
                ui_window_mgr_send_msg_(&g_windowMgr, UIM_CLOSE_DIALOG, 0, 0, 0, 0);
                ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)line, color,
                                      strlen(prefix) > 1 ? CHAT_CHANNEL_MEMBER_STATE : CHAT_CHANNEL_OWN_STATE, 0);
            }
            if (aid == g_session1.ownAid)
                actor_state_msg(mode->world->player, index, (unsigned char)state, val1, val2);
        }
    }
    party_member_info_destroy(&member);
}
