#include "skills/skill_info.h"
#include "world/mode_manager.h"
#include "ui/character/skill_tree_internal.h"

#define MODE_REQ_SKILL_UP 0x43

static BYTE prerequisites_met(struct SkillInfo *skill)
{
    const struct Vec *reqs = SKILL_INFO_REQUIREMENTS(skill);
    unsigned k;

    for (k = 0; k < VEC_COUNT(reqs, struct SkillRequirement); k++) {
        const struct SkillRequirement *req = &((const struct SkillRequirement *)reqs->first)[k];

        if (player_skill_bundle_tree_skill_level(&g_session1.skills, req->skill_id) < req->level)
            return 0;
    }
    return 1;
}

/* Sends one ordinary upgrade request when the skill can be raised now: skill points
 * remain, it is below its maximum level and the session already has every prerequisite
 * level. Pinned prerequisites (right-click) block the raise. */
BYTE skill_tree_raise_skill(struct UINewSkillListWnd *self, int skill_id)
{
    struct List *list;
    struct ListNode *node;

    if (g_session1.skillPoints <= 0 || self->show_required)
        return 0;
    list = skill_tree_shown_list(self);
    for (node = list->head->next; node != list->head; node = node->next) {
        struct SkillInfo *skill = LIST_VALUE(node, struct SkillInfo);

        if (skill->id != skill_id)
            continue;
        if (!skill->found || skill->level >= SKILL_INFO_MAX_LEVEL(skill) || !prerequisites_met(skill))
            return 0;
        mode_send_message(client_current_mode(), MODE_REQ_SKILL_UP, skill_id, 0, 0, 0);
        return 1;
    }
    return 0;
}

static void mark_required_tabs(struct UINewSkillListWnd *self, int skill_id)
{
    int tab, count = skill_tree_tab_count(self);

    for (tab = 0; tab < count; tab++) {
        const struct List *list;
        const struct ListNode *node;

        if (tab == self->tab)
            continue;
        /* Match the displayed lists: the final tab contains extra skills. */
        list = tab == count - 1 ? &g_session1.skills.extra_skills : player_skill_bundle_tree_tab(&g_session1.skills, tab);
        for (node = list->head->next; node != list->head; node = node->next) {
            if (LIST_VALUE(node, struct SkillInfo)->id == skill_id) {
                ((int *)self->tabs->colours.first)[tab] = 0xe1e7fc;
                ui_window_invalidate_base(&self->tabs->base);
                break;
            }
        }
    }
}

void skill_tree_mark_required(struct UINewSkillListWnd *self)
{
    struct UITabControl *tabs;
    struct List *list;
    struct ListNode *node;
    struct SkillRequirement value;
    struct Vec *reqs;
    unsigned k;
    int i;

    list_clear(&self->required, NULL);
    tabs = self->tabs;
    for (i = 0; i < skill_tree_tab_count(self); i++) {
        ((int *)tabs->colours.first)[i] = -1;
        ui_window_invalidate_base(&tabs->base);
        tabs = self->tabs;
    }
    list = skill_tree_shown_list(self);
    for (node = list->head->next; node != list->head; node = node->next) {
        int slot = SKILL_INFO_SLOT(LIST_VALUE(node, struct SkillInfo));

        if (slot != -1 && slot >= self->top * self->columns && slot == self->hover)
            break;
    }
    if (node == list->head)
        return;
    value.skill_id = LIST_VALUE(node, struct SkillInfo)->id;
    value.level = 0;
    list_push_back(&self->required, &value, sizeof value);
    reqs = SKILL_INFO_REQUIREMENTS(LIST_VALUE(node, struct SkillInfo));
    for (k = 0; k < VEC_COUNT(reqs, struct SkillRequirement); k++) {
        const struct SkillRequirement *req = &((const struct SkillRequirement *)reqs->first)[k];
        struct SkillRequirement *have;

        mark_required_tabs(self, req->skill_id);
        value = *req;
        have = skill_tree_level_list_find(&self->required, value.skill_id);
        if (have) {
            int had = have->level;

            list_erase(&self->required, LIST_NODE_OF(have), NULL);
            value.level = value.level > had ? value.level : had;
        }
        /* The skill lists are session state; the window keeps the merged level itself. */
        list_push_back(&self->required, &value, sizeof value);
    }
}
