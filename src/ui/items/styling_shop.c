#include "ui/controls/scroll.h"
#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "session/character_name.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "ui/items/styling_shop.h"
#include "platform/filesystem.h"
#include <stdlib.h>
#include "platform/code_page.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "core/cstr.h"

struct UIStylingTab;
struct UIWindow;
const struct UIWindowOps ui_stylingshop_tab_parent_wnd_ops = {
    .destroy = ui_stylingshop_tab_parent_wnd_destroy,
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
    .on_create = ui_stylingshop_tab_parent_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_stylingshop_tab_parent_wnd_on_draw,
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
    .send_msg = ui_stylingshop_tab_parent_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

const struct UIWindowOps ui_accessory_cus_wnd_ops = {
    .destroy = ui_accessory_cus_wnd_destroy,
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
    .on_create = ui_accessory_cus_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_accessory_cus_wnd_on_draw,
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
    .send_msg = ui_accessory_cus_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIAccessoryShowCase;
int ui_accessory_show_case_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_accessory_show_case_ops = {
    .destroy = ui_accessory_show_case_destroy,
    .resize = ui_scroll_wnd_resize,
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
    .on_create = ui_scroll_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_accessory_show_case_on_draw,
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
    .on_wheel = ui_scroll_wnd_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_accessory_show_case_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_scroll_wnd_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_scroll_wnd_make_quad,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UI2ndCostumeWnd;
const struct UIWindowOps ui2nd_costume_wnd_ops = {
    .destroy = ui2nd_costume_wnd_destroy,
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
    .on_create = ui2nd_costume_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui2nd_costume_wnd_on_draw,
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
    .send_msg = ui2nd_costume_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIMyStyleWnd;
const struct UIWindowOps ui_my_style_wnd_ops = {
    .destroy = ui_my_style_wnd_destroy,
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
    .on_create = ui_my_style_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_my_style_wnd_on_draw,
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
    .send_msg = ui_my_style_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIStylingShopWnd;
void ui_styling_shop_wnd_destroy(struct UIWindow *window);
void ui_styling_shop_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_styling_shop_wnd_on_process(struct UIWindow *self);
void ui_styling_shop_wnd_on_draw(struct UIWindow *self);
int ui_styling_shop_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_styling_shop_wnd_ops = {
    .destroy = ui_styling_shop_wnd_destroy,
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
    .on_create = ui_styling_shop_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_styling_shop_wnd_on_process,
    .on_draw = ui_styling_shop_wnd_on_draw,
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
    .send_msg = ui_styling_shop_wnd_send_msg,
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

#define STYLE_SAVE_FILE "savedata\\LikeListInfoFile.lua"
#define STYLE_SHADOW_ITEM_ON STYLE_SKIN "shadow_item_on.bmp"
#define STYLE_SHADOW_ITEM_OFF STYLE_SKIN "shadow_item_off.bmp"

BYTE my_style_all_ok(const struct UIMyStyleWnd *self)
{
    struct UIStylingTab *slot;

    if (self->selected >= MY_STYLE_SLOTS)
        return 0;
    slot = self->slots[self->selected];
    if (!slot)
        return 0;
    return slot->info->all_ok;
}

static BYTE style_same_look(const struct UIDisplayCharacter *a, const struct UIDisplayCharacter *b)
{
    return a->head_style == b->head_style && a->head_palette == b->head_palette
        && a->body_palette == b->body_palette && a->accessory_top == b->accessory_top
        && a->accessory_mid == b->accessory_mid && a->accessory_bottom == b->accessory_bottom
        && a->body_style == b->body_style;
}

struct UIStylingTab *styling_shop_find_tab(struct UIStylingShopWnd *shop, int id)
{
    struct TreeNode *node;

    for (node = tree_first(&shop->tabs); node != shop->tabs.head; node = tree_next(node)) {
        struct StyleTab *entry = TREE_VALUE(node, struct StyleTab);

        if (entry->button->id == id)
            return entry->tab;
    }
    return NULL;
}

static struct UIStylingTab *my_style_new_slot(void)
{
    struct UIStylingTab *slot = mem_alloc(sizeof *slot);

    if (slot) {
        ui_window_construct(&slot->base, 0);
        slot->base.ops = &ui_stylingshop_tab_parent_wnd_ops;
        slot->display = NULL;
        slot->info = NULL;
        slot->base.ops = &ui_slot_of_my_style_wnd_ops;
    }
    return slot;
}

static struct UIStylingTab *my_style_make_slot(struct UIMyStyleWnd *self, int i)
{
    struct UIWindow *wnd = &self->tab.base;
    struct UIStylingTab *slot = my_style_new_slot();

    ui_window_create(&slot->base, (int)(wnd->width * 0.31), wnd->height);
    ui_window_move_base(&slot->base, (int)(wnd->width * 0.33 * i + 4.0), 0);
    return slot;
}

BYTE my_style_add_slot(struct UIMyStyleWnd *self, struct UIDisplayCharacter *look)
{
    struct UIStylingTab *slot;
    int i;

    for (i = 0; i < MY_STYLE_SLOTS; i++)
        if (!self->slots[i])
            break;
    if (i == MY_STYLE_SLOTS)
        return 0;
    slot = my_style_make_slot(self, i);
    look->slot = i;
    ui_display_character_copy_look(slot->display, look);
    ui_window_prepend_child(&self->tab.base, &slot->base);
    self->slots[i] = slot;
    ui_button_show((struct UIButton *)(&self->delete_btn[i]->button.base), 1);
    ui_button_show((struct UIButton *)(&self->rotate_left[i]->button.base), 1);
    ui_button_show((struct UIButton *)(&self->rotate_right[i]->button.base), 1);
    return 1;
}

struct StyleRecord {
    int slot;
    int robe;
    int head_style;
    int body_style;
    int head_palette;
    int body_palette;
    int top, mid, bottom;
};

static void style_format_record(char *buf, const char *name, int slot, int head_palette, int head_style,
                                int body_style, int body_palette, int top, int mid, int bottom, int robe)
{
    snprintf(buf, 0x200,
                 "characterName:%s\nslotNum:%d\nheadPaletteNum:%d\nheadStyle:%d\nbodyStyle:%d\n"
                 "bodyPaletteNum:%d\ntopAccNum:%d\nmidAccNum:%d\nbottomAccNum:%d\nRobe:%d\n*\n",
                 name, slot, head_palette, head_style, body_style, body_palette, top, mid, bottom, robe);
}

static void style_parse_field(struct StyleRecord *rec, char *name, char *buf, int *i, const char *line)
{
    int *field;

    buf[*i] = 0;
    if (strstr("characterName", line)) {
        char text[0x18];
        unsigned n = 0;
        char c = buf[*i + 1];

        (*i)++;
        memset(text, 0, sizeof text);
        if (c != '\n') {
            while (c != '\r') {
                if (n < sizeof text - 1)
                    text[n] += c;
                n++;
                (*i)++;
                c = buf[*i];
                if (c == '\n')
                    break;
            }
        }
        cstr_copy(name, 0x18, text);
        return;
    }
    if (strstr("slotNum", line))
        field = &rec->slot;
    else if (strstr("headPaletteNum", line))
        field = &rec->head_palette;
    else if (strstr("headStyle", line))
        field = &rec->head_style;
    else if (strstr("bodyPaletteNum", line))
        field = &rec->body_palette;
    else if (strstr("topAccNum", line))
        field = &rec->top;
    else if (strstr("midAccNum", line))
        field = &rec->mid;
    else if (strstr("bottomAccNum", line))
        field = &rec->bottom;
    else if (strstr("Robe", line))
        field = &rec->robe;
    else if (strstr("bodyStyle", line))
        field = &rec->body_style;
    else
        return;
    (*i)++;
    *field = atoi(&buf[*i]);
}

static char *style_read_save_file(struct File *file, int *size)
{
    char *buf;

    if (!file_open(file, STYLE_SAVE_FILE, 0, 0, 0))
        return NULL;
    *size = file->size;
    buf = mem_alloc(*size + 1);
    file_read(file, buf, *size);
    buf[*size] = 0;
    file_close(file);
    return buf;
}

static void style_find_goods(const char *part, int view, int *goods)
{
    int count = 0, j, g = 0;

    client_lua_call(g_session1.luaState, "StylingShop_GetSizeInTable", "s>d", part, &count);
    for (j = 1; j < count + 1; j++) {
        client_lua_call(g_session1.luaState, "StylingShop_GetGoods", "sd>d", part, j, &g);
        if (view == item_table_get(g)->class_num) {
            *goods = g;
            break;
        }
    }
}

void ui_my_style_wnd_load_slots(struct UIMyStyleWnd *self)
{
    struct File file;
    struct StyleRecord rec;
    char name[0x18];
    const char *line;
    char *buf;
    int size = 0, i;

    file_construct(&file);
    buf = style_read_save_file(&file, &size);
    if (buf) {
        memset(&rec, 0, sizeof rec);
        memset(name, 0, sizeof name);
        line = buf;
        for (i = 0; i < size; i++) {
            switch (buf[i]) {
            case '\n':
            case '\r':
                line = &buf[i + 1];
                break;
            case '*':
                if (platform_code_page_find(name,
                                            session_character_name(&g_session1))
                    && rec.slot <= 3) {
                    struct UIDisplayCharacter *look = mem_alloc(sizeof *look);
                    struct UIStylingTab *slot;

                    if (look)
                        look = ui_display_character_construct(look);
                    look->slot = rec.slot;
                    look->robe = rec.robe;
                    look->head_style = rec.head_style;
                    look->body_style = rec.body_style;
                    look->head_palette = rec.head_palette;
                    look->body_palette = rec.body_palette;
                    look->accessory_mid = rec.mid;
                    look->accessory_top = rec.top;
                    look->accessory_bottom = rec.bottom;
                    if (rec.top)
                        style_find_goods("TopAccessory", rec.top, &look->goods_top);
                    if (rec.mid)
                        style_find_goods("MidAccessory", rec.mid, &look->goods_mid);
                    if (rec.bottom)
                        style_find_goods("BottomAccessory", rec.bottom, &look->goods_bottom);
                    slot = my_style_make_slot(self, rec.slot);
                    ui_display_character_copy_look(slot->display, look);
                    ui_window_add_child(&self->tab.base, &slot->base);
                    self->slots[rec.slot] = slot;
                    ui_display_character_destroy(&look->base);
                    memset(&rec, 0, sizeof rec);
                    rec.slot = 4;
                    line = &buf[i + 1];
                }
                break;
            case ':':
                style_parse_field(&rec, name, buf, &i, line);
                break;
            }
        }
        mem_free(buf);
    }
    file_destruct(&file);
}

void ui_my_style_wnd_save_slots(struct UIMyStyleWnd *self)
{
    struct String content;
    struct File file;
    struct StyleRecord rec;
    char name[0x18];
    char record[0x200];
    const char *line;
    char *buf;
    FILE *out;
    int size = 0, i;

    string_init(&content);
    file_construct(&file);
    buf = style_read_save_file(&file, &size);
    if (buf) {
        memset(&rec, 0, sizeof rec);
        memset(name, 0, sizeof name);
        line = buf;
        for (i = 0; i < size; i++) {
            switch (buf[i]) {
            case '\n':
            case '\r':
                line = &buf[i + 1];
                break;
            case '*':
                if (!platform_code_page_find(name,
                                             session_character_name(&g_session1))
                    && rec.slot <= 3) {
                    memset(record, 0, sizeof record);
                    style_format_record(record, name, rec.slot, rec.head_palette, rec.head_style, rec.body_style,
                                        rec.body_palette, rec.top, rec.mid, rec.bottom, rec.robe);
                    string_append_cstr(&content, record);
                    memset(&rec, 0, sizeof rec);
                    rec.slot = 4;
                    line = &buf[i + 1];
                }
                break;
            case ':':
                style_parse_field(&rec, name, buf, &i, line);
                break;
            }
        }
        mem_free(buf);
    }
    for (i = 0; i < MY_STYLE_SLOTS; i++) {
        struct UIDisplayCharacter *d;
        char copy[0x200];

        if (!self->slots[i])
            continue;
        memset(copy, 0, sizeof copy);
        memset(record, 0, sizeof record);
        d = self->slots[i]->display;
        style_format_record(record, session_character_name(&g_session1), d->slot, d->head_palette,
                            d->head_style, d->body_style, d->body_palette, d->accessory_top, d->accessory_mid,
                            d->accessory_bottom, d->robe);
        cstr_copy(copy, sizeof copy, record);
        string_append_cstr(&content, copy);
    }
    out = platform_file_open(STYLE_SAVE_FILE, "w");
    if (out) {
        fprintf(out, string_cstr(&content));
        fclose(out);
    }
    file_destruct(&file);
    string_free(&content);
}

static void show_case_add_part(struct UIAccessoryShowCase *self, int *placed, int *skipped, const char *part,
                               const int *count, const int *kind)
{
    struct UIWindow *wnd = &self->scroll.base;
    unsigned start = (unsigned)*placed, i;
    int j;

    for (i = start + 1, j = 1; i < (unsigned)*count + start + 1; i++, j++) {
        struct UIAccBitmapButton *btn;
        struct UIStaticBitmap *shadow;
        char path[0x80];
        char id_text[0x20];
        int goods = 0, per_row;
        unsigned index, row, col;

        client_lua_call(g_session1.luaState, "StylingShop_GetGoods", "sd>d", part, j, &goods);
        if (style_is_doram() == 1) {
            BYTE for_doram = 0;

            client_lua_call(g_session1.luaState, "StylingShop_GetIsDoram_by_goods", "sd>b", part, goods,
                    &for_doram);
            if (!for_doram) {
                (*skipped)++;
                continue;
            }
        }
        memset(path, 0, sizeof path);
        memset(id_text, 0, sizeof id_text);
        sprintf(id_text, "%d", goods);
        session_get_item_bitmap_file_name(id_text, path, 1);

        btn = mem_try_alloc(sizeof *btn);
        if (btn) {
            ui_bitmap_button_construct(&btn->btn);
            btn->btn.button.base.ops = &ui_acc_bitmap_button_ops.base;
        }
        ui_bitmap_button_set_bitmap(&btn->btn, path, UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(&btn->btn, path, UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(&btn->btn, path, UI_BTN_DOWN, 0);
        per_row = wnd->width / (btn->btn.bitmap_width + 7) - 1;

        shadow = mem_alloc(sizeof *shadow);
        if (shadow)
            shadow = ui_static_bitmap_construct(shadow);
        ui_static_bitmap_set_bitmap(shadow, STYLE_SHADOW_ITEM_OFF);
        ui_window_create(&shadow->base, shadow->base.width, shadow->base.height);
        index = i - *skipped - 1;
        row = index / (unsigned)per_row;
        col = index % (unsigned)per_row;
        ui_window_move_base(&shadow->base,
                 (int)((double)((unsigned)(btn->btn.bitmap_width + 7) * col) + wnd->width * 0.03),
                 (int)((double)((unsigned)(btn->btn.bitmap_height + 7) * row) + wnd->height * 0.03 + 10.0));
        ui_scroll_wnd_add_child(&self->scroll, &shadow->base);
        vec_push(&self->shadows, &shadow, sizeof shadow);

        ui_window_create(&btn->btn.button.base, btn->btn.bitmap_width, btn->btn.bitmap_height);
        ui_window_move_base(&btn->btn.button.base,
                 (int)((double)((unsigned)(btn->btn.bitmap_width + 7) * col) + wnd->width * 0.03 + 2.5),
                 (int)((double)((unsigned)(btn->btn.bitmap_height + 7) * row) + wnd->height * 0.03));
        btn->btn.button.base.id = STYLE_ID_ROW;
        btn->btn.button.cmd_value = (int)i;
        ui_bitmap_button_set_tool_tip(&btn->btn, item_table_get(goods)->name);
        btn->goods = goods;
        btn->kind = *kind;
        ui_scroll_wnd_add_child(&self->scroll, &btn->btn.button.base);
        (*placed)++;
    }
}

void ui_accessory_show_case_fill_headgear(struct UIAccessoryShowCase *self)
{
    static const char *const parts[3] = { "TopAccessory", "MidAccessory", "BottomAccessory" };
    int count = 0, placed = 0, skipped = 0, kind;

    for (kind = 0; kind < 3; kind++) {
        client_lua_call(g_session1.luaState, "StylingShop_GetSizeInTable", "s>d", parts[kind], &count);
        show_case_add_part(self, &placed, &skipped, parts[kind], &count, &kind);
    }
}

static void style_toggle_accessory(int goods, int kind)
{
    struct UIStylingShopWnd *shop = (struct UIStylingShopWnd *)ui_get_window(WND_STYLING_SHOP);
    struct UIStylingTab *tab = styling_shop_find_tab(shop, STYLE_TAB_ACCESSORY);
    int view = item_table_get(goods)->class_num;
    struct UIDisplayCharacter *d;
    int *view_field, *goods_field;

    switch (kind) {
    case 0:
        d = tab->display;
        view_field = &d->accessory_top;
        goods_field = &d->goods_top;
        break;
    case 1:
        d = tab->display;
        view_field = &d->accessory_mid;
        goods_field = &d->goods_mid;
        break;
    case 2:
        d = tab->display;
        view_field = &d->accessory_bottom;
        goods_field = &d->goods_bottom;
        break;
    default:
        ui_window_invalidate_children(&tab->base);
        return;
    }
    if (view == *view_field) {
        *goods_field = 0;
        *view_field = 0;
    } else {
        *goods_field = goods;
        *view_field = view;
    }
    ui_window_invalidate_children(&tab->base);
}

static void show_case_part_name(char *buf, int kind)
{
    memset(buf, 0, 0x40);
    if (kind == 0)
        cstr_copy(buf, 0x40, "TopAccessory");
    else if (kind == 1)
        cstr_copy(buf, 0x40, "MidAccessory");
    else if (kind == 2)
        cstr_copy(buf, 0x40, "BottomAccessory");
}

int ui_accessory_show_case_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    struct UIAccessoryShowCase *self = (struct UIAccessoryShowCase *)window;
    struct UIWindow *shop = ui_get_window(WND_STYLING_SHOP);
    struct UIAccBitmapButton *btn = (struct UIAccBitmapButton *)sender;
    char part[0x40];

    if (msg == UI_MSG_COMMAND) {
        unsigned i;
        BYTE ok;

        if (val1 != STYLE_ID_ROW || !sender)
            return 0;
        for (i = 0; i < (unsigned)((char **)self->shadows.last - (char **)self->shadows.first); i++)
            ui_static_bitmap_set_bitmap(VEC_AT(&self->shadows, struct UIStaticBitmap *, i),
                                     i == (unsigned)(btn->btn.button.cmd_value - 1) ? STYLE_SHADOW_ITEM_ON
                                                                                    : STYLE_SHADOW_ITEM_OFF);
        show_case_part_name(part, btn->kind);
        ok = 1;
        if (style_is_doram() == 1)
            client_lua_call(g_session1.luaState, "StylingShop_GetIsDoram_by_goods", "sd>b", part, btn->goods, &ok);
        if (ok == 1)
            style_toggle_accessory(btn->goods, btn->kind);
        return 0;
    }
    if (msg == STYLE_MSG_ACCESSORY) {
        struct ItemInfo info;
        struct UIWindow *wnd;

        if (!sender)
            return 0;
        show_case_part_name(part, btn->kind);
        item_info_construct(&info);
        item_info_set_item_id(&info, btn->goods);
        item_info_set_name(&info, item_table_get(btn->goods)->name);
        info.identified = 1;
        wnd = ui_window_open(0xc);
        ui_window_send(wnd, NULL, 0x18, (int)&info, 0, 0, 0);
        if (g_windowMgr.item_collection_wnd) {
            struct ItemInfo *worn = session_get_worn_counterpart(&g_session1, &info);

            if (g_session1.itemCompare && worn && val4 == 0) {
                struct UIWindow *compare = ui_window_open(0xea);

                if (compare)
                    ui_window_send_msg(compare, NULL, 0x18, (int)worn, 1, 0, 0);
                else
                    ui_window_mgr_post_quit1(&g_windowMgr, 0xea);
            }
        }
        item_info_destruct(&info);
        return 0;
    }
    ui_window_invalidate(shop);
    ui_window_invalidate_children(shop);
    return ui_scroll_wnd_send_msg((struct UIWindow *)(&self->scroll), sender, msg, val1, val2, val3, val4);
}

struct UIStylingShopWnd *ui_styling_shop_wnd_construct(struct UIStylingShopWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_styling_shop_wnd_ops;
    tree_init(&self->tabs);
    return self;
}

struct UIWindow *ui_styling_shop_wnd_new(void)
{
    return (struct UIWindow *)ui_styling_shop_wnd_construct(mem_alloc(sizeof(struct UIStylingShopWnd)));
}

static void styling_shop_destruct(struct UIStylingShopWnd *self)
{
    struct PACKET_CZ_CLOSE_UI_STYLINGSHOP packet;
    int len;

    self->frame.base.ops = &ui_styling_shop_wnd_ops;
    packet.packet_type = STYLE_PACKET_CLOSE;
    len = rag_connection_get_packet_length(rag_connection_instance(), STYLE_PACKET_CLOSE);
    rag_connection_send_packet(rag_connection_instance(), len, &packet);
    tree_free(&self->tabs, NULL);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_styling_shop_wnd_destroy(struct UIWindow *window)
{
    struct UIStylingShopWnd *self = (struct UIStylingShopWnd *)window;
    styling_shop_destruct(self);
    mem_free(self);
}

static struct UIStylingTab *styling_shop_shown_tab(struct UIStylingShopWnd *shop)
{
    struct TreeNode *node;
    struct UIStylingTab *shown = NULL;

    for (node = tree_first(&shop->tabs); node != shop->tabs.head; node = tree_next(node))
        if (TREE_VALUE(node, struct StyleTab)->tab->base.visible == 1)
            shown = TREE_VALUE(node, struct StyleTab)->tab;
    return shown;
}

BYTE styling_shop_save_look(struct UIStylingShopWnd *shop)
{
    struct UIStylingTab *shown = styling_shop_shown_tab(shop);
    struct UIStylingTab *my_style = styling_shop_find_tab(shop, STYLE_TAB_MY_STYLE);

    if (!my_style)
        return 0;
    return my_style_add_slot((struct UIMyStyleWnd *)my_style, shown ? shown->display : NULL);
}

void styling_shop_refresh_favourite(struct UIStylingShopWnd *shop)
{
    struct TreeNode *node;
    struct UIStylingTab *shown = NULL;

    for (node = tree_first(&shop->tabs); node != shop->tabs.head; node = tree_next(node)) {
        struct StyleTab *entry = TREE_VALUE(node, struct StyleTab);

        if (entry->tab->base.visible == 1) {
            if (entry->button->id == STYLE_TAB_MY_STYLE)
                return;
            shown = entry->tab;
        }
    }
    for (node = tree_first(&shop->tabs); node != shop->tabs.head; node = tree_next(node)) {
        struct StyleTab *entry = TREE_VALUE(node, struct StyleTab);
        struct UIMyStyleWnd *my_style;
        struct UIDisplayCharacter *display;
        BYTE saved = 0;
        int i;

        if (entry->button->id != STYLE_TAB_MY_STYLE)
            continue;
        my_style = (struct UIMyStyleWnd *)entry->tab;
        display = shown->display;
        for (i = 0; i < MY_STYLE_SLOTS; i++) {
            if (my_style->slots[i] && style_same_look(my_style->slots[i]->display, display)) {
                saved = 1;
                break;
            }
        }
        if (display)
            display_set_favourite(display, saved);
    }
}

void styling_shop_set_buyable(struct UIStylingShopWnd *shop, BYTE all_ok)
{
    struct UIBitmapButton *buy = shop->buy;

    if (!all_ok) {
        buy->button.cmd_value = 1;
        ui_bitmap_button_set_bitmap(shop->buy, STYLE_SKIN "btn_buy_disable.bmp", UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(shop->buy, STYLE_SKIN "btn_buy_disable.bmp", UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(shop->buy, STYLE_SKIN "btn_buy_disable.bmp", UI_BTN_DOWN, 0);
    } else {
        buy->button.cmd_value = 0;
        ui_bitmap_button_set_bitmap(shop->buy, STYLE_SKIN "btn_buy_out.bmp", UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(shop->buy, STYLE_SKIN "btn_buy_over.bmp", UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(shop->buy, STYLE_SKIN "btn_buy_down.bmp", UI_BTN_DOWN, 0);
    }
    ui_window_invalidate_base(&shop->buy->button.base);
}

static int tab_button_less(const void *a, const void *b)
{
    return (size_t)*(struct UIWindow *const *)a < (size_t)*(struct UIWindow *const *)b;
}

static void styling_shop_add_tab(struct UIStylingShopWnd *self, const char *up, const char *over, const char *down,
                                 const int *id, struct UIStylingTab *tab, const char *tooltip)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIBitmapButton *btn = mem_try_alloc(sizeof *btn);
    struct StyleTab entry;

    if (btn) {
        ui_bitmap_button_construct(btn);
        btn->button.base.ops = &ui_styling_shop_button_ops.base;
    }
    ui_bitmap_button_set_bitmap(btn, up, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(btn, over, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(btn, down, UI_BTN_DOWN, 0);
    ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
    ui_window_move_base(&btn->button.base, 3, (int)self->tabs.size * btn->bitmap_height + 0x15);
    btn->button.base.id = *id;
    ui_bitmap_button_set_tool_tip(btn, tooltip);
    ui_window_add_child(wnd, &btn->button.base);

    ui_window_create(&tab->base, (int)(wnd->width * 0.905), (int)(wnd->height * 0.885 - 17.0));
    ui_window_move_base(&tab->base, (int)(wnd->width * 0.1), 0x11);
    ui_window_set_show_base(&tab->base, 0);
    ui_window_add_child(wnd, &tab->base);
    entry.button = &btn->button.base;
    entry.tab = tab;
    tree_add(&self->tabs, &entry, sizeof entry, tab_button_less, NULL);
}

void ui_styling_shop_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIStylingShopWnd *self = (struct UIStylingShopWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIBasicStylingWnd *hair;
    struct UIStylingTab *accessory;
    struct UI2ndCostumeWnd *costume;
    struct UIMyStyleWnd *my_style;
    struct UIDisplayCharacter *display;
    struct UIBitmapButton *buy;
    struct UIWindow *label;
    struct String text;
    char money[0x100];
    int id;

    (void)width;
    (void)height;
    ui_window_add_close_button(wnd, "", 0);

    hair = mem_try_alloc(sizeof *hair);
    if (hair) {
        ui_window_construct(&hair->tab.base, 0);
        hair->tab.base.ops = &ui_stylingshop_tab_parent_wnd_ops;
        hair->tab.display = NULL;
        hair->tab.info = NULL;
        hair->tab.base.ops = &ui_basic_styling_wnd_ops;
        hair->palette.first = NULL;
        hair->palette.last = NULL;
        hair->palette.end = NULL;
    }
    id = STYLE_TAB_HAIR;
    styling_shop_add_tab(self, STYLE_SKIN "tab_hair_off.bmp", STYLE_SKIN "tab_hair_off.bmp",
                         STYLE_SKIN "tab_hair_on.bmp", &id, &hair->tab, msg_str(0xb15));
    ui_window_set_show_base(&hair->tab.base, 1);
    ui_window_set_state(TREE_VALUE(tree_first(&self->tabs), struct StyleTab)->button, UI_BTN_DOWN);

    accessory = mem_try_alloc(sizeof *accessory);
    if (accessory) {
        ui_window_construct(&accessory->base, 0);
        accessory->base.ops = &ui_stylingshop_tab_parent_wnd_ops;
        accessory->display = NULL;
        accessory->info = NULL;
        accessory->base.ops = &ui_accessory_cus_wnd_ops;
    }
    id = STYLE_TAB_ACCESSORY;
    styling_shop_add_tab(self, STYLE_SKIN "tab_accessories_off.bmp", STYLE_SKIN "tab_accessories_off_over.bmp",
                         STYLE_SKIN "tab_accessories_on.bmp", &id, accessory, msg_str(0xb0f));

    costume = mem_try_alloc(sizeof *costume);
    if (costume) {
        ui_window_construct(&costume->tab.base, 0);
        costume->tab.base.ops = &ui_stylingshop_tab_parent_wnd_ops;
        costume->tab.display = NULL;
        costume->tab.info = NULL;
        costume->tab.base.ops = &ui2nd_costume_wnd_ops;
        costume->show_box = NULL;
        costume->hide_box = NULL;
    }
    id = STYLE_TAB_2ND_COSTUME;
    if (session_is_third_job(&g_session1, session_get_drawn_job(&g_session1)) == 1)
        styling_shop_add_tab(self, STYLE_SKIN "tab_2ndcostume_off.bmp", STYLE_SKIN "tab_2ndcostume_on_over.bmp",
                             STYLE_SKIN "tab_2ndcostume_on.bmp", &id, &costume->tab, msg_str(0xb10));
    else
        styling_shop_add_tab(self, STYLE_SKIN "tab_2ndcostume_off.bmp", STYLE_SKIN "tab_2ndcostume_off.bmp",
                             STYLE_SKIN "tab_2ndcostume_off.bmp", &id, &costume->tab, msg_str(0xb10));

    my_style = mem_try_alloc(sizeof *my_style);
    if (my_style) {
        ui_window_construct(&my_style->tab.base, 0);
        my_style->tab.base.ops = &ui_stylingshop_tab_parent_wnd_ops;
        my_style->tab.display = NULL;
        my_style->tab.info = NULL;
        my_style->tab.base.ops = &ui_my_style_wnd_ops;
        my_style->selected = 0;
        my_style->slots[0] = NULL;
        my_style->slots[1] = NULL;
        my_style->slots[2] = NULL;
    }
    id = STYLE_TAB_MY_STYLE;
    styling_shop_add_tab(self, STYLE_SKIN "tab_favorite_off.bmp", STYLE_SKIN "tab_favorite_off_over.bmp",
                         STYLE_SKIN "tab_favorite_on.bmp", &id, &my_style->tab, msg_str(0xb11));

    display = mem_alloc(sizeof *display);
    if (display)
        display = ui_display_character_construct(display);
    self->display = display;
    ui_window_create(&display->base, 0x78, 0x78);
    ui_window_set_show_base(&self->display->base, 0);
    ui_window_add_child(wnd, &self->display->base);

    buy = mem_try_alloc(sizeof *buy);
    if (buy)
        buy = ui_bitmap_button_construct(buy);
    self->buy = buy;
    ui_bitmap_button_set_bitmap(buy, STYLE_SKIN "btn_buy_out.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(self->buy, STYLE_SKIN "btn_buy_over.bmp", UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(self->buy, STYLE_SKIN "btn_buy_down.bmp", UI_BTN_DOWN, 0);
    ui_window_create(&self->buy->button.base, self->buy->bitmap_width, self->buy->bitmap_height);
    ui_window_move_base(&self->buy->button.base, 0x13c, 0xe2);
    self->buy->button.base.id = STYLE_ID_BUY;
    ui_window_add_child(wnd, &self->buy->button.base);

    memset(money, 0, sizeof money);
    {
        const char *caption = msg_str(0xad5);
        const char *amount;

        string_init(&text);
        string_assign_cstr(&text, caption);
        string_append_cstr(&text, " ");
        amount = format_money(g_session1.zeny, money);
        string_append_cstr(&text, amount);
        string_append_cstr(&text, "\tz");
    }
    label = mem_try_alloc(sizeof(struct UIStaticText));
    if (label)
        label = ui_static_text_construct(label);
    ui_window_create(label, 0xc8, 0xc);
    ui_window_move(label, 9, 0xe6);
    ui_static_text_set_font(label, 0, 0xb, 0, 0);
    ui_static_text_set_text_colour(label, 0x84, 0x5a, 0x5a);
    ui_static_text_set_text(label, string_cstr(&text), 0);
    ui_window_add_child(wnd, label);
    string_free(&text);
}

void ui_styling_shop_wnd_on_process(struct UIWindow *window)
{
    struct UIStylingShopWnd *self = (struct UIStylingShopWnd *)window;
    struct TreeNode *node;

    for (node = tree_first(&self->tabs); node != self->tabs.head; node = tree_next(node)) {
        struct StyleTab *entry = TREE_VALUE(node, struct StyleTab);

        if (!entry->tab->base.visible)
            continue;
        if (entry->button->id == STYLE_TAB_MY_STYLE)
            return;
        ui_window_invalidate_base(&entry->tab->display->base);
    }
}

void ui_styling_shop_wnd_on_draw(struct UIWindow *window)
{
    struct UIStylingShopWnd *self = (struct UIStylingShopWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct BitmapRes *bg;

    styling_shop_refresh_favourite(self);
    ui_window_clear_canvas(wnd, 0xffffffff);
    ui_window_draw_title_bar(wnd, 1, msg_str(0xafd), 0);
    bg = ui_skin_bitmap(STYLE_SKIN "bg_styling.bmp");
    if (bg)
        ui_window_draw_bitmap(wnd, 0, 0, bg, 0);
}

static void styling_shop_buy(struct UIStylingShopWnd *self)
{
    struct TreeNode *node;

    for (node = tree_first(&self->tabs); node != self->tabs.head; node = tree_next(node)) {
        struct StyleTab *entry = TREE_VALUE(node, struct StyleTab);
        struct UIStylingTab *tab = entry->tab;

        if (tab->base.visible == 0)
            continue;
        if (entry->button->id == STYLE_TAB_MY_STYLE) {
            struct UIMyStyleWnd *my_style = (struct UIMyStyleWnd *)tab;
            struct UIStylingTab *slot = my_style->slots[my_style->selected];

            if (!slot || !slot->info || show_info_buy(slot->info) != 1)
                continue;
            ui_window_remove_child(&my_style->tab.base, &my_style->slots[my_style->selected]->base, 1);
            my_style->slots[my_style->selected] = NULL;
            ui_button_show((struct UIButton *)(&my_style->delete_btn[my_style->selected]->button.base), 0);
        } else if (tab->info) {
            show_info_buy(tab->info);
        }
    }
}

static void styling_shop_switch_tab(struct UIStylingShopWnd *self, int id)
{
    struct TreeNode *node;

    for (node = tree_first(&self->tabs); node != self->tabs.head; node = tree_next(node)) {
        struct StyleTab *entry = TREE_VALUE(node, struct StyleTab);

        if (entry->tab->base.visible != 1)
            continue;
        if (entry->button->id != STYLE_TAB_MY_STYLE) {
            struct UIStylingShopWnd *shop = (struct UIStylingShopWnd *)ui_window_find(WND_STYLING_SHOP);

            if (shop && shop->display)
                ui_display_character_copy_look(shop->display, entry->tab->display);
        }
        break;
    }
    for (node = tree_first(&self->tabs); node != self->tabs.head; node = tree_next(node)) {
        struct StyleTab *entry = TREE_VALUE(node, struct StyleTab);

        if (entry->button->id == id) {
            if (entry->button->id != STYLE_TAB_MY_STYLE)
                ui_display_character_copy_look(entry->tab->display, self->display);
            if (entry->button->id == STYLE_TAB_2ND_COSTUME) {
                struct UI2ndCostumeWnd *costume = (struct UI2ndCostumeWnd *)entry->tab;

                ui_check_box_set_check(&costume->show_box->base, costume->tab.display->body_style == 0);
                ui_check_box_set_check(&costume->hide_box->base, costume->show_box->base.state == 0);
            }
            ui_window_set_show_base(&entry->tab->base, 1);
            ui_window_invalidate_children(&entry->tab->base);
        } else {
            ui_window_set_state(entry->button, UI_BTN_UP);
            ui_window_set_show_base(&entry->tab->base, 0);
        }
    }
    ui_window_invalidate_base(&self->frame.base);
}

int ui_styling_shop_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                        int val2, int val3, int val4)
{
    struct UIStylingShopWnd *self = (struct UIStylingShopWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    switch (val1) {
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, WND_STYLING_SHOP);
        ui_window_invalidate_base(wnd);
        return 0;
    case STYLE_ID_BUY:
        if (self->buy->button.cmd_value == 0
            && ui_window_mgr_message_box(&g_windowMgr, msg_str(0xb02), UI_MSGBOX_QUESTION, 0, 0, 0,
                                      "메시지", 0x118, 0x78, 0)
                   != 0xb4
            && self->tabs.size) {
            styling_shop_buy(self);
            ui_window_invalidate_base(wnd);
            return 0;
        }
        break;
    case STYLE_TAB_HAIR:
    case STYLE_TAB_MY_STYLE:
    case STYLE_TAB_ACCESSORY:
    case STYLE_TAB_2ND_COSTUME:
        if (session_is_third_job(&g_session1, session_get_drawn_job(&g_session1)) == 0
            && val1 == STYLE_TAB_2ND_COSTUME)
            break;
        styling_shop_switch_tab(self, val1);
        break;
    }
    ui_window_invalidate_base(wnd);
    return 0;
}
