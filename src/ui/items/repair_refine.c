#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/items/items.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define ITEM_REPAIR_WND_ID 0x6b
#define WEAPON_REFINE_WND_ID 0x6f

#define MSG_ITEM_REPAIR_TITLE 0x32c
#define MSG_WEAPON_REFINE_TITLE 0x38e

#define GAME_REQ_REPAIR_ITEM 0xae
#define GAME_REQ_REFINE_WEAPON 0xb6

#define UI_MSG_ADD_ITEM 0x1f

struct UIItemPickWnd {
    struct UIFrameWnd frame;
    struct UIListBox *list;
    struct Vec item_ids;
    struct Vec indices;
};

static void pick_wnd_create(struct UIItemPickWnd *self, int width, int height)
{
    struct UIListBox *list = mem_alloc(sizeof(struct UIListBox));

    self->list = list ? ui_list_box_construct(list) : NULL;
    ui_list_box_create_window(self->list, 0xc, 0x16, width - 0x18, height - 0x37, 0);
    self->list->r = 0xf0;
    self->list->g = 0xf0;
    self->list->b = 0xf0;
    self->list->base.id = UI_ID_OK;
    ui_window_add_child(&self->frame.base, &self->list->base);

    ui_add_skin_button(&self->frame.base, "btn_ok", width - 0x5b, height - 0x18, UI_ID_OK);
    ui_add_skin_button(&self->frame.base, "btn_cancel", width - 0x2e, height - 0x18, UI_ID_CANCEL);
    UI_FRAME_DEFAULT_ID(self) = UI_ID_OK;
}

static void pick_wnd_draw(struct UIItemPickWnd *self, int title_msg)
{
    struct UIWindow *wnd = &self->frame.base;
    int title_height = ui_window_draw_title_bar(wnd, 1, msg_str(title_msg), 0);

    ui_window_draw_body(wnd, title_height, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

static void pick_wnd_add_line(struct UIItemPickWnd *self, int item_id, int index)
{
    struct ItemInfo info;
    struct String name = {0};

    vec_push(&self->item_ids, &item_id, sizeof item_id);
    vec_push(&self->indices, &index, sizeof index);
    item_info_construct(&info);
    item_info_set_item_id(&info, item_id);
    item_info_get_display_name(&info, &name);
    ui_list_box_append_text(self->list, string_cstr(&name));
    string_free(&name);
    item_info_destruct(&info);
}

static int pick_wnd_ok_selected(struct UIItemPickWnd *self, int request, int wnd_id)
{
    if (ui_list_box_count(self->list) == 0) {
        mode_send_message(g_modeMgr.curMode, request, -1, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, wnd_id);
        ui_window_mgr_post_quit1(&g_windowMgr, wnd_id);
        return -1;
    }
    return self->list->selected;
}

SIZE_ASSERT(RepairItem, 0x18);

struct UIItemRepairWnd;
void ui_item_repair_wnd_destroy(struct UIWindow *window);
void ui_item_repair_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_item_repair_wnd_on_draw(struct UIWindow *self);
int ui_item_repair_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_item_repair_wnd_ops = {
    .destroy = ui_item_repair_wnd_destroy,
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
    .on_create = ui_item_repair_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_repair_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
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
    .send_msg = ui_item_repair_wnd_send_msg,
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

struct UIItemRepairWnd {
    struct UIItemPickWnd pick;
    struct List items;
};

struct UIItemRepairWnd *ui_item_repair_wnd_construct(struct UIItemRepairWnd *self)
{
    ui_frame_wnd_construct(&self->pick.frame, 0);
    self->pick.frame.base.ops = &ui_item_repair_wnd_ops;
    memset(&self->pick.item_ids, 0, sizeof self->pick.item_ids);
    memset(&self->pick.indices, 0, sizeof self->pick.indices);
    list_init(&self->items);
    return self;
}

struct UIWindow *ui_item_repair_wnd_new(void)
{
    return (struct UIWindow *)ui_item_repair_wnd_construct(mem_alloc(sizeof(struct UIItemRepairWnd)));
}

static void ui_item_repair_wnd_destruct(struct UIItemRepairWnd *self)
{
    self->pick.frame.base.ops = &ui_item_repair_wnd_ops;
    list_free(&self->items, NULL);
    vec_free(&self->pick.indices);
    vec_free(&self->pick.item_ids);
    ui_frame_wnd_destruct(&self->pick.frame);
}

void ui_item_repair_wnd_destroy(struct UIWindow *window)
{
    struct UIItemRepairWnd *self = (struct UIItemRepairWnd *)window;
    ui_item_repair_wnd_destruct(self);
    mem_free(self);
}

void ui_item_repair_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemRepairWnd *self = (struct UIItemRepairWnd *)window;
    pick_wnd_create(&self->pick, width, height);
}

void ui_item_repair_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemRepairWnd *self = (struct UIItemRepairWnd *)window;
    pick_wnd_draw(&self->pick, MSG_ITEM_REPAIR_TITLE);
}

static void repair_add_item(struct UIItemRepairWnd *self, const struct RepairItem *item)
{
    list_push_back(&self->items, item, sizeof *item);
    pick_wnd_add_line(&self->pick, item->item_id, item->index);
}

static void repair_ok(struct UIItemRepairWnd *self)
{
    struct ListNode *node;
    int selected = pick_wnd_ok_selected(&self->pick, GAME_REQ_REPAIR_ITEM, ITEM_REPAIR_WND_ID);
    int index;

    if (selected < 0)
        return;
    node = self->items.head->next;
    if (node != self->items.head) {
        index = ((int *)self->pick.indices.first)[selected];
        while (LIST_VALUE(node, struct RepairItem)->index != index) {
            node = node->next;
            if (node == self->items.head) {
                ui_window_mgr_post_quit1(&g_windowMgr, ITEM_REPAIR_WND_ID);
                return;
            }
        }
        mode_send_message(g_modeMgr.curMode, GAME_REQ_REPAIR_ITEM, (int)LIST_VALUE(node, struct RepairItem), 0, 0, 0);
    }
    ui_window_mgr_post_quit1(&g_windowMgr, ITEM_REPAIR_WND_ID);
}

int ui_item_repair_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UIItemRepairWnd *self = (struct UIItemRepairWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            repair_ok(self);
        } else if (val1 == UI_ID_CANCEL) {
            mode_send_message(g_modeMgr.curMode, GAME_REQ_REPAIR_ITEM, -1, 0, 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, ITEM_REPAIR_WND_ID);
        }
        return 0;
    case UI_MSG_BAR_PAGE_UP:
    case UI_MSG_BAR_PAGE_DOWN:
        ui_window_send_msg(&self->pick.list->base, NULL, msg, 0, 0, 0, 0);
        return 0;
    case UI_MSG_ADD_ITEM:
        repair_add_item(self, (const struct RepairItem *)val1);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->pick.frame), sender, msg, val1, val2, val3, val4);
}

struct RefineRecord {
    int item_id;
    int index;
};

struct UIWeaponRefineWnd;
void ui_weapon_refine_wnd_destroy(struct UIWindow *window);
void ui_weapon_refine_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_weapon_refine_wnd_on_draw(struct UIWindow *self);
int ui_weapon_refine_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_weapon_refine_wnd_ops = {
    .destroy = ui_weapon_refine_wnd_destroy,
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
    .on_create = ui_weapon_refine_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_weapon_refine_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
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
    .send_msg = ui_weapon_refine_wnd_send_msg,
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

struct UIWeaponRefineWnd {
    struct UIItemPickWnd pick;
};

struct UIWeaponRefineWnd *ui_weapon_refine_wnd_construct(struct UIWeaponRefineWnd *self)
{
    ui_frame_wnd_construct(&self->pick.frame, 0);
    self->pick.frame.base.ops = &ui_weapon_refine_wnd_ops;
    memset(&self->pick.item_ids, 0, sizeof self->pick.item_ids);
    memset(&self->pick.indices, 0, sizeof self->pick.indices);
    return self;
}

struct UIWindow *ui_weapon_refine_wnd_new(void)
{
    return (struct UIWindow *)ui_weapon_refine_wnd_construct(mem_alloc(sizeof(struct UIWeaponRefineWnd)));
}

static void ui_weapon_refine_wnd_destruct(struct UIWeaponRefineWnd *self)
{
    self->pick.frame.base.ops = &ui_weapon_refine_wnd_ops;
    vec_free(&self->pick.indices);
    vec_free(&self->pick.item_ids);
    ui_frame_wnd_destruct(&self->pick.frame);
}

void ui_weapon_refine_wnd_destroy(struct UIWindow *window)
{
    struct UIWeaponRefineWnd *self = (struct UIWeaponRefineWnd *)window;
    ui_weapon_refine_wnd_destruct(self);
    mem_free(self);
}

void ui_weapon_refine_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIWeaponRefineWnd *self = (struct UIWeaponRefineWnd *)window;
    pick_wnd_create(&self->pick, width, height);
}

void ui_weapon_refine_wnd_on_draw(struct UIWindow *window)
{
    struct UIWeaponRefineWnd *self = (struct UIWeaponRefineWnd *)window;
    pick_wnd_draw(&self->pick, MSG_WEAPON_REFINE_TITLE);
}

static void refine_ok(struct UIWeaponRefineWnd *self)
{
    int selected = pick_wnd_ok_selected(&self->pick, GAME_REQ_REFINE_WEAPON,
                                        WEAPON_REFINE_WND_ID);

    if (selected < 0)
        return;
    mode_send_message(g_modeMgr.curMode, GAME_REQ_REFINE_WEAPON, ((int *)self->pick.indices.first)[selected], 0, 0, 0);
    ui_window_mgr_post_quit1(&g_windowMgr, WEAPON_REFINE_WND_ID);
}

int ui_weapon_refine_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UIWeaponRefineWnd *self = (struct UIWeaponRefineWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            refine_ok(self);
        } else if (val1 == UI_ID_CANCEL) {
            mode_send_message(g_modeMgr.curMode, GAME_REQ_REFINE_WEAPON, -1, 0, 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, WEAPON_REFINE_WND_ID);
        }
        return 0;
    case UI_MSG_BAR_PAGE_UP:
    case UI_MSG_BAR_PAGE_DOWN:
        ui_window_send_msg(&self->pick.list->base, NULL, msg, 0, 0, 0, 0);
        return 0;
    case UI_MSG_ADD_ITEM:
        pick_wnd_add_line(&self->pick, val1, val2);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->pick.frame), sender, msg, val1, val2, val3, val4);
}
