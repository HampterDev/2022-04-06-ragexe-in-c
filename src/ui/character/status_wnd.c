#include "world/tooltip.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "ui/misc/snap_mgr.h"
#include "contents/guild_info.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include <string.h>
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/wndmgr/make_window_internal.h"

#define STATUS_WND_ID 0xb

#define ID_PLUS_STR 0xc5

#define STATUS_MSG_REFRESH 0x23

#define REQ_STATUS_CHANGE 0x1e

#define STAT_COUNT 6
#define TITLE_HEIGHT 0x11
#define STATUS_WIDTH 0x118
#define STATUS_HEIGHT 0x8d
#define OFF_SCREEN (-100)

#define MSG_STATUS_TITLE 0x69

struct UIStatusWnd {
    struct UIFrameWnd frame;
    struct UIBitmapButton *plus[STAT_COUNT];
    int full_height;
    struct StatusWndInfo *info;
    int title_height;
    int tip_dy;
    int tip_dx;
};
OFFSET_ASSERT(UIStatusWnd, plus, 0x94);

void ui_status_wnd_store_info(struct UIWindow *window)
{
    struct UIStatusWnd *self = (struct UIStatusWnd *)window;
    struct StatusWndInfo *info = self->info;

    if (!info)
        return;
    info->rect.width = self->frame.base.width;
    self->info->rect.height = self->frame.base.height;
    self->info->rect.x = self->frame.base.x;
    self->info->rect.y = self->frame.base.y;
    self->info->org_height = self->full_height;
}

static struct UIBitmapButton *status_make_plus(struct UIStatusWnd *self, struct UIBitmapButton **slot,
                                               struct UIBitmapButton *const *row, const char *path, int y, int id)
{
    void *mem = mem_alloc(sizeof(struct UIBitmapButton));
    struct UIBitmapButton *button = mem ? ui_bitmap_button_construct(mem) : NULL;

    *slot = button;
    ui_bitmap_button_set_bitmap(button, path, 0, 0);
    ui_bitmap_button_set_bitmap(*slot, path, 1, 0);
    ui_bitmap_button_set_bitmap(*slot, path, 2, 0);
    ui_window_create(&(*slot)->button.base, row[0]->bitmap_width, row[0]->bitmap_height);
    ui_window_move_base(&(*slot)->button.base, 0x4e, y);
    (*slot)->button.base.id = id;
    ui_window_add_child(&self->frame.base, &(*slot)->button.base);
    return button;
}

struct UIWindow;
void ui_status_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_status_wnd_on_draw(struct UIWindow *self);
void ui_status_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_status_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_status_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
int ui_status_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_status_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps ui_status_wnd_ops = {
    .destroy = ui_status_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_status_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_status_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_status_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_status_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_status_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_status_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_status_wnd_send_msg,
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

struct UIStatusWnd *ui_status_wnd_construct(struct UIStatusWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_status_wnd_ops;
    self->frame.base.state = 0;
    self->info = NULL;
    self->title_height = 0x11;
    self->tip_dy = 0xb;
    self->tip_dx = 0x10;
    return self;
}

struct UIWindow *ui_status_wnd_new(void)
{
    return (struct UIWindow *)ui_status_wnd_construct(mem_alloc(sizeof(struct UIStatusWnd)));
}

void ui_status_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIStatusWnd *self = (struct UIStatusWnd *)window;
    static const char arrow[] = UI_SKIN "basic_interface\\arw_right.bmp";
    struct UIWindow *wnd = &self->frame.base;
    int i;

    (void)width;
    (void)height;
    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "Alt+A", 0);
    ui_window_add_mini_button(wnd, "Ctrl+A", 0);

    for (i = 0; i < STAT_COUNT; i++)
        status_make_plus(self, &self->plus[i], self->plus, arrow, 0x18 + 0x10 * i, ID_PLUS_STR + i);
    self->full_height = wnd->height;
}

static void status_text(struct UIStatusWnd *self, int x, int y, int font_height, DWORD colour, const char *fmt,
                        int value)
{
    struct String text;

    string_init(&text);
    string_format(&text, fmt, value);
    ui_window_text_out(&self->frame.base, x, y, string_cstr(&text), 0, 1, font_height, colour, 0, 0);
    string_free(&text);
}

static void status_text_right(struct UIStatusWnd *self, int x, int y, const char *fmt, int value)
{
    struct String text;

    string_init(&text);
    string_format(&text, fmt, value);
    ui_window_text_out_right(&self->frame.base, x, y, string_cstr(&text), 0, 1, 0xe, 0, 0);
    string_free(&text);
}

static void status_pair_right(struct UIStatusWnd *self, int x, int y, int a, int b)
{
    struct String text;

    string_init(&text);
    string_format(&text, "%d + %d", a, b);
    ui_window_text_out_right(&self->frame.base, x, y, string_cstr(&text), 0, 1, 0xe, 0, 0);
    string_free(&text);
}

static void status_bonus(struct UIStatusWnd *self, int y, int value, int bonus, const char *fmt)
{
    if (value < 100)
        status_text(self, 0x31, y, 0xe, 0, fmt, bonus);
    else
        status_text(self, 0x36, y + 2, 0xc, 0, fmt, bonus);
}

void ui_status_wnd_on_draw(struct UIWindow *window)
{
    struct UIStatusWnd *self = (struct UIStatusWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct {
        int base, bonus;
    } cells[STAT_COUNT];
    int costs[STAT_COUNT];
    const char *background;
    struct String text;
    int i, y;

    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_STATUS_TITLE), 0);
    if (wnd->height == self->title_height || wnd->state != 0)
        return;
    background = UI_SKIN "statuswnd\\w_statwin_bg.bmp";
    ui_window_draw_bitmap(wnd, 0, self->title_height, ui_skin_bitmap(background), 1);

    for (i = 0; i < STAT_COUNT; i++) {
        cells[i].base = g_session1.stats[i];
        cells[i].bonus = g_session1.statBonus[i];
    }
    for (i = 0, y = 0x17; y < 0x77; i++, y += 0x10) {
        int value = cells[i].base, bonus = cells[i].bonus;

        status_text(self, 0x24, y, 0xe, 0, "%d", value);
        if (bonus > 0)
            status_bonus(self, y, value, bonus, "+%d");
        else if (bonus < 0)
            status_bonus(self, y, value, bonus, "%d");
    }

    for (i = 0; i < STAT_COUNT; i++)
        costs[i] = g_session1.statCosts[i];
    for (i = 0, y = 0x17; y < 0x77; i++, y += 0x10)
        status_text_right(self, 0x65, y, "%d", costs[i]);

    status_pair_right(self, 0xbd, 0x16, g_session1.atk, g_session1.refineAtk);
    status_pair_right(self, 0xbd, 0x26, g_session1.matkMin, g_session1.matkMax);
    status_text_right(self, 0xbd, 0x36, "%d", g_session1.hit);
    status_text_right(self, 0xbd, 0x46, "%d", g_session1.critical);
    status_pair_right(self, 0x113, 0x16, g_session1.def, g_session1.plusDef);
    status_pair_right(self, 0x113, 0x26, g_session1.mdef, g_session1.plusMdef);
    status_pair_right(self, 0x113, 0x36, g_session1.flee, g_session1.plusFlee);
    string_init(&text);
    string_format(&text, "%d", (2000 - g_session1.aspd) / 10);
    ui_window_text_out_right(wnd, 0x113, 0x46, string_cstr(&text), 0, 1, 0xe, 0, 0);
    string_free(&text);
    status_text_right(self, 0x113, 0x56, "%d", g_session1.statusPoints);
    if (g_guildInfo.name.size)
        ui_window_text_out_right(wnd, 0x113, 0x66, string_cstr(&g_guildInfo.name), 0, 0, 0xc, 0, 0);
}

void ui_status_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIStatusWnd *self = (struct UIStatusWnd *)window;
    if (y < self->title_height)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
}

void ui_status_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIStatusWnd *self = (struct UIStatusWnd *)window;
    (void)x;
    if (y < TITLE_HEIGHT)
        ui_status_wnd_send_msg(&self->frame.base, &self->frame.base, UI_MSG_COMMAND, UI_ID_HELP, 0, 0, 0);
}

struct StatusTipCell {
    int x, y;
};

static int status_tip(struct UIStatusWnd *self, const struct StatusTipCell *cells, const char *const *texts,
                      int count, int x, int y, int stat_rows)
{
    int i, tx, ty;

    for (i = 0; i < count; i++) {
        if (cells[i].x <= x && x <= cells[i].x + 0x50 && cells[i].y <= y && y <= cells[i].y + 0xc)
            break;
    }
    if (i == count)
        return 0;
    if (stat_rows) {
        tx = cells[i].x - self->tip_dx;
        ty = self->title_height - self->tip_dy - 0x13 + cells[i].y;
    } else {
        tx = cells[i].x - 0x10;
        ty = cells[i].y - 0xd;
    }
    ui_window_to_screen(&self->frame.base, &tx, &ty);
    mode_show_tooltip(client_current_mode(), texts[i], tx, ty, 0xffffffff, 1);
    return 1;
}

void ui_status_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIStatusWnd *self = (struct UIStatusWnd *)window;
    static const struct StatusTipCell stat_cells[16] = {
        { 6, 0x19 },    { 6, 0x29 },    { 6, 0x39 },    { 6, 0x49 },    { 6, 0x59 },    { 6, 0x69 },
        { 0x6c, 0x18 }, { 0x6c, 0x28 }, { 0x6c, 0x38 }, { 0x6c, 0x48 }, { 0x6c, 0x58 }, { 0x6c, 0x68 },
        { 0xc2, 0x18 }, { 0xc2, 0x28 }, { 0xc2, 0x38 }, { 0xc2, 0x48 },
    };
    static const int stat_msgs[16] = { 0xc28, 0xc29, 0xc2a, 0xc2b, 0xc2c, 0xc2d, 0xc2e, 0xc34,
                                       0xc30, 0xc31, 0xc33, 0xc32, 0xc2f, 0xc35, 0xc36, 0xc37 };
    const char *texts[16];
    int i;

    if (self->frame.base.state != 0)
        return;
    for (i = 0; i < 16; i++)
        texts[i] = msg_str(stat_msgs[i]);
    status_tip(self, stat_cells, texts, 16, x, y, 1);
}

static void status_place_buttons(struct UIStatusWnd *self)
{
    int i;

    for (i = 0; i < STAT_COUNT; i++) {
        int cost = g_session1.statCosts[i];

        if (cost != 0 && g_session1.statusPoints >= cost)
            ui_window_move_base(&self->plus[i]->button.base, 0x4e, 0x18 + 0x10 * i);
        else
            ui_window_move_base(&self->plus[i]->button.base, OFF_SCREEN, OFF_SCREEN);
    }
    ui_window_invalidate_base(&self->frame.base);
}

static void status_plus(struct UIStatusWnd *self, int stat)
{
    if (g_session1.statCosts[stat] > g_session1.statusPoints)
        return;
    mode_send_message(client_current_mode(), REQ_STATUS_CHANGE, 0xd + stat, 1, 0, 0);
    ui_status_wnd_send_msg(&self->frame.base, &self->frame.base, STATUS_MSG_REFRESH, 0, 0, 0, 0);
}

static void status_on_command(struct UIStatusWnd *self, int id)
{
    struct UIWindow *wnd = &self->frame.base;

    switch (id) {
    case UI_ID_CLOSE:
        ui_close_window(STATUS_WND_ID);
        break;
    case UI_ID_HELP: {
        int old_height = wnd->height;

        if (old_height == TITLE_HEIGHT) {
            ui_window_resize_base(wnd, wnd->width, self->full_height);
        } else {
            self->full_height = old_height;
            ui_window_resize_base(wnd, wnd->width, TITLE_HEIGHT);
        }
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        ui_window_mgr_raise_window(&g_windowMgr, wnd);
        if (wnd->height - old_height)
            snap_mgr_move_chain(&g_snapMgr, wnd, 0, wnd->height - old_height, 3);
        break;
    }
    case ID_PLUS_STR:
    case ID_PLUS_STR + 1:
    case ID_PLUS_STR + 2:
    case ID_PLUS_STR + 3:
    case ID_PLUS_STR + 4:
    case ID_PLUS_STR + 5:
        status_plus(self, id - ID_PLUS_STR);
        break;
    default:
        break;
    }
}

int ui_status_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                   int val3, int val4)
{
    struct UIStatusWnd *self = (struct UIStatusWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct StatusWndInfo *info;

    switch (msg) {
    case UI_MSG_COMMAND:
        status_on_command(self, val1);
        return 0;

    case UI_MSG_SET_INFO:
        info = (struct StatusWndInfo *)val1;
        self->info = info;
        if (!status_wnd_info_is_valid(info))
            status_wnd_info_make_default(self->info);
        info = self->info;
        ui_status_wnd_send_msg(wnd, NULL, UI_MSG_RESIZE, 7, info->rect.width, info->org_height, 0);
        info = self->info;
        if (info->rect.height != info->org_height)
            ui_status_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, UI_ID_HELP, 0, 0, 0);
        ui_status_wnd_send_msg(wnd, NULL, STATUS_MSG_REFRESH, 0, 0, 0, 0);
        ui_frame_wnd_move(wnd, self->info->rect.x, self->info->rect.y);
        ui_window_resize_base(wnd, STATUS_WIDTH, STATUS_HEIGHT);
        self->full_height = wnd->height;
        return 0;

    case STATUS_MSG_REFRESH:
        if (wnd->state == 0)
            status_place_buttons(self);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_status_wnd_destroy(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    self->base.ops = &ui_status_wnd_ops;
    ui_frame_wnd_destruct(self);
    mem_free(self);
}
