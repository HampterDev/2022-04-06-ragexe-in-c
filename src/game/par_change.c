#include "game/packet_handlers.h"
#include "game/display_settings.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "platform/win_types.h"
#include <stdlib.h>
#include "game/character_stats.h"
#include "game/session_bridge.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "game/status_values.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "session/session_windows.h"
#include "ui/character/gauge.h"
#include "ui/character/status_wnd.h"
#include "ui/ui_std.h"

enum ParType {
    SP_SPEED = 0,
    SP_KARMA = 3,
    SP_MANNER = 4,
    SP_HP = 5,
    SP_MAXHP = 6,
    SP_SP = 7,
    SP_MAXSP = 8,
    SP_STATUSPOINT = 9,
    SP_BASELEVEL = 0xb,
    SP_SKILLPOINT = 0xc,
    SP_WEIGHT = 0x18,
    SP_MAXWEIGHT = 0x19,
    SP_PKHONOR = 0x3c,
    SP_POW = 0xdb,
    SP_TRAIT_LAST = 0xe9,
};

enum {
    MSG_MANNER_PENALTY = 0x298,
    MSG_MANNER_OK = 0x299,
    MSG_WEIGHT_OVER_HEAVY = 0x133,
    MSG_WEIGHT_BELOW_LOW = 0x134,
    MSG_WEIGHT_BELOW_HEAVY = 0x135,
    MSG_CHAT_SKILL_ITEM_BLOCKED = 0x35a,
    MSG_WEIGHT_OVER_LOW = 0xcef,
};

enum {
    UIWM_WEIGHT_ICON = 3,
    CHAT_COLOR_MANNER = 0xffff,
    CHAT_LINE_WEIGHT = 7,
    WEIGHT_HEAVY = 90,
    WEIGHT_LIGHT_AT_MAXWEIGHT = 50,
};

enum {
    ACTOR_MSG_MANNER_PENALTY = 0x15,
    ACTOR_MSG_MANNER_OK = 0x76,
    MM_PET_ACT = 0x94,
    PET_ACTION_HP_LOW = 2,
    PET_ACTION_LEVEL_UP = 6,
    PET_EMOTION_INTERVAL = 5000,
    PET_EMOTION_HP_PERCENT = 25,
    PET_EMOTION_CHANCE = 50,
    WINDOW_LEVEL_UP = 0x15,
    WINDOW_JOB_LEVEL_UP = 0x31,
    WINDOW_MENU_ICON = 0x133,
    UIM_PARTY_HP = 6,
    UIM_STATUS_REFRESH = 0x23,
};

enum {
    EF_LEVEL_UP = 0x173,
    EF_LEVEL_UP_SUPER = 0x152,
    EF_LEVEL_UP_STAR = 0x246,
    EF_JOB_LEVEL_UP = 0x9e,
    EF_JOB_LEVEL_UP_HIGH = 0x151,
    JOB_LEVEL_HIGH = 50,
};

#pragma pack(push, 1)
struct PACKET_ZC_PAR_CHANGE {
    unsigned short id;
    unsigned short type;
    int value;
};
#pragma pack(pop)

static void chat_line(const char *text, int color, int kind)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_PRINT, (int)text, color, kind, 0);
}

static void weight_icon(void)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_WEIGHT_ICON, 0, 0, 0, 0);
}

static void pet_emotion(struct GameMode *self, int action)
{
    int hunger = session_pet_hunger_level(g_session1.petFullness);
    int intimacy = session_pet_intimacy_level(g_session1.petIntimacy);

    mode_send_msg(self, MM_PET_ACT, session_get_pet_emotion(&g_session1, hunger, intimacy, action), 0, 0, 0);
}

static int pet_present(struct GameMode *self)
{
    return world_get_game_actor_by_aid(self->world, ((unsigned)g_session1.aid)) != 0;
}

static void launch_on_player(struct GameMode *self, int effect)
{
    Vec3 here = { 0.0f, 0.0f, 0.0f };

    game_actor_launch_effect((struct GameActor *)self->world->player, effect, here, 0.0f);
}

static void mark_menu_new(int what)
{
    struct UIWindow *w = ui_window_find(WINDOW_MENU_ICON);

    if (w)
        UIWINDOW_SENDMSG(w, 6, 0x116, what, 1, 0);
}

static int weight_percent(int weight, int max)
{
    return weight * 100 / (max > 1 ? max : 1);
}

static int level_up_effect(void)
{
    int job;

    if (session_is_any_super_novice_job(&g_session1, session_get_drawn_job(&g_session1)))
        return EF_LEVEL_UP_SUPER;
    job = session_get_drawn_job(&g_session1);
    if ((job >= 0x108e && job <= 0x1094) || job == 0x10ce || job == 0x10cf || job == 0x10dc)
        return EF_LEVEL_UP_STAR;
    if ((job >= 0xfce && job <= 0xfd1) || (job >= 0x1081 && job <= 0x1083))
        return EF_LEVEL_UP_STAR;
    if (job >= 0xfd2 && job <= 0xfd4)
        return EF_LEVEL_UP_SUPER;
    return EF_LEVEL_UP;
}

static void base_level_up(struct GameMode *self, int level)
{
    g_session1.charBaseLevel = level;
    session_invalidate_windows(&g_session1);
    self->world->player->level = (short)g_session1.charBaseLevel;
    sound_play_wave("levelup.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
    launch_on_player(self, level_up_effect());
    ui_window_open(WINDOW_LEVEL_UP);
    if (pet_present(self))
        pet_emotion(self, PET_ACTION_LEVEL_UP);
    if (g_windowMgr.basic_info_wnd)
        ui_basic_info_wnd_set_height((struct UIBasicInfoWnd *)g_windowMgr.basic_info_wnd, g_windowMgr.basic_info_wnd->height);
    game_mode_refresh_actor_effects(self, 1);
    mark_menu_new(0xb8);
}

static void job_level_up(struct GameMode *self, int level)
{
    g_session1.jobLevel = level;
    session_invalidate_windows(&g_session1);
    launch_on_player(self, level > JOB_LEVEL_HIGH ? EF_JOB_LEVEL_UP_HIGH : EF_JOB_LEVEL_UP);
    {
        ui_window_open(WINDOW_JOB_LEVEL_UP);
        if (g_windowMgr.basic_info_wnd)
            ui_basic_info_wnd_set_height((struct UIBasicInfoWnd *)g_windowMgr.basic_info_wnd, g_windowMgr.basic_info_wnd->height);
    }
    mark_menu_new(0xbc);
}

static void set_manner(struct GameMode *self, int value, struct String *line)
{
    struct GameActor *player = (struct GameActor *)self->world->player;

    if (value < 0) {
        struct String text;

        string_init(&text);
        string_format(&text, msg_str(MSG_MANNER_PENALTY), -value);
        string_assign(line, string_cstr(&text), text.size);
        string_free(&text);
        render_object_send(player, NULL, ACTOR_MSG_MANNER_PENALTY, 0x22, 0, 0, 0);
    } else {
        string_assign_cstr(line, msg_str(MSG_MANNER_OK));
        render_object_send(player, NULL, ACTOR_MSG_MANNER_OK, 0, 0, 0, 0);
    }
    chat_line(string_cstr(line), CHAT_COLOR_MANNER, 0);
    g_session1.manner = value;
}

static void set_hp(struct GameMode *self, int value)
{
    if (g_session1.maxHp > 0 && value * 100 / g_session1.maxHp < PET_EMOTION_HP_PERCENT && pet_present(self)
        && rand() % 100 < PET_EMOTION_CHANCE && session_pet_emotion_due(&g_session1, PET_EMOTION_INTERVAL) == 1)
        pet_emotion(self, PET_ACTION_HP_LOW);
    if (g_session1.hp != value) {
        g_session1.hp = value;
        session_refresh_basic_info(&g_session1);
    }
}

static void set_hp_sp_field(int *field, int value)
{
    if (*field != value) {
        *field = value;
        session_refresh_basic_info(&g_session1);
    }
}

static void set_weight(int *packet_value)
{
    int weight = *packet_value / 10;
    int before, now;

    *packet_value = weight;
    before = weight_percent(g_session1.weight, g_session1.maxWeight);
    g_session1.weight = weight;
    session_refresh_basic_info(&g_session1);
    session_invalidate_windows(&g_session1);
    if (g_session1.maxWeight == 0)
        return;
    now = weight_percent(g_session1.weight, g_session1.maxWeight);
    if (now > before) {
        if (now >= g_session1.weightPenaltyPercent && before < g_session1.weightPenaltyPercent) {
            struct String text;

            weight_icon();
            string_init(&text);
            string_format(&text, msg_str(MSG_WEIGHT_OVER_LOW), g_session1.weightPenaltyPercent);
            chat_line(string_cstr(&text), CHAT_COLOR_RED, CHAT_LINE_WEIGHT);
            string_free(&text);
        }
        if (now >= WEIGHT_HEAVY && before < WEIGHT_HEAVY) {
            weight_icon();
            chat_line(msg_str(MSG_WEIGHT_OVER_HEAVY), CHAT_COLOR_RED, CHAT_LINE_WEIGHT);
        }
    } else if (now < before) {
        if (now < g_session1.weightPenaltyPercent && before >= g_session1.weightPenaltyPercent) {
            weight_icon();
            chat_line(msg_str(MSG_WEIGHT_BELOW_LOW), CHAT_COLOR_YELLOW, CHAT_LINE_WEIGHT);
        }
        if (now < WEIGHT_HEAVY && before >= WEIGHT_HEAVY) {
            weight_icon();
            chat_line(msg_str(MSG_WEIGHT_BELOW_HEAVY), CHAT_COLOR_YELLOW, CHAT_LINE_WEIGHT);
        }
    }
}

static void set_max_weight(int *packet_value)
{
    int max = *packet_value / 10;
    int old_max = g_session1.maxWeight;
    int before, now;

    *packet_value = max;
    g_session1.maxWeight = max;
    before = weight_percent(g_session1.weight, old_max);
    session_refresh_basic_info(&g_session1);
    session_invalidate_windows(&g_session1);
    if (old_max == 0 || old_max >= g_session1.maxWeight)
        return;
    now = weight_percent(g_session1.weight, g_session1.maxWeight);
    if (now < WEIGHT_LIGHT_AT_MAXWEIGHT && before >= WEIGHT_LIGHT_AT_MAXWEIGHT) {
        weight_icon();
        chat_line(msg_str(MSG_WEIGHT_BELOW_LOW), CHAT_COLOR_YELLOW, CHAT_LINE_WEIGHT);
    }
    if (now < WEIGHT_HEAVY && before >= WEIGHT_HEAVY) {
        weight_icon();
        chat_line(msg_str(MSG_WEIGHT_BELOW_HEAVY), CHAT_COLOR_YELLOW, CHAT_LINE_WEIGHT);
    }
}

static int *stat_field(int type)
{
    switch (type) {
    case SP_ATK1: return &g_session1.atk;
    case SP_ATK2: return &g_session1.refineAtk;
    case SP_MATK1: return &g_session1.matkMax;
    case SP_MATK2: return &g_session1.matkMin;
    case SP_DEF1: return &g_session1.def;
    case SP_DEF2: return &g_session1.plusDef;
    case SP_MDEF1: return &g_session1.mdef;
    case SP_MDEF2: return &g_session1.plusMdef;
    case SP_HIT: return &g_session1.hit;
    case SP_FLEE1: return &g_session1.flee;
    case SP_FLEE2: return &g_session1.plusFlee;
    case SP_CRITICAL: return &g_session1.critical;
    case SP_ASPD: return &g_session1.aspd;
    case SP_ASPD2: return &g_session1.plusAspd;
    }
    if (type >= SP_STR && type < SP_STR + 6)
        return &STAT_BASE(type - SP_STR);
    return 0;
}

void game_mode_zc_par_change(struct GameMode *self, struct PACKET_ZC_PAR_CHANGE *packet)
{
    struct String line;
    int type = packet->type;
    int value = packet->value;
    int *stat;

    string_init(&line);
    switch (type) {
    case SP_SPEED:
        if (g_session1.walkSpeed == value)
            break;
        g_session1.walkSpeed = value;
        session_invalidate_windows(&g_session1);
        break;
    case SP_KARMA:
        g_session1.karma = value;
        session_invalidate_windows(&g_session1);
        break;
    case SP_MANNER:
        set_manner(self, value, &line);
        session_invalidate_windows(&g_session1);
        break;
    case SP_HP:
        set_hp(self, value);
        break;
    case SP_MAXHP:
        set_hp_sp_field(&g_session1.maxHp, value);
        if (g_windowMgr.messenger_group_wnd)
            UIWINDOW_SENDMSG(g_windowMgr.messenger_group_wnd, UIM_PARTY_HP, 0x22, g_session1.ownAid, 0, 0);
        break;
    case SP_SP:
        set_hp_sp_field(&g_session1.sp, value);
        break;
    case SP_MAXSP:
        set_hp_sp_field(&g_session1.maxSp, value);
        break;
    case SP_STATUSPOINT:
        g_session1.statusPoints = value;
        session_invalidate_windows(&g_session1);
        if (g_windowMgr.status_wnd)
            UIWINDOW_SENDMSG(g_windowMgr.status_wnd, UIM_STATUS_REFRESH, 0, 0, 0, 0);
        break;
    case SP_BASELEVEL:
        base_level_up(self, value);
        break;
    case SP_SKILLPOINT:
        g_session1.skillPoints = value;
        session_invalidate_windows(&g_session1);
        session_refresh_skill_wnds(&g_session1);
        break;
    case SP_WEIGHT:
        set_weight(&packet->value);
        break;
    case SP_MAXWEIGHT:
        set_max_weight(&packet->value);
        break;
    case SP_JOBLEVEL:
        job_level_up(self, value);
        break;
    case SP_PKHONOR:
        if (value < -60)
            chat_line(msg_str(MSG_CHAT_SKILL_ITEM_BLOCKED), CHAT_COLOR_MANNER, 0);
        break;
    default:
        if (type >= SP_POW && type <= SP_TRAIT_LAST) {
            session_set_advanced_status(&g_session1, type, value);
            if (g_windowMgr.status_wnd)
                ui_status_wnd_restore_frame((struct UIStatusWnd *)g_windowMgr.status_wnd);
            session_invalidate_windows(&g_session1);
            break;
        }
        stat = stat_field(type);
        if (stat) {
            *stat = value;
            session_invalidate_windows(&g_session1);
            break;
        }
        break;
    }
    string_free(&line);
}
