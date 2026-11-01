#include "ui/character/skill_tree_internal.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
static unsigned live_allocations, invalidations, cases, failures;

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
    f->window.points_left = 12;
    f->tabs.names.first = f->names;
    f->tabs.names.last = f->tabs.names.end = f->names + count;
    f->tabs.colours.first = f->colours;
    f->tabs.colours.last = f->tabs.colours.end = f->colours + count;
    for (i = 0; i < SKILL_TREE_TABS + 1; ++i) f->colours[i] = 0x123456;
    for (i = 0; i < SKILL_TREE_TABS; ++i) list_init(&f->window.tab_skills[i]);
    list_init(&f->window.required);
    list_init(&f->window.planned);
    list_init(&g_session1.skills.extra_skills);
    invalidations = 0;
}

static void teardown(struct Fixture *f)
{
    int i;
    /* Test skill requirements borrow stack arrays; list nodes are real allocations. */
    for (i = 0; i < SKILL_TREE_TABS; ++i) list_free(&f->window.tab_skills[i], NULL);
    list_free(&f->window.required, NULL);
    list_free(&f->window.planned, NULL);
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
    assert(f.window.points_left == 12 && !f.window.pending && !f.window.planned.size);
    assert(!f.window.show_required && f.window.required_cell == -1 && invalidations >= (unsigned)count);
    teardown(&f);
}

static void stale_and_hidden_lists(void)
{
    struct Fixture f;
    struct SkillRequirement reqs[] = {{50000, 2}, {50000, 5}, {50001, 4}};
    struct SkillInfo *target;
    setup(&f, 3, 0);
    target = add_skill(displayed(&f, 0), 60000, 0, reqs, 3);
    add_skill(displayed(&f, 1), 50000, 28, NULL, 0);
    add_skill(displayed(&f, 1), 50000, 35, NULL, 0);
    /* These backing lists are not displayed: tab 2 is the extra list, tab 3 is absent. */
    add_skill(&f.window.tab_skills[2], 50001, 0, NULL, 0);
    add_skill(&f.window.tab_skills[3], 50001, 0, NULL, 0);
    skill_tree_mark_required(&f.window);
    expect_colours(&f, 2, "visible lists only; duplicate requirements");
    assert(f.window.required.size == 3);
    assert(skill_tree_level_list_find(&f.window.required, 50000)->level == 5);
    assert(skill_tree_level_list_find(&f.window.required, 50001)->level == 4);
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
    assert(f.window.points_left == 12 && !f.window.planned.size);
    ui_new_skill_list_wnd_on_r_btn_up(&f.window.frame.base, 0x50, 0x30);
    assert(!f.window.show_required);
    ui_new_skill_list_wnd_on_r_btn_up(&f.window.frame.base, 0x50, 0x30);
    assert(f.window.show_required);
    ui_new_skill_list_wnd_on_r_btn_up(&f.window.frame.base, 0, 0);
    assert(!f.window.show_required && !f.window.required.size);
    expect_colours(&f, 0, "right-click outside clears pin and hints");
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
    printf("Skill prerequisite tab hints: %u checks, %u failures; no outstanding allocations.\n", cases, failures);
    return failures ? 1 : 0;
}
