#include "core/object_type_tables.h"
#include "ui/quests/achievement_internal.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_achievements.h"
#include "session/session_jobs.h"
#include "std/tree.h"

struct UIAchTab;
void ui_ach_tab_destroy(struct UIWindow *window);
void ui_ach_tab_on_draw(struct UIWindow *self);
void ui_ach_tab_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_ach_tab_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_ach_tab_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_ach_tab_on_wheel(struct UIWindow *self, int delta);
intptr_t ui_ach_tab_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_ach_tab_ops = {
    .destroy = ui_ach_tab_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_ach_tab_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_ach_tab_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_ach_tab_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_ach_tab_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_ach_tab_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_ach_tab_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIAchListWnd;
void ui_ach_list_wnd_destroy(struct UIWindow *window);
void ui_ach_list_wnd_on_draw(struct UIWindow *self);
void ui_ach_list_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_ach_list_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_ach_list_wnd_on_wheel(struct UIWindow *self, int delta);
intptr_t ui_ach_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_ach_list_wnd_ops = {
    .destroy = ui_ach_list_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_ach_list_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_ach_list_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_ach_list_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_ach_list_wnd_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_ach_list_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

static struct BitmapRes *ach_icon_bitmap(const struct String *icon)
{
    struct String path;
    struct BitmapRes *bitmap;

    string_init(&path);
    string_reserve(&path, icon->size + sizeof(ACH_DIR "icon_") - 1);
    string_append(&path, ACH_DIR "icon_", sizeof(ACH_DIR "icon_") - 1);
    string_append(&path, string_cstr(icon), icon->size);
    string_append_cstr(&path, ".bmp");
    bitmap = ui_skin_bitmap(string_cstr(&path));
    string_free(&path);
    return bitmap;
}

#define LIST_ID(self, i) VEC_AT(&(self)->ids, int, i)
#define LIST_COUNT(self) VEC_COUNT(&(self)->ids, int)

struct UIAchListWnd *ui_ach_list_wnd_construct(struct UIAchListWnd *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_ach_list_wnd_ops;
    self->row_height = 0;
    self->row_count = 0;
    self->top = 0;
    self->selected = 0;
    self->hover_row = 0;
    self->ids.first = NULL;
    self->ids.last = NULL;
    self->ids.end = NULL;
    self->scrollable = 0;
    self->scroll_width = 0xb;
    self->achs = NULL;
    self->scroll = NULL;
    self->rows = NULL;
    return self;
}

static void ui_ach_list_wnd_destruct(struct UIAchListWnd *self)
{
    self->base.ops = &ui_ach_list_wnd_ops;
    if (self->rows)
        mem_free(self->rows);
    vec_free(&self->ids);
    ui_window_destruct(&self->base);
}

void ui_ach_list_wnd_destroy(struct UIWindow *window)
{
    struct UIAchListWnd *self = (struct UIAchListWnd *)window;
    ui_ach_list_wnd_destruct(self);
    mem_free(self);
}

static void list_tell_parent(struct UIAchListWnd *self)
{
    struct UIWindow *parent = self->base.parent;

    if (parent)
        ui_window_send_msg(parent, &self->base, ACH_MSG_SHOW,
                 (unsigned)self->selected < LIST_COUNT(self) ? LIST_ID(self, self->selected) : -1, 0, 0, 0);
}

static void ui_ach_list_wnd_select(struct UIAchListWnd *self, int index)
{
    self->selected = index;
    list_tell_parent(self);
    ui_window_invalidate_base(&self->base);
}

void ui_ach_list_wnd_init(struct UIAchListWnd *self, int width, int height, int rows)
{
    struct UIAchScrollBar *bar;
    int i;

    self->row_height = height / rows;
    self->row_count = rows;
    bar = ach_new_scroll_bar();
    self->scroll = bar;
    ui_scroll_bar_create(&bar->bar, width - self->scroll_width, 0, 1, height);
    ui_window_add_child(&self->base, &self->scroll->bar.base);
    self->rows = mem_alloc(self->row_count > 0x3fffffff ? (size_t)-1 : (size_t)self->row_count * 4);
    for (i = 0; i < self->row_count; i++) {
        struct UIButton *row = mem_alloc(sizeof *row);

        self->rows[i] = row ? ui_button_construct(row) : NULL;
        ui_window_create(&self->rows[i]->base, 0x1e, 0x1e);
        ui_window_move_base(&self->rows[i]->base, 5, self->row_height * i + 5);
        self->rows[i]->base.id = i;
        self->rows[i]->skinned = 0;
        ui_window_add_child(&self->base, &self->rows[i]->base);
    }
    ui_window_on_create_base(&self->base, width, height);
    ui_window_resize_base(&self->base, width, height);
}

static void ui_ach_list_wnd_refresh(struct UIAchListWnd *self)
{
    int max, i;

    if (!self->achs || !self->scroll)
        return;
    max = (int)self->achs->size - self->row_count;
    if (max < 0)
        max = 0;
    self->scrollable = max > 0;
    if (max > 0) {
        ui_scroll_bar_set_range(&self->scroll->bar, max, self->row_count, self->achs->size);
        if (self->top < 0)
            self->top = 0;
        if (self->top > max)
            self->top = max;
        ui_scroll_bar_set_pos(&self->scroll->bar, self->top);
    } else {
        self->top = 0;
        ui_scroll_bar_set_range(&self->scroll->bar, 0, self->row_count, self->achs->size);
    }
    ui_window_invalidate_base(&self->scroll->bar.base);
    for (i = 0; i < self->row_count; i++) {
        self->rows[i]->disabled = (unsigned)(self->top + i) >= self->achs->size;
        ui_window_invalidate_base(&self->rows[i]->base);
    }
}

static void list_select_id(struct UIAchListWnd *self, int id)
{
    int *p;
    int index = 0, top;

    for (p = self->ids.first; p != (int *)self->ids.last && id != *p; p++)
        index++;
    self->selected = index;
    if (index < self->row_count) {
        top = 0;
    } else {
        unsigned max = self->achs->size - self->row_count;

        top = (unsigned)index > max ? (int)max : index;
    }
    self->top = top;
    ui_scroll_bar_set_pos(&self->scroll->bar, top);
    list_tell_parent(self);
    ui_window_invalidate_base(&self->base);
}

void ui_ach_list_wnd_select_id(struct UIAchListWnd *self, int id)
{
    list_select_id(self, id);
}

struct SessionAchievementState *ach_state(struct SessionAchievementState *out, int id)
{
    return session_get_achievement(&g_session1, out, id);
}

void ui_ach_list_wnd_fill(struct UIAchListWnd *self, struct Tree *achs, int order)
{
    struct TreeNode *node;
    struct SessionAchievementState state;

    self->ids.last = self->ids.first;
    if (order == 0) {
        for (node = tree_first(achs); node != achs->head; node = tree_next(node)) {
            const int *id = &TREE_VALUE(node, struct AchievementInfoEntry)->id;

            if (ach_state(&state, *id)->completed == 1)
                vec_push(&self->ids, id, sizeof *id);
            ach_sort_ids(self->ids.first, self->ids.last);
        }
        for (node = tree_first(achs); node != achs->head; node = tree_next(node)) {
            const int *id = &TREE_VALUE(node, struct AchievementInfoEntry)->id;

            if (ach_state(&state, *id)->completed == 0)
                vec_push(&self->ids, id, sizeof *id);
        }
    } else if (order == 1) {
        for (node = tree_first(achs); node != achs->head; node = tree_next(node)) {
            const int *id = &TREE_VALUE(node, struct AchievementInfoEntry)->id;

            ach_state(&state, *id);
            if (state.completed == 1 && achievement_has_reward(g_achievementDb, *id) == 1 && !state.rewarded)
                vec_push(&self->ids, id, sizeof *id);
            ach_sort_ids(self->ids.first, self->ids.last);
        }
        for (node = tree_first(achs); node != achs->head; node = tree_next(node)) {
            const int *id = &TREE_VALUE(node, struct AchievementInfoEntry)->id;

            if (ach_state(&state, *id)->completed == 0)
                vec_push(&self->ids, id, sizeof *id);
        }
    }
    self->achs = achs;
    self->selected = 0;
    self->top = 0;
    ui_ach_list_wnd_refresh(self);
    self->selected = 0;
    if (self->base.parent)
        ui_window_send_msg(self->base.parent, &self->base, ACH_MSG_SHOW,
                 LIST_COUNT(self) ? LIST_ID(self, 0) : -1, 0, 0, 0);
    ui_window_invalidate_base(&self->base);
}

void ui_ach_list_wnd_on_draw(struct UIWindow *window)
{
    struct UIAchListWnd *self = (struct UIAchListWnd *)window;
    int i;

    ach_draw(&self->base, 0, 0, ACH_DIR "bg_list.bmp", 0);
    for (i = 0; i < self->row_count; i++) {
        int index = self->top + i, y = self->row_height * i;
        struct AchievementInfoEntry *entry;
        struct AchievementInfo *info;
        struct SessionAchievementState state;
        struct BitmapRes *icon;
        struct String text;
        const char *row;
        int id;

        if ((unsigned)index >= LIST_COUNT(self))
            continue;
        if (!self->achs->size)
            continue;
        id = LIST_ID(self, index);
        entry = tree_find(self->achs, &id, tree_less_int);
        if (!entry)
            continue;
        ach_state(&state, entry->id);
        if (self->selected == self->top + i)
            row = state.completed == 1 ? ACH_DIR "list_complete_press.bmp" : ACH_DIR "list_press.bmp";
        else
            row = state.completed == 1 ? ACH_DIR "list_complete_out.bmp" : ACH_DIR "list_out.bmp";
        ach_draw(&self->base, 0, y, row, 0);
        info = &entry->info;
        icon = ach_icon_bitmap(&info->ui_text);
        if (icon)
            ui_window_draw_bitmap(&self->base, 8, y + 3, icon, 1);
        if (session_is_tracked_achievement(&g_session1, LIST_ID(self, self->top + i)) == 1)
            ach_draw(&self->base, 8, y + 3, ACH_DIR "list_pin.bmp", 1);
        ui_window_text_out(&self->base, 0x35, y + 0xe, string_cstr(&info->text[0]), 0, 0, 0xc,
                         ACH_TEXT_COLOUR, 1, 0);
        string_init(&text);
        string_format(&text, "%3d", info->info_number);
        ui_window_text_out_centred(&self->base, 0x102, y + 0xe, string_cstr(&text), 0x28, 0, 0xc,
                                ACH_TEXT_COLOUR, 1);
        string_free(&text);
        ui_window_text_out(&self->base, 0xa, y + 0x36, string_cstr(&info->text[1]), 0, 0, 0xc,
                         ACH_TEXT_COLOUR, 0, 0);
        if (state.completed == 1)
            ach_draw(&self->base, 0xfa, y + 5, ACH_DIR "badge_complete.bmp", 1);
        if (ach_has_reward(info) == 1 && !state.rewarded)
            ach_draw(&self->base, 0x107, y + 0x31,
                     state.completed == 1 ? ACH_DIR "list_rewardbox_not_receive.bmp"
                                          : ACH_DIR "list_rewardbox_default.bmp", 1);
        if (state.completed == 1) {
            int width;

            string_format(&text, "%4d.%2d.%2d", state.year, state.month, state.day);
            width = ui_window_text_width(&self->base, string_cstr(&text), 0, 0, 0xa, 0, 0);
            ui_window_text_out_outlined(&self->base, (int)(276.0 - width * 0.5), y + 0x37,
                                     string_cstr(&text), 0, ACH_DATE_COLOUR, 0xffffff, 0, 0xa, 0, 0);
            string_free(&text);
        }
    }
}

void ui_ach_list_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIAchListWnd *self = (struct UIAchListWnd *)window;
    int row;

    if (x < 0 || self->base.width - self->scroll_width < x)
        return;
    row = y / self->row_height;
    if (self->achs->size <= (unsigned)row)
        return;
    ui_ach_list_wnd_select(self, self->top + row);
    ui_window_mgr_set_capture(&g_windowMgr, &self->base);
}

void ui_ach_list_wnd_on_l_btn_up(struct UIWindow *self, int x, int y)
{
    ui_window_mgr_release_capture(&g_windowMgr);
}

void ui_ach_list_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIAchListWnd *self = (struct UIAchListWnd *)window;
    if (!self->scrollable)
        return;
    ui_ach_list_wnd_send_msg(&self->base, &self->scroll->bar.base, UI_MSG_SCROLL_V, delta < 0 ? 3 : -3, 0, 0, 0);
}

intptr_t ui_ach_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                    intptr_t val3, intptr_t val4)
{
    struct UIAchListWnd *self = (struct UIAchListWnd *)window;
    int old, top;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (session_toggle_tracked_achievement(&g_session1, LIST_ID(self, self->top + val1))) {
            struct UIWindow *tracer = ui_make_window(ACH_TRACING_WND_ID);

            if (tracer)
                ui_window_invalidate(tracer);
        }
        break;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        old = self->top;
        if (msg == UI_MSG_PAGE_UP)
            top = old - self->row_count + 1;
        else if (msg == UI_MSG_PAGE_DOWN)
            top = self->row_count - 1 + old;
        else
            top = val1 + old;
        self->top = top;
        if (old != top) {
            ui_ach_list_wnd_refresh(self);
            ui_window_invalidate_base(&self->base);
            top = self->top;
        }
        return top - old;
    }
    ui_window_invalidate_base(&self->base);
    return 0;
}

struct AchTab {
    int id;
    struct String name;
    struct Tree subs;
};

struct AchSubTab {
    int id;
    struct String name;
};

static void destroy_sub_tab(void *value)
{
    struct AchSubTab *sub = value;

    string_free(&sub->name);
}

static void destroy_tab(void *value)
{
    struct AchTab *tab = value;

    tree_free(&tab->subs, destroy_sub_tab);
    string_free(&tab->name);
}

static struct AchTab *tab_at(struct UIAchTab *self, int id)
{
    struct AchTab *tab = tree_find(&self->tabs, &id, tree_less_int);
    struct AchTab fresh;

    if (tab)
        return tab;
    fresh.id = id;
    string_init(&fresh.name);
    tree_init(&fresh.subs);
    return tree_add(&self->tabs, &fresh, sizeof fresh, tree_less_int, NULL);
}

static struct String *sub_tab_name(struct UIAchTab *self, int main, int sub)
{
    struct AchSubTab *entry = tree_get(&tab_at(self, main)->subs, &sub, sizeof sub, sizeof *entry, tree_less_int);

    return &entry->name;
}

struct UIAchTab *ui_ach_tab_construct(struct UIAchTab *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_ach_tab_ops;
    self->sel_main = -1;
    self->sel_sub = -1;
    self->font_height = 0;
    self->hover_main = 0;
    self->hover_sub = 0;
    tree_init(&self->tabs);
    self->scroll = NULL;
    self->scrollable = 0;
    self->scroll_x = 0xd;
    self->visible = 3;
    self->font_height = 0xc;
    self->top = 0;
    return self;
}

static void ui_ach_tab_destruct(struct UIAchTab *self)
{
    self->base.ops = &ui_ach_tab_ops;
    tree_free(&self->tabs, destroy_tab);
    ui_window_destruct(&self->base);
}

void ui_ach_tab_destroy(struct UIWindow *window)
{
    struct UIAchTab *self = (struct UIAchTab *)window;
    ui_ach_tab_destruct(self);
    mem_free(self);
}

static int tab_shown_subs(struct UIAchTab *self)
{
    int count = tab_at(self, self->sel_main)->subs.size;

    return count < self->visible ? count : self->visible;
}

static void ui_ach_tab_relayout(struct UIAchTab *self)
{
    struct TreeNode *node;
    int height = self->tabs.size * self->tab_height;

    for (node = tree_first(&self->tabs); node != self->tabs.head; node = tree_next(node))
        height += TREE_VALUE(node, struct AchTab)->subs.size * self->sub_height;
    ui_window_resize_base(&self->base, self->tab_width, height + 1);
    ui_window_invalidate_base(&self->base);
}

static void ui_ach_tab_update_scroll(struct UIAchTab *self)
{
    int count, max;

    if (!self->scroll)
        return;
    count = tab_at(self, self->sel_main)->subs.size;
    max = count - self->visible;
    if (max < 0)
        max = 0;
    self->scrollable = max > 0;
    if (max > 0) {
        ui_scroll_bar_set_range(&self->scroll->bar, max, self->visible, count);
        if (self->top < 0)
            self->top = 0;
        if (self->top > max)
            self->top = max;
        ui_scroll_bar_set_pos(&self->scroll->bar, self->top);
    } else {
        self->top = 0;
        ui_scroll_bar_set_range(&self->scroll->bar, 0, self->visible, count);
    }
    ui_window_move_base(&self->scroll->bar.base, 0, (self->sel_main + 1) * self->tab_height);
    ui_window_invalidate_base(&self->scroll->bar.base);
}

void ui_ach_tab_init(struct UIAchTab *self, int tab_width, int tab_height, int sub_height, BYTE flag,
                              COLORREF sel_colour, COLORREF colour, COLORREF sel_sub_colour, COLORREF sub_colour)
{
    struct UIAchScrollBar *bar;

    ui_window_on_create_base(&self->base, 0, 0);
    self->tab_width = tab_width;
    self->tab_height = tab_height;
    self->sub_height = sub_height;
    self->flag_ac = flag;
    self->sel_colour = sel_colour ? sel_colour : 0x9c1810;
    self->colour = colour ? colour : 0x9c6b5a;
    self->sel_sub_colour = sel_sub_colour ? sel_sub_colour : 0x9b6a59;
    self->sub_colour = sub_colour ? sub_colour : 0x9b6a59;
    tree_clear(&self->tabs, destroy_tab);
    self->sel_main = 0;
    self->sel_sub = -1;
    ui_ach_tab_relayout(self);
    bar = ach_new_scroll_bar();
    self->scroll = bar;
    ui_scroll_bar_create(&bar->bar, self->scroll_x, 0, 1, self->visible * self->sub_height);
    ui_window_add_child(&self->base, &self->scroll->bar.base);
}

void ui_ach_tab_set_name(struct UIAchTab *self, const char *name, int main, int sub)
{
    struct String *s;

    if (sub == -1)
        s = &tab_at(self, main)->name;
    else
        s = sub_tab_name(self, main, sub);
    string_assign_cstr(s, name);
    self->sel_main = 0;
    self->sel_sub = -1;
    ui_ach_tab_relayout(self);
}

struct String *ui_ach_tab_get_name(struct UIAchTab *self, struct String *out, int main, int sub)
{
    struct String name;

    string_copy(&name, &tab_at(self, main)->name);
    if (sub == -1) {
        *out = name;
        string_init(&name);
    } else {
        string_init(out);
        string_format(out, "%s", string_cstr(sub_tab_name(self, main, sub)));
    }
    string_free(&name);
    return out;
}

static BYTE ui_ach_tab_hit_test(struct UIAchTab *self, int x, int y, int *main, int *sub)
{
    int subs_top, span;

    if (x < 0 || self->tab_width < x || y < 0)
        return 0;
    subs_top = (self->sel_main + 1) * self->tab_height;
    span = self->sub_height * tab_shown_subs(self);
    if (y > subs_top && y < span + subs_top) {
        *main = self->sel_main;
        *sub = (y - subs_top) / self->sub_height + self->top;
        return 1;
    }
    if (y >= subs_top)
        y -= span;
    *main = y / self->tab_height;
    *sub = -1;
    return 1;
}

static void ui_ach_tab_draw_tab(struct UIAchTab *self, int main)
{
    struct String name;
    int y = self->tab_height * main, text_y;

    string_copy(&name, &tab_at(self, main)->name);
    text_y = (self->tab_height - ui_window_text_height(&self->base, string_cstr(&name), 0, 0,
                                                     self->font_height, 0, 0)) / 2 + y;
    if (main > self->sel_main) {
        y += self->sub_height * tab_shown_subs(self);
        text_y += self->sub_height * tab_shown_subs(self);
    }
    if (main == self->sel_main) {
        struct Tree *subs;
        struct TreeNode *node;
        int index = 0, shown = 0;

        ach_draw(&self->base, 0, y, ACH_DIR "tab_press.bmp", 0);
        ui_window_text_out(&self->base, 0xb, text_y, string_cstr(&name), 0, 0, self->font_height,
                         self->sel_colour, 1, 0);
        subs = &tab_at(self, main)->subs;
        for (node = tree_first(subs); node != subs->head && shown < self->visible; node = tree_next(node), index++) {
            const struct AchSubTab *sub = TREE_VALUE(node, struct AchSubTab);
            COLORREF colour;
            int sub_y;

            if (index < self->top)
                continue;
            sub_y = (main + 1) * self->tab_height + self->sub_height * shown++;
            string_assign(&name, string_cstr(&sub->name), sub->name.size);
            text_y = (self->sub_height - ui_window_text_height(&self->base, string_cstr(&name), 0, 0,
                                                             self->font_height, 0, 0)) / 2 + sub_y;
            if (sub->id == self->sel_sub) {
                ach_draw(&self->base, 0, sub_y, ACH_DIR "tab_sub_press.bmp", 0);
                ach_draw(&self->base, 0xc, sub_y + 7, ACH_DIR "tab_subarrow.bmp", 1);
                colour = self->sel_sub_colour;
            } else {
                ach_draw(&self->base, 0, sub_y,
                         sub->id == self->hover_sub ? ACH_DIR "tab_sub_over.bmp" : ACH_DIR "tab_sub_out.bmp", 0);
                colour = self->sub_colour;
            }
            ui_window_text_out(&self->base, 0x1a, text_y, string_cstr(&name), 0, 0, self->font_height,
                             colour, 0, 0);
        }
    } else {
        ach_draw(&self->base, 0, y, main == self->hover_main ? ACH_DIR "tab_over.bmp" : ACH_DIR "tab_out.bmp", 0);
        ui_window_text_out(&self->base, 0xb, text_y, string_cstr(&name), 0, 0, self->font_height,
                         self->colour, 1, 0);
    }
    string_free(&name);
}

void ui_ach_tab_on_draw(struct UIWindow *window)
{
    struct UIAchTab *self = (struct UIAchTab *)window;
    struct TreeNode *node;

    ach_draw(&self->base, 0, 0, ACH_DIR "bg_tab.bmp", 0);
    if (self->base.parent)
        ui_window_send_msg(self->base.parent, &self->base, ACH_MSG_TAB_DRAWN, 0, 0, 0, 0);
    for (node = tree_first(&self->tabs); node != self->tabs.head; node = tree_next(node))
        ui_ach_tab_draw_tab(self, TREE_VALUE(node, struct AchTab)->id);
}

void ui_ach_tab_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIAchTab *self = (struct UIAchTab *)window;
    int main = 0, sub = 0;

    if (!ui_ach_tab_hit_test(self, x, y, &main, &sub))
        return;
    self->sel_main = main;
    self->sel_sub = sub;
    if (self->base.parent) {
        ui_window_send_msg(self->base.parent, &self->base, ACH_MSG_SELECT_TAB, main, sub, 0, 0);
        ui_window_invalidate(self->base.parent);
    }
    ui_ach_tab_update_scroll(self);
    ui_window_invalidate_base(&self->base);
    ui_window_mgr_set_capture(&g_windowMgr, &self->base);
}

void ui_ach_tab_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIAchTab *self = (struct UIAchTab *)window;
    int main = 0, sub = 0;

    ui_ach_tab_hit_test(self, x, y, &main, &sub);
    self->hover_main = main;
    self->hover_sub = sub;
    ui_window_invalidate_base(&self->base);
}

void ui_ach_tab_on_l_btn_up(struct UIWindow *self, int x, int y)
{
    ui_window_mgr_release_capture(&g_windowMgr);
}

void ui_ach_tab_on_wheel(struct UIWindow *window, int delta)
{
    struct UIAchTab *self = (struct UIAchTab *)window;
    if (!self->scrollable)
        return;
    ui_ach_tab_send_msg(&self->base, &self->scroll->bar.base, UI_MSG_SCROLL_V, delta < 0 ? 1 : -1, 0, 0, 0);
}

intptr_t ui_ach_tab_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3,
                                intptr_t val4)
{
    struct UIAchTab *self = (struct UIAchTab *)window;
    int old, top;

    if (msg != UI_MSG_SCROLL_V && msg != UI_MSG_PAGE_UP && msg != UI_MSG_PAGE_DOWN)
        return 0;
    old = self->top;
    if (msg == UI_MSG_PAGE_UP)
        top = old - self->visible + 1;
    else if (msg == UI_MSG_PAGE_DOWN)
        top = self->visible - 1 + old;
    else
        top = val1 + old;
    self->top = top;
    if (old != top) {
        ui_ach_tab_update_scroll(self);
        ui_window_invalidate_base(&self->base);
        top = self->top;
    }
    return top - old;
}
