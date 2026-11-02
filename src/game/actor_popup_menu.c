#include "actor/actor_ops.h"
#include "contents/whisper_block.h"
#include "session/number_codec.h"
#include "std/string.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "core/object_type.h"
#include "ui/window_access.h"
#include "platform/win_types.h"
#include <string.h>
#include "game/mode_msg.h"
#include "game/input.h"
#include "game/party_member.h"
#include "game/popup_entries.h"
#include "actor/actor_common.h"
#include "core/msg_str.h"
#include "game/store_search_items.h"
#include "platform/clientinfo.h"
#include "session/session_jobs.h"
#include "session/session_party.h"
#include "session/session_skills.h"
#include "session/session_talk.h"
#include "ui/ui_std.h"
#include "ui/window_mgr.h"

#define WID_POPUP_MENU          0x12
#define MSG_MENU_FIT            0x1e
#define MSG_MENU_ADD_ITEM       0x1f
#define MSG_MENU_ADD_SEPARATOR  0x48

#define OBJECT_TYPE_PET 7
#define SERVER_TYPE_GUILD 3

#define JOB_NPC_FIRST 0x2d
#define JOB_NPC_END   0x3e8

static struct UIWindow *menu_open(struct GameMode *self, int target)
{
    struct UIWindow *menu;

    if (g_windowMgr.menu_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, WID_POPUP_MENU);
    vec_clear(&self->popup_entries);
    menu = ui_window_open(WID_POPUP_MENU);
    self->popup_target_aid = target;
    return menu;
}

static void menu_entry(struct GameMode *self, int entry)
{
    vec_push(&self->popup_entries, &entry, sizeof entry);
}

static void menu_item(struct UIWindow *menu, const char *text)
{
    ui_window_send(menu, NULL, MSG_MENU_ADD_ITEM, (int)text, 0, 0, 0);
}

static void menu_row(struct GameMode *self, struct UIWindow *menu, int entry, int text_id)
{
    menu_entry(self, entry);
    menu_item(menu, msg_str(text_id));
}

static void menu_row_copy(struct GameMode *self, struct UIWindow *menu, char *line, int entry, int text_id)
{
    menu_entry(self, entry);
    sprintf(line, msg_str(text_id));
    menu_item(menu, line);
}

static void menu_row_printf(struct GameMode *self, struct UIWindow *menu, char *line, int entry, int text_id,
                            const char *name)
{
    menu_entry(self, entry);
    sprintf(line, msg_str(text_id), name);
    menu_item(menu, line);
}

static void menu_separator(struct UIWindow *menu)
{
    ui_window_send(menu, NULL, MSG_MENU_ADD_SEPARATOR, 0, 0, 0, 0);
}

static void menu_show(struct UIWindow *menu)
{
    ui_window_send(menu, NULL, MSG_MENU_FIT, 0, 0, 0, 0);
    ui_window_move_safe(menu, g_mouse.x + 1, g_mouse.y + 1);
}

static int companion_menu(struct GameMode *self, const struct PickInfo *pick, struct GameActor *actor)
{
    struct UIWindow *menu;
    char line[256];

    if (session_is_homunculus_aid(&g_session1, actor->aid)) {
        menu = menu_open(self, pick->aid);
        menu_row_copy(self, menu, line, POPUP_HOMUN_INFO, 0x458);
        menu_row_copy(self, menu, line, POPUP_HOMUN_FEED, 0x459);
        menu_row_copy(self, menu, line, POPUP_HOMUN_SHOW_EQUIP, 0x45a);
    } else if (session_is_mercenary_aid(&g_session1, pick->aid)) {
        menu = menu_open(self, pick->aid);
        menu_row_copy(self, menu, line, POPUP_MERCENARY_COMMAND, 0x458);
        menu_row_copy(self, menu, line, POPUP_MERCENARY_SHOW_EQUIP, 0x45a);
    } else if (pick->aid == g_session1.aid && actor->object_type == OBJECT_TYPE_PET) {
        menu = menu_open(self, pick->aid);
        menu_row_copy(self, menu, line, POPUP_PET_INFO, 0x254);
        menu_row_copy(self, menu, line, POPUP_PET_FEED, 0x250);
        menu_row_copy(self, menu, line, POPUP_PET_PERFORMANCE, 0x251);
        menu_row_copy(self, menu, line, POPUP_PET_UNEQUIP, 0x253);
        menu_row_copy(self, menu, line, POPUP_PET_RETURN_TO_EGG, 0x252);
    } else {
        return 0;
    }
    menu_show(menu);
    return 1;
}

static int may_invite_to_party(const char *name)
{
    struct PartyMember member;
    int may;

    if (!g_session1.partyLeader)
        return 0;
    may = session_get_party_member_by_name(&g_session1, &member, name)->found == 0;
    party_member_info_destroy(&member);
    return may;
}

static void guild_rows(struct GameMode *self, struct UIWindow *menu, char *line, int target, const char *name)
{
    struct GameActor *actor;
    int their_guild;

    if (!g_guildInfo.guild_id)
        return;
    actor = world_get_game_actor_by_aid(self->world, target);
    their_guild = actor ? game_actor_get_guild_id(actor) : 0;
    if (g_guildInfo.can_invite && their_guild == 0)
        menu_row_printf(self, menu, line, POPUP_GUILD_INVITE, 0x17e, name);
    if (g_guildInfo.is_master && their_guild != g_guildInfo.guild_id) {
        menu_separator(menu);
        menu_row(self, menu, POPUP_GUILD_ALLY, 0x18f);
        menu_row(self, menu, POPUP_GUILD_HOSTILE, 0x193);
    }
}

static void gm_rows(struct GameMode *self, struct UIWindow *menu, char *line, const struct String *name)
{
    menu_separator(menu);
    menu_row(self, menu, POPUP_GM_FULLSTRIP, 0x636);
    if (g_serverType != SERVER_TYPE_GUILD) {
        menu_row(self, menu, POPUP_GM_PROHIBIT_REMOVE, 0x185);
        menu_row(self, menu, POPUP_GM_PROHIBIT_ADD, 0x186);
        int prohibitions = session_count_prohibits_of(&g_session1, self->popup_target_aid);

        sprintf(line, msg_str(0x2c9), prohibitions);
        menu_entry(self, POPUP_PROHIBIT_LIST);
        menu_item(menu, line);
    }
    sprintf(line, "%s : %d (GID) : %s", string_cstr(&g_session1.serverName), self->popup_target_aid,
                string_cstr(name));
    menu_entry(self, POPUP_COPY_GID);
    menu_item(menu, line);
}

static void char_code_row(struct GameMode *self, struct UIWindow *menu, int target)
{
    struct String code, prefix, text;
    void *pc = object_as_type(world_get_game_actor_by_aid(self->world, target), OBJECT_PC);
    struct String *scrambled;

    if (!pc)
        return;
    scrambled = session_scramble_number(&g_session1, &code, actor_get_char_id(pc));
    string_init(&prefix);
    string_assign_cstr(&prefix, "[C-Code] : ");
    string_append(&prefix, string_cstr(scrambled), scrambled->size);
    string_move(&text, &prefix);
    string_free(&code);
    menu_entry(self, POPUP_COPY_CHAR_CODE);
    menu_item(menu, string_cstr(&text));
    string_free(&text);
}

static void player_menu(struct GameMode *self, const struct PickInfo *pick, struct GameActor *actor, char we_are_gm)
{
    struct UIWindow *menu;
    struct String name, tmp;
    char line[128];
    int blocked;
    struct Player *player;

    if (actor->is_store_assistant == 1) {
        if (we_are_gm != 1)
            return;
        menu = menu_open(self, pick->aid);
        menu_row(self, menu, POPUP_KICK_STORE_ASSISTANT, 0xda5);
        menu_show(menu);
        return;
    }

    string_init(&name);
    if (actor->name_masked == 1 && (self->world->is_pvp == 1 || self->world->is_siege == 1)) {
        string_assign_cstr(&name, "??????");
    } else {
        game_mode_get_actor_name(self, &tmp, pick->aid);
        string_assign(&name, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
    }

    menu = menu_open(self, pick->aid);
    memset(line, 0, sizeof line);
    if (we_are_gm)
        menu_row_printf(self, menu, line, POPUP_DISCONNECT, 0x1ca, string_cstr(&name));

    player = self->world->player;
    if (player && !is_effect_state_burrow(actor_get_effect_state(player))
        && !is_effect_state_star2(actor_get_effect_state(player)))
        menu_row_printf(self, menu, line, POPUP_VIEW_EQUIPMENT, 0x550, string_cstr(&name));
    if (player && !is_effect_state_burrow(actor_get_effect_state(player))
        && !is_effect_state_star2(actor_get_effect_state(player)))
        menu_row_printf(self, menu, line, POPUP_TRADE, 0x187, string_cstr(&name));
    if (may_invite_to_party(string_cstr(&name)))
        menu_row_printf(self, menu, line, POPUP_PARTY_INVITE, 0x188, string_cstr(&name));
    if (session_can_request_adoption(&g_session1, pick->aid) == 1)
        menu_row_printf(self, menu, line, POPUP_REQ_JOIN_BABY, 0x377, string_cstr(&name));
    guild_rows(self, menu, line, pick->aid, string_cstr(&name));

    menu_separator(menu);
    menu_row(self, menu, POPUP_ACTOR_INFO, 0x168);
    if (!clientinfo_is_admin_aid(pick->aid) && !ui_window_mgr_is_friend(&g_windowMgr, string_cstr(&name))
        && g_serverType != SERVER_TYPE_GUILD)
        menu_row(self, menu, POPUP_ADD_FRIEND, 0x166);
    menu_row(self, menu, POPUP_WRITE_RODEX, 0xaf8);

    menu_separator(menu);
    blocked = whisper_block_contains(g_whisperBlockMgr, string_cstr(&name));
    if (blocked)
        menu_row(self, menu, POPUP_WHISPER_UNBLOCK, 0xeeb);
    else
        menu_row(self, menu, POPUP_WHISPER_BLOCK, 0xeec);
    if (clientinfo_is_admin_aid(g_session1.ownAid))
        gm_rows(self, menu, line, &name);

    menu_separator(menu);
    char_code_row(self, menu, pick->aid);
    menu_show(menu);
    string_free(&name);
}

void game_mode_open_actor_popup_menu(struct GameMode *self, struct PickInfo *pick, int verdict)
{
    struct String name;
    struct GameActor *actor;
    char we_are_gm, nameless;
    int job;

    if (g_mouse.button[MOUSE_LEFT] == BUTTON_HELD)
        return;
    if ((unsigned char)((unsigned)GetAsyncKeyState(VK_SHIFT) >> 8))
        return;
    we_are_gm = clientinfo_is_admin_aid(g_session1.ownAid);
    if (!pick || verdict != 0 || g_mouse.button[MOUSE_RIGHT] != BUTTON_RELEASED || g_windowMgr.item_store_wnd)
        return;

    job = pick->job;
    if ((job >= JOB_NPC_FIRST && job < JOB_NPC_END) || job_is_monster(job)
        || session_is_homun_or_merc_job(&g_session1, job) || pick->kind == 4) {
        actor = world_get_game_actor_by_aid(self->world, pick->aid);
        if (actor && companion_menu(self, pick, actor))
            return;
    }

    if (pick->aid == g_session1.ownAid)
        return;
    if (!session_is_pc_job(&g_session1, job) && !we_are_gm)
        return;
    if (session_is_npc(&g_session1, pick->aid, job) == 1)
        return;
    if (!name_card_holder_has(&self->actor_names, pick->aid) && !we_are_gm)
        return;
    nameless = game_mode_get_actor_name(self, &name, pick->aid)->size == 0 && !we_are_gm;
    string_free(&name);
    if (nameless)
        return;
    if (self->right_press_x != g_mouse.x || self->right_press_y != g_mouse.y)
        return;
    if (!session_is_pc_job(&g_session1, job))
        return;
    actor = world_get_game_actor_by_aid(self->world, pick->aid);
    if (!actor)
        return;
    player_menu(self, pick, actor, we_are_gm);
}
