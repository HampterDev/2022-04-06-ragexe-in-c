#include "render/rp_pool.h"
#include "skills/skill_info.h"
#include "core/object_type_tables.h"
#include "ui/items/item_list.h"
#include <stdlib.h>
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_names.h"

struct UIPetEggListWnd {
    struct UIItemIdentifyWnd list;
};

struct Vec;
struct UIWindow;
void ui_pet_egg_list_wnd_destroy(struct UIWindow *window);
void ui_pet_egg_list_wnd_on_draw(struct UIWindow *self);
intptr_t ui_pet_egg_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender_unused, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

const struct UIWindowOps ui_pet_egg_list_wnd_ops = {
    .destroy = ui_pet_egg_list_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_item_identify_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_identify_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_pet_egg_list_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_identify_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_identify_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_item_identify_wnd_on_r_btn_up,
    .on_wheel = ui_item_identify_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_pet_egg_list_wnd_send_msg,
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

struct UIPetEggListWnd *ui_pet_egg_list_wnd_construct(struct UIPetEggListWnd *self)
{
    ui_item_identify_wnd_construct(&self->list);
    self->list.frame.base.ops = &ui_pet_egg_list_wnd_ops;
    return self;
}

struct UIWindow *ui_pet_egg_list_wnd_new(void)
{
    return (struct UIWindow *)ui_pet_egg_list_wnd_construct(mem_alloc(sizeof(struct UIPetEggListWnd)));
}

static void ui_pet_egg_list_wnd_destruct(struct UIPetEggListWnd *self)
{
    self->list.frame.base.ops = &ui_pet_egg_list_wnd_ops;
    ui_item_identify_wnd_destruct(&self->list);
}

void ui_pet_egg_list_wnd_destroy(struct UIWindow *window)
{
    struct UIPetEggListWnd *self = (struct UIPetEggListWnd *)window;
    ui_pet_egg_list_wnd_destruct(self);
    mem_free(self);
}

void ui_pet_egg_list_wnd_on_draw(struct UIWindow *window)
{
    struct UIPetEggListWnd *self = (struct UIPetEggListWnd *)window;
    const char *title = msg_str(MSG_PET_EGG_LIST_TITLE);

    string_assign_cstr(&self->list.title, title);
    ui_item_identify_wnd_on_draw((struct UIWindow *)(&self->list));
}

intptr_t ui_pet_egg_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender_unused, int msg,
                                       intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIPetEggListWnd *self = (struct UIPetEggListWnd *)window;
    struct UIItemIdentifyWnd *list = &self->list;

    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            struct ItemInfo info;
            int found = 0;

            ui_item_identify_wnd_get_item(list, &info, list->selected, &found);
            if (found)
                mode_send_message(g_modeMgr.curMode, GAME_REQ_HATCH_EGG, info.index, 0, 0, 0);
            item_info_destruct(&info);
        } else if (val1 == UI_ID_CANCEL) {
            ui_window_mgr_post_quit1(&g_windowMgr, PET_EGG_LIST_WND_ID);
        }
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        return item_list_scroll(list, msg, val1);
    case UI_MSG_LIST_ADD:
        item_list_add_inventory_item(list, val2);
        return 0;
    case UI_MSG_LIST_CHANGED:
        item_list_update_scroll(list);
        ui_window_invalidate_base(&list->frame.base);
        return 0;
    }
    return 0;
}

struct UIMakingArrowListWnd {
    struct UIItemIdentifyWnd list;
    short kind;
};

void ui_making_arrow_list_wnd_destroy(struct UIWindow *window);
void ui_making_arrow_list_wnd_on_draw(struct UIWindow *self);
intptr_t ui_making_arrow_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender_unused, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

const struct UIWindowOps ui_making_arrow_list_wnd_ops = {
    .destroy = ui_making_arrow_list_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_item_identify_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_identify_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_making_arrow_list_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_identify_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_identify_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_item_identify_wnd_on_r_btn_up,
    .on_wheel = ui_item_identify_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_making_arrow_list_wnd_send_msg,
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

struct UIMakingArrowListWnd *ui_making_arrow_list_wnd_construct(struct UIMakingArrowListWnd *self)
{
    ui_item_identify_wnd_construct(&self->list);
    self->list.frame.base.ops = &ui_making_arrow_list_wnd_ops;
    return self;
}

struct UIWindow *ui_making_arrow_list_wnd_new(void)
{
    return (struct UIWindow *)ui_making_arrow_list_wnd_construct(mem_alloc(sizeof(struct UIMakingArrowListWnd)));
}

static void ui_making_arrow_list_wnd_destruct(struct UIMakingArrowListWnd *self)
{
    self->list.frame.base.ops = &ui_making_arrow_list_wnd_ops;
    ui_item_identify_wnd_destruct(&self->list);
}

void ui_making_arrow_list_wnd_destroy(struct UIWindow *window)
{
    struct UIMakingArrowListWnd *self = (struct UIMakingArrowListWnd *)window;
    ui_making_arrow_list_wnd_destruct(self);
    mem_free(self);
}

void ui_making_arrow_list_wnd_on_draw(struct UIWindow *window)
{
    struct UIMakingArrowListWnd *self = (struct UIMakingArrowListWnd *)window;
    string_assign_cstr(&self->list.title, "LIST");
    ui_item_identify_wnd_on_draw((struct UIWindow *)(&self->list));
}

static void making_request(struct UIMakingArrowListWnd *self, int item_id)
{
    if (self->kind == MAKING_KIND_ARROW)
        mode_send_message(g_modeMgr.curMode, GAME_REQ_MAKE_ARROW, item_id, 0, 0, 0);
    else
        mode_send_message(g_modeMgr.curMode, GAME_REQ_MAKE_ITEM, item_id, self->kind, 0, 0);
}

intptr_t ui_making_arrow_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender_unused,
                                            int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMakingArrowListWnd *self = (struct UIMakingArrowListWnd *)window;
    struct UIItemIdentifyWnd *list = &self->list;

    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            struct ItemInfo info;
            int found;

            ui_item_identify_wnd_get_item(list, &info, list->selected, &found);
            if (found) {
                making_request(self, item_info_get_item_id(&info));
                ui_window_mgr_post_quit1(&g_windowMgr, MAKING_ARROW_LIST_WND_ID);
            }
            item_info_destruct(&info);
        } else if (val1 == UI_ID_CANCEL) {
            making_request(self, -1);
            ui_window_mgr_post_quit1(&g_windowMgr, MAKING_ARROW_LIST_WND_ID);
        }
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        return item_list_scroll(list, msg, val1);
    case UI_MSG_LIST_ADD: {
        struct ItemInfo info;

        item_info_construct(&info);
        item_info_set_item_id(&info, *(const int *)val1);
        info.identified = 1;
        item_info_copy(list_push_back(&list->items, NULL, sizeof(struct ItemInfo)), &info);
        ui_making_arrow_list_wnd_send_msg(&list->frame.base, NULL, UI_MSG_LIST_CHANGED, 0, 0, 0, 0);
        item_info_destruct(&info);
        return 0;
    }
    case UI_MSG_LIST_SET_KIND:
        self->kind = (short)val1;
        return 0;
    case UI_MSG_LIST_CHANGED:
        item_list_update_scroll(list);
        ui_window_invalidate_base(&list->frame.base);
        return 0;
    case UI_MSG_LIST_CLEAR:
        list_clear(&list->items, item_info_destroy);
        return 0;
    }
    return 0;
}

struct UISpellListWnd {
    struct UIItemIdentifyWnd list;
    struct Vec skill_ids;
    short kind;
};

void ui_spell_list_wnd_destroy(struct UIWindow *window);
void ui_spell_list_wnd_on_draw(struct UIWindow *self);
intptr_t ui_spell_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender_unused, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_spell_list_wnd_ops = {
    .destroy = ui_spell_list_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_item_identify_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_identify_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_spell_list_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_identify_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_identify_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_item_identify_wnd_on_r_btn_up,
    .on_wheel = ui_item_identify_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_spell_list_wnd_send_msg,
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

struct UISpellListWnd *ui_spell_list_wnd_construct(struct UISpellListWnd *self)
{
    ui_item_identify_wnd_construct(&self->list);
    self->list.frame.base.ops = &ui_spell_list_wnd_ops;
    memset(&self->skill_ids, 0, sizeof self->skill_ids);
    self->kind = 0;
    return self;
}

struct UIWindow *ui_spell_list_wnd_new(void)
{
    return (struct UIWindow *)ui_spell_list_wnd_construct(mem_alloc(sizeof(struct UISpellListWnd)));
}

static void ui_spell_list_wnd_destruct(struct UISpellListWnd *self)
{
    self->list.frame.base.ops = &ui_spell_list_wnd_ops;
    self->skill_ids.last = self->skill_ids.first;
    vec_free(&self->skill_ids);
    ui_item_identify_wnd_destruct(&self->list);
}

void ui_spell_list_wnd_destroy(struct UIWindow *window)
{
    struct UISpellListWnd *self = (struct UISpellListWnd *)window;
    ui_spell_list_wnd_destruct(self);
    mem_free(self);
}

void ui_spell_list_wnd_on_draw(struct UIWindow *window)
{
    struct UISpellListWnd *self = (struct UISpellListWnd *)window;
    struct UIItemIdentifyWnd *list = &self->list;
    struct UIWindow *wnd = &list->frame.base;
    int title_height = ui_window_draw_title_bar(wnd, 1, msg_str(MSG_SPELL_LIST_TITLE), 0);
    struct SkillInfo skill;
    unsigned i;
    int k = 0, drawn = 0;

    ui_window_draw_body(wnd, title_height, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    skill_info_construct(&skill);
    for (i = 0; i < VEC_COUNT(&self->skill_ids, int); i++, k++) {
        int skill_id = ((int *)self->skill_ids.first)[i];
        char name[64];
        char icon[256];
        int y;

        strcpy(name, skill_name(skill_id));
        if (k < list->first_row * list->columns)
            continue;
        y = drawn / list->columns * LIST_ROW_HEIGHT + LIST_TOP;
        if (list->selected == k)
            ui_window_fill_rect(wnd, 5, y - 3, wnd->width - 0x18, 0x1e, ui_skin_colour(6, 2));
        skill.id = skill_id;
        session_format_item_bitmap_path(skill_info_get_id_name(&skill), icon);
        ui_window_draw_bitmap(wnd, 8, y, res_mgr_get(g_resMgr(), icon), 1);
        ui_window_text_out(wnd, 0x28, y + 4, name, 0, 0, 0xc, 0, 0, 0);
        if (++drawn == list->rows * list->columns)
            break;
    }
    skill_info_destruct(&skill);
}

static void spell_request(struct UISpellListWnd *self, int skill_id)
{
    if (self->kind == 0)
        mode_send_message(g_modeMgr.curMode, GAME_REQ_SPELL, skill_id, 0, 0, 0);
    else
        mode_send_message(g_modeMgr.curMode, GAME_REQ_SPELL_KIND, skill_id, self->kind, 0, 0);
}

intptr_t ui_spell_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender_unused, int msg,
                                      intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UISpellListWnd *self = (struct UISpellListWnd *)window;
    struct UIItemIdentifyWnd *list = &self->list;

    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            int selected = list->selected;

            if (selected >= 0 && (unsigned)selected < VEC_COUNT(&self->skill_ids, int)) {
                spell_request(self, ((int *)self->skill_ids.first)[selected]);
                ui_window_mgr_post_quit1(&g_windowMgr, SPELL_LIST_WND_ID);
            }
        } else if (val1 == UI_ID_CANCEL) {
            spell_request(self, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, SPELL_LIST_WND_ID);
        }
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        return item_list_scroll(list, msg, val1);
    case UI_MSG_SPELL_SET_KIND:
        self->kind = (short)val1;
        return 0;
    case UI_MSG_LIST_ADD: {
        struct ItemInfo row;

        vec_push(&self->skill_ids, &val1, sizeof val1);
        item_info_construct(&row);
        item_info_copy(list_push_back(&list->items, NULL, sizeof(struct ItemInfo)), &row);
        ui_spell_list_wnd_send_msg(&list->frame.base, NULL, UI_MSG_LIST_CHANGED, 0, 0, 0, 0);
        item_info_destruct(&row);
        return 0;
    }
    case UI_MSG_LIST_CHANGED:
        item_list_update_scroll(list);
        ui_window_invalidate_base(&list->frame.base);
        return 0;
    }
    return 0;
}

struct UIItemCompositionWnd {
    struct UIItemIdentifyWnd list;
    struct WindowInfo *info;
    int card_index;
};

void ui_item_composition_wnd_destroy(struct UIWindow *window);
void ui_item_composition_wnd_store_info(struct UIWindow *self);
void ui_item_composition_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_item_composition_wnd_on_draw(struct UIWindow *self);
intptr_t ui_item_composition_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

const struct UIWindowOps ui_item_composition_wnd_ops = {
    .destroy = ui_item_composition_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_item_composition_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_composition_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_composition_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_identify_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_identify_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_item_identify_wnd_on_r_btn_up,
    .on_wheel = ui_item_identify_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_composition_wnd_send_msg,
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

struct UIItemCompositionWnd *ui_item_composition_wnd_construct(struct UIItemCompositionWnd *self)
{
    ui_item_identify_wnd_construct(&self->list);
    self->list.frame.base.ops = &ui_item_composition_wnd_ops;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_item_composition_wnd_new(void)
{
    return (struct UIWindow *)ui_item_composition_wnd_construct(mem_alloc(sizeof(struct UIItemCompositionWnd)));
}

static void ui_item_composition_wnd_destruct(struct UIItemCompositionWnd *self)
{
    self->list.frame.base.ops = &ui_item_composition_wnd_ops;
    ui_item_identify_wnd_destruct(&self->list);
}

void ui_item_composition_wnd_destroy(struct UIWindow *window)
{
    struct UIItemCompositionWnd *self = (struct UIItemCompositionWnd *)window;
    ui_item_composition_wnd_destruct(self);
    mem_free(self);
}

void ui_item_composition_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemCompositionWnd *self = (struct UIItemCompositionWnd *)window;
    const char *title;

    ui_item_identify_wnd_on_create((struct UIWindow *)(&self->list), width, height);
    title = msg_str(MSG_ITEM_COMPOSITION_TITLE);
    string_assign_cstr(&self->list.title, title);
    self->card_index = 0;
}

void ui_item_composition_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemCompositionWnd *self = (struct UIItemCompositionWnd *)window;
    ui_item_identify_wnd_on_draw((struct UIWindow *)(&self->list));
}

void ui_item_composition_wnd_store_info(struct UIWindow *window)
{
    struct UIItemCompositionWnd *self = (struct UIItemCompositionWnd *)window;
    struct UIWindow *wnd = &self->list.frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}

static void composition_set_card(struct UIItemCompositionWnd *self, int card_index)
{
    struct ItemInfo card;
    struct String title;
    struct String name = {0};

    self->card_index = card_index;
    if (!card_index)
        return;
    session_get_inventory_item(&g_session1, &card, card_index);
    if ((unsigned)card.index > 0) {
        item_info_get_display_name(&card, &name);
        string_init(&title);
        string_format(&title, "%s (%s)", msg_str(MSG_ITEM_COMPOSITION_TITLE), string_cstr(&name));
        string_assign(&self->list.title, string_cstr(&title), title.size);
        string_free(&title);
        string_free(&name);
    }
    item_info_destruct(&card);
}

intptr_t ui_item_composition_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIItemCompositionWnd *self = (struct UIItemCompositionWnd *)window;
    struct UIItemIdentifyWnd *list = &self->list;

    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            if (list->selected != -1) {
                struct ItemInfo info;
                int found = val1;

                ui_item_identify_wnd_get_item(list, &info, list->selected, &found);
                if (found)
                    mode_send_message(g_modeMgr.curMode, GAME_REQ_COMPOSE_CARD, info.index, self->card_index, 0, 0);
                else
                    mode_send_message(g_modeMgr.curMode, GAME_REQ_COMPOSE_CARD, -1, -1, 0, 0);
                ui_window_mgr_post_quit1(&g_windowMgr, ITEM_COMPOSITION_WND_ID);
                item_info_destruct(&info);
            }
        } else if (val1 == UI_ID_CANCEL) {
            mode_send_message(g_modeMgr.curMode, GAME_REQ_COMPOSE_CARD, -1, -1, 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, ITEM_COMPOSITION_WND_ID);
        }
        return 0;
    case UI_MSG_SET_INFO:
        item_list_set_info(list, &self->info, (struct WindowInfo *)val1);
        return 0;
    case UI_MSG_COMPOSE_CARD:
        composition_set_card(self, val1);
        return 0;
    }
    return ui_item_identify_wnd_send_msg((struct UIWindow *)(list), sender, msg, val1, val2, val3, val4);
}
