#include "ui/controls/listbox.h"
#include "ui/controls/edit.h"
#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "contents/connection_api.h"
#include "ui/admin/admin.h"
#include "game/context.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/ui_std.h"

struct UIAccountLimitedToolWnd;
struct UIWindow;
void ui_account_limited_tool_wnd_destroy(struct UIWindow *window);
void ui_account_limited_tool_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_account_limited_tool_wnd_on_draw(struct UIWindow *self);
int ui_account_limited_tool_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_account_limited_tool_wnd_ops = {
    .destroy = ui_account_limited_tool_wnd_destroy,
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
    .on_create = ui_account_limited_tool_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_account_limited_tool_wnd_on_draw,
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
    .send_msg = ui_account_limited_tool_wnd_send_msg,
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

#define UI_MSG_ACCOUNT_LIMIT 0x40
#define ACCOUNT_LIMIT_FOUND 0
#define ACCOUNT_LIMIT_ADDED 1
#define ACCOUNT_LIMIT_REMOVED 2

#define ACCOUNT_LIMIT_ID_SEARCH 0x139
#define ACCOUNT_LIMIT_ID_ADD 0x229
#define ACCOUNT_LIMIT_ID_DELETE 0xcb

#define MODE_MSG_CLOSE_ACCOUNT_LIMIT 0x13f

#define MSG_ACCOUNT_LIMIT_TITLE 0xe42
#define MSG_ACCOUNT_LIMIT_NAME 0xe43
#define MSG_ACCOUNT_LIMIT_ACCOUNT 0xe44
#define MSG_ACCOUNT_LIMIT_ROW1 0xe45
#define MSG_ACCOUNT_LIMIT_START 0xe46
#define MSG_ACCOUNT_LIMIT_END 0xe47
#define MSG_ACCOUNT_LIMIT_DAILY 0xf4c
#define MSG_ACCOUNT_LIMIT_ADDED_FMT 0xe4c
#define MSG_ACCOUNT_LIMIT_NO_NAME 0x9d0
#define MSG_ACCOUNT_LIMIT_NO_VALUE 0x9cc
#define MSG_ACCOUNT_LIMIT_EMPTY_FIELD 0x9cd
#define MSG_ACCOUNT_LIMIT_BAD_FIELD 0x9ce
#define MSG_ACCOUNT_LIMIT_BAD_DAY 0xf4d
#define MSG_ACCOUNT_LIMIT_ADD_OK 0x9d3
#define MSG_ACCOUNT_LIMIT_ADD_FAILED 0x9d4
#define MSG_ACCOUNT_LIMIT_REMOVE_FAILED 0x9d6
#define MSG_ACCOUNT_LIMIT_REMOVED_FMT 0x9d7

#define ACCOUNT_LIMIT_FIELDS 13
#define ACCOUNT_LIMIT_NAME_LEN 0x32
#define ACCOUNT_LIMIT_LOG_ARG 0x3c
#define ACCOUNT_LIMIT_FIELD_BACK 0xf2
#define ACCOUNT_LIMIT_GREY 0xff969696
#define ACCOUNT_LIMIT_RULE 0x99938a

#define PACKET_ACCOUNT_LIMIT_DELETE 0xb4a
#define PACKET_ACCOUNT_LIMIT_SEARCH 0xb53
#define PACKET_ACCOUNT_LIMIT_ADD 0xbac

#pragma pack(push, 1)
struct PacketAccountLimitDelete {
    short id;
    int account;
};
struct PacketAccountLimitSearch {
    short id;
    char name[ACCOUNT_LIMIT_NAME_LEN];
};
struct PacketAccountLimitAdd {
    short id;
    int account;
    int value;
    __time32_t start;
    __time32_t end;
    __time32_t daily;
};
#pragma pack(pop)
SIZE_ASSERT(PacketAccountLimitDelete, 6);
SIZE_ASSERT(PacketAccountLimitSearch, 0x34);
SIZE_ASSERT(PacketAccountLimitAdd, 0x16);

enum {
    FIELD_NAME = 0,
    FIELD_VALUE = 1,
    FIELD_START = 2,
    FIELD_END = 6,
    FIELD_DAILY_DAY = 10,
    FIELD_DAILY_HOUR = 11,
    FIELD_DAILY_MINUTE = 12,
};
enum { BUTTON_SEARCH, BUTTON_ADD, BUTTON_DELETE };

struct UIAccountLimitedToolWnd {
    struct UIFrameWnd frame;
    struct UIWindow *account;
    struct UIListBox *log;
    struct UIWindow *fields[ACCOUNT_LIMIT_FIELDS];
    struct UIWindow *buttons[3];
    struct String account_text;
};

struct UIAccountLimitedToolWnd *ui_account_limited_tool_wnd_construct(struct UIAccountLimitedToolWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_account_limited_tool_wnd_ops;
    self->account = NULL;
    self->log = NULL;
    string_init(&self->account_text);
    memset(self->fields, 0, sizeof self->fields);
    memset(self->buttons, 0, sizeof self->buttons);
    return self;
}

struct UIWindow *ui_account_limited_tool_wnd_new(void)
{
    return (struct UIWindow *)ui_account_limited_tool_wnd_construct(mem_alloc(sizeof(struct UIAccountLimitedToolWnd)));
}

void ui_account_limited_tool_wnd_destroy(struct UIWindow *window)
{
    struct UIAccountLimitedToolWnd *self = (struct UIAccountLimitedToolWnd *)window;
    self->frame.base.ops = &ui_account_limited_tool_wnd_ops;
    string_free(&self->account_text);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

static const POINT field_pos[ACCOUNT_LIMIT_FIELDS] = {
    { 0x5a, 0x19 }, { 0x5a, 0x4d },
    { 0x5a, 0x63 }, { 0x87, 0x63 }, { 0xf0, 0x63 }, { 0x11d, 0x63 },
    { 0x5a, 0x79 }, { 0x87, 0x79 }, { 0xf0, 0x79 }, { 0x11d, 0x79 },
    { 0x5a, 0x97 }, { 0xf0, 0x97 }, { 0x11d, 0x97 },
};

static void set_field_locked(struct UIWindow *field, int locked)
{
    UI_EDIT_NO_BORDER(field) = locked;
    ((struct UIEditCtrl *)field)->disabled = (BYTE)locked;
    ui_window_invalidate(field);
}

static void set_button_disabled(struct UIWindow *button, BYTE disabled)
{
    ((struct UIButton *)button)->disabled = disabled;
    ui_window_invalidate(button);
}

void ui_account_limited_tool_wnd_on_create(struct UIWindow *window, int width_unused,
                                                 int height_unused)
{
    struct UIAccountLimitedToolWnd *self = (struct UIAccountLimitedToolWnd *)window;
    static const char *const bitmaps[3][4] = {
        { "btn_search", "btn_search_a", "btn_search_b", "btn_search_c" },
        { "btn_AddItem", "btn_AddItem_a", "btn_AddItem_b", "btn_AddItem_c" },
        { "btn_deleteItem", "btn_deleteItem_a", "btn_deleteItem_b", "btn_deleteItem_c" },
    };
    static const int ids[3] = { ACCOUNT_LIMIT_ID_SEARCH, ACCOUNT_LIMIT_ID_ADD, ACCOUNT_LIMIT_ID_DELETE };
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *field, *bottom;
    struct UITextViewer *viewer;
    struct String skin, name, path;
    POINT button_pos[3];
    int i, state;

    ui_window_add_close_button(wnd, "", 0);

    for (i = 0; i < ACCOUNT_LIMIT_FIELDS; i++) {
        field = mem_alloc(sizeof(struct UIEditCtrl));
        if (field)
            field = ui_edit_ctrl_construct(field);
        self->fields[i] = field;
        set_field_locked(field, 1);
        switch (i) {
        case FIELD_NAME:
            ui_window_create(field, 0xc8, 0x11);
            set_field_locked(field, 0);
            break;
        case FIELD_VALUE:
            ui_window_create(field, 0x3c, 0x11);
            break;
        case FIELD_DAILY_DAY:
            ui_window_create(field, 0x50, 0x11);
            break;
        default:
            ui_window_create(field, 0x23, 0x11);
            break;
        }
        UI_EDIT_MAX_CHARS(self->fields[i]) = 0x18;
        ui_window_move(self->fields[i], field_pos[i].x, field_pos[i].y);
        ui_edit_ctrl_set_back_colour(self->fields[i], ACCOUNT_LIMIT_FIELD_BACK, ACCOUNT_LIMIT_FIELD_BACK,
                                 ACCOUNT_LIMIT_FIELD_BACK);
        ui_window_add_child(wnd, self->fields[i]);
    }

    self->account = mem_alloc(sizeof(struct UIStaticText));
    if (self->account)
        self->account = ui_static_text_construct(self->account);
    ui_window_create(self->account, 0xc8, 0x11);
    ui_window_move(self->account, 0x5a, 0x2f);
    ((struct UIStaticText *)self->account)->background = ACCOUNT_LIMIT_GREY;
    ui_window_invalidate(self->account);
    ui_window_add_child(wnd, self->account);

    button_pos[0].x = wnd->width - 0x46;
    button_pos[0].y = 0x16;
    button_pos[1].x = 0xf;
    button_pos[1].y = 0xb8;
    button_pos[2].x = 0x104;
    button_pos[2].y = 0xb8;
    UI_FRAME_DEFAULT_ID(self) = ACCOUNT_LIMIT_ID_ADD;

    string_init(&skin);
    string_assign(&skin, UI_SKIN, sizeof UI_SKIN - 1);
    for (i = 0; i < 3; i++) {
        struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

        if (button)
            button = ui_bitmap_button_construct(button);
        self->buttons[i] = &button->button.base;
        for (state = 0; state < 4; state++) {
            string_copy(&name, &skin);
            string_append_cstr(&name, bitmaps[i][state]);
            string_append_cstr(&name, ".bmp");
            path = name;
            string_init(&name);
            ui_bitmap_button_set_bitmap((struct UIBitmapButton *)self->buttons[i], string_cstr(&path),
                                     state, 0);
            string_free(&path);
            string_free(&name);
        }
        button = (struct UIBitmapButton *)self->buttons[i];
        ui_window_create(self->buttons[i], button->bitmap_width, button->bitmap_height);
        ui_window_move(self->buttons[i], button_pos[i].x, button_pos[i].y);
        self->buttons[i]->id = ids[i];
        ui_button_show((struct UIButton *)(self->buttons[i]), 1);
        ui_window_add_child(wnd, self->buttons[i]);
        set_button_disabled(self->buttons[i], i != BUTTON_SEARCH);
    }
    string_free(&skin);

    viewer = mem_alloc(sizeof(struct UITextViewer));
    if (viewer)
        viewer = (struct UITextViewer *)ui_text_viewer_construct((struct UIWindow *)viewer);
    self->log = &viewer->list;
    bottom = self->buttons[BUTTON_ADD];
    ui_list_box_create_window(&viewer->list, 0xa, bottom->height + 0xbd, wnd->width - 0x14, wnd->height - bottom->height - 0xc7, 0);
    UI_TEXT_VIEWER_BACKGROUND(self->log) = ACCOUNT_LIMIT_GREY;
    ui_window_invalidate_base(&self->log->base);
    ui_window_add_child(wnd, &self->log->base);
}

void ui_account_limited_tool_wnd_on_draw(struct UIWindow *window)
{
    struct UIAccountLimitedToolWnd *self = (struct UIAccountLimitedToolWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

#define LABEL(x, y, text) ui_window_text_out(wnd, (x), (y), (text), 0, 0, 0xc, 0, 0, 0)
    ui_window_draw_title_bar(wnd, 0, msg_str(MSG_ACCOUNT_LIMIT_TITLE), 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
    LABEL(5, 0x1c, msg_str(MSG_ACCOUNT_LIMIT_NAME));
    LABEL(5, 0x32, msg_str(MSG_ACCOUNT_LIMIT_ACCOUNT));
    ui_window_fill_rect(wnd, 3, 0x46, wnd->width - 6, 2, ACCOUNT_LIMIT_RULE);
    LABEL(5, 0x50, msg_str(MSG_ACCOUNT_LIMIT_ROW1));
    LABEL(5, 0x66, msg_str(MSG_ACCOUNT_LIMIT_START));
    LABEL(0x7f, 0x66, "/");
    LABEL(0xac, 0x66, "(MM/DD)");
    LABEL(0x116, 0x66, ":");
    LABEL(0x142, 0x66, "(HH:MM)");
    LABEL(5, 0x7c, msg_str(MSG_ACCOUNT_LIMIT_END));
    LABEL(0x7f, 0x7c, "/");
    LABEL(0xac, 0x7c, "(MM/DD)");
    LABEL(0x116, 0x7c, ":");
    LABEL(0x142, 0x7c, "(HH:MM)");
    ui_window_fill_rect(wnd, 3, 0x90, wnd->width - 6, 2, ACCOUNT_LIMIT_RULE);
    LABEL(5, 0x9a, msg_str(MSG_ACCOUNT_LIMIT_DAILY));
    LABEL(0xac, 0x9a, "(DD)");
    LABEL(0x116, 0x9a, ":");
    LABEL(0x142, 0x9a, "(HH:MM)");
    ui_window_fill_rect(wnd, 3, 0xae, wnd->width - 6, 2, ACCOUNT_LIMIT_RULE);
#undef LABEL
}

static void log_add(struct UIAccountLimitedToolWnd *self, const char *text)
{
    ui_window_send_msg(&self->log->base, NULL, UI_MSG_ADD_TEXT, (int)text, ACCOUNT_LIMIT_LOG_ARG, 0, 0);
}
static void log_follow(struct UIAccountLimitedToolWnd *self)
{
    if (self->log->has_vscroll)
        ui_window_send_msg(&self->log->base, NULL, UI_MSG_SCROLL_V, ui_list_box_count(self->log), 0, 0, 0);
}
static void log_line(struct UIAccountLimitedToolWnd *self, const char *text)
{
    log_add(self, text);
    log_follow(self);
}

static int field_int(struct UIAccountLimitedToolWnd *self, int field)
{
    return atoi(ui_edit_ctrl_get_text(self->fields[field]));
}

static int shown_account(struct UIAccountLimitedToolWnd *self)
{
    struct String text;
    int account;

    ui_static_text_get_text(self->account, &text);
    account = atoi(string_cstr(&text));
    string_free(&text);
    return account;
}

static void send_packet(const void *packet, short id)
{
    int len = rag_connection_get_packet_length(rag_connection_instance(), id);

    rag_connection_send_packet(rag_connection_instance(), len, packet);
}

static __time32_t time_this_month(int month, int day, int hour, int minute)
{
    __time32_t now = _time32(NULL);
    struct tm *t = _localtime32(&now);

    t->tm_mon = month - 1;
    t->tm_mday = day;
    t->tm_hour = hour;
    t->tm_min = minute;
    t->tm_sec = 0;
    return _mktime32(t);
}

static void send_add(struct UIAccountLimitedToolWnd *self)
{
    static const int ranges[4][2] = { { 1, 12 }, { 1, 31 }, { 0, 23 }, { 0, 59 } };
    struct PacketAccountLimitAdd packet;
    __time32_t now;
    struct tm *t;
    int i, value, day;

    if (field_int(self, FIELD_VALUE) <= 0) {
        log_line(self, msg_str(MSG_ACCOUNT_LIMIT_NO_VALUE));
        ui_edit_ctrl_select_all(self->fields[FIELD_VALUE]);
        return;
    }
    for (i = FIELD_START; i < FIELD_DAILY_DAY; i++) {
        if (ui_edit_ctrl_get_text_size(self->fields[i]) != 0)
            continue;
        log_add(self, msg_str(MSG_ACCOUNT_LIMIT_EMPTY_FIELD));
        if (!self->log->has_vscroll) {
            value = field_int(self, i);
            if (value >= ranges[(i - FIELD_START) % 4][0] && value <= ranges[(i - FIELD_START) % 4][1])
                continue;
            log_line(self, msg_str(MSG_ACCOUNT_LIMIT_BAD_FIELD));
        } else {
            log_follow(self);
        }
        ui_edit_ctrl_select_all(self->fields[i]);
        return;
    }
    if (field_int(self, FIELD_DAILY_DAY) > 14) {
        log_add(self, msg_str(MSG_ACCOUNT_LIMIT_BAD_DAY));
        ui_edit_ctrl_select_all(self->fields[FIELD_DAILY_DAY]);
        return;
    }

    packet.start = time_this_month(field_int(self, FIELD_START), field_int(self, FIELD_START + 1),
                                   field_int(self, FIELD_START + 2), field_int(self, FIELD_START + 3));
    packet.end = time_this_month(field_int(self, FIELD_END), field_int(self, FIELD_END + 1),
                                 field_int(self, FIELD_END + 2), field_int(self, FIELD_END + 3));
    now = _time32(NULL);
    t = _localtime32(&now);
    day = field_int(self, FIELD_DAILY_DAY);
    t->tm_mday = day == 0 ? 15 : day;
    t->tm_hour = field_int(self, FIELD_DAILY_HOUR);
    t->tm_min = field_int(self, FIELD_DAILY_MINUTE);
    t->tm_sec = 0;
    packet.daily = _mktime32(t);

    packet.id = PACKET_ACCOUNT_LIMIT_ADD;
    packet.account = shown_account(self);
    packet.value = field_int(self, FIELD_VALUE);
    send_packet(&packet, packet.id);
}

static void send_search(struct UIAccountLimitedToolWnd *self)
{
    struct PacketAccountLimitSearch packet;
    struct String name;
    const char *text = ui_edit_ctrl_get_text(self->fields[FIELD_NAME]);

    string_init(&name);
    string_assign_cstr(&name, text);
    if (name.size == 0) {
        log_line(self, msg_str(MSG_ACCOUNT_LIMIT_NO_NAME));
        ui_edit_ctrl_select_all(self->fields[FIELD_NAME]);
    } else if (name.size < ACCOUNT_LIMIT_NAME_LEN) {
        packet.id = PACKET_ACCOUNT_LIMIT_SEARCH;
        memset(packet.name, 0, sizeof packet.name);
        strncpy_s(packet.name, sizeof packet.name, string_cstr(&name), name.size);
        send_packet(&packet, packet.id);
    }
    string_free(&name);
}

int ui_account_limited_tool_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                               int val1, int val2, int val3, int val4)
{
    struct UIAccountLimitedToolWnd *self = (struct UIAccountLimitedToolWnd *)window;
    struct String line;
    int i;

    if (msg == UI_MSG_COMMAND) {
        switch (val1) {
        case UI_ID_CLOSE: {
            game_send_message(MODE_MSG_CLOSE_ACCOUNT_LIMIT, 0, 0, 0, 0);
            break;
        }
        case ACCOUNT_LIMIT_ID_DELETE: {
            struct PacketAccountLimitDelete packet;

            packet.id = PACKET_ACCOUNT_LIMIT_DELETE;
            packet.account = shown_account(self);
            send_packet(&packet, packet.id);
            break;
        }
        case ACCOUNT_LIMIT_ID_SEARCH:
            send_search(self);
            break;
        case ACCOUNT_LIMIT_ID_ADD:
            send_add(self);
            break;
        }
        return 0;
    }
    if (msg != UI_MSG_ACCOUNT_LIMIT)
        return 0;

    switch (val1) {
    case ACCOUNT_LIMIT_FOUND:
        ui_static_text_set_number(self->account, val2);
        set_button_disabled(self->buttons[BUTTON_ADD], 0);
        set_button_disabled(self->buttons[BUTTON_DELETE], 0);
        for (i = 1; i < ACCOUNT_LIMIT_FIELDS; i++)
            set_field_locked(self->fields[i], 0);
        break;
    case ACCOUNT_LIMIT_ADDED:
        if (val2 == 0) {
            string_init(&line);
            string_format(&line, msg_str(MSG_ACCOUNT_LIMIT_ADDED_FMT),
                          ui_edit_ctrl_get_text(self->fields[FIELD_NAME]), field_int(self, 1),
                          field_int(self, 2), field_int(self, 3), field_int(self, 4), field_int(self, 5),
                          field_int(self, 6), field_int(self, 7), field_int(self, 8), field_int(self, 9));
            log_add(self, string_cstr(&line));
            log_line(self, msg_str(MSG_ACCOUNT_LIMIT_ADD_OK));
            set_button_disabled(self->buttons[BUTTON_DELETE], 0);
            for (i = 1; i < ACCOUNT_LIMIT_FIELDS; i++)
                set_field_locked(self->fields[i], 1);
            string_free(&line);
        } else if (val2 == 1 || val2 == 3) {
            log_line(self, msg_str(MSG_ACCOUNT_LIMIT_ADD_FAILED));
            set_button_disabled(self->buttons[BUTTON_ADD], 1);
            set_button_disabled(self->buttons[BUTTON_DELETE], 1);
        }
        break;
    case ACCOUNT_LIMIT_REMOVED:
        if (val2 == 1) {
            log_line(self, msg_str(MSG_ACCOUNT_LIMIT_REMOVE_FAILED));
            break;
        }
        string_init(&line);
        string_format(&line, msg_str(MSG_ACCOUNT_LIMIT_REMOVED_FMT),
                      ui_edit_ctrl_get_text(self->fields[FIELD_NAME]));
        ui_window_send_msg(&self->log->base, NULL, UI_MSG_ADD_TEXT, (int)string_cstr(&line), ACCOUNT_LIMIT_LOG_ARG,
                 0, 0);
        string_free(&line);
        log_follow(self);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->fields[FIELD_NAME], "");
        ui_static_text_set_text(self->account, "", 0);
        for (i = 1; i < ACCOUNT_LIMIT_FIELDS; i++) {
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->fields[i], "");
            set_field_locked(self->fields[i], 1);
        }
        set_button_disabled(self->buttons[BUTTON_ADD], 1);
        set_button_disabled(self->buttons[BUTTON_DELETE], 1);
        ui_window_invalidate_children(&self->frame.base);
        break;
    }
    return 0;
}
