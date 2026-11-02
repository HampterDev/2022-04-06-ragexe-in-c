#include "ui/window_types.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/character/character.h"
#include "ui/character/popup_menu.h"
#include "diag/perf.h"

struct Vec;
struct UIMenuWnd;
struct UIWindow;
void ui_menu_wnd_destroy(struct UIWindow *window);
void ui_menu_wnd_move(struct UIWindow *self, int x, int y);
void ui_menu_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_menu_wnd_on_draw(struct UIWindow *self);
void ui_menu_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_menu_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_menu_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_menu_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_menu_wnd_ops = {
    .destroy = ui_menu_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_menu_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_menu_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_menu_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_menu_wnd_on_mouse_move,
    .on_mouse_hover = ui_menu_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_menu_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_menu_wnd_send_msg,
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

static const char menu_separator[] = "-";

#define WND_ID_MENU 0x12
#define MODE_REQ_MENU_CHOICE 0x18
#define MSG_MENU_CHOICE 0x30

#define MSG_MENU_FIT 0x1e
#define MSG_MENU_ADD_ITEM 0x1f
#define MSG_MENU_ADD_SEPARATOR 0x48
#define MSG_MENU_CLEAR 0x4b
#define MSG_MENU_SET_OWNER 0x53

struct UIMenuWnd {
    struct UIFrameWnd frame;
    int owner_id;
    int selected;
    int row_height2;
    struct Vec items;
    int owner_key;
    BYTE hold;
};

#define MENU_ITEMS(self) ((struct String *)(self)->items.first)
#define MENU_COUNT(self) ((int)VEC_COUNT(&(self)->items, struct String))

struct UIMenuWnd *ui_menu_wnd_construct(struct UIMenuWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_menu_wnd_ops;
    self->owner_id = MENU_OWNER_MODE;
    self->selected = -1;
    self->row_height2 = 0xe;
    self->items.first = NULL;
    self->items.last = NULL;
    self->items.end = NULL;
    self->owner_key = 0;
    self->hold = 0;
    return self;
}

struct UIWindow *ui_menu_wnd_new(void)
{
    return (struct UIWindow *)ui_menu_wnd_construct(mem_alloc(sizeof(struct UIMenuWnd)));
}

void ui_menu_wnd_destroy(struct UIWindow *window)
{
    struct UIMenuWnd *self = (struct UIMenuWnd *)window;
    self->frame.base.ops = &ui_menu_wnd_ops;
    string_vec_free(&self->items);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_menu_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UIMenuWnd *self = (struct UIMenuWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (y > g_renderer->height - wnd->height)
        y += -2 - wnd->height;
    if (x > g_renderer->width - wnd->width)
        x += -2 - wnd->width;
    ui_window_move_base(wnd, x, y);
}

void ui_menu_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused)
{
}

static COLORREF chip(int x, int y)
{
    int red, green, blue;

    ui_skin_color_chip(&g_skinMgr, x, y, &red, &green, &blue);
    return RGB((BYTE)red, (BYTE)green, (BYTE)blue);
}

void ui_menu_wnd_on_draw(struct UIWindow *window)
{
    struct UIMenuWnd *self = (struct UIMenuWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct BitmapRes *res;
    COLORREF highlight;
    int count, i;

    ui_window_clear_canvas(wnd, chip(6, 6));
    ui_window_fill_rect(wnd, 1, 1, wnd->width - 2, wnd->height - 2, chip(2, 2));
    res = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_lu.bmp");
    ui_window_draw_bitmap(wnd, 0, 0, res, 0);
    res = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_ld.bmp");
    ui_window_draw_bitmap(wnd, 0, wnd->height - res->height, res, 0);
    res = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_ru.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - res->width, 0, res, 0);
    res = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_rd.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - res->width, wnd->height - res->height, res, 0);

    highlight = chip(6, 2);
    count = MENU_COUNT(self);
    for (i = 0; i < count; i++) {
        struct String *item = &MENU_ITEMS(self)[i];
        int separator = string_equal_cstr(item, menu_separator);
        int first = !(i > 0 && string_equal(item - 1, item));
        int row = first ? i : i - 1;

        if (separator) {
            ui_window_fill_rect(wnd, 0, self->row_height2 * i / 2 + 2, wnd->width, 1, chip(0xe, 6));
            continue;
        }
        if (i == self->selected)
            ui_window_fill_rect(wnd, 3, self->row_height2 / 2 * row + 1, wnd->width - 6,
                              self->row_height2 - 2, highlight);
        ui_window_text_out(wnd, 3, row * self->row_height2 / 2 + 1, string_cstr(item), 0, 0, 12, 0, 0,
                         0);
    }
}

static int item_at(struct UIMenuWnd *self, int x, int y)
{
    struct UIWindow *wnd = &self->frame.base;
    int row, last;

    if (x < 0 || x >= wnd->width || y < 0 || y >= wnd->height)
        return -1;
    row = y / (self->row_height2 / 2);
    if (row < 0)
        row = 0;
    last = MENU_COUNT(self) - 1;
    return (unsigned)row <= (unsigned)last ? row : last;
}

void ui_menu_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIMenuWnd *self = (struct UIMenuWnd *)window;
    ui_menu_wnd_on_mouse_hover(&self->frame.base, x, y);
}

void ui_menu_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIMenuWnd *self = (struct UIMenuWnd *)window;
    int before = self->selected;

    self->selected = item_at(self, x, y);
    if (before != self->selected)
        ui_window_invalidate_base(&self->frame.base);
}

void ui_menu_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIMenuWnd *self = (struct UIMenuWnd *)window;
    int chosen = item_at(self, x, y);
    int count, last, separators, index, i;
    struct UIWindow *owner;

    self->selected = chosen;
    if (chosen == -1 || string_equal_cstr(&MENU_ITEMS(self)[chosen], menu_separator))
        return;
    count = MENU_COUNT(self);
    separators = 0;
    if (count) {
        last = chosen < count ? chosen : count - 1;
        for (i = 0; i < last; i++)
            if (string_equal_cstr(&MENU_ITEMS(self)[i], menu_separator))
                separators++;
    }
    index = (chosen - separators) / 2;
    if (self->owner_id == MENU_OWNER_BY_KEY)
        owner = ui_window_mgr_find_by_key(&g_windowMgr, self->owner_key);
    else
        owner = ui_window_find(self->owner_id);
    if (owner)
        ui_window_send_msg(owner, NULL, MSG_MENU_CHOICE, index, 0, 0, 0);
    else if (self->owner_id == MENU_OWNER_MODE)
        mode_send_message(g_modeMgr.curMode, MODE_REQ_MENU_CHOICE, index, 0, 0, 0);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_MENU);
}

static void add_item(struct UIMenuWnd *self, const char *text)
{
    string_vec_push_text(&self->items, text, strlen(text));
}

int ui_menu_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                 int val3, int val4)
{
    struct UIMenuWnd *self = (struct UIMenuWnd *)window;
    int width, count, i;

    switch (msg) {
    case MSG_MENU_FIT:
        count = MENU_COUNT(self);
        width = 0x14;
        for (i = 0; i < count; i++) {
            int text_width = ui_window_text_width(&self->frame.base, string_cstr(&MENU_ITEMS(self)[i]), 0, 0,
                                                12, 0, 0);
            if (width <= text_width)
                width = text_width;
        }
        ui_window_resize_base(&self->frame.base, width + 6, self->row_height2 * count / 2);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    case MSG_MENU_ADD_ITEM:
        add_item(self, (const char *)val1);
        add_item(self, (const char *)val1);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    case MSG_MENU_ADD_SEPARATOR:
        add_item(self, menu_separator);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    case MSG_MENU_CLEAR:
        string_vec_clear(&self->items);
        return 0;
    case MSG_MENU_SET_OWNER:
        self->owner_id = val1;
        self->owner_key = val2;
        return 0;
    }
    return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
}

int ui_menu_owner_id(const struct UIWindow *menu)
{
    return ((const struct UIMenuWnd *)menu)->owner_id;
}

int ui_menu_owner_key(const struct UIWindow *menu)
{
    return ((const struct UIMenuWnd *)menu)->owner_key;
}

int ui_menu_take_hold(struct UIWindow *menu)
{
    struct UIMenuWnd *self = (struct UIMenuWnd *)menu;

    if (!self->hold)
        return 0;
    self->hold = 0;
    return 1;
}

void ui_menu_set_hold(struct UIWindow *menu)
{
    ((struct UIMenuWnd *)menu)->hold = 1;
}
