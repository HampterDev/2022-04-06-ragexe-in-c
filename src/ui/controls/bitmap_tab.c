#include "ui/controls/tabs.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include "core/text_util.h"
#include "diag/perf.h"

struct UIBitmapTab;
struct UIWindow;
const struct UIWindowOps ui_bitmap_tab_ops = {
    .destroy = ui_bitmap_tab_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_bitmap_tab_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_bitmap_tab_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_bitmap_tab_on_l_btn_down,
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
    .send_msg = ui_bitmap_tab_send_msg,
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

#define BITMAP_TAB_IDLE 'a'
#define BITMAP_TAB_SELECTED 'b'

static struct BitmapRes *bitmap_tab_bitmap(const char *format, int c)
{
    struct String path;
    struct BitmapRes *bmp;

    string_init(&path);
    string_format(&path, format, c);
    bmp = ui_skin_bitmap(string_cstr(&path));
    string_free(&path);
    return bmp;
}

static void tabs_clear(struct Vec *tabs)
{
    struct IntString *tab;

    for (tab = tabs->first; tab != tabs->last; tab++)
        string_free(&tab->text);
    vec_clear(tabs);
}

void ui_bitmap_tab_destroy(struct UIWindow *window)
{
    struct UIBitmapTab *self = (struct UIBitmapTab *)window;
    tree_free(&self->tab_lookup, NULL);
    tabs_clear(&self->tabs);
    vec_free(&self->tabs);
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_bitmap_tab_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIBitmapTab *self = (struct UIBitmapTab *)window;
    if (self->base.parent)
        ui_window_drag_and_drop(self->base.parent, x, y, drag);
}

void ui_bitmap_tab_on_draw(struct UIWindow *window)
{
    struct UIBitmapTab *self = (struct UIBitmapTab *)window;
    unsigned i;
    int y = 0;

    for (i = 0; i < VEC_COUNT(&self->tabs, struct IntString); i++) {
        struct IntString *tab = &VEC_AT(&self->tabs, struct IntString, i);
        int c = self->base.state == (int)i ? BITMAP_TAB_SELECTED : BITMAP_TAB_IDLE;
        struct BitmapRes *bmp = bitmap_tab_bitmap(string_cstr(&tab->text), c);

        if (bmp) {
            ui_window_draw_bitmap(&self->base, 0, y, bmp, 0);
            y += bmp->height;
        }
    }
}

void ui_bitmap_tab_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIBitmapTab *self = (struct UIBitmapTab *)window;
    unsigned i;
    int bottom = 0;

    if (!self->base.parent)
        return;
    for (i = 0; i < VEC_COUNT(&self->tabs, struct IntString); i++) {
        bottom += VEC_AT(&self->tabs, struct IntString, i).key;
        if (y < bottom) {
            if (self->base.state != (int)i) {
                self->base.state = i;
                ui_window_invalidate_base(&self->base);
                ui_window_send_msg(self->base.parent, &self->base, UI_MSG_TAB_CHANGED, self->base.state, 0, 0, 0);
            }
            return;
        }
    }
}

int ui_bitmap_tab_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                   int val3, int val4)
{
    struct UIBitmapTab *self = (struct UIBitmapTab *)window;
    switch (msg) {
    case UI_MSG_SET_STATE:
        self->base.state = val1;
        ui_window_invalidate_base(&self->base);
        break;
    case BITMAP_TAB_MSG_CLEAR:
        tabs_clear(&self->tabs);
        break;
    case BITMAP_TAB_MSG_ADD: {
        const char *format = (const char *)val1;
        struct BitmapRes *bmp;
        struct IntString tab;

        if (!format)
            break;
        bmp = bitmap_tab_bitmap(format, BITMAP_TAB_IDLE);
        if (!bmp)
            break;
        tab.key = bmp->height;
        string_init(&tab.text);
        string_assign_cstr(&tab.text, format);
        vec_push(&self->tabs, &tab, sizeof tab);
        break;
    }
    case BITMAP_TAB_MSG_FIT: {
        unsigned i;
        int width = 0, height = 0;

        if (self->tabs.first == self->tabs.last)
            break;
        for (i = 0; i < VEC_COUNT(&self->tabs, struct IntString); i++) {
            struct IntString *tab = &VEC_AT(&self->tabs, struct IntString, i);
            struct BitmapRes *bmp = bitmap_tab_bitmap(string_cstr(&tab->text), BITMAP_TAB_IDLE);

            if (bmp) {
                width = bmp->width;
                height += bmp->height;
            }
        }
        ui_window_resize(&self->base, width, height);
        break;
    }
    }
    return 0;
}
