#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/shops/storage_internal.h"
#include "diag/perf.h"
#include "ui/items/items.h"

struct UIItemStoreSubWnd;
void ui_item_store_sub_wnd_destroy(struct UIWindow *window);
void ui_item_store_sub_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_item_store_sub_wnd_store_info(struct UIWindow *self);
void ui_item_store_sub_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_item_store_sub_wnd_on_draw(struct UIWindow *self);
void ui_item_store_sub_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_item_store_sub_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_item_store_sub_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_item_store_sub_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_item_store_sub_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_item_store_sub_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_item_store_sub_wnd_ops = {
    .destroy = ui_item_store_sub_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_store_sub_wnd_drag_and_drop,
    .store_info = ui_item_store_sub_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_store_sub_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_store_sub_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_store_sub_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_store_sub_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_store_sub_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_store_sub_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_store_sub_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_store_sub_wnd_send_msg,
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

static const int s_subWndPos[STORE_TABS][2] = {
    { 0xd2, 0x32 }, { 0x3c, 0x32 }, { 0xd2, 0xcd }, { 0x3c, 0xcd },
    { 0xd2, 0x168 }, { 0x3c, 0x168 }, { 0xd2, 0x203 }, { 0x3c, 0x203 },
};

struct UIItemStoreSubWnd *ui_item_store_sub_wnd_construct(struct UIItemStoreSubWnd *self)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_item_store_sub_wnd_ops;
    string_init(&self->count_text);
    string_init(&self->search);
    self->info = NULL;
    self->base.scroll_bar = NULL;
    self->base.scroll = 0;
    self->base.columns = 1;
    self->base.rows = 3;
    self->base.scrollable = 0;
    self->title_height = 0x12;
    return self;
}

struct UIWindow *ui_item_store_sub_wnd_new(void)
{
    return (struct UIWindow *)ui_item_store_sub_wnd_construct(mem_alloc(sizeof(struct UIItemStoreSubWnd)));
}

void ui_item_store_sub_wnd_destruct(struct UIItemStoreSubWnd *self)
{
    self->base.frame.base.ops = &ui_item_store_sub_wnd_ops;
    string_free(&self->search);
    string_free(&self->count_text);
    ui_item_base_wnd_destruct(&self->base);
}

void ui_item_store_sub_wnd_destroy(struct UIWindow *window)
{
    struct UIItemStoreSubWnd *self = (struct UIItemStoreSubWnd *)window;
    ui_item_store_sub_wnd_destruct(self);
    mem_free(self);
}

void ui_item_store_sub_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemStoreSubWnd *self = (struct UIItemStoreSubWnd *)window;
    storage_drop(&self->base, x, y, drag);
}

void ui_item_store_sub_wnd_store_info(struct UIWindow *window)
{
    struct UIItemStoreSubWnd *self = (struct UIItemStoreSubWnd *)window;
    storage_store_info(&self->base.frame.base, self->info);
}

void ui_item_store_sub_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemStoreSubWnd *self = (struct UIItemStoreSubWnd *)window;
    (void)width, (void)height;
    storage_list_create(&self->base, &self->title, &self->resizer);
}

void ui_item_store_sub_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemStoreSubWnd *self = (struct UIItemStoreSubWnd *)window;
    storage_list_draw(&self->base, &self->count_text);
}

void ui_item_store_sub_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreSubWnd *self = (struct UIItemStoreSubWnd *)window;
    storage_on_lbtn_down(&self->base, x, y, 0x14);
}

void ui_item_store_sub_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreSubWnd *self = (struct UIItemStoreSubWnd *)window;
    (void)x, (void)y;
    ui_item_store_sub_wnd_send_msg(&self->base.frame.base, NULL, UI_MSG_COMMAND, 0xe0, 0, 0, 0);
}

void ui_item_store_sub_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreSubWnd *self = (struct UIItemStoreSubWnd *)window;
    storage_on_mouse_hover(&self->base, x, y, 0x14);
}

void ui_item_store_sub_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreSubWnd *self = (struct UIItemStoreSubWnd *)window;
    storage_on_r_btn_down(&self->base, x, y, 0x14);
}

void ui_item_store_sub_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIItemStoreSubWnd *self = (struct UIItemStoreSubWnd *)window;
    storage_on_wheel(&self->base, delta);
}

int ui_item_store_sub_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UIItemStoreSubWnd *self = (struct UIItemStoreSubWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    switch (msg) {
    case UI_MSG_RESIZE:
        if (val1 == UI_EDGE_BOTTOM_RIGHT)
            storage_list_resize(&self->base, self->resizer, NULL, val2, val3);
        return 0;
    case STORE_MSG_REFILL:
        storage_fill(&self->base, self->category);
        storage_sort_chosen(&self->base);
        if (self->search.size && !storage_filter(&self->base, &self->search, 0))
            return 0;
        break;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!storage_list_info_usable(self->info)) {
            self->info->width = 0x8c;
            self->info->height = 0x91;
            self->info->x = s_subWndPos[self->category][0];
            self->info->y = s_subWndPos[self->category][1];
        }
        storage_apply_info(wnd, self->info);
        return 0;
    case UI_MSG_ADD_DRAGGED:
        storage_add_dragged();
        return 0;
    case STORE_MSG_COUNT:
        storage_set_count_text(wnd, &self->count_text, val1, val2);
        return 0;
    case STORE_MSG_UNSORT:
        self->sorting = 0;
        ui_item_store_sub_wnd_send_msg(wnd, NULL, STORE_MSG_REFILL, 0, 0, 0, 0);
        return 0;
    case STORE_MSG_SORT:
        if (g_session1.itemSort[0] == 1) {
            ui_item_store_sub_wnd_send_msg(wnd, NULL, STORE_MSG_UNSORT, 0, 0, 0, 0);
            return 0;
        }
        self->sorting = 1;
        if (g_session1.itemSort[1] == 1)
            storage_sort(&self->base, 0);
        else if (g_session1.itemSort[2] == 1)
            storage_sort(&self->base, 1);
        break;
    case STORE_MSG_SEARCH:
        if (!self->search.size || !storage_filter(&self->base, &self->search, 0))
            return 0;
        break;
    default:
        return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
    }
    ui_window_invalidate_base(wnd);
    ui_item_base_wnd_update_scroll(&self->base);
    return 0;
}

void ui_item_store_sub_wnd_set_category(struct UIItemStoreSubWnd *self, int category)
{
    self->category = category;
}

void ui_item_store_sub_wnd_set_title(struct UIItemStoreSubWnd *self, struct String title)
{
    ui_static_text_set_text_ellipsis(self->title, string_cstr(&title));
    string_free(&title);
}
