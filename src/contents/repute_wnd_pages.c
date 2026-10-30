#include "ui/controls/edit.h"
#include "ui/controls/buttons.h"
#include "ui/window_manager_state.h"
#include <string.h>
#include "contents/repute_wnd.h"
#include "contents/client_res.h"
#include "contents/ui_window.h"
#include "contents/ui_window_mgr.h"
#include "core/msg_str.h"
#include "ui/ui_std.h"

#define MSG_ALL_GROUPS         0xef5

#define REPUTE_MSG_SELECT_GROUP 0x16
#define REPUTE_MSG_COMBO_CHANGED 0x27
#define REPUTE_MSG_SEARCH_ENTER 0x88

static const char *search_text(struct UIReputeWnd *self)
{
    return ui_edit_ctrl_get_text(self->search_edit) ? ui_edit_ctrl_get_text(self->search_edit) : "";
}

static void list_search(struct UIReputeWnd *self, const struct String *text)
{
    struct Vec found;

    self->entries.last = self->entries.first;
    repute_wnd_entries_move(&self->entries,
                               repute_mgr_search_infos(repute_mgr_instance(), &found, text));
    vec_free(&found);
}

int repute_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                   int a, int b, int c, int d)
{
    struct UIReputeWnd *self = (struct UIReputeWnd *)window;
    struct String text, group;

    switch (msg) {
    case UI_MSG_BUTTON:
        switch (a) {
        case REPUTE_CLOSE_ID:
            repute_wnd_unhighlight_page(self);
            ui_window_mgr_post_quit1(&g_windowMgr, REPUTE_WND_ID);
            break;
        case REPUTE_PREV_ID:
            repute_wnd_unhighlight_page(self);
            repute_wnd_set_page(self, self->page - 1);
            ui_window_invalidate_base(&self->frame.base);
            break;
        case REPUTE_NEXT_ID:
            repute_wnd_unhighlight_page(self);
            repute_wnd_set_page(self, self->page + 1);
            ui_window_invalidate_base(&self->frame.base);
            break;
        case REPUTE_SEARCH_ID:
            string_init(&text);
            string_assign_cstr(&text, search_text(self));
            if (text.size == 0) {
                ui_window_send(&self->frame.base, self->group_combo, REPUTE_MSG_COMBO_CHANGED, 0, 0, 0, 0);
            } else {
                repute_wnd_unhighlight_page(self);
                list_search(self, &text);
                repute_wnd_set_page(self, 1);
                ui_window_invalidate_base(&self->frame.base);
            }
            string_free(&text);
            break;
        }
        break;

    case REPUTE_MSG_SELECT_GROUP: {
        const char *name = (const char *)a;
        char all;
        int index;

        if (!name)
            break;
        string_init(&text);
        string_assign_cstr(&text, name);
        all = string_equal_cstr(&text, msg_str(MSG_ALL_GROUPS));
        string_free(&text);
        if (all) {
            index = 0;
        } else {
            index = ui_combo_box_def_wnd_find_text(self->group_combo, name);
            if (index == -1)
                break;
        }
        ui_combo_box_def_wnd_select(self->group_combo, index);
        ui_window_send(&self->frame.base, self->group_combo, REPUTE_MSG_COMBO_CHANGED, 0, 0, 0, 0);
        break;
    }

    case REPUTE_MSG_COMBO_CHANGED:
        if (sender != self->group_combo)
            break;
        ui_combo_box_def_wnd_get_label_text(self->group_combo, &group);
        if (group.size != 0) {
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->search_edit, "");
            repute_wnd_unhighlight_page(self);
            repute_wnd_show_group(self, &group);
            repute_wnd_set_page(self, 1);
            ui_window_invalidate_base(&self->frame.base);
        }
        string_free(&group);
        break;

    case REPUTE_MSG_SEARCH_ENTER:
        string_init(&text);
        string_assign_cstr(&text, search_text(self));
        if (text.size == 0) {
            ui_combo_box_def_wnd_get_label_text(self->group_combo, &group);
            if (group.size == 0) {
                string_free(&group);
                string_free(&text);
                break;
            }
            repute_wnd_show_group(self, &group);
            string_free(&group);
        } else {
            list_search(self, &text);
        }
        repute_wnd_set_page(self, self->page);
        ui_window_invalidate_base(&self->frame.base);
        string_free(&text);
        break;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, a, b, c, d);
}

void repute_wnd_show_group(struct UIReputeWnd *self, const struct String *group)
{
    const char *all = msg_str(MSG_ALL_GROUPS);
    struct Vec list;
    struct Vec *got;
    struct ReputeInfo **it;
    long long total;

    self->entries.last = self->entries.first;
    if (string_equal_cstr(group, all)) {
        got = repute_mgr_all_infos(repute_mgr_instance(), &list);
        self->show_points = 0;
    } else {
        got = repute_mgr_group_infos(repute_mgr_instance(), &list, group);
        self->show_points = 1;
    }
    if (&self->entries != got) {
        vec_free(&self->entries);
        self->entries = *got;
        got->first = got->last = got->end = 0;
    }
    vec_free(&list);
    if (!self->show_points)
        return;
    total = 0;
    for (it = (struct ReputeInfo **)self->entries.first; it != (struct ReputeInfo **)self->entries.last; it++)
        if (*it)
            total += (*it)->points;
    self->group_points = total;
}

void repute_wnd_set_page(struct UIReputeWnd *self, unsigned page)
{
    unsigned count, clamped;

    if (self->entries.first == self->entries.last)
        count = 1;
    else
        count = (unsigned)(VEC_COUNT(&self->entries, struct ReputeInfo *) - 1) / self->rows_per_page + 1;
    self->page_count = count;
    clamped = page < count ? page : count;
    self->page = clamped < 1 ? 1 : clamped;
    ui_button_show((struct UIButton *)(self->prev_button), self->page > 1);
    ui_button_show((struct UIButton *)(self->next_button), self->page_count > self->page);
}

void repute_wnd_unhighlight_page(struct UIReputeWnd *self)
{
    unsigned i, at;
    struct ReputeInfo *e;

    for (i = 0; i < self->rows_per_page; i++) {
        at = (self->page - 1) * self->rows_per_page + i;
        if (at >= VEC_COUNT(&self->entries, struct ReputeInfo *))
            break;
        e = ((struct ReputeInfo **)self->entries.first)[at];
        if (e)
            e->highlighted = 0;
    }
}
