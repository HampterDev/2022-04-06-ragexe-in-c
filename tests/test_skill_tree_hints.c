#include "ui/character/skill_tree_internal.h"
#include "world/mode_manager.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
static unsigned live_allocations, invalidations, cases, failures;

/* Session tree tabs: the window shows these lists directly. */
static struct List session_tabs[SKILL_TREE_TABS], no_tab;
static int mode_token, sent_count, sent_message, sent_skill;

void *mem_alloc(size_t size)
{
    void *p = malloc(size);
    assert(p);
    ++live_allocations;
    return p;
}

void mem_free(void *p)
{
    if (p) {
        assert(live_allocations);
        --live_allocations;
        free(p);
    }
}

void ui_window_invalidate_base(struct UIWindow *window)
{
    assert(window);
    ++invalidations;
}

struct List *player_skill_bundle_tree_tab(struct PlayerSkillBundle *self, int key)
{
    assert(self == &g_session1.skills);
    return key >= 0 && key < SKILL_TREE_TABS ? &session_tabs[key] : &no_tab;
}

int player_skill_bundle_tree_skill_level(struct PlayerSkillBundle *self, int skill_id)
{
    int i;
    assert(self == &g_session1.skills);
    for (i = 0; i < SKILL_TREE_TABS; ++i) {
        const struct ListNode *node;
        for (node = session_tabs[i].head->next; node != session_tabs[i].head; node = node->next)
            if (LIST_VALUE(node, struct SkillInfo)->id == skill_id)
                return LIST_VALUE(node, struct SkillInfo)->level;
    }
    return 0;
}

struct Mode *client_current_mode(void)
{
    return (struct Mode *)&mode_token;
}

int mode_send_message(struct Mode *mode, int message, int a, int b, int c, int d)
{
    assert(mode == (struct Mode *)&mode_token && !b && !c && !d);
    ++sent_count;
    sent_message = message;
    sent_skill = a;
    return 0;
}

#include "skill_tree_hints_helpers.inc"

struct Fixture {
    struct UINewSkillListWnd window;
    struct UITabControl tabs;
    struct String names[SKILL_TREE_TABS + 1];
    int colours[SKILL_TREE_TABS + 1];
};

static void setup(struct Fixture *f, int count, int selected)
{
    int i;
    assert(!live_allocations && count > 0 && count <= SKILL_TREE_TABS + 1);
    memset(f, 0, sizeof *f);
    memset(&g_session1, 0, sizeof g_session1);
    f->window.tabs = &f->tabs;
    f->window.tab = selected;
    f->window.columns = 7;
    f->window.rows = 6;
    f->window.top_margin = 0x14;
    f->window.hover = 0;
    f->window.required_cell = -1;
    g_session1.skillPoints = 12;
    f->tabs.names.first = f->names;
    f->tabs.names.last = f->tabs.names.end = f->names + count;
    f->tabs.colours.first = f->colours;
    f->tabs.colours.last = f->tabs.colours.end = f->colours + count;
    for (i = 0; i < SKILL_TREE_TABS + 1; ++i) f->colours[i] = 0x123456;
    for (i = 0; i < SKILL_TREE_TABS; ++i) list_init(&session_tabs[i]);
    list_init(&no_tab);
    list_init(&f->window.required);
    list_init(&g_session1.skills.extra_skills);
    invalidations = 0;
    sent_count = sent_message = sent_skill = 0;
}

static void teardown(struct Fixture *f)
{
    int i;
    /* Test skill requirements borrow stack arrays; list nodes are real allocations. */
    for (i = 0; i < SKILL_TREE_TABS; ++i) list_free(&session_tabs[i], NULL);
    list_free(&no_tab, NULL);
    list_free(&f->window.required, NULL);
    list_free(&g_session1.skills.extra_skills, NULL);
    assert(!live_allocations);
}

static struct List *displayed(struct Fixture *f, int tab)
{
    int selected = f->window.tab;
    struct List *list;
    f->window.tab = tab;
    list = skill_tree_shown_list(&f->window);
    f->window.tab = selected;
    return list;
}

static struct SkillInfo *add_skill(struct List *list, int id, int cell,
                                 struct SkillRequirement *requirements, size_t count)
{
    struct SkillInfo info = {0};
    info.id = id;
    info.found = 1;
    info.cell_index = cell;
    info.requirements.first = requirements;
    info.requirements.last = info.requirements.end = count ? requirements + count : requirements;
    return list_push_back(list, &info, sizeof info);
}

static void expect_colours(struct Fixture *f, unsigned mask, const char *label)
{
    int i, count = skill_tree_tab_count(&f->window);
    ++cases;
    for (i = 0; i < count; ++i) {
        int expected = mask & (1u << i) ? 0xe1e7fc : -1;
        if (f->colours[i] != expected) {
            printf("FAIL %s: tab %d colour %x, expected %x\n", label, i, f->colours[i], expected);
            ++failures;
        }
    }
    for (; i < SKILL_TREE_TABS + 1; ++i) assert(f->colours[i] == 0x123456);
}

static void expect_sent(int count, int skill_id, const char *label)
{
    ++cases;
    if (sent_count != count || (count && (sent_message != 0x43 || sent_skill != skill_id))) {
        printf("FAIL %s: %d requests (message %x, skill %d), expected %d for skill %d\n", label, sent_count,
               sent_message, sent_skill, count, skill_id);
        ++failures;
    }
}

static void placement(const char *label, int count, int selected, int prerequisite,
                      unsigned locations, unsigned expected)
{
    struct Fixture f;
    struct SkillRequirement req = { prerequisite, 3 };
    struct SkillRequirement *actual;
    int i;
    setup(&f, count, selected);
    add_skill(displayed(&f, selected), 60000, 0, &req, 1);
    for (i = 0; i < count; ++i)
        if (locations & (1u << i)) add_skill(displayed(&f, i), prerequisite, 7, NULL, 0);
    skill_tree_mark_required(&f.window);
    expect_colours(&f, expected, label);
    assert(f.window.required.size == 2);
    actual = skill_tree_level_list_find(&f.window.required, prerequisite);
    assert(actual && actual->level == 3);
    assert(g_session1.skillPoints == 12 && !sent_count && req.level == 3);
    assert(!f.window.show_required && f.window.required_cell == -1 && invalidations >= (unsigned)count);
    teardown(&f);
}

static void stale_and_hidden_lists(void)
{
    struct Fixture f;
    struct SkillRequirement reqs[] = {{50000, 5}, {50000, 2}, {50001, 4}};
    struct SkillInfo *target;
    setup(&f, 3, 0);
    target = add_skill(displayed(&f, 0), 60000, 0, reqs, 3);
    add_skill(displayed(&f, 1), 50000, 28, NULL, 0);
    add_skill(displayed(&f, 1), 50000, 35, NULL, 0);
    /* These session tabs are not displayed: tab 2 is the extra list, tab 3 is absent. */
    add_skill(&session_tabs[2], 50001, 0, NULL, 0);
    add_skill(&session_tabs[3], 50001, 0, NULL, 0);
    skill_tree_mark_required(&f.window);
    expect_colours(&f, 2, "visible lists only; duplicate requirements");
    assert(f.window.required.size == 3);
    assert(skill_tree_level_list_find(&f.window.required, 50000)->level == 5);
    assert(skill_tree_level_list_find(&f.window.required, 50001)->level == 4);
    /* The merged level stays in the window; session requirements are not rewritten. */
    assert(reqs[0].level == 5 && reqs[1].level == 2 && reqs[2].level == 4);
    target->requirements.last = target->requirements.first;
    skill_tree_mark_required(&f.window);
    expect_colours(&f, 0, "new target has no prerequisites");
    assert(f.window.required.size == 1);
    f.window.hover = -1;
    skill_tree_mark_required(&f.window);
    expect_colours(&f, 0, "invalid hover clears stale hints");
    assert(!f.window.required.size);
    teardown(&f);
}

static void pinning(void)
{
    struct Fixture f;
    struct SkillRequirement req = {50000, 3};
    setup(&f, 5, 0);
    add_skill(displayed(&f, 0), 60000, 0, &req, 1);
    add_skill(displayed(&f, 3), 50000, 14, NULL, 0);
    ui_new_skill_list_wnd_on_r_btn_up(&f.window.frame.base, 0x50, 0x30);
    assert(f.window.show_required && f.window.required_cell == 0);
    expect_colours(&f, 8, "right-click pin uses actual prerequisite tab");
    assert(!skill_tree_raise_skill(&f.window, 60000));
    expect_sent(0, 0, "pinned prerequisites block the raise");
    ui_new_skill_list_wnd_on_r_btn_up(&f.window.frame.base, 0x50, 0x30);
    assert(!f.window.show_required);
    ui_new_skill_list_wnd_on_r_btn_up(&f.window.frame.base, 0x50, 0x30);
    assert(f.window.show_required);
    ui_new_skill_list_wnd_on_r_btn_up(&f.window.frame.base, 0, 0);
    assert(!f.window.show_required && !f.window.required.size);
    expect_colours(&f, 0, "right-click outside clears pin and hints");
    teardown(&f);
}

static struct SkillInfo *raisable(struct List *list, int id, int level, int max_level,
                                  struct SkillRequirement *requirements, size_t count)
{
    struct SkillInfo *skill = add_skill(list, id, (int)list->size, requirements, count);
    skill->level = (short)level;
    skill->max_level = max_level;
    return skill;
}

static void raising(void)
{
    struct Fixture f;
    struct SkillRequirement reqs[] = {{100, 3}, {101, 1}};
    struct SkillInfo *target, *first, *second;
    setup(&f, 3, 1);
    first = raisable(&session_tabs[0], 100, 3, 10, NULL, 0);
    second = raisable(&session_tabs[0], 101, 1, 5, NULL, 0);
    target = raisable(&session_tabs[1], 200, 0, 5, reqs, 2);
    raisable(&session_tabs[1], 201, 5, 5, NULL, 0);

    assert(skill_tree_raise_skill(&f.window, 200));
    expect_sent(1, 200, "prerequisites met: one immediate request");
    assert(target->level == 0 && g_session1.skillPoints == 12 && !f.window.required.size);
    assert(skill_tree_raise_skill(&f.window, 200));
    expect_sent(2, 200, "each activation sends one request; levels wait for the server");

    sent_count = 0;
    first->level = 2;
    assert(!skill_tree_raise_skill(&f.window, 200));
    expect_sent(0, 0, "prerequisite one level short: nothing sent, no prerequisite raise");
    first->level = 3;
    second->id = 102;
    assert(!skill_tree_raise_skill(&f.window, 200));
    expect_sent(0, 0, "prerequisite missing from the session");
    second->id = 101;

    assert(!skill_tree_raise_skill(&f.window, 201));
    expect_sent(0, 0, "maximum level");
    assert(!skill_tree_raise_skill(&f.window, 100));
    expect_sent(0, 0, "skill not on the displayed tab");
    target->found = 0;
    assert(!skill_tree_raise_skill(&f.window, 200));
    expect_sent(0, 0, "unknown skill");
    target->found = 1;

    g_session1.skillPoints = 0;
    assert(!skill_tree_raise_skill(&f.window, 200));
    expect_sent(0, 0, "no skill points");
    g_session1.skillPoints = 1;
    f.window.show_required = 1;
    assert(!skill_tree_raise_skill(&f.window, 200));
    expect_sent(0, 0, "pinned prerequisites");
    f.window.show_required = 0;

    f.window.tab = 2;
    raisable(&g_session1.skills.extra_skills, 300, 0, 1, NULL, 0);
    assert(skill_tree_raise_skill(&f.window, 300));
    expect_sent(1, 300, "extra tab skill");
    assert(g_session1.skills.extra_skills.size == 1);
    teardown(&f);
}

int main(void)
{
    placement("custom prerequisite moved to first tab", 4, 2, 50000, 1, 1);
    placement("low-ID prerequisite moved to fourth class tab", 5, 1, 5, 8, 8);
    placement("third-class ID moved to second tab", 4, 0, 2030, 2, 2);
    placement("custom prerequisite on extra tab", 5, 0, 50000, 16, 16);
    placement("every other matching tab", 5, 1, 50000, 31, 29);
    placement("absent skill colours no tab", 4, 3, 50000, 0, 0);
    placement("same-tab prerequisite colours no tab", 4, 2, 50000, 4, 0);
    placement("target in extra tab", 4, 3, 50000, 1, 1);
    placement("only extra tab exists", 1, 0, 50000, 1, 0);
    placement("two tabs; first contains custom prerequisite", 2, 1, 50000, 1, 1);
    placement("full integer ID lookup", 3, 1, INT_MAX, 5, 5);
    placement("existing first-class layout", 4, 1, 5, 1, 1);
    stale_and_hidden_lists();
    pinning();
    raising();
    printf("Skill prerequisite tab hints and immediate raises: %u checks, %u failures; no outstanding allocations.\n",
           cases, failures);
    return failures ? 1 : 0;
}
