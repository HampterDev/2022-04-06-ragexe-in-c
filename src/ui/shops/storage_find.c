#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/shops/storage_internal.h"
#include "ui/controls/controls.h"
#include "diag/perf.h"
#include "ui/items/items.h"

struct UIItemStoreFindWnd;
void ui_item_store_find_wnd_destroy(struct UIWindow *window);
void ui_item_store_find_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_item_store_find_wnd_store_info(struct UIWindow *self);
void ui_item_store_find_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_item_store_find_wnd_on_draw(struct UIWindow *self);
void ui_item_store_find_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_item_store_find_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_item_store_find_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_item_store_find_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_item_store_find_wnd_on_wheel(struct UIWindow *self, int delta);
intptr_t ui_item_store_find_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_item_store_find_wnd_ops = {
    .destroy = ui_item_store_find_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_store_find_wnd_drag_and_drop,
    .store_info = ui_item_store_find_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_store_find_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_store_find_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_store_find_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_store_find_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_store_find_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_store_find_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_store_find_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_store_find_wnd_send_msg,
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

struct UIItemStoreFindWnd *ui_item_store_find_wnd_construct(struct UIItemStoreFindWnd *self)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_item_store_find_wnd_ops;
    string_init(&self->count_text);
    string_init(&self->search);
    self->info = NULL;
    self->base.scroll_bar = NULL;
    self->base.scroll = 0;
    self->base.columns = 1;
    self->base.rows = 3;
    self->base.scrollable = 0;
    self->line_height = 0x12;
    return self;
}

struct UIWindow *ui_item_store_find_wnd_new(void)
{
    return (struct UIWindow *)ui_item_store_find_wnd_construct(mem_alloc(sizeof(struct UIItemStoreFindWnd)));
}

void ui_item_store_find_wnd_destruct(struct UIItemStoreFindWnd *self)
{
    self->base.frame.base.ops = &ui_item_store_find_wnd_ops;
    string_free(&self->search);
    string_free(&self->count_text);
    ui_item_base_wnd_destruct(&self->base);
}

void ui_item_store_find_wnd_destroy(struct UIWindow *window)
{
    struct UIItemStoreFindWnd *self = (struct UIItemStoreFindWnd *)window;
    ui_item_store_find_wnd_destruct(self);
    mem_free(self);
}

void ui_item_store_find_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemStoreFindWnd *self = (struct UIItemStoreFindWnd *)window;
    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->base.frame.base, x, y, drag);
        return;
    }
    game_send_message(MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

void ui_item_store_find_wnd_store_info(struct UIWindow *window)
{
    struct UIItemStoreFindWnd *self = (struct UIItemStoreFindWnd *)window;
    storage_store_info(&self->base.frame.base, self->info);
}

void ui_item_store_find_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemStoreFindWnd *self = (struct UIItemStoreFindWnd *)window;
    (void)height;
    storage_list_create(&self->base, &self->title, &self->resizer);
    self->close = ui_window_add_close_button(&self->base.frame.base, "", 0);
    ui_window_move_base(self->close, width - 0x10, 3);
}

void ui_item_store_find_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemStoreFindWnd *self = (struct UIItemStoreFindWnd *)window;
    storage_list_draw(&self->base, &self->count_text);
}

void ui_item_store_find_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreFindWnd *self = (struct UIItemStoreFindWnd *)window;
    storage_on_lbtn_down(&self->base, x, y, 0x14);
}

void ui_item_store_find_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreFindWnd *self = (struct UIItemStoreFindWnd *)window;
    (void)x, (void)y;
    ui_item_store_find_wnd_send_msg(&self->base.frame.base, NULL, UI_MSG_COMMAND, 0xe0, 0, 0, 0);
}

void ui_item_store_find_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreFindWnd *self = (struct UIItemStoreFindWnd *)window;
    storage_on_mouse_hover(&self->base, x, y, 0x14);
}

void ui_item_store_find_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreFindWnd *self = (struct UIItemStoreFindWnd *)window;
    storage_on_r_btn_down(&self->base, x, y, 0x14);
}

void ui_item_store_find_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIItemStoreFindWnd *self = (struct UIItemStoreFindWnd *)window;
    storage_on_wheel(&self->base, delta);
}

void storage_find_set_search(struct UIItemStoreFindWnd *self, struct String text)
{
    string_assign(&self->search, string_cstr(&text), text.size);
    string_free(&text);
}

intptr_t ui_item_store_find_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                          intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIItemStoreFindWnd *self = (struct UIItemStoreFindWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CLOSE) {
            struct UIItemStoreWnd *store;
            ui_window_mgr_post_quit1(&g_windowMgr, STORE_FIND_WND_ID);
            store = (struct UIItemStoreWnd *)ui_window_find(STORE_WND_ID);
            if (store)
                store->find_wnd = NULL;
            return 0;
        }
    case UI_MSG_RESIZE:
        if (val1 == UI_EDGE_BOTTOM_RIGHT)
            storage_list_resize(&self->base, self->resizer, self->close, val2, val3);
        return 0;
    case STORE_MSG_REFILL:
        storage_fill(&self->base, -1);
        if (g_session1.itemSort[1])
            storage_sort(&self->base, 0);
        if (g_session1.itemSort[2])
            storage_sort(&self->base, 1);
        if (self->search.size)
            storage_filter(&self->base, &self->search, 1);
        break;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!storage_list_info_usable(self->info)) {
            self->info->x = 0x1fe;
            self->info->y = 0x168;
            self->info->width = 0x118;
            self->info->height = 0x138;
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
        ui_item_store_find_wnd_send_msg(wnd, NULL, STORE_MSG_REFILL, 0, 0, 0, 0);
        return 0;
    case STORE_MSG_SORT:
        if (g_session1.itemSort[0] == 1) {
            ui_item_store_find_wnd_send_msg(wnd, NULL, STORE_MSG_UNSORT, 0, 0, 0, 0);
            return 0;
        }
        self->sorting = 1;
        if (g_session1.itemSort[1] == 1)
            storage_sort(&self->base, 0);
        else if (g_session1.itemSort[2] == 1)
            storage_sort(&self->base, 1);
        break;
    case STORE_MSG_SEARCH:
        if (!self->search.size || !storage_filter(&self->base, &self->search, 1))
            return 0;
        break;
    default:
        return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
    }
    ui_window_invalidate_base(wnd);
    ui_item_base_wnd_update_scroll(&self->base);
    return 0;
}

void ui_item_store_find_wnd_set_title(struct UIItemStoreFindWnd *self, struct String title)
{
    ui_static_text_set_text_ellipsis(self->title, string_cstr(&title));
    string_free(&title);
}
