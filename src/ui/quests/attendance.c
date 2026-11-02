#include "contents/check_attendance.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "render/texture.h"
#include "render/texture_cache.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include <time.h>
#include "platform/win_types.h"
#include "ui/quests/quests.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "render/screen_draw.h"
#include "ui/wndmgr/wndmgr.h"
#include "platform/clock.h"

#define ATTENDANCE_WND_ID 0x148
#define ATTENDANCE_DIR UI_SKIN "check_attendance\\"

#define ATTENDANCE_ID_CHECK_IN UI_ID_OK
#define ATTENDANCE_MSG_STATE 0x22
#define PACKET_CHECK_ATTENDANCE 0xaef

#define ATTENDANCE_SLOTS_PER_ROW 5
#define ATTENDANCE_SLOT_X 0x1a
#define ATTENDANCE_SLOT_Y 0x5f
#define ATTENDANCE_SLOT_PITCH_X 62
#define ATTENDANCE_SLOT_PITCH_Y 64

enum AttendanceTodayState {
    ATTENDANCE_TODAY_A = 0,
    ATTENDANCE_TODAY_B = 1,
    ATTENDANCE_TODAY_PRESSED = 2,
};

#define ITEM_INFO_WND_ID 0xc
#define ITEM_INFO_MSG_SHOW 0x18

struct UICheckAttendanceWnd {
    struct UIFrameWnd frame;
    int day_count;
    BYTE today_open;
    int days_checked;
    int days_left;
    int today_state;
    struct UIBmpTextButton *close;
};
void ui_check_attendance_wnd_destroy(struct UIWindow *window);
void ui_check_attendance_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_check_attendance_wnd_on_process(struct UIWindow *self);
void ui_check_attendance_wnd_on_draw(struct UIWindow *self);
void ui_check_attendance_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_check_attendance_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_check_attendance_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_check_attendance_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_check_attendance_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_check_attendance_wnd_render_overlay(struct UIWindow *self);
const struct UIWindowOps ui_check_attendance_wnd_ops = {
    .destroy = ui_check_attendance_wnd_destroy,
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
    .on_create = ui_check_attendance_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_check_attendance_wnd_on_process,
    .on_draw = ui_check_attendance_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_check_attendance_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_check_attendance_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_check_attendance_wnd_on_l_btn_up,
    .on_r_btn_down = ui_check_attendance_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_check_attendance_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_check_attendance_wnd_render_overlay,
};

struct UICheckAttendanceWnd *ui_check_attendance_wnd_construct(struct UICheckAttendanceWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_check_attendance_wnd_ops;
    self->today_open = 0;
    self->days_checked = 0;
    self->days_left = 0;
    self->today_state = ATTENDANCE_TODAY_A;
    self->close = NULL;
    self->day_count = g_attendanceTable->rewards.size;
    return self;
}

struct UIWindow *ui_check_attendance_wnd_new(void)
{
    return (struct UIWindow *)ui_check_attendance_wnd_construct(mem_alloc(sizeof(struct UICheckAttendanceWnd)));
}

void ui_check_attendance_wnd_destroy(struct UIWindow *window)
{
    struct UICheckAttendanceWnd *self = (struct UICheckAttendanceWnd *)window;
    self->frame.base.ops = &ui_check_attendance_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_check_attendance_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UICheckAttendanceWnd *self = (struct UICheckAttendanceWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int date = g_attendanceTable->end_date;
    struct tm end;
    __time32_t now, left;
    struct UIBmpTextButton *close;

    end.tm_sec = 0;
    end.tm_min = 0;
    end.tm_hour = 15;
    end.tm_mday = date % 100;
    end.tm_mon = date % 10000 / 100 - 1;
    end.tm_year = date / 10000 - 1900;
    _time32(&now);
    left = _mktime32(&end) - now;
    if (left > 0)
        self->days_left = _localtime32(&left)->tm_yday;
    else
        self->days_left = 0;

    close = mem_alloc(sizeof *close);
    if (close)
        close = ui_bmp_text_button_construct(close);
    self->close = close;
    ui_bitmap_button_set_bitmap(&close->bmp, ATTENDANCE_DIR "bt_ok_normal.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(&self->close->bmp, ATTENDANCE_DIR "bt_ok_normal.bmp", UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(&self->close->bmp, ATTENDANCE_DIR "bt_ok_press.bmp", UI_BTN_DOWN, 0);
    close = self->close;
    ui_window_create(&close->bmp.button.base, close->bmp.bitmap_width, close->bmp.bitmap_height);
    ui_bmp_text_button_set_caption(close, text_view_cstr(msg_str(0xbe5)));
    ui_bmp_text_button_set_text_style(self->close, UI_BTN_UP, 0xe, 0x313131, 1, 1, UI_TEXT_CENTRED,
                                 UI_TEXT_CENTRED);
    ui_bmp_text_button_set_text_style(self->close, UI_BTN_HOVER, 0xe, 0x313131, 1, 1, UI_TEXT_CENTRED,
                                 UI_TEXT_CENTRED);
    ui_bmp_text_button_set_text_style(self->close, UI_BTN_DOWN, 0xe, 0x313131, 1, 1, UI_TEXT_CENTRED,
                                 UI_TEXT_CENTRED);
    ui_window_move_base(&self->close->bmp.button.base, 0xaa, 0x178);
    self->close->bmp.button.base.id = UI_ID_CLOSE;
    ui_window_add_child(wnd, &self->close->bmp.button.base);

}

static void text_centred(struct UICheckAttendanceWnd *self, int x, int y, const char *text, int width, int height,
                         COLORREF colour)
{
    ui_window_text_out_centred(&self->frame.base, x, y, text, width, 0, height, colour, 1);
}

static void destroy_string(void *value) { string_free(value); }

static void draw_today_text(struct UICheckAttendanceWnd *self, const struct String *text)
{
    struct UIWindow *wnd = &self->frame.base;
    struct List lines;
    struct List widths;
    struct String copy;
    struct ListNode *node;
    int top, i;

    list_init(&lines);
    string_copy(&copy, text);
    ui_window_wrap_text_plain(wnd, &widths, copy, &lines, 0x70, 0, 0xc, 1);
    list_free(&widths, NULL);
    top = (int)((double)(unsigned)(0x4d - lines.size * 15) * 0.5 + 95.0);
    for (node = lines.head->next, i = 0; node != lines.head; node = node->next, i++) {
        int step = i - 1 > 0 ? 5 * i - 1 : 4 * i;

        text_centred(self, 0x157, step * 3 + top, string_cstr(LIST_VALUE(node, struct String)), 0x70,
                     0xc, 0x613ca5);
    }
    list_free(&lines, destroy_string);
}

void ui_check_attendance_wnd_on_draw(struct UIWindow *window)
{
    struct UICheckAttendanceWnd *self = (struct UICheckAttendanceWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct CheckAttendanceMgr *table;
    struct String text, line;
    int start, end, day;

    ui_window_draw_bitmap(wnd, 0, 0,
                        res_mgr_get(g_resMgr(),
                                     ATTENDANCE_DIR "attendance_bg.bmp"),
                        0);
    string_init(&text);
    string_format(&text, msg_str(0xd89), string_cstr(&g_session1.serverName));
    text_centred(self, 0, 0x29, string_cstr(&text), wnd->width, 0xd, 0x203f66);
    string_free(&text);

    table = g_attendanceTable;
    start = table->start_date;
    end = table->end_date;
    string_format(&text, msg_str(0xd8a), start % 10000 / 100, start % 100, end % 10000 / 100, end % 100);
    text_centred(self, 0, 0x46, string_cstr(&text), wnd->width, 0xc, 0x3950a4);
    string_free(&text);

    text_centred(self, 0x159, 0x121, msg_str(0xd8e), 0x6d, 0xe, 0x595939);
    if (self->days_left > 0) {
        string_format(&text, "%d", self->days_left);
        text_centred(self, 0x170, 0x135, string_cstr(&text), 0x36, 0x16, 0xffffff);
        string_free(&text);
    }
    text_centred(self, 0x1a9, 0x13e, msg_str(0xd8f), 0, 0xc, 0x595939);

    string_init(&line);
    string_init(&text);
    if (self->today_open == 1)
        string_format(&text, msg_str(0xd8c), self->days_checked + 1);
    else
        string_format(&text, msg_str(0xd8d), self->days_checked);
    string_assign(&line, string_cstr(&text), text.size);
    string_free(&text);
    if (line.size)
        draw_today_text(self, &line);
    string_free(&line);

    if (self->today_open == 1) {
        struct BitmapRes *slot = res_mgr_get(g_resMgr(), ATTENDANCE_DIR "bt_slot_a.bmp");

        switch (self->today_state) {
        case ATTENDANCE_TODAY_A:
            slot = res_mgr_get(g_resMgr(), ATTENDANCE_DIR "bt_slot_a.bmp");
            break;
        case ATTENDANCE_TODAY_B:
            slot = res_mgr_get(g_resMgr(), ATTENDANCE_DIR "bt_slot_b.bmp");
            break;
        case ATTENDANCE_TODAY_PRESSED:
            slot = res_mgr_get(g_resMgr(), ATTENDANCE_DIR "bt_slot_press.bmp");
            break;
        }
        ui_window_draw_bitmap(wnd,
                            self->days_checked % ATTENDANCE_SLOTS_PER_ROW * ATTENDANCE_SLOT_PITCH_X +
                                ATTENDANCE_SLOT_X,
                            self->days_checked / ATTENDANCE_SLOTS_PER_ROW * ATTENDANCE_SLOT_PITCH_Y +
                                ATTENDANCE_SLOT_Y,
                            slot, 1);
    }

    for (day = 1; day <= self->day_count; day++) {
        struct ItemInfo info;
        struct AttendanceItem reward;
        int col = (day - 1) % ATTENDANCE_SLOTS_PER_ROW, row = (day - 1) / ATTENDANCE_SLOTS_PER_ROW;

        item_info_construct(&info);
        item_info_set_item_id(&info, check_attendance_reward(g_attendanceTable, &reward, day)->item_id);
        info.count = check_attendance_reward(g_attendanceTable, &reward, day)->count;
        check_attendance_reward(g_attendanceTable, &reward, day);
        ui_window_draw_item_icon_count(wnd, col * ATTENDANCE_SLOT_PITCH_X + 0x2b,
                                   row * ATTENDANCE_SLOT_PITCH_Y + 0x6d, &info, 1, 0);
        string_format(&text, msg_str(0xd8b), day);
        text_centred(self, col * ATTENDANCE_SLOT_PITCH_X + ATTENDANCE_SLOT_X,
                     row * ATTENDANCE_SLOT_PITCH_Y + 0x8c, string_cstr(&text), 0x3a, 0xc, 0xa44f3b);
        string_free(&text);
        item_info_destruct(&info);
    }
}

void ui_check_attendance_wnd_render_overlay(struct UIWindow *window)
{
    struct UICheckAttendanceWnd *self = (struct UICheckAttendanceWnd *)window;
    struct Texture *tex = texture_cache_get(&g_textureCache, ATTENDANCE_DIR "bt_slot_off.tga", 0, 0);
    int i;

    for (i = 0; i < self->days_checked; i++) {
        int row, col;

        if (!tex)
            continue;
        if (i == self->days_checked - 1 && !self->today_open)
            tex = texture_cache_get(&g_textureCache, ATTENDANCE_DIR "bt_slot_complete.tga", 0, 0);
        row = i / ATTENDANCE_SLOTS_PER_ROW;
        col = i - row * ATTENDANCE_SLOTS_PER_ROW;
        renderer_queue_texture_quad((float)(self->frame.base.x + col * ATTENDANCE_SLOT_PITCH_X + ATTENDANCE_SLOT_X),
                                  (float)(ATTENDANCE_SLOT_Y + row * ATTENDANCE_SLOT_PITCH_Y +
                                          self->frame.base.y),
                                  (float)tex->reqWidth, (float)tex->reqHeight, 0xffffffff, tex);
    }
}

static int ui_check_attendance_wnd_hit_slot(struct UICheckAttendanceWnd *self, int x, int y)
{
    int top = ATTENDANCE_SLOT_Y;

    x -= ATTENDANCE_SLOT_X;
    if ((unsigned)x > 0x132 || y < top || y > top + 0xfc)
        return -1;
    if (x % ATTENDANCE_SLOT_PITCH_X > 58)
        return -1;
    y -= top;
    if (y % ATTENDANCE_SLOT_PITCH_Y > 60)
        return -1;
    return y / ATTENDANCE_SLOT_PITCH_Y % 4 * ATTENDANCE_SLOTS_PER_ROW + x / ATTENDANCE_SLOT_PITCH_X % 5;
}

void ui_check_attendance_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UICheckAttendanceWnd *self = (struct UICheckAttendanceWnd *)window;
    if (ui_check_attendance_wnd_hit_slot(self, x, y) == self->days_checked) {
        self->today_state = ATTENDANCE_TODAY_PRESSED;
        ui_window_invalidate_base(&self->frame.base);
    }
}

void ui_check_attendance_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UICheckAttendanceWnd *self = (struct UICheckAttendanceWnd *)window;
    int slot = ui_check_attendance_wnd_hit_slot(self, x, y);

    if (slot == self->days_checked)
        ui_check_attendance_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, ATTENDANCE_ID_CHECK_IN, slot, 0, 0);
}

void ui_check_attendance_wnd_on_process(struct UIWindow *window)
{
    struct UICheckAttendanceWnd *self = (struct UICheckAttendanceWnd *)window;
    int phase;

    if (!self->today_open)
        return;
    phase = platform_ticks_ms() / 100 & 1;
    if (self->today_state != ATTENDANCE_TODAY_PRESSED && self->today_state != phase) {
        self->today_state = phase;
        ui_window_invalidate_base(&self->frame.base);
    }
}

void ui_check_attendance_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UICheckAttendanceWnd *self = (struct UICheckAttendanceWnd *)window;
    int slot = ui_check_attendance_wnd_hit_slot(self, x, y);
    struct ItemInfo info;
    struct AttendanceItem reward;
    const char *name;

    if (slot < 0 || slot >= self->day_count)
        return;
    item_info_construct(&info);
    item_info_set_item_id(&info, check_attendance_reward(g_attendanceTable, &reward, slot + 1)->item_id);
    info.identified = 1;

    name = item_info_get_name(&info);
    if (strlen(name)) {
        int sx = 0, sy = -20;

        ui_window_to_screen(&self->frame.base, &sx, &sy);
        mode_show_tooltip(client_current_mode(), name, sx + x, sy + y, 0xffffffff, 0);
    }
    item_info_destruct(&info);
}

void ui_check_attendance_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UICheckAttendanceWnd *self = (struct UICheckAttendanceWnd *)window;
    int slot = ui_check_attendance_wnd_hit_slot(self, x, y);
    struct UIWindow *item_wnd;
    struct ItemInfo info;
    struct AttendanceItem reward;

    if (slot < 0 || slot >= self->day_count)
        return;
    item_wnd = ui_make_window(ITEM_INFO_WND_ID);
    if (!item_wnd)
        return;
    item_info_construct(&info);
    item_info_set_item_id(&info, check_attendance_reward(g_attendanceTable, &reward, slot + 1)->item_id);
    info.identified = 1;
    ui_window_send_msg(item_wnd, NULL, ITEM_INFO_MSG_SHOW, (int)&info, 0, 0, 0);
    item_info_destruct(&info);
}

int ui_check_attendance_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            int val1, int val2, int val3, int val4)
{
    struct UICheckAttendanceWnd *self = (struct UICheckAttendanceWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == ATTENDANCE_ID_CHECK_IN) {
            if (self->today_open) {
                WORD packet = PACKET_CHECK_ATTENDANCE;
                int len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_CHECK_ATTENDANCE);

                rag_connection_send_packet(rag_connection_instance(), len, &packet);
            }
        } else if (val1 == UI_ID_CLOSE) {
            ui_close_window(ATTENDANCE_WND_ID);
        }
    } else if (msg == ATTENDANCE_MSG_STATE) {
        ui_window_resize_base(&self->frame.base, 0x1e8, 0x19d);
        ui_window_move_base(&self->close->bmp.button.base, 0xaa, 0x178);
        self->days_checked = val1 / 10;
        self->today_open = val1 == self->days_checked * 10;
        if (self->day_count <= self->days_checked)
            self->today_open = 0;
        ui_window_invalidate_base(&self->frame.base);
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
