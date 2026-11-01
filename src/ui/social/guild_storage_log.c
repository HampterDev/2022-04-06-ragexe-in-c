#include "render/rp_pool.h"
#include <stdlib.h>
#include "session/state.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/social/social.h"
#include "core/msg_str.h"
#include "game/guild_storage.h"
#include "diag/perf.h"

#define STORAGE_LOG_WND_ID 0xfd

#define STORAGE_LOG_ID_PREV 0xee
#define STORAGE_LOG_ID_NEXT 0xd2
#define STORAGE_LOG_ID_CLOSE 0xb0

#define STORAGE_LOG_TABS 3
#define STORAGE_LOG_ROWS 10
#define STORAGE_LOG_COLUMNS 4
#define STORAGE_LOG_ROW_TOP 0x3c
#define STORAGE_LOG_ROW_HEIGHT 0xf

#define MSG_STORAGE_LOG_TITLE 0x9f3
#define MSG_STORAGE_LOG_TAB_FIRST 0x9f0
#define MSG_STORAGE_LOG_NOTE 0x9f4
#define MSG_STORAGE_LOG_HEAD_ICON 0x9ee
#define MSG_STORAGE_LOG_HEAD_0 0x9ea
#define MSG_STORAGE_LOG_HEAD_1 0x9eb
#define MSG_STORAGE_LOG_HEAD_2 0x9ec
#define MSG_STORAGE_LOG_HEAD_3 0x9ed

#define MSG_STORAGE_LOG_TAB 0x16
#define MSG_STORAGE_LOG_CLEAR 0x3c

#define BASIC_INTERFACE UI_SKIN "basic_interface\\"

struct UIGuild_Storage_Log {
    struct UIFrameWnd frame;
    struct UIWindow *texts[STORAGE_LOG_COLUMNS][STORAGE_LOG_ROWS];
    int tab;
    struct UIWindow *tabs;
    unsigned page;
    int pages[STORAGE_LOG_TABS];
    unsigned index;
    unsigned row;
    int y;
    unsigned count[STORAGE_LOG_TABS];
    struct Vec in_flags;
};
OFFSET_ASSERT(UIGuild_Storage_Log, tab, 0x134);
OFFSET_ASSERT(UIGuild_Storage_Log, in_flags, 0x164);
SIZE_ASSERT(UIGuild_Storage_Log, 0x170);
void ui_guild_storage_log_destroy(struct UIWindow *window);
void ui_guild_storage_log_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_guild_storage_log_on_draw(struct UIWindow *self);
int ui_guild_storage_log_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_guild_storage_log_ops = {
    .destroy = ui_guild_storage_log_destroy,
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
    .on_create = ui_guild_storage_log_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_guild_storage_log_on_draw,
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
    .send_msg = ui_guild_storage_log_send_msg,
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

static const struct Vec *storage_log(int tab)
{
    if (tab == 1)
        return &g_session1.guildStorageLogPut;
    if (tab == 2)
        return &g_session1.guildStorageLogTaken;
    return &g_session1.guildStorageLog;
}

struct UIGuild_Storage_Log *ui_guild_storage_log_construct(struct UIGuild_Storage_Log *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_guild_storage_log_ops;
    self->in_flags.first = self->in_flags.last = self->in_flags.end = NULL;
    self->tab = 0;
    self->index = 0;
    self->row = 1;
    self->y = 0;
    for (i = 0; i < STORAGE_LOG_TABS; i++)
        self->count[i] = VEC_COUNT(storage_log(i), struct GuildStorageLogEntry);
    self->page = 1;
    for (i = 0; i < STORAGE_LOG_TABS; i++)
        self->pages[i] = ((int)self->count[i] + 9) / STORAGE_LOG_ROWS;
    return self;
}

struct UIWindow *ui_guild_storage_log_new(void)
{
    return (struct UIWindow *)ui_guild_storage_log_construct(mem_alloc(sizeof(struct UIGuild_Storage_Log)));
}

static void ui_guild_storage_log_destruct(struct UIGuild_Storage_Log *self)
{
    int i;

    self->frame.base.ops = &ui_guild_storage_log_ops;
    self->tab = 0;
    for (i = 0; i < STORAGE_LOG_TABS; i++)
        guild_log_clear(&(*(struct Vec (*)[3])&g_session1.guildStorageLog)[i]);
    vec_clear(&self->in_flags);
    self->index = 0;
    self->row = 1;
    self->y = 0;
    self->page = 1;
    for (i = 0; i < STORAGE_LOG_TABS; i++)
        self->pages[i] = 1;
    vec_free(&self->in_flags);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_guild_storage_log_destroy(struct UIWindow *window)
{
    struct UIGuild_Storage_Log *self = (struct UIGuild_Storage_Log *)window;
    ui_guild_storage_log_destruct(self);
    mem_free(self);
}

void ui_guild_storage_log_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIGuild_Storage_Log *self = (struct UIGuild_Storage_Log *)window;
    static const struct {
        int x, width;
    } column[STORAGE_LOG_COLUMNS] = { { 0x32, 0x64 }, { 0x96, 0x14 }, { 0xbe, 0x50 }, { 0x118, 0x82 } };
    static const struct {
        const char *normal, *hover, *pressed;
        int x, y, id;
    } button[3] = {
        { BASIC_INTERFACE "stickon.bmp", BASIC_INTERFACE "stickon_a.bmp", BASIC_INTERFACE "stickon_b.bmp", 0xa0,
          0xe5, STORAGE_LOG_ID_PREV },
        { BASIC_INTERFACE "stickoff.bmp", BASIC_INTERFACE "stickoff_a.bmp", BASIC_INTERFACE "stickoff_b.bmp", 0xd7,
          0xe5, STORAGE_LOG_ID_NEXT },
        { BASIC_INTERFACE "btn_close.bmp", BASIC_INTERFACE "btn_close_a.bmp", BASIC_INTERFACE "btn_close_b.bmp",
          0x15e, 0xe1, STORAGE_LOG_ID_CLOSE },
    };
    struct UIWindow *wnd = &self->frame.base;
    int row, c, i;

    for (row = 0; row < STORAGE_LOG_ROWS; row++) {
        for (c = 0; c < STORAGE_LOG_COLUMNS; c++) {
            struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

            if (text)
                text = ui_static_text_construct(text);
            self->texts[c][row] = text;
            ui_window_create(text, column[c].width, 0xf);
            ui_static_text_set_text_ellipsis(text, "");
            ui_window_move(text, column[c].x, STORAGE_LOG_ROW_TOP + STORAGE_LOG_ROW_HEIGHT * row);
            ui_window_add_child(wnd, text);
        }
    }

    for (i = 0; i < 3; i++)
        ui_add_bitmap_button(wnd, button[i].normal, button[i].hover, button[i].pressed, button[i].x, button[i].y,
                             button[i].id);

    self->tabs = mem_alloc(sizeof(struct UITabControl));
    if (self->tabs)
        self->tabs = ui_tab_control_construct(self->tabs);
    ui_tab_control_setup(self->tabs, 1, 0x14, 0x32, 1, 0, 0, 0, 0, 0, 0, 0, 0);
    ui_window_move(self->tabs, 0, 0x11);
    ui_window_add_child(wnd, self->tabs);
    for (i = 0; i < STORAGE_LOG_TABS; i++)
        ui_tab_control_add_tab(self->tabs, msg_str(MSG_STORAGE_LOG_TAB_FIRST + i));
}

static void draw_text(struct UIWindow *wnd, int x, int y, const char *text)
{
    ui_window_text_out(wnd, x, y, text, 0, 0, 0xc, 0, 0, 0);
}

static void storage_log_fill(struct UIGuild_Storage_Log *self)
{
    struct UIWindow *wnd = &self->frame.base;
    int t = self->tab == 0 ? 0 : self->tab == 1 ? 1 : 2;
    const struct Vec *log = storage_log(t);
    const struct GuildStorageLogEntry *first = log->first, *e;
    char page[10] = { 0 };
    char number[0x100];

    if (t != 0 && self->pages[t] == 0)
        sprintf(page, "%02d / 01", self->page);
    else
        sprintf(page, "%02d / %02d", self->page, self->pages[t]);
    draw_text(wnd, 0xaf, 0xe4, page);

    memset(number, 0, 0xff);
    for (e = first; e != (const struct GuildStorageLogEntry *)log->last; e++) {
        const struct GuildStorageLogEntry *line;
        int r, c, in;

        if (self->row >= STORAGE_LOG_ROWS + 1 || self->index >= self->count[t])
            break;
        line = first + self->index;
        in = line->action;
        vec_push(&self->in_flags, &in, sizeof in);
        _itoa(line->amount, number, 10);
        r = (int)self->row - 1;
        for (c = 0; c < STORAGE_LOG_COLUMNS; c++)
            ui_static_text_set_text_colour(self->texts[c][r], line->attribute == 1 ? 0xff : 0, 0, 0);
        ui_static_text_set_text_ellipsis(self->texts[0][r], string_cstr(&line->item_name));
        ui_static_text_set_text_ellipsis(self->texts[1][r], number);
        ui_static_text_set_text_ellipsis(self->texts[2][r], line->name);
        ui_static_text_set_text_ellipsis(self->texts[3][r], line->time);
        self->y += STORAGE_LOG_ROW_HEIGHT;
        self->row++;
        self->index++;
    }
}

void ui_guild_storage_log_on_draw(struct UIWindow *window)
{
    struct UIGuild_Storage_Log *self = (struct UIGuild_Storage_Log *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UITabControl *tabs = (struct UITabControl *)self->tabs;
    void *out, *in;
    unsigned i;
    int y;

    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_STORAGE_LOG_TITLE), 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    out = res_mgr_get(g_resMgr(), ui_skin_path(BASIC_INTERFACE "out.bmp"));
    in = res_mgr_get(g_resMgr(), ui_skin_path(BASIC_INTERFACE "in.bmp"));

    for (i = 0; i < VEC_COUNT(&tabs->names, struct String); i++) {
        ((DWORD *)tabs->colours.first)[i] = 0xffffffff;
        ui_window_invalidate(self->tabs);
    }
    ((DWORD *)tabs->colours.first)[self->tab] = 0xc89623;
    ui_window_invalidate(self->tabs);

    ui_window_text_out(wnd, 0xea, 0x17, msg_str(MSG_STORAGE_LOG_NOTE), 0, 0, 0xb, 0xff, 0, 0);
    draw_text(wnd, 6, 0x2f, msg_str(MSG_STORAGE_LOG_HEAD_ICON));
    draw_text(wnd, 0x46, 0x2f, msg_str(MSG_STORAGE_LOG_HEAD_0));
    draw_text(wnd, 0x96, 0x2f, msg_str(MSG_STORAGE_LOG_HEAD_1));
    draw_text(wnd, 0xc8, 0x2f, msg_str(MSG_STORAGE_LOG_HEAD_2));
    draw_text(wnd, 0x140, 0x2f, msg_str(MSG_STORAGE_LOG_HEAD_3));

    storage_log_fill(self);

    y = STORAGE_LOG_ROW_TOP;
    for (i = 0; i < VEC_COUNT(&self->in_flags, int); i++, y += STORAGE_LOG_ROW_HEIGHT)
        ui_window_draw_bitmap(wnd, 6, y, ((int *)self->in_flags.first)[i] == 1 ? in : out, 1);
}

static void storage_log_restart(struct UIGuild_Storage_Log *self)
{
    self->row = 1;
    self->y = 0;
    vec_clear(&self->in_flags);
}

static void storage_log_clear(struct UIGuild_Storage_Log *self)
{
    ui_guild_storage_log_send_msg(&self->frame.base, &self->frame.base, MSG_STORAGE_LOG_CLEAR, 0, 0, 0, 0);
}

int ui_guild_storage_log_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    struct UIGuild_Storage_Log *self = (struct UIGuild_Storage_Log *)window;
    int r, c;

    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case STORAGE_LOG_ID_CLOSE:
            ui_window_mgr_post_quit1(&g_windowMgr, STORAGE_LOG_WND_ID);
            break;
        case STORAGE_LOG_ID_NEXT: {
            int t = self->tab == 0 ? 0 : self->tab == 1 ? 1 : 2;

            if (self->page < (unsigned)self->pages[t]) {
                self->page++;
                storage_log_restart(self);
                storage_log_clear(self);
            }
            break;
        }
        case STORAGE_LOG_ID_PREV:
            if (self->page > 1) {
                if (self->row == STORAGE_LOG_ROWS + 1)
                    self->index -= 2 * STORAGE_LOG_ROWS;
                else
                    self->index -= self->index % STORAGE_LOG_ROWS + STORAGE_LOG_ROWS;
                storage_log_restart(self);
                self->page = self->page - 1 >= 1 ? self->page - 1 : 1;
                storage_log_clear(self);
            }
            break;
        }
        return 0;
    case MSG_STORAGE_LOG_TAB:
        self->tab = val1;
        self->page = 1;
        self->index = 0;
        storage_log_restart(self);
        storage_log_clear(self);
        return 0;
    case MSG_STORAGE_LOG_CLEAR:
        for (r = 0; r < STORAGE_LOG_ROWS; r++)
            for (c = 0; c < STORAGE_LOG_COLUMNS; c++)
                ui_static_text_set_text(self->texts[c][r], "", 0);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    }
    return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
}
