#include "ui/window_keyboard.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "core/skin_path.h"
#include "diag/perf.h"
#include "ui/wndmgr/make_window_internal.h"

#define CMDLIST_TITLE_HEIGHT 0x11
#define CMDLIST_ROW_HEIGHT 20
#define CMDLIST_TALK_SHOW_NAME 0x6a
#define CMDLIST_QUIT_ID 0x70

struct CmdListEntry {
    const char *text;
    int value;
};

struct UICommandListWnd {
    struct UIFrameWnd frame;
    struct UITab *tab;
    struct UIScrollBar *scroll;
    int hover;
    int columns;
    int rows;
    int has_scroll;
    int scroll_row;
};
OFFSET_ASSERT(UICommandListWnd, tab, 0x94);
OFFSET_ASSERT(UICommandListWnd, scroll_row, 0xac);

#define CMDLIST_TAB(self) ((self)->tab->base.state)

static COLORREF chip_colour(int x, int y)
{
    int red, green, blue;
    ui_skin_color_chip(&g_skinMgr, x, y, &red, &green, &blue);
    return RGB((BYTE)red, (BYTE)green, (BYTE)blue);
}

int ui_command_list_wnd_entry_at(struct UICommandListWnd *self, int x, int y)
{
    int col, row, cell_width;
    unsigned long long index;

    if (x < 0 || y < CMDLIST_TITLE_HEIGHT || self->columns <= 0 || self->rows <= 0 || self->scroll_row < 0)
        return -1;
    cell_width = self->frame.base.width / self->columns;
    if (cell_width <= 0)
        return -1;
    col = x / cell_width;
    row = (y - CMDLIST_TITLE_HEIGHT) / CMDLIST_ROW_HEIGHT;
    if (col >= self->columns || row >= self->rows)
        return -1;
    index = ((unsigned long long)self->scroll_row + row) * self->columns + col;
    return index < g_session1.talkOptionSettings.size ? (int)index : -1;
}

static void ui_command_list_wnd_update_scroll(struct UICommandListWnd *self)
{
    int count, pages, rows, extra;

    if (!self->scroll || self->columns <= 0)
        return;
    rows = self->rows;
    if (CMDLIST_TAB(self) == 1)
        count = (int)VEC_COUNT(&g_session1.commandListCommands, struct CmdListEntry);
    else
        count = (int)g_session1.talkOptionSettings.size;
    pages = count / self->columns + (count % self->columns != 0);
    extra = pages - rows;
    if (extra < 0)
        extra = 0;
    self->has_scroll = extra != 0;
    if (extra) {
        int pos;
        ui_scroll_bar_set_range(self->scroll, extra, rows, pages);
        pos = self->scroll_row;
        if (pos < 0) {
            self->scroll_row = 0;
            pos = 0;
        }
        if (pos > pages - self->rows) {
            pos = pages - self->rows;
            self->scroll_row = pos;
        }
        ui_scroll_bar_set_pos(self->scroll, pos);
    } else {
        self->scroll_row = 0;
        ui_scroll_bar_set_range(self->scroll, 0, rows, pages);
    }
    ui_window_invalidate_base(&self->scroll->base);
}

struct UIWindow;
void ui_command_list_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_command_list_wnd_on_draw(struct UIWindow *self);
void ui_command_list_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_command_list_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_command_list_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_command_list_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_command_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_command_list_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps ui_command_list_wnd_ops = {
    .destroy = ui_command_list_wnd_destroy,
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
    .on_create = ui_command_list_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_command_list_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_command_list_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_command_list_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_command_list_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_command_list_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_command_list_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UICommandListWnd *ui_command_list_wnd_construct(struct UICommandListWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_command_list_wnd_ops;
    self->hover = -1;
    self->columns = 2;
    return self;
}

struct UIWindow *ui_command_list_wnd_new(void)
{
    return (struct UIWindow *)ui_command_list_wnd_construct(mem_alloc(sizeof(struct UICommandListWnd)));
}

void ui_command_list_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UICommandListWnd *self = (struct UICommandListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UITab *tab;
    struct UIScrollBar *scroll;

    (void)width;
    tab = mem_alloc(sizeof(struct UITab));
    if (tab)
        tab = ui_tab_construct(tab);
    self->tab = tab;
    ui_window_create(&tab->base, 0x14, 0x59);
    self->tab->count = 2;
    self->tab->kind = 1;
    ui_window_move_base(&self->tab->base, 0, CMDLIST_TITLE_HEIGHT);
    ui_tab_send_msg(&self->tab->base, wnd, 0xd, 0, 0, 0, 0);
    ui_window_add_child(wnd, &self->tab->base);

    scroll = mem_alloc(sizeof(struct UIScrollBar));
    if (scroll)
        scroll = ui_scroll_bar_construct(scroll);
    self->scroll = scroll;
    ui_scroll_bar_create(scroll, wnd->width - 0xf, 0xe, 1, wnd->height - 0x26);
    ui_window_add_child(wnd, &self->scroll->base);
    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "Alt+Y", 0);
    self->rows = (height - 0x26) / CMDLIST_ROW_HEIGHT;
    ui_command_list_wnd_update_scroll(self);
}

void ui_command_list_wnd_on_draw(struct UIWindow *window)
{
    struct UICommandListWnd *self = (struct UICommandListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct BitmapRes *bmp;
    int n, y, left_w = 0, right_w = 0, bar_h = 0, x, w, i, shown, tab;

    ui_window_draw_body(wnd, ui_window_draw_title_bar(wnd, 0, "", 0), 0, "");
    n = (wnd->height - 0x2a) / 8 + ((wnd->height - 0x2a) % 8 != 0);
    for (y = 0x13; n > 0; n--, y += 8)
        ui_window_draw_bitmap(wnd, 0, y, res_mgr_get(g_resMgr(),
                            ui_skin_path(UI_SKIN "basic_interface\\itemwin_left.bmp")), 0);
    n = (wnd->height - 0x26) / 8 + ((wnd->height - 0x26) % 8 != 0);
    for (y = 0x11; n > 0; n--, y += 8) {
        bmp = res_mgr_get(g_resMgr(),
                           ui_skin_path(UI_SKIN "basic_interface\\itemwin_right.bmp"));
        ui_window_draw_bitmap(wnd, wnd->width - bmp->width, y, bmp, 0);
    }
    ui_window_fill_rect(wnd, 0x12, 0x11, 1, wnd->height - 0x15, chip_colour(0xe, 6));

    w = wnd->width;
    bmp = res_mgr_get(g_resMgr(),
                       ui_skin_path(UI_SKIN "basic_interface\\btnbar_left.bmp"));
    if (bmp) {
        bar_h = bmp->height;
        left_w = bmp->width;
        ui_window_draw_bitmap(wnd, 0, wnd->height - bar_h, bmp, 0);
    }
    bmp = res_mgr_get(g_resMgr(),
                       ui_skin_path(UI_SKIN "basic_interface\\btnbar_right.bmp"));
    if (bmp) {
        right_w = bmp->width;
        ui_window_draw_bitmap(wnd, w - right_w, wnd->height - bar_h, bmp, 0);
    }
    bmp = res_mgr_get(g_resMgr(),
                       ui_skin_path(UI_SKIN "basic_interface\\btnbar_mid.bmp"));
    if (bmp) {
        int span = w - right_w - left_w;
        int tile = bmp->width;
        n = span / tile + (span % tile != 0);
        for (x = left_w; n > 0; n--, x += tile)
            ui_window_draw_bitmap(wnd, x, wnd->height - bar_h, bmp, 0);
    }
    ui_window_draw_shadow_text(wnd, 0x11, 2, g_serviceType == 0xc ? "Lista de Comandos" : "Command List",
                            0, 0, 0xffffff, 0, 0xc, 0);

    tab = CMDLIST_TAB(self);
    shown = 0;
    if (tab == 2 || tab == 1) {
        struct Vec *v = tab == 2 ? &g_session1.windowNames : &g_session1.commandListCommands;
        struct CmdListEntry *it;
        for (it = v->first, i = 0; it != (struct CmdListEntry *)v->last; it++, i++) {
            int half, cx, rowy;
            if (i < self->scroll_row * self->columns)
                continue;
            half = (wnd->width - 0x28) / 2;
            cx = half * (shown % self->columns) + 0x19;
            rowy = shown / self->columns * CMDLIST_ROW_HEIGHT;
            if (self->hover == i) {
                ui_window_fill_rect(wnd, cx, shown / self->columns * CMDLIST_ROW_HEIGHT + 0x13, half, 0x14,
                                  chip_colour(6, 2));
                ui_window_fill_rect(wnd, cx + 1, (shown / self->columns + 1) * CMDLIST_ROW_HEIGHT, half - 2, 0x12,
                                  0xffffff);
            }
            ui_window_text_out(wnd, cx + 0xf, rowy + 0x16, it->text, 0, 0, 0xc, 0, 0, 0);
            if (++shown == self->rows * self->columns)
                return;
        }
    } else if (tab == 0) {
        for (i = 0; (unsigned)i < g_session1.talkOptionSettings.size; ++i) {
            const struct TalkOptionSetting *entry;
            const char *text;
            int value = 0;
            int half, cx, rowy;
            if (i < self->scroll_row * self->columns)
                continue;
            entry = talk_option_settings_at(&g_session1.talkOptionSettings, (unsigned)i);
            text = owned_text_cstr(&entry->text);
            talk_option_read(&entry->option, &value);
            half = (wnd->width - 0x28) / 2;
            cx = half * (shown % self->columns) + 0x19;
            rowy = shown / self->columns * CMDLIST_ROW_HEIGHT;
            if (self->hover == i) {
                ui_window_fill_rect(wnd, cx, shown / self->columns * CMDLIST_ROW_HEIGHT + 0x13, half, 0x14,
                                  chip_colour(6, 2));
                ui_window_fill_rect(wnd, cx + 1, (shown / self->columns + 1) * CMDLIST_ROW_HEIGHT, half - 2, 0x12,
                                  0xffffff);
            }
            ui_window_text_out(wnd, cx + 0x19, rowy + 0x16, text, 0, 0, 0xc, 0, 0, 0);
            ui_window_draw_bitmap(wnd, cx + 2, rowy + 0x16, res_mgr_get(g_resMgr(), ui_skin_path(value
                ? UI_SKIN "basic_interface\\grp_online.bmp"
                : UI_SKIN "basic_interface\\grp_offline.bmp")), 1);
            if (++shown == self->rows * self->columns)
                return;
        }
    }
}

static void update_hover(struct UICommandListWnd *self, int x, int y)
{
    int old = self->hover;
    self->hover = ui_command_list_wnd_entry_at(self, x, y);
    if (old != self->hover)
        ui_window_invalidate_base(&self->frame.base);
}

void ui_command_list_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UICommandListWnd *self = (struct UICommandListWnd *)window;
    struct UIWindow *other;
    int tab;

    if (y < CMDLIST_TITLE_HEIGHT) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    update_hover(self, x, y);
    if (self->hover == -1)
        return;
    tab = CMDLIST_TAB(self);
    if (tab == 2) {
        if ((unsigned)self->hover >= VEC_COUNT(&g_session1.windowNames, struct CmdListEntry))
            return;
        ui_window_mgr_on_key_down(&g_windowMgr, ((struct CmdListEntry *)g_session1.windowNames.first)[self->hover].value, 0, 10000);
    } else if (tab == 1) {
        if ((unsigned)self->hover >= VEC_COUNT(&g_session1.commandListCommands, struct CmdListEntry))
            return;
        mode_send_message(client_current_mode(), 0x2a, ((struct CmdListEntry *)g_session1.commandListCommands.first)[self->hover].value, 0, 0, 0);
    } else if (tab == 0) {
        const struct TalkOptionSetting *entry = talk_option_settings_at(&g_session1.talkOptionSettings, (unsigned)self->hover);
        if (!entry) return;
        mode_send_message(client_current_mode(), 0x2a, entry->option.type, 0, 0, 0);
    }
    ui_window_invalidate_base(&self->frame.base);
    other = ui_window_find(0x131);
    if (other)
        ui_window_send_msg(other, NULL, 0x3c, 0, 0, 0, 0);
}

void ui_command_list_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UICommandListWnd *self = (struct UICommandListWnd *)window;
    if (y < CMDLIST_TITLE_HEIGHT)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
}

void ui_command_list_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UICommandListWnd *self = (struct UICommandListWnd *)window;
    struct String tip;
    int tx, ty;

    update_hover(self, x, y);
    string_init(&tip);
    if (CMDLIST_TAB(self) == 2) {
        const char *text;
        if (self->hover == -1 || VEC_COUNT(&g_session1.altKeyNames, const char *) <= (unsigned)self->hover)
            goto done;
        mode_set_cursor_action(client_current_mode(), 2);
        text = ((const char **)g_session1.altKeyNames.first)[self->hover];
        string_assign_cstr(&tip, text);
    } else {
        struct String name;
        char *help = NULL;
        int arg = -1;
        int tab;

        string_init(&name);
        tab = CMDLIST_TAB(self);
        if (tab == 1) {
            const char *text;
            if (self->hover == -1 || (int)VEC_COUNT(&g_session1.commandListCommands, struct CmdListEntry) <= self->hover) {
                string_free(&name);
                goto done;
            }
            mode_set_cursor_action(client_current_mode(), 2);
            text = ((struct CmdListEntry *)g_session1.commandListCommands.first)[self->hover].text;
            string_assign_cstr(&name, text);
        } else if (tab == 0) {
            const struct TalkOptionSetting *entry = talk_option_settings_at(&g_session1.talkOptionSettings, (unsigned)self->hover);
            if (!entry) {
                string_free(&name);
                goto done;
            }
            string_assign_cstr(&name, owned_text_cstr(&entry->text));
            if (entry->option.type == CMDLIST_TALK_SHOW_NAME)
                talk_option_read(&entry->option, &arg);
            mode_set_cursor_action(client_current_mode(), 2);
        }

        client_lua_call(g_session1.luaState, "l_GetCmdHelpMsg", "sd>s", string_cstr(&name), arg, &help);
        if (help)
            string_assign_cstr(&tip, help);
        string_free(&name);
    }
    if (tip.size) {
        tx = ((x - 0x28) / CMDLIST_ROW_HEIGHT + 2) * CMDLIST_ROW_HEIGHT;
        ty = (y - CMDLIST_TITLE_HEIGHT) / CMDLIST_ROW_HEIGHT * CMDLIST_ROW_HEIGHT + 1;
        ui_window_to_screen(&self->frame.base, &tx, &ty);
        mode_show_tooltip(client_current_mode(), string_cstr(&tip), tx, ty, 0xffffff, 0);
    }
done:
    string_free(&tip);
}

void ui_command_list_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UICommandListWnd *self = (struct UICommandListWnd *)window;
    int step;

    if (!self->has_scroll)
        return;
    step = self->rows > 3 ? 3 : self->rows - 1;
    if (delta >= 0)
        step = -step;
    ui_window_send_msg_base(&self->scroll->base, &self->scroll->base, 7, step, 0, 0, 0);
}

int ui_command_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                        int val1, int val2, int val3, int val4)
{
    struct UICommandListWnd *self = (struct UICommandListWnd *)window;
    int old, pos;

    switch (msg) {
    case 6:
        if (val1 == 0xc1)
            ui_window_mgr_post_quit1(&g_windowMgr, CMDLIST_QUIT_ID);
        return 0;
    case 0x16:
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    case 7:
    case 9:
    case 0xa:
        old = self->scroll_row;
        if (msg == 9)
            pos = old - self->rows + 1;
        else if (msg == 0xa)
            pos = self->rows - 1 + old;
        else
            pos = val1 + old;
        self->scroll_row = pos;
        if (old != pos) {
            ui_command_list_wnd_update_scroll(self);
            ui_window_invalidate_base(&self->frame.base);
            pos = self->scroll_row;
        }
        return pos - old;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
}

void ui_command_list_wnd_destroy(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    self->base.ops = &ui_command_list_wnd_ops;
    ui_frame_wnd_destruct(self);
    mem_free(self);
}
