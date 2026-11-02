#include "game/gamemode.h"
#include "session/option_pairs.h"
#include "skills/skill_bundle.h"
#include "session/skill_queries.h"
#include "skills/skill_info.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "skills/attack_range.h"
#include <string.h>
#include "platform/win_types.h"
#include "game/mode_msg.h"
#include "game/skill_list.h"
#include "game/mode_util.h"
#include "platform/language.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "platform/clock.h"

#define CZ_USE_SKILL2                   0x438
#define CZ_USE_SKILL_TOGROUND2          0xaf4
#define CZ_USE_SKILL_TOGROUND_WITHTALKBOX2 0x367
#define CZ_USE_SKILL_START                0xb10
#define CZ_USE_SKILL_END            0xb11

#define SKILL_GC_ROLLINGCUTTER 0x7f4

int g_skillAimGood;

#define COLOR_SKILL_REFUSED 0xff
#define CHAT_CHANNEL_SKILL 0xa

int game_mode_msg_skill_aim_level_change(struct GameMode *self, int by)
{
    struct SkillInfo skill;
    int level;

    if (!skill_info_is_level_used(self->skill_use.skill_id))
        return 0;
    skill_table_find(&g_session1.skills, &skill, self->skill_use.skill_id);
    if (skill.found) {
        level = self->skill_use.level + by;
        self->skill_use.level = level;
        if (level > skill.level) {
            self->skill_use.level = skill.level;
            level = skill.level;
        }
        if (level <= 0)
            self->skill_use.level = 1;
    }
    skill_info_destruct(&skill);
    return 0;
}

int game_mode_msg_skill_shout(struct GameMode *self, int skill_id, struct GameActor *actor, int level)
{
    char line[0x68];
    char skill[0x40];
    struct String name;
    int shout;

    self->skill_shout_time = platform_ticks_ms();
    shout = 1;
    if (level < 0 || skill_id < 0)
        shout = 0;
    switch (skill_id) {
    case 0x28: case 0x29: case 0x33: case 0x87: case 0x185: case 0x7f1:
    case 0x8c7: case 0x8f2: case 0x926: case 0x9e7: case 0xbb9: case 0x1506: case 0x1507:
        shout = 0;
        break;
    }
    if (session_is_monster_actor(&g_session1, actor) == 1)
        shout = 1;
    if (!actor || !shout)
        return 0;

    if (skill_id == 0x14e || skill_id == 0x14f) {
        game_mode_get_actor_name(game_current_mode(), &name, level);
        strcpy(line, string_cstr(&name));
        strcpy(skill, skill_name(skill_id));
        if (g_serviceType == 0)
            strcat(skill, " ♡");
        strcat(line, " !!  ");
        strcat(line, msg_str(0x2f7));
        render_object_send(actor, NULL, 7, MSG_ARG_PTR(line), 0, 0, 0);
        string_free(&name);
        return 0;
    }
    if (skill_id == 0x150) {
        wsprintfA(line, "%s", self->couple_name);
        strcpy(skill, skill_name(skill_id));
        if (g_serviceType == 0)
            strcat(skill, " ♡");
        strcat(line, " !!  ");
        strcat(line, skill);
        render_object_send(actor, NULL, 7, MSG_ARG_PTR(line), 0, 0, 0);
        return 0;
    }
    if (skill_id == 0x146 || skill_id == 0x13e || skill_id == 0x1ec)
        return 0;
    if (session_is_monster_actor(&g_session1, actor) == 1 && actor->visible == 0)
        return 0;
    strcpy(skill, skill_name(skill_id));
    strcat(skill, " !!");
    render_object_send(actor, NULL, 7, MSG_ARG_PTR(skill), 0, 0, 0);
    return 0;
}

int game_mode_msg_aim_skill(struct GameMode *self, int type, const struct SkillInfo *info, int level)
{
    struct SkillInfo skill;

    if (ui_window_find(0xc4) || g_windowMgr.merchant_item_my_shop_wnd)
        return 0;
    self->skill_use.type = type;
    if (!info)
        return 0;
    self->skill_use.skill_id = info->id;
    self->skill_use.range = info->range;
    if (skill_info_is_level_used(info->id)) {
        self->skill_use.level = level;
    } else {
        session_get_skill_info(&skill, self->skill_use.skill_id);
        self->skill_use.level = skill.found ? skill.level : 0;
        skill_info_destruct(&skill);
    }
    g_skillAimGood = skill_info_is_good_skill_for_actor(self->skill_use.skill_id);
    return 0;
}

int game_mode_msg_skill_target(struct GameMode *self, const struct SkillInfo *info, int level, int quiet, int d)
{
    struct Player *player;
    int skill_id, target;

    if (!quiet) {
        player = self->world->player;
        if (player->mounted && !session_is_homunculus_skill(&g_session1, info->id)
                && !session_is_merc_or_elemental_skill(&g_session1, info->id)) {
            ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)msg_str(0x75e), COLOR_SKILL_REFUSED, 0, 0);
            session_clear_rolling_cutter_one_shot(info->id);
            return 0;
        }
    }
    self_msg(self, MODE_MSG_CANCEL_SKILL_AIM, 0, 0, 0, 0);
    self->skill_use_unchecked = d == -1;
    switch (info->type) {
    case 1:
        return self_msg(self, MODE_MSG_AIM_SKILL, 2, (int)info, level, 0), 0;
    case 2:
        return self_msg(self, MODE_MSG_AIM_SKILL, 1, (int)info, level, 0), 0;
    case 4:
        if (session_is_homunculus_skill(&g_session1, info->id))
            target = g_session1.homunAid;
        else
            target = session_is_merc_or_elemental_skill(&g_session1, info->id) ? g_session1.mercAid : g_session1.ownAid;
        skill_id = info->id;
        self_msg(self, MODE_MSG_USE_SKILL, skill_id, target, level, quiet);
        self_msg(self, MODE_MSG_AIM_SKILL, 0, 0, 0, 0);
        return 0;
    case 8:
        return self_msg(self, MODE_MSG_AIM_SKILL, 3, (int)info, level, 0), 0;
    case 16:
        return self_msg(self, MODE_MSG_AIM_SKILL, 4, (int)info, level, 0), 0;
    case 32:
        return self_msg(self, MODE_MSG_AIM_SKILL, 5, (int)info, level, 0), 0;
    }
    return 0;
}

static void skill_refused(const char *text)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)text, COLOR_SKILL_REFUSED, CHAT_CHANNEL_SKILL, 0);
}

static void skill_not_usable(int skill_id)
{
    char text[0x100];
    const char *name = skill_name(skill_id);

    sprintf(text, msg_str(0x5b4), name);
    skill_refused(text);
}

static void tell_window_target(int id, int target)
{
    struct UIWindow *w = ui_window_open(id);

    ui_window_send(w, NULL, 0x50, 0, target, 0, 0);
}

int game_mode_msg_use_skill(struct GameMode *self, int skill_id, int target, int level, int quiet)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short level;
        unsigned short skill_id;
        int target;
    } packet;
    struct {
        unsigned short id;
        unsigned short skill_id;
        unsigned short level;
        int target;
    } skill_start;
    struct {
        unsigned short id;
        unsigned short skill_id;
    } skill_end;
#pragma pack(pop)
    char text[0x100];
    struct SkillInfo skill, table;
    struct GameActor *actor;
    int count;

    if (ui_window_find(0xc4))
        goto done;
    if (quiet == 0 && self->world->player->mounted && !session_is_homunculus_skill(&g_session1, skill_id)
            && !session_is_merc_or_elemental_skill(&g_session1, skill_id)) {
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)msg_str(0x75e), COLOR_SKILL_REFUSED,
                            CHAT_CHANNEL_SKILL, 0);
        goto done;
    }
    if (game_mode_is_blocking_window_open() == 1)
        return 0;

    switch (skill_id) {
    case 10000:
        if (target == g_session1.aid)
            return 0;
        actor = mode_actor(self, target);
        if (actor && actor->object_type == 0)
            return 0;
        tell_window_target(0x5b, target);
        return 0;
    case 20000:
        send_u32(0x1e5, target);
        return 0;
    case 20001:
        sprintf(text, "[ (val1 == 20001) ]");
        chat_print(text, 0x6e96ff);
        send_u32(0x1f9, target);
        return 0;
    case 40000:
        if (target == g_session1.aid)
            return 0;
        tell_window_target(0xc4, target);
        return 0;
    }
    if (g_windowMgr.merchant_item_my_shop_wnd)
        return 0;
    if (!self->skill_use_unchecked && !session_is_skill_delay_over(&g_session1, skill_id)) {
        skill_not_usable(skill_id);
        goto done;
    }
    if (level == 0) {
        session_get_skill_info(&skill, skill_id);
        if (skill.found != 0) {
            level = skill.level;
        } else {
            player_skill_bundle_copy_autorun_skill(&g_session1.skills, &table, skill_id);
            level = table.found ? table.level : 0;
            skill_info_destruct(&table);
        }
        skill_info_destruct(&skill);
    }
    if (!session_is_homunculus_skill(&g_session1, skill_id) && !session_is_merc_or_elemental_skill(&g_session1, skill_id)
            && self->world->player->state == 6)
        goto done;
    if (g_windowMgr.chat_room_wnd)
        goto done;
    if (skill_id == 0x9a || skill_id == 0x29) {
        if (g_session1.sp < (skill_id == 0x9a ? 0x28 : 0x1e)) {
            sprintf(text, msg_str(0xca));
            skill_refused(text);
            return 0;
        }
        if (g_session1.cartType <= 0) {
            sprintf(text, msg_str(0x294));
            skill_refused(text);
            return 0;
        }
        if (skill_id == 0x9a)
            ui_window_open(0x5f);
    } else if (skill_id == SKILL_GC_ROLLINGCUTTER) {
        skill_start.id = CZ_USE_SKILL_START;
        skill_start.skill_id = SKILL_GC_ROLLINGCUTTER;
        skill_start.level = (unsigned short)level;
        skill_start.target = target;
        send_raw(10, &skill_start);
        if (session_get_instant_var(&g_session1, 4)) {
            skill_end.id = CZ_USE_SKILL_END;
            skill_end.skill_id = SKILL_GC_ROLLINGCUTTER;
            send_raw(4, &skill_end);
            session_set_pair(&g_session1, 4, 0);
            return 0;
        }
        g_session1.skillUseKey = g_session1.lastKeyDown;
        count = session_get_instant_var(&g_session1, 2) + 1;
        session_set_pair(&g_session1, 2, count);
        return 0;
    }
    packet.id = CZ_USE_SKILL2;
    packet.level = (unsigned short)level;
    packet.skill_id = (unsigned short)skill_id;
    packet.target = target;
    if ((unsigned short)skill_id == 0x5d)
        g_session1.senseTargetAid = target;
    send_packet(CZ_USE_SKILL2, &packet);
    return 0;

done:
    session_clear_rolling_cutter_one_shot(skill_id);
    return 0;
}

int game_mode_msg_use_skill_to_ground(struct GameMode *self, const struct GroundSkillUse *use, int b)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short level;
        unsigned short skill_id;
        unsigned short x;
        unsigned short y;
        unsigned char b;
    } packet;
#pragma pack(pop)
    struct SkillInfo skill;
    int skill_id, x, y, level;

    if (self->world->player->mounted && !session_is_homunculus_skill(&g_session1, (int)use)
            && !session_is_merc_or_elemental_skill(&g_session1, (int)use)) {
        chat_print_msg(0x75e, COLOR_SKILL_REFUSED);
        return 0;
    }
    if (game_mode_is_blocking_window_open() == 1)
        return 0;
    skill_id = use->skill_id;
    x = use->x;
    y = use->y;
    level = use->level;
    if (!self->skill_use_unchecked && !session_is_skill_delay_over(&g_session1, skill_id)) {
        skill_not_usable(skill_id);
        return 0;
    }
    if (level == 0) {
        level = skill_table_find(&g_session1.skills, &skill, skill_id)->level;
        skill_info_destruct(&skill);
    }
    packet.skill_id = (unsigned short)skill_id;
    packet.id = CZ_USE_SKILL_TOGROUND2;
    packet.b = (unsigned char)b;
    packet.level = (unsigned short)level;
    packet.x = (unsigned short)x;
    packet.y = (unsigned short)y;
    send_packet(CZ_USE_SKILL_TOGROUND2, &packet);
    return 0;
}

int game_mode_msg_use_skill_to_ground_talkbox(struct GameMode *self, const struct GroundSkillTalkbox *talkbox)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short level;
        unsigned short skill_id;
        unsigned short x;
        unsigned short y;
        char text[80];
    } packet;
#pragma pack(pop)
    struct SkillInfo skill;

    packet.id = CZ_USE_SKILL_TOGROUND_WITHTALKBOX2;
    packet.level = (unsigned short)talkbox->use.level;
    packet.skill_id = (unsigned short)talkbox->use.skill_id;
    packet.x = (unsigned short)talkbox->use.x;
    packet.y = (unsigned short)talkbox->use.y;
    if (!self->skill_use_unchecked && !session_is_skill_delay_over(&g_session1, packet.skill_id)) {
        skill_not_usable(packet.skill_id);
        return 0;
    }
    _snprintf_s(packet.text, 0x15, (size_t)-1, "%s", talkbox->text);
    if (packet.level == 0) {
        packet.level = (unsigned short)skill_table_find(&g_session1.skills, &skill, packet.skill_id)->level;
        skill_info_destruct(&skill);
    }
    send_packet(packet.id, &packet);
    return 0;
}

#define SKILL_CAPTURE_MONSTER 10000

int skill_attack_range(int skill_id, int level)
{
    struct SkillInfo skill, table;

    int range, base;

    if (skill_id == SKILL_CAPTURE_MONSTER)
        return 5;
    range = 1;
    session_get_skill_info(&skill, skill_id);
    if (skill.found == 0) {
        skill_info_assign(&skill, player_skill_bundle_copy_autorun_skill(&g_session1.skills, &table, skill_id));
        skill_info_destruct(&table);
        if (skill.found == 0)
            goto done;
        level = skill.level;
    }
    base = 0;

    client_lua_call(g_session1.luaState, "GetSkillAttackRange", "ddd>dd", skill_id, level, skill.level, &range, &base);
    range += skill.range - base;
done:
    skill_info_destruct(&skill);
    return range;
}
