#include "ui/window_types.h"
#include "std/list.h"
#include "std/std.h"
#include "skills/skill_info.h"
#include "ui/window_access.h"
#include "core/fatal.h"
#include <stdlib.h>
#include "skills/skill_bundle.h"
#include "skills/skill_action.h"
#include "contents/ui_window_mgr.h"

struct List g_skillBundleEmptyList;
struct Once g_skillBundleEmptyListGuard;
static void skill_bundle_empty_list_destroy(void);

struct TreeTab {
    int job;
    struct List skills;
};

struct LevelChange {
    int skill_id;
    struct SkillLevelChange info;
};

struct LevelFlag {
    int skill_id;
    unsigned char flag;
};

#define SKILL_ID_LAST 0x157b
#define SKILL_WINDOW_ID 0x25

static void tree_tab_destroy(void *value)
{
    struct TreeTab *tab = value;

    list_free(&tab->skills, skill_info_destroy);
}

static void skill_list_push_back(struct List *list, const struct SkillInfo *info)
{
    struct SkillInfo copy;

    skill_info_copy(&copy, info);
    list_push_back(list, &copy, sizeof copy);
}

static struct ListNode *skill_list_find(const struct List *list, int skill_id)
{
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next)
        if (skill_id == LIST_VALUE(node, struct SkillInfo)->id)
            break;
    return node;
}

static void level_change_erase(struct PlayerSkillBundle *self, int key)
{
    if (!tree_erase_key(&self->level_changes, &key, tree_less_int, NULL))
        return;
    tree_erase_key(&self->level_change_flags, &key, tree_less_int, NULL);
}

static void level_change_drop_marked(struct PlayerSkillBundle *self)
{
    int *id;

    for (id = (int *)self->level_change_drops.first; id != (int *)self->level_change_drops.last; id++)
        level_change_erase(self, *id);
    self->level_change_drops.last = self->level_change_drops.first;
}

static void skill_window_notify(void)
{
    void *window = ui_window_find(SKILL_WINDOW_ID);

    if (window)
        ui_window_send_msg(window, NULL, 0xa1, 0, 0, 0, 0);
}

struct PlayerSkillBundle *player_skill_bundle_construct(struct PlayerSkillBundle *self)
{
    list_init(&self->skills);
    tree_init(&self->tree_tabs);
    list_init(&self->extra_skills);
    self->extra_skill_ids.first = self->extra_skill_ids.last = self->extra_skill_ids.end = NULL;
    tree_init(&self->level_changes);
    tree_init(&self->level_change_flags);
    self->level_change_drops.first = self->level_change_drops.last = self->level_change_drops.end = NULL;
    self->chosen_levels.first = self->chosen_levels.last = self->chosen_levels.end = NULL;
    list_init(&self->autorun_skills);
    return self;
}

void player_skill_bundle_destruct(struct PlayerSkillBundle *self)
{
    list_free(&self->autorun_skills, skill_info_destroy);
    vec_free(&self->chosen_levels);
    vec_free(&self->level_change_drops);
    tree_free(&self->level_change_flags, NULL);
    tree_free(&self->level_changes, NULL);
    vec_free(&self->extra_skill_ids);
    list_free(&self->extra_skills, skill_info_destroy);
    tree_free(&self->tree_tabs, tree_tab_destroy);
    list_free(&self->skills, skill_info_destroy);
}

unsigned char player_skill_bundle_put_autorun_skill(struct PlayerSkillBundle *self, const struct SkillInfo *info)
{
    struct ListNode *node = skill_list_find(&self->autorun_skills, info->id);
    struct SkillInfo *skill;

    if (node == self->autorun_skills.head) {
        skill_list_push_back(&self->autorun_skills, info);
        return 1;
    }
    skill = LIST_VALUE(node, struct SkillInfo);
    skill->level = info->level;
    skill->id = info->id;
    skill->sp_cost = info->sp_cost;
    skill->upgradable = info->upgradable;
    skill->ap_amount = info->ap_amount;
    return 0;
}

void player_skill_bundle_note_level_raise(struct PlayerSkillBundle *self, const struct SkillInfo *info)
{
    struct SkillInfo *known;
    struct LevelChange *change;
    struct LevelFlag *flag;
    int before, after;

    if (!info->level)
        return;
    known = player_skill_bundle_find_tree_skill(self, info->id);
    if (!known)
        return;
    before = known->level;
    after = info->level;
    if (before >= after)
        return;
    change = tree_get(&self->level_changes, &known->id, sizeof known->id, sizeof *change, tree_less_int);
    change->info.level_before = before;
    change->info.level_after = after;
    flag = tree_get(&self->level_change_flags, &known->id, sizeof known->id, sizeof *flag, tree_less_int);
    flag->flag = 1;
}

void player_skill_bundle_clear_level_changes(struct PlayerSkillBundle *self)
{
    tree_clear(&self->level_changes, NULL);
    tree_clear(&self->level_change_flags, NULL);
    self->level_change_drops.last = self->level_change_drops.first;
}

void player_skill_bundle_append_skill(struct PlayerSkillBundle *self,
                                               const struct SkillInfo *info)
{
    skill_list_push_back(&self->skills, info);
}

void player_skill_bundle_remove_skill(struct PlayerSkillBundle *self, int skill_id)
{
    struct ListNode *node = skill_list_find(&self->skills, skill_id);

    if (node != self->skills.head)
        list_erase(&self->skills, node, skill_info_destroy);
}

struct SkillActionAnswer *player_skill_bundle_level_before_change(struct PlayerSkillBundle *self,
                                                struct SkillActionAnswer *out, int skill_id)
{
    struct LevelChange *change = tree_find(&self->level_changes, &skill_id, tree_less_int);

    if (change) {
        out->found = 1;
        out->value = change->info.level_before;
    } else {
        out->found = 0;
        out->value = 0;
    }
    return out;
}

void player_skill_bundle_add_extra_skill(struct PlayerSkillBundle *self, const struct SkillInfo *info)
{
    vec_push(&self->extra_skill_ids, &info->id, sizeof info->id);
    skill_window_notify();
    skill_list_push_back(&self->extra_skills, info);
}

void player_skill_bundle_clear_extra_skills(struct PlayerSkillBundle *self)
{
    list_clear(&self->extra_skills, skill_info_destroy);
    self->extra_skill_ids.last = self->extra_skill_ids.first;
}

unsigned char player_skill_bundle_remove_extra_skill(struct PlayerSkillBundle *self, int skill_id)
{
    struct ListNode *node = skill_list_find(&self->extra_skills, skill_id);
    int *id, *last;

    if (node == self->extra_skills.head)
        return 0;
    list_erase(&self->extra_skills, node, skill_info_destroy);
    last = (int *)self->extra_skill_ids.last;
    for (id = (int *)self->extra_skill_ids.first; id != last; id++)
        if (*id == skill_id)
            break;
    if (id != last) {
        memmove(id, id + 1, (char *)last - (char *)(id + 1));
        self->extra_skill_ids.last = (int *)self->extra_skill_ids.last - 1;
        skill_window_notify();
    }
    return 1;
}

struct SkillInfo *player_skill_bundle_find_extra_skill(struct PlayerSkillBundle *self, int skill_id)
{
    struct ListNode *node = skill_list_find(&self->extra_skills, skill_id);

    return node == self->extra_skills.head ? NULL : LIST_VALUE(node, struct SkillInfo);
}

struct SkillInfo *player_skill_bundle_find_skill(struct PlayerSkillBundle *self, int skill_id)
{
    struct ListNode *node = skill_list_find(&self->skills, skill_id);

    return node == self->skills.head ? NULL : LIST_VALUE(node, struct SkillInfo);
}

void player_skill_bundle_number_extra_skills(struct PlayerSkillBundle *self)
{
    struct ListNode *node;
    int index = 0;

    for (node = self->extra_skills.head->next; node != self->extra_skills.head; node = node->next)
        LIST_VALUE(node, struct SkillInfo)->cell_index = index++;
}

static struct SkillInfo *skill_list_copy_out(const struct List *list, struct SkillInfo *out,
                                             struct ListNode *node)
{
    if (node == list->head)
        return skill_info_construct_unknown(out);
    return skill_info_copy(out, LIST_VALUE(node, struct SkillInfo));
}

struct SkillInfo *skill_table_find(struct PlayerSkillBundle *self,
                                             struct SkillInfo *out, int skill_id)
{
    if (!self->skills.size)
        return skill_info_construct_unknown(out);
    return skill_list_copy_out(&self->skills, out, skill_list_find(&self->skills, skill_id));
}

struct SkillInfo *player_skill_bundle_copy_autorun_skill(struct PlayerSkillBundle *self, struct SkillInfo *out,
                                        int skill_id)
{
    if (!self->autorun_skills.size)
        return skill_info_construct_unknown(out);
    return skill_list_copy_out(&self->autorun_skills, out, skill_list_find(&self->autorun_skills, skill_id));
}

void player_skill_bundle_flag_all_level_changes(struct PlayerSkillBundle *self)
{
    struct TreeNode *node;

    for (node = tree_first(&self->level_change_flags); node != self->level_change_flags.head; node = tree_next(node))
        TREE_VALUE(node, struct LevelFlag)->flag = 1;
}

unsigned char player_skill_bundle_is_level_boosted(struct PlayerSkillBundle *self, int skill_id)
{
    struct SkillInfo *skill = player_skill_bundle_find_tree_skill(self, skill_id);

    if (!skill)
        return 0;
    return skill->level > (short)skill->level2;
}

void player_skill_bundle_zero_first_level_before(struct PlayerSkillBundle *self)
{
    struct TreeNode *first = tree_first(&self->level_changes);

    if (first != self->level_changes.head)
        TREE_VALUE(first, struct LevelChange)->info.level_before = 0;
}

void player_skill_bundle_add_tree_skill(struct PlayerSkillBundle *self, int key,
                                     const struct SkillInfo *info)
{
    struct TreeTab *tab = tree_find(&self->tree_tabs, &key, tree_less_int);

    if (!tab) {
        struct TreeTab made;

        made.job = key;
        list_init(&made.skills);
        tab = tree_add(&self->tree_tabs, &made, sizeof made, tree_less_int, NULL);
    }
    skill_list_push_back(&tab->skills, info);
}

void player_skill_bundle_clear_trees(struct PlayerSkillBundle *self)
{
    tree_clear(&self->tree_tabs, tree_tab_destroy);
}

struct SkillInfo *player_skill_bundle_find_tree_skill(struct PlayerSkillBundle *self, int skill_id)
{
    struct TreeNode *lists;

    for (lists = tree_first(&self->tree_tabs); lists != self->tree_tabs.head; lists = tree_next(lists)) {
        struct List *skills = &TREE_VALUE(lists, struct TreeTab)->skills;
        struct ListNode *node = skill_list_find(skills, skill_id);

        if (node != skills->head)
            return LIST_VALUE(node, struct SkillInfo);
    }
    return NULL;
}

struct List *player_skill_bundle_tree_tab(struct PlayerSkillBundle *self, int key)
{
    struct TreeTab *tab;

    if (once_begin(&g_skillBundleEmptyListGuard)) {
        list_init(&g_skillBundleEmptyList);
        atexit(skill_bundle_empty_list_destroy);
        once_complete(&g_skillBundleEmptyListGuard);
    }
    tab = tree_find(&self->tree_tabs, &key, tree_less_int);
    if (tab)
        return &tab->skills;
    return &g_skillBundleEmptyList;
}

unsigned player_skill_bundle_tree_tab_size(struct PlayerSkillBundle *self, int key)
{
    struct TreeTab *tab = tree_find(&self->tree_tabs, &key, tree_less_int);

    if (tab)
        return tab->skills.size;
    return 0;
}

int player_skill_bundle_tree_skill_level(struct PlayerSkillBundle *self, int skill_id)
{
    struct SkillInfo *skill = player_skill_bundle_find_tree_skill(self, skill_id);

    return skill ? skill->level : 0;
}

void player_skill_bundle_set_chosen_level(struct PlayerSkillBundle *self, int skill_id, int value)
{
    if ((unsigned)(skill_id - 1) <= SKILL_ID_LAST - 1)
        ((int *)self->chosen_levels.first)[skill_id] = value;
}

void player_skill_bundle_reconcile_level(struct PlayerSkillBundle *self, struct SkillInfo *info)
{
    struct LevelChange *change = tree_find(&self->level_changes, &info->id, tree_less_int);

    if (change) {
        struct SkillLevelChange *pair = &change->info;
        int level = info->level, after = pair->level_after;

        if (level == pair->level_before || after < level) {
            vec_push(&self->level_change_drops, &info->id, sizeof info->id);
        } else if (pair->level_before < level && level < after) {
            pair->level_before = level;
            info->level = after;
        }
    }
    level_change_drop_marked(self);
}

static void skill_bundle_empty_list_destroy(void)
{
    list_free(&g_skillBundleEmptyList, skill_info_destroy);
}
