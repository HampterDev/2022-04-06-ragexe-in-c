#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include "ui/hotkeys/hotkey_guide.h"
#include "ui/hotkeys/hotkeys_std.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "std/tree.h"

struct Vec;
struct UIHotkeyGuideWnd;
struct UIWindow;
void ui_hotkey_guide_wnd_destroy(struct UIWindow *window);
void ui_hotkey_guide_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_hotkey_guide_wnd_on_process(struct UIWindow *self);
void ui_hotkey_guide_wnd_on_draw(struct UIWindow *self);
int ui_hotkey_guide_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_hotkey_guide_wnd_ops = {
    .destroy = ui_hotkey_guide_wnd_destroy,
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
    .on_create = ui_hotkey_guide_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_hotkey_guide_wnd_on_process,
    .on_draw = ui_hotkey_guide_wnd_on_draw,
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
    .send_msg = ui_hotkey_guide_wnd_send_msg,
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

#define UI_WND_HOTKEY_GUIDE 0x12f
#define UI_WND_HOTKEY_SETTINGS 0x9c
#define HOTKEY_GUIDE_ID_SETTINGS 0x14e
#define HOTKEY_GUIDE_NONE_LIT 0xf91
#define HOTKEY_GUIDE_ESC_LABEL 0x109
#define VK_ESC 0x1b
#define HOTKEY_SETTINGS_MSG_FROM_GUIDE 0x16

#define HOTKEY_GUIDE_FONT 0xc
#define HOTKEY_GUIDE_TEXT 0x2b2625
#define HOTKEY_GUIDE_SHORTCUT 0xa55139
#define HOTKEY_GUIDE_HEADER 0x5b2c24
#define HOTKEY_GUIDE_ROWS 6
#define HOTKEY_GUIDE_COLUMN 0xf6
#define HOTKEY_GUIDE_ROW 0x1e

#define MSG_HOTKEY_GUIDE_TITLE 0xc38
#define MSG_HOTKEY_GUIDE_KEY 0xc39
#define MSG_HOTKEY_GUIDE_CLOSE 0xbe5
#define MSG_HOTKEY_GUIDE_KEYBOARD 0xc3a
#define MSG_HOTKEY_GUIDE_WINDOWS 0xc3b
#define MSG_HOTKEY_GUIDE_SETTINGS_A 0xc3c
#define MSG_HOTKEY_GUIDE_SETTINGS_B 0xc3d
#define MSG_HOTKEY_GUIDE_SETTINGS 0xc4c
#define MSG_HOTKEY_GUIDE_NOTE_A 0xc43
#define MSG_HOTKEY_GUIDE_NOTE_B 0xc44

struct HotkeyGuideItem {
    int msg;
    int vk;
    int modifier;
};
SIZE_ASSERT(HotkeyGuideItem, 0xc);

struct UIHotkeyGuideWnd {
    struct UIFrameWnd frame;
    struct Tree caps;
    struct Vec items;
    struct Vec buttons;
    int held;
    struct Vec lit;
};
OFFSET_ASSERT(UIHotkeyGuideWnd, items, 0x9c);
OFFSET_ASSERT(UIHotkeyGuideWnd, lit, 0xb8);
SIZE_ASSERT(UIHotkeyGuideWnd, 0xc4);

struct KeyCapDef {
    int vk;
    const char *label;
    int msg;
    struct KeyCapBox boxes[2];
};
#define BOX(x, y, align, width, inset, height) { 1, (x), (y), (align), (width), (inset), (height) }
#define KEY(x, y) BOX(x, y, 6, 0x24, 0, 0x24)
#define NO_BOX { 0 }
static const struct KeyCapDef key_caps[] = {
    { 0x08, "\xe2\x86\x90" /* "←" */, 0, { BOX(0x210, 0x5d, 0, 0x4b, 0xf, 0x24), NO_BOX } },
    { 0x09, "Tab", 0, { BOX(0x15, 0x84, 0, 0x37, 0xf, 0x24), NO_BOX } },
    { 0x0d, "Enter", 0, { BOX(0x210, 0xab, 2, 0x4b, 0x14, 0x24), BOX(0x36c, 0xd2, 6, 0x24, 0x0, 0x4b) } },
    { 0x10, "Shift", 0, { BOX(0x15, 0xd2, 0, 0x5e, 0xf, 0x24), BOX(0x1fc, 0xd2, 2, 0x5f, 0x14, 0x24) } },
    { 0x11, "Ctrl", 0, { BOX(0x15, 0xf9, 0, 0x37, 0xf, 0x24), NO_BOX } },
    { 0x12, "Alt", 0, { BOX(0x76, 0xf9, 6, 0x31, 0x0, 0x24), NO_BOX } },
    { 0x13, "Pause", 0, { KEY(0x2be, 0x2e), NO_BOX } },
    { 0x14, "Caps", 0, { BOX(0x15, 0xab, 0, 0x4b, 0xf, 0x24), NO_BOX } },
    { 0x19, NULL, 0xc4b, { BOX(0x223, 0xf9, 6, 0x38, 0x0, 0x24), NO_BOX } },
    { 0x1b, "Esc", 0, { KEY(0x15, 0x2e), NO_BOX } },
    { 0x20, "Space", 0, { BOX(0xaa, 0xf9, 6, 0xf6, 0x0, 0x24), NO_BOX } },
    { 0x21, "PgUp", 0, { KEY(0x2be, 0x5d), NO_BOX } },
    { 0x22, "PgDn", 0, { KEY(0x2be, 0x84), NO_BOX } },
    { 0x23, "End", 0, { KEY(0x297, 0x84), NO_BOX } },
    { 0x24, "Home", 0, { KEY(0x297, 0x5d), NO_BOX } },
    { 0x25, "\xe2\x86\x90" /* "←" */, 0, { KEY(0x270, 0xf9), NO_BOX } },
    { 0x26, "\xe2\x86\x91" /* "↑" */, 0, { KEY(0x297, 0xd2), NO_BOX } },
    { 0x27, "\xe2\x86\x92" /* "→" */, 0, { KEY(0x2be, 0xf9), NO_BOX } },
    { 0x28, "\xe2\x86\x93" /* "↓" */, 0, { KEY(0x297, 0xf9), NO_BOX } },
    { 0x2c, "PrtSc", 0, { KEY(0x270, 0x2e), NO_BOX } },
    { 0x2d, "Insert", 0, { KEY(0x270, 0x5d), NO_BOX } },
    { 0x2e, "Delete", 0, { KEY(0x270, 0x84), NO_BOX } },
    { 0x30, "0", 0, { KEY(0x19b, 0x5d), NO_BOX } },
    { 0x31, "1", 0, { KEY(0x3c, 0x5d), NO_BOX } },
    { 0x32, "2", 0, { KEY(0x63, 0x5d), NO_BOX } },
    { 0x33, "3", 0, { KEY(0x8a, 0x5d), NO_BOX } },
    { 0x34, "4", 0, { KEY(0xb1, 0x5d), NO_BOX } },
    { 0x35, "5", 0, { KEY(0xd8, 0x5d), NO_BOX } },
    { 0x36, "6", 0, { KEY(0xff, 0x5d), NO_BOX } },
    { 0x37, "7", 0, { KEY(0x126, 0x5d), NO_BOX } },
    { 0x38, "8", 0, { KEY(0x14d, 0x5d), NO_BOX } },
    { 0x39, "9", 0, { KEY(0x174, 0x5d), NO_BOX } },
    { 0x41, "A", 0, { KEY(0x63, 0xab), NO_BOX } },
    { 0x42, "B", 0, { KEY(0x112, 0xd2), NO_BOX } },
    { 0x43, "C", 0, { KEY(0xc4, 0xd2), NO_BOX } },
    { 0x44, "D", 0, { KEY(0xb1, 0xab), NO_BOX } },
    { 0x45, "E", 0, { KEY(0x9d, 0x84), NO_BOX } },
    { 0x46, "F", 0, { KEY(0xd8, 0xab), NO_BOX } },
    { 0x47, "G", 0, { KEY(0xff, 0xab), NO_BOX } },
    { 0x48, "H", 0, { KEY(0x126, 0xab), NO_BOX } },
    { 0x49, "I", 0, { KEY(0x160, 0x84), NO_BOX } },
    { 0x4a, "J", 0, { KEY(0x14d, 0xab), NO_BOX } },
    { 0x4b, "K", 0, { KEY(0x174, 0xab), NO_BOX } },
    { 0x4c, "L", 0, { KEY(0x19b, 0xab), NO_BOX } },
    { 0x4d, "M", 0, { KEY(0x160, 0xd2), NO_BOX } },
    { 0x4e, "N", 0, { KEY(0x139, 0xd2), NO_BOX } },
    { 0x4f, "O", 0, { KEY(0x187, 0x84), NO_BOX } },
    { 0x50, "P", 0, { KEY(0x1ae, 0x84), NO_BOX } },
    { 0x51, "Q", 0, { KEY(0x4f, 0x84), NO_BOX } },
    { 0x52, "R", 0, { KEY(0xc4, 0x84), NO_BOX } },
    { 0x53, "S", 0, { KEY(0x8a, 0xab), NO_BOX } },
    { 0x54, "T", 0, { KEY(0xeb, 0x84), NO_BOX } },
    { 0x55, "U", 0, { KEY(0x139, 0x84), NO_BOX } },
    { 0x56, "V", 0, { KEY(0xeb, 0xd2), NO_BOX } },
    { 0x57, "W", 0, { KEY(0x76, 0x84), NO_BOX } },
    { 0x58, "X", 0, { KEY(0x9d, 0xd2), NO_BOX } },
    { 0x59, "Y", 0, { KEY(0x112, 0x84), NO_BOX } },
    { 0x5a, "Z", 0, { KEY(0x76, 0xd2), NO_BOX } },
    { 0x5d, "\xe2\x96\xa4" /* "▤" */, 0, { KEY(0x1fc, 0xf9), NO_BOX } },
    { 0x60, "0", 0, { KEY(0x2f7, 0xf9), NO_BOX } },
    { 0x61, "1", 0, { KEY(0x2f7, 0xd2), NO_BOX } },
    { 0x62, "2", 0, { KEY(0x31e, 0xd2), NO_BOX } },
    { 0x63, "3", 0, { KEY(0x345, 0xd2), NO_BOX } },
    { 0x64, "4", 0, { KEY(0x2f7, 0xab), NO_BOX } },
    { 0x65, "5", 0, { KEY(0x31e, 0xab), NO_BOX } },
    { 0x66, "6", 0, { KEY(0x345, 0xab), NO_BOX } },
    { 0x67, "7", 0, { KEY(0x2f7, 0x84), NO_BOX } },
    { 0x68, "8", 0, { KEY(0x31e, 0x84), NO_BOX } },
    { 0x69, "9", 0, { KEY(0x345, 0x84), NO_BOX } },
    { 0x6a, "*", 0, { KEY(0x345, 0x5d), NO_BOX } },
    { 0x6b, "+", 0, { BOX(0x36c, 0x84, 6, 0x24, 0x0, 0x4b), NO_BOX } },
    { 0x6d, "-", 0, { KEY(0x36c, 0x5d), NO_BOX } },
    { 0x6e, "Del", 0, { KEY(0x345, 0xf9), NO_BOX } },
    { 0x6f, "/", 0, { KEY(0x31e, 0x5d), NO_BOX } },
    { 0x70, "F1", 0, { KEY(0x53, 0x2e), NO_BOX } },
    { 0x71, "F2", 0, { KEY(0x7a, 0x2e), NO_BOX } },
    { 0x72, "F3", 0, { KEY(0xa1, 0x2e), NO_BOX } },
    { 0x73, "F4", 0, { KEY(0xc8, 0x2e), NO_BOX } },
    { 0x74, "F5", 0, { KEY(0x10b, 0x2e), NO_BOX } },
    { 0x75, "F6", 0, { KEY(0x132, 0x2e), NO_BOX } },
    { 0x76, "F7", 0, { KEY(0x159, 0x2e), NO_BOX } },
    { 0x77, "F8", 0, { KEY(0x180, 0x2e), NO_BOX } },
    { 0x78, "F9", 0, { KEY(0x1c2, 0x2e), NO_BOX } },
    { 0x79, "F10", 0, { KEY(0x1e9, 0x2e), NO_BOX } },
    { 0x7a, "F11", 0, { KEY(0x210, 0x2e), NO_BOX } },
    { 0x7b, "F12", 0, { KEY(0x237, 0x2e), NO_BOX } },
    { 0x90, "Num", 0, { KEY(0x2f7, 0x5d), NO_BOX } },
    { 0x91, "Scroll", 0, { KEY(0x297, 0x2e), NO_BOX } },
    { 0xba, ";", 0, { KEY(0x1c2, 0xab), NO_BOX } },
    { 0xbb, "=", 0, { KEY(0x1e9, 0x5d), NO_BOX } },
    { 0xbc, ",", 0, { KEY(0x187, 0xd2), NO_BOX } },
    { 0xbd, "-", 0, { KEY(0x1c2, 0x5d), NO_BOX } },
    { 0xbe, ".", 0, { KEY(0x1ae, 0xd2), NO_BOX } },
    { 0xbf, "/", 0, { KEY(0x1d5, 0xd2), NO_BOX } },
    { 0xc0, "`", 0, { KEY(0x15, 0x5d), NO_BOX } },
    { 0xdb, "[", 0, { KEY(0x1d5, 0x84), NO_BOX } },
    { 0xdc, "\\", 0, { BOX(0x223, 0x84, 0, 0x38, 0xb, 0x24), NO_BOX } },
    { 0xdd, "]", 0, { KEY(0x1fc, 0x84), NO_BOX } },
    { 0xde, "'", 0, { KEY(0x1e9, 0xab), NO_BOX } },
};

static const int item_labels[18] = {
    0x69, 0x68, 0x11b, 0x6a, 0x67, 0xc3e, 0x525, 0xb2b, 0x931,
    HOTKEY_GUIDE_ESC_LABEL, 0xc3f, 0x92e, 0xc40, 0xa56, 0xc99, 0xc41, 0x23e, 0xc42,
};
static const int item_hotkeys[18] = {
    0x2f, 1, 2, 9, 5, 0xa, 3, 8, 0x2a, -1, 0x2e, 0x26, 0x33, 0x31, 6, 0x25, 0x11, 0xc,
};

struct KeyCapEntry {
    int vk;
    struct KeyCap cap;
};

static void destroy_key_cap(void *value)
{
    key_cap_destruct(&((struct KeyCapEntry *)value)->cap);
}

static struct KeyCap *key_cap_at(struct Tree *caps, int vk)
{
    struct KeyCapEntry *entry = tree_get(caps, &vk, sizeof vk, sizeof *entry, tree_less_int);

    return &entry->cap;
}

struct UIHotkeyGuideWnd *ui_hotkey_guide_wnd_construct(struct UIHotkeyGuideWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_hotkey_guide_wnd_ops;
    tree_init(&self->caps);
    self->items.first = self->items.last = self->items.end = NULL;
    self->buttons.first = self->buttons.last = self->buttons.end = NULL;
    self->lit.first = self->lit.last = self->lit.end = NULL;
    self->held = HOTKEY_GUIDE_NONE_LIT;
    return self;
}

struct UIWindow *ui_hotkey_guide_wnd_new(void)
{
    return (struct UIWindow *)ui_hotkey_guide_wnd_construct(mem_alloc(sizeof(struct UIHotkeyGuideWnd)));
}

static void destruct(struct UIHotkeyGuideWnd *self)
{
    self->frame.base.ops = &ui_hotkey_guide_wnd_ops;
    vec_free(&self->lit);
    vec_free(&self->buttons);
    vec_free(&self->items);
    tree_free(&self->caps, destroy_key_cap);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_hotkey_guide_wnd_destroy(struct UIWindow *window)
{
    struct UIHotkeyGuideWnd *self = (struct UIHotkeyGuideWnd *)window;
    destruct(self);
    mem_free(self);
}

void ui_hotkey_guide_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIHotkeyGuideWnd *self = (struct UIHotkeyGuideWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIBmpTextButton *settings;
    size_t i;

    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "", 0);

    for (i = 0; i < sizeof key_caps / sizeof key_caps[0]; i++) {
        const struct KeyCapDef *def = &key_caps[i];
        const char *label = def->label ? def->label : msg_str(def->msg);
        struct String text;
        struct KeyCap cap;

        string_init(&text);
        string_assign_cstr(&text, label);
        key_cap_construct(&cap, text, def->boxes[0], def->boxes[1]);
        key_cap_assign(key_cap_at(&self->caps, def->vk), &cap);
        key_cap_destruct(&cap);
    }

    for (i = 0; i < 18; i++) {
        struct HotKey key;
        struct HotkeyGuideItem item;

        session_get_hot_key(&g_session1, &key, 1, item_hotkeys[i]);
        item.msg = item_labels[i];
        if (item_labels[i] == HOTKEY_GUIDE_ESC_LABEL) {
            item.vk = VK_ESC;
            item.modifier = 0;
        } else {
            item.vk = key.code;
            item.modifier = key.modifier;
        }
        vec_push(&self->items, &item, sizeof item);
        string_free(&key.action);
        string_free(&key.label);
    }

    for (i = 0; i < (size_t)((struct HotkeyGuideItem *)self->items.last - (struct HotkeyGuideItem *)self->items.first);
         i++) {
        const struct HotkeyGuideItem *item = (struct HotkeyGuideItem *)self->items.first + i;
        struct UIBasicButton *button = mem_alloc(sizeof *button);
        struct UIWindow *child;

        if (button)
            button = ui_basic_button_construct(button);
        child = &button->button.base;
        ui_basic_button_set_label(child, 0x76, msg_str(item->msg));
        ui_window_move(child, (int)(i / HOTKEY_GUIDE_ROWS) * HOTKEY_GUIDE_COLUMN + 0x1a,
                 (int)(i % HOTKEY_GUIDE_ROWS) * HOTKEY_GUIDE_ROW + 0x136);
        child->id = item->msg;
        ui_window_add_child(wnd, child);
        vec_push(&self->buttons, &child, sizeof(struct UIWindow *));
    }

    settings = mem_alloc(sizeof *settings);
    if (settings)
        settings = ui_bmp_text_button_construct(settings);
    ui_bitmap_button_set_bitmap(&settings->bmp, UI_SKIN "hotkey_guide\\bt_keysettings_normal.bmp", 0, 0);
    ui_bitmap_button_set_bitmap(&settings->bmp, UI_SKIN "hotkey_guide\\bt_keysettings_over.bmp", 1, 0);
    ui_bitmap_button_set_bitmap(&settings->bmp, UI_SKIN "hotkey_guide\\bt_keysettings_press.bmp", 2, 0);
    ui_window_create(&settings->bmp.button.base, settings->bmp.bitmap_width, settings->bmp.bitmap_height);
    ui_bmp_text_button_set_caption(settings, text_view_cstr(msg_str(MSG_HOTKEY_GUIDE_SETTINGS)));
    ui_window_move_base(&settings->bmp.button.base, 0x2fb, 0x1ce);
    settings->bmp.button.base.id = HOTKEY_GUIDE_ID_SETTINGS;
    ui_window_add_child(wnd, &settings->bmp.button.base);
}

static struct BitmapRes *guide_bitmap(const char *name)
{
    return (struct BitmapRes *)res_mgr_get(g_resMgr(), name);
}

static void draw_lit_box(struct UIHotkeyGuideWnd *self, const struct KeyCapBox *box)
{
    struct UIWindow *wnd = &self->frame.base;
    struct BitmapRes *right;
    int i;

    if (box->width == 0x24) {
        ui_window_draw_bitmap(wnd, box->x, box->y,
                            guide_bitmap(box->height == 0x4b ? UI_SKIN "hotkey_guide\\bt_enter2.bmp"
                                                             : UI_SKIN "hotkey_guide\\bt_keyboard.bmp"),
                            1);
        return;
    }
    ui_window_draw_bitmap(wnd, box->x, box->y, guide_bitmap(UI_SKIN "hotkey_guide\\bt_left.bmp"), 1);
    for (i = 1; i <= (box->width - 12) / 6; i++)
        ui_window_draw_bitmap(wnd, box->x + 6 * i, box->y, guide_bitmap(UI_SKIN "hotkey_guide\\bt_mid.bmp"), 1);
    if ((box->width - 12) % 6)
        ui_window_draw_bitmap(wnd, box->x + box->width - 12, box->y,
                            guide_bitmap(UI_SKIN "hotkey_guide\\bt_mid.bmp"), 1);
    right = guide_bitmap(UI_SKIN "hotkey_guide\\bt_right.bmp");
    ui_window_draw_bitmap(wnd, box->x + box->width - 6, box->y, right, 1);
}

static void draw_label(struct UIHotkeyGuideWnd *self, struct String label,
                                  const struct KeyCapBox *box)
{
    struct UIWindow *wnd = &self->frame.base;
    const char *text = string_cstr(&label);
    int x = box->x;

    if (box->align == 6)
        x = (int)((float)(box->width - ui_window_text_width(wnd, text, 0, 0, HOTKEY_GUIDE_FONT, 0, 0)) * 0.5f +
                  (float)x);
    else if (box->align == 0)
        x += box->text_inset;
    else if (box->align == 2)
        x += box->width - box->text_inset - ui_window_text_width(wnd, text, 0, 0, HOTKEY_GUIDE_FONT, 0, 0);
    ui_window_text_out(wnd, x, (int)((double)(box->height - 12) * 0.5 + (double)box->y), text, 0, 0,
                     HOTKEY_GUIDE_FONT, HOTKEY_GUIDE_TEXT, 0, 0);
    string_free(&label);
}

void ui_hotkey_guide_wnd_on_draw(struct UIWindow *window)
{
    struct UIHotkeyGuideWnd *self = (struct UIHotkeyGuideWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct TreeNode *node;
    struct String text, header;
    int *vk;
    size_t i;

    ui_window_draw_bitmap(wnd, 0, 0, guide_bitmap(UI_SKIN "hotkey_guide\\bg_shortcuts.bmp"), 0);
    ui_window_draw_title_bar(wnd, 0, msg_str(MSG_HOTKEY_GUIDE_TITLE), 0);

    for (vk = self->lit.first; vk != self->lit.last; vk++) {
        struct KeyCap *cap = key_cap_at(&self->caps, *vk);

        if (cap->boxes[0].shown == 1)
            draw_lit_box(self, &cap->boxes[0]);
        if (cap->boxes[1].shown == 1)
            draw_lit_box(self, &cap->boxes[1]);
    }

    for (node = tree_first(&self->caps); node != self->caps.head; node = tree_next(node)) {
        struct KeyCap *cap = &TREE_VALUE(node, struct KeyCapEntry)->cap;

        if (cap->boxes[0].shown == 1) {
            string_copy(&text, &cap->label);
            draw_label(self, text, &cap->boxes[0]);
        }
        if (cap->boxes[1].shown == 1) {
            string_copy(&text, &cap->label);
            draw_label(self, text, &cap->boxes[1]);
        }
    }

    for (i = 0; i < (size_t)((struct HotkeyGuideItem *)self->items.last - (struct HotkeyGuideItem *)self->items.first);
         i++) {
        const struct HotkeyGuideItem *item = (struct HotkeyGuideItem *)self->items.first + i;
        int column = (int)(i / HOTKEY_GUIDE_ROWS), width;

        string_init(&text);
        if (!item->vk) {
            string_assign_cstr(&text, "Non");
        } else {
            struct KeyCap *cap = key_cap_at(&self->caps, item->vk);

            string_assign(&text, string_cstr(&cap->label), cap->label.size);
            if (item->modifier) {
                struct String prefix, joined;

                cap = key_cap_at(&self->caps, item->modifier);
                string_init(&prefix);
                string_format(&prefix, "%s + ", string_cstr(&cap->label));
                string_init(&joined);
                string_append(&joined, string_cstr(&prefix), prefix.size);
                string_append(&joined, string_cstr(&text), text.size);
                string_free(&text);
                text = joined;
                string_free(&prefix);
            }
        }
        width = ui_window_text_width(wnd, string_cstr(&text), 0, 0, HOTKEY_GUIDE_FONT, 1, 0);
        ui_window_text_out(wnd, (int)((double)(100 - width) * 0.5 + (double)(column * HOTKEY_GUIDE_COLUMN + 0x9c)),
                         (int)(i % HOTKEY_GUIDE_ROWS) * HOTKEY_GUIDE_ROW + 0x13b, string_cstr(&text), 0, 0,
                         HOTKEY_GUIDE_FONT, HOTKEY_GUIDE_SHORTCUT, 1, 0);
        string_free(&text);
    }

    string_init(&header);
    string_format(&header, "%s / %s", msg_str(MSG_HOTKEY_GUIDE_KEY), msg_str(MSG_HOTKEY_GUIDE_CLOSE));
    ui_window_text_out_centred(wnd, 4, 0x20, string_cstr(&header), 0x4b, 0, HOTKEY_GUIDE_FONT,
                            HOTKEY_GUIDE_TEXT, 0);
    string_free(&header);
    ui_window_text_out_centred(wnd, 0xcf, 0x22, msg_str(MSG_HOTKEY_GUIDE_KEYBOARD), 0x9f, 0, 0xb,
                            HOTKEY_GUIDE_HEADER, 0);
    ui_window_text_out_centred(wnd, 0x25c, 0x20, msg_str(MSG_HOTKEY_GUIDE_WINDOWS), 0x4b, 0, HOTKEY_GUIDE_FONT,
                            HOTKEY_GUIDE_TEXT, 0);
    string_format(&header, "%s / %s", msg_str(MSG_HOTKEY_GUIDE_SETTINGS_A), msg_str(MSG_HOTKEY_GUIDE_SETTINGS_B));
    ui_window_text_out_centred(wnd, 0x317, 0x139, string_cstr(&header), 0x66, 0, HOTKEY_GUIDE_FONT,
                            HOTKEY_GUIDE_TEXT, 0);
    string_free(&header);
    ui_window_text_out_centred(wnd, 0x302, 0x15a, msg_str(MSG_HOTKEY_GUIDE_NOTE_A), 0x37, 0, HOTKEY_GUIDE_FONT,
                            HOTKEY_GUIDE_TEXT, 0);
    ui_window_text_out_centred(wnd, 0x35b, 0x15a, msg_str(MSG_HOTKEY_GUIDE_NOTE_B), 0x37, 0, HOTKEY_GUIDE_FONT,
                            HOTKEY_GUIDE_TEXT, 0);
}

void ui_hotkey_guide_wnd_on_process(struct UIWindow *window)
{
    struct UIHotkeyGuideWnd *self = (struct UIHotkeyGuideWnd *)window;
    struct UIWindow **button;

    for (button = self->buttons.first; button != self->buttons.last; button++) {
        int id;
        struct HotkeyGuideItem *item;

        if ((*button)->state != UI_BTN_DOWN)
            continue;
        id = (*button)->id;
        if (id == self->held)
            return;
        self->lit.last = self->lit.first;
        for (item = self->items.first; item != (struct HotkeyGuideItem *)self->items.last; item++)
            if (item->msg == id)
                break;
        if (item != (struct HotkeyGuideItem *)self->items.last && item->vk) {
            vec_push(&self->lit, &item->vk, sizeof item->vk);
            if (item->modifier)
                vec_push(&self->lit, &item->modifier, sizeof item->modifier);
        }
        self->held = id;
        ui_window_invalidate_base(&self->frame.base);
        return;
    }
    if (self->held != HOTKEY_GUIDE_NONE_LIT) {
        self->lit.last = self->lit.first;
        self->held = HOTKEY_GUIDE_NONE_LIT;
        ui_window_invalidate_base(&self->frame.base);
    }
}

int ui_hotkey_guide_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                        int val2, int val3, int val4)
{
    struct UIHotkeyGuideWnd *self = (struct UIHotkeyGuideWnd *)window;
    if (msg == UI_MSG_COMMAND && (val1 == UI_ID_CLOSE || val1 == HOTKEY_GUIDE_ID_SETTINGS)) {
        if (val1 == HOTKEY_GUIDE_ID_SETTINGS) {
            struct UIWindow *settings = ui_window_open(UI_WND_HOTKEY_SETTINGS);

            if (settings)
                ui_window_send_msg(settings, &self->frame.base, HOTKEY_SETTINGS_MSG_FROM_GUIDE, 1, 0, 0, 0);
        }
        ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_HOTKEY_GUIDE);
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
