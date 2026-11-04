#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/chat/chat.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "ui/ui_std.h"

struct Vec;
struct UIBattleMsgOptionWnd;
struct UIWindow;
void ui_battle_msg_option_wnd_destroy(struct UIWindow *window);
void ui_battle_msg_option_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_battle_msg_option_wnd_on_draw(struct UIWindow *self);
void ui_battle_msg_option_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_battle_msg_option_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_battle_msg_option_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_battle_msg_option_wnd_on_wheel(struct UIWindow *self, int delta);
intptr_t ui_battle_msg_option_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
static void ui_battle_msg_option_wnd_update_scroll(struct UIBattleMsgOptionWnd *self);
const struct UIWindowOps ui_battle_msg_option_wnd_ops = {
    .destroy = ui_battle_msg_option_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_battle_msg_option_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_battle_msg_option_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_battle_msg_option_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_battle_msg_option_wnd_on_l_btn_up,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_battle_msg_option_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_battle_msg_option_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_battle_msg_option_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define BATTLE_OPTION_COUNT 21
#define BATTLE_ROW_HEIGHT 20
#define MSG_BATTLE_OPTION_HIDDEN 0x60d
#define MSG_BATTLE_OPTION_TITLE 0x509

static const int battle_option_msgs[BATTLE_OPTION_COUNT] = {
    0x50b, 0x4fd, 0x4fe, 0x4ff, 0x500, 0x501, 0x502, 0x503, 0x504, 0x505, 0x506,
    0x507, 0x508, 0x533, 0x60d, 0x652, 0x653, 0x654, 0x656, 0x657, 0xba4,
};

static const unsigned char battle_option_flags[BATTLE_OPTION_COUNT] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21,
};

struct UIBattleMsgOptionWnd {
    struct UIFrameWnd frame;
    struct UIScrollBar *scroll;
    int hover;
    int rows;
    int scrollable;
    int top;
    struct Vec labels;
    struct String title;
    BYTE on[BATTLE_OPTION_COUNT];
    int key;
    int all_on;
    BYTE kind;
    struct UIWindow *all_on_box;
};

static struct ChatTabSettings *battle_settings(struct UIBattleMsgOptionWnd *self)
{
    return chat_tabs_get_or_add(self->kind ? &g_session1.chatWndTabs : &g_session1.subChatWnds, self->key);
}

#define LABEL_COUNT(self) VEC_COUNT(&(self)->labels, const char *)
#define LABEL(self, i) VEC_AT(&(self)->labels, const char *, i)

struct UIBattleMsgOptionWnd *ui_battle_msg_option_wnd_construct(struct UIBattleMsgOptionWnd *self)
{
    const char *title;
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_battle_msg_option_wnd_ops;
    self->labels.first = self->labels.last = self->labels.end = NULL;
    string_init(&self->title);
    self->hover = -1;
    self->key = -1;
    self->kind = 1;
    self->all_on = 0;
    for (i = 0; i < BATTLE_OPTION_COUNT; i++) {
        const char *label = msg_str(battle_option_msgs[i]);

        vec_push(&self->labels, &label, sizeof(const char *));
    }
    title = msg_str(MSG_BATTLE_OPTION_TITLE);
    string_assign_cstr(&self->title, title);
    return self;
}

struct UIWindow *ui_battle_msg_option_wnd_new(void)
{
    return (struct UIWindow *)ui_battle_msg_option_wnd_construct(mem_alloc(sizeof(struct UIBattleMsgOptionWnd)));
}

void ui_battle_msg_option_wnd_destroy(struct UIWindow *window)
{
    struct UIBattleMsgOptionWnd *self = (struct UIBattleMsgOptionWnd *)window;
    self->frame.base.ops = &ui_battle_msg_option_wnd_ops;
    string_free(&self->title);
    vec_free(&self->labels);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_battle_msg_option_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIBattleMsgOptionWnd *self = (struct UIBattleMsgOptionWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIScrollBar *scroll = mem_alloc(sizeof(struct UIScrollBar));
    struct UIWindow *box;

    self->scroll = scroll ? ui_scroll_bar_construct(scroll) : NULL;
    ui_scroll_bar_create(self->scroll, wnd->width - 0xf, 0xe, 1, wnd->height - 0x26);
    ui_window_add_child(wnd, (struct UIWindow *)self->scroll);
    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "ESC", 0);

    box = mem_alloc(sizeof(struct UICheckBox));
    if (box)
        box = (struct UIWindow *)ui_check_box_construct(box);
    self->all_on_box = box;
    ui_window_create(box, 0x46, 0xc);
    ui_check_box_set_bitmaps(self->all_on_box, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp", " all on");
    ui_window_move_base(self->all_on_box, 0x14, wnd->height - 0x10);
    ui_check_box_set_check(self->all_on_box, self->all_on);
    ui_window_add_child(wnd, self->all_on_box);

    self->rows = (height - 0x26) / BATTLE_ROW_HEIGHT;
    ui_battle_msg_option_wnd_update_scroll(self);
}

static int is_hidden(const char *label)
{
    return strcmp(label, msg_str(MSG_BATTLE_OPTION_HIDDEN)) == 0;
}

void ui_battle_msg_option_wnd_on_draw(struct UIWindow *window)
{
    struct UIBattleMsgOptionWnd *self = (struct UIBattleMsgOptionWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int top = ui_window_draw_title_bar(wnd, 1, string_cstr(&self->title), 0);
    int body, strips, i, y, r, g, b, shown;
    struct BitmapRes *res;
    unsigned index;

    ui_window_draw_body(wnd, top, 0, "");
    body = wnd->height - 0x2a;
    strips = body / 8 + (body % 8 != 0);
    for (i = 0, y = 0x13; i < strips; i++, y += 8)
        ui_window_draw_bitmap(wnd, 0, y, ui_skin_bitmap(UI_SKIN "basic_interface\\itemwin_left.bmp"), 0);
    body = wnd->height - 0x26;
    strips = body / 8 + (body % 8 != 0);
    for (i = 0, y = 0x11; i < strips; i++, y += 8) {
        res = ui_skin_bitmap(UI_SKIN "basic_interface\\itemwin_right.bmp");
        ui_window_draw_bitmap(wnd, wnd->width - res->width, y, res, 0);
    }
    ui_skin_color_chip(&g_skinMgr, 0xe, 6, &r, &g, &b);
    ui_window_fill_rect(wnd, 0x12, 0x11, 1, wnd->height - 0x15, RGB(r, g, b));
    ui_window_draw_button_bar(wnd, 0, 0, 1);

    shown = 1;
    y = 0x16;
    for (index = 0; index < LABEL_COUNT(self); index++) {
        if (is_hidden(LABEL(self, index)) || (int)index < self->top)
            continue;
        if (self->hover == (int)index) {
            ui_skin_color_chip(&g_skinMgr, 6, 2, &r, &g, &b);
            ui_window_fill_rect(wnd, 0x19, y - 3, wnd->width - 0x28, 0x14, RGB(r, g, b));
            ui_window_fill_rect(wnd, 0x1a, y - 2, wnd->width - 0x2a, 0x12, 0xffffff);
        }
        ui_window_text_out(wnd, 0x32, y, LABEL(self, index), 0, 0, 0xc, 0, 0, 0);
        ui_window_draw_bitmap(wnd, 0x1b, y,
                            ui_skin_bitmap(self->on[index] ? UI_SKIN "basic_interface\\grp_online.bmp"
                                                        : UI_SKIN "basic_interface\\grp_offline.bmp"),
                            1);
        if (shown == self->rows)
            break;
        y += BATTLE_ROW_HEIGHT;
        shown++;
    }
}

static int battle_option_at(struct UIBattleMsgOptionWnd *self, int x, int y)
{
    int row, shown = 0;
    unsigned index;

    if (x < 0 || y < 0x11 || y >= self->rows * BATTLE_ROW_HEIGHT + 0x11)
        return -1;
    row = (y - 0x11) / BATTLE_ROW_HEIGHT + x / self->frame.base.width;
    for (index = 0; index < LABEL_COUNT(self); index++) {
        if (is_hidden(LABEL(self, index)) || (int)index < self->top)
            continue;
        if (shown == row)
            return index;
        shown++;
    }
    return -1;
}

void ui_battle_msg_option_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIBattleMsgOptionWnd *self = (struct UIBattleMsgOptionWnd *)window;
    int old_hover, all_on, i;
    struct ChatTabSettings *tab;

    if (y < 0x11) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    old_hover = self->hover;
    self->hover = battle_option_at(self, x, y);
    if (old_hover != self->hover)
        ui_window_invalidate_base(&self->frame.base);
    if (self->hover == -1)
        return;
    tab = battle_settings(self);
    if (!tab)
        return;
    self->on[self->hover] = self->on[self->hover] == 0;
    tab->shown[battle_option_flags[self->hover]] = self->on[self->hover];
    self->all_on = 1;
    all_on = 1;
    for (i = 0; i < BATTLE_OPTION_COUNT; i++) {
        if (self->on[i] == 0) {
            self->all_on = 0;
            all_on = 0;
            break;
        }
    }
    self->all_on_box->state = all_on;
    ui_window_invalidate_base(self->all_on_box);
    ui_window_invalidate_base(&self->frame.base);
}

void ui_battle_msg_option_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIBattleMsgOptionWnd *self = (struct UIBattleMsgOptionWnd *)window;
    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
}

void ui_battle_msg_option_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIBattleMsgOptionWnd *self = (struct UIBattleMsgOptionWnd *)window;
    int old_hover = self->hover;

    self->hover = battle_option_at(self, x, y);
    if (old_hover != self->hover)
        ui_window_invalidate_base(&self->frame.base);
}

void ui_battle_msg_option_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIBattleMsgOptionWnd *self = (struct UIBattleMsgOptionWnd *)window;
    int step;

    if (!self->scrollable)
        return;
    step = self->rows <= 3 ? self->rows - 1 : 3;
    ui_battle_msg_option_wnd_send_msg(&self->frame.base, (struct UIWindow *)self->scroll, UI_MSG_SCROLL_V, delta < 0 ? step : -step, 0, 0, 0);
}

static void ui_battle_msg_option_wnd_update_scroll(struct UIBattleMsgOptionWnd *self)
{
    int total, extra;

    if (!self->scroll)
        return;
    total = (int)LABEL_COUNT(self) + ((self->labels.first != self->labels.last) - 1);
    extra = total - self->rows < 0 ? 0 : total - self->rows;
    self->scrollable = extra != 0;
    if (extra) {
        ui_scroll_bar_set_range(self->scroll, extra, self->rows, total);
        if (self->top < 0)
            self->top = 0;
        if (self->top > total - self->rows)
            self->top = total - self->rows;
        ui_scroll_bar_set_pos(self->scroll, self->top);
    } else {
        self->top = 0;
        ui_scroll_bar_set_range(self->scroll, 0, self->rows, total);
    }
    ui_window_invalidate_base(&self->scroll->base);
}

#define BATTLE_ID_CLOSE 0xc1
#define BATTLE_ID_ALL_ON 0xcd
#define MSG_BATTLE_OPTION_SUFFIX 0x6f1

static void show_tab(struct UIBattleMsgOptionWnd *self, int key, int kind, int how)
{
    struct ChatTabSettings tab = {0};
    struct ChatTabSettings *source;

    if (self->key == key && self->kind == (kind != 0) && how != 2) {
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_BATTLE_MSG_OPTION);
        return;
    }
    if (how == 1)
        return;
    source = chat_tabs_get_or_add(kind ? &g_session1.chatWndTabs : &g_session1.subChatWnds, key);
    if (!source || !chat_tab_settings_copy(&tab, source))
        return;
    self->key = key;
    self->kind = kind != 0;
    if (tab.name.size) {
        int i;

        self->all_on = 1;
        for (i = 0; i < BATTLE_OPTION_COUNT; i++) {
            self->on[i] = tab.shown[battle_option_flags[i]];
            if (!self->on[i])
                self->all_on = 0;
        }
        self->all_on_box->state = self->all_on;
        ui_window_invalidate_base(self->all_on_box);
        string_assign(&self->title, owned_text_cstr(&tab.name), tab.name.size);
        {
            const char *suffix = msg_str(MSG_BATTLE_OPTION_SUFFIX);

            string_append_cstr(&self->title, suffix);
        }
        ui_window_invalidate_base(&self->frame.base);
    }
    ui_battle_msg_option_wnd_update_scroll(self);
    chat_tab_settings_clear(&tab);
}

intptr_t ui_battle_msg_option_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIBattleMsgOptionWnd *self = (struct UIBattleMsgOptionWnd *)window;
    int old_top, i;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == BATTLE_OPTION_FOR_TAB) {
            show_tab(self, val2, val3, val4);
        } else if (val1 == BATTLE_ID_CLOSE) {
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_BATTLE_MSG_OPTION);
        } else if (val1 == BATTLE_ID_ALL_ON) {
            struct ChatTabSettings *tab = battle_settings(self);

            if (!tab)
                return 0;
            self->all_on = val2;
            for (i = 0; i < BATTLE_OPTION_COUNT; i++) {
                BYTE on = self->all_on != 0;

                self->on[i] = on;
                tab->shown[battle_option_flags[i]] = on;
            }
            ui_window_invalidate_base(&self->frame.base);
        }
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        old_top = self->top;
        if (msg == UI_MSG_PAGE_UP)
            self->top = old_top - self->rows + 1;
        else if (msg == UI_MSG_PAGE_DOWN)
            self->top = old_top + self->rows - 1;
        else
            self->top = old_top + val1;
        if (old_top == self->top)
            return 0;
        ui_battle_msg_option_wnd_update_scroll(self);
        ui_window_invalidate_base(&self->frame.base);
        return self->top - old_top;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
