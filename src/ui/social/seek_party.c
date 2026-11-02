#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/social/social.h"
#include "ui/character/character.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/social/seek_party.h"
#include "platform/clock.h"

#define JOB_LIST_WND_ID 0xaa

#define SEEK_PARTY_ID_CANCEL UI_ID_CLOSE
#define SEEK_PARTY_ID_MINIMISE 0xc2
#define SEEK_PARTY_ID_REWRITE 0x114

#define MSG_SEEK_PARTY_TITLE 0x6da
#define SEEK_PARTY_LEVEL_SPAN 0xf

#define CZ_SEEK_PARTY_CANCEL 0x806

#define SEEK_PARTY_MINIMISED_HEIGHT 0x2c
#define SEEK_PARTY_FRAME_MS 1000

#define UI_MSG_REBUILD 0x17

struct UISeekPartyWnd {
    struct UIFrameWnd frame;
    struct UIWindow *title;
    struct UIBitmapButton *cancel;
    struct UIBitmapButton *rewrite;
    int frame_no;
    int type;
    unsigned frame_ms;
    unsigned clock_ms;
    unsigned since_frame;
    DWORD frame_tick;
    int full_height;
    BYTE minimised;
    struct WindowInfo *info;
};
void ui_seek_party_wnd_destroy(struct UIWindow *window);
void ui_seek_party_wnd_store_info(struct UIWindow *self);
void ui_seek_party_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_seek_party_wnd_on_process(struct UIWindow *self);
void ui_seek_party_wnd_on_draw(struct UIWindow *self);
intptr_t ui_seek_party_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_seek_party_wnd_ops = {
    .destroy = ui_seek_party_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_seek_party_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_seek_party_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_seek_party_wnd_on_process,
    .on_draw = ui_seek_party_wnd_on_draw,
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
    .send_msg = ui_seek_party_wnd_send_msg,
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

extern int(*_imp__rand)(void);

struct UISeekPartyWnd *ui_seek_party_wnd_construct(struct UISeekPartyWnd *self)
{
    DWORD start;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_seek_party_wnd_ops;
    self->title = NULL;
    self->cancel = NULL;
    self->frame_no = 1;
    self->type = 0;
    self->frame_ms = SEEK_PARTY_FRAME_MS;
    self->clock_ms = 0;
    self->since_frame = 0;
    self->frame_tick = 0;
    self->full_height = 0;
    self->minimised = 0;
    start = ((DWORD)g_session1.partyBooking.answer_tick);
    self->clock_ms = platform_coarse_ticks_ms() - start;
    return self;
}

struct UIWindow *ui_seek_party_wnd_new(void)
{
    return (struct UIWindow *)ui_seek_party_wnd_construct(mem_alloc(sizeof(struct UISeekPartyWnd)));
}

static void ui_seek_party_wnd_destruct(struct UISeekPartyWnd *self)
{
    self->frame.base.ops = &ui_seek_party_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_seek_party_wnd_destroy(struct UIWindow *window)
{
    struct UISeekPartyWnd *self = (struct UISeekPartyWnd *)window;
    ui_seek_party_wnd_destruct(self);
    mem_free(self);
}

void ui_seek_party_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UISeekPartyWnd *self = (struct UISeekPartyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    char title[0x80];
    int high;

    memset(title, 0, sizeof title);
    high = g_session1.partyBookingSearchLevel + SEEK_PARTY_LEVEL_SPAN;
    if (high > g_session1.baseLevel3rd)
        high = g_session1.baseLevel3rd;
    sprintf(title, "%s - %d~%d", msg_str(MSG_SEEK_PARTY_TITLE), g_session1.partyBookingSearchLevel, high);
    self->title = mem_alloc(sizeof(struct UIStaticText));
    if (self->title)
        self->title = ui_static_text_construct(self->title);
    ui_window_create(self->title, wnd->width - 0x20, 0xf);
    ui_static_text_set_text_ellipsis(self->title, title);
    ui_window_move(self->title, 0x10, 2);
    ((struct UIStaticText *)self->title)->white_shadow = 1;
    ui_window_add_child(wnd, self->title);

    ui_window_add_close_button(wnd, "", 0);
    ui_window_add_mini_button(wnd, "", 0);

    self->cancel = ui_add_bitmap_button(wnd, UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp",
                                        UI_SKIN "btn_cancel_b.bmp", wnd->width - 0x2e, wnd->height - 0x18,
                                        SEEK_PARTY_ID_CANCEL);
    self->rewrite = ui_add_bitmap_button(wnd, UI_SKIN "btn_rewrite.bmp", UI_SKIN "btn_rewrite_a.bmp",
                                         UI_SKIN "btn_rewrite_b.bmp", wnd->width - 0x5c, wnd->height - 0x18,
                                         SEEK_PARTY_ID_REWRITE);

    self->type = _imp__rand() % 5;
    self->frame_ms = SEEK_PARTY_FRAME_MS;
}

void ui_seek_party_wnd_on_process(struct UIWindow *window)
{
    struct UISeekPartyWnd *self = (struct UISeekPartyWnd *)window;
    static const int last_frame[6] = { 5, 5, 0x11, 0xb, 0xb, 0xb };
    DWORD now = platform_coarse_ticks_ms();

    self->since_frame = now - self->frame_tick;
    if (self->since_frame <= self->frame_ms)
        return;
    self->frame_no = self->frame_no < last_frame[self->type] ? self->frame_no + 1 : 0;
    self->clock_ms += self->frame_ms;
    self->frame_tick = now;
    ui_window_invalidate_base(&self->frame.base);
}

void ui_seek_party_wnd_on_draw(struct UIWindow *window)
{
    struct UISeekPartyWnd *self = (struct UISeekPartyWnd *)window;
    static const char *const names[6] = { "bibibic", "bibibic", "달기린", "달기린",
                                          "boya", "boya" };
    struct UIWindow *wnd = &self->frame.base;
    int r, g, b;
    char clock[10];
    unsigned seconds;

    ui_window_draw_title_bar(wnd, 1, "", 0);
    ui_skin_color_chip(&g_skinMgr, 2, 2, &r, &g, &b);
    ui_window_fill_rect(wnd, 0, 0x11, wnd->width, wnd->height - 0x26, RGB(r, g, b));
    if (!self->minimised) {
        struct String path, formatted;

        string_init(&path);
        string_init(&formatted);
        string_format(&formatted, UI_SKIN "basic_interface\\seekparty\\type_0%d\\seekparty_%s_0%d_%d.bmp", self->type,
                   names[self->type], self->type, self->frame_no);
        string_free(&path);
        string_move(&path, &formatted);
        if (res_mgr_is_available(g_resMgr(), string_cstr(&path)))
            ui_window_draw_bitmap(wnd, 1, 0x12,
                                res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&path))), 0);
        string_free(&path);
    }
    ui_window_draw_button_bar(wnd, 2, 0, 1);

    memset(clock, 0, sizeof clock);
    seconds = self->clock_ms / 1000;
    sprintf(clock, "%02d:%02d:%02d", seconds / 3600 % 60, seconds / 60 % 60, seconds % 60);
    ui_window_text_out(wnd, 0xa, wnd->height - 0x14, clock, 0, 0, 0xc, 0, 0, 0);
}

static int seek_party_info_ok(const struct WindowInfo *info)
{
    if (info->width < 0x98 || info->height < 0xc7)
        return 0;
    if (info->x < 0 || info->x > g_renderer->width - info->width)
        return 0;
    if (info->y < 0 || info->y > g_renderer->height - info->height)
        return 0;
    return 1;
}

static void seek_party_info_reset(struct WindowInfo *info)
{
    info->x = 0x1fe;
    info->y = 0x168;
    info->width = 0x98;
    info->height = 0xc7;
}

void ui_seek_party_wnd_store_info(struct UIWindow *window)
{
    struct UISeekPartyWnd *self = (struct UISeekPartyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}

intptr_t ui_seek_party_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                      intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UISeekPartyWnd *self = (struct UISeekPartyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (msg == UI_MSG_SET_INFO) {
        self->info = (struct WindowInfo *)val1;
        if (!seek_party_info_ok(self->info))
            seek_party_info_reset(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        ui_seek_party_wnd_send_msg(&self->frame.base, NULL, UI_MSG_RESIZE, 3, self->info->width, self->info->height, 0);
        ui_seek_party_wnd_send_msg(&self->frame.base, NULL, UI_MSG_REBUILD, 0, 0, 0, 0);
        return 0;
    }
    if (msg == UI_MSG_COMMAND) {
        if (val1 == SEEK_PARTY_ID_CANCEL) {
            short packet = CZ_SEEK_PARTY_CANCEL;

            ui_send_packet(&packet);
            ui_window_mgr_post_quit1(&g_windowMgr, JOB_LIST_WND_ID);
        } else if (val1 == SEEK_PARTY_ID_MINIMISE) {
            if (self->minimised) {
                ui_window_resize_base(&self->frame.base, wnd->width, self->full_height);
            } else {
                self->full_height = wnd->height;
                ui_window_resize_base(&self->frame.base, wnd->width, SEEK_PARTY_MINIMISED_HEIGHT);
            }
            self->minimised = !self->minimised;
            ui_window_move_base(&self->cancel->button.base, wnd->width - 0x2e, wnd->height - 0x18);
            ui_window_move_base(&self->rewrite->button.base, wnd->width - 0x5c, wnd->height - 0x18);
            ui_window_invalidate_base(&self->frame.base);
        } else if (val1 == SEEK_PARTY_ID_REWRITE) {
            struct UIWindow *job_list;

            ui_make_window(JOB_LIST_WND_ID);
            job_list = ui_window_find(JOB_LIST_WND_ID);
            if (job_list)
                ui_job_list_wnd_show_jobs((struct UIJobListWnd *)job_list);
        }
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
