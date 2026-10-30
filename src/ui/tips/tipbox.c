#include "ui/controls/listbox.h"
#include "ui/controls/edit.h"
#include "ui/controls/buttons.h"
#include "core/text_array.h"
#include "render/rp_pool.h"
#include "render/texture.h"
#include "render/texture_cache.h"
#include "resources/sprite.h"
#include "platform/client_config.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/tips/tips.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include "core/msg_str.h"
#include "diag/perf.h"
#include "ui/wndmgr/wndmgr.h"

#define ID_SEARCH 0x165
#define ID_PREV_PAGE 0xee
#define ID_NEXT_PAGE 0xd2
#define ID_RESULTS UI_ID_OK
#define ID_REFRESH 0x217

#define MSG_TIPBOX_TITLE 0xca6
#define MSG_SHOW_AT_STARTUP 0xf63
#define MSG_CLOSE 0x513
#define MSG_SEARCH_HINT 0xcb5

#define TIPBOX_DIR UI_SKIN "tipbox\\"
#define TIPBOX_ICON_PREFIX TIPBOX_DIR "ico_effect"

#define TIPBOX_TEXT_R 0x2c
#define TIPBOX_TEXT_G 0x4d
#define TIPBOX_TEXT_B 0xa0

#define TIPBOX_PAGE_WRAP 1000
#define TIPBOX_TITLE_WRAP 0x1e

#define TIPBOX_DOT_STEP_MS 300
#define TIPBOX_BLINK_OFF_MS 500
#define TIPBOX_BLINK_MS 1000
#define TIPBOX_DEFAULT_DOTS 5

static void tipbox_set_disabled(struct UIBitmapButton *button, BYTE disabled)
{
    button->button.disabled = disabled;
    ui_window_invalidate_base(&button->button.base);
    ui_window_invalidate_base(&button->button.base);
}

static struct UIBitmapButton *tipbox_add_arrow(struct UITipboxWnd *self, const char *on, const char *off, int x,
                                               int y, int id)
{
    void *mem = mem_alloc(sizeof(struct UIBitmapButton));
    struct UIBitmapButton *button = mem ? ui_bitmap_button_construct(mem) : NULL;

    ui_bitmap_button_set_bitmap(button, on, 0, 0);
    ui_bitmap_button_set_bitmap(button, on, 1, 0);
    ui_bitmap_button_set_bitmap(button, on, 2, 0);
    ui_bitmap_button_set_bitmap(button, off, UI_BMP_DISABLED, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    ui_window_add_child(&self->frame.base, &button->button.base);
    return button;
}

static void tipbox_set_bitmaps3(struct UIBitmapButton *button, const char *up, const char *hover, const char *down)
{
    ui_bitmap_button_set_bitmap(button, up, 0, 0);
    ui_bitmap_button_set_bitmap(button, hover, 1, 0);
    ui_bitmap_button_set_bitmap(button, down, 2, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
}

static void tipbox_cut_at(struct String *s, const char *needle)
{
    int pos = string_find(s, needle, 0);

    if (pos != -1) {
        struct String head;

        string_init(&head);
        string_substring(&head, s, 0, pos);
        if (s != &head) {
            string_free(s);
            *s = head;
            string_init(&head);
        }
        string_free(&head);
    }
}

static void tipbox_count_text(struct String *out, const struct TipboxView *mgr)
{
    string_init(out);
    string_format(out, "/%d", (int)mgr->tip.page_count);
}

static void ui_tipbox_wnd_show_page_number(struct UITipboxWnd *self)
{
    const struct TipboxView *mgr = tipbox_view();
    struct String num, count;
    int num_width, count_width, left;

    string_init(&num);
    string_format(&num, "%d", mgr->tip.page_count ? mgr->page + 1 : 0);
    ui_static_text_set_text(self->page_no, string_cstr(&num), 0);
    string_free(&num);
    ui_static_text_set_font(self->page_no, 0, 0xc, 1, 0);
    tipbox_count_text(&count, mgr);
    ui_static_text_set_text(self->page_count, string_cstr(&count), 0);
    string_free(&count);

    string_format(&num, "%d", mgr->tip.page_count ? mgr->page + 1 : 0);
    num_width = ui_window_get_color_text_width(&self->frame.base, string_cstr(&num),
                                           (int)strlen(string_cstr(&num)), 0, 0xc, 1);
    string_free(&num);
    tipbox_count_text(&count, mgr);
    count_width = ui_window_get_color_text_width(&self->frame.base, string_cstr(&count),
                                             (int)strlen(string_cstr(&count)), 0, 0xc, 0);
    string_free(&count);

    left = (0x24 - count_width - num_width) / 2;
    ui_window_move(self->page_no, left + 0x1aa, 0x140);
    ui_window_move(self->page_count, num_width + 0x1aa + left, 0x140);
}

static BYTE ui_tipbox_wnd_build_markers(struct UITipboxWnd *self)
{
    const struct TipboxView *mgr = tipbox_view();
    unsigned i;

    self->icons.last = self->icons.first;
    self->dots.last = self->dots.first;
    if (!(mgr->tip.effect_count != 0))
        return 0;

    for (i = 0; i < mgr->tip.effect_count; i++) {
        const struct TipEffect *e = &mgr->tip.effects[i];

        if (e->kind == 6) {
            float dx = (float)e->x1 - (float)e->x0, dy = (float)e->y1 - (float)e->y0;
            int n = TIPBOX_DEFAULT_DOTS, k;

            if (e->duration != 0.0f)
                n = (int)e->duration;
            for (k = 0; k < n; k++) {
                const struct TipEffect *cur = &mgr->tip.effects[i];
                struct TipboxDot dot;

                dot.x = (float)k * dx / (float)n + (float)cur->x0;
                dot.y = (float)k * dy / (float)n + (float)cur->y0;
                dot.visible = 0;
                vec_push(&self->dots, &dot, sizeof(struct TipboxDot));
            }
        } else {
            struct TipboxIcon icon;

            icon.number = e->kind;
            icon.x = (float)e->x0;
            icon.y = (float)e->y0;
            icon.start = timeGetTime();
            icon.blinks = e->blinks;
            icon.blink_time = timeGetTime();
            icon.visible = 1;
            e = &mgr->tip.effects[i];
            icon.from_x = e->x0;
            icon.from_y = e->y0;
            icon.to_x = e->x1;
            icon.to_y = e->y1;
            icon.duration = e->duration;
            vec_push(&self->icons, &icon, sizeof(struct TipboxIcon));
        }
    }
    self->anim_time = timeGetTime();
    return 1;
}

static void ui_tipbox_wnd_refresh(struct UITipboxWnd *self)
{
    struct String title;
    struct TextArray lines = {0};
    unsigned count, i, shown;

    ui_tipbox_wnd_show_page_number(self);
    ui_tipbox_wnd_build_markers(self);
    ui_window_set_show_base(self->results, 0);

    string_init(&title);
    string_assign_cstr(&title, tipbox_view()->tip.title);
    tipbox_cut_at(&title, "#");
    text_split_lines(string_cstr(&title), &lines, TIPBOX_TITLE_WRAP);
    for (i = 0; i < 2; i++) {
        ui_static_text_set_text(self->titles[i], "", 0);
        ui_window_set_show(self->titles[i], 0);
    }
    count = lines.size;
    if (count) {
        shown = count > 1 ? 2 : 1;
        for (i = 0; i < shown; i++) {
            ui_static_text_set_text(self->titles[i],
                                 text_array_at(&lines, i).data, 0);
            ui_window_set_show(self->titles[i], 1);
        }
        if (count <= 1) {
            ui_window_move(self->text, 0xfa, 0x46);
            ui_window_resize(self->text, 0xe2, 0xf5);
        } else {
            ui_window_move(self->text, 0xfa, 0x5a);
            ui_window_resize(self->text, 0xe2, 0xe1);
        }
        self->page_dirty = 1;
    }
    text_array_clear(&lines);
    string_free(&title);
}

static void ui_tipbox_wnd_search(struct UITipboxWnd *self, const struct String *words)
{
    size_t count = tipbox_search(string_cstr(words)), index;

    ui_list_box_ex_clear((struct UIListBoxEx *)self->results);
    for (index = 0; index < count; ++index) {
        int id = tipbox_search_id(index);
        struct String title;

        string_init(&title);
        string_assign_cstr(&title, tipbox_title(id));
        tipbox_cut_at(&title, "#");
        if (title.size)
            ui_list_box_id_add_item((struct UIListBoxID *)self->results, string_cstr(&title), id);
        string_free(&title);
    }
    ui_window_invalidate_base(self->results);
}

void ui_tipbox_wnd_animate_icons(struct UITipboxWnd *self)
{
    unsigned i;

    for (i = 0; i < VEC_COUNT(&self->icons, struct TipboxIcon); i++) {
        struct TipboxIcon *icon = &VEC_AT(&self->icons, struct TipboxIcon, i);

        if (icon->blinks && self->now - icon->blink_time >= TIPBOX_BLINK_OFF_MS) {
            icon->visible = 1;
            if (self->now - icon->blink_time >= TIPBOX_BLINK_MS) {
                icon->visible = 0;
                icon->blink_time = self->now;
            }
        }
    }
    for (i = 0; i < VEC_COUNT(&self->icons, struct TipboxIcon); i++) {
        struct TipboxIcon *icon = &VEC_AT(&self->icons, struct TipboxIcon, i);
        float step, age;

        if (icon->from_x == icon->to_x && icon->from_y == icon->to_y)
            continue;
        step = (float)(double)(DWORD)(timeGetTime() - self->now) * 0.001f;
        age = (float)(double)(DWORD)(self->now - icon->start);
        if (age >= icon->duration * 1000.0f) {
            icon->x = (float)icon->from_x;
            icon->y = (float)icon->from_y;
            icon->start = self->now;
        } else {
            icon->x = ((float)icon->to_x - (float)icon->from_x) * step / icon->duration + icon->x;
            icon->y = ((float)icon->to_y - (float)icon->from_y) * step / icon->duration + icon->y;
        }
    }
}

static void ui_tipbox_wnd_draw_markers(struct UITipboxWnd *self)
{
    struct String path;
    unsigned i;

    string_init(&path);
    for (i = 0; i < VEC_COUNT(&self->icons, struct TipboxIcon); i++) {
        struct String num;
        struct TipboxIcon *icon;

        string_init(&num);
        string_format(&num, "%d", VEC_AT(&self->icons, struct TipboxIcon, i).number);
        string_insert(&num, 0, TIPBOX_ICON_PREFIX, sizeof(TIPBOX_ICON_PREFIX) - 1);
        string_append_cstr(&num, ".bmp");
        string_free(&path);
        path = num;
        icon = &VEC_AT(&self->icons, struct TipboxIcon, i);
        if (icon->visible == 1)
            ui_window_draw_bitmap(&self->frame.base, (int)icon->x, (int)icon->y,
                                (struct BitmapRes *)res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
    }
    for (i = 0; i < VEC_COUNT(&self->dots, struct TipboxDot); i++) {
        struct TipboxDot *dot = &VEC_AT(&self->dots, struct TipboxDot, i);

        if (dot->visible == 1)
            ui_window_draw_bitmap(&self->frame.base, (int)dot->x, (int)dot->y,
                                ui_skin_bitmap(TIPBOX_DIR "ico_effect6.bmp"), 1);
    }
    string_free(&path);
}

void tips_save_show_at_startup(const int *value)
{
    HKEY key;
    DWORD disposition = 0;

    if (RegCreateKeyExA(HKEY_CURRENT_USER, g_registryKeyPath, 0, NULL, 0, 0xf003f, NULL, &key, &disposition) != 0)
        return;
    RegSetValueExA(key, "SHOWTIPSATSTARTUP", 0, REG_DWORD, (const BYTE *)value, 4);
    RegCloseKey(key);
}

struct UIWindow;
void ui_tipbox_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_tipbox_wnd_on_process(struct UIWindow *self);
void ui_tipbox_wnd_on_draw(struct UIWindow *self);
int ui_tipbox_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_tipbox_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps ui_tipbox_wnd_ops = {
    .destroy = ui_tipbox_wnd_destroy,
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
    .on_create = ui_tipbox_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_tipbox_wnd_on_process,
    .on_draw = ui_tipbox_wnd_on_draw,
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
    .send_msg = ui_tipbox_wnd_send_msg,
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

struct UITipboxWnd *ui_tipbox_wnd_construct(struct UITipboxWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_tipbox_wnd_ops;
    self->search_edit = NULL;
    self->results = NULL;
    self->text = NULL;
    self->prev = NULL;
    self->next = NULL;
    self->icons.first = self->icons.last = self->icons.end = NULL;
    self->dots.first = self->dots.last = self->dots.end = NULL;
    self->anim_time = 0;
    self->now = 0;
    self->page_dirty = 0;
    self->show_at_startup = 0;
    self->titles[0] = NULL;
    self->titles[1] = NULL;
    return self;
}

struct UIWindow *ui_tipbox_wnd_new(void)
{
    return (struct UIWindow *)ui_tipbox_wnd_construct(mem_alloc(sizeof(struct UITipboxWnd)));
}

void ui_tipbox_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UITipboxWnd *self = (struct UITipboxWnd *)window;
    const struct TipboxTip *tip;
    struct String slash, count, text;
    struct UIBitmapButton *button;
    struct UIBmpTextButton *close;
    struct UIWindow *check, *edit, *list, *x_button;
    DWORD colour;
    void *mem;
    int i;

    self->show_at_startup = g_showTipsAtStartup;
    self->frame.default_id = ID_SEARCH;
    colour = ui_skin_argb(2, 2);

    mem = mem_alloc(sizeof(struct UITextViewer));
    self->text = mem ? ui_text_viewer_construct(mem) : NULL;
    ui_list_box_create_window((struct UIListBox *)self->text, 0xfa, 0x46, 0xe2, 0xf5, 0);
    UI_TEXT_VIEWER_BACKGROUND(self->text) = colour;
    ui_window_invalidate(self->text);
    ((struct UIListBox *)self->text)->selection_colour = 0x38302e;
    ((struct UITextViewer *)self->text)->split_tipbox_links = 1;
    ui_window_add_child(&self->frame.base, self->text);
    ui_window_set_show(self->text, 1);

    for (i = 0; i < 2; i++) {
        mem = mem_alloc(sizeof(struct UIStaticText));
        self->titles[i] = mem ? ui_static_text_construct(mem) : NULL;
        ui_window_create(self->titles[i], 0xdc, 0x12);
        ui_static_text_set_text(self->titles[i], "", 0);
        ui_static_text_set_font(self->titles[i], 0, 0xd, 1, 0);
        ui_static_text_set_text_colour(self->titles[i], TIPBOX_TEXT_R, TIPBOX_TEXT_G, TIPBOX_TEXT_B);
        ui_window_move(self->titles[i], 0xfe, 0x32 + i * 0x14);
        ui_window_add_child(&self->frame.base, self->titles[i]);
    }

    tip = tipbox_pick_random();

    mem = mem_alloc(sizeof(struct UIStaticText));
    self->page_no = mem ? ui_static_text_construct(mem) : NULL;
    ui_window_create(self->page_no, 0x96, 0x14);
    ui_static_text_set_text(self->page_no, "1", 0);
    ui_static_text_set_font(self->page_no, 0, 0xc, 1, 0);
    ui_static_text_set_text_colour(self->page_no, TIPBOX_TEXT_R, TIPBOX_TEXT_G, TIPBOX_TEXT_B);
    ui_window_add_child(&self->frame.base, self->page_no);

    string_init(&slash);
    string_assign_cstr(&slash, "/");
    mem = mem_alloc(sizeof(struct UIStaticText));
    self->page_count = mem ? ui_static_text_construct(mem) : NULL;
    ui_window_create(self->page_count, 0x5a, 0xf);
    string_init(&count);
    string_format(&count, "%d", (int)tip->page_count);
    string_insert(&count, 0, string_cstr(&slash), slash.size);
    string_move(&text, &count);
    ui_static_text_set_text(self->page_count, string_cstr(&text), 0);
    string_free(&text);
    string_free(&count);
    ui_static_text_set_font(self->page_count, 0, 0xc, 0, 0);
    ui_static_text_set_text_colour(self->page_count, TIPBOX_TEXT_R, TIPBOX_TEXT_G, TIPBOX_TEXT_B);
    ui_window_add_child(&self->frame.base, self->page_count);

    mem = mem_alloc(sizeof(struct UIBitmapButton));
    button = mem ? ui_bitmap_button_construct(mem) : NULL;
    tipbox_set_bitmaps3(button, TIPBOX_DIR "btn_searchbar_normal.bmp", TIPBOX_DIR "btn_searchbar_over.bmp",
                        TIPBOX_DIR "btn_searchbar_press.bmp");
    ui_window_move_base(&button->button.base, 0x1c0, 0x15);
    button->button.base.id = ID_SEARCH;
    ui_window_add_child(&self->frame.base, &button->button.base);

    mem = mem_alloc(sizeof(struct UICheckBox));
    check = mem ? (struct UIWindow *)ui_check_box_construct(mem) : NULL;
    ui_window_create(check,
                     ui_window_text_width(&self->frame.base, msg_str(MSG_SHOW_AT_STARTUP), 0, 0, 0xc, 0, 0) + 0xc,
                     0xe);
    ui_check_box_set_bitmaps(check, TIPBOX_DIR "checkBox_check.bmp", TIPBOX_DIR "checkBox.bmp",
                          msg_str(MSG_SHOW_AT_STARTUP));
    ui_window_move_base(check, 9, 0x15b);
    ui_check_box_set_check(check, self->show_at_startup);
    check->id = UI_ID_CHECKBOX;
    ui_window_add_child(&self->frame.base, check);

    mem = mem_alloc(sizeof(struct UIBmpTextButton));
    close = mem ? ui_bmp_text_button_construct(mem) : NULL;
    tipbox_set_bitmaps3(&close->bmp, TIPBOX_DIR "btn_close_normal.bmp", TIPBOX_DIR "btn_close_over.bmp",
                        TIPBOX_DIR "btn_close_press.bmp");
    ui_bmp_text_button_set_caption(close, text_view_cstr(msg_str(MSG_CLOSE)));
    ui_window_move_base(&close->bmp.button.base, 0x19f, 0x155);
    close->bmp.button.base.id = UI_ID_CLOSE;
    ui_window_add_child(&self->frame.base, &close->bmp.button.base);

    mem = mem_alloc(sizeof(struct UIEditCtrl4));
    edit = mem ? (struct UIWindow *)ui_edit_ctrl4_construct(mem) : NULL;
    self->search_edit = edit;
    ui_window_create(edit, 0xc1, 0xf);
    UI_EDIT_MAX_CHARS(edit) = 0x96;
    ui_edit_ctrl_set_text_colour(edit, 0, 0, 0, -1);
    ((struct UIEditCtrl4 *)edit)->use_alt = 1;
    ui_edit_ctrl4_set_empty_text(edit, msg_str(MSG_SEARCH_HINT));
    ((struct UIEditCtrl4 *)edit)->alt_colour = 0xb1b1b1;
    ui_window_move_base(edit, 0xfd, 0x16);
    ui_edit_ctrl_set_back_colour(edit, 0xff, 0xff, 0xff);
    ui_window_add_child(&self->frame.base, edit);

    self->prev = tipbox_add_arrow(self, TIPBOX_DIR "bt_arrowL_on.bmp", TIPBOX_DIR "bt_arrowL_off.bmp", 0x19f, 0x13f,
                                  ID_PREV_PAGE);
    self->next = tipbox_add_arrow(self, TIPBOX_DIR "bt_arrowR_on.bmp", TIPBOX_DIR "bt_arrowR_off.bmp", 0x1ce, 0x13f,
                                  ID_NEXT_PAGE);

    mem = mem_alloc(sizeof(struct UIListBoxID));
    list = mem ? (struct UIWindow *)ui_list_box_id_construct(mem) : NULL;
    self->results = list;
    ui_list_box_ex_create((struct UIListBoxEx *)list, 0xfa, 0x32, 0xd8, 0x10a, 0);
    ((struct UIListBoxEx *)list)->r = 0xf0;
    ((struct UIListBoxEx *)list)->g = 0xf0;
    ((struct UIListBoxEx *)list)->b = 0xf0;
    list->id = ID_RESULTS;
    ui_window_add_child(&self->frame.base, list);
    ui_window_set_show_base(list, 0);

    x_button = ui_window_add_close_button(&self->frame.base, "", 0);
    ui_window_move_base(x_button, width - 0xf, 3);

    ui_tipbox_wnd_refresh(self);
    if (tip->page_count <= 1)
        tipbox_set_disabled(self->next, 1);
    tipbox_set_disabled(self->prev, 1);
    string_free(&slash);
}

void ui_tipbox_wnd_on_process(struct UIWindow *window)
{
    struct UITipboxWnd *self = (struct UITipboxWnd *)window;
    if (self->page_dirty == 1) {
        struct String page;
        struct TextArray lines = {0};
        unsigned i;

        self->page_dirty = 0;
        string_init(&page);
        string_assign_cstr(&page, tipbox_page_text());
        text_split_lines(string_cstr(&page), &lines, TIPBOX_PAGE_WRAP);
        ui_list_box_clear((struct UIListBox *)self->text);
        for (i = 0; i < lines.size; i++)
            ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT,
                     (int)text_array_at(&lines, i).data, 0x20, 0, 0);
        ui_window_invalidate_base(&self->frame.base);
        text_array_clear(&lines);
        string_free(&page);
    }
    if (tipbox_view()->tip.effect_count != 0)
        ui_window_invalidate_base(&self->frame.base);

    if (self->now - self->anim_time >= TIPBOX_DOT_STEP_MS) {
        unsigned i, j;

        self->anim_time = timeGetTime();
        for (i = 0; i < VEC_COUNT(&self->dots, struct TipboxDot); i++) {
            struct TipboxDot *dot = &VEC_AT(&self->dots, struct TipboxDot, i);

            if (!dot->visible) {
                dot->visible = 1;
                break;
            }
            if (VEC_COUNT(&self->dots, struct TipboxDot) - 1 == i)
                for (j = 0; j < VEC_COUNT(&self->dots, struct TipboxDot); j++)
                    VEC_AT(&self->dots, struct TipboxDot, j).visible = 0;
        }
    }
    ui_tipbox_wnd_animate_icons(self);
    self->now = timeGetTime();
}

void ui_tipbox_wnd_on_draw(struct UIWindow *window)
{
    struct UITipboxWnd *self = (struct UITipboxWnd *)window;
    const struct TipboxView *mgr;

    ui_window_clear_canvas(&self->frame.base, UI_TRANSPARENT);
    ui_window_draw_title_bar(&self->frame.base, 1, msg_str(MSG_TIPBOX_TITLE), 0);
    ui_window_draw_bitmap(&self->frame.base, 0, 0x11, ui_skin_bitmap(TIPBOX_DIR "TipboxBG.bmp"), 0);

    if (self->results->visible == 1) {
        ui_window_draw_bitmap(&self->frame.base, 1, 0x11,
                            ui_skin_bitmap(tipbox_search_count() != 0 ? TIPBOX_DIR "img_loading.bmp"
                                                                               : TIPBOX_DIR "img_NoSearchResults.bmp"),
                            0);
    } else {
        mgr = tipbox_view();
        switch (mgr->picture) {
        case TIP_PICTURE_TEXTURE:
            if (mgr->tip.image_kind == 5 && mgr->tip.image_arg == 100) {
                struct String path;

                string_init(&path);
                string_assign_cstr(&path, mgr->sprite);
                ui_window_draw_bitmap(&self->frame.base, 1, 0x11, ui_skin_bitmap(string_cstr(&path)), 0);
                string_free(&path);
            } else {
                struct Texture *tex = texture_cache_get(&g_textureCache, mgr->sprite, 0, 0);
                struct DcBitmap *canvas = self->frame.base.canvas;

                if (tex) dc_bitmap_draw_texels_scaled(canvas, (int)mgr->left, (int)mgr->top, (int)mgr->right,
                                         (int)mgr->bottom, tex, tex->reqWidth, tex->reqHeight, mgr->scale, mgr->scale, -1,
                                         0, 0, 0, mgr->tex_arg1, mgr->tex_arg2);
            }
            break;
        case TIP_PICTURE_SPRITE: {
            struct String name;
            struct ActRes *act;
            struct SprRes *spr;
            struct SprClip adjusted, *clip = &adjusted;

            string_init(&name);
            string_assign_cstr(&name, mgr->sprite);
            string_append_cstr(&name, ".act");
            act = res_mgr_get_string(g_resMgr(), name);
            string_init(&name);
            string_assign_cstr(&name, mgr->sprite);
            string_append_cstr(&name, ".spr");
            spr = res_mgr_get_string(g_resMgr(), name);
            if (!act || !spr)
                return;
            adjusted = *motion_get_clip(act_res_get_motion(act, 0, 0), 0);
            clip->x = 0;
            clip->y = 0;
            clip->zoomX = 1.0f;
            clip->zoomY = 1.0f;
            ui_window_draw_spr_clip(&self->frame.base, (int)mgr->left, (int)mgr->top,
                                 sprite_resource_image(spr, 0, 0), clip, 1.0f, 0, -1, spr->palette, mgr->scale);
            break;
        }
        case TIP_PICTURE_NONE:
            ui_window_draw_bitmap(&self->frame.base, 1, 0x11, ui_skin_bitmap(TIPBOX_DIR "img_noImage.bmp"), 0);
            break;
        }
    }
    ui_tipbox_wnd_draw_markers(self);
    ui_window_draw_bitmap(&self->frame.base, 0xf1, 0x11, ui_skin_bitmap(TIPBOX_DIR "bg_tipbox.bmp"), 0);
    ui_window_draw_bitmap(&self->frame.base, 0, 0x151, ui_skin_bitmap(TIPBOX_DIR "btnbar_bottom.bmp"), 0);
}

int ui_tipbox_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                   int val3, int val4)
{
    struct UITipboxWnd *self = (struct UITipboxWnd *)window;
    const struct TipboxView *mgr;
    int old;

    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);

    switch (val1) {
    case UI_ID_CHECKBOX:
        self->show_at_startup = val2;
        g_showTipsAtStartup = val2;
        tips_save_show_at_startup(&self->show_at_startup);
        break;
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, TIPBOX_WND_ID);
        break;
    case ID_RESULTS: {
        struct UIListBoxEx *list = (struct UIListBoxEx *)self->results;

        ui_button_show((struct UIButton *)(self->prev), 1);
        ui_button_show((struct UIButton *)(self->next), 1);
        tipbox_show(ui_list_box_id_row_at(&list->rows, &list->selected)->id);
    }
    case ID_REFRESH:
        tipbox_set_disabled(self->next, tipbox_on_last_page() == 1);
        tipbox_set_disabled(self->prev, 1);
        ui_tipbox_wnd_refresh(self);
        ui_tipbox_wnd_build_markers(self);
        break;
    case ID_NEXT_PAGE:
        mgr = tipbox_view();
        tipbox_set_page(mgr->page + 1);
        if (tipbox_on_last_page() == 1)
            tipbox_set_disabled(self->next, 1);
        tipbox_set_disabled(self->prev, 0);
        ui_tipbox_wnd_refresh(self);
        break;
    case ID_PREV_PAGE:
        mgr = tipbox_view();
        old = mgr->page;
        tipbox_set_page(old - 1);
        tipbox_set_disabled(self->next, 0);
        if (old == 1)
            tipbox_set_disabled(self->prev, 1);
        ui_tipbox_wnd_refresh(self);
        break;
    case ID_SEARCH: {
        struct String words;

        string_init(&words);
        string_assign_cstr(&words, ui_edit_ctrl_get_text(self->search_edit));
        if (words.size) {
            int i;

            ui_window_set_show_base(self->results, 1);
            ui_list_box_ex_clear((struct UIListBoxEx *)self->results);
            ui_list_box_clear((struct UIListBox *)self->text);
            for (i = 0; i < 2; i++)
                ui_static_text_set_text(self->titles[i], "", 0);
            ui_static_text_set_text(self->page_no, "", 0);
            ui_static_text_set_text(self->page_count, "", 0);
            ui_button_show((struct UIButton *)(self->prev), 0);
            ui_button_show((struct UIButton *)(self->next), 0);
            ui_edit_ctrl_set_text_default((struct UIEditCtrl *)self->search_edit, "");
            self->icons.last = self->icons.first;
            self->dots.last = self->dots.first;
            ui_tipbox_wnd_search(self, &words);
        }
        string_free(&words);
        break;
    }
    }
    return 0;
}

void ui_tipbox_wnd_show_tip(struct UITipboxWnd *self, int id)
{
    tipbox_show(id);
    ui_tipbox_wnd_send_msg(&self->frame.base, &self->frame.base, UI_MSG_COMMAND, ID_REFRESH, 0, 0, 0);
}

static void tipbox_destruct(struct UITipboxWnd *self)
{
    self->frame.base.ops = &ui_tipbox_wnd_ops;
    vec_free(&self->dots);
    vec_free(&self->icons);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_tipbox_wnd_destroy(struct UIWindow *window)
{
    struct UITipboxWnd *self = (struct UITipboxWnd *)window;
    tipbox_destruct(self);
    mem_free(self);
}
