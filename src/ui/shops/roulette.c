#include "world/mode_types.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include <stdio.h>
#include "ui/shops/shops.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "ui/shops/roulette.h"
#include "ui/misc/roulette_state.h"
#include "std/tree.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/render/graphic_setting.h"

#define PKT_CZ_REQ_OPEN_ROULETTE 0x0a19
#define PKT_CZ_REQ_GENERATE_ROULETTE 0x0a1f
#define PKT_CZ_RECV_ROULETTE_ITEM 0x0a21
#pragma pack(push, 1)
struct PACKET_CZ_RECV_ROULETTE_ITEM {
    unsigned short packet_type;
    BYTE condition;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_CZ_RECV_ROULETTE_ITEM, 3);

struct UIOpenRoulletteWnd;
void ui_open_roullette_wnd_destroy(struct UIWindow *window);
void ui_open_roullette_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_open_roullette_wnd_on_process(struct UIWindow *self);
void ui_open_roullette_wnd_on_draw(struct UIWindow *self);
int ui_open_roullette_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_open_roullette_wnd_ops = {
    .destroy = ui_open_roullette_wnd_destroy,
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
    .on_create = ui_open_roullette_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_open_roullette_wnd_on_process,
    .on_draw = ui_open_roullette_wnd_on_draw,
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
    .send_msg = ui_open_roullette_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define OPEN_ROULETTE_ICON UI_SKIN "basic_interface\\roullette\\RoulletteIcon.bmp"
#define OPEN_ROULETTE_BUTTON_ID 0xb8

struct UIOpenRoulletteWnd {
    struct UIFrameWnd frame;
};
SIZE_ASSERT(UIOpenRoulletteWnd, 0x94);

struct UIOpenRoulletteWnd *ui_open_roullette_wnd_construct(struct UIOpenRoulletteWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_open_roullette_wnd_ops;
    return self;
}

struct UIWindow *ui_open_roullette_wnd_new(void)
{
    return (struct UIWindow *)ui_open_roullette_wnd_construct(mem_alloc(sizeof(struct UIOpenRoulletteWnd)));
}

static void ui_open_roullette_wnd_destruct(struct UIOpenRoulletteWnd *self)
{
    self->frame.base.ops = &ui_open_roullette_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_open_roullette_wnd_destroy(struct UIWindow *window)
{
    struct UIOpenRoulletteWnd *self = (struct UIOpenRoulletteWnd *)window;
    ui_open_roullette_wnd_destruct(self);
    mem_free(self);
}

void ui_open_roullette_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIOpenRoulletteWnd *self = (struct UIOpenRoulletteWnd *)window;
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, OPEN_ROULETTE_ICON, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, OPEN_ROULETTE_ICON, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, OPEN_ROULETTE_ICON, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    button->button.base.id = OPEN_ROULETTE_BUTTON_ID;
    ui_window_add_child(&self->frame.base, &button->button.base);
}

void ui_open_roullette_wnd_on_process(struct UIWindow *self)
{
}

void ui_open_roullette_wnd_on_draw(struct UIWindow *self)
{
}

int ui_open_roullette_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4)
{
    struct UIOpenRoulletteWnd *self = (struct UIOpenRoulletteWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
    if (val1 == OPEN_ROULETTE_BUTTON_ID && !g_windowMgr.roullette_wnd) {
        unsigned short pkt = PKT_CZ_REQ_OPEN_ROULETTE;

        send_packet(&pkt, rag_connection_get_packet_length(rag_connection_instance(), PKT_CZ_REQ_OPEN_ROULETTE));
    }
    return 0;
}

struct Roulette *g_roulette;

#define ROULETTE_WND_ID 0x10d
#define UI_ID_ROULETTE_START 0x191
#define UI_ID_ROULETTE_PRIZE 0x192
#define UI_MSG_ROULETTE_REDRAW 0x3c

#define MSG_ROULETTE_TITLE 0xa51
#define MSG_ROULETTE_BUSY 0xa50
#define MSG_ROULETTE_PRIZE_TIP 0xa7b
#define MSG_ROULETTE_START_TIP 0xa7c
#define MSG_ITEM_WITH_OPTIONS 0xa28

#define ROULETTE_DIR UI_SKIN "basic_interface\\roullette\\"
#define ROULETTE_ARROW ROULETTE_DIR "stageArrow.bmp"

#define ROULETTE_ROW_Y(row) (0x158 - (row) * 40)
#define ROULETTE_SLOT_X(row, col) (((row) + (col)) * 36 + 0x9f)
#define ROULETTE_HIDDEN (-100)

#define SPRITE_PLAYED 2
#define SPRITE_IDLE 3

struct UIRoulletteWnd;
void ui_roullette_wnd_destroy(struct UIWindow *window);
void ui_roullette_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_roullette_wnd_on_process(struct UIWindow *self);
void ui_roullette_wnd_on_draw(struct UIWindow *self);
void ui_roullette_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_roullette_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_roullette_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_roullette_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_roullette_wnd_ops = {
    .destroy = ui_roullette_wnd_destroy,
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
    .on_create = ui_roullette_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_roullette_wnd_on_process,
    .on_draw = ui_roullette_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_roullette_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_roullette_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_roullette_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_roullette_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIRoulletteWnd {
    struct UIFrameWnd frame;
    struct UIBitmapButton *start;
    struct UIWindow *start_sprite;
    struct UIBitmapButton *prize_button;
    struct UIWindow *prize_sprite;
    struct UIWindow *arrow;
    BYTE may_spin;
    struct ItemInfo hover_item;
    struct List prize_items;
    int hover_col;
    int hover_row;
    struct UIWindow *gold_text;
    struct UIWindow *silver_text;
    struct UIWindow *bronze_text;
};
OFFSET_ASSERT(UIRoulletteWnd, may_spin, 0xa8);

static int mode_cursor_action(void)
{
    return client_current_mode()->cursor_action;
}

struct UIRoulletteWnd *ui_roullette_wnd_construct(struct UIRoulletteWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_roullette_wnd_ops;
    item_info_construct(&self->hover_item);
    list_init(&self->prize_items);
    self->start = NULL;
    self->start_sprite = NULL;
    self->prize_button = NULL;
    self->prize_sprite = NULL;
    self->arrow = NULL;
    self->may_spin = 1;
    list_clear(&self->prize_items, item_info_destroy);
    self->hover_col = 0;
    self->hover_row = 0;
    self->gold_text = NULL;
    self->silver_text = NULL;
    self->bronze_text = NULL;
    return self;
}

struct UIWindow *ui_roullette_wnd_new(void)
{
    return (struct UIWindow *)ui_roullette_wnd_construct(mem_alloc(sizeof(struct UIRoulletteWnd)));
}

static void ui_roullette_wnd_destruct(struct UIRoulletteWnd *self)
{
    self->frame.base.ops = &ui_roullette_wnd_ops;
    list_free(&self->prize_items, item_info_destroy);
    item_info_destruct(&self->hover_item);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_roullette_wnd_destroy(struct UIWindow *window)
{
    struct UIRoulletteWnd *self = (struct UIRoulletteWnd *)window;
    ui_roullette_wnd_destruct(self);
    mem_free(self);
}

struct ItemInfo *ui_roullette_wnd_item_at(struct UIRoulletteWnd *self, struct ItemInfo *out, int x, int y)
{
    struct TreeNode *node;
    int row = 0, col, top = ROULETTE_ROW_Y(0), right;

    self->hover_row = 0;
    self->hover_col = 0;
    for (;;) {
        if (top + 0x1a >= y && top <= y)
            break;
        top -= 40;
        row++;
        if (top <= 0x40)
            return item_info_construct_with_arg(out);
    }
    self->hover_row = row;
    for (col = 0, right = row * 36 + 0xb9;; col++, right += 36) {
        if (col >= 9 - row)
            return item_info_construct_with_arg(out);
        if (right - 0x1b <= x && right >= x)
            break;
    }
    self->hover_col = col;
    for (node = tree_first(&g_roulette->prizes); node != g_roulette->prizes.head; node = tree_next(node)) {
        struct PrizeEntry *entry = TREE_VALUE(node, struct PrizeEntry);

        if (entry->prize.row == row && entry->prize.position == col)
            return item_info_copy(out, &entry->item);
    }
    return item_info_construct_with_arg(out);
}

static struct UIBitmapButton *add_button(struct UIRoulletteWnd *self, const char *a, const char *b, int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, a, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, a, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, b, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, ROULETTE_HIDDEN, ROULETTE_HIDDEN);
    button->button.base.id = id;
    ui_window_add_child(&self->frame.base, &button->button.base);
    return button;
}

static struct UIWindow *add_sprite(struct UIRoulletteWnd *self, const char *spr, const char *act, int width,
                                   int height)
{
    struct UIWindow *sprite = mem_alloc(sizeof(struct UISpriteButton));

    if (sprite)
        sprite = ui_sprite_button_construct(sprite);
    ui_sprite_button_set_sprite(sprite, spr);
    ui_sprite_button_set_act(sprite, act);
    ui_window_create(sprite, width, height);
    ui_window_move(sprite, ROULETTE_HIDDEN, ROULETTE_HIDDEN);
    ((struct UISpriteButton *)sprite)->action = 0;
    ((struct UISpriteButton *)sprite)->motion = 0;
    ((struct UISpriteButton *)sprite)->animating = 1;
    ui_window_add_child(&self->frame.base, sprite);
    return sprite;
}

static struct UIWindow *add_count(struct UIRoulletteWnd *self, int y)
{
    struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

    if (text)
        text = ui_static_text_construct(text);
    ui_window_create(text, 0x19, 0xc);
    ui_window_move(text, 0x23f, y);
    ui_static_text_set_text_colour(text, 0, 0, 0);
    ui_window_add_child(&self->frame.base, text);
    return text;
}

void ui_roullette_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIRoulletteWnd *self = (struct UIRoulletteWnd *)window;
    ui_window_add_close_button(&self->frame.base, "", 0);
    self->start = add_button(self, ROULETTE_DIR "StartRoullette_a.bmp", ROULETTE_DIR "StartRoullette_b.bmp",
                             UI_ID_ROULETTE_START);
    if (g_spriteMode == 2) {
        self->start_sprite = add_sprite(self, "roullette\\Start.spr", "roullette\\Start.act", 0x63, 0x63);
        self->prize_sprite = add_sprite(self, "roullette\\GetWinPrize.spr", "roullette\\GetWinPrize.act", 0x63, 0x63);
        self->arrow = add_sprite(self, "roullette\\stageArrow.spr", "roullette\\stageArrow.act", 0x23, 0x28);
    }
    self->prize_button = add_button(self, ROULETTE_DIR "GetWinPrize_a.bmp", ROULETTE_DIR "GetWinPrize_b.bmp",
                                    UI_ID_ROULETTE_PRIZE);
    self->gold_text = add_count(self, 0x86);
    self->silver_text = add_count(self, 0xda);
    self->bronze_text = add_count(self, 0x12e);
}

static int next_spin_offset(void)
{
    struct Roulette *r = g_roulette;

    if (r->has_slot == 1)
        return r->step + 1 < 7 && !r->slot_flag ? 1 : 0;
    if (r->coins[ROULETTE_BRONZE] > 0)
        return 0;
    if (r->coins[ROULETTE_SILVER] >= 10)
        return 2;
    return r->coins[ROULETTE_GOLD] >= 10 ? 4 : 0;
}

static void place_buttons(struct UIRoulletteWnd *self)
{
    struct Roulette *r = g_roulette;

    if (r->step != 6 && (r->coins[ROULETTE_BRONZE] > 0 || r->coins[ROULETTE_SILVER] >= 10 || r->coins[ROULETTE_GOLD] >= 10))
        ui_window_move_base(&self->start->button.base, 0x153, 0x18e);
    if (g_roulette->spinning) {
        ui_window_move_base(&self->start->button.base, ROULETTE_HIDDEN, ROULETTE_HIDDEN);
        if (g_spriteMode == 2)
            ui_window_move(self->arrow, ROULETTE_HIDDEN, ROULETTE_HIDDEN);
    }
    if (g_roulette->has_slot != 1) {
        if (g_spriteMode == 2)
            ui_window_move(self->prize_sprite, ROULETTE_HIDDEN, ROULETTE_HIDDEN);
        ui_window_move_base(&self->prize_button->button.base, ROULETTE_HIDDEN, ROULETTE_HIDDEN);
        return;
    }
    ui_window_move_base(&self->prize_button->button.base, 0xcb, 0x18e);
    if (g_roulette->step != 6)
        ui_window_move_base(&self->start->button.base, 0x153, 0x18e);
}

static void set_count(struct UIWindow *text, int count)
{
    char buf[0x28] = {0};

    sprintf(buf, "%d", count);
    ui_static_text_set_text_ellipsis(text, buf);
}

void ui_roullette_wnd_on_draw(struct UIWindow *window)
{
    struct UIRoulletteWnd *self = (struct UIRoulletteWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct Roulette *r;
    struct TreeNode *node;

    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_ROULETTE_TITLE), 0);
    ui_window_draw_body(wnd, 0x11, 0, ROULETTE_DIR "roulletteBG.bmp");
    set_count(self->gold_text, g_roulette->coins[ROULETTE_GOLD]);
    set_count(self->silver_text, g_roulette->coins[ROULETTE_SILVER]);
    set_count(self->bronze_text, g_roulette->coins[ROULETTE_BRONZE]);
    r = g_roulette;
    if (r->spinning || (r->has_slot == 1 && !r->slot_flag))
        ui_window_draw_bitmap(wnd, 0x1e2, ROULETTE_ROW_Y(r->step), ui_skin_bitmap(ROULETTE_ARROW), 1);
    place_buttons(self);
    r = g_roulette;
    for (node = tree_first(&r->prizes); node != r->prizes.head; node = tree_next(node)) {
        struct PrizeEntry *entry = TREE_VALUE(node, struct PrizeEntry);
        const struct RoulettePrize *slot = &entry->prize;
        int x = ROULETTE_SLOT_X(slot->row, slot->position), y = ROULETTE_ROW_Y(slot->row);

        if (r->step == slot->row && (r->has_slot || r->spinning))
            ui_window_draw_item_icon_count(wnd, x, y, &entry->item, 0, 0);
        else
            ui_window_draw_item_icon_count_grey(wnd, 0, x, y, &entry->item, 0, 0);
        r = g_roulette;
    }
    if (r->has_slot == 1 && r->slot_item.count != 0) {
        ui_window_draw_item_icon_count(wnd, 0xbd, 0x91, &r->slot_item, 0, 0);
        r = g_roulette;
    }
    if (r->addition_item.count != 0)
        ui_window_draw_item_icon_count(wnd, 0x3c, 0x87, &r->addition_item, 0, 0);
}

void ui_roullette_wnd_on_l_btn_down(struct UIWindow *self, int x, int y)
{
}

void ui_roullette_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIRoulletteWnd *self = (struct UIRoulletteWnd *)window;
    struct ItemInfo item;
    int tip_x, tip_y;

    ui_roullette_wnd_item_at(self, &item, x, y);
    tip_x = (self->hover_row + self->hover_col) * 36 + 0x9b;
    tip_y = 0x14a - self->hover_row * 40;
    if ((unsigned)(y - 0x8f) <= 0x1e && (unsigned)(x - 0xba) <= 0x1e) {
        tip_x = 0xba;
        tip_y = 0x82;
        if (g_roulette->has_slot == 1)
            item_info_assign(&item, &g_roulette->slot_item);
    }
    if ((unsigned)(y - 0x85) <= 0x1e && (unsigned)(x - 0x3c) <= 0x1c) {
        tip_x = 0x39;
        tip_y = 0x79;
        item_info_assign(&item, &g_roulette->addition_item);
    }
    if (item.count != 0) {
        struct String text;
        struct String name = {0};

        ui_window_to_screen(&self->frame.base, &tip_x, &tip_y);
        item_info_get_display_name(&item, &name);
        if (item.identified == 1 && item.option_count != 0) {
            string_init(&text);
            string_format(&text, msg_str(MSG_ITEM_WITH_OPTIONS), string_cstr(&name), item.option_count);
            mode_show_tooltip(client_current_mode(), string_cstr(&text), tip_x, tip_y,
                              session_get_item_dis_color(&item), 0);
            string_free(&text);
        } else {
            mode_show_tooltip(client_current_mode(), string_cstr(&name), tip_x, tip_y,
                              session_get_item_dis_color(&item), 0);
        }
        string_free(&name);
    }
    item_info_destruct(&item);
}

void ui_roullette_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIRoulletteWnd *self = (struct UIRoulletteWnd *)window;
    struct ItemInfo item;

    ui_roullette_wnd_item_at(self, &item, x, y);
    if ((unsigned)(y - 0x8f) <= 0x1e && (unsigned)(x - 0xba) <= 0x1e && g_roulette->has_slot == 1)
        item_info_assign(&item, &g_roulette->slot_item);
    if ((unsigned)(y - 0x85) <= 0x1e && (unsigned)(x - 0x3c) <= 0x1c)
        item_info_assign(&item, &g_roulette->addition_item);
    if (item.count != 0) {
        struct UIWindow *wnd = ui_window_open(ITEM_WND_ID);

        ui_window_send(wnd, NULL, UI_MSG_SHOW_ITEM, (int)&item, 0, 0, 0);
    }
    item_info_destruct(&item);
}

static void point_arrow(struct UIRoulletteWnd *self)
{
    if (g_spriteMode == 2) {
        if (!g_roulette->spinning) {
            ui_window_move(self->start_sprite, 0x153, 0x18e);
            ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)self->start_sprite,
                                      msg_str(MSG_ROULETTE_START_TIP));
            ui_window_move(self->arrow, 0x1e0, 0x151 - (next_spin_offset() + g_roulette->step) * 40);
        }
        if (g_roulette->has_slot == 1 && !g_roulette->slot_flag)
            ui_window_move(self->arrow, 0x1e0, 0x129 - g_roulette->step * 40);
    } else {
        struct UIWindow *wnd = &self->frame.base;

        if (!g_roulette->spinning) {
            int stage = g_roulette->step;
            struct BitmapRes *arrow = ui_skin_bitmap(ROULETTE_ARROW);

            ui_window_draw_bitmap(wnd, 0x1e0, 0x151 - (next_spin_offset() + stage) * 40, arrow, 1);
        }
        if (g_roulette->has_slot == 1 && !g_roulette->slot_flag)
            ui_window_draw_bitmap(wnd, 0x1e0, 0x129 - g_roulette->step * 40, ui_skin_bitmap(ROULETTE_ARROW), 1);
    }
}

static void sprite_played(struct UIRoulletteWnd *self, struct UIWindow *sprite, int id, struct UIBitmapButton *button)
{
    ui_window_set_state(sprite, SPRITE_IDLE);
    ui_window_move(sprite, ROULETTE_HIDDEN, ROULETTE_HIDDEN);
    ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)sprite, "");
    ui_roullette_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, id, 0, 0, 0);
    ui_window_set_state(&button->button.base, UI_BTN_DOWN);
}

void ui_roullette_wnd_on_process(struct UIWindow *window)
{
    struct UIRoulletteWnd *self = (struct UIRoulletteWnd *)window;
    if (g_spriteMode != 2) {
        if (self->start->button.base.state == UI_BTN_HOVER && self->may_spin == 1) {
            point_arrow(self);
            ui_window_set_state(&self->start->button.base, UI_BTN_UP);
        }
        return;
    }
    if (self->start->button.base.state == UI_BTN_HOVER && self->may_spin == 1) {
        point_arrow(self);
        ui_window_set_state(&self->start->button.base, UI_BTN_UP);
    }
    if (self->start_sprite->state == SPRITE_PLAYED)
        sprite_played(self, self->start_sprite, UI_ID_ROULETTE_START, self->start);
    if (self->prize_button->button.base.state == UI_BTN_HOVER) {
        ui_window_move(self->prize_sprite, 0xcb, 0x18e);
        ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)self->prize_sprite, msg_str(MSG_ROULETTE_PRIZE_TIP));
        ui_window_set_state(&self->prize_button->button.base, UI_BTN_UP);
    }
    if (self->prize_sprite->state == SPRITE_PLAYED)
        sprite_played(self, self->prize_sprite, UI_ID_ROULETTE_PRIZE, self->prize_button);
    if (mode_cursor_action() != 2 || g_roulette->spinning == 1) {
        ui_window_move(self->start_sprite, ROULETTE_HIDDEN, ROULETTE_HIDDEN);
        ui_window_move(self->prize_sprite, ROULETTE_HIDDEN, ROULETTE_HIDDEN);
        ui_window_move(self->arrow, ROULETTE_HIDDEN, ROULETTE_HIDDEN);
        ui_window_invalidate_base(&self->frame.base);
    }
    if (g_roulette->has_slot == 1)
        ui_window_invalidate(self->prize_sprite);
    if (!g_roulette->spinning) {
        ui_window_invalidate(self->start_sprite);
        ui_window_invalidate(self->arrow);
    }
}

int ui_roullette_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                      int val2, int val3, int val4)
{
    struct UIRoulletteWnd *self = (struct UIRoulletteWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_CLOSE) {
            if (g_roulette->spinning)
                ui_chat_line(msg_str(MSG_ROULETTE_BUSY), 0x6464ff);
            else if (g_windowMgr.roullette_wnd)
                ui_close_window(ROULETTE_WND_ID);
        } else if (val1 == UI_ID_ROULETTE_START) {
            if (self->may_spin == 1) {
                unsigned short pkt = PKT_CZ_REQ_GENERATE_ROULETTE;

                send_packet(&pkt, rag_connection_get_packet_length(rag_connection_instance(),
                                                                 PKT_CZ_REQ_GENERATE_ROULETTE));
                self->may_spin = 0;
            }
        } else if (val1 == UI_ID_ROULETTE_PRIZE) {
            if (g_roulette->has_slot == 1) {
                struct PACKET_CZ_RECV_ROULETTE_ITEM pkt;

                pkt.packet_type = PKT_CZ_RECV_ROULETTE_ITEM;
                pkt.condition = (BYTE)val2;
                send_packet(&pkt, rag_connection_get_packet_length(rag_connection_instance(),
                                                                PKT_CZ_RECV_ROULETTE_ITEM));
            }
        }
        return 0;
    }
    if (msg == UI_MSG_ROULETTE_REDRAW) {
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_roulette_allow_spin(struct UIWindow *wnd, int allow)
{
    ((struct UIRoulletteWnd *)wnd)->may_spin = (BYTE)allow;
}
