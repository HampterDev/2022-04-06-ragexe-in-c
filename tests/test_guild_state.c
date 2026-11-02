#include "contents/guild_info.h"
#include "contents/game_session.h"
#include "contents/client_res.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/guild_windows.h"
#include "ui/window_types.h"
#include "ui/social/guild_requests.h"
#include "ui/social/guild_member_sort.h"
#include "game/context.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "core/fatal.h"
#include "std/std.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static struct { unsigned char *p; size_t size; } blocks[512];
static unsigned live, invalidations, name_refreshes, actor_updates, sends;
static int world_present, window_present, mode, world, expected_master;
static int requested[32];
static void *window_slots[36];
static struct { void **slots; int guild_id; } player, member_actor;
static struct { void **slots; } window;

void *mem_alloc(size_t size)
{
    unsigned i;
    unsigned char *p = malloc(size + 16);
    assert(p);
    memset(p, 0xa5, size + 16);
    for (i = 0; i < 512 && blocks[i].p; ++i) {}
    assert(i < 512);
    blocks[i].p = p;
    blocks[i].size = size;
    ++live;
    return p;
}

void mem_free(void *memory)
{
    size_t size;
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 512 && blocks[i].p != memory; ++i) {}
    assert(i < 512);
    size = blocks[i].size;
    for (j = 0; j < 16; ++j) assert(blocks[i].p[size + j] == 0xa5);
    free(memory);
    blocks[i].p = NULL;
    --live;
}

__declspec(noreturn) void fatal_error(const char *message)
{ (void)message; abort(); }

void game_actor_set_guild_id(struct GameActor *self, int id)
{
    assert((void *)self == &player || (void *)self == &member_actor);
    if ((void *)self == &player) player.guild_id = id;
    else member_actor.guild_id = id;
    ++actor_updates;
}
static void refresh_name(void *self)
{ assert(self == &window); ++name_refreshes; }
struct Player *game_current_player(void)
{ return world_present ? (struct Player *)&player : NULL; }
struct World *game_current_world(void)
{ return world_present ? (struct World *)&world : NULL; }
void *world_find_pc_by_char_id(struct World *self, int gid)
{ assert((void *)self == &world); return gid == 42 ? &member_actor : NULL; }
void window_mgr_invalidate_guild_wnd(struct UIWindowMgr *self)
{ assert(self == &g_windowMgr); ++invalidations; }
struct UIWindow *window_mgr_guild_window(struct UIWindowMgr *self)
{ assert(self == &g_windowMgr); return window_present ? (struct UIWindow *)&window : NULL; }
char *msg_str(int id) { assert(id == 0x245); return "unknown"; }
struct Mode *client_current_mode(void) { return (struct Mode *)&mode; }
intptr_t mode_send_message(struct Mode *self, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(self == (struct Mode *)&mode && message == 0x6b);
    assert(!b && !c && !d && sends < 32);
    assert(g_guildInfo.master_gid == expected_master);
    requested[sends++] = a;
    return 1;
}

static void set_text(struct String *s, const char *text)
{ string_assign_cstr(s, text); }
static void expect_text(const struct String *s, const char *text)
{ assert(s->size == strlen(text) && strcmp(string_cstr(s), text) == 0); }
static struct GuildMember *member_at(unsigned index)
{
    struct ListNode *node = g_guildInfo.members.head->next;
    while (index--) node = node->next;
    assert(node != g_guildInfo.members.head);
    return LIST_VALUE(node, struct GuildMember);
}

static void test_requests(void)
{
    int page, other;
    g_guildInfo.master_gid = expected_master = 0x123456;
    assert(!g_guildInfo.master_name.size);
    for (page = 0; page < GUILD_PAGE_COUNT; ++page) {
        memset(g_guildInfo.page_asked, 0, sizeof g_guildInfo.page_asked);
        sends = 0;
        guild_request_page((enum GuildPage)page);
        assert(sends == 2 && requested[0] == GUILD_PAGE_INFO && requested[1] == page);
        assert(g_guildInfo.master_gid == expected_master);
        for (other = 0; other < GUILD_PAGE_COUNT; ++other)
            assert(g_guildInfo.page_asked[other] == (page == other));
    }
    set_text(&g_guildInfo.master_name, "Known master");
    sends = 0;
    for (page = 0; page < GUILD_PAGE_COUNT; ++page) {
        guild_request_page((enum GuildPage)page);
        assert(requested[page] == page && g_guildInfo.page_asked[page] == 1);
    }
    assert(sends == GUILD_PAGE_COUNT && g_guildInfo.master_gid == expected_master);
    guild_request_page(GUILD_PAGE_TOTAL_INFO);
    assert(sends == GUILD_PAGE_COUNT + 1);
    guild_request_page((enum GuildPage)-1);
    guild_request_page(GUILD_PAGE_COUNT);
    assert(sends == GUILD_PAGE_COUNT + 1);
    guild_info_reset(&g_guildInfo);
    for (page = 0; page < GUILD_PAGE_COUNT; ++page) assert(!g_guildInfo.page_asked[page]);
    assert(!g_guildInfo.master_gid && !g_guildInfo.master_name.size);
}

static const char *sort_names[] = { "Beta", "Alpha", "Gamma", "Alpha" };

const char *session_get_job_name(struct Session *session, int job, int unused)
{
    assert(session == &g_session1 && job >= 0 && job < 4 && unused == -1);
    return sort_names[job];
}

static int sort_compare(const struct GuildMember *a, const struct GuildMember *b, int key)
{
    switch (key) {
    case 0: return (a->exp > b->exp) - (a->exp < b->exp);
    case 1: return strcmp(sort_names[a->job], sort_names[b->job]);
    case 2: return (a->level > b->level) - (a->level < b->level);
    case 3: return strcmp(string_cstr(&a->name), string_cstr(&b->name));
    case 4: return (a->online > b->online) - (a->online < b->online);
    case 5: return (a->position > b->position) - (a->position < b->position);
    default: return (a->exp_share > b->exp_share) - (a->exp_share < b->exp_share);
    }
}

static void destroy_member(void *value)
{
    guild_member_destruct(value);
}

static void test_member_sorts(void)
{
    static const struct { GuildMemberSort sort; int key, descending; } cases[] = {
        {guild_member_sort_exp_up,0,0}, {guild_member_sort_exp_down,0,1},
        {guild_member_sort_job_up,1,0}, {guild_member_sort_job_down,1,1},
        {guild_member_sort_level_up,2,0}, {guild_member_sort_level_down,2,1},
        {guild_member_sort_name_up,3,0}, {guild_member_sort_name_down,3,1},
        {guild_member_sort_online_first,4,1},
        {guild_member_sort_position_up,5,0}, {guild_member_sort_position_down,5,1},
        {guild_member_sort_share_up,6,0}, {guild_member_sort_share_down,6,1}
    };
    static const int counts[] = {0,1,9}, keys[] = {5,2,5,0,3,1,3,2,4};
    unsigned c, n, i, pass, before;
    for (c = 0; c < sizeof cases / sizeof cases[0]; ++c) {
        for (n = 0; n < sizeof counts / sizeof counts[0]; ++n) {
            struct List list;
            struct ListNode *nodes[9], *node;
            struct GuildMember snapshot[9];
            list_init(&list);
            for (i = 0; i < (unsigned)counts[n]; ++i) {
                struct GuildMember value = {0};
                guild_member_construct(&value);
                value.aid = (int)i; value.exp = keys[i]; value.job = keys[i] % 4;
                value.level = keys[i] * 10; value.online = keys[i] % 2;
                value.position = 9 - keys[i]; value.exp_share = keys[i] * 0.25f;
                set_text(&value.name, sort_names[value.job]);
                set_text(&value.memo, "Sorting preserves this owned member memo");
                nodes[i] = LIST_NODE_OF(list_push_back(&list, &value, sizeof value));
                snapshot[i] = *LIST_VALUE(nodes[i], struct GuildMember);
            }
            before = live;
            for (pass = 0; pass < 2; ++pass) {
                struct GuildMember *previous = NULL;
                unsigned seen = 0, walked = 0;
                cases[c].sort(&list);
                assert(live == before && list.size == (unsigned)counts[n]);
                for (node = list.head->next; node != list.head; node = node->next) {
                    struct GuildMember *value = LIST_VALUE(node, struct GuildMember);
                    unsigned id = (unsigned)value->aid;
                    assert(walked++ < list.size && id < list.size && !(seen & (1u << id)));
                    seen |= 1u << id;
                    assert(node == nodes[id] && node->prev->next == node && node->next->prev == node);
                    assert(!memcmp(value, &snapshot[id], sizeof *value));
                    if (previous) {
                        int comparison = sort_compare(previous, value, cases[c].key);
                        assert(cases[c].descending ? comparison >= 0 : comparison <= 0);
                    }
                    previous = value;
                }
                assert(walked == list.size && list.head->next->prev == list.head);
                assert(list.head->prev->next == list.head);
            }
            list_free(&list, destroy_member);
        }
    }
    puts("Guild sorting: all 13 orders, empty/singleton/odd lists, equal keys, repeated sorts and unchanged owned nodes passed");
}

static void test_members(void)
{
    struct GuildMember value = {0};
    struct GuildMember *first, *second;
    struct GuildPositionName *title;
    int key = 7;
    unsigned before;
    guild_member_construct(&value);
    value.aid = 12; value.gid = 42; value.exp = 25; value.position = key;
    set_text(&value.name, "Member with a long owned name");
    set_text(&value.memo, "A memo exceeding inline capacity");
    set_text(&value.position_name, "An initial position name");
    g_guildInfo.guild_id = 314;
    world_present = 1;
    before = actor_updates;
    guild_info_add_member(&g_guildInfo, &value);
    assert(actor_updates == before + 1 && member_actor.guild_id == 314);
    first = member_at(0);
    assert(first->name.data != value.name.data && first->memo.data != value.memo.data);
    guild_member_destruct(&value);
    expect_text(&first->name, "Member with a long owned name");
    expect_text(&first->memo, "A memo exceeding inline capacity");
    guild_member_construct(&value);
    value.aid = 13; value.gid = 43; value.exp = 75; value.online = 1; value.position = 0;
    set_text(&value.name, "unknown");
    guild_info_add_member(&g_guildInfo, &value);
    guild_member_destruct(&value);
    assert(g_guildInfo.members.size == 2 && actor_updates == before + 1);
    second = member_at(1);
    title = tree_get(&g_guildInfo.position_names, &key, sizeof key, sizeof *title, tree_less_int);
    set_text(&title->name, "A long position title");
    guild_info_update_member_shares(&g_guildInfo);
    assert(first->exp_share == 25.0f && second->exp_share == 75.0f);
    expect_text(&first->position_name, "A long position title");
    guild_info_set_member_position(&g_guildInfo, 13, 999, key);
    assert(second->position == 0);
    guild_info_set_member_position(&g_guildInfo, 13, 43, key);
    expect_text(&second->position_name, "A long position title");
    guild_info_set_member_status(&g_guildInfo, 1, 42, 1, 22, 3);
    assert(first->sex == 1 && first->hair_style == 22 && first->hair_color == 3);
    assert(g_guildInfo.online_count == 2);
    guild_info_set_member_online(&g_guildInfo, 12, 0);
    assert(g_guildInfo.online_count == 1);
    first->exp = second->exp = 0;
    guild_info_update_member_shares(&g_guildInfo);
    assert(!first->exp_share && !second->exp_share);
    assert(guild_info_member_aid_by_name(&g_guildInfo, "Member with a long owned name") == 12);
    assert(guild_info_member_aid_by_name(&g_guildInfo, "Member") == -1);
    g_session1.ownGid = 43;
    guild_info_set_master(&g_guildInfo, 42);
    assert(!g_guildInfo.is_master && g_guildInfo.master_gid == 42);
    guild_info_set_master(&g_guildInfo, 43);
    assert(g_guildInfo.is_master && g_guildInfo.master_gid == 43);
    window_present = 1;
    guild_info_resolve_name(&g_guildInfo, 43, "A resolved master name");
    expect_text(&second->name, "A resolved master name");
    expect_text(&g_guildInfo.master_name, "A resolved master name");
    assert(name_refreshes == 1);
    window_present = 0;
    guild_info_resolve_name(&g_guildInfo, 42, "Must not replace known member");
    expect_text(&first->name, "Member with a long owned name");
    assert(!guild_info_remove_member(&g_guildInfo, 999));
    assert(guild_info_remove_member(&g_guildInfo, 42) && member_actor.guild_id == 0);
    assert(!guild_info_remove_member_by_name(&g_guildInfo, "A resolved"));
    assert(guild_info_remove_member_by_name(&g_guildInfo, "A resolved master name"));
    assert(!g_guildInfo.members.size);
}

static void test_other_owned_values(void)
{
    struct GuildRelation relation, relation_copy;
    struct GuildExpelled expelled;
    struct GuildPosition position = {0}, copy;
    struct GuildPositionEdit *pending;
    struct GuildMemberEdit *edit;
    struct ListNode *node;
    struct GuildTotalInfoRow *total;
    int key = 7, i;
    relation.id = 91; relation.kind = 0;
    string_init(&relation.name);
    set_text(&relation.name, "Related guild with a long name");
    guild_relation_copy(&relation_copy, &relation);
    list_push_back(&g_guildInfo.relations, &relation_copy, sizeof relation_copy);
    relation.kind = 1;
    guild_relation_copy(&relation_copy, &relation);
    list_push_back(&g_guildInfo.relations, &relation_copy, sizeof relation_copy);
    string_free(&relation.name);
    assert(guild_info_count_relations(&g_guildInfo, 0) == 1);
    assert(guild_info_has_kind0_relation(&g_guildInfo, 91));
    guild_info_remove_relation(&g_guildInfo, 91, 0);
    assert(!guild_info_has_kind0_relation(&g_guildInfo, 91));
    assert(guild_info_count_relations(&g_guildInfo, 1) == 1);
    expelled.gid = 55;
    string_init(&expelled.name); string_init(&expelled.account_name); string_init(&expelled.reason);
    set_text(&expelled.name, "unknown");
    set_text(&expelled.account_name, "Owned secondary string");
    set_text(&expelled.reason, "A long expulsion reason");
    guild_info_add_expelled(&g_guildInfo, &expelled);
    string_free(&expelled.name); string_free(&expelled.account_name); string_free(&expelled.reason);
    guild_info_resolve_name(&g_guildInfo, 55, "Resolved expelled member");
    expect_text(&LIST_VALUE(g_guildInfo.expelled.head->next, struct GuildExpelled)->name, "Resolved expelled member");
    expect_text(&LIST_VALUE(g_guildInfo.expelled.head->next, struct GuildExpelled)->reason, "A long expulsion reason");
    string_init(&position.name);
    set_text(&position.name, "A long guild position");
    position.position_id = key; position.ranking = 9;
    guild_position_copy(&copy, &position);
    node = LIST_NODE_OF(list_push_back(&g_guildInfo.positions, &copy, sizeof copy));
    position.ranking = 3; position.exp_tax = 25; position.can_invite = 1;
    set_text(&position.name, "An updated guild position");
    guild_info_update_position(&g_guildInfo, position);
    expect_text(&LIST_VALUE(node, struct GuildPosition)->name, "An updated guild position");
    assert(LIST_VALUE(node, struct GuildPosition)->ranking == 3);
    edit = tree_get(&g_guildInfo.member_position_edits, &key, sizeof key, sizeof *edit, tree_less_int);
    assert(!edit->change.aid && !edit->change.gid && !edit->change.position);
    edit->change.aid = 12; edit->change.gid = 42; edit->change.position = 7;
    assert(tree_get(&g_guildInfo.member_position_edits, &key, sizeof key, sizeof *edit, tree_less_int) == edit);
    pending = tree_get(&g_guildInfo.position_edits, &key, sizeof key, sizeof *pending, tree_less_int);
    set_text(&pending->position.name, "A pending position edit");
    assert(tree_get(&g_guildInfo.position_edits, &key, sizeof key, sizeof *pending, tree_less_int) == pending);
    total = list_push_back(&g_guildInfo.total_info, NULL, sizeof *total);
    total->id = 12;
    string_init(&total->name); set_text(&total->name, "A total-info row name");
    for (i = 0; i < 4; ++i) total->values[i] = i;
    for (i = 0; i < 25; ++i) guild_info_add_castle(&g_guildInfo, i);
    guild_info_add_castle(&g_guildInfo, 7);
    assert(guild_info_castle_count(&g_guildInfo) == 25);
    guild_info_remove_castle(&g_guildInfo, 7);
    guild_info_remove_castle(&g_guildInfo, 99);
    assert(guild_info_castle_count(&g_guildInfo) == 24 && guild_info_castle_at(&g_guildInfo, 7) == 8);
    guild_info_clear_castles(&g_guildInfo);
    assert(!guild_info_castle_count(&g_guildInfo));
    guild_info_add_castle(&g_guildInfo, 31);
}

int main(void)
{
    struct GuildInfo fresh = {0};
    window_slots[offsetof(struct UIWindowOps, invalidate) / sizeof(void *)] = refresh_name;
    window.slots = window_slots;
    assert(!g_guildInfo.guild_id && !g_guildInfo.members.head);
    assert(guild_info_construct(&g_guildInfo) == &g_guildInfo);
    assert(live == 8 && g_guildInfo.menu_flags == -1 && !g_guildInfo.name.size);
    test_requests();
    test_member_sorts();
    test_members();
    test_other_owned_values();
    guild_info_destruct(&g_guildInfo);
    assert(!live && invalidations);
    world_present = 0;
    guild_info_construct(&fresh);
    assert(live == 8);
    guild_info_destruct(&fresh);
    assert(!live);
    puts("guild state: page identity, member updates, owned containers and teardown passed");
    return 0;
}
