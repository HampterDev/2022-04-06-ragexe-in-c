#include "ui/controls/buttons.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "ui/items/styling_shop.h"
#include "core/msg_str.h"
#include "diag/perf.h"

struct UIBasicStylingWnd;
void ui_basic_styling_wnd_destroy(struct UIWindow *window);
void ui_basic_styling_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_basic_styling_wnd_on_draw(struct UIWindow *self);
int ui_basic_styling_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_basic_styling_wnd_ops = {
    .destroy = ui_basic_styling_wnd_destroy,
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
    .on_create = ui_basic_styling_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_basic_styling_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_basic_styling_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIStylingTab;
void ui_slot_of_my_style_wnd_destroy(struct UIWindow *window);
void ui_slot_of_my_style_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_slot_of_my_style_wnd_on_draw(struct UIWindow *self);
int ui_slot_of_my_style_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_slot_of_my_style_wnd_ops = {
    .destroy = ui_slot_of_my_style_wnd_destroy,
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
    .on_create = ui_slot_of_my_style_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_slot_of_my_style_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_slot_of_my_style_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

void ui_stylingshop_tab_parent_wnd_on_create(struct UIWindow *self, int width, int height)
{
    (void)self;
    (void)width;
    (void)height;
}

void ui_stylingshop_tab_parent_wnd_on_draw(struct UIWindow *self)
{
    (void)self;
}

int ui_stylingshop_tab_parent_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                                 int val1, int val2, int val3, int val4)
{
    (void)self;
    (void)sender;
    (void)msg;
    (void)val1;
    (void)val2;
    (void)val3;
    (void)val4;
    return 0;
}

void ui_basic_styling_wnd_destroy(struct UIWindow *window)
{
    struct UIBasicStylingWnd *self = (struct UIBasicStylingWnd *)window;
    self->tab.base.ops = &ui_basic_styling_wnd_ops;
    vec_free(&self->palette);
    self->tab.base.ops = &ui_stylingshop_tab_parent_wnd_ops;
    ui_window_destruct(&self->tab.base);
    mem_free(self);
}

static struct UIBitmapButton *style_new_shop_button(void)
{
    struct UIBitmapButton *btn = mem_try_alloc(sizeof *btn);

    if (btn) {
        ui_bitmap_button_construct(btn);
        btn->button.base.ops = &ui_styling_shop_button_ops.base;
    }
    return btn;
}

void ui_basic_styling_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIBasicStylingWnd *self = (struct UIBasicStylingWnd *)window;
    struct UIWindow *wnd = &self->tab.base;
    struct UIDisplayCharacter *display;
    struct UIShowInfoWnd *info;
    struct UIBitmapButton *btn;
    struct String name;
    int count, i, kind;

    (void)width;
    (void)height;
    display = mem_try_alloc(sizeof *display);
    if (display)
        display = ui_display_character_construct(display);
    self->tab.display = display;
    ui_window_create(&display->base, (int)(wnd->width * 0.33), (int)(wnd->height * 0.6));
    ui_window_move_base(&self->tab.display->base, 0, 0);
    ui_window_add_child(wnd, &self->tab.display->base);

    count = 0;
    style_kind_name(&name, STYLE_HEAD_PALETTE);
    style_lua_size(string_cstr(&name), &count);
    string_free(&name);
    for (i = 0; i < count; i++) {
        char off[0x40], off_over[0x40], on[0x40];

        sprintf_s(off, sizeof off, STYLE_SKIN "color%02d_off.bmp", i + 1);
        sprintf_s(off_over, sizeof off_over, STYLE_SKIN "color%02d_off_over.bmp", i + 1);
        sprintf_s(on, sizeof on, STYLE_SKIN "color%02d_on.bmp", i + 1);
        btn = style_new_shop_button();
        ui_bitmap_button_set_bitmap(btn, off, UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(btn, off_over, UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(btn, on, UI_BTN_DOWN, 0);
        ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
        ui_window_move_base(&btn->button.base, i % 4 * (btn->bitmap_width + 4) + 0x28,
                 i / 4 * (btn->bitmap_height + 4) + 0x86);
        btn->button.base.id = STYLE_ID_PALETTE;
        ui_window_add_child(wnd, &btn->button.base);
        vec_push(&self->palette, &btn, sizeof btn);
    }

    btn = style_new_shop_button();
    ui_bitmap_button_set_bitmap(btn, STYLE_SKIN "color00_off.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(btn, STYLE_SKIN "color00_on.bmp", UI_BTN_DOWN, 0);
    ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
    ui_window_move_base(&btn->button.base, 0xb, 0x98);
    btn->button.base.id = STYLE_ID_PALETTE;
    ui_window_add_child(wnd, &btn->button.base);
    self->no_colour = btn;

    ui_get_window(WND_STYLING_SHOP);

    info = mem_try_alloc(sizeof *info);
    if (info)
        info = ui_show_info_wnd_construct(info);
    self->tab.info = info;
    ui_window_create(&info->base, 0xc2, 0xb6);
    kind = STYLE_TAB_HAIR;
    ui_show_info_wnd_setup(self->tab.info, &kind);
    ui_window_move_base(&self->tab.info->base, 0x8e, 0xd);
    self->tab.info->display = self->tab.display;
    ui_window_add_child(wnd, &self->tab.info->base);
}

void ui_basic_styling_wnd_on_draw(struct UIWindow *window)
{
    struct UIBasicStylingWnd *self = (struct UIBasicStylingWnd *)window;
    struct UIWindow *wnd = &self->tab.base;
    struct BitmapRes *bitmap;

    ui_window_clear_canvas(wnd, 0xffffffff);
    bitmap = ui_skin_bitmap(STYLE_SKIN "line_dotted_2px.bmp");
    if (bitmap)
        ui_window_draw_bitmap(wnd, 0x77, 0, bitmap, 0);
    bitmap = ui_skin_bitmap(STYLE_SKIN "bg_styling_color.bmp");
    if (bitmap)
        ui_window_draw_bitmap(wnd, 4, 0x81, bitmap, 0);
}

int ui_basic_styling_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                         int val1, int val2, int val3, int val4)
{
    struct UIBasicStylingWnd *self = (struct UIBasicStylingWnd *)window;
    struct UIWindow *wnd = &self->tab.base;

    if (msg != UI_MSG_COMMAND)
        return ui_window_send_msg_base(wnd, sender, msg, val1, val2, val3, val4);
    if (val1 == STYLE_ID_PALETTE) {
        int count = 0, i;
        BYTE none = 1;

        style_lua_size("HeadPalette", &count);
        for (i = 0; i < count; i++) {
            struct UIWindow *btn = VEC_AT(&self->palette, struct UIWindow *, i);

            if (sender == btn) {
                BYTE ok = 1;

                none = 0;
                if (style_is_doram() == 1)
                    client_lua_call(g_session1.luaState, "StylingShop_GetIsColoring_by_goods", "d>b",
                            self->tab.display->head_style, &ok);
                if (ok) {
                    self->tab.display->head_palette = i + 1;
                    ui_window_invalidate_base(&self->tab.display->base);
                } else {
                    ui_window_mgr_message_box(&g_windowMgr, msg_str(0xb12), UI_MSGBOX_OK, 0, 0, 0, "Message",
                                           0x118, 0x78, 0);
                }
            } else {
                ui_window_set_state(btn, UI_BTN_UP);
            }
        }
        if (!none) {
            ui_window_set_state(&self->no_colour->button.base, UI_BTN_UP);
        } else if (sender == &self->no_colour->button.base) {
            self->tab.display->head_palette = 0;
            ui_window_invalidate_base(&self->tab.display->base);
        }
        ui_window_invalidate_base(wnd);
        ui_window_invalidate_base(&self->tab.info->base);
    }
    ui_window_invalidate_base(wnd);
    return 0;
}

void ui_slot_of_my_style_wnd_destroy(struct UIWindow *window)
{
    struct UIStylingTab *self = (struct UIStylingTab *)window;
    self->base.ops = &ui_slot_of_my_style_wnd_ops;
    self->base.ops = &ui_stylingshop_tab_parent_wnd_ops;
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_slot_of_my_style_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIStylingTab *self = (struct UIStylingTab *)window;
    struct UIWindow *wnd = &self->base;
    struct UIDisplayCharacter *display;
    struct UIShowInfoWnd *info;
    int kind;

    (void)width;
    (void)height;
    display = mem_try_alloc(sizeof *display);
    if (display)
        display = style_display_construct(display, 1);
    self->display = display;
    ui_window_create(&display->base, wnd->width, (int)(wnd->height * 0.6));
    ui_window_move_base(&self->display->base, 0, 5);
    self->display->draw_x = 0x1e;
    self->display->draw_y = 0x4c;
    ui_window_add_child(wnd, &self->display->base);

    info = mem_try_alloc(sizeof *info);
    if (info)
        info = ui_show_info_wnd_construct(info);
    self->info = info;
    ui_window_create(&info->base, wnd->width, (int)(wnd->height * 0.4));
    kind = STYLE_TAB_MY_STYLE;
    ui_show_info_wnd_setup(self->info, &kind);
    ui_window_move_base(&self->info->base, 0, (int)(wnd->height * 0.6 + 5.0));
    self->info->display = self->display;
    ui_window_add_child(wnd, &self->info->base);
}

void ui_slot_of_my_style_wnd_on_draw(struct UIWindow *self)
{
    (void)self;
}

int ui_slot_of_my_style_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                          int val2, int val3, int val4)
{
    struct UIStylingTab *self = (struct UIStylingTab *)window;
    (void)sender;
    (void)val1;
    (void)val2;
    (void)val3;
    (void)val4;
    if (msg == UI_MSG_COMMAND)
        ui_window_invalidate_base(&self->base);
    return 0;
}

void ui_slot_for_select_wnd_destroy(struct UIWindow *self)
{
    self->ops = &ui_slot_for_select_wnd_ops;
    ui_window_destruct(self);
    mem_free(self);
}

void ui_slot_for_select_wnd_on_l_btn_down(struct UIWindow *self, int x, int y)
{
    ui_window_send_msg(self->parent, NULL, UI_MSG_COMMAND, self->id, x, y, 0);
}

void ui_slot_for_select_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y)
{
    (void)x;
    (void)y;
    if (self->parent)
        ui_window_send_msg(self->parent, NULL, STYLE_MSG_MY_STYLE_APPLY, 0, 0, 0, 0);
}
