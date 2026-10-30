#include "ui/character/character_draw.h"
#include "ui/controls/buttons.h"
#include "ui/window_access.h"
#include "ui/character/select_char_new.h"
#include "ui/character/select_char_storage.h"
#include "render/rp_pool.h"
#include "session/character_name.h"
#include "web/service_managers.h"
#include "contents/sound.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include <stdarg.h>
#include <stdio.h>
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/character/char_info.h"
#include "ui/runtime.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "login/loginmode.h"
#include "session/session_names.h"
#include "session/session_talk.h"
#include "ui/draw_key.h"
#include "ui/character/selection_buttons.h"

#define SELECT_DIR UI_SKIN "select_character_ver3\\"
#define COLUMNS 5
#define INFO_LINES SELECT_CHAR_INFO_LINES

#define ID_START 0xb0
#define ID_BACK 0xb1
#define ID_SELECT_PAGE_SLOT 0xc3
#define ID_SLOT 0xc4
#define ID_DELETE 0xcb
#define ID_ERROR_TEXT_A 0x114
#define ID_ERROR_TEXT_B 0x116
#define ID_PAGE_LEFT 0x134
#define ID_PAGE_RIGHT 0x135
#define ID_CHANGE_NAME 0x157
#define ID_NAME_CHANGED 0x158
#define ID_DELETE_RESERVE 0x18f
#define ID_DELETE_CANCEL 0x190
#define ID_MAKE 0x198
#define ID_DEL 0x199
#define ID_PIN_ENABLE 0x19a
#define ID_MOVE_FROM 0x1a8
#define ID_MOVE_TO 0x1a9
#define ID_PRESS_SLOT_0 0x1aa
#define ID_PRESS_SLOT_1 0x1ab
#define ID_PRESS_SLOT_2 0x1ac

#define MSG_ROW_UP 0x12
#define MSG_ROW_DOWN 0x13
#define MSG_SLOT_LEFT 0x14
#define MSG_SLOT_RIGHT 0x15
#define MSG_DELETE_KEY 0x21

#define LOGIN_REQ_BACK 2
#define LOGIN_CMD_START 0x2712
#define LOGIN_CMD_DELETE 0x271a
#define LOGIN_REQ_BACK_TO_LOGIN 0x271b
#define LOGIN_REQ_ERROR_TEXT 0x2724

#define NOTICE_MOVE_PENDING 0x2e
#define NOTICE_CAN_MOVE 0x2d
#define NOTICE_CAN_RENAME 0x2f

#define SOUND_BUTTON "버튼소리.wav"

#define CURSLOT_ROOT HKEY_CURRENT_USER

struct UINewSelectCharWnd;
struct UIWindow;
void ui_new_select_char_wnd_destroy(struct UIWindow *window);
void ui_new_select_char_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_new_select_char_wnd_on_process(struct UIWindow *self);
void ui_new_select_char_wnd_on_draw(struct UIWindow *self);
BYTE ui_new_select_char_wnd_render(struct UIWindow *self, int force);
void ui_new_select_char_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_new_select_char_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_new_select_char_wnd_ops = {
    .destroy = ui_new_select_char_wnd_destroy,
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
    .on_create = ui_new_select_char_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_new_select_char_wnd_on_process,
    .on_draw = ui_new_select_char_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_new_select_char_wnd_on_l_btn_down,
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
    .send_msg = ui_new_select_char_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_new_select_char_wnd_render,
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

struct UINewSelectCharWnd {
    struct UIFrameWnd frame;
    struct UIBmpTextButton *info_button;
    struct UIBitmapButton *make_button;
    struct UIBitmapButton *del_button;
    struct UIBmpTextButton *reserve_button;
    struct UIBmpTextButton *cancel_button;
    struct Tree gids;
    struct SelectCharStorage storage;
    struct UIBitmapButton *left_button;
    struct UIBitmapButton *right_button;
    struct UIBmpTextButton *start_button;
    struct UIBmpTextButton *delete_button;
    struct UIWindow *page_label;
    DWORD created_time;
    BYTE busy;
    int page;
    int cur;
    int move_from;
    unsigned long long drawn_key;
};

#define WND(self) (&(self)->frame.base)
#define SLOT_BUTTON(self, i) ((self)->storage.slots[i])
#define NAME_LABEL(self, i) ((self)->storage.names[i])
#define INFO_LINE(self, i) ((self)->storage.info_lines[i])

static struct Mode *login_mode(void)
{
    return mode_mgr_get_login_mode(&g_modeMgr);
}

static void send_command(struct UIWindow *to, struct UIWindow *sender, int id, int val2)
{
    ui_window_send_msg(to, sender, UI_MSG_COMMAND, id, val2, 0, 0);
}

static void button_disable(void *button, BYTE disabled)
{
    ((struct UIButton *)button)->disabled = disabled;
    ui_window_invalidate(button);
}

static int slot_x(int col) { return (col % COLUMNS) * 0x9d; }
static int slot_y(int col) { return (col / COLUMNS) * 0xc3; }

static int page_column(int index, int page, int per_page)
{
    return page ? index % (page * per_page) : index;
}

static void set_caption(struct UIBmpTextButton *button, const char *text)
{
    ui_bmp_text_button_set_caption(button, text_view_cstr(text));
}

static void set_bitmaps(struct UIBitmapButton *button, const char *up, const char *hover, const char *down)
{
    ui_bitmap_button_set_bitmap(button, up, 0, 0);
    ui_bitmap_button_set_bitmap(button, hover, 1, 0);
    ui_bitmap_button_set_bitmap(button, down, 2, 0);
}

static struct UIBitmapButton *new_bitmap_button(void)
{
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    return button ? ui_bitmap_button_construct(button) : NULL;
}

static struct UIBmpTextButton *new_text_button(void)
{
    struct UIBmpTextButton *button = mem_alloc(sizeof(struct UIBmpTextButton));

    return button ? ui_bmp_text_button_construct(button) : NULL;
}

static struct UIWindow *new_static_text(void)
{
    struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

    return text ? ui_static_text_construct(text) : NULL;
}

static struct UIBmpTextButton *add_info_button(int x, int y, int caption_id,
                                               int with_disabled)
{
    struct UIBmpTextButton *button = new_text_button();
    int state;

    set_bitmaps(&button->bmp, SELECT_DIR "bt_info_normal.bmp", SELECT_DIR "bt_info_over.bmp",
                SELECT_DIR "bt_info_press.bmp");
    if (with_disabled)
        ui_bitmap_button_set_bitmap(&button->bmp, SELECT_DIR "bt_info_normal.bmp", UI_BMP_DISABLED, 0);
    ui_window_create(&button->bmp.button.base, button->bmp.bitmap_width, button->bmp.bitmap_height);
    ui_window_move_base(&button->bmp.button.base, x, y);
    set_caption(button, msg_str(caption_id));
    for (state = 0; state < 3; state++)
        ui_bmp_text_button_set_text_style(button, state, 0xc, 0x313131, 0, 1, UI_TEXT_CENTRED, UI_TEXT_CENTRED);
    return button;
}

static void set_left_bitmaps(struct UINewSelectCharWnd *self, int off)
{
    if (off)
        set_bitmaps(self->left_button, SELECT_DIR "bt_left_off.bmp", SELECT_DIR "bt_left_off.bmp",
                    SELECT_DIR "bt_left_off.bmp");
    else
        set_bitmaps(self->left_button, SELECT_DIR "bt_left_on.bmp", SELECT_DIR "bt_left_over.bmp",
                    SELECT_DIR "bt_left_press.bmp");
}

static void set_right_bitmaps(struct UINewSelectCharWnd *self, int off)
{
    if (off)
        set_bitmaps(self->right_button, SELECT_DIR "bt_right_off.bmp", SELECT_DIR "bt_right_off.bmp",
                    SELECT_DIR "bt_right_off.bmp");
    else
        set_bitmaps(self->right_button, SELECT_DIR "bt_right_on.bmp", SELECT_DIR "bt_right_over.bmp",
                    SELECT_DIR "bt_right_press.bmp");
}

static void place_name(struct UINewSelectCharWnd *self, struct UIWindow *label, int col)
{
    struct String text;
    int width;

    ui_static_text_get_text(label, &text);
    width = ui_window_text_width(WND(self), string_cstr(&text), 0, 0, 0xc, 0, 0);
    ui_window_move(label, slot_x(col) - width / 2 + 0x58, slot_y(col) + 0xbd);
    string_free(&text);
}

static void set_page_label(struct UINewSelectCharWnd *self)
{
    char text[16] = { 0 };

    if (!self->page_label)
        return;
    sprintf(text, "%d / %d", self->page + 1, self->storage.page_count);
    ui_static_text_set_text(self->page_label, text, 0);
}

static void read_cur_slot(struct UINewSelectCharWnd *self)
{
    HKEY key;
    DWORD type = REG_DWORD, size = 4, slot;

    LONG failed = RegOpenKeyExA(CURSLOT_ROOT, g_registryKeyPath, 0, KEY_QUERY_VALUE, &key);

    self->page = 0;
    self->cur = 0;
    if (failed)
        return;
    RegCloseKey(key);
    if (RegOpenKeyExA(CURSLOT_ROOT, g_registryKeyPath, 0, KEY_QUERY_VALUE, &key))
        return;
    RegQueryValueExA(key, "CURSLOT", NULL, &type, (BYTE *)&slot, &size);
    RegCloseKey(key);
    self->page = slot / (unsigned)self->storage.per_page;
    self->cur = slot % (unsigned)self->storage.per_page;
    if (self->page >= self->storage.page_count || self->page < 0)
        self->page = 0;
}

void ui_new_select_char_wnd_write_to_reg(struct UINewSelectCharWnd *self)
{
    HKEY key;
    DWORD disposition = 0, slot;

    if (RegCreateKeyExA(CURSLOT_ROOT, g_registryKeyPath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &disposition))
        return;
    slot = self->storage.per_page * self->page + self->cur;
    RegSetValueExA(key, "CURSLOT", 0, REG_DWORD, (const BYTE *)&slot, 4);
    RegCloseKey(key);
}

static void store_selected_slot(struct UINewSelectCharWnd *self)
{
    g_loginCharInfo.slot = (BYTE)((BYTE)self->page * (BYTE)self->storage.per_page + (BYTE)self->cur);
}

struct UIWindow *ui_new_select_char_wnd_create(void)
{
    struct UINewSelectCharWnd *self = mem_alloc(sizeof *self);

    ui_frame_wnd_construct(&self->frame, 0);
    WND(self)->ops = &ui_new_select_char_wnd_ops;
    WND(self)->redraws_tree = 1;
    self->drawn_key = 0;
    tree_init(&self->gids);
    select_char_init_storage(&self->storage,
        (int64_t)g_session1.charListCounts[1] + g_session1.charListCounts[2] + g_session1.charListCounts[3],
        login_mode_max_char_slot((struct LoginMode *)login_mode()));
    self->delete_button = NULL;
    self->info_button = NULL;
    self->make_button = NULL;
    self->del_button = NULL;
    self->reserve_button = NULL;
    self->cancel_button = NULL;
    self->busy = 0;
    self->page = 0;
    self->cur = 0;
    self->created_time = timeGetTime();
    self->move_from = -1;
    return WND(self);
}

static void select_char_destruct(struct UINewSelectCharWnd *self)
{
    WND(self)->ops = &ui_new_select_char_wnd_ops;
    select_char_destroy_storage(&self->storage);
    tree_free(&self->gids, NULL);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_new_select_char_wnd_destroy(struct UIWindow *window)
{
    struct UINewSelectCharWnd *self = (struct UINewSelectCharWnd *)window;
    select_char_destruct(self);
    mem_free(self);
}

static void set_line(struct UINewSelectCharWnd *self, int line, const char *fmt, long long value)
{
    struct String text;

    string_init(&text);
    string_format(&text, fmt, value);
    ui_static_text_set_text_ellipsis(INFO_LINE(self, line), string_cstr(&text));
    string_free(&text);
}

static void update_slot_styles(struct UINewSelectCharWnd *self)
{
    int i;

    for (i = 0; i < self->storage.per_page; i++) {
        int index = self->page * self->storage.per_page + i;
        int style = 1;

        if (index >= self->storage.slot_count)
            continue;
        if (index < g_session1.charListCounts[4] && self->storage.empty[index] == 1 && index < g_session1.charListCounts[6])
            style = 0;
        ui_window_send_msg((SLOT_BUTTON(self, i)), NULL, SLOT_MSG_SET_STYLE, style, 0, 0, 0);
    }
}

static void update_delete_buttons(struct UINewSelectCharWnd *self)
{
    int index = self->storage.per_page * self->page + self->cur;
    struct CharInfo *info;

    if (index >= self->storage.slot_count)
        return;
    if (self->storage.empty[index] == 1) {
        button_disable(self->reserve_button, 1);
        ui_button_set_show((struct UIButton *)(self->cancel_button), 0);
        ui_button_set_show((struct UIButton *)(self->delete_button), 0);
        return;
    }
    info = ui_char_info(index);
    if (!info)
        return;
    if (info->delete_date > 0) {
        struct Time t;

        button_disable(self->reserve_button, 1);
        ui_button_set_show((struct UIButton *)(self->delete_button), 1);
        ui_button_set_show((struct UIButton *)(self->cancel_button), 1);
        time_construct(&t, info->delete_date);
        ui_button_set_show((struct UIButton *)(self->delete_button), time_is_past(&t) == 1);
        time_destruct(&t);
    } else {
        button_disable(self->reserve_button, 0);
        ui_button_set_show((struct UIButton *)(self->delete_button), 0);
        ui_button_set_show((struct UIButton *)(self->cancel_button), 0);
    }
}

void ui_new_select_char_wnd_update_info(struct UINewSelectCharWnd *self)
{
    int index = self->storage.per_page * self->page + self->cur;
    struct CharInfo *info;
    int i;

    if (index >= self->storage.slot_count)
        return;
    info = ui_char_info(index);
    if (info) {
        ui_static_text_set_text_ellipsis(INFO_LINE(self, 0),
                                     session_get_job_name(&g_session1, info->job, info->sex));
        set_line(self, 1, "%d", info->level);
        set_line(self, 2, "%lld", info->exp);
        set_line(self, 3, "%lld", info->hp);
        set_line(self, 4, "%lld", info->sp);
        for (i = 0; i < 6; i++)
            set_line(self, 5 + i, "%d", info->stats[i]);
        ui_static_text_set_text_ellipsis(INFO_LINE(self, 11),
                                     session_map_display_name(info->map_name));
    } else {
        for (i = 0; i < INFO_LINES; i++)
            ui_static_text_set_text(INFO_LINE(self, i), "", 0);
    }
    self->start_button->bmp.button.disabled = self->storage.empty[index] == 1;
    ui_window_invalidate_base(&self->start_button->bmp.button.base);
    update_slot_styles(self);
    ui_window_invalidate_children(WND(self));
    update_delete_buttons(self);
}

static void fill_preview(struct UINewSelectCharWnd *self, int index, struct CharInfo *info)
{
    int slot = info->slot, col, head, palette;
    struct CharPreview *preview;
    struct String tmp;

    self->storage.empty[slot] = 0;
    col = slot / self->storage.per_page ? index % (slot / self->storage.per_page * self->storage.per_page) : slot;
    preview = &self->storage.previews[slot];
    preview->x = slot_x(col) + 0x5a;
    preview->y = slot_y(col) + 0x9e;
    preview->job = info->job;
    preview->robe = info->robe;
    if (info->delete_date > 0) {
        preview->action_base = 0x10;
        preview->action = 0x10;
    } else {
        preview->action_base = 0;
        preview->action = 0;
    }
    preview->frame = 0;
    string_free(&preview->act[0]);
    string_move(&preview->act[0], session_get_body_spr_name_variant0(&tmp, info->job, info->sex, info->body, 0));
    string_free(&preview->spr[0]);
    string_move(&preview->spr[0], session_get_body_spr_name_variant1(&tmp, info->job, info->sex, info->body, 0));
    head = info->head;
    string_free(&preview->act[1]);
    string_move(&preview->act[1], session_get_head_spr_name0(&tmp, info->job, info->sex, &head));
    string_free(&preview->spr[1]);
    string_move(&preview->spr[1], session_get_head_spr_name1(&tmp, info->job, info->sex, &head));
    string_free(&preview->act[2]);
    string_move(&preview->act[2], session_get_res_name0(&tmp, info->job, info->sex, head, info->accessory3));
    string_free(&preview->spr[2]);
    string_move(&preview->spr[2], session_get_res_name1(&tmp, info->job, info->sex, head, info->accessory3));
    info->head = (short)head;
    string_free(&preview->act[3]);
    string_move(&preview->act[3], session_get_res_name0(&tmp, info->job, info->sex, head, info->accessory2));
    string_free(&preview->spr[3]);
    string_move(&preview->spr[3], session_get_res_name1(&tmp, info->job, info->sex, head, info->accessory2));
    self->storage.headgear[slot] = info->accessory2;
    string_free(&preview->act[4]);
    string_move(&preview->act[4], session_get_res_name0(&tmp, info->job, info->sex, head, info->accessory));
    string_free(&preview->spr[4]);
    string_move(&preview->spr[4], session_get_res_name1(&tmp, info->job, info->sex, head, info->accessory));
    palette = info->body_palette;
    if ((unsigned short)(info->job - 4001) <= 6 && palette < 1)
        palette = 1;
    preview->body_palette = palette;
    if (preview->body_palette) {
        string_free(&preview->body_palette_name);
        string_move(&preview->body_palette_name, session_get_body_pal_name(&tmp, info->job, info->sex, preview->body_palette,
                                               info->body, 0));
    }
    preview->head_palette = info->head_palette;
    if (preview->head_palette) {
        string_free(&preview->head_palette_name);
        string_move(&preview->head_palette_name, session_get_head_pal_name(&tmp, head, info->job, info->sex,
                                               preview->head_palette));
    }
    string_free(&preview->imf);
    string_move(&preview->imf, session_get_imf_name(&tmp, info->job, info->sex));
}

void ui_new_select_char_wnd_on_create(struct UIWindow *window, int width_unused,
                                            int height_unused)
{
    struct UINewSelectCharWnd *self = (struct UINewSelectCharWnd *)window;
    struct UIWindow *wnd = WND(self);
    struct UIStaticBitmap *info_bitmap;
    struct UIBitmapButton *close;
    struct UIWindow *title, *label;
    struct IntInt64Pair *var;
    char text[16];
    int i, width, height, key = 6;

    read_cur_slot(self);
    var = tree_get(&g_session1.instantVars, &key, sizeof key, sizeof *var, tree_less_int);
    var->value = 1;
    store_selected_slot(self);

    for (i = 0; i < self->storage.per_page; i++) {
        int index = self->page * self->storage.per_page + i;
        struct UIWindow *button = mem_alloc(sizeof(struct UISlotBmpButton));

        if (button)
            button = (struct UIWindow *)ui_slot_bmp_button_construct((struct UISlotBmpButton *)button, i);
        ui_bitmap_button_set_bitmap((struct UIBitmapButton *)button, SELECT_DIR "img_slot2_normal.bmp", 0, 0);
        ui_bitmap_button_set_bitmap((struct UIBitmapButton *)button, SELECT_DIR "img_slot2_over.bmp", 1, 0);
        ui_bitmap_button_set_bitmap((struct UIBitmapButton *)button, SELECT_DIR "img_slot2_press.bmp", 2, 0);
        ui_bitmap_button_set_bitmap((struct UIBitmapButton *)button, SELECT_DIR "img_slot_lock.bmp", 3, 0);
        ui_window_create(button, 0xa1, 0xc5);
        ui_window_move_base(button, slot_x(i) + 0xa, slot_y(i) + 0x15);
        button->id = ID_SLOT;
        ui_slot_bmp_button_set_slot((struct UISlotBmpButton *)button, index);
        ui_window_add_child(wnd, button);
        SLOT_BUTTON(self, i) = button;
        ui_slot_bmp_button_send_msg(button, wnd, UI_MSG_SET_STATE, i == self->cur, 0, 0, 0);
    }

    info_bitmap = mem_alloc(sizeof(struct UIStaticBitmap));
    if (info_bitmap)
        info_bitmap = ui_static_bitmap_construct(info_bitmap);
    ui_static_bitmap_set_bitmap(info_bitmap, SELECT_DIR "img_info.bmp");
    ui_window_create(&info_bitmap->base, info_bitmap->base.width, info_bitmap->base.height);
    ui_window_move_base(&info_bitmap->base, 0x329, 0x1a);
    ui_window_add_child(wnd, &info_bitmap->base);

    close = new_bitmap_button();
    set_bitmaps(close, SELECT_DIR "bt_close2_normal.bmp", SELECT_DIR "bt_close2_over.bmp",
                SELECT_DIR "bt_close2_press.bmp");
    ui_window_create(&close->button.base, close->bitmap_width, close->bitmap_height);
    ui_window_move_base(&close->button.base, wnd->width - 0x34, 5);
    close->button.base.id = ID_BACK;
    ui_window_add_child(wnd, &close->button.base);

    self->info_button = add_info_button(0x33e, 0x15f, 0xd10, 0);
    self->info_button->bmp.button.base.id = 0x116;
    ui_button_set_show((struct UIButton *)(self->info_button), 0);
    ui_window_add_child(wnd, &self->info_button->bmp.button.base);

    self->make_button = new_bitmap_button();
    set_bitmaps(self->make_button, UI_SKIN "btn_make.bmp", UI_SKIN "btn_make_a.bmp", UI_SKIN "btn_make_b.bmp");
    ui_window_create(&self->make_button->button.base, self->make_button->bitmap_width,
                     self->make_button->bitmap_height);
    ui_window_move_base(&self->make_button->button.base, 0xc8, 0x14e);
    self->make_button->button.base.id = ID_MAKE;
    ui_button_show((struct UIButton *)(self->make_button), 0);
    ui_window_add_child(wnd, &self->make_button->button.base);

    self->del_button = new_bitmap_button();
    set_bitmaps(self->del_button, UI_SKIN "btn_del.bmp", UI_SKIN "btn_del_a.bmp", UI_SKIN "btn_del_b.bmp");
    ui_window_create(&self->del_button->button.base, self->del_button->bitmap_width,
                     self->del_button->bitmap_height);
    ui_window_move_base(&self->del_button->button.base, 0xc8, 0x14e);
    self->del_button->button.base.id = ID_DEL;
    ui_button_show((struct UIButton *)(self->del_button), 0);
    ui_window_add_child(wnd, &self->del_button->button.base);
    ui_button_show((struct UIButton *)(self->make_button), 0);
    ui_button_show((struct UIButton *)(self->del_button), 0);

    self->reserve_button = add_info_button(0x33e, 0x142, 0xd0b, 1);
    ui_bmp_text_button_set_text_style(self->reserve_button, 3, 0xc, 0x909090, 0, 0, UI_TEXT_CENTRED,
                                 UI_TEXT_CENTRED);
    self->reserve_button->bmp.button.base.id = ID_DELETE_RESERVE;
    ui_window_add_child(wnd, &self->reserve_button->bmp.button.base);
    if (self->storage.empty[self->storage.per_page * self->page + self->cur] == 1)
        button_disable(self->reserve_button, 1);

    self->cancel_button = add_info_button(0x33e, 0x142, 0xd0c, 0);
    self->cancel_button->bmp.button.base.id = ID_DELETE_CANCEL;
    ui_button_set_show((struct UIButton *)(self->cancel_button), 0);
    ui_window_add_child(wnd, &self->cancel_button->bmp.button.base);

    self->start_button = new_text_button();
    set_bitmaps(&self->start_button->bmp, SELECT_DIR "bt_gameStart_off.bmp", SELECT_DIR "bt_gameStart_over.bmp",
                SELECT_DIR "bt_gameStart_press.bmp");
    ui_bitmap_button_set_bitmap(&self->start_button->bmp, SELECT_DIR "bt_gameStart_off.bmp", UI_BMP_DISABLED, 0);
    ui_window_create(&self->start_button->bmp.button.base, self->start_button->bmp.bitmap_width,
                     self->start_button->bmp.bitmap_height);
    ui_window_move_base(&self->start_button->bmp.button.base, 0x330, 0x197);
    self->start_button->bmp.button.base.id = ID_START;
    set_caption(self->start_button, msg_str(0xd0d));
    for (i = 0; i < 3; i++)
        ui_bmp_text_button_set_text_style(self->start_button, i, 0x12, 0xffffff, 1, 1, UI_TEXT_CENTRED, 0x39);
    ui_bmp_text_button_set_text_style(self->start_button, 3, 0x12, 0x7f74dc, 1, 0, UI_TEXT_CENTRED, 0x39);
    ui_window_add_child(wnd, &self->start_button->bmp.button.base);

    self->delete_button = add_info_button(0x33e, 0x15f, 0xd0e, 0);
    self->delete_button->bmp.button.base.id = ID_DELETE;
    ui_button_set_show((struct UIButton *)(self->delete_button), 0);
    ui_window_add_child(wnd, &self->delete_button->bmp.button.base);

    self->left_button = new_bitmap_button();
    set_left_bitmaps(self, self->page == 0);
    ui_window_create(&self->left_button->button.base, self->left_button->bitmap_width,
                     self->left_button->bitmap_height);
    ui_window_move_base(&self->left_button->button.base, 0x33e, 0x22e);
    self->left_button->button.base.id = ID_PAGE_LEFT;
    ui_window_add_child(wnd, &self->left_button->button.base);

    self->right_button = new_bitmap_button();
    set_right_bitmaps(self, self->page >= self->storage.page_count - 1);
    ui_window_create(&self->right_button->button.base, self->right_button->bitmap_width,
                     self->right_button->bitmap_height);
    ui_window_move_base(&self->right_button->button.base, 0x3ab, 0x22e);
    self->right_button->button.base.id = ID_PAGE_RIGHT;
    ui_window_add_child(wnd, &self->right_button->button.base);

    title = new_static_text();
    width = ui_window_text_width(wnd, msg_str(0xd0f), 0, 0, 0xc, 1, 0);
    height = ui_window_text_height(wnd, msg_str(0xd0f), 0, 0, 0xc, 1, 0);
    ui_window_create(title, width, height);
    ui_static_text_set_text(title, msg_str(0xd0f), 0);
    ui_static_text_set_text_colour(title, 0x57, 0x66, 0x91);
    ui_window_move(title, 0x380 - width / 2, 0x21a);
    ui_window_add_child(wnd, title);

    memset(text, 0, sizeof text);
    sprintf(text, "%d / %d", self->page + 1, self->storage.page_count);
    self->page_label = new_static_text();
    width = ui_window_text_width(wnd, text, 0, 0, 0x12, 1, 0);
    height = ui_window_text_height(wnd, text, 0, 0, 0x12, 1, 0);
    ui_window_create(self->page_label, width, height);
    ui_static_text_set_text(self->page_label, text, 0);
    ui_static_text_set_font(self->page_label, 0, 0x12, 1, 0);
    ui_static_text_set_text_colour(self->page_label, 0xff, 0xff, 0xff);
    ui_window_move(self->page_label, 0x380 - width / 2, 0x234);
    ui_window_add_child(wnd, self->page_label);

    for (i = 0; i < INFO_LINES; i++) {
        INFO_LINE(self, i) = new_static_text();
        ui_static_text_set_font(INFO_LINE(self, i), 0, 0xb, 0, 0);
        ui_window_create(INFO_LINE(self, i), 0x6b, 0xf);
        if (i == 11)
            ui_window_move((INFO_LINE(self, i)), 0x364, 0x74);
        else
            ui_window_move((INFO_LINE(self, i)), 0x364, i * 0x11 + 0x85);
        ui_window_add_child(wnd, INFO_LINE(self, i));
    }

    for (i = 0; i < self->storage.slot_count; i++) {
        struct CharInfo *info = ui_char_info(i);
        struct IntIntPair *gid;

        if (!info)
            continue;
        if (info->sex == CHAR_SEX_ACCOUNT)
            info->sex = (BYTE)g_session1.accountSex;
        if (session_is_bard_or_dancer(&g_session1, info->job, info->sex) == 1) {
            char message[0x104];

            memset(message, 0, sizeof message);
            sprintf(message, msg_str(0x974), i, 0x9c41);
            ui_window_mgr_message_box(&g_windowMgr, message, UI_MSGBOX_OK, 1, 0, 0, "메시지", 0x118, 0x78,
                                   0);
            mode_send_message(client_current_mode(), LOGIN_REQ_BACK, 0, 0, 0, 0);
            break;
        }
        gid = tree_get(&self->gids, &i, sizeof i, sizeof *gid, tree_less_int);
        gid->value = info->gid;
        if (info->slot >= self->storage.slot_count)
            continue;
        fill_preview(self, i, info);
    }

    for (i = 0; i < self->storage.slot_count; i++) {
        int col;
        struct CharInfo *info;

        if (i / self->storage.per_page != self->page)
            continue;
        col = page_column(i, self->page, self->storage.per_page);
        label = new_static_text();
        ui_window_create(label, 0x64, 0x14);
        ui_window_move(label, slot_x(col) + 0x58, slot_y(col) + 0x15);
        ui_static_text_set_font(label, 0, 0xb, 1, 0);
        ui_static_text_set_text_colour(label, 0x37, 0x41, 0x77);
        ui_window_add_child(wnd, label);
        NAME_LABEL(self, col) = label;
        info = ui_char_info(i);
        if (!info)
            continue;
        ui_static_text_set_text_ellipsis(label, info->name);
        place_name(self, label, col);
    }
    ui_new_select_char_wnd_update_info(self);
    self->frame.default_id = ID_START;
}

static void draw_slot_kind(struct UINewSelectCharWnd *self, int index)
{
    int col = index % self->storage.per_page;
    int normal = g_session1.charListCounts[1], premium = g_session1.charListCounts[2];
    const char *kind;

    if (premium > 0 && index >= normal && index < normal + premium)
        kind = "Premium ";
    else if (g_session1.charListCounts[3] > 0 && index >= normal + premium &&
             index < g_session1.charListCounts[3] + normal + premium)
        kind = "Billing ";
    else
        return;
    ui_window_text_out(WND(self), col * 0xa3 + 0x88, 0x32, kind, 0, 1, 0xf, 0xff, 0, 0);
    ui_window_text_out(WND(self), col * 0xa3 + 0x8e, 0x3f, "Service", 0, 1, 0xf, 0xff, 0, 0);
}

static void draw_character(struct UINewSelectCharWnd *self, const struct CharInfo *info,
                           const struct CharPreview *preview)
{
    BYTE sex = info->sex == CHAR_SEX_ACCOUNT ? (BYTE)g_session1.accountSex : info->sex;

    struct CharacterDrawOptions options = {
        .x = preview->x,
        .y = preview->y,
        .sex = sex,
        .job = info->job,
        .body = info->body,
        .head = info->head,
        .accessory = info->accessory,
        .accessory2 = info->accessory2,
        .accessory3 = info->accessory3,
        .robe = info->robe,
        .riding = 0,
        .scaled = 0,
        .body_palette = info->body_palette,
        .head_palette = info->head_palette,
        .action = preview->action,
        .motion = preview->frame,
        .start_time = self->created_time,
    };
    ui_draw_character(WND(self), &options, 1);
}

static void update_slot_button(struct UINewSelectCharWnd *self, int index)
{
    int col = index / self->storage.per_page ? index % (self->storage.per_page * (index / self->storage.per_page)) : index;
    BYTE empty;

    if (col >= self->storage.per_page || index >= self->storage.slot_count)
        return;
    ui_button_show((struct UIButton *)(SLOT_BUTTON(self, col)), 1);
    empty = self->storage.empty[index];
    if (index >= g_session1.charListCounts[4]) {
        if (empty != 1)
            return;
    } else if (empty != 1 || index < g_session1.charListCounts[6]) {
        return;
    }
    button_disable(SLOT_BUTTON(self, col), 1);
}

static BYTE update_deletion(struct UINewSelectCharWnd *self, const struct CharInfo *info, int index)
{
    struct CharPreview *preview;

    if (index >= self->storage.slot_count || !info)
        return 0;
    preview = &self->storage.previews[index];
    if (info->delete_date) {
        struct Time t;

        time_construct(&t, info->delete_date);
        if (index == self->cur && time_is_past(&t) == 1 &&
            self->delete_button->bmp.button.disabled == 1)
            ui_button_set_show((struct UIButton *)(self->delete_button), 1);
        preview->action_base = 0x10;
        preview->action = 0x10;
        time_destruct(&t);
    } else {
        preview->action_base = 0;
        preview->action = 0;
    }
    return 1;
}

static void update_delete_time(struct UINewSelectCharWnd *self)
{
    struct CharInfo *info = ui_char_info(g_loginCharInfo.slot);
    struct Time t;

    if (!info || info->delete_date <= 0)
        return;
    time_construct(&t, info->delete_date);
    button_disable(self->delete_button, time_is_past(&t) == 1 ? 0 : 1);
    time_destruct(&t);
}

static int deletion_passed(int delete_date)
{
    struct Time t;
    int passed;

    time_construct(&t, delete_date);
    passed = time_is_past(&t) == 1;
    time_destruct(&t);
    return passed;
}

static unsigned long long deletion_key(unsigned long long key, const struct CharInfo *info)
{
    if (!info)
        return draw_key_int(key, -1);
    key = draw_key_int(key, info->delete_date);
    return draw_key_int(key, info->delete_date ? deletion_passed(info->delete_date) : 0);
}

static unsigned long long select_char_key(struct UINewSelectCharWnd *self)
{
    unsigned long long key = DRAW_KEY_START;
    int i;

    key = draw_key_int(key, self->page);
    key = draw_key_int(key, self->cur);
    key = draw_key_bytes(key, g_session1.charListCounts, sizeof g_session1.charListCounts);
    key = draw_key_int(key, self->delete_button->bmp.button.disabled);
    key = draw_key_int(key, g_loginCharInfo.slot);
    key = deletion_key(key, ui_char_info(g_loginCharInfo.slot));
    for (i = 0; i < self->storage.per_page; i++) {
        int index = self->page * self->storage.per_page + i;

        if (index >= self->storage.slot_count)
            continue;
        key = draw_key_int(key, self->storage.empty[index]);
        key = deletion_key(key, ui_char_info(index));
    }
    return draw_key_children(key, WND(self));
}

static void draw_characters(struct UINewSelectCharWnd *self)
{
    int i;

    for (i = 0; i < self->storage.per_page; i++) {
        int index = self->page * self->storage.per_page + i;
        struct CharInfo *info;

        if (index >= self->storage.slot_count)
            continue;
        info = ui_char_info(index);
        if (!self->storage.empty[index] && info)
            draw_character(self, info, &self->storage.previews[index]);
    }
}

BYTE ui_new_select_char_wnd_render(struct UIWindow *window, int force)
{
    struct UINewSelectCharWnd *self = (struct UINewSelectCharWnd *)window;
    struct UIWindow *button = SLOT_BUTTON(self, self->cur);

    if (button && ui_slot_bmp_button_changed((struct UISlotBmpButton *)button))
        ui_window_invalidate_children(button);
    if (select_char_key(self) != self->drawn_key)
        ui_window_invalidate_base(window);
    draw_characters(self);
    return ui_window_render_base(window, force);
}

void ui_new_select_char_wnd_on_draw(struct UIWindow *window)
{
    struct UINewSelectCharWnd *self = (struct UINewSelectCharWnd *)window;
    int i;

    ui_window_clear_canvas(WND(self), UI_TRANSPARENT);
    for (i = 0; i < self->storage.per_page; i++) {
        int index = self->page * self->storage.per_page + i;
        struct CharInfo *info;

        if (index >= self->storage.slot_count)
            continue;
        info = ui_char_info(index);
        draw_slot_kind(self, index);
        update_slot_button(self, index);
        update_deletion(self, info, index);
    }
    update_delete_time(self);
    self->drawn_key = select_char_key(self);
}

void ui_new_select_char_wnd_on_l_btn_down(struct UIWindow *self, int x, int y)
{
}

void ui_new_select_char_wnd_on_process(struct UIWindow *window)
{
    struct UINewSelectCharWnd *self = (struct UINewSelectCharWnd *)window;
    struct UIWindow *top, *button;

    if (g_mouse.button[MOUSE_LEFT] != 3 || !g_windowMgr.menu_wnd) {
        if (g_mouse.button[MOUSE_LEFT])
            ui_window_invalidate_base(WND(self));
    } else {
        ui_close_window(0x12);
        if (g_mouse.button[MOUSE_LEFT])
            ui_window_invalidate_base(WND(self));
    }
    top = ui_get_window(0x5d);
    if (top)
        ui_window_mgr_raise_window(&g_windowMgr, top);
    button = SLOT_BUTTON(self, self->cur);
    if (button)
        ui_window_on_process(button);
}

void ui_new_select_char_wnd_hide_button(struct UINewSelectCharWnd *self, int id)
{
    if (id == ID_MAKE)
        ui_button_show((struct UIButton *)(self->make_button), 0);
    else if (id == ID_DEL)
        ui_button_show((struct UIButton *)(self->del_button), 0);
}

void ui_new_select_char_wnd_show_button(struct UINewSelectCharWnd *self, int id)
{
    if (id == ID_MAKE)
        ui_button_show((struct UIButton *)(self->make_button), 1);
    else if (id == ID_DEL)
        ui_button_show((struct UIButton *)(self->del_button), 1);
}

static void send_packet(int id, const void *packet)
{
    int len = rag_connection_get_packet_length(rag_connection_instance(), id);

    rag_connection_send_packet(rag_connection_instance(), len, packet);
}

#pragma pack(push, 1)
struct PacketCharId {
    WORD id;
    int value;
};
struct PacketMoveChar {
    WORD id;
    WORD from, to;
    WORD count;
};
#pragma pack(pop)

static int refuse_blocked(const struct CharInfo *info)
{
    int i;

    for (i = 0; i < g_session1.charListCounts[5]; i++) {
        if (info->gid == g_session1.blockedChars[i].gid) {
            char text[0x104];
            char name[sizeof g_session1.blockedChars[i].name + 1];

            memcpy(name, g_session1.blockedChars[i].name, sizeof g_session1.blockedChars[i].name);
            name[sizeof name - 1] = '\0';
            memset(text, 0, sizeof text);
            sprintf(text, msg_str(0x337), name);
            ui_window_mgr_message_box(&g_windowMgr, text, UI_MSGBOX_OK, 1, 1, 0, "메시지", 0x118, 0x78, 0);
            return 1;
        }
    }
    return 0;
}

static int question(int msg_id)
{
    return ui_window_mgr_message_box(&g_windowMgr, msg_str(msg_id), UI_MSGBOX_QUESTION, 1, 0, 0, "메시지",
                                  0x118, 0x78, 0);
}

static void refresh_page(struct UINewSelectCharWnd *self)
{
    unsigned i;

    set_page_label(self);
    for (i = 0; i < (unsigned)self->storage.slot_count; i++) {
        int col;
        struct UIWindow *button;
        struct CharInfo *info;

        if (i / (unsigned)self->storage.per_page != (unsigned)self->page)
            continue;
        col = self->page ? i % (unsigned)(self->page * self->storage.per_page) : i;
        button = SLOT_BUTTON(self, col);
        if (self->storage.empty[i] == 1) {
            ui_slot_bmp_button_set_slot((struct UISlotBmpButton *)button, -1);
            ui_static_text_set_text(NAME_LABEL(self, col), "", 0);
            if (i < (unsigned)g_session1.charListCounts[4])
                ui_window_send_msg(button, NULL, SLOT_MSG_SET_STYLE, 0, 0, 0, 0);
            else
                button_disable(button, 1);
            continue;
        }
        ui_window_send_msg(button, NULL, SLOT_MSG_SET_STYLE, 1, 0, 0, 0);
        info = ui_char_info(i);
        if (!info)
            continue;
        ui_static_text_set_text_ellipsis(NAME_LABEL(self, col), info->name);
        place_name(self, NAME_LABEL(self, col), col);
        ui_slot_bmp_button_set_slot((struct UISlotBmpButton *)SLOT_BUTTON(self, col), i);
    }
    ui_window_invalidate_children(WND(self));
    ui_new_select_char_wnd_update_info(self);
}

static void select_cur(struct UINewSelectCharWnd *self)
{
    int i;

    for (i = 0; i < self->storage.per_page; i++)
        ui_window_send_msg((SLOT_BUTTON(self, i)), WND(self), UI_MSG_SET_STATE, i == self->cur, 0, 0, 0);
    store_selected_slot(self);
    ui_new_select_char_wnd_update_info(self);
    ui_new_select_char_wnd_write_to_reg(self);
}

static int key_blocked(struct UINewSelectCharWnd *self)
{
    if (g_windowMgr.menu_wnd)
        ui_close_window(0x12);
    return g_windowMgr.modal_wnd || self->busy == 1;
}

static void move_back(struct UINewSelectCharWnd *self, int step)
{
    int old = self->cur;

    if (key_blocked(self))
        return;
    self->cur -= step;
    if (self->cur < 0) {
        if (self->page <= 0) {
            self->cur = step == 1 ? 0 : old;
            return;
        }
        send_command(WND(self), NULL, ID_PAGE_LEFT, 0);
        if (step == 1)
            self->cur = self->storage.per_page - 1;
        else
            self->cur += self->storage.per_page;
    }
    select_cur(self);
}

static void move_on(struct UINewSelectCharWnd *self, int step)
{
    int cur;

    if (key_blocked(self))
        return;
    cur = self->cur + step;
    self->cur = cur;
    if (self->page * self->storage.per_page + cur >= self->storage.listed_slots) {
        self->cur = cur - step;
        return;
    }
    if (cur >= self->storage.per_page) {
        if (self->page < self->storage.page_count - 1)
            send_command(WND(self), NULL, ID_PAGE_RIGHT, 0);
        self->cur = self->cur % (self->storage.per_page * self->page);
    }
    select_cur(self);
}

static void on_start(struct UINewSelectCharWnd *self)
{
    struct CharInfo *info = ui_char_info(g_loginCharInfo.slot);

    if (!info) {
        send_command(WND(self), NULL, ID_SELECT_PAGE_SLOT, self->cur);
        return;
    }
    if (info->delete_date > 0)
        return;
    session_set_character_name(&g_session1, text_view(info->name, sizeof info->name));
    session_set_sex(&g_session1,
                     info->sex == CHAR_SEX_ACCOUNT ? g_session1.accountSex : info->sex);
    self->busy = 1;
    if (refuse_blocked(info)) {
        self->busy = 0;
        return;
    }
    sound_play_wave(SOUND_BUTTON, 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
    mode_send_message(client_current_mode(), MODE_MSG_UI_COMMAND, LOGIN_CMD_START, 0, 0, 0);
    if (self->start_button) {
        ui_bitmap_button_set_bitmap(&self->start_button->bmp, SELECT_DIR "bt_gameStart_on.bmp", 0, 0);
        ui_bitmap_button_set_bitmap(&self->start_button->bmp, SELECT_DIR "bt_gameStart_on.bmp", 1, 0);
        ui_bitmap_button_set_bitmap(&self->start_button->bmp, SELECT_DIR "bt_gameStart_on.bmp", 2, 0);
    }
}

static void on_back(struct UINewSelectCharWnd *self)
{
    account_user_data_save(g_accountUserDataMgr);
    if (g_session1.canBackToLogin == 1) {
        if (question(0xd68) == UI_MSGBOX_ANSWER_YES)
            mode_send_message(client_current_mode(), LOGIN_REQ_BACK_TO_LOGIN, 0, 0, 0, 0);
    } else if (question(0x11) == UI_MSGBOX_ANSWER_YES) {
        mode_send_message(client_current_mode(), LOGIN_REQ_BACK, 0, 0, 0, 0);
        ui_window_mgr_post_quit2(&g_windowMgr, WND(self));
    }
}

static void on_slot(struct UINewSelectCharWnd *self, struct UIWindow *sender, int how)
{
    int i;

    if (g_windowMgr.modal_wnd)
        return;
    for (i = 0; i < self->storage.per_page; i++) {
        struct UIWindow *button = SLOT_BUTTON(self, i);

        if (sender != button) {
            ui_window_send_msg(button, WND(self), UI_MSG_SET_STATE, 0, 0, 0, 0);
            continue;
        }
        self->cur = i;
        store_selected_slot(self);
        ui_window_send_msg((SLOT_BUTTON(self, i)), WND(self), UI_MSG_SET_STATE, 1, 0, 0, 0);
        if (how == SLOT_VAL_R_BTN_DOWN)
            continue;
        if (self->storage.empty[g_loginCharInfo.slot] && !((struct UISlotBmpButton *)SLOT_BUTTON(self, i))->style) {
            send_command(WND(self), NULL, ID_SELECT_PAGE_SLOT, self->cur);
            break;
        }
    }
    if (how == SLOT_VAL_R_BTN_DOWN) {
        BYTE flag = 0;
        int slot = g_loginCharInfo.slot;
        struct CharInfo *info = ui_char_info(slot);

        if (self->move_from != slot) {
            if (slot >= self->storage.listed_slots && !info)
                goto done;
            if (self->move_from != -1)
                flag = login_mode_add_notice((struct LoginMode *)login_mode(), NOTICE_MOVE_PENDING, 0);
            if (!info)
                goto done;
            if (info->slot_change_count > 0)
                flag = login_mode_add_notice((struct LoginMode *)login_mode(), NOTICE_CAN_MOVE, flag);
        }
        if (info && info->rename_count > 0)
            login_mode_add_notice((struct LoginMode *)login_mode(), NOTICE_CAN_RENAME, flag);
    }
done:
    ui_new_select_char_wnd_update_info(self);
    ui_new_select_char_wnd_write_to_reg(self);
    ui_window_invalidate_base(WND(self));
}

static void on_delete(struct UINewSelectCharWnd *self)
{
    struct CharInfo *info;

    if (self->storage.empty[g_loginCharInfo.slot] == 1)
        return;
    info = ui_char_info(g_loginCharInfo.slot);
    if (refuse_blocked(info))
        return;
    if (ui_window_mgr_message_box(&g_windowMgr, msg_str(0x13), UI_MSGBOX_QUESTION, 0, 0, 0, "메시지", 0x118,
                               0x78, 0) != UI_MSGBOX_ANSWER_YES)
        return;
    self->busy = 1;
    if (ui_window_mgr_run_email_address_box(&g_windowMgr) == UI_ID_OK) {
        mode_send_message(client_current_mode(), MODE_MSG_UI_COMMAND, LOGIN_CMD_DELETE, 0, 0, 0);
        ui_window_mgr_post_quit2(&g_windowMgr, WND(self));
    }
    self->busy = 0;
}

static void on_page_left(struct UINewSelectCharWnd *self)
{
    if (self->page <= 0)
        return;
    self->page--;
    set_left_bitmaps(self, self->page == 0);
    set_right_bitmaps(self, 0);
    refresh_page(self);
}

static void on_page_right(struct UINewSelectCharWnd *self)
{
    if (self->page >= self->storage.page_count - 1)
        return;
    self->page++;
    set_right_bitmaps(self, self->page >= self->storage.page_count - 1);
    set_left_bitmaps(self, 0);
    refresh_page(self);
}

static void on_name_changed(struct UINewSelectCharWnd *self)
{
    struct CharInfo *info = ui_char_info(g_loginCharInfo.slot);
    int col;

    if (info->gid != g_session1.renameCharId)
        return;
    memcpy(info->name, g_session1.renameNewName, sizeof info->name);
    col = g_loginCharInfo.slot - self->storage.per_page * self->page;
    ui_static_text_set_text_ellipsis(NAME_LABEL(self, col), info->name);
    place_name(self, NAME_LABEL(self, col), col);
}

static void on_move_to(struct UINewSelectCharWnd *self)
{
    int to = g_loginCharInfo.slot;

    if (question(0x7e0) == UI_MSGBOX_ANSWER_YES) {
        struct CharInfo *info = ui_char_info(self->move_from);

        if (info) {
            struct PacketMoveChar packet;

            packet.id = 0x8d4;
            packet.from = (WORD)self->move_from;
            packet.to = (WORD)to;
            packet.count = (WORD)info->slot_change_count;
            send_packet(0x8d4, &packet);
        }
    }
    self->move_from = -1;
}

static void on_command(struct UINewSelectCharWnd *self, struct UIWindow *sender, int id, int val2)
{
    struct PacketCharId packet;
    struct UIWindow *wnd;
    int slot, index;

    switch (id) {
    case ID_START:
        on_start(self);
        break;
    case ID_BACK:
        on_back(self);
        break;
    case ID_SELECT_PAGE_SLOT:
        if (self->cur != val2) {
            self->cur = val2;
            ui_new_select_char_wnd_send_msg(WND(self), SLOT_BUTTON(self, val2), UI_MSG_COMMAND, ID_SLOT, 0, 0, 0);
        }
        slot = g_loginCharInfo.slot;
        if (self->storage.empty[slot] == 1 && slot < g_session1.charListCounts[4] && ui_make_window(0x116))
            ui_close_window(0x115);
        break;
    case ID_SLOT:
        on_slot(self, sender, val2);
        break;
    case ID_DELETE:
        on_delete(self);
        break;
    case ID_ERROR_TEXT_A:
    case ID_ERROR_TEXT_B: {
        char text[0x104];

        memset(text, 0, sizeof text);
        mode_send_message(client_current_mode(), LOGIN_REQ_ERROR_TEXT, (int)text, 0, 0, 0);
        ui_window_mgr_message_box(&g_windowMgr, text, UI_MSGBOX_OK, 1, 0, 0, "메시지", 0x118, 0x78, 0);
        break;
    }
    case ID_PAGE_LEFT:
        on_page_left(self);
        break;
    case ID_PAGE_RIGHT:
        on_page_right(self);
        break;
    case ID_CHANGE_NAME: {
        struct CharInfo *info;

        if (g_windowMgr.change_name_wnd)
            ui_close_window(0x80);
        info = ui_char_info(g_loginCharInfo.slot);
        wnd = ui_make_window(0x80);
        if (wnd)
            ui_window_send_msg(wnd, NULL, UI_MSG_SET_INFO, (int)info, 0, 0, 0);
        break;
    }
    case ID_NAME_CHANGED:
        on_name_changed(self);
        break;
    case ID_DELETE_RESERVE:
        slot = g_loginCharInfo.slot;
        if (self->storage.empty[slot] == 1)
            break;
        packet.id = 0x827;
        packet.value = ui_char_info(slot)->gid;
        send_packet(0x827, &packet);
        break;
    case ID_DELETE_CANCEL: {
        struct IntIntPair *gid;

        index = g_loginCharInfo.slot;
        packet.id = 0x82b;
        gid = tree_get(&self->gids, &index, sizeof index, sizeof *gid, tree_less_int);
        packet.value = gid->value;
        send_packet(0x82b, &packet);
        break;
    }
    case ID_MAKE:
        ui_make_window(0xc8);
        ui_window_send_msg(g_windowMgr.second_password_edit_wnd, NULL, UI_MSG_COMMAND, 0x11c, 0, 0, 0);
        ui_window_send_msg(g_windowMgr.second_password_edit_wnd, NULL, UI_MSG_COMMAND, UI_ID_OK, 1, 0, 0);
        break;
    case ID_DEL:
        ui_make_window(0xc8);
        break;
    case ID_PIN_ENABLE:
        packet.id = 0x8c5;
        packet.value = g_session1.ownAid;
        send_packet(0x8c5, &packet);
        break;
    case ID_MOVE_FROM:
        self->move_from = g_loginCharInfo.slot;
        break;
    case ID_MOVE_TO:
        on_move_to(self);
        break;
    case ID_PRESS_SLOT_0:
    case ID_PRESS_SLOT_1:
    case ID_PRESS_SLOT_2:
        ui_window_on_l_btn_down((SLOT_BUTTON(self, id - ID_PRESS_SLOT_0)), 0, 0);
        send_command(WND(self), NULL, ID_MOVE_FROM, 0);
        break;
    }
}

int ui_new_select_char_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                          int msg, int val1, int val2, int val3, int val4)
{
    struct UINewSelectCharWnd *self = (struct UINewSelectCharWnd *)window;
    if (g_windowMgr.second_password_edit_wnd || g_windowMgr.second_password_input_wnd || ui_get_window(0x116))
        return 0;
    switch (msg) {
    case UI_MSG_COMMAND:
        on_command(self, sender, val1, val2);
        return 0;
    case UI_MSG_SET_STATE:
        self->cur = val1;
        return 0;
    case MSG_ROW_UP:
        move_back(self, COLUMNS);
        return 0;
    case MSG_ROW_DOWN:
        move_on(self, COLUMNS);
        return 0;
    case MSG_SLOT_LEFT:
        move_back(self, 1);
        return 0;
    case MSG_SLOT_RIGHT:
        move_on(self, 1);
        return 0;
    case MSG_DELETE_KEY:
        send_command(WND(self), NULL, ID_DELETE, 0);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

#ifndef _UCRT
#endif
