#include "ui/window_types.h"
#include "actor/actor_ops.h"
#include "world/actor_lookup.h"
#include "ui/guild_windows.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include <string.h>
#include "contents/client_res.h"
#include "contents/game_session.h"
#include "contents/guild_info.h"
#include "std/std.h"
#include "contents/ui_window_mgr.h"
#include "core/msg_str.h"
#include "ui/ui_std.h"

struct GuildInfo g_guildInfo;

#define MSG_UNKNOWN_NAME  0x245

static void destroy_position(void *value)
{
    guild_position_destruct(value);
}

static void destroy_member(void *value)
{
    guild_member_destruct(value);
}

static void destroy_relation(void *value)
{
    guild_relation_destruct(value);
}

static void destroy_expelled(void *value)
{
    guild_expelled_destruct(value);
}

static void destroy_total_info_row(void *value)
{
    guild_total_info_row_destruct(value);
}

struct GuildInfo *guild_info_construct(struct GuildInfo *self)
{
    string_init(&self->name);
    string_init(&self->master_name);
    string_init(&self->notice_subject);
    string_init(&self->notice_body);
    string_init(&self->castle_name);

    list_init(&self->positions);
    list_init(&self->members);
    list_init(&self->relations);
    list_init(&self->expelled);
    list_init(&self->total_info);

    tree_init(&self->position_names);
    tree_init(&self->member_position_edits);
    tree_init(&self->position_edits);

    self->castles.first = NULL;
    self->castles.last = NULL;
    self->castles.end = NULL;
    guild_info_reset(self);
    return self;
}

void guild_info_destruct(struct GuildInfo *self)
{
    vec_free(&self->castles);
    tree_free(&self->position_edits, guild_position_edit_destroy);
    tree_free(&self->member_position_edits, NULL);
    tree_free(&self->position_names, guild_position_name_destroy);

    list_free(&self->total_info, destroy_total_info_row);
    list_free(&self->expelled, destroy_expelled);
    list_free(&self->relations, destroy_relation);
    list_free(&self->members, destroy_member);
    list_free(&self->positions, destroy_position);

    string_free(&self->castle_name);
    string_free(&self->notice_body);
    string_free(&self->notice_subject);
    string_free(&self->master_name);
    string_free(&self->name);
}

void guild_info_reset(struct GuildInfo *self)
{
    struct Player *player;
    int i;

    self->guild_id = 0;
    player = game_current_player();
    if (player)
        game_actor_set_guild_id((struct GameActor *)player, 0);

    self->can_invite = 0;
    self->can_expel = 0;
    self->is_master = 0;
    self->master_gid = 0;
    self->menu_flags = -1;
    for (i = 0; i < GUILD_PAGE_COUNT; i++)
        self->page_asked[i] = 0;
    string_clear(&self->name);
    string_clear(&self->master_name);
    string_clear(&self->notice_subject);
    string_clear(&self->notice_body);
    self->level = 0;
    self->member_count = 0;
    self->max_members = 0;
    self->average_level = 0;
    string_clear(&self->castle_name);
    self->exp = 0;
    self->exp_next = 0;
    self->tax_points = 0;
    self->honor = 0;
    self->virtue = 0;

    list_clear(&self->positions, destroy_position);
    list_clear(&self->members, destroy_member);
    list_clear(&self->relations, destroy_relation);
    list_clear(&self->expelled, destroy_expelled);
    list_clear(&self->total_info, destroy_total_info_row);
    tree_clear(&self->position_names, guild_position_name_destroy);
    tree_clear(&self->member_position_edits, NULL);
    tree_clear(&self->position_edits, guild_position_edit_destroy);
    self->castles.last = self->castles.first;
}

void guild_info_set_master(struct GuildInfo *self, int gid)
{
    self->master_gid = gid;
    self->is_master = gid == g_session1.ownGid;
}

void guild_info_resolve_name(struct GuildInfo *self, int gid, const char *name)
{
    struct String unknown;
    struct ListNode *head, *node;
    void *window;

    if (self->master_gid == gid)
        string_assign_cstr(&self->master_name, name);

    string_init(&unknown);
    string_assign_cstr(&unknown, msg_str(MSG_UNKNOWN_NAME));

    head = self->members.head;
    for (node = head->next; node != head; node = node->next) {
        struct GuildMember *m = LIST_VALUE(node, struct GuildMember);
        if (m->gid != gid)
            continue;
        if (m->name.size == 0 || string_equal(&m->name, &unknown))
            string_assign_cstr(&m->name, name);
        break;
    }

    head = self->expelled.head;
    for (node = head->next; node != head; node = node->next) {
        struct GuildExpelled *e = LIST_VALUE(node, struct GuildExpelled);
        if (e->gid != gid)
            continue;
        if (e->name.size == 0 || string_equal(&e->name, &unknown))
            string_assign_cstr(&e->name, name);
        break;
    }

    if (window_mgr_guild_window(&g_windowMgr)) {
        window = window_mgr_guild_window(&g_windowMgr);
        ui_window_invalidate(window);
    }
    string_free(&unknown);
}

void guild_info_add_member(struct GuildInfo *self, const struct GuildMember *member)
{
    struct GuildMember copy;
    void *world, *actor;

    guild_member_copy(&copy, member);
    list_push_back(&self->members, &copy, sizeof copy);
    world = game_current_world();
    if (world) {
        actor = world_find_pc_by_char_id(world, member->gid);
        if (actor)
            game_actor_set_guild_id(actor, self->guild_id);
    }
}

int guild_info_remove_member(struct GuildInfo *self, int gid)
{
    struct ListNode *head = self->members.head;
    struct ListNode *node;
    void *world, *actor;

    for (node = head->next; node != head; node = node->next)
        if (LIST_VALUE(node, struct GuildMember)->gid == gid)
            break;
    if (node == head)
        return 0;

    world = game_current_world();
    if (world) {
        actor = world_find_pc_by_char_id(world, LIST_VALUE(node, struct GuildMember)->gid);
        if (actor)
            game_actor_set_guild_id(actor, 0);
    }
    list_erase(&self->members, node, destroy_member);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
    return 1;
}

int guild_info_remove_member_by_name(struct GuildInfo *self, const char *name)
{
    struct ListNode *head = self->members.head;
    struct ListNode *node;

    for (node = head->next; node != head; node = node->next)
        if (string_equal_cstr(&LIST_VALUE(node, struct GuildMember)->name, name))
            break;
    if (node == head)
        return 0;
    list_erase(&self->members, node, destroy_member);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
    return 1;
}

void guild_info_clear_members(struct GuildInfo *self)
{
    list_clear(&self->members, destroy_member);
}

int guild_info_member_aid_by_name(struct GuildInfo *self, const char *name)
{
    struct ListNode *head = self->members.head;
    struct ListNode *node;

    for (node = head->next; node != head; node = node->next) {
        struct GuildMember *m = LIST_VALUE(node, struct GuildMember);
        if (string_equal_cstr(&m->name, name))
            return m->aid;
    }
    return -1;
}

void guild_info_set_member_position(struct GuildInfo *self, int aid, int gid, int position)
{
    struct ListNode *head = self->members.head;
    struct ListNode *node;
    struct GuildPositionName *pos_name;

    for (node = head->next; node != head; node = node->next) {
        struct GuildMember *m = LIST_VALUE(node, struct GuildMember);
        if (m->aid != aid || m->gid != gid)
            continue;
        m->position = position;
        pos_name = tree_get(&self->position_names, &m->position, sizeof m->position, sizeof *pos_name,
                            tree_less_int);
        string_assign(&m->position_name, string_cstr(&pos_name->name), pos_name->name.size);
        return;
    }
}

void guild_info_count_online(struct GuildInfo *self)
{
    struct ListNode *head = self->members.head;
    struct ListNode *node;
    int count = 0;

    for (node = head->next; node != head; node = node->next)
        if (LIST_VALUE(node, struct GuildMember)->online != 0)
            count++;
    self->online_count = count;
}

void guild_info_set_member_status(struct GuildInfo *self, int online, int gid,
                                             short sex, short hair_style, short hair_color)
{
    struct ListNode *head = self->members.head;
    struct ListNode *node;

    for (node = head->next; node != head; node = node->next) {
        struct GuildMember *m = LIST_VALUE(node, struct GuildMember);
        if (m->gid == gid) {
            m->sex = sex;
            m->hair_style = hair_style;
            m->hair_color = hair_color;
            m->online = online;
            break;
        }
    }
    guild_info_count_online(self);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

void guild_info_set_member_online(struct GuildInfo *self, int aid, int online)
{
    struct ListNode *head = self->members.head;
    struct ListNode *node;

    for (node = head->next; node != head; node = node->next) {
        if (LIST_VALUE(node, struct GuildMember)->aid == aid) {
            LIST_VALUE(node, struct GuildMember)->online = online;
            break;
        }
    }
    guild_info_count_online(self);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

void guild_info_update_member_shares(struct GuildInfo *self)
{
    struct ListNode *head = self->members.head;
    struct ListNode *node;
    struct GuildPositionName *pos_name;
    double sum = 0.0;

    for (node = head->next; node != head; node = node->next)
        sum += (double)LIST_VALUE(node, struct GuildMember)->exp;

    for (node = head->next; node != self->members.head; node = node->next) {
        struct GuildMember *m = LIST_VALUE(node, struct GuildMember);
        float share = 0.0f;
        if (sum != 0.0)
            share = (float)m->exp * 100.0f / (float)sum;
        m->exp_share = share;
        pos_name = tree_get(&self->position_names, &m->position, sizeof m->position, sizeof *pos_name,
                            tree_less_int);
        string_assign(&m->position_name, string_cstr(&pos_name->name), pos_name->name.size);
    }
}

void guild_info_update_position(struct GuildInfo *self, struct GuildPosition position)
{
    struct ListNode *head = self->positions.head;
    struct ListNode *node;

    for (node = head->next; node != head; node = node->next) {
        struct GuildPosition *p = LIST_VALUE(node, struct GuildPosition);
        if (p->position_id != position.position_id)
            continue;
        p->ranking = position.ranking;
        p->position_id = position.position_id;
        string_assign(&p->name, string_cstr(&position.name), position.name.size);
        p->can_invite = position.can_invite;
        p->can_expel = position.can_expel;
        p->exp_tax = position.exp_tax;
        p->can_use_storage = position.can_use_storage;
        break;
    }
    string_free(&position.name);
}

void guild_info_remove_relation(struct GuildInfo *self, int id, int kind)
{
    struct ListNode *head = self->relations.head;
    struct ListNode *node;

    for (node = head->next; node != head; node = node->next) {
        struct GuildRelation *r = LIST_VALUE(node, struct GuildRelation);
        if (r->id != id || r->kind != kind)
            continue;
        list_erase(&self->relations, node, destroy_relation);
        window_mgr_invalidate_guild_wnd(&g_windowMgr);
        return;
    }
}

int guild_info_count_relations(struct GuildInfo *self, int kind)
{
    struct ListNode *head = self->relations.head;
    struct ListNode *node;
    int count = 0;

    for (node = head->next; node != head; node = node->next)
        if (LIST_VALUE(node, struct GuildRelation)->kind == kind)
            count++;
    return count;
}

void guild_info_add_expelled(struct GuildInfo *self, const struct GuildExpelled *entry)
{
    struct GuildExpelled copy;

    guild_expelled_copy(&copy, entry);
    list_push_back(&self->expelled, &copy, sizeof copy);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

void guild_info_add_castle(struct GuildInfo *self, int id)
{
    int *it, *last = (int *)self->castles.last;

    for (it = (int *)self->castles.first; it != last; it++)
        if (*it == id)
            return;
    vec_push(&self->castles, &id, sizeof(int));
}

void guild_info_remove_castle(struct GuildInfo *self, int id)
{
    int *it, *last = (int *)self->castles.last;

    for (it = (int *)self->castles.first; it != last; it++) {
        if (*it == id) {
            memmove(it, it + 1, (char *)last - (char *)(it + 1));
            self->castles.last = last - 1;
            return;
        }
    }
}

void guild_info_clear_castles(struct GuildInfo *self)
{
    self->castles.last = self->castles.first;
}

int guild_info_castle_count(struct GuildInfo *self)
{
    return (int)VEC_COUNT(&self->castles, int);
}

int guild_info_castle_at(struct GuildInfo *self, int index)
{
    return ((int *)self->castles.first)[index];
}

char guild_info_has_kind0_relation(struct GuildInfo *self, int guild_id)
{
    struct ListNode *head = self->relations.head;
    struct ListNode *node;

    for (node = head->next; node != head; node = node->next) {
        struct GuildRelation *r = LIST_VALUE(node, struct GuildRelation);
        if (r->id == guild_id && r->kind == 0)
            return 1;
    }
    return 0;
}
