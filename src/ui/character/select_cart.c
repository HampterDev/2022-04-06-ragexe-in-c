#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/controls/controls.h"
#include "ui/ui_items.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/wndmgr/make_window_internal.h"

#define SELECT_CART_WND_ID 0x5f
#define MSG_SELECT_CART_TITLE 0x293
#define MSG_SELECT_CART_SET UI_MSG_SET_INFO
#define REQ_CHANGE_CART 0x9a
#define CART_BUTTON_ID_BASE 9000
#define CART_ROW_HEIGHT 0x62
#define CARTS_PER_ROW 5
#define CART_SPECIAL_KIND 0xd

struct UISelectCartWnd {
    struct UIFrameWnd frame;
    int value;
    struct Vec kinds;
};

void ui_select_cart_wnd_add_cart(struct UISelectCartWnd *self, int index, int kind);

static const int cart_levels[9] = { 0, 40, 65, 80, 90, 100, 110, 120, 130 };

static const char *const cart_names[14] = {
    "NOTHING_CART",
    "손수레",
    "손수레" "1",
    "손수레" "2",
    "손수레" "3",
    "손수레" "4",
    "손수레" "5",
    "손수레" "6",
    "손수레" "7",
    "손수레" "8",
    "선물상자카트",
    "포링싣은카트",
    "포링카트",
    "마도카트",
};

struct String *select_cart_cart_name(struct String *out, int kind)
{
    string_init(out);
    string_assign_cstr(out, cart_names[kind]);
    return out;
}

#define CART_SPRITE_DIR "이팩트\\"
#define CART_CELL_WIDTH 0x46
#define CART_CELL_HEIGHT 0x52
#define CART_CELL_TOP 0x1c

void ui_select_cart_wnd_add_cart(struct UISelectCartWnd *self, int index, int kind)
{
    struct String name, path;
    struct UIWindow *button;
    int x = index % CARTS_PER_ROW * CART_CELL_WIDTH;
    int y = index / CARTS_PER_ROW * CART_CELL_HEIGHT + CART_CELL_TOP;

    select_cart_cart_name(&name, kind);
    switch (kind) {
    case 1: case 2: case 10: y -= 0xd; break;
    case 4: x += 0x12; y -= 0xa; break;
    case 5: x += 0xf; y -= 5; break;
    case 6: case 7: case 8: case 9: x += 0xd; break;
    case 11: x += 0x10; y -= 0xd; break;
    case 12: y -= 0x17; break;
    case 13: x += 4; y -= 0xa; break;
    }
    button = ui_sprite_button_construct(mem_alloc(sizeof(struct UISpriteButton)));
    string_init(&path);
    string_format(&path, CART_SPRITE_DIR "%s.spr", string_cstr(&name));
    ui_sprite_button_set_sprite(button, string_cstr(&path));
    string_free(&path);
    string_format(&path, CART_SPRITE_DIR "%s.act", string_cstr(&name));
    ui_sprite_button_set_act(button, string_cstr(&path));
    string_free(&path);
    ui_window_create(button, 100, 100);
    ui_window_move(button, x, y);
    ((struct UISpriteButton *)button)->action = 0;
    ((struct UISpriteButton *)button)->motion = 0;
    button->id = kind + CART_BUTTON_ID_BASE;
    ui_window_add_child(&self->frame.base, button);
    string_free(&name);
}

static void select_cart_fit(struct UISelectCartWnd *self, int count)
{
    if (count)
        ui_window_resize_base(&self->frame.base, self->frame.base.width, ((count - 1) / CARTS_PER_ROW + 1) * CART_ROW_HEIGHT);
}

static void select_cart_add_all(struct UISelectCartWnd *self)
{
    int *kind;
    int i = 0;

    for (kind = self->kinds.first; kind != self->kinds.last; kind++, i++)
        ui_select_cart_wnd_add_cart(self, i, *kind);
}

void ui_select_cart_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_select_cart_wnd_on_draw(struct UIWindow *self);
void ui_select_cart_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
intptr_t ui_select_cart_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
void ui_select_cart_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps ui_select_cart_wnd_ops = {
    .destroy = ui_select_cart_wnd_destroy,
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
    .on_create = ui_select_cart_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_select_cart_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_select_cart_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_select_cart_wnd_send_msg,
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

struct UISelectCartWnd *ui_select_cart_wnd_construct(struct UISelectCartWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_select_cart_wnd_ops;
    self->value = 0;
    self->kinds.first = self->kinds.last = self->kinds.end = NULL;
    return self;
}

struct UIWindow *ui_select_cart_wnd_new(void)
{
    return (struct UIWindow *)ui_select_cart_wnd_construct(mem_alloc(sizeof(struct UISelectCartWnd)));
}

void ui_select_cart_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UISelectCartWnd *self = (struct UISelectCartWnd *)window;
    int level, i;

    ui_window_add_close_button(&self->frame.base, "", 0);
    self->kinds.last = self->kinds.first;
    level = g_session1.charBaseLevel;
    for (i = 1; i <= 9 && level > cart_levels[i - 1]; i++)
        vec_push(&self->kinds, &i, sizeof i);
    if (level > 100)
        *(int *)vec_add(&self->kinds, 1, sizeof(int)) = CART_SPECIAL_KIND;
    select_cart_add_all(self);
    select_cart_fit(self, (int)VEC_COUNT(&self->kinds, int));
}

void ui_select_cart_wnd_on_draw(struct UIWindow *window)
{
    struct UISelectCartWnd *self = (struct UISelectCartWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct BitmapRes *corner;
    int top = ui_window_draw_title_bar(wnd, 1, msg_str(MSG_SELECT_CART_TITLE), 0);

    ui_window_draw_body(wnd, top, 0, "");
    corner = ui_skin_bitmap(UI_SKIN "sysboxs_ld.bmp");
    ui_window_draw_bitmap(wnd, 0, wnd->height - corner->height, corner, 0);
    corner = ui_skin_bitmap(UI_SKIN "sysboxs_rd.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - corner->width, wnd->height - corner->height, corner, 0);
}

void ui_select_cart_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISelectCartWnd *self = (struct UISelectCartWnd *)window;
    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
}

intptr_t ui_select_cart_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                       intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UISelectCartWnd *self = (struct UISelectCartWnd *)window;
    if (msg == MSG_SELECT_CART_SET) {
        int i;

        ui_window_destroy_children(&self->frame.base);
        ui_window_add_close_button(&self->frame.base, "", 0);
        self->kinds.last = self->kinds.first;
        self->value = val3;
        for (i = 0; i < val2; i++)
            *(int *)vec_add(&self->kinds, 1, sizeof(int)) = ((const signed char *)val1)[i];
        select_cart_add_all(self);
        select_cart_fit(self, val2);
        return 0;
    }
    if (msg == UI_MSG_COMMAND) {
        if (val1 != UI_ID_CLOSE) {
            int kind = val1 - CART_BUTTON_ID_BASE;
            int *p;

            for (p = self->kinds.first; p != self->kinds.last && *p != kind; p++)
                ;

            if (p == self->kinds.last)
                return 0;
            game_send_message(REQ_CHANGE_CART, kind, self->value, 0, 0);
        }
        ui_window_mgr_post_quit1(&g_windowMgr, SELECT_CART_WND_ID);
    }
    return 0;
}

static void select_cart_destruct(struct UISelectCartWnd *self)
{
    self->frame.base.ops = &ui_select_cart_wnd_ops;
    vec_free(&self->kinds);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_select_cart_wnd_destroy(struct UIWindow *window)
{
    struct UISelectCartWnd *self = (struct UISelectCartWnd *)window;
    select_cart_destruct(self);
    mem_free(self);
}
