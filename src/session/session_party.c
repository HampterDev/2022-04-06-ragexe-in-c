#include "contents/player_names.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "game/party_member.h"
#include "ui/window_access.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_party.h"

#include <string.h>
#include "actor/actor_common.h"
#include "ui/ui_std.h"
#include "world/world.h"

#define EFFECT_PARTY_JOIN   0xa0

DWORD session_get_party_color_by_seq(struct Session *self, unsigned seq)
{
    static const DWORD colors[16] = {
        0xff0a3278, 0xff007878, 0xff000000, 0xff780000,
        0xff007800, 0xff787800, 0xff780078, 0xff783200,
        0xff787878, 0xff000078, 0xff003232, 0xff320000,
        0xff003200, 0xff323200, 0xff320032, 0xff780000,
    };
    if (seq > 15)
        return 0xff323232;
    return colors[seq];
}

static struct ListNode *party_find_aid(struct List *list, DWORD aid)
{
    struct ListNode *node;
    for (node = list->head->next; node != list->head; node = node->next)
        if (LIST_VALUE(node, struct PartyMember)->aid == aid)
            return node;
    return NULL;
}

static void destroy_member(void *value)
{
    party_member_info_destroy(value);
}

void session_add_friend(struct Session *self, struct PartyMember *m)
{
    struct List *list = &self->friendList;
    struct ListNode *node;
    struct PartyMember copy;
    unsigned seq = 0;

    for (node = list->head->next; node != list->head; node = node->next, seq++) {
        if (!string_equal(&LIST_VALUE(node, struct PartyMember)->name, &m->name))
            continue;
        m->color = session_get_party_color_by_seq(self, seq);
        party_member_assign(LIST_VALUE(node, struct PartyMember), m);
    }
    m->color = session_get_party_color_by_seq(self, seq);
    party_member_info_copy(&copy, m);
    list_push_back(list, &copy, sizeof copy);
}

void session_add_party_member(struct Session *self, struct PartyMember *m)
{
    struct List *list = &self->partyMembers;
    struct ListNode *node;
    struct PartyMember copy;
    unsigned seq = 0;
    struct GameMode *gameMode;

    if (m->map_name.size > 4) {
        size_t length = m->map_name.size - 3;
        string_resize(&m->map_name, length, 0);
        string_append_cstr(&m->map_name, "rsw");
    }
    for (node = list->head->next; node != list->head; node = node->next, seq++) {
        if (string_equal(&LIST_VALUE(node, struct PartyMember)->name, &m->name)) {
            m->color = session_get_party_color_by_seq(self, seq);
            party_member_assign(LIST_VALUE(node, struct PartyMember), m);
            return;
        }
    }
    m->color = session_get_party_color_by_seq(self, seq);
    party_member_info_copy(&copy, m);
    list_push_back(list, &copy, sizeof copy);

    gameMode = game_current_mode();
    if (gameMode) {
        if (game_map_has_party_marks()) {
            void *actor = game_find_actor(m->aid);
            if (actor && !able_to_make_effect_is_effect_exist(actor, EFFECT_PARTY_JOIN)) {
                struct Vec3 origin = { 0.0f, 0.0f, 0.0f };
                game_actor_launch_effect(actor, EFFECT_PARTY_JOIN, origin, 0.0f);
            }
        }
    }
}

void session_set_party_member_state(struct Session *self, DWORD aid, DWORD role)
{
    struct ListNode *node = party_find_aid(&self->partyMembers, aid);
    if (node)
        LIST_VALUE(node, struct PartyMember)->dead = role;
}

void session_set_party_member_job_level(struct Session *self, DWORD aid, WORD a, WORD b)
{
    struct ListNode *node = party_find_aid(&self->partyMembers, aid);
    if (node) {
        LIST_VALUE(node, struct PartyMember)->job = a;
        LIST_VALUE(node, struct PartyMember)->level = b;
    }
}

void session_set_party_member_role(struct Session *self, DWORD aid, DWORD value)
{
    struct ListNode *node = party_find_aid(&self->partyMembers, aid);
    if (node)
        LIST_VALUE(node, struct PartyMember)->role = value;
}

void session_clear_friends(struct Session *self)
{
    list_clear(&self->friendList, destroy_member);
}

#include "actor/render_ops.h"
#define ACTOR_MSG_STOP_EFFECT 0x6d

void session_clear_party_members(struct Session *self)
{
    struct GameMode *gameMode = game_current_mode();

    if (gameMode) {
        struct List *list = &self->partyMembers;
        struct ListNode *node;
        for (node = list->head->next; node != list->head; node = node->next) {
            void *actor = world_get_actor_or_player(game_current_world(),
                                     LIST_VALUE(node, struct PartyMember)->aid);
            if (actor && able_to_make_effect_is_effect_exist(actor, EFFECT_PARTY_JOIN) == 1)
                render_object_send(actor, NULL, ACTOR_MSG_STOP_EFFECT, EFFECT_PARTY_JOIN, 0, 0, 0);
        }
    }
    list_clear(&self->partyMembers, destroy_member);
    self->partyLeader = 0;
    string_clear(&self->partyName);
}

int session_remove_friend(struct Session *self, DWORD value)
{
    struct List *list = &self->friendList;
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next)
        if (LIST_VALUE(node, struct PartyMember)->char_id == value)
            break;
    if (node == list->head)
        return 0;
    list_erase(list, node, destroy_member);
    return 1;
}

void session_remove_party_member(struct Session *self, const char *name)
{
    struct List *list = &self->partyMembers;
    struct ListNode *node;
    struct GameMode *gameMode;
    void *actor;

    for (node = list->head->next; node != list->head; node = node->next) {
        const struct String *have = &LIST_VALUE(node, struct PartyMember)->name;
        if (string_equal_cstr(have, name))
            break;
    }
    if (node == list->head)
        return;
    gameMode = game_current_mode();
    if (!gameMode)
        return;
    actor = world_get_actor_or_player(game_current_world(), LIST_VALUE(node, struct PartyMember)->aid);
    if (actor && able_to_make_effect_is_effect_exist(actor, EFFECT_PARTY_JOIN) == 1)
        render_object_send(actor, NULL, ACTOR_MSG_STOP_EFFECT, EFFECT_PARTY_JOIN, 0, 0, 0);
    list_erase(list, node, destroy_member);
}

DWORD session_get_party_member_aid(struct Session *self, const char *name)
{
    struct List *list = &self->partyMembers;
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next) {
        const struct String *have = &LIST_VALUE(node, struct PartyMember)->name;
        if (string_equal_cstr(have, name))
            return LIST_VALUE(node, struct PartyMember)->aid;
    }
    return 0;
}

static struct PartyMember *default_member(struct PartyMember *out)
{
    out->found = 0;
    string_init(&out->name);
    string_init(&out->map_name);
    out->dead = 0;
    return out;
}

struct PartyMember *session_get_party_member(struct Session *self,
                                                               struct PartyMember *out, DWORD aid)
{
    struct ListNode *node;

    for (node = self->partyMembers.head->next; node != self->partyMembers.head; node = node->next)
        if (LIST_VALUE(node, struct PartyMember)->aid == aid)
            return party_member_info_copy(out, LIST_VALUE(node, struct PartyMember));
    return default_member(out);
}

struct PartyMember *session_get_party_member_by_name(struct Session *self,
                                                                struct PartyMember *out, const char *name)
{
    struct ListNode *node;

    for (node = self->partyMembers.head->next; node != self->partyMembers.head; node = node->next) {
        const struct String *have = &LIST_VALUE(node, struct PartyMember)->name;
        if (string_equal_cstr(have, name))
            return party_member_info_copy(out, LIST_VALUE(node, struct PartyMember));
    }
    return default_member(out);
}

void session_hi_friends(struct Session *self, const char *text)
{
    struct String hi;
    struct ListNode *node;

    string_init(&hi);
    string_assign_cstr(&hi, "(Hi) ");
    if (text)
        string_append_cstr(&hi, text);
    else
        string_append_cstr(&hi, "*^_^*");
    for (node = self->friendList.head->next; node != self->friendList.head; node = node->next) {
        const struct PartyMember *m = LIST_VALUE(node, struct PartyMember);
        void *mode;
        signed char n;

        string_assign_cstr(&g_windowMgr.chat_line, string_cstr(&hi));
        mode = client_current_mode();
        mode_send_message(mode, 0xb, (intptr_t)string_cstr(&m->name), 0, 0, 0);
        n = (signed char)(g_silentWhisperAcks + 1);
        g_silentWhisperAcks = n > 0x28 ? 0x28 : n;
    }
    string_free(&hi);
}

#define MODE_MSG_MEMBER_GONE 0xb0

void session_rename_friend(struct Session *self, int id, const char *name)
{
    struct ListNode *node;
    struct UIWindow *wnd;

    for (node = self->friendList.head->next; node != self->friendList.head; node = node->next) {
        struct PartyMember *m = LIST_VALUE(node, struct PartyMember);

        if ((int)m->char_id != id)
            continue;
        if (strlen(name) == 0) {
            void *mode = client_current_mode();
            mode_send_message(mode, MODE_MSG_MEMBER_GONE, (int)m->aid,
                                                         (int)m->char_id, 0, 0);
            list_erase(&self->friendList, node, destroy_member);
        } else {
            string_assign_cstr(&m->name, name);
        }
        break;
    }
    wnd = ui_window_find(0x22);
    if (wnd)
        ui_window_refresh(wnd);
}

#define MSG_FRIEND_LOGGED_IN  0x411
#define MSG_FRIEND_LOGGED_OUT  0x412

int session_set_friend_state(struct Session *self, int aid, int char_id,
                                                BYTE value)
{
    struct ListNode *node;
    struct PartyMember *m = NULL;
    char line[0x100];

    for (node = self->friendList.head->next; node != self->friendList.head; node = node->next) {
        m = LIST_VALUE(node, struct PartyMember);
        if ((int)m->aid == aid && (int)m->char_id == char_id)
            break;
    }
    if (node == self->friendList.head)
        return 0;
    m->state = value;
    if (g_session1.showLoginNotices == 1) {
        if (m->name.size == 0 && game_current_mode()) {
            {
                struct TextView cached_name = player_name_lookup((int)m->char_id);
                string_assign(&m->name, cached_name.data, cached_name.size);
            }
        }
        sprintf(line, msg_str(m->state == 0 ? MSG_FRIEND_LOGGED_IN : MSG_FRIEND_LOGGED_OUT), string_cstr(&m->name));
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)line, 0xffff00, 0, 0);
    }
    return 1;
}
