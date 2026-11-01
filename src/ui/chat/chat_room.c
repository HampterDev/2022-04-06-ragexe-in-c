#include "render/rp_pool.h"
#include "ui/actor_labels.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include "ui/chat/chat_room_wnd.h"
#include "ui/ui_std.h"
#include <time.h>
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include <mmsystem.h>
#include <stdlib.h>
#include "core/msg_str.h"
#include "diag/perf.h"
#include "core/app.h"

struct UIChatRoomTitle;
struct UIWindow;
void ui_chat_room_title_destroy(struct UIWindow *window);
void ui_chat_room_title_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_chat_room_title_on_draw(struct UIWindow *self);
void ui_chat_room_title_on_l_btn_dbl_clk(struct UIWindow *self, int x_unused, int y_unused);
void ui_chat_room_title_on_mouse_shape(struct UIWindow *self, int x, int y);
int ui_chat_room_title_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_chat_room_title_ops = {
    .destroy = ui_chat_room_title_destroy,
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
    .on_create = ui_chat_room_title_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_chat_room_title_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_chat_room_title_on_l_btn_dbl_clk,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_chat_room_title_on_mouse_shape,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_chat_room_title_send_msg,
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

#define CURSOR_CHAT_ROOM 2

struct UIChatRoomTitle {
    struct UIWindow base;
    struct UIWindow *text;
    struct ChatRoomInfo room;
    short font_code;
};
OFFSET_ASSERT(UIChatRoomTitle, room, 0x80);

const struct ChatRoomInfo *ui_chat_room_title_info(const struct UIWindow *window)
{
    const struct UIChatRoomTitle *title = (const struct UIChatRoomTitle *)window;
    return title ? &title->room : NULL;
}

static struct UIChatRoomTitle *ui_chat_room_title_construct(struct UIChatRoomTitle *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_chat_room_title_ops;
    string_init(&self->room.title);
    string_init(&self->room.password);
    return self;
}

struct UIChatRoomTitle *ui_chat_room_title_new(void)
{
    struct UIChatRoomTitle *self = mem_alloc(sizeof *self);

    return self ? ui_chat_room_title_construct(self) : NULL;
}

void ui_chat_room_title_destroy(struct UIWindow *window)
{
    struct UIChatRoomTitle *self = (struct UIChatRoomTitle *)window;
    self->base.ops = &ui_chat_room_title_ops;
    string_free(&self->room.password);
    string_free(&self->room.title);
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_chat_room_title_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIChatRoomTitle *self = (struct UIChatRoomTitle *)window;
    struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

    if (text)
        text = ui_static_text_construct(text);
    self->text = text;
    ui_window_create(text, self->base.width - 0x22, 0xf);
    ui_window_move(self->text, 0x1f, 8);
    ui_static_text_set_text_ellipsis(self->text, "");
    ((struct UIStaticText *)self->text)->font_face = 0;
    ((struct UIStaticText *)self->text)->font_height = 0xc;
    ((struct UIStaticText *)self->text)->font_arg3 = 0;
    ((struct UIStaticText *)self->text)->font_arg4 = 0;
    ui_window_add_child(&self->base, self->text);
}

void ui_chat_room_title_on_draw(struct UIWindow *window)
{
    struct UIChatRoomTitle *self = (struct UIChatRoomTitle *)window;
    struct UIWindow *wnd = &self->base;
    struct BitmapRes *res;
    int steps, x, i;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "sysbox_lu.bmp"), 0);
    res = ui_skin_bitmap(UI_SKIN "sysbox_ru.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - res->width, 0, res, 0);
    steps = (wnd->width - 0x18) / 12 + ((wnd->width - 0x18) % 12 != 0);
    for (i = 0, x = 12; i < steps; i++, x += 12)
        ui_window_draw_bitmap(wnd, x, 0, ui_skin_bitmap(UI_SKIN "sysbox_mu.bmp"), 0);

    res = ui_skin_bitmap(UI_SKIN "sysbox_ld.bmp");
    ui_window_draw_bitmap(wnd, 0, wnd->height - res->height - 6, res, 0);
    res = ui_skin_bitmap(UI_SKIN "sysbox_rd.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - res->width, wnd->height - res->height - 6,
                        res, 0);
    steps = (wnd->width - 0x18) / 12 + ((wnd->width - 0x18) % 12 != 0);
    for (i = 0, x = 12; i < steps; i++, x += 12) {
        res = ui_skin_bitmap(UI_SKIN "sysbox_md.bmp");
        ui_window_draw_bitmap(wnd, x, wnd->height - res->height - 6, res, 0);
    }

    res = ui_skin_bitmap(UI_SKIN "sysbox_arr_l.bmp");
    ui_window_draw_bitmap(wnd, wnd->width / 2 - res->width / 2, wnd->height - res->height,
                        res, 0);
    res = ui_skin_bitmap(self->room.type != CHAT_ROOM_PRIVATE ? UI_SKIN "chat_open.bmp" : UI_SKIN "chat_close.bmp");
    ui_window_draw_bitmap(wnd, 3, 3, res, 1);
}

void ui_chat_room_title_on_mouse_shape(struct UIWindow *window, int x, int y)
{
    struct UIChatRoomTitle *self = (struct UIChatRoomTitle *)window;
    if (x >= 0 && x < self->base.width && y >= 0 && y <= self->base.height)
        mode_set_cursor_action(g_modeMgr.curMode, CURSOR_CHAT_ROOM);
    else
        mode_set_cursor_action(g_modeMgr.curMode, CURSOR_ARROW);
}

#define WND_ID_CHAT_ROOM_PASSWORD 0x1d
#define CHAT_TITLE_MSG_JOIN 0x2f
#define MODE_REQ_JOIN_CHAT_ROOM 0x2d
#define CHAT_TITLE_MSG_ROOM 0x22

void ui_chat_room_title_on_l_btn_dbl_clk(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIChatRoomTitle *self = (struct UIChatRoomTitle *)window;
    struct Mode *mode = game_current_base_mode();
    struct UIWindow *wnd;

    switch (self->room.type) {
    case CHAT_ROOM_PRIVATE:
        wnd = ui_window_open(WND_ID_CHAT_ROOM_PASSWORD);
        ui_window_send(wnd, NULL, CHAT_TITLE_MSG_JOIN, self->room.room_id, 0, 0, 0);
        break;
    case CHAT_ROOM_NON_CLICKABLE:
        break;
    default:
        mode_send_message(mode, MODE_REQ_JOIN_CHAT_ROOM, self->room.room_id, 0, 0, 0);
        break;
    }
}

static int title_font_face(short font_code)
{
    switch (font_code) {
    case 1: return 0x15;
    case 2: return 0x14;
    case 3: return 0x16;
    case 4: return 0x17;
    case 5: return 0x18;
    case 6: return 0x19;
    case 7: return 0x1a;
    case 8: return 0x1b;
    case 9: return 0x1c;
    }
    return 0;
}

static void cut_at_cr(struct String *text)
{
    const char *chars = string_cstr(text);
    const char *cr;

    if (!text->size)
        return;
    cr = memchr(chars, '\r', text->size);
    if (!cr)
        return;
    string_truncate(text, (size_t)(cr - chars));
}

int ui_chat_room_title_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UIChatRoomTitle *self = (struct UIChatRoomTitle *)window;
    const struct ChatRoomInfo *room;
    char line[0x180];
    struct UIWindow *text;

    switch (msg) {
    case UI_MSG_COMMAND:
        return 0;
    case CHAT_TITLE_MSG_ROOM:
        break;
    default:
        return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
    }
    room = (const struct ChatRoomInfo *)val1;
    string_assign(&self->room.title, string_cstr(&room->title), room->title.size);
    string_assign(&self->room.password, string_cstr(&room->password), room->password.size);
    self->room.type = room->type;
    self->room.users = room->users;
    self->room.max_users = room->max_users;
    self->room.room_id = room->room_id;
    cut_at_cr(&self->room.title);

    if (self->room.type == CHAT_ROOM_NON_CLICKABLE)
        sprintf(line, "%s", string_cstr(&self->room.title));
    else
        sprintf(line, "%s (%d/%d)", string_cstr(&self->room.title), self->room.users, self->room.max_users);
    self->font_code = (short)val2;
    text = self->text;
    ((struct UIStaticText *)text)->font_face = title_font_face(self->font_code);
    ((struct UIStaticText *)text)->font_arg3 = 0;
    ((struct UIStaticText *)text)->font_arg4 = 0;
    ((struct UIStaticText *)text)->font_height = 0xc;
    ui_static_text_set_text_ellipsis(self->text, line);
    ui_window_invalidate_base(&self->base);
    return 0;
}

void ui_chat_room_wnd_destroy(struct UIWindow *window)
{
    struct UIChatRoomWnd *self = (struct UIChatRoomWnd *)window;

    self->frame.base.ops = &ui_chat_room_wnd_ops;
    ui_menu_entry_vec_free(&self->menu);
    tree_free(&self->members, chat_room_member_destroy);
    string_free(&self->room.password);
    string_free(&self->room.title);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_chat_room_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UIChatRoomWnd *self = (struct UIChatRoomWnd *)window;
    self->frame.base.x = x;
    self->frame.base.y = y;
    ui_frame_wnd_snap(&self->frame.base);
}

void ui_chat_room_wnd_store_info(struct UIWindow *window)
{
    struct UIChatRoomWnd *self = (struct UIChatRoomWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}

static int talk_width(int width)
{
    return width - chat_room_member_width(width);
}

void ui_chat_room_wnd_on_draw(struct UIWindow *window)
{
    struct UIChatRoomWnd *self = (struct UIChatRoomWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int red = 0, green, blue, top;

    top = ui_window_draw_title_bar(wnd, 0, "", 0);
    ui_window_draw_body(wnd, top, 0, "");
    ui_skin_color_chip(&g_skinMgr, 0x14, 6, &green, &green, &blue);
    ui_window_fill_rect(wnd, talk_width(wnd->width) - 2, 0x11, 1, wnd->height - 0x26,
                      RGB(red, (BYTE)green, (BYTE)blue));
    ui_window_draw_button_bar(wnd, 0, 0, 1);
}

#define UIWM_MSG_SHOW_CHAT 3
#define CHAT_SAVED_COLOUR 0xff00

void ui_chat_room_wnd_save_talk(struct UIChatRoomWnd *self)
{
    char path[0x800];
    struct String name, line;
    unsigned i;
    int number = 0;

    sprintf(path, "%s\\Chat", g_clientDir);
    if (_access(path, 0) != 0 && _mkdir(path) != 0)
        return;
    string_copy(&name, &self->room.title);
    for (i = 0; i < name.size; i++) {
        switch (string_cstr(&name)[i]) {
        case '\\': case '/': case ':': case '*': case '?': case '"': case '<': case '>': case '|':
            name.data[i] = '_';
            break;
        }
    }
    sprintf(path, "%s\\Chat\\%s.txt", g_clientDir, string_cstr(&name));
    while (_access(path, 0) == 0) {
        number++;
        sprintf(path, "%s\\Chat\\%s%03d.txt", g_clientDir, string_cstr(&name), number);
    }
    if (ui_chat_his_box_save_to_file(&self->talk->box, path)) {
        string_init(&line);
        string_assign_cstr(&line, path);
        string_append_cstr(&line, " is Saved.");
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
        ui_chat_line(string_cstr(&line), CHAT_SAVED_COLOUR);
        string_free(&line);
    }
    string_free(&name);
}
