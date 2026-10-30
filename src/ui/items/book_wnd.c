#include "ui/controls/listbox.h"
#include "ui/window_access.h"
#include "ui/items/book_wnd.h"
#include "platform/code_page.h"
#include "platform/language.h"
#include "render/rp_pool.h"
#include "session/character_name.h"
#include "core/hex.h"
#include "platform/client_config.h"
#include "core/object_type_tables.h"
#include <string.h>
#include <stdlib.h>
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/controls/controls.h"
#include <mmsystem.h>
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/ui_items.h"
#include "core/msg_str.h"
#include "diag/perf.h"

const char *g_bookmarkSubkey = "\\BookMark";

#define BOOK_LINES_PER_PAGE 0x13

#define ID_BOOKMARK 0x15c
#define ID_PREV 0x15a
#define ID_NEXT 0x15b

#define BOOK_MSG_PREV 0x14
#define BOOK_MSG_NEXT 0x15
#define BOOK_MSG_GOTO 0x5c
#define BOOK_MSG_SPEAK 0x5d
#define BOOK_MSG_OPEN 0x5e
#define BOOK_MSG_CLOSE 0x72
#define BOOK_OPEN_TO_READ 0x117

#define MSG_BOOK_TIPS 0x510

static void book_bookmark_key(char *key)
{
    strcpy(key, g_registryKeyPath);
    strcat(key, g_bookmarkSubkey);
    strcat(key, "\\");
    strcat(key, string_cstr(&g_session1.serverName));
    strcat(key, "\\");
    strcat(key, session_character_name(&g_session1));
}

static int book_load_bookmark(struct UIBookWnd *self, const char *title)
{
    char key[0x200];
    HKEY hkey;
    DWORD type, size;
    int page;

    (void)self;
    book_bookmark_key(key);
    page = 1;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, key, 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
        return 1;
    RegCloseKey(hkey);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, key, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {
        type = REG_DWORD;
        size = sizeof(page);
        if (RegQueryValueExA(hkey, title, NULL, &type, (BYTE *)&page, &size) != ERROR_SUCCESS)
            page = 1;
        RegCloseKey(hkey);
    }
    return page;
}

static void book_save_bookmark(struct UIBookWnd *self)
{
    char key[0x200];
    HKEY hkey;
    DWORD disposition = 0;
    int page = self->page;

    book_bookmark_key(key);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, key, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, &disposition) !=
        ERROR_SUCCESS)
        return;
    RegSetValueExA(hkey, self->title, 0, REG_DWORD, (const BYTE *)&page, sizeof(page));
    RegCloseKey(hkey);
}

void ui_book_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_book_wnd_on_draw(struct UIWindow *self);
void ui_book_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_book_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_book_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps ui_book_wnd_ops = {
    .destroy = ui_book_wnd_destroy,
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
    .on_create = ui_book_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_book_wnd_on_process,
    .on_draw = ui_book_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_book_wnd_on_l_btn_down,
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
    .send_msg = ui_book_wnd_send_msg,
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

struct UIBookWnd *ui_book_wnd_construct(struct UIBookWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_book_wnd_ops;
    text_array_init(&self->lines);
    self->line_count = 0;
    self->page_count = 0;
    self->page = 1;
    self->speak_time = 0;
    self->speak_line = -1;
    self->speaking = 0;
    self->balloon = NULL;
    self->title[0] = 0;
    return self;
}

struct UIWindow *ui_book_wnd_new(void)
{
    return (struct UIWindow *)ui_book_wnd_construct(mem_alloc(sizeof(struct UIBookWnd)));
}

void ui_book_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIBookWnd *self = (struct UIBookWnd *)window;
    static const char *const names[4] = {
        "책갈피", "책왼쪽", "책오른쪽",
        "책닫기",
    };
    static const int ids[4] = { ID_BOOKMARK, ID_PREV, ID_NEXT, UI_ID_CLOSE };
    struct UIWindow *wnd = &self->frame.base;
    struct String dir;
    int pos[4][2];
    const char *tips[4];
    void *mem;
    int i;

    mem = mem_alloc(sizeof(struct UITextViewer));
    self->text = mem ? ui_text_viewer_construct(mem) : NULL;
    ui_list_box_create_window((struct UIListBox *)self->text, 0x1e, 0x2d, width - 0x3c, height - 0x3c, 0);
    ui_window_add_child(wnd, self->text);

    string_init(&dir);
    string_assign_cstr(&dir, "book\\");
    pos[0][0] = 0xa;
    pos[0][1] = 0x19;
    pos[1][0] = width - 0x39;
    pos[1][1] = 0x19;
    pos[2][0] = width - 0x23;
    pos[2][1] = 0x19;
    pos[3][0] = width - 0x19;
    pos[3][1] = height - 0x19;
    for (i = 0; i < 4; i++)
        tips[i] = msg_str(MSG_BOOK_TIPS + i);
    for (i = 0; i < 4; i++) {
        struct String path, part;
        struct UIWindow *button;

        mem = mem_alloc(sizeof(struct UISpriteButton));
        self->buttons[i] = button = mem ? ui_sprite_button_construct(mem) : NULL;
        string_copy(&part, &dir);
        string_append_cstr(&part, names[i]);
        string_append_cstr(&part, ".spr");
        path = part;
        ui_sprite_button_set_sprite(self->buttons[i], string_cstr(&path));
        string_free(&path);
        string_copy(&part, &dir);
        string_append_cstr(&part, names[i]);
        string_append_cstr(&part, ".act");
        path = part;
        ui_sprite_button_set_act(self->buttons[i], string_cstr(&path));
        string_free(&path);
        button = self->buttons[i];
        ui_window_create(button, ui_sprite_button_frame_width(button, 0, 0),
                         ui_sprite_button_frame_height(self->buttons[i], 0, 0));
        ui_window_move(self->buttons[i], pos[i][0], pos[i][1]);
        self->buttons[i]->id = ids[i];
        ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)self->buttons[i], tips[i]);
        ui_window_add_child(wnd, self->buttons[i]);
    }
    string_free(&dir);
}

void ui_book_wnd_on_draw(struct UIWindow *window)
{
    struct UIBookWnd *self = (struct UIBookWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int top = ui_window_draw_title_bar(wnd, 0, "", 0);
    char title[0x6e];
    COLORREF colour;

    ui_window_draw_body(wnd, top, 0, "");
    snprintf(title, sizeof title, "%s ( %d / %d )", self->title, self->page, self->page_count);
    ui_window_text_out(wnd, wnd->width / 2 - 0x48, 2, title, 0, 0, 0xc, 0, 0, 0);
    colour = ((BYTE)self->blue << 16 | (BYTE)self->green << 8) | (BYTE)self->red;
    ui_window_fill_rect(wnd, 1, top, wnd->width - 2, wnd->height - 0x12, colour);
    UI_TEXT_VIEWER_BACKGROUND(self->text) = 0xff000000 | colour;
    ui_window_invalidate(self->text);
}

void ui_book_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIBookWnd *self = (struct UIBookWnd *)window;
    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
}

#define HEX(c) hex_digit_value((unsigned char)(c))

static void book_open(struct UIBookWnd *self, const char *id_text, int mode, int page)
{
    struct UIWindow *wnd = &self->frame.base;
    const char *name = item_table_get(atoi(id_text))->name;
    char path[0x100];
    struct File file;
    const char *file_name;
    char *text;
    int size, count;

    if (strcmp(self->title, name) == 0) {
        if (!self->speaking && mode == BOOK_OPEN_TO_READ)
            return;
        page = self->page;
    }
    strcpy(self->title, name);
    if (g_serviceType == 0xa)
        sprintf(path, "%sbook\\%s.txt", g_language->folder,
                    id_text);
    else
        sprintf(path, "book\\%s.txt", id_text);
    file_construct(&file);
    file_name = path;
    if (g_language)
        file_name = language_language_file(g_language, path);
    if (!file_open(&file, file_name, 0, 1, 0)) {
        file_destruct(&file);
        return;
    }
    size = file.size;
    text = size >= 0 ? malloc((size_t)size + 1) : NULL;
    if (!text) {
        file_close(&file);
        file_destruct(&file);
        return;
    }
    if (!file_read(&file, text, size)) {
        free(text);
        file_close(&file);
        file_destruct(&file);
        return;
    }
    text[size] = 0;
    file_close(&file);
    text_array_clear(&self->lines);
    ui_window_break_lines(wnd, text, &self->lines, self->text->width - 10);
    count = (int)self->lines.size;
    self->line_count = count;
    if (size >= 7 && text[0] == '%') {
        self->red = HEX(text[1]) * 16 + HEX(text[2]);
        self->green = HEX(text[3]) * 16 + HEX(text[4]);
        self->blue = HEX(text[5]) * 16 + HEX(text[6]);
        if (count > 0) {
            text_array_erase(&self->lines, 0);
            self->line_count = count - 1;
        }
    } else {
        ui_skin_color_chip(&g_skinMgr, 2, 2, &self->red, &self->green, &self->blue);
    }
    free(text);
    self->page_count = self->line_count / BOOK_LINES_PER_PAGE;
    if (self->line_count % BOOK_LINES_PER_PAGE > 0)
        self->page_count++;
    file_destruct(&file);
    ui_book_wnd_send_msg(wnd, wnd, mode == BOOK_OPEN_TO_READ ? BOOK_MSG_GOTO : BOOK_MSG_SPEAK, page, 0, 0, 0);
}

static void book_goto(struct UIBookWnd *self, int page)
{
    struct UIWindow *wnd = &self->frame.base;
    int i;

    self->speaking = 0;
    ui_window_set_show_base(wnd, 1);
    if (self->balloon) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->balloon);
        self->balloon = NULL;
    }
    if (page)
        self->page = page;
    else
        self->page = page = book_load_bookmark(self, self->title);
    if (page > self->page_count)
        self->page = 0;
    ui_list_box_clear((struct UIListBox *)self->text);
    for (i = 0; i < BOOK_LINES_PER_PAGE; i++) {
        int line = self->page * BOOK_LINES_PER_PAGE - BOOK_LINES_PER_PAGE + i;
        const char *text;

        if (line < 0 || line >= self->line_count)
            break;
        if (g_useCharsetTag) {
            struct OwnedText tagged = {0};
            if (!owned_text_concat(&tagged, text_view_cstr(language_charset_tag(g_language, 1)),
                                    text_array_at(&self->lines, line)))
                break;
            ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)tagged.data, 100, 0, 0);
            owned_text_clear(&tagged);
        } else {
            text = book_line(self, line);
            ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)text, 100, 0, 0);
        }
    }
    ui_window_invalidate_base(wnd);
    ui_window_mgr_raise_window(&g_windowMgr, wnd);
}

static void book_close(struct UIBookWnd *self)
{
    if (self->balloon) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->balloon);
        self->balloon = NULL;
    }
    ui_close_window(BOOK_WND_ID);
}

static void book_speak(struct UIBookWnd *self, int page)
{
    self->speaking = !self->speaking;
    ui_window_set_show_base(&self->frame.base, 0);
    if (!self->speaking) {
        book_close(self);
        return;
    }
    self->speak_time = timeGetTime() - BOOK_SPEAK_INTERVAL;
    if (page)
        self->page = page;
    else
        self->page = page = book_load_bookmark(self, self->title);
    if (page > self->page_count) {
        self->page = 0;
        page = 0;
    }
    self->speak_line = page * BOOK_LINES_PER_PAGE - BOOK_LINES_PER_PAGE;
    if (self->speak_line > self->line_count)
        self->speak_line = 0;
}

int ui_book_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                 int val3, int val4)
{
    struct UIBookWnd *self = (struct UIBookWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    (void)sender;
    (void)val4;
    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_CLOSE:
            ui_book_wnd_send_msg(wnd, wnd, UI_MSG_COMMAND, ID_BOOKMARK, 0, 0, 0);
            ui_close_window(BOOK_WND_ID);
            break;
        case ID_PREV:
            if (!self->speaking)
                ui_book_wnd_send_msg(wnd, wnd, BOOK_MSG_GOTO, self->page == 1 ? self->page : self->page - 1, 0, 0, 0);
            break;
        case ID_NEXT:
            if (!self->speaking)
                ui_book_wnd_send_msg(wnd, wnd, BOOK_MSG_GOTO,
                         self->page == self->page_count ? self->page : self->page + 1, 0, 0, 0);
            break;
        case ID_BOOKMARK:
            book_save_bookmark(self);
            break;
        }
        break;
    case BOOK_MSG_PREV:
        ui_book_wnd_send_msg(wnd, wnd, UI_MSG_COMMAND, ID_PREV, 0, 0, 0);
        break;
    case BOOK_MSG_NEXT:
        ui_book_wnd_send_msg(wnd, wnd, UI_MSG_COMMAND, ID_NEXT, 0, 0, 0);
        break;
    case BOOK_MSG_GOTO:
        book_goto(self, val1);
        break;
    case BOOK_MSG_SPEAK:
        book_speak(self, val1);
        break;
    case BOOK_MSG_OPEN:
        book_open(self, (const char *)val1, val2, val3);
        break;
    case BOOK_MSG_CLOSE:
        book_close(self);
        break;
    }
    return 0;
}

static void book_destruct(struct UIBookWnd *self)
{
    self->frame.base.ops = &ui_book_wnd_ops;
    text_array_clear(&self->lines);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_book_wnd_destroy(struct UIWindow *window)
{
    struct UIBookWnd *self = (struct UIBookWnd *)window;
    book_destruct(self);
    mem_free(self);
}
