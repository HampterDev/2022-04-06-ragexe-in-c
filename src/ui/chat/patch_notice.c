#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "contents/sound.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include <string.h>
#include "ui/chat/chat.h"
#include "diag/perf.h"

struct UINoticeWnd;
struct UIWindow;
void ui_notice_wnd_destroy(struct UIWindow *window);
void ui_notice_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_notice_wnd_on_destroy(struct UIWindow *self);
void ui_notice_wnd_on_draw(struct UIWindow *self);
void ui_notice_wnd_on_mouse_hover(struct UIWindow *self, int unused1, int unused2);
void ui_notice_wnd_on_wheel(struct UIWindow *self, int delta);
intptr_t ui_notice_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_notice_wnd_ops = {
    .destroy = ui_notice_wnd_destroy,
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
    .on_create = ui_notice_wnd_on_create,
    .on_destroy = ui_notice_wnd_on_destroy,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_notice_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_notice_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_notice_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_notice_wnd_send_msg,
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

#define NOTICE_PATCH_FILE "notice_patch.txt"
#define NOTICE_CHECK_FILE "notice_check.txt"
#define NOTICE_READ_FILE "notice_read.txt"
#define NOTICE_PATCH_PLACEHOLDER "공지사항파일없음"
#define NOTICE_CHECK_PLACEHOLDER "없으니만들자"
#define NOTICE_READ_ALL 0xff
#define NOTICE_COMPARE_LIMIT 1000

#define NOTICE_LINES 19
#define NOTICE_PAGE_MAX 10
#define NOTICE_ID_BACK 0xee
#define NOTICE_ID_NEXT 0xd2
#define SOUND_BUTTON "버튼소리.wav"

struct UINoticeWnd {
    struct UIFrameWnd frame;
    struct UIWindow *line[NOTICE_LINES];
    int line_count;
    int changed_count;
    const char *file;
    int last_page;
    int page;
    int page_first[NOTICE_PAGE_MAX];
    int page_end[NOTICE_PAGE_MAX];
    int top;
    struct UIScrollBar *scroll;
};

struct UINoticeWnd *ui_notice_wnd_construct(struct UINoticeWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_notice_wnd_ops;
    for (i = 0; i < NOTICE_LINES; i++) {
        struct UIWindow *line = mem_alloc(sizeof(struct UIStaticText));

        self->line[i] = line ? ui_static_text_construct(line) : NULL;
    }
    self->line_count = 20;
    self->changed_count = 0;
    self->last_page = 0;
    self->page = 0;
    self->page_first[0] = 0;
    self->page_end[0] = 0;
    self->top = 0;
    ui_window_mgr_set_focus_edit(&g_windowMgr, &self->frame.base);
    return self;
}

struct UIWindow *ui_notice_wnd_new(void)
{
    return (struct UIWindow *)ui_notice_wnd_construct(mem_alloc(sizeof(struct UINoticeWnd)));
}

void ui_notice_wnd_destroy(struct UIWindow *window)
{
    struct UINoticeWnd *self = (struct UINoticeWnd *)window;
    self->frame.base.ops = &ui_notice_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

static char *read_file(FILE *file, long *size)
{
    char *buf;

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    buf = mem_alloc(*size + 1);
    fseek(file, 0, SEEK_SET);
    fread(buf, *size, 1, file);
    buf[*size] = 0;
    fclose(file);
    return buf;
}

static BYTE read_mark(void)
{
    FILE *file = fopen(NOTICE_READ_FILE, "rb");
    BYTE mark;

    fread(&mark, 1, 1, file);
    fclose(file);
    return mark;
}

static void write_mark(BYTE mark)
{
    FILE *file = fopen(NOTICE_READ_FILE, "wb");

    fwrite(&mark, 1, 1, file);
    fclose(file);
}

static void notice_show_lines(struct UINoticeWnd *self)
{
    int start[10000];
    char *text;
    long size, i;
    int page_break = 0, count = 1, shown;

    {
        int lines = self->page_end[self->page] - self->page_first[self->page];

        ui_scroll_bar_set_range(self->scroll, lines - NOTICE_LINES, NOTICE_LINES, lines);
        if (self->page_end[self->page] - self->page_first[self->page] < NOTICE_LINES)
            ui_scroll_bar_set_range(self->scroll, 0, NOTICE_LINES, NOTICE_LINES);
    }
    ui_scroll_bar_set_pos(self->scroll, self->top - self->page_first[self->page]);
    ui_window_invalidate_base(&self->scroll->base);

    text = read_file(fopen(self->file, "r"), &size);
    start[0] = 0;
    memset(&start[1], 0, 0x270b);
    for (i = 0; i < size; i++) {
        switch (text[i]) {
        case '\n':
            if (page_break == 0) {
                start[count++] = i + 1;
                text[i] = 0;
            }
            break;
        case '\r':
            text[i] = 0;
            break;
        case '^':
            if (page_break == 0 && count > self->page_first[self->page]) {
                page_break = count - 1;
                if (count < 0)
                    count = 0;
            }
            break;
        }
    }
    if (size > 0 && page_break > 0)
        count = page_break - self->page_first[self->page];
    shown = count - 1;
    if (shown > NOTICE_LINES)
        shown = NOTICE_LINES;

    for (i = 0; i < NOTICE_LINES; i++) {
        if (i < shown)
            ui_static_text_set_text(self->line[i], text + start[self->top + i], 1);
        else
            ui_static_text_set_text(self->line[i], "", 0);
    }
    ui_window_invalidate_base(&self->frame.base);
}

void ui_notice_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UINoticeWnd *self = (struct UINoticeWnd *)window;
    static const char *const bitmaps[3][3] = {
        { UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp" },
        { UI_SKIN "btn_back.bmp", UI_SKIN "btn_back_a.bmp", UI_SKIN "btn_back_b.bmp" },
        { UI_SKIN "btn_next.bmp", UI_SKIN "btn_next_a.bmp", UI_SKIN "btn_next_b.bmp" },
    };
    static const int place[3][2] = { { 0x216, 0x178 }, { 0xf4, 0x178 }, { 0x126, 0x178 } };
    static const int ids[3] = { UI_ID_OK, NOTICE_ID_BACK, NOTICE_ID_NEXT };
    struct UIWindow *wnd = &self->frame.base;
    FILE *file, *check_file;
    char *notice, *check, *copy;
    long notice_size, check_size, copy_size, i;
    int changed, line_start[21], count, y;

    for (i = 0; i < 3; i++) {
        struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));
        int state;

        if (button)
            button = ui_bitmap_button_construct(button);
        for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++)
            ui_bitmap_button_set_bitmap(button, bitmaps[i][state], state, 0);
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, place[i][0], place[i][1]);
        button->button.base.id = ids[i];
        ui_window_add_child(wnd, &button->button.base);
    }
    UI_FRAME_DEFAULT_ID(&self->frame) = UI_ID_OK;
    self->file = NOTICE_PATCH_FILE;

    if (fopen(NOTICE_PATCH_FILE, "r") == NULL) {
        file = fopen(self->file, "w");
        fwrite(NOTICE_PATCH_PLACEHOLDER, 0x10, 1, file);
        fclose(file);
        ui_window_mgr_post_quit2(&g_windowMgr, wnd);
    }
    if (fopen(NOTICE_CHECK_FILE, "r") == NULL) {
        file = fopen(NOTICE_CHECK_FILE, "w");
        fwrite(NOTICE_CHECK_PLACEHOLDER, 0xc, 1, file);
        fclose(file);
    }
    if (fopen(NOTICE_READ_FILE, "r") == NULL)
        write_mark(0);

    file = fopen(self->file, "r");
    check_file = fopen(NOTICE_CHECK_FILE, "r");
    fseek(file, 0, SEEK_END);
    notice_size = ftell(file);
    fseek(check_file, 0, SEEK_END);
    check_size = ftell(check_file);
    changed = check_size == 0 ? NOTICE_COMPARE_LIMIT : 0;
    notice = mem_alloc(notice_size + 1);
    check = mem_alloc(check_size + 1);
    fseek(file, 0, SEEK_SET);
    fread(notice, notice_size, 1, file);
    notice[notice_size] = 0;
    fclose(file);
    fseek(check_file, 0, SEEK_SET);
    fread(check, check_size, 1, check_file);
    check[check_size] = 0;
    fclose(check_file);

    line_start[0] = 0;
    count = 1;
    self->line_count = 0;
    for (i = 0; i < notice_size; i++) {
        switch (notice[i]) {
        case '\n':
            line_start[count] = i + 1;
            notice[i] = 0;
            if (++count > 20)
                count = 20;
            self->line_count++;
            break;
        case '\r':
            notice[i] = 0;
            break;
        case '^':
            self->page_end[self->last_page] = self->line_count - 1;
            self->last_page++;
            self->page_first[self->last_page] = self->line_count + 1;
            count = count - 1 < 0 ? 0 : count - 1;
            break;
        default:
            if (i < check_size && i < NOTICE_COMPARE_LIMIT && notice[i] != check[i])
                changed++;
            break;
        }
    }
    if (count > NOTICE_LINES)
        count = NOTICE_LINES;
    self->page_end[self->last_page] = self->line_count;

    if (!changed) {
        if (read_mark() == NOTICE_READ_ALL)
            ui_window_mgr_post_quit2(&g_windowMgr, wnd);
    } else {
        write_mark(0);
    }

    for (i = 0, y = 0x16; y < 0x16c; i++, y += 0x12) {
        ui_window_create(self->line[i], 0x21c, 0x10);
        ui_static_text_set_font(self->line[i], 0, 0x10, 0, 0);
        ui_static_text_set_text(self->line[i], i < count ? notice + line_start[i] : "", 1);
        ui_window_move(self->line[i], 0x1e, y);
        ui_window_add_child(wnd, self->line[i]);
    }

    {
        struct UIScrollBar *scroll = mem_alloc(sizeof(struct UIScrollBar));
        int lines;

        self->scroll = scroll ? ui_scroll_bar_construct(scroll) : NULL;
        ui_scroll_bar_create(self->scroll, 0x247, 0x12, 1, wnd->height - 0x30);
        lines = self->page_end[self->page] - self->page_first[self->page];
        ui_scroll_bar_set_range(self->scroll, lines - NOTICE_LINES, NOTICE_LINES, lines);
        if (self->page_end[self->page] - self->page_first[self->page] < NOTICE_LINES)
            ui_scroll_bar_set_range(self->scroll, 0, NOTICE_LINES, NOTICE_LINES);
        ui_scroll_bar_set_pos(self->scroll, self->top);
        ui_window_add_child(wnd, (struct UIWindow *)self->scroll);
    }

    file = fopen(self->file, "r");
    check_file = fopen(NOTICE_CHECK_FILE, "w");
    fseek(file, 0, SEEK_END);
    fseek(check_file, 0, SEEK_END);
    copy_size = ftell(file);
    copy = mem_alloc(copy_size + 1);
    fseek(file, 0, SEEK_SET);
    fread(copy, copy_size, 1, file);
    copy[copy_size] = 0;
    fclose(file);
    for (i = 0; i < copy_size; i++)
        fwrite(copy + i, 1, 1, check_file);
    fclose(check_file);

    if (!changed) {
        BYTE mark = read_mark();

        if (mark != NOTICE_READ_ALL)
            self->top = mark;
        if (self->top > self->page_end[self->page] - NOTICE_LINES)
            self->top = self->page_end[self->page] - NOTICE_LINES;
        if (self->top < 0)
            self->top = 0;
        notice_show_lines(self);
    }
}

void ui_notice_wnd_on_destroy(struct UIWindow *self)
{
}

void ui_notice_wnd_on_draw(struct UIWindow *window)
{
    struct UINoticeWnd *self = (struct UINoticeWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int body, strips, i, y, green, blue;
    int red = 0;
    struct BitmapRes *res;
    COLORREF colour;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (wnd->height == 0x11)
        return;
    body = wnd->height - 0x26;
    strips = body / 8 + (body % 8 != 0);
    for (i = 0, y = 0x11; i < strips; i++, y += 8)
        ui_window_draw_bitmap(wnd, 0, y, ui_skin_bitmap(UI_SKIN "basic_interface\\itemwin_left.bmp"), 0);
    body = wnd->height - 0x26;
    strips = body / 8 + (body % 8 != 0);
    for (i = 0, y = 0x11; i < strips; i++, y += 8) {
        res = ui_skin_bitmap(UI_SKIN "basic_interface\\itemwin_right.bmp");
        ui_window_draw_bitmap(wnd, wnd->width - res->width, y, res, 0);
    }

    ui_skin_color_chip(&g_skinMgr, 2, 2, &green, &green, &blue);
    colour = RGB(red, green, blue);
    ui_window_fill_rect(wnd, 0x14, 0x11, wnd->width - 0x28, wnd->height - 0x26, colour);
    body = wnd->height - 0x26;
    ui_window_fill_rect(wnd, 0x14, body / 32 * 32 + 0x11, wnd->width - 0x18, body % 32, colour);
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

void ui_notice_wnd_on_mouse_hover(struct UIWindow *self, int unused1, int unused2)
{
}

void ui_notice_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UINoticeWnd *self = (struct UINoticeWnd *)window;
    ui_notice_wnd_send_msg(&self->frame.base, (struct UIWindow *)self->scroll, UI_MSG_SCROLL_V, delta < 0 ? 6 : -6, 0, 0, 0);
}

static BYTE notice_read_mark(struct UINoticeWnd *self)
{
    int last_top = self->page_end[self->page] - NOTICE_LINES;
    BYTE mark;

    if (self->page > 0 || self->top > 0xff)
        return NOTICE_READ_ALL;
    mark = (BYTE)self->top;
    if (mark == last_top)
        return NOTICE_READ_ALL;
    if (mark == NOTICE_READ_ALL)
        return mark;
    if (self->top + NOTICE_LINES <= last_top)
        return (BYTE)(self->top + NOTICE_LINES);
    return last_top < 0 ? 0 : (BYTE)last_top;
}

intptr_t ui_notice_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                   intptr_t val3, intptr_t val4)
{
    struct UINoticeWnd *self = (struct UINoticeWnd *)window;
    int old_top, top;

    switch (msg) {
    case 0:
        ui_notice_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, UI_ID_OK, 0, 0, 0);
        return 0;
    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_OK: {
            FILE *file;
            BYTE mark;

            sound_play_wave(SOUND_BUTTON, 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
            file = fopen(NOTICE_READ_FILE, "wb");
            mark = notice_read_mark(self);
            fwrite(&mark, 1, 1, file);
            fclose(file);
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_NOTICE);
            break;
        }
        case NOTICE_ID_NEXT:
            if (++self->page > self->last_page) {
                self->page = self->last_page;
                return 0;
            }
            self->top = self->page_first[self->page];
            notice_show_lines(self);
            break;
        case NOTICE_ID_BACK:
            if (--self->page < 0) {
                self->page = 0;
                return 0;
            }
            self->top = self->page_first[self->page];
            notice_show_lines(self);
            break;
        }
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        old_top = self->top;
        top = old_top + (msg == UI_MSG_PAGE_UP ? -1 : msg == UI_MSG_PAGE_DOWN ? 1 : val1);
        self->top = top;
        if (top > self->page_end[self->page] - NOTICE_LINES) {
            self->top = self->page_end[self->page] - NOTICE_LINES;
            top = self->top;
        }
        if (top < self->page_first[self->page])
            self->top = self->page_first[self->page];
        notice_show_lines(self);
        return self->top - old_top;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
