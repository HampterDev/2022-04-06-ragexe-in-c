#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ui/social/social.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"

enum MemorialState {
    MEMORIAL_WAITING = 0,
    MEMORIAL_ENTER_BY = 1,
    MEMORIAL_DESTROY_AT = 2,
    MEMORIAL_NOTICE = 3,
};

#define MEMORIAL_ID_OK 0xb0
#define MEMORIAL_ID_DESTROY 0xcb

#define MSG_MEMORIAL_TITLE 0x538
#define MSG_MEMORIAL_HEADER_FIRST 0x539
#define MSG_MEMORIAL_NOTICE_1 0x53c
#define MSG_MEMORIAL_NOTICE_2 0x53d
#define MSG_MEMORIAL_QUEUE_PLACE 0x53e
#define MSG_MEMORIAL_ENTER_BY 0x53f
#define MSG_MEMORIAL_DESTROY_AT 0x540
#define MSG_TIME_FORMAT 0x4ec
#define MSG_MEMORIAL_DESTROY_BUTTON 0xcc6

#define MEMORIAL_FONT_HEIGHT 0xc
#define MEMORIAL_BORDER 0x8e8e8e
#define MEMORIAL_BODY 0xffffff

struct UIMemorialDunWnd {
    struct UIFrameWnd frame;
    struct UIBitmapButton *ok;
    struct UIWindow *destroy;
};

void ui_memorial_dun_wnd_destroy(struct UIWindow *window);
void ui_memorial_dun_wnd_move(struct UIWindow *self, int x, int y);
void ui_memorial_dun_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_memorial_dun_wnd_on_draw(struct UIWindow *self);
intptr_t ui_memorial_dun_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_memorial_dun_wnd_ops = {
    .destroy = ui_memorial_dun_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_memorial_dun_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_memorial_dun_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_memorial_dun_wnd_on_draw,
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
    .send_msg = ui_memorial_dun_wnd_send_msg,
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

struct UIMemorialDunWnd *ui_memorial_dun_wnd_construct(struct UIMemorialDunWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_memorial_dun_wnd_ops;
    self->destroy = NULL;
    return self;
}

struct UIWindow *ui_memorial_dun_wnd_new(void)
{
    return (struct UIWindow *)ui_memorial_dun_wnd_construct(mem_alloc(sizeof(struct UIMemorialDunWnd)));
}

void ui_memorial_dun_wnd_destruct(struct UIMemorialDunWnd *self)
{
    self->frame.base.ops = &ui_memorial_dun_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_memorial_dun_wnd_destroy(struct UIWindow *window)
{
    struct UIMemorialDunWnd *self = (struct UIMemorialDunWnd *)window;
    ui_memorial_dun_wnd_destruct(self);
    mem_free(self);
}

void ui_memorial_dun_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UIMemorialDunWnd *self = (struct UIMemorialDunWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (x > g_renderer->width - wnd->width)
        x = 0;
    if (y > g_renderer->height - wnd->height)
        y = 0x136;
    ui_frame_wnd_move((struct UIWindow *)(&self->frame), x, y);
    g_session1.mdWndX = x;
    g_session1.mdWndY = y;
}

void ui_memorial_dun_wnd_place_buttons(struct UIMemorialDunWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *ok = &self->ok->button.base;

    if (self->destroy)
        ui_window_move(self->destroy, -100, -100);
    switch (g_session1.mdState) {
    case MEMORIAL_WAITING:
    case MEMORIAL_ENTER_BY:
    case MEMORIAL_DESTROY_AT:
        ui_window_move_base(ok, -100, -100);
        self->frame.default_id = 0x239;
        if (g_session1.partyLeader && self->destroy && g_session1.mdState == MEMORIAL_ENTER_BY)
            ui_window_move(self->destroy, 0x29, 0x73);
        break;
    case MEMORIAL_NOTICE:
        ui_window_move_base(ok, (wnd->width - ok->width) / 2, wnd->height - 0x1e);
        self->frame.default_id = MEMORIAL_ID_OK;
        break;
    }
    ui_window_invalidate_base(wnd);
}

void ui_memorial_dun_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIMemorialDunWnd *self = (struct UIMemorialDunWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *button;

    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "Alt+B", 0);
    self->ok = ui_add_bitmap_button(wnd, UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp", -100,
                                    -100, MEMORIAL_ID_OK);

    button = mem_try_alloc(sizeof(struct UIBasicButton));
    if (button)
        button = (struct UIWindow *)ui_basic_button_construct((struct UIBasicButton *)button);
    self->destroy = button;
    ui_basic_button_set_label(button, 0x7d, msg_str(MSG_MEMORIAL_DESTROY_BUTTON));
    ui_window_move(self->destroy, -100, -100);
    self->destroy->id = MEMORIAL_ID_DESTROY;
    ui_window_add_child(wnd, self->destroy);
    ui_memorial_dun_wnd_place_buttons(self);
}

static void draw_line(struct UIWindow *wnd, int x, int y, const char *text, COLORREF *colour)
{
    ui_window_text_out_runs(wnd, x, y, text, 0, colour, 0, MEMORIAL_FONT_HEIGHT, 0);
}

static void draw_centred(struct UIWindow *wnd, int y, const char *text, COLORREF *colour)
{
    int w = ui_window_get_color_text_width(wnd, text, 0, 0, MEMORIAL_FONT_HEIGHT, 0);

    draw_line(wnd, (wnd->width - w) / 2, y, text, colour);
}

#define LINE(lines, i) text_array_at(&(lines), (i)).data
#define LINE_COUNT(lines) ((lines).size)

static void draw_wrapped(struct UIWindow *wnd, struct TextArray *lines, const char *text, int x, int y, int step,
                         COLORREF *colour)
{
    unsigned i;

    ui_window_break_lines(wnd, text, lines, wnd->width - 0x14);
    for (i = 0; i < LINE_COUNT(*lines); i++, y += step)
        draw_line(wnd, x, y, LINE(*lines, i), colour);
}

static void format_time(char *buf, __time32_t timestamp)
{
    const struct tm *local = _localtime32(&timestamp);
    buf[0] = 0;
    if (local)
        strftime(buf, 0x80, msg_str(MSG_TIME_FORMAT), local);
}

void ui_memorial_dun_wnd_on_draw(struct UIWindow *window)
{
    struct UIMemorialDunWnd *self = (struct UIMemorialDunWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    char blue[12], black[12], name[80], header[100], when[0x80], text[500];
    COLORREF colour = 0;
    struct TextArray lines;

    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_MEMORIAL_TITLE), 0);
    if (wnd->height == 0x11)
        return;
    ui_window_fill_rect(wnd, 0, 0x11, wnd->width, wnd->height - 0x13, MEMORIAL_BODY);
    ui_window_fill_rect(wnd, 1, wnd->height - 2, wnd->width - 2, 1, MEMORIAL_BODY);
    ui_window_fill_rect(wnd, 2, wnd->height - 1, wnd->width - 4, 1, MEMORIAL_BODY);

    if (g_session1.mdState != MEMORIAL_NOTICE) {
        sprintf(blue, "^%02X%02X%02X", 0x41, 0x69, 0xe1);
        sprintf(black, "^%02X%02X%02X", 0, 0, 0);
        sprintf(name, "%s[%s]%s", blue, string_cstr(&g_session1.mdName), black);
        sprintf(header, msg_str(MSG_MEMORIAL_HEADER_FIRST + g_session1.mdState), name);
        draw_centred(wnd, 0x1e, header, &colour);
    }

    switch (g_session1.mdState) {
    case MEMORIAL_WAITING:
        sprintf(text, msg_str(MSG_MEMORIAL_QUEUE_PLACE), g_session1.mdQueuePlace);
        draw_centred(wnd, (wnd->height - 0x11) / 2, text, &colour);
        break;
    case MEMORIAL_ENTER_BY:
        if (g_session1.mdEnterLimit == 0)
            break;
        format_time(when, (__time32_t)g_session1.mdEnterLimit);
        sprintf(text, msg_str(MSG_MEMORIAL_ENTER_BY), when);
        memset(&lines, 0, sizeof lines);
        ui_window_break_lines(wnd, text, &lines, wnd->width - 0x14);
        if (LINE_COUNT(lines) > 1) {
            unsigned i, y = ((unsigned)(wnd->height - LINE_COUNT(lines) * 0x11 - 0x48) >> 1) + 0x28;

            for (i = 0; i < LINE_COUNT(lines); i++, y += 0x11)
                draw_line(wnd, 0xa, y, LINE(lines, i), &colour);
        } else {
            int w = ui_window_get_color_text_width(wnd, text, 0, 0, MEMORIAL_FONT_HEIGHT, 0);

            draw_line(wnd, (wnd->width - w) / 2, 0x37, LINE(lines, 0), &colour);
        }
        text_array_clear(&lines);
        break;
    case MEMORIAL_DESTROY_AT:
        if (g_session1.mdDestroyTime == 0)
            break;
        ui_window_text_out(wnd, 0xa, 0x37, msg_str(MSG_MEMORIAL_DESTROY_AT), 0, 0, MEMORIAL_FONT_HEIGHT, 0, 0,
                         0);
        format_time(when, (__time32_t)g_session1.mdDestroyTime);
        sprintf(text, "^ff0000%s^ffffff", when);
        draw_centred(wnd, 0x4b, text, &colour);
        break;
    case MEMORIAL_NOTICE:
        memset(&lines, 0, sizeof lines);
        draw_wrapped(wnd, &lines, msg_str(MSG_MEMORIAL_NOTICE_1), 5, 0x37, 0xf, &colour);
        draw_wrapped(wnd, &lines, msg_str(MSG_MEMORIAL_NOTICE_2), 0xa, 0x46, 0xf, &colour);
        text_array_clear(&lines);
        break;
    }

    ui_window_fill_rect(wnd, 2, 0, wnd->width - 4, 1, MEMORIAL_BORDER);
    ui_window_fill_rect(wnd, 1, 1, 1, 1, MEMORIAL_BORDER);
    ui_window_fill_rect(wnd, 0, 2, 1, wnd->height - 4, MEMORIAL_BORDER);
    ui_window_fill_rect(wnd, 1, wnd->height - 2, 1, 1, MEMORIAL_BORDER);
    ui_window_fill_rect(wnd, wnd->width - 2, 1, 1, 1, MEMORIAL_BORDER);
    ui_window_fill_rect(wnd, wnd->width - 1, 2, 1, wnd->height - 4, MEMORIAL_BORDER);
    ui_window_fill_rect(wnd, 2, wnd->height - 1, wnd->width - 4, 1, MEMORIAL_BORDER);
    ui_window_fill_rect(wnd, wnd->width - 2, wnd->height - 2, 1, 1, MEMORIAL_BORDER);
}

void memorial_dun_info_reset(void)
{
    string_clear(&g_session1.mdName);
    g_session1.mdQueuePlace = 0;
    g_session1.mdDestroyTime = 0;
    g_session1.mdEnterLimit = 0;
    g_session1.mdState = MEMORIAL_WAITING;
}

#define MEMORIAL_PACKET_ID 0x2cf

#pragma pack(push, 1)
struct MemorialCancelPacket {
    WORD id;
    int state;
};
#pragma pack(pop)

intptr_t ui_memorial_dun_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                        intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMemorialDunWnd *self = (struct UIMemorialDunWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (msg == UI_MSG_DEFAULT_BUTTON) {
        ui_memorial_dun_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, self->frame.default_id, 0, 0, 0);
        return 0;
    }
    if (msg == 0x3c) {
        ui_memorial_dun_wnd_place_buttons(self);
        return 0;
    }
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    switch (val1) {
    case MEMORIAL_ID_OK:
        ui_window_mgr_post_quit1(&g_windowMgr, 0x89);
        break;
    case UI_ID_CANCEL: {
        struct MemorialCancelPacket packet;

        packet.id = MEMORIAL_PACKET_ID;
        packet.state = g_session1.mdState == MEMORIAL_ENTER_BY ? 1 : 0;
        rag_connection_send_packet(rag_connection_instance(),
                                  rag_connection_get_packet_length(rag_connection_instance(), MEMORIAL_PACKET_ID),
                                  &packet);
        break;
    }
    case UI_ID_CLOSE:
        ui_window_set_show_base(wnd, 0);
        if (g_session1.mdState == MEMORIAL_DESTROY_AT)
            g_session1.mdWndShown = 0;
        break;
    case MEMORIAL_ID_DESTROY:
        if (g_session1.partyLeader) {
            game_send_message(0x132, 0, 0, 0, 0);
        }
        break;
    }
    return 0;
}
