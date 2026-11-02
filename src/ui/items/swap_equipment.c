#include "ui/character/character_draw.h"
#include "ui/controls/buttons.h"
#include "ui/window_access.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "actor/appearance.h"
#include "ui/items/items.h"
#include "ui/shops/shops.h"
#include <string.h>
#include "ui/chat/chat_window_parts.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_names.h"
#include "session/session_talk.h"
#include "ui/draw_key.h"
#include "ui/quests/npc_input.h"
#include "ui/shops/merchant.h"
#include "platform/clock.h"

#define SWAP_SKIN UI_SKIN "swap_equipment\\"
#define SWAP_BTN_NORMAL SWAP_SKIN "btn_change2_normal.bmp"
#define SWAP_BTN_OVER SWAP_SKIN "btn_change2_over.bmp"
#define SWAP_BTN_PRESS SWAP_SKIN "btn_change2_press.bmp"
#define SWAP_BTN_DISABLE SWAP_SKIN "btn_change2_disable.bmp"
#define SWAP_BG_EQUIP SWAP_SKIN "equipwin_bg2_change.bmp"
#define SWAP_BG_SPECIAL SWAP_SKIN "equipwin_special_change.bmp"
#define SWAP_GRADE_ICON UI_SKIN "grade_enchant\\grade_icon%d.bmp"

#define SWAP_KIND_EQUIP 1
#define SWAP_KIND_SPECIAL 2
#define SWAP_SLOTS 10
#define SWAP_SLOT_AMMO 0xf
#define SWAP_SLOT_NONE (-1)
#define SWAP_ROW_HEIGHT 0x1a
#define SWAP_ID_SWAP 0x1f4
#define SWAP_WND_ID 0x128
#define SWAP_COOLDOWN 10000
#define SWAP_TEXT_COLOUR 0x172842
#define SWAP_TEXT_COLOUR_GREY 0x969696
#define SWAP_MSG_REFRESH 0x3c

#define MODE_REQ_SWAP 0x130
#define MODE_REQ_SET_PUT 0x12e
#define MODE_REQ_SET_ITEM 0x131
#define MODE_REQ_SET_TAKE 0x12f
#define DRAG_SWAP_EQUIP 0x11
#define DRAG_SWAP_SPECIAL 0x12
#define DRAG_EQUIP 2
#define DRAG_SPECIAL_EQUIP 0x18

struct UISwapBitmapButton {
    struct UIBitmapButton btn;
    COLORREF text_colour;
};

struct DragInfo;
struct UIWindow;
void ui_swap_equipment_wnd_destroy(struct UIWindow *window);
void ui_swap_equipment_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_swap_equipment_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_swap_equipment_wnd_on_draw(struct UIWindow *self);
void ui_swap_equipment_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_swap_equipment_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_swap_equipment_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_swap_equipment_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
intptr_t ui_swap_equipment_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_swap_equipment_wnd_ops = {
    .destroy = ui_swap_equipment_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_swap_equipment_wnd_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_swap_equipment_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_swap_equipment_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_swap_equipment_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_swap_equipment_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_swap_equipment_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_swap_equipment_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_swap_equipment_wnd_send_msg,
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
struct UIWindow;
void ui_swap_bitmap_button_destroy(struct UIWindow *window);
void ui_swap_bitmap_button_on_draw(struct UIWindow *self);
const struct UIButtonOps ui_swap_bitmap_button_ops = {
    .base = {
        .destroy = ui_swap_bitmap_button_destroy,
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
        .on_create = ui_window_on_create_base,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_swap_bitmap_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_window_send_msg_base,
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
    },
    .set_show = ui_button_set_show,
};

static struct BitmapRes *swap_bitmap(const char *path)
{
    return (struct BitmapRes *)res_mgr_get(g_resMgr(), path);
}

static void swap_draw_grade(struct UIWindow *wnd, int x, int y, short grade)
{
    struct String path;

    string_init(&path);
    string_format(&path, SWAP_GRADE_ICON, (int)grade);
    ui_window_draw_bitmap(wnd, x, y, swap_bitmap(string_cstr(&path)), 1);
    string_free(&path);
}

struct UISwapEquipmentWnd *ui_swap_equipment_wnd_construct(struct UISwapEquipmentWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_swap_equipment_wnd_ops;
    self->swap = NULL;
    self->kind = 0;
    self->made_at = platform_ticks_ms();
    ui_animation_reset(&self->animation);
    self->hover[0] = SWAP_SLOT_NONE;
    self->hover[1] = SWAP_SLOT_NONE;
    self->hover[2] = SWAP_SLOT_NONE;
    self->frame.base.state = 0;
    return self;
}

void ui_swap_equipment_wnd_destroy(struct UIWindow *window)
{
    struct UISwapEquipmentWnd *self = (struct UISwapEquipmentWnd *)window;
    self->frame.base.ops = &ui_swap_equipment_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_swap_equipment_wnd_set_look(struct UISwapEquipmentWnd *self, int slot, int view)
{
    switch (slot) {
    case 0:
        self->accessory_bottom = view;
        break;
    case 2:
        self->robe = view;
        break;
    case 8:
        self->accessory_top = view;
        break;
    case 9:
        self->accessory_mid = view;
        break;
    }
}

struct ItemInfo *ui_swap_equipment_wnd_set_item(struct UISwapEquipmentWnd *self, struct ItemInfo *out, int slot)
{
    struct ItemInfo item;

    if (slot == SWAP_SLOT_NONE) {
        item_info_construct_with_arg(out);
        return out;
    }
    if (self->kind == SWAP_KIND_SPECIAL)
        session_get_switch_costume(&g_session1, &item, slot);
    else
        session_get_switch_equip(&g_session1, &item, slot);
    if ((unsigned)item.index > 0) {
        item_info_copy(out, &item);
    } else {
        struct ItemInfo empty;

        item_info_construct_with_arg(&empty);
        item_info_copy(out, &empty);
        item_info_destruct(&empty);
    }
    item_info_destruct(&item);
    return out;
}

int ui_swap_equipment_wnd_slot_at(struct UISwapEquipmentWnd *self, int x, int y)
{
    struct UIWindow *wnd = &self->frame.base;
    int slot = SWAP_SLOT_NONE;

    if (y > wnd->height || (unsigned)(y - 0x16) > 0x81)
        return SWAP_SLOT_NONE;
    if ((x >= 6 && x < 0x6a) || (x >= 0xaa && x < wnd->width - 6)) {
        int row = (y - 0x16) / SWAP_ROW_HEIGHT;

        if (x >= 0xaa)
            slot = row == 0 ? 9 : row + 3;
        else
            slot = row == 0 ? 8 : row - 1;
    }
    if (x > 0x70 && x < 0xaa && (unsigned)(y - 0x1c) <= 0x18)
        slot = SWAP_SLOT_AMMO;
    return slot;
}

struct ItemInfo *ui_swap_equipment_wnd_item_at(struct UISwapEquipmentWnd *self, struct ItemInfo *out, int x,
                                                      int y)
{
    return ui_swap_equipment_wnd_set_item(self, out, ui_swap_equipment_wnd_slot_at(self, x, y));
}

void ui_swap_equipment_wnd_draw_icon_tinted(struct UISwapEquipmentWnd *self, int x, int y, const char *id_text,
                                                  BYTE identified)
{
    char name[0x80];
    struct BitmapRes *bitmap;

    memset(name, 0, sizeof name);
    session_get_item_bitmap_file_name(id_text, name, identified);
    bitmap = swap_bitmap(name);
    if (bitmap)
        ui_window_draw_bitmap_tinted(&self->frame.base, 0, x, y, bitmap, 1, 0);
}

void ui_swap_equipment_wnd_draw_items(struct UISwapEquipmentWnd *self)
{
    static const int icon_x[2] = {6, 0xfa}, name_x[2] = {0x22, 0xad};
    static const int name_y[4] = {0x36, 0x50, 0x6a, 0x84};
    struct UIWindow *wnd = &self->frame.base;
    int i;

    self->robe = 0;
    self->accessory_bottom = 0;
    self->accessory_top = 0;
    self->accessory_mid = 0;
    for (i = 0; i < SWAP_SLOTS; i++) {
        struct ItemInfo item;
        BYTE from_set = 1;
        int x, y, nx, ny;

        ui_swap_equipment_wnd_set_item(self, &item, i);
        if ((unsigned)item.index == 0) {
            struct ItemInfo worn;

            from_set = 0;
            if (self->kind == SWAP_KIND_SPECIAL)
                session_get_worn_costume(&g_session1, &worn, i);
            else
                session_get_equipped_item(&g_session1, &worn, i);
            item_info_assign(&item, &worn);
            item_info_destruct(&worn);
            if ((unsigned)item.index == 0) {
                item_info_destruct(&item);
                continue;
            }
        }
        if (item.count != 1) {
            item_info_destruct(&item);
            continue;
        }
        if (i < 8) {
            x = icon_x[i >> 2];
            y = (i & 3) * SWAP_ROW_HEIGHT + 0x30;
            nx = name_x[i >> 2];
            ny = name_y[i & 3];
        } else {
            x = icon_x[i & 3];
            y = 0x16;
            nx = name_x[i - 8];
            ny = 0x1e;
        }
        if (!from_set) {
            int slots[3], order[3], k;
            BYTE taken = 0;

            session_equip_slots_of(slots, item.worn_location);
            order[0] = slots[2];
            order[1] = slots[1];
            order[2] = slots[0];
            for (k = 0; k < 3; k++) {
                struct ItemInfo other;

                ui_swap_equipment_wnd_set_item(self, &other, order[k]);
                taken = (unsigned)other.index > 0;
                item_info_destruct(&other);
            }
            if (taken == 1) {
                item_info_destruct(&item);
                continue;
            }
        }
        ui_swap_equipment_wnd_set_look(self, i, item.view);
        if (from_set)
            ui_window_draw_item_icon(wnd, x, y, &item, 0);
        else
            ui_swap_equipment_wnd_draw_icon_tinted(self, x, y, string_cstr(&item.id_text), 1);
        ui_window_draw_item_name_short(wnd, nx, ny, 0x46, &item, 0xc);
        if (i < 8 && item.grade > 0)
            swap_draw_grade(wnd, x, y + 0xc, item.grade);
        item_info_destruct(&item);
    }
}

void ui_swap_equipment_wnd_draw_character(struct UISwapEquipmentWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;

    struct CharacterDrawOptions options = {
        .x = wnd->width / 2 + 1,
        .y = wnd->height - 0x35,
        .sex = session_get_sex(&g_session1),
        .job = g_session1.job,
        .body = (unsigned short)g_session1.body,
        .head = (unsigned short)g_session1.hairStyle,
        .accessory = (unsigned short)self->accessory_bottom,
        .accessory2 = (unsigned short)self->accessory_top,
        .accessory3 = (unsigned short)self->accessory_mid,
        .robe = self->robe,
        .riding = 0,
        .scaled = 0,
        .body_palette = g_session1.clothesColor,
        .head_palette = g_session1.hairColor,
        .action = 0,
        .motion = 0,
        .start_time = self->made_at,
        .animation = &self->animation,
    };
    ui_draw_character(wnd, &options, 0);
}

unsigned long long ui_swap_equipment_wnd_key(struct UISwapEquipmentWnd *self, unsigned long long key)
{
    struct ItemInfo item;
    int i;

    key = draw_key_int(key, self->kind);
    for (i = 0; i < SWAP_SLOTS; i++) {
        ui_swap_equipment_wnd_set_item(self, &item, i);
        key = draw_key_item(key, &item);
        item_info_destruct(&item);
        if (self->kind == SWAP_KIND_SPECIAL)
            session_get_worn_costume(&g_session1, &item, i);
        else
            session_get_equipped_item(&g_session1, &item, i);
        key = draw_key_item(key, &item);
        item_info_destruct(&item);
    }
    key = draw_key_int(key, session_get_sex(&g_session1));
    key = draw_key_int(key, g_session1.job);
    key = draw_key_int(key, g_session1.body);
    key = draw_key_int(key, g_session1.hairStyle);
    key = draw_key_int(key, g_session1.clothesColor);
    key = draw_key_int(key, g_session1.hairColor);
    session_get_inventory_item(&g_session1, &item, g_session1.switchArrowIndex);
    key = draw_key_item(key, &item);
    item_info_destruct(&item);
    return key;
}

void ui_swap_equipment_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UISwapEquipmentWnd *self = (struct UISwapEquipmentWnd *)window;
    struct UISwapBitmapButton *swap;
    struct UIBasicButton *cancel;
    const char *text;

    (void)width;
    (void)height;
    swap = mem_try_alloc(sizeof *swap);
    if (swap) {
        ui_bitmap_button_construct(&swap->btn);
        swap->btn.button.base.ops = &ui_swap_bitmap_button_ops.base;
    }
    self->swap = swap;
    ui_bitmap_button_set_bitmap(&swap->btn, SWAP_BTN_NORMAL, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(&self->swap->btn, SWAP_BTN_OVER, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(&self->swap->btn, SWAP_BTN_PRESS, UI_BTN_DOWN, 0);
    ui_window_create(&self->swap->btn.button.base, self->swap->btn.bitmap_width,
                     self->swap->btn.bitmap_height);
    text = msg_str(0xc27);
    string_assign_cstr(&self->swap->btn.button.text, text);
    self->swap->text_colour = SWAP_TEXT_COLOUR;
    ui_window_move_base(&self->swap->btn.button.base, 0x51, 0x9b);
    self->swap->btn.button.base.id = SWAP_ID_SWAP;
    ui_window_add_child(&self->frame.base, &self->swap->btn.button.base);

    cancel = mem_try_alloc(sizeof *cancel);
    cancel = cancel ? ui_basic_button_construct(cancel) : NULL;
    ui_basic_button_set_label((struct UIWindow *)cancel, 0x32, msg_str(0xda));
    ui_window_move_base(&cancel->button.base, 0xe0, 0x9b);
    ((struct UIWindow *)cancel)->id = UI_ID_CANCEL;
    ui_window_add_child(&self->frame.base, (struct UIWindow *)cancel);
}

void ui_swap_equipment_wnd_on_draw(struct UIWindow *window)
{
    struct UISwapEquipmentWnd *self = (struct UISwapEquipmentWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String path;
    struct BitmapRes *bitmap;
    struct ItemInfo ammo;
    int len;

    ui_animation_reset(&self->animation);
    ui_window_clear_canvas(wnd, 0xffffffff);
    string_init(&path);
    if (self->kind == SWAP_KIND_EQUIP)
        string_assign(&path, SWAP_BG_EQUIP, sizeof SWAP_BG_EQUIP - 1);
    else if (self->kind == SWAP_KIND_SPECIAL)
        string_assign(&path, SWAP_BG_SPECIAL, sizeof SWAP_BG_SPECIAL - 1);
    bitmap = swap_bitmap(string_cstr(&path));
    if (bitmap)
        ui_window_draw_bitmap(wnd, 0, 0, bitmap, 0);
    len = (int)strlen(msg_str(0xc26));
    ui_window_text_out_centred(wnd, 0x8c, 4, msg_str(0xc26), len, 0, 0xc, 0x14234b, 0);
    ui_swap_equipment_wnd_draw_items(self);
    ui_swap_equipment_wnd_draw_character(self);
    session_get_inventory_item(&g_session1, &ammo, g_session1.switchArrowIndex);
    if ((unsigned)ammo.index > 0) {
        ui_window_draw_item_icon(wnd, wnd->width / 2 - 0x18, 0x1b, &ammo, 0);
        ui_window_draw_item_name(wnd, wnd->width / 2 - 0x18, 0x33, &ammo, 0, 0);
    }
    item_info_destruct(&ammo);
    string_free(&path);
}

static void swap_link_item(const struct ItemInfo *info)
{
    struct UIWindow *edit = ui_window_mgr_get_focus_edit(&g_windowMgr);

    if (!edit || !info->identified)
        return;
    switch (edit->id) {
    case EDIT_ID_MEGAPHONE:
        megaphone_wnd_add_item_link(g_windowMgr.npc_text_edit_dialog_wnd, info);
        break;
    case EDIT_ID_WHISPER:
    case EDIT_ID_CHAT_ROOM:
        ui_chat_edit_ctrl_add_item_link(edit, info);
        break;
    case EDIT_ID_CHAT:
        ui_chat_edit_ctrl_add_item_link(ui_chat_window_edit(g_windowMgr.new_chat_wnd), info);
        break;
    case EDIT_ID_RODEX:
        ui_rodex_compose_wnd_add_item_link(g_windowMgr.rodex_compose_wnd, info);
        break;
    }
}

void ui_swap_equipment_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISwapEquipmentWnd *self = (struct UISwapEquipmentWnd *)window;
    struct ItemInfo item;

    ui_swap_equipment_wnd_item_at(self, &item, x, y);
    if ((unsigned)item.index > 0 && item.count > 0) {
        int slots[3];

        session_equip_slots_of(slots, item.worn_location);
        game_mode_drag_item(game_current_mode(),
                          self->kind == SWAP_KIND_SPECIAL ? DRAG_SWAP_SPECIAL : DRAG_SWAP_EQUIP, &item, item.count,
                          slots[2]);
        if ((GetAsyncKeyState(VK_SHIFT) >> 8) & 0xff)
            swap_link_item(&item);
    }
    item_info_destruct(&item);
}

void ui_swap_equipment_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UISwapEquipmentWnd *self = (struct UISwapEquipmentWnd *)window;
    struct ItemInfo item;

    if (y < 0x11)
        ui_swap_equipment_wnd_send_msg(&self->frame.base, &self->frame.base, UI_MSG_COMMAND, UI_ID_HELP, 0, 0, 0);
    ui_swap_equipment_wnd_item_at(self, &item, x, y);
    if ((unsigned)item.index > 0 && item.count > 0) {
        int slots[3];
        struct Mode *mode;

        session_equip_slots_of(slots, item.worn_location);
        mode = client_current_mode();
        if (GetAsyncKeyState(VK_CONTROL))
            mode_send_message(mode, MODE_REQ_SET_ITEM, (intptr_t)&item, 0, 0, 0);
        else
            mode_send_message(mode, MODE_REQ_SET_TAKE, slots[2], self->kind == SWAP_KIND_SPECIAL, 0, 0);
    }
    item_info_destruct(&item);
}

void ui_swap_equipment_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UISwapEquipmentWnd *self = (struct UISwapEquipmentWnd *)window;
    struct ItemInfo item;
    struct Mode *mode;
    int old[3], index;

    ui_swap_equipment_wnd_item_at(self, &item, x, y);
    if ((unsigned)item.index > 0) {
        struct String text, line;
        struct String name = {0};
        int tip_x = 6, tip_y = y - (y - 0x16) % SWAP_ROW_HEIGHT - 0x13;

        if (x >= 0xaa)
            tip_x = 0xaf;
        else if (x >= 0x6a)
            tip_x = self->frame.base.width / 2 - 0x14;
        ui_window_to_screen(&self->frame.base, &tip_x, &tip_y);
        string_init(&text);
        if (item.count <= 1) {
            item_info_get_display_name(&item, &name);
            if (item.option_count != 0) {
                string_init(&line);
                string_format(&line, msg_str(0xa28), string_cstr(&name), item.option_count);
                string_assign(&text, string_cstr(&line), line.size);
                string_free(&line);
            } else {
                string_assign(&text, string_cstr(&name), name.size);
            }
            string_free(&name);
        } else {
            item_info_get_display_name(&item, &name);
            string_init(&line);
            if (item.option_count != 0)
                string_format(&line, msg_str(0x9f7), string_cstr(&name),
                                  item.option_count, item.count);
            else
                string_format(&line, msg_str(0xb7), string_cstr(&name), item.count);
            string_assign(&text, string_cstr(&line), line.size);
            string_free(&line);
            string_free(&name);
        }
        mode = game_current_base_mode();
        mode_show_tooltip(mode, string_cstr(&text), tip_x, tip_y,
                          session_get_item_dis_color(&item), 0);
        string_free(&text);
    }
    item_info_destruct(&item);

    old[0] = self->hover[0];
    old[1] = self->hover[1];
    old[2] = self->hover[2];
    self->hover[0] = SWAP_SLOT_NONE;
    self->hover[1] = SWAP_SLOT_NONE;
    self->hover[2] = SWAP_SLOT_NONE;
    if (game_inventory_drag_index(&index)) {
        if (!((x >= 6 && x < 0x1e) || (x >= 0xfb && x < 0x113)))
            return;
        session_get_inventory_item(&g_session1, &item, index);
        if ((unsigned)item.index > 0 && item.equip_location != 0) {
            int slots[3];

            session_equip_slots_of(slots, item.equip_location);
            self->hover[0] = slots[2];
            if (slots[1] != SWAP_SLOT_NONE)
                self->hover[1] = slots[1];
            if (slots[0] != SWAP_SLOT_NONE)
                self->hover[2] = slots[0];
        }
        item_info_destruct(&item);
    }
    if (old[0] != self->hover[0] || old[1] != self->hover[1] || old[2] != self->hover[2])
        ui_window_invalidate_base(&self->frame.base);
}

void ui_swap_equipment_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISwapEquipmentWnd *self = (struct UISwapEquipmentWnd *)window;
    struct ItemInfo item;

    ui_swap_equipment_wnd_item_at(self, &item, x, y);
    if ((unsigned)item.index > 0) {
        struct UIWindow *wnd = ui_window_open(ITEM_WND_ID);

        ui_window_send(wnd, NULL, 0x18, (intptr_t)&item, 0, 0, 0);
    }
    item_info_destruct(&item);
}

void ui_swap_equipment_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UISwapEquipmentWnd *self = (struct UISwapEquipmentWnd *)window;
    struct Mode *mode;
    struct ItemInfo item, tmp;
    BYTE worn = 0;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->frame.base, x, y, drag);
        return;
    }
    mode = game_current_base_mode();
    item_info_construct_with_arg(&item);
    switch (drag->source) {
    case DRAG_INVENTORY:
        session_get_inventory_item(&g_session1, &tmp, drag->index);
        item_info_assign(&item, &tmp);
        item_info_destruct(&tmp);
        break;
    case DRAG_EQUIP:
        worn = 1;
        session_get_equipped_item(&g_session1, &tmp, drag->count);
        item_info_assign(&item, &tmp);
        item_info_destruct(&tmp);
        break;
    case DRAG_SPECIAL_EQUIP:
        worn = 1;
        session_get_worn_costume(&g_session1, &tmp, drag->count);
        item_info_assign(&item, &tmp);
        item_info_destruct(&tmp);
        break;
    }
    if ((unsigned)item.index > 0) {
        switch (item.type) {
        case 10:
        case 16:
        case 17:
        case 19:
            mode_send_message(mode, MODE_REQ_SET_PUT, drag->index, item.equip_location, 0, 0);
            break;
        default:
            if (worn == 1)
                mode_send_message(mode, MODE_REQ_SET_ITEM, (intptr_t)&item, 0, 0, 0);
            else if (item.equip_location != 0 && item.identified && item.worn_location == 0)
                mode_send_message(mode, MODE_REQ_SET_PUT, drag->index, item.equip_location, 0, 0);
            break;
        }
    }
    mode_send_message(client_current_mode(), MODE_REQ_DRAG_END, 0, 0, 0, 0);
    item_info_destruct(&item);
}

intptr_t ui_swap_equipment_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                          intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UISwapEquipmentWnd *self = (struct UISwapEquipmentWnd *)window;
    int refusal;

    (void)sender;
    (void)val2;
    (void)val3;
    (void)val4;
    if (msg == SWAP_MSG_REFRESH) {
        ui_window_invalidate_base(&self->frame.base);
        ui_window_invalidate_children(&self->frame.base);
        return 1;
    }
    if (msg != UI_MSG_COMMAND)
        return 1;
    switch (val1) {
    case UI_ID_CANCEL:
        g_session1.equipSwapOpen = 0;
        if (g_windowMgr.equip_wnd)
            ui_window_send_msg(g_windowMgr.equip_wnd, NULL, UI_MSG_COMMAND, SWAP_ID_SWAP, 0, 0, 0);
        return 1;
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, SWAP_WND_ID);
        return 1;
    case SWAP_ID_SWAP:
        if (!game_current_player())
            return 1;
        if (g_windowMgr.exchange_wnd != 0) {
            refusal = 0xbb9;
        } else if (game_player_is_dead()) {
            refusal = 0xbb7;
        } else if (g_windowMgr.merchant_item_my_shop_wnd != 0 || g_windowMgr.merchant_item_purchase_wnd != 0 || g_windowMgr.merchant_shop_make_wnd != 0) {
            refusal = 0xbba;
        } else {
            if (player_has_cast_gauge(game_current_player())) {
                refusal = 0xbb8;
            } else if (g_windowMgr.say_dialog_wnd != 0 || g_windowMgr.choose_wnd != 0) {
                refusal = 0xbbb;
            } else {
                game_send_message(MODE_REQ_SWAP, 0, 0, 0, 0);
                return 1;
            }
        }
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)msg_str(refusal), 0xff, 0, 0);
        return 1;
    }
    return 1;
}

void ui_swap_bitmap_button_destroy(struct UIWindow *window)
{
    struct UISwapBitmapButton *self = (struct UISwapBitmapButton *)window;
    ui_bitmap_button_destruct(&self->btn);
    mem_free(self);
}

void ui_swap_bitmap_button_on_draw(struct UIWindow *window)
{
    struct UISwapBitmapButton *self = (struct UISwapBitmapButton *)window;
    struct UIWindow *wnd = &self->btn.button.base;
    DWORD elapsed;

    ui_bitmap_button_on_draw((struct UIWindow *)(&self->btn));
    elapsed = platform_ticks_ms() - g_session1.equipSwapSentAt;
    ui_bitmap_button_set_bitmap(&self->btn, elapsed < SWAP_COOLDOWN ? SWAP_BTN_DISABLE : SWAP_BTN_NORMAL,
                             UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(&self->btn, elapsed < SWAP_COOLDOWN ? SWAP_BTN_DISABLE : SWAP_BTN_OVER,
                             UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(&self->btn, elapsed < SWAP_COOLDOWN ? SWAP_BTN_DISABLE : SWAP_BTN_PRESS,
                             UI_BTN_DOWN, 0);
    self->text_colour = elapsed < SWAP_COOLDOWN ? SWAP_TEXT_COLOUR_GREY : SWAP_TEXT_COLOUR;
    ui_window_text_out_centred(wnd, wnd->width / 2, wnd->height / 2 - 5, string_cstr(&self->btn.button.text),
                            (int)self->btn.button.text.size, 0, 0xc, self->text_colour, 0);
}
