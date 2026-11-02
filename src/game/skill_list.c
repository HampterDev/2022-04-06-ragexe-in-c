#include "game/packet_handlers.h"
#include "std/list.h"
#include "skills/skill_bundle.h"
#include "session/skill_queries.h"
#include "skills/skill_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/skill_list.h"
#include "game/client.h"
#include "core/msg_str.h"
#include "game/store_search_items.h"
#include "session/session_jobs.h"
#include "session/session_skills.h"
#include "session/session_windows.h"

#define SKILL_ID_LAST_PLAIN 0x157b

void game_mode_skill_list(int packet_id, const struct Vec *entries)
{
    const struct SkillListEntry *e;
    struct SkillInfo skill;

    if (packet_id != HEADER_ZC_SKILLINFO_LIST && packet_id != HEADER_ZC_SKILLINFO_LIST3) {
        if (packet_id == HEADER_ZC_HOSKILLINFO_LIST)
            list_clear(&g_session1.homunSkills, skill_info_destroy);
        else if (packet_id == HEADER_ZC_MER_SKILLINFO_LIST)
            list_clear(&g_session1.merSkills, skill_info_destroy);
    }

    for (e = (const struct SkillListEntry *)entries->first; e != (const struct SkillListEntry *)entries->last; e++) {
        skill_info_construct(&skill);
        if ((e->id < 1 || e->id > SKILL_ID_LAST_PLAIN)
            && !session_is_homunculus_skill(&g_session1, e->id)
            && !session_is_merc_or_elemental_skill(&g_session1, e->id)) {
            skill_info_destruct(&skill);
            continue;
        }
        skill.id = e->id;
        skill.type = e->type;
        skill.level = e->level;
        skill.sp_cost = e->sp;
        skill.upgradable = e->upgradable;
        skill.range = e->range;
        skill.id_name = skill_info_get_id_name(&skill);
        skill.level2 = e->level2;
        skill.ap_amount = skill_info_get_ap_amount(&skill, e->level);
        player_skill_bundle_reconcile_level(&g_session1.skills, &skill);

        if (packet_id == HEADER_ZC_SKILLINFO_LIST || packet_id == HEADER_ZC_SKILLINFO_LIST3)
            session_put_skill(&g_session1, &skill, 0, 0);
        else if (packet_id == HEADER_ZC_HOSKILLINFO_LIST)
            session_put_homun_skill(&g_session1, &skill, 0);
        else if (packet_id == HEADER_ZC_MER_SKILLINFO_LIST)
            session_put_mer_skill(&g_session1, &skill, 0);
        skill_info_destruct(&skill);
    }

    if (packet_id == HEADER_ZC_SKILLINFO_LIST || packet_id == HEADER_ZC_SKILLINFO_LIST3)
        session_refresh_skill_wnds(&g_session1);
    else if (packet_id == HEADER_ZC_HOSKILLINFO_LIST)
        session_refresh_homun_skill_item_wnd(&g_session1);
    else if (packet_id == HEADER_ZC_MER_SKILLINFO_LIST)
        session_refresh_mer_skill_item_wnd(&g_session1);
}

void game_mode_skill_update(struct GameMode *self, unsigned short id, short level, short sp,
                                     unsigned char upgradable, short range, int type, unsigned short level2)
{
    struct SkillInfo skill;

    self->skill_upgrade_pending = 0;
    session_get_skill_info(&skill, id);
    skill.level = level;
    skill.sp_cost = sp;
    skill.upgradable = upgradable;
    skill.range = range;
    skill.type = type;
    skill.id = id;
    skill.level2 = level2;
    if (session_is_homunculus_skill(&g_session1, id))
        session_put_homun_skill(&g_session1, &skill, 1);
    else if (session_is_merc_or_elemental_skill(&g_session1, id))
        session_put_mer_skill(&g_session1, &skill, 1);
    else
        session_put_skill_keep_type(&g_session1, &skill, 1);
    skill_info_destruct(&skill);
}

#pragma pack(push, 1)
struct PACKET_ZC_SKILLINFO_UPDATE {
    unsigned short id;
    unsigned short skill_id;
    short level;
    short sp;
    short range;
    unsigned char upgradable;
};
struct OldSkillListEntry {
    unsigned short id;
    int type;
    short level;
    short sp;
    short range;
    char name[24];
    unsigned char upgradable;
};
struct PACKET_ZC_SKILL_SELECT_REQUEST {
    unsigned short id;
    unsigned short length;
    int why;
    short skill_ids[1];
};
#pragma pack(pop)
SIZE_ASSERT(OldSkillListEntry, 37);

enum {
    HEADER_ZC_SKILLINFO_UPDATE = 0x10e,
    HEADER_ZC_HOSKILLINFO_UPDATE = 0x239,
    HEADER_ZC_MER_SKILLINFO_UPDATE = 0x29e,
};
enum { WID_SKILL_SELECT = 0x63 };
enum {
    SKILLSELECT_MSG_WHY = 0x1b,
    SKILLSELECT_MSG_ADD = 0x1f,
};

void game_mode_zc_skill_info_update(struct GameMode *self, const struct PACKET_ZC_SKILLINFO_UPDATE *packet)
{
    struct SkillInfo skill, copy;
    int known = 1;

    self->skill_upgrade_pending = 0;
    skill_info_construct(&skill);
    if (packet->id == HEADER_ZC_SKILLINFO_UPDATE)
        skill_table_find(&g_session1.skills, &copy, packet->skill_id);
    else if (packet->id == HEADER_ZC_HOSKILLINFO_UPDATE)
        session_get_homun_skill_by_id(&g_session1, &copy, packet->skill_id);
    else if (packet->id == HEADER_ZC_MER_SKILLINFO_UPDATE)
        session_get_mer_skill_by_id(&g_session1, &copy, packet->skill_id);
    else
        known = 0;
    if (known) {
        skill_info_assign(&skill, &copy);
        skill_info_destruct(&copy);
    }

    skill.id = packet->skill_id;
    skill.level = packet->level;
    skill.sp_cost = packet->sp;
    skill.upgradable = packet->upgradable;
    skill.range = packet->range;
    skill.ap_amount = skill_info_get_ap_amount(&skill, packet->level);
    player_skill_bundle_reconcile_level(&g_session1.skills, &skill);
    if (packet->id == HEADER_ZC_SKILLINFO_UPDATE)
        session_put_skill(&g_session1, &skill, 1, 0);
    else if (packet->id == HEADER_ZC_HOSKILLINFO_UPDATE)
        session_put_homun_skill(&g_session1, &skill, 1);
    else if (packet->id == HEADER_ZC_MER_SKILLINFO_UPDATE)
        session_put_mer_skill(&g_session1, &skill, 1);
    skill_info_destruct(&skill);
}

void game_mode_zc_skill_select_request(const struct PACKET_ZC_SKILL_SELECT_REQUEST *packet)
{
    int count = (packet->length - 8) / 2, i;
    struct UIWindow *wnd;

    if (count <= 0)
        return;
    wnd = ui_window_open(WID_SKILL_SELECT);
    if (wnd == 0)
        return;
    UIWINDOW_SENDMSG(wnd, SKILLSELECT_MSG_WHY, packet->why, 0, 0, 0);
    for (i = 0; i < count; i++)
        UIWINDOW_SENDMSG(wnd, SKILLSELECT_MSG_ADD, packet->skill_ids[i], 0, 0, 0);
}

enum {
    WINDOW_AUTOSPELL = 0x63,
    AUTOSPELL_MSG_ADD = 0x1f,
};

void game_mode_on_autospell_list2(int count, const int *skills)
{
    struct UIWindow *wnd;
    unsigned char valid = 0;
    int i;

    if (count <= 0 || skills == 0)
        return;
    for (i = 0; i < count; i++)
        if (skills[i] > 0)
            valid++;
    if ((signed char)valid <= 0)
        return;
    wnd = ui_window_open(WINDOW_AUTOSPELL);
    if (wnd == 0)
        return;
    for (i = 0; i < count; i++)
        if (skills[i] > 0)
            UIWINDOW_SENDMSG(wnd, AUTOSPELL_MSG_ADD, skills[i], 0, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_AUTOSPELLLIST {
    short id;
    int skills[7];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_AUTOSPELLLIST, 30);

void game_mode_zc_auto_spell_list(const unsigned char *pkt)
{
    const struct PACKET_ZC_AUTOSPELLLIST *packet = (const struct PACKET_ZC_AUTOSPELLLIST *)pkt;
    const int *skills = packet->skills;
    unsigned char count = 0;
    struct UIWindow *wnd;
    int i;

    for (i = 0; i < 7; i++)
        if (skills[i] != 0)
            count++;
    if (count == 0)
        return;
    wnd = ui_window_open(WINDOW_AUTOSPELL);
    if (wnd == 0)
        return;
    for (i = 0; i < 7; i++)
        if (skills[i] != 0)
            UIWINDOW_SENDMSG(wnd, AUTOSPELL_MSG_ADD, skills[i], 0, 0, 0);
}

enum {
    WINDOW_MAKING_LIST = 0x5e,
    MAKING_LIST_MSG_START = 0x4b,
    MAKING_LIST_MSG_ADD = 0x1f,
    MAKING_LIST_MSG_KIND = 0x22,
    MAKING_KIND_ARROWS = 2,
};

#pragma pack(push, 1)
struct MakingListEntry {
    int item_id;
};
#pragma pack(pop)
SIZE_ASSERT(MakingListEntry, 4);

#pragma pack(push, 1)
struct PACKET_ZC_MAKINGARROW_LIST {
    short id;
    unsigned short length;
    struct MakingListEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MAKINGARROW_LIST, 4);

#pragma pack(push, 1)
struct PACKET_ZC_MAKINGITEM_LIST {
    short id;
    unsigned short length;
    short kind;
    struct MakingListEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MAKINGITEM_LIST, 6);

void game_mode_zc_making_arrow_list(const unsigned char *pkt)
{
    const struct PACKET_ZC_MAKINGARROW_LIST *packet = (const struct PACKET_ZC_MAKINGARROW_LIST *)pkt;
    unsigned n = (unsigned)(packet->length - 4) >> 2;
    const struct MakingListEntry *entry = packet->entries;
    struct UIWindow *wnd;

    wnd = ui_window_open(WINDOW_MAKING_LIST);
    UIWINDOW_SENDMSG(wnd, MAKING_LIST_MSG_START, 0, 0, 0, 0);
    for (; n != 0; n--, entry++)
        UIWINDOW_SENDMSG(wnd, MAKING_LIST_MSG_ADD, (intptr_t)&entry->item_id, 0, 0, 0);
    UIWINDOW_SENDMSG(wnd, MAKING_LIST_MSG_KIND, MAKING_KIND_ARROWS, 0, 0, 0);
}

void game_mode_zc_making_item_list(const unsigned char *pkt)
{
    const struct PACKET_ZC_MAKINGITEM_LIST *packet = (const struct PACKET_ZC_MAKINGITEM_LIST *)pkt;
    unsigned n = (unsigned)(packet->length - 6) >> 2;
    const struct MakingListEntry *entry = packet->entries;
    struct UIWindow *wnd;

    wnd = ui_window_open(WINDOW_MAKING_LIST);
    if (wnd == 0)
        return;
    UIWINDOW_SENDMSG(wnd, MAKING_LIST_MSG_START, 0, 0, 0, 0);
    UIWINDOW_SENDMSG(wnd, MAKING_LIST_MSG_KIND, packet->kind, 0, 0, 0);
    for (; n != 0; n--, entry++)
        UIWINDOW_SENDMSG(wnd, MAKING_LIST_MSG_ADD, (intptr_t)&entry->item_id, 0, 0, 0);
}

#pragma pack(push, 1)
struct GuildSkillEntry {
    unsigned short id;
    int type;
    short level;
    short sp;
    short range;
    char name[24];
    unsigned char upgradable;
};
struct PACKET_ZC_GUILD_SKILLINFO {
    unsigned short id;
    short length;
    short skill_points;
    struct GuildSkillEntry skills[1];
};
#pragma pack(pop)
SIZE_ASSERT(GuildSkillEntry, 37);

#define GUILD_SKILL_FIRST        10000
#define GUILD_SKILL_COUNT        20
#define UIM_SKILLS_CHANGED       0x17

void game_mode_zc_guild_skill_info(struct GameMode *self, const struct PACKET_ZC_GUILD_SKILLINFO *packet)
{
    const struct GuildSkillEntry *e = packet->skills;
    struct SkillInfo skill;
    int n;

    self->skill_upgrade_pending = 0;
    list_clear(&g_session1.guildSkills, skill_info_destroy);
    for (n = (int)((unsigned)(packet->length - 6) / sizeof *e); n > 0; n--, e++) {
        skill_info_construct(&skill);
        skill.level = e->level;
        skill.id = e->id;
        skill.sp_cost = e->sp;
        skill.type = e->type;
        skill.upgradable = e->upgradable;
        skill.id_name = skill_info_get_id_name(&skill);
        skill.ap_amount = skill_info_get_ap_amount(&skill, e->level);
        if ((unsigned)(skill.id - GUILD_SKILL_FIRST) < GUILD_SKILL_COUNT)
            session_put_skill4(&g_session1, &skill);
        skill_info_destruct(&skill);
    }
    g_session1.guildSkillPoints = packet->skill_points;
    session_invalidate_windows(&g_session1);
    if (g_windowMgr.guild_skill_wnd)
        ui_window_send(g_windowMgr.guild_skill_wnd, NULL, UIM_SKILLS_CHANGED, 0, 0, 0, 0);
    if (g_windowMgr.short_cut_wnd)
        ui_window_send(g_windowMgr.short_cut_wnd, NULL, UIM_SKILLS_CHANGED, 0, 0, 0, 0);
}

void game_mode_zc_skill_info_list(struct GameMode *self, const unsigned char *pkt)
{
    struct Vec entries = { 0, 0, 0 };
    const struct PACKET_ZC_SKILLINFO_LIST *packet = (const struct PACKET_ZC_SKILLINFO_LIST *)pkt;
    int n = (int)((unsigned)(packet->length - 4) / sizeof(struct SkillListEntryV1));
    const struct SkillListEntryV1 *e = packet->skills;
    int i;

    for (i = 0; i < n; i++, e++) {
        struct SkillListEntry entry;

        entry.id = e->id;
        entry.type = e->type;
        entry.level = e->level;
        entry.sp = e->sp;
        entry.range = e->range;
        entry.upgradable = e->upgradable;
        entry.level2 = e->level;
        vec_push(&entries, &entry, sizeof entry);
    }
    game_mode_skill_list(packet->id, &entries);
    vec_free(&entries);
}

#pragma pack(push, 1)
struct PACKET_ZC_SKILLINFO_LIST3 {
    short id;
    unsigned short length;
    struct SkillListEntry skills[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SKILLINFO_LIST3, 4);

void game_mode_zc_skill_info_list3(struct GameMode *self, const unsigned char *pkt)
{
    struct Vec entries = { 0, 0, 0 };
    const struct PACKET_ZC_SKILLINFO_LIST3 *packet = (const struct PACKET_ZC_SKILLINFO_LIST3 *)pkt;
    int n = (int)((unsigned)(packet->length - 4) / sizeof(struct SkillListEntry));
    const struct SkillListEntry *e = packet->skills;
    int i;

    for (i = 0; i < n; i++, e++)
        vec_push(&entries, e, sizeof *e);
    game_mode_skill_list(packet->id, &entries);
    vec_free(&entries);
}

#define SKILL_MSG_COLOR         0xffcdcd
#define CHAT_CHANNEL_SKILL      0xa

#pragma pack(push, 1)
struct PACKET_ZC_MSG_SKILL {
    short id;
    unsigned short skill_id;
    int msg_id;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MSG_SKILL, 8);

void game_mode_zc_msg_skill(const unsigned char *pkt)
{
    const struct PACKET_ZC_MSG_SKILL *packet = (const struct PACKET_ZC_MSG_SKILL *)pkt;
    char text[0x100], line[0x100];

    sprintf(text, msg_str(packet->msg_id));
    sprintf(line, "[%s] %s", skill_name(packet->skill_id), text);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)line, SKILL_MSG_COLOR, CHAT_CHANNEL_SKILL, 0);
}

#define MM_AUTORUN_SKILL         0x71

#pragma pack(push, 1)
struct PACKET_ZC_AUTORUN_SKILL {
    short id;
    unsigned short skill_id;
    int type;
    short level;
    short sp_cost;
    short range;
    char name[24];
    unsigned char upgradable;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_AUTORUN_SKILL, 39);

void game_mode_zc_autorun_skill(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_AUTORUN_SKILL *packet = (const struct PACKET_ZC_AUTORUN_SKILL *)pkt;
    struct SkillInfo skill;

    skill_info_construct(&skill);
    skill.level = packet->level;
    skill.type = packet->type;
    skill.id = packet->skill_id;
    skill.upgradable = packet->upgradable;
    skill.id_name = skill_info_get_id_name(&skill);
    skill.sp_cost = packet->sp_cost;
    skill.range = packet->range;
    skill.ap_amount = skill_info_get_ap_amount(&skill, packet->level);
    player_skill_bundle_put_autorun_skill(&g_session1.skills, &skill);
    mode_send_msg(self, MM_AUTORUN_SKILL, (intptr_t)&skill, skill.level, 1, -1);
    skill_info_destruct(&skill);
}
