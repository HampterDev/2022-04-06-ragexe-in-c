#include "core/thread_text.h"
#include "skills/skill_info.h"
#include "world/mode_manager.h"
#include "ui/character/skill_tree_internal.h"
#include "core/msg_str.h"

#define MODE_REQ_SKILL_UP 0x43
#define MSG_CANNOT_RAISE 0xdcb

int skill_tree_planned_level(struct UINewSkillListWnd *self, int skill_id)
{
    struct SkillRequirement *planned = skill_tree_level_list_find(&self->planned, skill_id);

    return planned ? planned->level : 0;
}

void skill_tree_reset_points(struct UINewSkillListWnd *self)
{
    unsigned i;

    self->pending = 0;
    self->points_left = g_session1.skillPoints;
    for (i = 0; i < SKILL_TREE_TABS; i++) {
        struct List *src = player_skill_bundle_tree_tab(&g_session1.skills, i);
        struct ListNode *node;

        if (&self->tab_skills[i] == src)
            continue;
        list_clear(&self->tab_skills[i], skill_info_destroy);
        for (node = src->head->next; node != src->head; node = node->next) {
            struct SkillInfo copy;

            skill_info_copy(&copy, LIST_VALUE(node, struct SkillInfo));
            list_push_back(&self->tab_skills[i], &copy, sizeof copy);
        }
    }
    list_clear(&self->planned, NULL);
    player_skill_bundle_flag_all_level_changes(&g_session1.skills);
}

BYTE skill_tree_raise_skill(struct UINewSkillListWnd *self, int skill_id)
{
    struct ListNode *it;

    if (self->points_left <= 0 || self->show_required)
        return 0;
    if (g_session1.simpleSkillWnd) {
        struct List *list = skill_tree_shown_list(self);
        struct ListNode *node;

        for (node = list->head->next; node != list->head; node = node->next) {
            struct SkillInfo *skill = LIST_VALUE(node, struct SkillInfo);

            if (skill->id == skill_id) {
                if (skill->level >= SKILL_INFO_MAX_LEVEL(skill))
                    return 0;
                skill->level++;
                self->points_left--;
                return 1;
            }
        }
        return 0;
    }
    for (it = self->required.head->next; it != self->required.head; it = it->next) {
        const struct SkillRequirement *req = LIST_VALUE(it, struct SkillRequirement);
        int id, target, base, have;
        struct SkillInfo *info;

        if (self->points_left <= 0)
            return 1;
        id = req->skill_id;
        info = player_skill_bundle_find_tree_skill(&g_session1.skills, id);
        if (!info || !info->found)
            continue;
        if (id == skill_id) {
            base = SKILL_INFO_BASE_LEVEL(info);
            target = (base > skill_tree_planned_level(self, id) ? base : skill_tree_planned_level(self, id)) + 1;
            if (SKILL_INFO_BASE_LEVEL(info) >= SKILL_INFO_MAX_LEVEL(info))
                return 0;
        } else {
            target = req->level;
        }
        if (target > SKILL_INFO_MAX_LEVEL(info))
            continue;
        base = SKILL_INFO_BASE_LEVEL(info);
        have = base > skill_tree_planned_level(self, id) ? base : skill_tree_planned_level(self, id);
        if (have >= target)
            continue;
        if (SKILL_INFO_LEVEL_LIMIT(info) <= 0) {
            ui_window_mgr_send_msg_(&g_windowMgr, 1,
                                (int)thread_text_format(msg_str(MSG_CANNOT_RAISE), skill_info_get_name(info)),
                                0xff, 0, 0);
            return 0;
        }
        {
            int n = target - have < self->points_left ? target - have : self->points_left;
            struct SkillRequirement *planned;
            struct SkillRequirement value;

            self->points_left -= n;
            planned = skill_tree_level_list_find(&self->planned, id);
            if (planned) {
                planned->level = n + have;
            } else {
                value.skill_id = id;
                value.level = n + have;
                list_push_back(&self->planned, &value, sizeof value);
            }
        }
    }
    return 1;
}

void skill_tree_apply_points(struct UINewSkillListWnd *self)
{
    if (!g_session1.simpleSkillWnd) {
        struct ListNode *node;

        for (node = self->planned.head->next; node != self->planned.head; node = node->next) {
            int id = LIST_VALUE(node, struct SkillRequirement)->skill_id;
            int level = LIST_VALUE(node, struct SkillRequirement)->level;
            struct SkillInfo *info = player_skill_bundle_find_tree_skill(&g_session1.skills, id);
            int n;

            if (!info || !info->found || SKILL_INFO_BASE_LEVEL(info) + 1 > level)
                continue;
            for (n = level - SKILL_INFO_BASE_LEVEL(info); n; n--)
                mode_send_message(client_current_mode(), MODE_REQ_SKILL_UP, id, 0, 0, 0);
        }
        list_clear(&self->planned, NULL);
    } else {
        struct List *ours, *theirs;
        struct ListNode *mine, *session;

        theirs = skill_tree_tab_count(self) - 1 == self->tab ? &g_session1.skills.extra_skills : player_skill_bundle_tree_tab(&g_session1.skills, self->tab);
        ours = skill_tree_shown_list(self);
        session = theirs->head->next;
        for (mine = ours->head->next; mine != ours->head; mine = mine->next, session = session->next) {
            const struct SkillInfo *want = LIST_VALUE(mine, struct SkillInfo);
            const struct SkillInfo *have = LIST_VALUE(session, struct SkillInfo);
            int i;

            if (!have->found || have->id != want->id || have->level >= want->level)
                continue;
            for (i = 0; i < want->level - have->level; i++)
                mode_send_message(client_current_mode(), MODE_REQ_SKILL_UP, have->id, 0, 0, 0);
        }
    }
    self->pending = 0;
    skill_tree_layout_buttons(self);
}

static void mark_required_tabs(struct UINewSkillListWnd *self, int skill_id)
{
    int tab, count = skill_tree_tab_count(self);

    for (tab = 0; tab < count; tab++) {
        const struct List *list;
        const struct ListNode *node;

        if (tab == self->tab)
            continue;
        list = tab == count - 1 ? &g_session1.skills.extra_skills : &self->tab_skills[tab];
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
    list = skill_tree_tab_count(self) - 1 == self->tab ? &g_session1.skills.extra_skills : &self->tab_skills[self->tab];
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
        struct SkillRequirement *req = &((struct SkillRequirement *)reqs->first)[k];
        int id = req->skill_id, level;
        struct SkillRequirement *have;

        mark_required_tabs(self, id);
        level = req->level;
        have = skill_tree_level_list_find(&self->required, id);
        if (have) {
            int had = have->level;

            list_erase(&self->required, LIST_NODE_OF(have), NULL);
            level = level > had ? level : had;
        }
        req->level = level;
        list_push_back(&self->required, req, sizeof *req);
    }
}
