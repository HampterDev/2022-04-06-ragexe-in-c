#include "actor/render_object.h"
#include "session/option_pairs.h"
#include "session/state.h"
#include "std/std.h"
#include "ui/ui_std.h"
#include "ui/chat/sub_chat_map.h"
#include "ui/controls/listbox.h"
#include "skills/skill_bundle.h"
#include "ui/window_types.h"
#define SKILL_FOUND(skill) ((skill)->found)
#include "core/fatal.h"
#include "world/world.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct { unsigned char *memory; size_t bytes; } blocks[128];
static unsigned live_blocks, allocations;

void *mem_alloc(size_t bytes)
{
    unsigned i;
    unsigned char *p = malloc(bytes + 16);
    assert(p);
    memset(p, 0xa5, bytes + 16);
    for (i = 0; i < 128 && blocks[i].memory; ++i) {}
    assert(i < 128);
    blocks[i].memory = p;
    blocks[i].bytes = bytes;
    ++live_blocks;
    ++allocations;
    return p;
}

void mem_free(void *memory)
{
    size_t bytes;
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 128 && blocks[i].memory != memory; ++i) {}
    assert(i < 128);
    bytes = blocks[i].bytes;
    for (j = 0; j < 16; ++j) assert(blocks[i].memory[bytes + j] == 0xa5);
    free(memory);
    blocks[i].memory = NULL;
    --live_blocks;
}

__declspec(noreturn) void fatal_error(const char *message)
{
    (void)message;
    abort();
}

static void instant_values(void)
{
    static const int keys[] = { INT_MIN, -1, 0, 1, INT_MAX };
    static const long long values[] = {
        LLONG_MIN, -0x1234567887654321LL, -1, 0, 1,
        0x80000000LL, 0xffffffffLL, 0x1234567887654321LL, LLONG_MAX
    };
    static struct Session session;
    struct GameActor actor, expected;
    unsigned i, j, before;

    memset(&actor, 0x5a, sizeof actor);
    tree_init(&session.instantVars);
    tree_init(&actor.vars);
    assert(session_get_instant_var(&session, 42) == 0);
    for (i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
        session_set_pair(&session, keys[i], values[i]);
        game_actor_set_instant_var(&actor, keys[i], values[i + 1]);
    }
    before = allocations;
    for (j = 0; j < sizeof values / sizeof values[0]; ++j) {
        for (i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
            long long actor_value = values[(j + 1) % (sizeof values / sizeof values[0])];
            session_set_pair(&session, keys[i], values[j]);
            game_actor_set_instant_var(&actor, keys[i], actor_value);
            assert(((struct IntInt64Pair *)tree_find(&session.instantVars, &keys[i], tree_less_int))->value
                   == values[j]);
            assert(((struct IntInt64Pair *)tree_find(&actor.vars, &keys[i], tree_less_int))->value == actor_value);
            assert(session_get_instant_var(&session, keys[i]) == (int)values[j]);
        }
    }
    assert(allocations == before && session.instantVars.size == 5 && actor.vars.size == 5);
    for (i = 0; i < sizeof values / sizeof values[0]; ++i) {
        for (j = 0; j < sizeof values / sizeof values[0]; ++j) {
            actor.sphere_count = values[i];
            expected = actor;
            expected.sphere_count = values[j];
            assert(game_actor_update_sphere_count(&actor, values[j]) == (values[j] > values[i]));
            assert(!memcmp(&actor, &expected, sizeof actor));
        }
    }
    tree_free(&session.instantVars, NULL);
    tree_free(&actor.vars, NULL);
    memset(&session.instantVars, 0, sizeof session.instantVars);
    assert(!live_blocks);
    puts("native values: signed 64-bit options, actor values, replacement, independent maps and sphere comparisons passed");
}

static void integer_maps(void)
{
    static const int keys[] = { INT_MAX, 0, -1, INT_MIN, 1 };
    static const int ordered[] = { INT_MIN, -1, 0, 1, INT_MAX };
    struct Tree small, wide;
    struct TreeNode *a, *b;
    struct IntIntPair *narrow_value;
    struct IntInt64Pair *wide_value;
    int key;
    unsigned i;

    tree_init(&small);
    tree_init(&wide);
    for (i = 0; i < 5; ++i) {
        narrow_value = tree_get(&small, &keys[i], sizeof keys[i], sizeof *narrow_value, tree_less_int);
        wide_value = tree_get(&wide, &keys[i], sizeof keys[i], sizeof *wide_value, tree_less_int);
        assert(!TREE_NODE_OF(narrow_value)->is_head && !TREE_NODE_OF(wide_value)->is_head);
        assert(narrow_value->value == 0);
        assert(wide_value->value == 0);
        narrow_value->value = (int)i + 10;
        wide_value->value = 0x1234567800000000LL + i;
    }
    a = tree_first(&small);
    b = tree_first(&wide);
    for (i = 0; i < 5; ++i) {
        assert(TREE_VALUE(a, struct IntIntPair)->key == ordered[i]);
        assert(TREE_VALUE(b, struct IntInt64Pair)->key == ordered[i]);
        assert(tree_find(&small, &ordered[i], tree_less_int) == TREE_VALUE(a, void));
        assert(tree_find(&wide, &ordered[i], tree_less_int) == TREE_VALUE(b, void));
        a = tree_next(a);
        b = tree_next(b);
    }
    assert(a == small.head && b == wide.head);
    for (i = 0; i < 5; ++i) {
        assert(((struct IntIntPair *)tree_find(&small, &keys[i], tree_less_int))->value == (int)i + 10);
        assert(((struct IntInt64Pair *)tree_find(&wide, &keys[i], tree_less_int))->value
               == 0x1234567800000000LL + i);
    }
    key = -1;
    wide_value = tree_get(&wide, &key, sizeof key, sizeof *wide_value, tree_less_int);
    wide_value->value = -0x1234567800000000LL;
    assert(wide.size == 5);
    assert(((struct IntInt64Pair *)tree_find(&wide, &key, tree_less_int))->value == -0x1234567800000000LL);
    key = 12;
    wide_value = tree_get(&wide, &key, sizeof key, sizeof *wide_value, tree_less_int);
    wide_value->value = LLONG_MIN;
    assert(wide.size == 6);
    assert(((struct IntInt64Pair *)tree_find(&wide, &key, tree_less_int))->value == LLONG_MIN);
    tree_free(&small, NULL);
    tree_free(&wide, NULL);
    assert(live_blocks == 0);
}

struct TestTreeTab {
    int job;
    struct List skills;
};

static void skill_trees(void)
{
    struct PlayerSkillBundle bundle = { 0 };
    struct SkillInfo info = { 0 };
    struct TestTreeTab *tab;
    struct List *list;
    const struct SkillInfo *copy;
    int key = -1;

    tree_init(&bundle.tree_tabs);
    info.id = 123;
    info.level = 7;
    player_skill_bundle_add_tree_skill(&bundle, -1, &info);
    info.id = 456;
    player_skill_bundle_add_tree_skill(&bundle, -1, &info);
    player_skill_bundle_add_tree_skill(&bundle, INT_MAX, &info);
    assert(bundle.tree_tabs.size == 2);
    tab = tree_find(&bundle.tree_tabs, &key, tree_less_int);
    list = &tab->skills;
    assert(list->size == 2);
    copy = LIST_VALUE(list->head->next, struct SkillInfo);
    assert(copy->id == 123 && copy->level == 7);
    copy = LIST_VALUE(list->head->prev, struct SkillInfo);
    assert(copy->id == 456 && copy->level == 7);
    player_skill_bundle_clear_trees(&bundle);
    assert(bundle.tree_tabs.size == 0 && bundle.tree_tabs.head->parent == bundle.tree_tabs.head);
    assert(live_blocks == 1);
    tree_free(&bundle.tree_tabs, NULL);
    assert(live_blocks == 0);
}

static void skill_values(void)
{
    struct SkillInfo source = {0}, copy, assigned = {0}, empty = {0};
    struct SkillRequirement *requirements;
    void *saved;
    unsigned before, i;

    skill_info_construct(&source);
    source.id = 42; source.type = 3; source.level = 4;
    source.sp_cost = 12; source.upgradable = 1; source.range = 9;
    source.id_name = "skill-name";
    source.cell_index = 7; source.max_level = 10; source.level_limit = 8;
    source.level2 = 2;
    requirements = std_allocate_bytes(8 * sizeof *requirements);
    for (i = 0; i < 8; ++i) {
        requirements[i].skill_id = (int)i + 100;
        requirements[i].level = (int)i + 1;
    }
    source.requirements.first = requirements;
    source.requirements.last = requirements + 2;
    source.requirements.end = requirements + 8;

    memset(&copy, 0xa5, sizeof copy);
    assert(skill_info_copy(&copy, &source) == &copy);
    assert(SKILL_FOUND(&copy) == 1);
    assert(copy.id == 42 && copy.type == 3 && copy.level == 4);
    assert(copy.sp_cost == 12 && copy.upgradable == 1 && copy.range == 9);
    assert(copy.id_name == source.id_name && copy.cell_index == 7);
    assert(copy.max_level == 10 && copy.level_limit == 8);
    assert(copy.level2 == 2);
    assert(copy.requirements.first != source.requirements.first);
    assert(vec_count(&copy.requirements, sizeof *requirements) == 2);
    assert(vec_capacity(&copy.requirements, sizeof *requirements) == 2);
    assert(!memcmp(copy.requirements.first, requirements, 2 * sizeof *requirements));
    requirements[0].level = 99;
    assert(((struct SkillRequirement *)copy.requirements.first)[0].level == 1);

    assert(skill_info_assign(&assigned, &source) == &assigned);
    assert(assigned.id == 42 && assigned.id_name == source.id_name);
    assert(assigned.requirements.first != requirements);
    source.requirements.last = requirements + 5;
    skill_info_assign(&assigned, &source);
    assert(vec_capacity(&assigned.requirements, sizeof *requirements) == 5);
    source.requirements.last = requirements + 6;
    skill_info_assign(&assigned, &source);
    assert(vec_capacity(&assigned.requirements, sizeof *requirements) == 7);
    before = allocations;
    saved = assigned.requirements.first;
    source.requirements.last = requirements + 7;
    skill_info_assign(&assigned, &source);
    assert(assigned.requirements.first == saved && allocations == before);
    assert(!memcmp(saved, requirements, 7 * sizeof *requirements));
    source.requirements.last = requirements + 1;
    skill_info_assign(&assigned, &source);
    assert(assigned.requirements.last == (struct SkillRequirement *)saved + 1);
    skill_info_assign(&assigned, &assigned);
    assert(assigned.requirements.first == saved && allocations == before);
    assert(((struct SkillRequirement *)saved)[0].level == 99);

    skill_info_construct_unknown(&empty);
    assert(!SKILL_FOUND(&empty) && !strcmp(empty.id_name, "Unknown-Skill"));
    assert(!vec_count(&empty.requirements, sizeof *requirements));
    assert(!vec_capacity(&empty.requirements, sizeof *requirements));
    skill_info_assign(&assigned, &empty);
    assert(assigned.requirements.first == saved && assigned.requirements.last == saved);
    assert(vec_capacity(&assigned.requirements, sizeof *requirements) == 7);
    assert(allocations == before);
    skill_info_destruct(&copy);
    skill_info_assign(&copy, &empty);
    assert(!copy.requirements.first && !copy.requirements.last && !copy.requirements.end);
    skill_info_copy(&copy, &empty);
    assert(!copy.requirements.first && allocations == before);
    skill_info_destruct(&source);
    skill_info_destruct(&copy);
    skill_info_destruct(&copy);
    skill_info_destruct(&assigned);
    skill_info_destruct(&empty);
    assert(!live_blocks);
}

static void ui_map_entries(void)
{
    static const int keys[] = { 0, INT_MAX, -1, INT_MIN, 2, 1 };
    struct Tree chat;
    struct SubChatWnd *windows[6];
    struct UIWindow borrowed = {0};
    unsigned i, before;

    tree_init(&chat);
    for (i = 0; i < 6; ++i) {
        before = allocations;
        windows[i] = tree_get(&chat, &keys[i], sizeof keys[i], sizeof *windows[i], tree_less_int);
        assert(allocations == before + 1);
        assert(chat.size == i + 1);
        assert(!windows[i]->wnd);
        windows[i]->wnd = &borrowed;
    }
    before = allocations;
    for (i = 0; i < 6; ++i) {
        assert(tree_get(&chat, &keys[i], sizeof keys[i], sizeof *windows[i], tree_less_int) == windows[i]);
        assert(windows[i]->wnd == &borrowed);
    }
    assert(allocations == before && chat.size == 6);
    tree_free(&chat, NULL);
    assert(!live_blocks);
    puts("UI map entries: missing keys, default values, signed ordering, duplicate reuse, Korean text and guarded cleanup passed");
}

static struct List *cleaned_list;
static unsigned cleanup_count, cleanup_size;

static void cleanup_string(void *value)
{
    struct ListNode *node = LIST_NODE_OF(value);

    assert(cleaned_list->size == cleanup_size);
    if (!cleanup_size) {
        assert(cleaned_list->head->next == cleaned_list->head);
        assert(cleaned_list->head->prev == cleaned_list->head);
    } else {
        assert(node->prev->next == node->next);
        assert(node->next->prev == node->prev);
    }
    ++cleanup_count;
    string_free(value);
}

static void shared_list_ownership(void)
{
    static const char long_text[] = "A string whose owned buffer is longer than the inline capacity";
    struct List pointers = {0}, strings = {0};
    struct ListNode *nodes[3], *head;
    struct String moved, copied, small, copy, *value;
    const char *owned_buffer;
    int values[3] = {11, 22, 33};
    unsigned i, before;

    list_init(&pointers);
    for (i = 0; i < 3; ++i) {
        void *pointer = &values[i];
        nodes[i] = LIST_NODE_OF(list_push_back(&pointers, &pointer, sizeof pointer));
    }
    assert(pointers.size == 3 && pointers.head->prev == nodes[2]);
    assert(list_erase(&pointers, nodes[0], NULL) == nodes[1]);
    assert(list_erase(&pointers, nodes[1], NULL) == nodes[2]);
    assert(pointers.size == 1 && pointers.head->next == nodes[2]);
    assert(*LIST_VALUE(nodes[2], void *) == &values[2]);
    list_free(&pointers, NULL);
    assert(!pointers.head && values[0] == 11 && values[1] == 22 && values[2] == 33 && !live_blocks);

    list_init(&strings);
    head = strings.head;
    string_init(&moved);
    string_init(&copied);
    string_init(&small);
    string_assign(&moved, long_text, sizeof long_text - 1);
    owned_buffer = string_cstr(&moved);
    before = allocations;
    value = list_push_back(&strings, &moved, sizeof moved);
    string_init(&moved);
    nodes[0] = LIST_NODE_OF(value);
    assert(!moved.size && !moved.capacity && !moved.data);
    assert(string_cstr(value) == owned_buffer);
    assert(allocations == before + 1);

    string_assign(&copied, long_text, sizeof long_text - 1);
    string_copy(&copy, &copied);
    value = list_push_back(&strings, &copy, sizeof copy);
    nodes[1] = LIST_NODE_OF(value);
    assert(!strcmp(string_cstr(value), long_text));
    assert(string_cstr(value) != string_cstr(&copied));
    string_assign_cstr(&copied, "changed");
    assert(!strcmp(string_cstr(value), long_text));

    string_assign_cstr(&small, "short");
    list_push_back(&strings, &small, sizeof small);
    string_init(&small);
    assert(strings.size == 3 && !small.size);
    cleaned_list = &strings;
    cleanup_count = 0;
    cleanup_size = 2;
    list_erase(&strings, nodes[1], cleanup_string);
    assert(cleanup_count == 1 && strings.size == 2);
    cleanup_size = 0;
    list_clear(&strings, cleanup_string);
    assert(cleanup_count == 3 && strings.head == head && !strings.size);
    list_clear(&strings, cleanup_string);
    assert(cleanup_count == 3);

    string_assign(&moved, long_text, sizeof long_text - 1);
    list_push_back(&strings, &moved, sizeof moved);
    string_init(&moved);
    list_free(&strings, cleanup_string);
    assert(cleanup_count == 4 && !strings.head);
    string_free(&moved);
    string_free(&copied);
    string_free(&small);
    assert(!live_blocks);
    puts("Shared lists: borrowed pointers, range erase, moved/copied strings, callback ordering and guarded cleanup passed");
}

extern struct UIWindow *map_nodes_test_window;
extern unsigned map_nodes_window_queries;
static unsigned skill_notifications;
static unsigned skill_digest = 2166136261u;
static FILE *skill_trace;

static void skill_observe(unsigned value)
{
    unsigned i;
    for (i = 0; i < 4; ++i) {
        unsigned char byte = (unsigned char)(value >> (i * 8));
        skill_digest = (skill_digest ^ byte) * 16777619u;
        if (skill_trace) assert(fwrite(&byte, 1, 1, skill_trace) == 1);
    }
}

static int skill_notify(struct UIWindow *self, struct UIWindow *sender, int msg,
                        int a, int b, int c, int d)
{
    assert(self == map_nodes_test_window && !sender);
    assert(msg == 0xa1 && !a && !b && !c && !d);
    ++skill_notifications;
    return 0;
}

static void observe_skill_list(const struct List *list)
{
    const struct ListNode *node;
    skill_observe(list->size);
    for (node = list->head->next; node != list->head; node = node->next) {
        const struct SkillInfo *info = LIST_VALUE(node, struct SkillInfo);
        const struct SkillRequirement *requirement;
        skill_observe(SKILL_FOUND(info));
        skill_observe(info->id);
        skill_observe(info->level);
        skill_observe(info->sp_cost);
        skill_observe(info->cell_index);
        skill_observe(vec_count(&info->requirements, sizeof *requirement));
        for (requirement = info->requirements.first; requirement != info->requirements.last;
             ++requirement) {
            skill_observe(requirement->skill_id);
            skill_observe(requirement->level);
        }
    }
}

static void skill_bundle_embedded(void)
{
    struct Session session = {0};
    struct SkillInfo info = {0};
    int *levels;
    session.mapType = -17;
    session.guildSkills.size = 0x11223344;
    session.homunSkills.size = 0x55667788;
    session.merSkills.size = 0x778899aa;
    session.hairStyle = 41;
    session.weapon = INT64_C(0x123456789abcdef);
    player_skill_bundle_construct(&session.skills);
    levels = mem_alloc(0x157c * sizeof *levels);
    memset(levels, 0, 0x157c * sizeof *levels);
    session.skills.chosen_levels.first = levels;
    session.skills.chosen_levels.last = session.skills.chosen_levels.end = levels + 0x157c;
    player_skill_bundle_set_chosen_level(&session.skills, 0, 99);
    player_skill_bundle_set_chosen_level(&session.skills, 1, 7);
    player_skill_bundle_set_chosen_level(&session.skills, 0x157b, 12);
    player_skill_bundle_set_chosen_level(&session.skills, 0x157c, 99);
    player_skill_bundle_set_chosen_level(&session.skills, -1, 99);
    assert(!levels[0] && levels[1] == 7 && levels[0x157b] == 12);
    skill_observe(levels[0]); skill_observe(levels[1]); skill_observe(levels[0x157b]);
    skill_info_construct(&info);
    info.id = 99; info.level = 2;
    player_skill_bundle_append_skill(&session.skills, &info);
    assert(player_skill_bundle_find_skill(&session.skills, 99)->level == 2);
    player_skill_bundle_destruct(&session.skills);
    skill_info_destruct(&info);
    assert(session.mapType == -17 && session.guildSkills.size == 0x11223344);
    assert(session.homunSkills.size == 0x55667788 && session.merSkills.size == 0x778899aa);
    assert(session.hairStyle == 41 && session.weapon == INT64_C(0x123456789abcdef));
    assert(!live_blocks);
    skill_observe(session.guildSkills.size); skill_observe(session.homunSkills.size);
    skill_observe(session.merSkills.size); skill_observe(session.hairStyle);
}

static void skill_bundle_values(void)
{
    struct UIWindowOps operations = {0};
    struct UIWindow window = {0};
    unsigned iteration;
    const char *trace_path = getenv("RAGEXE_SKILL_VALUES_TRACE");
    if (trace_path) {
        skill_trace = fopen(trace_path, "wb");
        assert(skill_trace);
    }
    operations.send_msg = skill_notify;
    window.ops = &operations;
    for (iteration = 0; iteration < 64; ++iteration) {
        struct PlayerSkillBundle bundle;
        struct SkillInfo info = {0}, copy = {0};
        struct SkillActionAnswer answer;
        struct SkillRequirement *requirement;
        int incoming;
        player_skill_bundle_construct(&bundle);
        skill_info_construct(&info);
        info.id = 42; info.level = 2; info.level2 = 1;
        info.id_name = "owned-requirements";
        info.sp_cost = 3; info.cell_index = 5;
        requirement = mem_alloc(sizeof *requirement);
        requirement->skill_id = 100; requirement->level = 1;
        info.requirements.first = requirement;
        info.requirements.last = info.requirements.end = requirement + 1;
        player_skill_bundle_append_skill(&bundle, &info);
        player_skill_bundle_add_tree_skill(&bundle, -1, &info);
        assert(player_skill_bundle_is_level_boosted(&bundle, 42));
        assert(!player_skill_bundle_is_level_boosted(&bundle, 43));
        assert(player_skill_bundle_put_autorun_skill(&bundle, &info) == 1);
        info.level = 7; info.sp_cost = 11;
        requirement->level = 99;
        assert(player_skill_bundle_put_autorun_skill(&bundle, &info) == 0);
        player_skill_bundle_copy_autorun_skill(&bundle, &copy, 42);
        assert(copy.level == 7 && copy.sp_cost == 11);
        assert(((struct SkillRequirement *)copy.requirements.first)->level == 1);
        skill_info_destruct(&copy);
        map_nodes_test_window = iteration & 1 ? &window : NULL;
        player_skill_bundle_add_extra_skill(&bundle, &info);
        info.id = 43;
        player_skill_bundle_add_extra_skill(&bundle, &info);
        player_skill_bundle_number_extra_skills(&bundle);
        assert(player_skill_bundle_find_extra_skill(&bundle, 42)->cell_index == 0);
        assert(player_skill_bundle_find_extra_skill(&bundle, 43)->cell_index == 1);
        observe_skill_list(&bundle.skills);
        observe_skill_list(&bundle.extra_skills);
        observe_skill_list(&bundle.autorun_skills);
        assert(!player_skill_bundle_remove_extra_skill(&bundle, 99));
        assert(player_skill_bundle_remove_extra_skill(&bundle, 42));
        assert(vec_count(&bundle.extra_skill_ids, sizeof(int)) == 1);
        assert(*(int *)bundle.extra_skill_ids.first == 43);
        player_skill_bundle_clear_extra_skills(&bundle);
        assert(!bundle.extra_skills.size);
        info.id = 42;
        player_skill_bundle_note_level_raise(&bundle, &info);
        assert(bundle.level_changes.size == 1 && bundle.level_change_flags.size == 1);
        player_skill_bundle_level_before_change(&bundle, &answer, 42);
        assert(answer.found && answer.value == 2);
        skill_observe(answer.found); skill_observe(answer.value);
        player_skill_bundle_flag_all_level_changes(&bundle);
        incoming = (int)(iteration % 4);
        info.level = incoming == 0 ? 2 : incoming == 1 ? 4 : incoming == 2 ? 7 : 8;
        player_skill_bundle_reconcile_level(&bundle, &info);
        player_skill_bundle_level_before_change(&bundle, &answer, 42);
        assert(answer.found == (incoming == 1 || incoming == 2));
        if (answer.found) assert(answer.value == (incoming == 1 ? 4 : 2));
        assert(info.level == (incoming == 0 ? 2 : incoming == 3 ? 8 : 7));
        skill_observe(info.level); skill_observe(answer.found); skill_observe(answer.value);
        skill_observe(bundle.level_changes.size); skill_observe(bundle.level_change_flags.size);
        player_skill_bundle_clear_level_changes(&bundle);
        player_skill_bundle_clear_level_changes(&bundle);
        player_skill_bundle_remove_skill(&bundle, 42);
        player_skill_bundle_remove_skill(&bundle, 42);
        assert(!bundle.skills.size);
        skill_table_find(&bundle, &copy, 42);
        assert(!SKILL_FOUND(&copy) && !strcmp(copy.id_name, "Unknown-Skill"));
        skill_info_destruct(&copy);
        skill_info_destruct(&info);
        player_skill_bundle_destruct(&bundle);
        assert(!live_blocks);
        skill_observe(live_blocks);
    }
    map_nodes_test_window = NULL;
    assert(map_nodes_window_queries == 192 && skill_notifications == 96);
    skill_observe(map_nodes_window_queries); skill_observe(skill_notifications);
    skill_bundle_embedded();
    if (skill_trace) assert(!fclose(skill_trace));
    skill_trace = NULL;
    printf("Skill values: bundle copies, pending raises, UI notifications and guarded cleanup passed; digest %08x.\n", skill_digest);
}

int main(void)
{
    shared_list_ownership();
    instant_values();
    ui_map_entries();
    integer_maps();
    skill_trees();
    skill_values();
    skill_bundle_values();
    puts("skill values: independent copies, capacity reuse/growth, empty/self assignment and cleanup passed");
    puts("map nodes: signed keys, 64-bit values, owned lists/vectors and skill-tree cleanup passed");
    return 0;
}
