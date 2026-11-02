#include "game/drag.h"
#include "ui/character/character_draw.h"
#include "ui/controls/listbox.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "iteminfo/item_api.h"
#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "actor/appearance.h"
#include "game/context.h"
#include <string.h>
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/controls/controls.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/ui_items.h"
#include "ui/shops/shops.h"
#include "ui/chat/chat_window_parts.h"
#include "ui/character/equip_wnd.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "ui/items/items.h"
#include "ui/quests/npc_input.h"
#include "ui/shops/merchant.h"
#include "ui/wndmgr/wndmgr.h"
#include "session/session_skills.h"
#include "ui/character/popup_menu.h"
#include "ui/draw_key.h"
#include "platform/clock.h"

#define EQUIP_WND_ID 0xa
#define EQUIP_VIEW_WND_ID 0x8b
#define CART_ITEMS_WND_ID 0x28
#define ITEM_INFO_WND_ID 0xc
#define CHOICE_MENU_WND_ID 0x12
#define MERGE_ITEM_WND_ID 0xe3

#define ID_SHOW_EQUIP_BOX 0xcd
#define ID_OFF 0xf0
#define ID_CART_ITEMS 0xf1
#define ID_TITLE_LIST 0x1d8
#define ID_SWAP 0x1f4
#define ID_TAKE_OFF_ALL 0x237

#define EQUIP_MSG_TAB 0x16
#define EQUIP_MSG_RELEASE 0x30
#define EQUIP_MSG_REFRESH 0x3c
#define EQUIP_MSG_UPDATE_BUTTON 0x6a
#define SWAP_MSG_REFRESH 0x3c

#define EQUIP_TAB_EQUIPMENT 0
#define EQUIP_TAB_COSTUME 1
#define EQUIP_TAB_TITLE 2

#define REQ_TAKE_OFF 0x14
#define REQ_RELEASE 0x50
#define REQ_SHOW_EQUIP 0xfd
#define RELEASE_CART 6
#define RELEASE_HIDING 5
#define RELEASE_RUWACH 4
#define RELEASE_SANTA 3
#define RELEASE_TRANSFORM 1

#define DRAG_FROM_EQUIP 2
#define DRAG_FROM_COSTUME 0x18

#define PACKET_TITLE_CHANGE 0xa2e
#define PACKET_TAKE_OFF_ALL 0xbad

#define MENU_MSG_RESET 0x4b
#define MENU_MSG_ADD 0x1f
#define MENU_MSG_SET_OWNER 0x53
#define MENU_MSG_FIT 0x1e

#define MSG_SHOW_EQUIP 0x552
#define MSG_EQUIPMENT_TITLE 0x68
#define MSG_VIEWED_EQUIPMENT 0x551
#define MSG_TAB_EQUIPMENT 0x70d
#define MSG_TAB_COSTUME 0x973
#define MSG_TAB_TITLE 0xa7d
#define MSG_NO_TITLE 0xa7e
#define MSG_RELEASE_HIDING 0x1b9
#define MSG_RELEASE_RUWACH 0x1ba
#define MSG_RELEASE_RUWACH_DORAM 0x830
#define MSG_RELEASE_SANTA 0x5b8
#define MSG_RELEASE_TRANSFORM 0x5b7
#define MSG_RELEASE_CART 0xdf
#define MSG_RELEASE_BOTH 0xe34
#define MSG_CART_ITEMS 0xed
#define MSG_SWAP 0xbaa
#define MSG_TAKE_OFF_ALL 0xf73
#define MSG_CANNOT_NOW 0x20d
#define MSG_ITEM_TIP 0xb7
#define MSG_ITEM_TIP_OPTIONS 0xa28
#define MSG_ITEM_TIP_OPTIONS_COUNT 0x9f7

#define EQUIP_SLOT_HEAD_TOP 8
#define EQUIP_SLOT_HEAD_MID 9
#define EQUIP_SLOT_HEAD_LOW 0
#define EQUIP_SLOT_GARMENT 2
#define EQUIP_SLOT_MIDDLE 15
#define EQUIP_ROW_HEIGHT 0x1a

struct UIEquipWnd {
    struct UIFrameWnd frame;
    int viewing_other;
    int marked[3];
    int full_height;
    struct WndInfoTall *info;
    struct UIBitmapButton *off_button;
    struct UIBitmapButton *cart_button;
    DWORD created;
    int bar_height;
    struct UIWindow *show_box;
    struct UIWindow *tabs;
    int tab;
    int top;
    int job;
    int sex;
    int head;
    int head_low;
    int head_top;
    int head_mid;
    int body_palette;
    int hair_palette;
    int garment;
    int body;
    struct UIWindow *title_list;
    struct UISwapEquipmentWnd *swap_wnd;
    int swap_mode;
    struct UIBitmapButton *swap_button;
    struct UIBitmapButton *take_off_button;
    struct UIAnimation animation;
    unsigned long long drawn_key;
};

#define ITEM_LOCATION(info) ((info)->worn_location)

void ui_equip_wnd_draw_special_slots(struct UIWindow *self);

static BYTE equip_nothing_to_release(void)
{
    if (g_session1.cartType > 0)
        return 0;
    if (effect_state_bit4(g_session1.effectState) || effect_state_bit5(g_session1.effectState) ||
        effect_state_bit19(g_session1.effectState) || effect_state_bit23(g_session1.effectState) ||
        effect_state_bit24(g_session1.effectState) || effect_state_bit25(g_session1.effectState) ||
        effect_state_bit26(g_session1.effectState) || effect_state_bit22(g_session1.effectState))
        return 0;
    return 1;
}

static int ruwach_doram_job(int job)
{
    return job == 0xff2 || job == 0xff3 || job == 0x100e || job == 0x10b9;
}

static int transformed(void)
{
    return effect_state_bit19(g_session1.effectState) || effect_state_bit23(g_session1.effectState) ||
           effect_state_bit24(g_session1.effectState) || effect_state_bit25(g_session1.effectState) ||
           effect_state_bit26(g_session1.effectState);
}

static void equip_release(int what)
{
    struct Mode *mode = client_current_mode();
    int kind = 0;

    if (what == 0 && g_session1.cartType > 0) {
        mode_send_message(mode, REQ_RELEASE, RELEASE_CART, 0, 0, 0);
        return;
    }
    if (effect_state_bit4(g_session1.effectState))
        kind = RELEASE_HIDING;
    else if (effect_state_bit5(g_session1.effectState))
        kind = RELEASE_RUWACH;
    else if (effect_state_bit22(g_session1.effectState))
        kind = RELEASE_SANTA;
    else if (transformed())
        kind = RELEASE_TRANSFORM;
    mode_send_message(mode, REQ_RELEASE, kind, 0, 0, 0);
}

static struct ItemInfo *equip_item_in_slot(struct UIEquipWnd *self, struct ItemInfo *out, int slot)
{
    if (slot == -1)
        return item_info_construct_with_arg(out);
    if (self->tab == EQUIP_TAB_COSTUME) {
        if (self->viewing_other)
            return session_get_viewed_costume(&g_session1, out, slot);
        return session_get_worn_costume(&g_session1, out, slot);
    }
    if (self->tab == EQUIP_TAB_EQUIPMENT) {
        if (self->viewing_other)
            return session_get_viewed_equip(&g_session1, out, slot);
        return session_get_equipped_item(&g_session1, out, slot);
    }
    return item_info_construct_with_arg(out);
}

static int equip_slot_of(struct UIEquipWnd *self, int col, int row, int in_column, int x, int y)
{
    int slot = -1;
    int half = self->frame.base.width / 2;

    if (in_column) {
        if (col != 0)
            slot = row == 0 ? EQUIP_SLOT_HEAD_MID : row + 3;
        else
            slot = row == 0 ? EQUIP_SLOT_HEAD_TOP : row - 1;
    }
    if (half - 0x19 < x && x < half - 1 && self->top + 0x2c < y && y < self->top + 0x44)
        slot = EQUIP_SLOT_MIDDLE;
    return slot;
}

static struct ItemInfo *equip_item_at_icon(struct UIEquipWnd *self, struct ItemInfo *out, int x, int y)
{
    int top = self->top;
    int in_column;

    if (y < top + 0x11 || y > top + 0x91)
        return item_info_construct_with_arg(out);
    in_column = (x >= 6 && x < 0x1e) || (unsigned)(x - 0xfb) <= 0x17;
    return equip_item_in_slot(self, out,
                              equip_slot_of(self, x / 0xfb, (y - top - 0x11) / EQUIP_ROW_HEIGHT, in_column, x, y));
}

static struct ItemInfo *equip_item_at(struct UIEquipWnd *self, struct ItemInfo *out,
                                                 int x, int y)
{
    int top = self->top;
    int in_column;

    if (y < top + 0x11 || y > top + 0x91)
        return item_info_construct_with_arg(out);
    in_column = (x >= 6 && x < 0x6a) || (x >= 0xaa && x < self->frame.base.width - 6);
    return equip_item_in_slot(self, out,
                              equip_slot_of(self, x / 0xaa, (y - top - 0x11) / EQUIP_ROW_HEIGHT, in_column, x, y));
}

static void equip_update_look(struct UIEquipWnd *self)
{
    struct ItemInfo top, mid, low, low_item, garment;

    if (!self->viewing_other) {
        struct PlayerAppearance appearance;

        self->job = session_get_drawn_job(&g_session1);
        self->sex = session_get_sex(&g_session1);
        self->head = g_session1.hairStyle;
        self->body_palette = g_session1.clothesColor;
        self->hair_palette = g_session1.hairColor;
        if (player_get_appearance(game_current_player(), &appearance))
            self->body = appearance.body_style;
    } else {
        self->job = g_session1.viewedLook[0];
        self->sex = g_session1.viewedLook[1];
        self->head = g_session1.viewedLook[2];
        self->body_palette = g_session1.viewedLook[6];
        self->hair_palette = g_session1.viewedLook[7];
        self->body = g_session1.viewedBody;
    }
    self->head_low = 0;
    self->head_top = 0;
    self->head_mid = 0;
    self->garment = 0;

    equip_item_in_slot(self, &top, EQUIP_SLOT_HEAD_TOP);
    if (top.index)
        self->head_top = top.view;
    equip_item_in_slot(self, &mid, EQUIP_SLOT_HEAD_MID);
    if (mid.index && top.index != mid.index)
        self->head_mid = mid.view;
    equip_item_in_slot(self, &low, EQUIP_SLOT_HEAD_LOW);
    if (low.index && top.index != low.index && mid.index != low.index)
        self->head_low = low.view;
    item_info_construct_with_arg(&garment);
    equip_item_in_slot(self, &low_item, EQUIP_SLOT_GARMENT);
    item_info_assign(&garment, &low_item);
    item_info_destruct(&low_item);
    if (garment.index)
        self->garment = garment.view;
    item_info_destruct(&garment);
    item_info_destruct(&low);
    item_info_destruct(&mid);
    item_info_destruct(&top);
}

static void equip_fill_titles(struct UIEquipWnd *self)
{
    int *id;

    ui_list_box_id_clear_items((struct UIListBoxEx *)self->title_list);
    ui_list_box_id_add_item((struct UIListBoxID *)self->title_list, msg_str(MSG_NO_TITLE), 0);
    for (id = g_session1.titles.first; id != g_session1.titles.last; id++) {
        struct String name;

        session_get_title_string(&g_session1, &name, *id);
        ui_list_box_id_add_item((struct UIListBoxID *)self->title_list, string_cstr(&name), *id);
        string_free(&name);
    }
}

static void equip_place_buttons(struct UIEquipWnd *self, int bottom)
{
    struct UIWindow *wnd = &self->frame.base;

    ui_window_move_base(&self->off_button->button.base, wnd->width / 2 - 0x12, bottom - 0x28 + equip_nothing_to_release() * 0xe6);
    ui_window_move_base(&self->cart_button->button.base, wnd->width / 2 - 0xe, bottom + (g_session1.cartType > 0 ? -0x3c : 0xaa));
}

void ui_equip_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;
    ui_frame_wnd_move((struct UIWindow *)(&self->frame), x, y);
}

void ui_equip_wnd_store_info(struct UIWindow *window)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;
    struct WndInfoTall *info = self->info;

    if (!info)
        return;
    info->rect.width = self->frame.base.width;
    info->rect.height = self->frame.base.height;
    info->rect.x = self->frame.base.x;
    info->rect.y = self->frame.base.y;
    info->org_height = self->full_height;
}

static void equip_button_bitmaps(struct UIBitmapButton *button, const struct String *dir, const char *up,
                                 const char *hover, const char *down)
{
    ui_button_bitmap(button, dir, up, 0);
    ui_button_bitmap(button, dir, hover, 1);
    ui_button_bitmap(button, dir, down, 2);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
}

static struct UIBitmapButton *equip_new_button(void)
{
    void *mem = mem_alloc(sizeof(struct UIBitmapButton));

    return mem ? ui_bitmap_button_construct(mem) : NULL;
}

struct DragInfo;
void ui_equip_wnd_drag_and_drop(struct UIWindow *self, int x, int y, const struct DragInfo *drag);
void ui_equip_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_equip_wnd_on_process(struct UIWindow *self);
void ui_equip_wnd_on_draw(struct UIWindow *self);
BYTE ui_equip_wnd_render(struct UIWindow *self, int force);
void ui_equip_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_equip_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_equip_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_equip_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_equip_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_equip_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_equip_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_equip_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps ui_equip_wnd_ops = {
    .destroy = ui_equip_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
    .move = ui_equip_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_equip_wnd_drag_and_drop,
    .store_info = ui_equip_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_equip_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_equip_wnd_on_process,
    .on_draw = ui_equip_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_equip_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_equip_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_equip_wnd_on_mouse_move,
    .on_mouse_hover = ui_equip_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_equip_wnd_on_l_btn_up,
    .on_r_btn_down = ui_equip_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_equip_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_equip_wnd_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIEquipWnd *ui_equip_wnd_construct(struct UIEquipWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_equip_wnd_ops;
    self->frame.base.redraws_tree = 1;
    ui_animation_reset(&self->animation);
    self->drawn_key = 0;
    self->swap_wnd = NULL;
    self->swap_mode = 0;
    self->swap_button = NULL;
    self->take_off_button = NULL;
    self->info = NULL;
    self->created = platform_ticks_ms();
    self->bar_height = 0;
    self->tab = 0;
    self->top = 0x14;
    self->job = 0;
    self->sex = 0;
    self->head = 0;
    self->head_low = 0;
    self->head_top = 0;
    self->head_mid = 0;
    self->body_palette = 0;
    self->hair_palette = 0;
    self->body = 0;
    self->swap_button = NULL;
    return self;
}

struct UIWindow *ui_equip_wnd_new(void)
{
    return (struct UIWindow *)ui_equip_wnd_construct(mem_alloc(sizeof(struct UIEquipWnd)));
}

void ui_equip_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String close_key, mini_key, dir;
    int extra;

    (void)width;
    if (!self->viewing_other) {
        self->bar_height = 0x14;
        extra = 0x14;
    } else {
        self->bar_height = 0;
        extra = 0;
    }
    if (height != extra + wnd->height)
        ui_window_resize_base(wnd, wnd->width, extra + wnd->height);

    if (!self->viewing_other) {
        struct UIWindow *box = mem_alloc(sizeof(struct UICheckBox));

        box = box ? (struct UIWindow *)ui_check_box_construct(box) : NULL;
        self->show_box = box;
        ui_window_create(box, 0x50, 0xc);
        ui_check_box_set_bitmaps(box, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp", msg_str(MSG_SHOW_EQUIP));
        ui_window_move_base(box, 10, wnd->height - 0xf);
        ui_check_box_set_check(box, g_session1.equipOpenToOthers);
        ui_window_add_child(wnd, box);
    }

    string_init(&close_key);
    string_assign_cstr(&close_key, "Alt+Q");
    string_init(&mini_key);
    string_assign_cstr(&mini_key, "Ctrl+Q");
    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, string_cstr(&close_key), 0);
    ui_window_add_mini_button(wnd, string_cstr(&mini_key), 0);

    if (!self->viewing_other) {
        const char *tip = NULL;
        int x, bottom;

        string_init(&dir);
        string_assign(&dir, UI_SKIN "basic_interface\\", 0x1f);
        x = wnd->width / 2 - 0x12;
        bottom = wnd->height - self->bar_height;
        self->off_button = equip_new_button();
        equip_button_bitmaps(self->off_button, &dir, "btn_off", "btn_off", "btn_off");
        ui_window_move_base(&self->off_button->button.base, x, bottom - 0x28 + equip_nothing_to_release() * 0xe6);
        self->off_button->button.base.id = ID_OFF;
        if (effect_state_bit4(g_session1.effectState))
            tip = msg_str(MSG_RELEASE_HIDING);
        else if (effect_state_bit5(g_session1.effectState))
            tip = msg_str(ruwach_doram_job(session_get_drawn_job(&g_session1)) ? MSG_RELEASE_RUWACH_DORAM
                                                                                  : MSG_RELEASE_RUWACH);
        else if (effect_state_bit22(g_session1.effectState))
            tip = msg_str(MSG_RELEASE_SANTA);
        else if (transformed())
            tip = msg_str(MSG_RELEASE_TRANSFORM);
        if (g_session1.cartType > 0)
            tip = msg_str(tip ? MSG_RELEASE_BOTH : MSG_RELEASE_CART);
        ui_bitmap_button_set_tool_tip(self->off_button, tip);
        ui_window_add_child(wnd, &self->off_button->button.base);
        string_free(&dir);

        if (!self->viewing_other) {
            string_init(&dir);
            string_assign(&dir, UI_SKIN "basic_interface\\", 0x1f);
            x = wnd->width / 2 - 0xe;
            bottom = wnd->height - self->bar_height;
            self->cart_button = equip_new_button();
            equip_button_bitmaps(self->cart_button, &dir, "btn_items_off", "btn_items_on", "btn_items_on");
            ui_window_move_base(&self->cart_button->button.base, x, bottom + (g_session1.cartType > 0 ? -0x3c : 0xaa));
            self->cart_button->button.base.id = ID_CART_ITEMS;
            ui_bitmap_button_set_tool_tip(self->cart_button, g_session1.cartType > 0 ? msg_str(MSG_CART_ITEMS) : "");
            ui_window_add_child(wnd, &self->cart_button->button.base);
            string_free(&dir);
        }
    }

    self->full_height = wnd->height;
    self->marked[0] = -1;
    self->marked[1] = -1;
    self->marked[2] = -1;

    {
        struct UIWindow *tabs = mem_alloc(sizeof(struct UITabControl));

        tabs = tabs ? ui_tab_control_construct(tabs) : NULL;
        self->tabs = tabs;
        ui_tab_control_setup(tabs, 1, 0x11, 0x14, 1, 0, 0, 0, 0, 0, 0, 0, 0);
        ui_window_move(tabs, 0, 0x11);
        ui_window_add_child(wnd, tabs);
        ui_tab_control_add_tab(tabs, msg_str(MSG_TAB_EQUIPMENT));
        ui_tab_control_add_tab(tabs, msg_str(MSG_TAB_COSTUME));
        if (!self->viewing_other)
            ui_tab_control_add_tab(tabs, msg_str(MSG_TAB_TITLE));
    }

    {
        struct UIWindow *list = mem_alloc(sizeof(struct UIListBoxTitle));

        list = list ? (struct UIWindow *)ui_list_box_title_construct((struct UIListBoxTitle *)list) : NULL;
        self->title_list = list;
        ui_list_box_ex_create((struct UIListBoxEx *)list, 3, self->top + 0x27, wnd->width - 6, 0x69, 0);
        ((struct UIListBoxEx *)list)->r = 0xe1;
        ((struct UIListBoxEx *)list)->g = 0xe1;
        ((struct UIListBoxEx *)list)->b = 0xe1;
        list->id = ID_TITLE_LIST;
        ui_window_set_show_base(list, 0);
        ui_window_add_child(wnd, list);
    }

    if (self->viewing_other == 0) {
        if (!self->swap_button) {
            self->swap_button = equip_new_button();
            ui_bitmap_button_set_bitmap(self->swap_button, UI_SKIN "basic_interface\\btn_e_change_a.bmp", 0, 0);
            ui_bitmap_button_set_bitmap(self->swap_button, UI_SKIN "basic_interface\\btn_e_change_b.bmp", 1, 0);
            ui_bitmap_button_set_bitmap(self->swap_button, UI_SKIN "basic_interface\\btn_e_change_b.bmp", 2, 0);
            ui_window_create(&self->swap_button->button.base, self->swap_button->bitmap_width,
                             self->swap_button->bitmap_height);
            ui_window_move_base(&self->swap_button->button.base, 0xc1, 0xa7);
            self->swap_button->button.base.id = ID_SWAP;
            ui_bitmap_button_set_tool_tip(self->swap_button, msg_str(MSG_SWAP));
            ui_window_add_child(wnd, &self->swap_button->button.base);
        }
        if (g_session1.equipSwapOpen == 1)
            ui_equip_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, ID_SWAP, 0, 0, 0);
    }

    self->take_off_button = equip_new_button();
    ui_bitmap_button_set_bitmap(self->take_off_button, UI_SKIN "basic_interface\\btn_e_off_a.bmp", 0, 0);
    ui_bitmap_button_set_bitmap(self->take_off_button, UI_SKIN "basic_interface\\btn_e_off_b.bmp", 1, 0);
    ui_bitmap_button_set_bitmap(self->take_off_button, UI_SKIN "basic_interface\\btn_e_off_b.bmp", 2, 0);
    ui_bitmap_button_set_bitmap(self->take_off_button, UI_SKIN "basic_interface\\btn_e_off_c.bmp", UI_BMP_DISABLED,
                             0);
    ui_window_create(&self->take_off_button->button.base, self->take_off_button->bitmap_width,
                     self->take_off_button->bitmap_height);
    ui_window_move_base(&self->take_off_button->button.base, 0xeb, 0xa7);
    self->take_off_button->button.base.id = ID_TAKE_OFF_ALL;
    ui_bitmap_button_set_tool_tip(self->take_off_button, msg_str(MSG_TAKE_OFF_ALL));
    ui_window_add_child(wnd, &self->take_off_button->button.base);
    ui_equip_wnd_send_msg(wnd, wnd, EQUIP_MSG_UPDATE_BUTTON, ID_TAKE_OFF_ALL, 0, 0, 0);

    string_free(&mini_key);
    string_free(&close_key);
}

void ui_equip_wnd_on_process(struct UIWindow *self)
{
}

static unsigned long long equip_key(struct UIEquipWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    unsigned long long key = DRAW_KEY_START;
    int i;

    key = draw_key_int(key, wnd->width);
    key = draw_key_int(key, wnd->height);
    key = draw_key_int(key, self->viewing_other);
    key = draw_key_int(key, self->tab);
    key = draw_key_int(key, self->top);
    key = draw_key_bytes(key, self->marked, sizeof self->marked);
    if (self->viewing_other) {
        key = draw_key_bytes(key, g_session1.viewedName, sizeof g_session1.viewedName);
        key = draw_key_bytes(key, g_session1.viewedLook, sizeof g_session1.viewedLook);
        key = draw_key_int(key, g_session1.viewedBody);
    } else {
        struct PlayerAppearance appearance;

        key = draw_key_int(key, session_get_drawn_job(&g_session1));
        key = draw_key_int(key, session_get_sex(&g_session1));
        key = draw_key_int(key, g_session1.hairStyle);
        key = draw_key_int(key, g_session1.clothesColor);
        key = draw_key_int(key, g_session1.hairColor);
        if (player_get_appearance(game_current_player(), &appearance))
            key = draw_key_int(key, appearance.body_style);
        else
            key = draw_key_int(key, self->body);
    }
    key = draw_key_int(key, g_session1.madogear);
    key = draw_key_int(key, g_session1.ownTitleId);
    for (i = 0; i < 10; i++) {
        struct ItemInfo item;

        equip_item_in_slot(self, &item, i);
        key = draw_key_item(key, &item);
        item_info_destruct(&item);
    }
    if (!self->viewing_other && self->tab == EQUIP_TAB_EQUIPMENT) {
        struct ItemInfo item;

        session_get_inventory_item(&g_session1, &item, g_session1.wornArrowIndex);
        key = draw_key_item(key, &item);
        item_info_destruct(&item);
    }
    if (self->swap_wnd && self->swap_wnd->frame.base.state)
        key = ui_swap_equipment_wnd_key(self->swap_wnd, key);
    return draw_key_children(key, wnd);
}

BYTE ui_equip_wnd_render(struct UIWindow *window, int force)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;

    if (equip_key(self) != self->drawn_key || ui_animation_due(&self->animation)
        || (self->swap_wnd && self->swap_wnd->frame.base.state && ui_animation_due(&self->swap_wnd->animation)))
        ui_window_invalidate_base(window);
    return ui_window_render_base(window, force);
}

static void equip_draw(struct UIEquipWnd *self)
{
    static const int mark_x[2] = {4, 0xf8};
    struct UIWindow *wnd = &self->frame.base;
    struct BitmapRes *bmp;
    const char *title;
    char buf[0x7c];

    bmp = ui_skin_bitmap(UI_SKIN "basic_interface\\titlebar_fix.bmp");
    if (bmp)
        ui_window_draw_bitmap(wnd, 0, 0, bmp, 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
    ui_window_fill_rect(wnd, self->tabs->width, 0x22, wnd->width - self->tabs->width, 1, RGB(0xc3, 0xc3, 0xc3));
    if (self->viewing_other == 1) {
        sprintf(buf, msg_str(MSG_VIEWED_EQUIPMENT), g_session1.viewedName);
        title = buf;
        ui_window_text_out(wnd, 0x11, 4, title, 0, 0, 0xc, RGB(0xff, 0xff, 0xff), 0, 0);
    } else {
        ui_window_text_out(wnd, 0x11, 4, msg_str(MSG_EQUIPMENT_TITLE), 0, 0, 0xc, RGB(0xff, 0xff, 0xff), 0, 0);
        title = msg_str(MSG_EQUIPMENT_TITLE);
    }
    ui_window_text_out(wnd, 0x10, 3, title, 0, 0, 0xc, 0, 0, 0);
    if (wnd->height == 0x11)
        return;

    bmp = ui_skin_bitmap(self->viewing_other == 0 ? UI_SKIN "basic_interface\\equipwin_bg2.bmp"
                                                  : UI_SKIN "basic_interface\\equipwin_bg.bmp");
    if (bmp)
        ui_window_draw_bitmap(wnd, 0, self->top + 0x11, bmp, 0);

    if (self->tab == EQUIP_TAB_TITLE) {
        struct String name;

        ui_window_fill_rect(wnd, 1, self->top + 0xf, wnd->width - 2, 0x83, RGB(0xff, 0xff, 0xff));
        ui_window_draw_outline(wnd, 2, self->top + 0x26, wnd->width - 4, 0x6b, 0x646464);
        session_get_title_string(&g_session1, &name, g_session1.ownTitleId);
        ui_window_text_out_centred(wnd, 1, self->top + 0x14, string_cstr(&name), wnd->width - 2, 0, 0xd, 0, 1);
        ui_button_set_show(&self->cart_button->button, 1);
        ui_button_set_show(&self->off_button->button, 1);
        string_free(&name);
    } else {
        int i;

        bmp = ui_skin_bitmap(UI_SKIN "basic_interface\\item_invert.bmp");
        if (bmp) {
            for (i = 0; i < 3; i++) {
                int slot = self->marked[i];

                if ((unsigned)slot <= 7)
                    ui_window_draw_bitmap(wnd, mark_x[slot / 4], self->top + 0x33 + slot % 4 * EQUIP_ROW_HEIGHT,
                                        bmp, 1);
                else if (slot == EQUIP_SLOT_HEAD_TOP || slot == EQUIP_SLOT_HEAD_MID)
                    ui_window_draw_bitmap(wnd, mark_x[slot % 4], self->top + 0x19, bmp, 1);
            }
        }
        ui_equip_wnd_draw_special_slots(&self->frame.base);
        equip_update_look(self);
        struct CharacterDrawOptions options = {
            .x = wnd->width / 2 + 1,
            .y = 0x95,
            .sex = (BYTE)self->sex,
            .job = (short)self->job,
            .body = (short)self->body,
            .head = (short)self->head,
            .accessory = (short)self->head_low,
            .accessory2 = (short)self->head_top,
            .accessory3 = (short)self->head_mid,
            .robe = self->garment,
            .riding = 0,
            .scaled = g_session1.madogear,
            .body_palette = self->body_palette,
            .head_palette = self->hair_palette,
            .action = 0,
            .motion = 0,
            .start_time = self->created,
            .animation = &self->animation,
        };
        ui_draw_character(wnd, &options, 0);

        if (self->viewing_other == 0) {
            if (self->tab == EQUIP_TAB_EQUIPMENT) {
                struct ItemInfo item;

                session_get_inventory_item(&g_session1, &item, g_session1.wornArrowIndex);
                if (item.index) {
                    ui_window_draw_item_icon(wnd, wnd->width / 2 - 0x19, self->top + 0x2c, &item, 0);
                    ui_window_draw_item_name_short(wnd, wnd->width / 2 - 0x19, self->top + 0x44, 0x28, &item,
                                               0xc);
                }
                ui_button_set_show(&self->cart_button->button, 1);
                ui_button_set_show(&self->off_button->button, 1);
                item_info_destruct(&item);
            } else {
                ui_button_set_show(&self->cart_button->button, 0);
                ui_button_set_show(&self->off_button->button, 0);
            }
        }
    }
    if (self->swap_wnd)
        ui_swap_equipment_wnd_send_msg(&self->swap_wnd->frame.base, NULL, SWAP_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_equip_wnd_on_draw(struct UIWindow *window)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;

    ui_animation_reset(&self->animation);
    equip_draw(self);
    self->drawn_key = equip_key(self);
}

static void equip_link_item(const struct ItemInfo *info)
{
    struct UIWindow *edit;

    if (!((GetAsyncKeyState(VK_SHIFT) >> 8) & 0xff))
        return;
    if (!ui_window_mgr_get_focus_edit(&g_windowMgr) || !info->identified)
        return;
    edit = ui_window_mgr_get_focus_edit(&g_windowMgr);
    if (edit->id == EDIT_ID_MEGAPHONE) {
        megaphone_wnd_add_item_link(g_windowMgr.npc_text_edit_dialog_wnd, info);
    } else if (ui_window_mgr_get_focus_edit(&g_windowMgr)->id == EDIT_ID_WHISPER) {
        ui_chat_edit_ctrl_add_item_link(ui_window_mgr_get_focus_edit(&g_windowMgr), info);
    } else if (ui_window_mgr_get_focus_edit(&g_windowMgr)->id == EDIT_ID_CHAT) {
        ui_chat_edit_ctrl_add_item_link(ui_chat_window_edit(g_windowMgr.new_chat_wnd), info);
    } else if (ui_window_mgr_get_focus_edit(&g_windowMgr)->id == EDIT_ID_RODEX) {
        ui_rodex_compose_wnd_add_item_link(g_windowMgr.rodex_compose_wnd, info);
    } else if (ui_window_mgr_get_focus_edit(&g_windowMgr)->id == EDIT_ID_CHAT_ROOM) {
        ui_chat_edit_ctrl_add_item_link(ui_window_mgr_get_focus_edit(&g_windowMgr), info);
    }
}

void ui_equip_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;
    struct ItemInfo item;

    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
    if (self->viewing_other)
        return;
    equip_item_at_icon(self, &item, x, y);
    if (item.index) {
        if (item.count > 0) {
            struct GameMode *mode = game_current_mode();
            int slots[3];

            session_equip_slots_of(slots, ITEM_LOCATION(&item));
            game_mode_drag_item(mode, self->tab == EQUIP_TAB_COSTUME ? DRAG_FROM_COSTUME : DRAG_FROM_EQUIP, &item,
                              item.count, slots[2]);
        }
        equip_link_item(&item);
    }
    item_info_destruct(&item);
}

void ui_equip_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;
    struct ItemInfo item;

    if (y < 0x11)
        ui_equip_wnd_send_msg(&self->frame.base, &self->frame.base, UI_MSG_COMMAND, UI_ID_HELP, 0, 0, 0);
    if (self->viewing_other)
        return;
    equip_item_at_icon(self, &item, x, y);
    if (item.index && item.count > 0) {
        int slots[3];

        session_equip_slots_of(slots, ITEM_LOCATION(&item));
        mode_send_message(client_current_mode(), REQ_TAKE_OFF, slots[2], self->tab == EQUIP_TAB_COSTUME, 0, 0);
    }
    item_info_destruct(&item);
}

void ui_equip_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;
    ui_frame_wnd_on_mouse_move((struct UIWindow *)(&self->frame), x, y);
    ui_equip_wnd_on_mouse_hover(&self->frame.base, x, y);
}

void ui_equip_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct ItemInfo item;
    struct Mode *mode;
    int old[3], inventory_index;

    equip_item_at(self, &item, x, y);
    if (item.index) {
        struct String text;
        struct String name = {0};
        int tip_x = 6;
        int tip_y = (y - self->top - 0x11) / 24 * EQUIP_ROW_HEIGHT - 3 + self->top;

        item.identified = 1;
        if (x >= 0xaa)
            tip_x = 0xaf;
        else if (x >= 0x6a)
            tip_x = wnd->width / 2 - 0x14;
        ui_window_to_screen(wnd, &tip_x, &tip_y);
        mode = client_current_mode();
        if (item.count <= 1) {
            if (item.identified == 1 && item.option_count) {
                item_info_get_display_name(&item, &name);
                string_init(&text);
                string_format(&text, msg_str(MSG_ITEM_TIP_OPTIONS), string_cstr(&name), item.option_count);
                mode_show_tooltip(mode, string_cstr(&text), tip_x, tip_y,
                                  session_get_item_dis_color(&item), 0);
                string_free(&text);
                string_free(&name);
            } else {
                item_info_get_display_name(&item, &name);
                mode_show_tooltip(mode, string_cstr(&name), tip_x, tip_y,
                                  session_get_item_dis_color(&item), 0);
                string_free(&name);
            }
        } else if (item.identified == 1 && item.option_count) {
            item_info_get_display_name(&item, &name);
            string_init(&text);
            string_format(&text, msg_str(MSG_ITEM_TIP_OPTIONS_COUNT), string_cstr(&name), item.option_count,
                          item.count);
            mode_show_tooltip(mode, string_cstr(&text), tip_x, tip_y,
                              session_get_item_dis_color(&item), 0);
            string_free(&text);
            string_free(&name);
        } else {
            item_info_get_display_name(&item, &name);
            string_init(&text);
            string_format(&text, msg_str(MSG_ITEM_TIP), string_cstr(&name), item.count);
            mode_show_tooltip(mode, string_cstr(&text), tip_x, tip_y,
                              session_get_item_dis_color(&item), 0);
            string_free(&text);
            string_free(&name);
        }
    }

    old[0] = self->marked[0];
    old[1] = self->marked[1];
    old[2] = self->marked[2];
    self->marked[0] = -1;
    self->marked[1] = -1;
    self->marked[2] = -1;
    if (game_inventory_drag_index(&inventory_index) && x >= 0 && y >= 0 && x < wnd->width && y < wnd->height) {
        struct ItemInfo dragged;

        session_get_inventory_item(&g_session1, &dragged, inventory_index);
        if (dragged.index && dragged.equip_location) {
            int slots[3];

            session_equip_slots_of(slots, dragged.equip_location);
            self->marked[0] = slots[2];
            if (slots[1] != -1)
                self->marked[1] = slots[1];
            if (slots[0] != -1)
                self->marked[2] = slots[0];
        }
        item_info_destruct(&dragged);
    }
    if (old[0] != self->marked[0] || old[1] != self->marked[1] || old[2] != self->marked[2])
        ui_window_invalidate_base(wnd);
    item_info_destruct(&item);
}

void ui_equip_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;
    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
}

void ui_equip_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;
    struct ItemInfo item;

    equip_item_at(self, &item, x, y);
    if (item.index) {
        struct UIWindow *info_wnd = ui_window_open(ITEM_INFO_WND_ID);

        ui_window_send(info_wnd, NULL, UI_MSG_SHOW_ITEM, (int)&item, 0, 0, 0);
    }
    item_info_destruct(&item);
}

static void equip_on_off_button(struct UIEquipWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct String what;
    BYTE has_cart;

    if (g_windowMgr.merchant_item_my_shop_wnd) {
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_chat_line(msg_str(MSG_CANNOT_NOW), RGB(0xff, 0, 0));
        return;
    }
    string_init(&what);
    has_cart = g_session1.cartType > 0;
    if (effect_state_bit5(g_session1.effectState)) {
        int job = session_get_drawn_job(&g_session1);

        if (job == 0xff2 || session_get_drawn_job(&g_session1) == 0xff3 ||
            session_get_drawn_job(&g_session1) == 0x100e)
            string_assign_cstr(&what, msg_str(MSG_RELEASE_RUWACH_DORAM));
        else
            string_assign_cstr(&what, msg_str(MSG_RELEASE_RUWACH));
    } else if (effect_state_bit22(g_session1.effectState)) {
        string_assign_cstr(&what, msg_str(MSG_RELEASE_SANTA));
    } else if (transformed()) {
        string_assign_cstr(&what, msg_str(MSG_RELEASE_TRANSFORM));
    } else {
        string_clear(&what);
    }

    if (has_cart && what.size) {
        struct UIWindow *menu;
        int menu_x, menu_y;

        if (g_windowMgr.menu_wnd)
            ui_close_window(CHOICE_MENU_WND_ID);
        menu = ui_window_open(CHOICE_MENU_WND_ID);
        if (!menu) {
            string_free(&what);
            return;
        }
        ui_window_send_msg(menu, NULL, MENU_MSG_RESET, 0, 0, 0, 0);
        ui_window_send_msg(menu, NULL, MENU_MSG_ADD, (int)"카트 해제", 0, 0, 0);
        ui_window_send_msg(menu, NULL, MENU_MSG_ADD, (int)string_cstr(&what), 0, 0, 0);
        ui_window_send_msg(menu, NULL, MENU_MSG_SET_OWNER, 0xa, 0, 0, 0);
        ui_window_send_msg(menu, NULL, MENU_MSG_FIT, 0, 0, 0, 0);
        ui_menu_set_hold(menu);
        menu_x = g_mouse.x + 1;
        if (menu_x < 1)
            menu_x = 1;
        if (menu_x > g_renderer->width - menu->width)
            menu_x = g_renderer->width - menu->width;
        menu_y = g_mouse.y + 1;
        if (menu_y < 1)
            menu_y = 1;
        if (menu_y > g_renderer->height - menu->height)
            menu_y = g_renderer->height - menu->height;
        ui_window_move(menu, menu_x, menu_y);
    } else {
        equip_release(0);
    }
    equip_place_buttons(self, wnd->height - self->bar_height);
    ui_window_invalidate_base(wnd);
    string_free(&what);
}

static void equip_on_swap_button(struct UIEquipWnd *self, struct UIWindow *sender)
{
    struct UIWindow *wnd = &self->frame.base;
    int height = self->bar_height + 0xa7;

    ui_window_resize_base(wnd, wnd->width, height + 0x9e);
    if (!self->swap_wnd) {
        struct UISwapEquipmentWnd *swap = mem_try_alloc(sizeof(struct UISwapEquipmentWnd));

        swap = swap ? ui_swap_equipment_wnd_construct(swap) : NULL;
        self->swap_wnd = swap;
        ui_window_create(&swap->frame.base, 0x118, 0xb3);
        if (self->tab == EQUIP_TAB_EQUIPMENT)
            self->swap_wnd->kind = 1;
        else if (self->tab == EQUIP_TAB_COSTUME)
            self->swap_wnd->kind = 2;
        ui_window_add_child(wnd, &self->swap_wnd->frame.base);
    }
    if (self->swap_wnd->frame.base.state == 0) {
        ui_frame_wnd_move(&self->swap_wnd->frame.base, 0, 0xa7);
        self->swap_wnd->frame.base.state = 1;
        if (sender == &self->swap_button->button.base)
            g_session1.equipSwapOpen = 1;
    } else {
        ui_frame_wnd_move(&self->swap_wnd->frame.base, -1000, -1000);
        self->swap_wnd->frame.base.state = 0;
        ui_window_resize_base(wnd, wnd->width, height);
    }
}

#pragma pack(push, 1)
struct EquipPacketTitle {
    short id;
    int title;
};
#pragma pack(pop)

static void equip_on_command(struct UIEquipWnd *self, struct UIWindow *sender, int id, int val2)
{
    struct UIWindow *wnd = &self->frame.base;

    switch (id) {
    case UI_ID_CLOSE:
        ui_close_window(self->viewing_other == 1 ? EQUIP_VIEW_WND_ID : EQUIP_WND_ID);
        break;
    case UI_ID_HELP: {
        int old_height = wnd->height;

        if (old_height == 0x11) {
            ui_window_resize_base(wnd, wnd->width, self->full_height);
        } else {
            self->full_height = old_height;
            ui_window_resize_base(wnd, wnd->width, 0x11);
        }
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        ui_window_mgr_raise_window(&g_windowMgr, wnd);
        if (wnd->height - old_height)
            snap_mgr_move_chain(&g_snapMgr, wnd, 0, wnd->height - old_height, 3);
        break;
    }
    case ID_SHOW_EQUIP_BOX:
        if (!self->viewing_other)
            mode_send_message(client_current_mode(), REQ_SHOW_EQUIP, val2, 0, 0, 0);
        break;
    case ID_OFF:
        equip_on_off_button(self);
        break;
    case ID_CART_ITEMS:

        if (ui_get_window(MERGE_ITEM_WND_ID))
            break;
        if (ui_close_window(CART_ITEMS_WND_ID))
            break;
        if (g_session1.cartType > 0)
            ui_window_open(CART_ITEMS_WND_ID);
        break;
    case ID_TITLE_LIST: {
        struct EquipPacketTitle packet;

        packet.id = PACKET_TITLE_CHANGE;
        packet.title = val2;
        rag_connection_send_packet(rag_connection_instance(),
                                  rag_connection_get_packet_length(rag_connection_instance(), PACKET_TITLE_CHANGE),
                                  &packet);
        break;
    }
    case ID_SWAP:
        equip_on_swap_button(self, sender);
        break;
    case ID_TAKE_OFF_ALL: {
        short packet = PACKET_TAKE_OFF_ALL;

        rag_connection_send_packet(rag_connection_instance(),
                                  rag_connection_get_packet_length(rag_connection_instance(), PACKET_TAKE_OFF_ALL),
                                  &packet);
        self->take_off_button->button.disabled = 1;
        ui_window_invalidate_base(&self->take_off_button->button.base);
        break;
    }
    }
}

int ui_equip_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                  int val3, int val4)
{
    struct UIEquipWnd *self = (struct UIEquipWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case UI_MSG_COMMAND:
        equip_on_command(self, sender, val1, val2);
        return 0;

    case EQUIP_MSG_TAB:
        self->tab = val1;
        if (val1 == EQUIP_TAB_TITLE) {
            ui_window_set_show_base(self->title_list, 1);
            equip_fill_titles(self);
        } else {
            ui_window_set_show_base(self->title_list, 0);
        }
        if (self->tab == EQUIP_TAB_TITLE) {
            if (self->swap_wnd && self->swap_wnd->frame.base.state)
                ui_equip_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, ID_SWAP, 0, 0, 0);
            if (self->swap_button) {
                ui_window_set_state(&self->swap_button->button.base, 0);
                ui_button_set_show(&self->swap_button->button, 0);
            }
        } else if (self->swap_button) {
            ui_button_set_show(&self->swap_button->button, 1);
            if (self->swap_wnd) {
                if (g_session1.equipSwapOpen == 1) {
                    self->swap_wnd->frame.base.state = 0;
                    ui_equip_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, ID_SWAP, 0, 0, 0);
                }
                self->swap_wnd->kind = self->tab + 1;
                ui_window_invalidate_base(&self->swap_wnd->frame.base);
            }
        }
        ui_window_invalidate_base(wnd);
        return 0;

    case UI_MSG_SET_INFO: {
        struct WndInfoTall *info = (struct WndInfoTall *)val1;

        self->info = info;
        if (!equip_wnd_info_is_valid((struct WndInfoTall *)info))
            equip_wnd_info_set_default(self->info);
        info = self->info;
        if (info->rect.height != info->org_height && g_session1.equipSwapOpen == 0) {
            ui_equip_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, UI_ID_HELP, 0, 0, 0);
        } else {
            info->org_height = wnd->height + 0x14;
            self->info->rect.height = wnd->height + 0x14;
        }
        ui_equip_wnd_move(wnd, self->info->rect.x, self->info->rect.y);
        return 0;
    }

    case EQUIP_MSG_RELEASE:
        equip_release(val1);
        return 0;

    case EQUIP_MSG_REFRESH:
        if (self->tab == EQUIP_TAB_TITLE) {
            equip_fill_titles(self);
            ui_window_invalidate_base(wnd);
            return 0;
        }
        if (self->viewing_other == 0) {
            self->show_box->state = g_session1.equipOpenToOthers;
            ui_window_invalidate_base(self->show_box);
        }
        equip_place_buttons(self, 0xa7);
        ui_window_invalidate_base(wnd);
        return 0;

    case EQUIP_MSG_UPDATE_BUTTON:
        if (val1 == ID_TAKE_OFF_ALL) {
            self->take_off_button->button.disabled = session_count_worn_equips(&g_session1) == 0;
            ui_window_invalidate_base(&self->take_off_button->button.base);
        }
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_equip_wnd_destroy(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    self->base.ops = &ui_equip_wnd_ops;
    ui_frame_wnd_destruct(self);
    mem_free(self);
}

#define EQUIP_OTHERS(wnd) (((struct UIEquipWnd *)(wnd))->viewing_other)
#define EQUIP_PAGE(wnd) (((struct UIEquipWnd *)(wnd))->tab)
#define EQUIP_TOP(wnd) (((struct UIEquipWnd *)(wnd))->top)
#define SPECIAL_BMP UI_SKIN "basic_interface\\equipwin_special.bmp"
#define GRADE_ICON UI_SKIN "grade_enchant\\grade_icon%d.bmp"
#define SLOT_PITCH 0x1a
#define NAME_WIDTH 0x46

void ui_equip_wnd_draw_special_slots(struct UIWindow *self)
{
    static const int name_x[2] = { 0x1c, 0xa7 };
    static const int icon_x[2] = { 6, 0xfb };
    int i;

    if (EQUIP_PAGE(self) == 1) {
        struct BitmapRes *bmp = ui_skin_bitmap(SPECIAL_BMP);

        if (bmp)
            ui_window_draw_bitmap(self, 0, EQUIP_TOP(self) + 0xe, bmp, 0);
    }
    for (i = 0; i < 10; i++) {
        struct ItemInfo info, tmp;
        struct ItemInfo *src;
        int row = i & 3;

        item_info_construct(&info);
        if (EQUIP_PAGE(self) == 1)
            src = !EQUIP_OTHERS(self) ? session_get_worn_costume(&g_session1, &tmp, i)
                                      : session_get_viewed_costume(&g_session1, &tmp, i);
        else
            src = !EQUIP_OTHERS(self) ? session_get_equipped_item(&g_session1, &tmp, i)
                                      : session_get_viewed_equip(&g_session1, &tmp, i);
        item_info_assign(&info, src);
        item_info_destruct(&tmp);
        if (info.index) {
            if (info.count == 1) {
                if (i < 8) {
                    ui_window_draw_item_icon(self, icon_x[i >> 2], EQUIP_TOP(self) + 0x2c + row * SLOT_PITCH,
                                          &info, 0);
                    ui_window_draw_item_name_short(self, name_x[i >> 2] + 5,
                                               EQUIP_TOP(self) + 0x33 + row * SLOT_PITCH, NAME_WIDTH, &info,
                                               0xc);
                } else {
                    ui_window_draw_item_icon(self, icon_x[i & 3], EQUIP_TOP(self) + 0x12, &info, 0);
                    ui_window_draw_item_name_short(self, name_x[i - 8] + 5, EQUIP_TOP(self) + 0x19, NAME_WIDTH,
                                               &info, 0xc);
                }
            }
            if (info.grade > 0) {
                struct String path;

                string_init(&path);
                string_format(&path, GRADE_ICON, (int)info.grade);
                if (i < 8)
                    ui_window_draw_bitmap(self, icon_x[i >> 2], EQUIP_TOP(self) + 0x38 + row * SLOT_PITCH,
                                        res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
                else
                    ui_window_draw_bitmap(self, icon_x[i & 3], EQUIP_TOP(self) + 0x1e,
                                        res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
                string_free(&path);
            }
        }
        item_info_destruct(&info);
    }
}

#define DRAG_EQUIP_SET_A 0x11
#define DRAG_EQUIP_SET_B 0x12
#define MODE_REQ_WEAR_AMMO 0x57
#define MODE_REQ_WEAR_FROM_SET 0x131
#define MODE_REQ_WEAR 0x13

static int worn_as_ammo(int type)
{
    static const BYTE table[10] = { 0, 1, 1, 1, 1, 1, 0, 0, 1, 0 };

    return (unsigned)(type - 10) <= 9 && table[type - 10] == 0;
}

void ui_equip_wnd_drag_and_drop(struct UIWindow *self, int x, int y, const struct DragInfo *drag)
{
    struct Mode *mode;
    struct ItemInfo info, tmp;
    BYTE from_set = 0;

    if (EQUIP_OTHERS(self))
        return;
    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(self, x, y, drag);
        return;
    }
    mode = game_current_base_mode();
    item_info_construct_with_arg(&info);
    switch (drag->source) {
    case DRAG_INVENTORY:
        item_info_assign(&info, session_get_inventory_item(&g_session1, &tmp, drag->index));
        item_info_destruct(&tmp);
        break;
    case DRAG_EQUIP_SET_A:
        from_set = 1;
        item_info_assign(&info, session_get_switch_equip(&g_session1, &tmp, drag->count));
        item_info_destruct(&tmp);
        break;
    case DRAG_EQUIP_SET_B:
        from_set = 1;
        item_info_assign(&info, session_get_switch_costume(&g_session1, &tmp, drag->count));
        item_info_destruct(&tmp);
        break;
    }
    if (info.index) {
        if (worn_as_ammo(info.type))
            mode_send_message(mode, MODE_REQ_WEAR_AMMO, drag->index, info.equip_location, 0, 0);
        else if (from_set == 1)
            mode_send_message(mode, MODE_REQ_WEAR_FROM_SET, (int)&info, 0, 0, 0);
        else if (info.identified && info.equip_location && !info.worn_location)
            mode_send_message(mode, MODE_REQ_WEAR, drag->index, info.equip_location, 0, 0);
    }
    mode_send_message(client_current_mode(), MODE_REQ_DRAG_END, 0, 0, 0, 0);
    item_info_destruct(&info);
}

void ui_equip_wnd_set_viewing_other(struct UIWindow *wnd, int other)
{
    ((struct UIEquipWnd *)wnd)->viewing_other = other;
}

void ui_equip_wnd_refresh_swap_wnd(struct UIWindow *wnd)
{
    struct UIWindow *swap = (struct UIWindow *)((struct UIEquipWnd *)wnd)->swap_wnd;

    if (swap)
        ui_swap_equipment_wnd_send_msg(swap, NULL, 0x3c, 0, 0, 0, 0);
}
