#include "render/rp_pool.h"
#include "session/character_name.h"
#include "contents/sound.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include <string.h>
#include "ui/social/seek_party.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "ui/wndmgr/wndmgr.h"

#define SEEK_PARTY_LIST_WND_ID 0xa9
#define SEEK_PARTY_MB_WND_ID 0xa8
#define PARTY_BOOKING_HELP_WND_ID 0xad

#define LIST_ID_CLOSE UI_ID_CLOSE
#define LIST_ID_MINIMISE 0xc2
#define LIST_ID_REDRAW 0x114
#define LIST_ID_HELP 0x14f
#define LIST_ID_FIND 0x165
#define LIST_ID_FORM 0x172
#define LIST_ID_MORE 0x174
#define LIST_ID_WHISPER 0x175

#define MSG_LIST_TITLE 0x699
#define MSG_LIST_HINT 0x72a
#define MSG_LIST_COLUMN_0 0x6a1
#define MSG_LIST_COLUMN_1 0x197
#define MSG_LIST_COLUMN_2 0x198
#define MSG_LIST_COLUMN_3 0x6a2
#define MSG_LIST_CHECKBOX 0x69e
#define MSG_LIST_FORM_BUTTON 0x6e3
#define MSG_LIST_FORM_TOOLTIP 0x6e4
#define MSG_LIST_ANY 0x6e6
#define MSG_LIST_CATEGORY 0x6db
#define MSG_LIST_FIRST_JOBS 0x6b0
#define MSG_LIST_JOBS 0x65d
#define MSG_LIST_SEARCHED 0x6e5
#define MSG_LIST_BAD_LEVEL 0x6ea
#define MSG_LIST_LEVEL_RANGE 0xa29

#define LIST_CATEGORIES 9
#define LIST_ROWS 5
#define LIST_ROW_HEIGHT 0x28
#define LIST_TOP 0x78
#define LIST_ROW_LEFT 0x14
#define LIST_ROW_WIDTH 0x1fe
#define LIST_MINIMISED_HEIGHT 0x11
#define LIST_RESULTS_ASKED 10

#define CZ_PARTY_BOOKING_REQ_SEARCH 0x804

#define UIWM_MSG_WHISPER 0xe
#define UI_MSG_REBUILD 0x17

#pragma pack(push, 1)
struct PacketPartyBookingSearch {
    short id;
    short level;
    short where;
    short job;
    int last_index;
    short count;
};
#pragma pack(pop)
SIZE_ASSERT(PacketPartyBookingSearch, 0xe);

struct UISeekPartyListWnd {
    struct UIFrameWnd frame;
    struct UIWindow *title;
    struct UIWindow *labels[3];
    struct UIWindow *columns[4];
    struct UIWindow *extra_column;
    struct UIWindow *hint;
    struct UIWindow *level;
    struct UIWindow *region_combo;
    struct UIWindow *map_combo;
    struct UIWindow *category_combo;
    struct UIWindow *job_combo;
    struct UIWindow *checkbox;
    struct UIBitmapButton *find;
    struct UIBitmapButton *close;
    struct UIBitmapButton *whisper;
    struct UIWindow *form_button;
    struct UIBitmapButton *help;
    struct UIScrollBar *bar;
    int bar_pos;
    struct PartySearchRegions regions;
    struct Vec jobs[LIST_CATEGORIES];
    int region;
    int map;
    int category;
    int job;
    int level_filter;
    struct Vec menu;
    int top;
    int selected;
    int per_row;
    int rows;
    int menu_row;
    int scrollable;
    int full_height;
    BYTE minimised;
    BYTE first_open;
    short last_level;
    short last_where;
    short last_job;
    short last_index[2];
    short last_count;
    struct WindowInfo *info;
};
OFFSET_ASSERT(UISeekPartyListWnd, regions, 0xf0);
OFFSET_ASSERT(UISeekPartyListWnd, region, 0x164);
OFFSET_ASSERT(UISeekPartyListWnd, top, 0x184);
OFFSET_ASSERT(UISeekPartyListWnd, info, 0x1b0);
SIZE_ASSERT(UISeekPartyListWnd, 0x1b4);
void ui_seek_party_list_wnd_destroy(struct UIWindow *window);
void ui_seek_party_list_wnd_store_info(struct UIWindow *self);
void ui_seek_party_list_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_seek_party_list_wnd_on_draw(struct UIWindow *self);
void ui_seek_party_list_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_seek_party_list_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_seek_party_list_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_seek_party_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_seek_party_list_wnd_ops = {
    .destroy = ui_seek_party_list_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_seek_party_list_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_seek_party_list_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_seek_party_list_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_seek_party_list_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_seek_party_list_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_seek_party_list_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_seek_party_list_wnd_send_msg,
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

struct UISeekPartyListWnd *ui_seek_party_list_wnd_construct(struct UISeekPartyListWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_seek_party_list_wnd_ops;
    self->title = NULL;
    for (i = 0; i < 3; i++)
        self->labels[i] = NULL;
    for (i = 0; i < 4; i++)
        self->columns[i] = NULL;
    self->extra_column = NULL;
    self->hint = NULL;
    self->level = NULL;
    self->region_combo = NULL;
    self->map_combo = NULL;
    self->category_combo = NULL;
    self->job_combo = NULL;
    self->checkbox = NULL;
    self->find = NULL;
    self->close = NULL;
    self->whisper = NULL;
    self->bar = NULL;
    self->bar_pos = 0;
    self->regions.items = NULL;
    self->regions.size = 0;
    for (i = 0; i < LIST_CATEGORIES; i++)
        self->jobs[i].first = self->jobs[i].last = self->jobs[i].end = NULL;
    self->region = 0;
    self->category = 0;
    self->job = 0;
    self->level_filter = 0;
    self->menu.first = self->menu.last = self->menu.end = NULL;
    self->minimised = 0;
    self->first_open = 1;
    self->last_level = -5;
    self->last_where = -5;
    self->last_job = -5;
    self->full_height = 0;
    self->last_index[0] = self->last_index[1] = 0;
    self->last_count = 0;
    self->form_button = NULL;
    return self;
}

struct UIWindow *ui_seek_party_list_wnd_new(void)
{
    return (struct UIWindow *)ui_seek_party_list_wnd_construct(mem_alloc(sizeof(struct UISeekPartyListWnd)));
}

void ui_seek_party_list_wnd_destruct(struct UISeekPartyListWnd *self)
{
    int i;

    self->frame.base.ops = &ui_seek_party_list_wnd_ops;
    ui_menu_entry_vec_free(&self->menu);
    for (i = LIST_CATEGORIES - 1; i >= 0; i--)
        string_vec_free(&self->jobs[i]);
    party_search_regions_clear(&self->regions);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_seek_party_list_wnd_destroy(struct UIWindow *window)
{
    struct UISeekPartyListWnd *self = (struct UISeekPartyListWnd *)window;
    ui_seek_party_list_wnd_destruct(self);
    mem_free(self);
}

void ui_seek_party_list_wnd_store_info(struct UIWindow *window)
{
    struct UISeekPartyListWnd *self = (struct UISeekPartyListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}

static int list_info_ok(const struct WindowInfo *info)
{
    if (info->width < 0x226 || info->height < 0x15e)
        return 0;
    if (info->x < 0 || info->x > g_renderer->width - info->width)
        return 0;
    if (info->y < 0 || info->y > g_renderer->height - info->height)
        return 0;
    return 1;
}

static void list_info_reset(struct WindowInfo *info)
{
    info->x = 0x1fe;
    info->y = 0x168;
    info->width = 0x226;
    info->height = 0x15e;
}

static void update_scroll(struct UISeekPartyListWnd *self)
{
    int pages, over;

    if (!self->bar)
        return;
    pages = ((int)g_session1.partyBooking.rows.size) / self->per_row + (((int)g_session1.partyBooking.rows.size) % self->per_row != 0);
    over = pages - self->rows;
    if (over < 0)
        over = 0;
    self->scrollable = over != 0;
    if (over) {
        ui_scroll_bar_set_range(self->bar, over, self->rows, pages);
        if (self->top < 0)
            self->top = 0;
        if (self->top > pages - self->rows)
            self->top = pages - self->rows;
        ui_scroll_bar_set_pos(self->bar, self->top);
    } else {
        self->top = 0;
        ui_scroll_bar_set_range(self->bar, 0, self->rows, pages);
    }
    ui_window_invalidate_base(&self->bar->base);
}

static int row_at(struct UISeekPartyListWnd *self, int x, int y)
{
    struct UIWindow *wnd = &self->frame.base;
    long long first, index;
    int row;
    if (x > wnd->width - LIST_ROW_LEFT || x < LIST_ROW_LEFT || y < LIST_TOP || y > wnd->height - 0x1e ||
        self->per_row <= 0)
        return -1;
    row = (y - LIST_TOP) / LIST_ROW_HEIGHT;
    first = (long long)self->top * self->per_row;
    if (first < 0)
        first = 0;
    index = first + (long long)self->per_row * row;
    return index < g_session1.partyBooking.rows.size ? (int)index : -1;
}

static void choose_row(struct UISeekPartyListWnd *self, int x, int y)
{
    int old = self->selected;

    self->selected = row_at(self, x, y);
    if (old != self->selected)
        ui_window_invalidate_base(&self->frame.base);
}

void ui_seek_party_list_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISeekPartyListWnd *self = (struct UISeekPartyListWnd *)window;
    if (y < LIST_TOP) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    choose_row(self, x, y);
}

void ui_seek_party_list_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UISeekPartyListWnd *self = (struct UISeekPartyListWnd *)window;
    if (y < LIST_TOP)
        return;
    choose_row(self, x, y);
    ui_seek_party_list_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, LIST_ID_WHISPER, 0, 0, 0);
}

void ui_seek_party_list_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UISeekPartyListWnd *self = (struct UISeekPartyListWnd *)window;
    int step;

    if (!self->scrollable)
        return;
    step = self->rows > 3 ? 3 : self->rows - 1;
    if (delta >= 0)
        step = -step;
    ui_seek_party_list_wnd_send_msg(&self->frame.base, &self->bar->base, UI_MSG_SCROLL_V, step, 0, 0, 0);
}

static void combo_add_copy(struct UIWindow *combo, const struct String *text, int value)
{
    struct String copy;

    string_copy(&copy, text);
    ui_combo_box_def2_wnd_add_item(combo, copy, value);
}

static void region_chosen(struct UISeekPartyListWnd *self, int region)
{
    if (!seek_party_show_maps(self->map_combo, &self->regions, region))
        return;
    ui_combo_box_def_wnd_select(self->map_combo, 0);
    self->map = 0;
    ui_window_invalidate_base(&self->frame.base);
}

static void category_chosen(struct UISeekPartyListWnd *self, int category)
{
    struct String *job;

    if (category == self->category && self->category != 0)
        return;
    ui_combo_box_def_wnd_clear_list(self->job_combo);
    for (job = self->jobs[category].first; job != (struct String *)self->jobs[category].last; job++)
        combo_add_copy(self->job_combo, job, 0);
    ui_combo_box_def_wnd_select(self->job_combo, 0);
    self->category = category;
    self->job = 1;
    ui_window_invalidate_base(&self->frame.base);
}

static void load_regions(struct UISeekPartyListWnd *self)
{
    seek_party_load_regions(self->region_combo, &self->regions, 1);
    ui_combo_box_def_wnd_select(self->region_combo, 0);
    region_chosen(self, 1);
}

static void push_job(struct Vec *jobs, const char *name)
{
    struct String text;

    string_init(&text);
    string_assign_cstr(&text, name);
    string_vec_push_move(jobs, &text);
    string_free(&text);
}

static void load_jobs(struct UISeekPartyListWnd *self)
{
    static const int count[LIST_CATEGORIES] = { 0, 0, 6, 0xd, 6, 7, 6, 0xd, 8 };
    int label = 0;
    int c, j;

    ui_combo_add(self->category_combo, msg_str(MSG_LIST_ANY));
    for (c = 0; c < LIST_CATEGORIES - 1; c++)
        ui_combo_add(self->category_combo, msg_str(MSG_LIST_CATEGORY + c));
    push_job(&self->jobs[0], msg_str(MSG_LIST_ANY));
    for (j = 0; j < 4; j++)
        push_job(&self->jobs[1], msg_str(MSG_LIST_FIRST_JOBS + j));
    for (c = 2; c < LIST_CATEGORIES; c++)
        for (j = 0; j < count[c]; j++)
            push_job(&self->jobs[c], msg_str(MSG_LIST_JOBS + label++));
    ui_combo_box_def_wnd_select(self->category_combo, 0);
    category_chosen(self, 0);
}

static struct UIWindow *add_static(struct UIWindow *wnd, int width, int height, const char *text, int outline,
                                   int x, int y)
{
    struct UIWindow *label = mem_alloc(sizeof(struct UIStaticText));

    if (label)
        label = ui_static_text_construct(label);
    ui_window_create(label, width, height);
    ui_static_text_set_text(label, text, outline);
    ui_window_move(label, x, y);
    return label;
}

static struct UIWindow *add_combo(struct UIWindow *wnd, int width, int x, int y)
{
    struct UIWindow *combo = mem_alloc(sizeof(struct UIComboBoxDefWnd));
    struct UIComboBoxDefWnd *def;

    if (combo)
        combo = ui_combo_box_def_wnd_construct(combo);
    def = (struct UIComboBoxDefWnd *)combo;
    def->list_height = 0x50;
    ui_window_create(combo, width, 0x12);
    ui_window_move(combo, x, y);
    def->border_r = def->border_g = def->border_b = 0x64;
    def->back_r = def->back_g = def->back_b = 0xe8;
    ui_window_add_child(wnd, combo);
    return combo;
}

void ui_seek_party_list_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UISeekPartyListWnd *self = (struct UISeekPartyListWnd *)window;
    static const char *const labels[3] = { "LEVEL", "MAP", "JOB" };
    static const int label_y[3] = { 0x17, 0x2d, 0x46 };
    static const int column_msg[4] = { MSG_LIST_COLUMN_0, MSG_LIST_COLUMN_1, MSG_LIST_COLUMN_2, MSG_LIST_COLUMN_3 };
    static const int column_x[4] = { 0x21, 0x6a, 0xdc, 0x16d };
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *box;
    struct UITextButton *form;
    struct String title;
    int i;

    string_init(&title);
    string_assign_cstr(&title, msg_str(MSG_LIST_TITLE));
    self->title = mem_alloc(sizeof(struct UIStaticText));
    if (self->title)
        self->title = ui_static_text_construct(self->title);
    ui_window_create(self->title, wnd->width - 0x20, 0xf);
    ui_static_text_set_text_ellipsis(self->title, string_cstr(&title));
    ui_window_move(self->title, 0x10, 2);
    ((struct UIStaticText *)self->title)->white_shadow = 1;
    ui_window_add_child(wnd, self->title);
    string_free(&title);
    ui_window_add_close_button(wnd, "", 0);
    ui_window_add_mini_button(wnd, "", 0);

    for (i = 0; i < 3; i++) {
        self->labels[i] = add_static(wnd, wnd->width - 0xa, 0x1e, labels[i], 1, 0xa, label_y[i]);
        ((struct UIStaticText *)self->labels[i])->white_shadow = 1;
        ui_window_add_child(wnd, self->labels[i]);
        if (i == 0) {
            self->level = mem_alloc(sizeof(struct UIEditCtrl));
            if (self->level)
                self->level = ui_edit_ctrl_construct(self->level);
            ui_window_create(self->level, 0x28, 0x10);
            ui_window_move(self->level, 0x32, 0x14);
            UI_EDIT_MAX_CHARS(self->level) = 5;
            ui_edit_ctrl_set_back_colour(self->level, 0xe8, 0xe8, 0xe8);
            ui_edit_ctrl_set_border_colour(self->level, 0x64, 0x64, 0x64);
            ((struct UIEditCtrl *)self->level)->no_border = 0;
            ui_window_add_child(wnd, self->level);
        }
    }
    self->hint = add_static(wnd, wnd->width - 0xa, 0x1e, msg_str(MSG_LIST_HINT), 0, 0x64, wnd->height - 0x14);
    ui_window_add_child(wnd, self->hint);
    for (i = 0; i < 4; i++) {
        self->columns[i] = add_static(wnd, 0x32, 0x14, msg_str(column_msg[i]), 0, column_x[i], 0x68);
        ui_window_add_child(wnd, self->columns[i]);
    }

    self->bar = mem_alloc(sizeof(struct UIScrollBar));
    if (self->bar)
        self->bar = ui_scroll_bar_construct(self->bar);
    ui_scroll_bar_create(self->bar, 0x203, LIST_TOP, UI_SCROLL_VERT, 0xc8);
    ui_window_add_child(wnd, &self->bar->base);

    self->category_combo = add_combo(wnd, 0x96, 0x32, 0x44);
    self->job_combo = add_combo(wnd, 0xc8, 0xdc, 0x44);
    self->region_combo = add_combo(wnd, 0x96, 0x32, 0x2b);
    self->map_combo = add_combo(wnd, 0xc8, 0xdc, 0x2b);

    box = mem_alloc(sizeof(struct UICheckBox));
    if (box)
        box = (struct UIWindow *)ui_check_box_construct(box);
    self->checkbox = box;
    ui_window_create(box, 0x3c, 0x14);
    ui_window_create(self->checkbox, 0x64, 0xc);
    ui_check_box_set_bitmaps(self->checkbox, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp",
                          msg_str(MSG_LIST_CHECKBOX));
    ui_window_move_base(self->checkbox, 0x1ae, 0x2b);
    ui_check_box_set_check(self->checkbox, g_session1.partyBooking.alarm);
    ui_window_add_child(wnd, self->checkbox);

    self->close = ui_add_bitmap_button(wnd, UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp",
                                       wnd->width - 0x2d, wnd->height - 0x18, LIST_ID_CLOSE);
    self->find = ui_add_bitmap_button(wnd, UI_SKIN "btn_find.bmp", UI_SKIN "btn_find_a.bmp",
                                      UI_SKIN "btn_find_b.bmp", 0x1e0, 0x25, LIST_ID_FIND);
    self->whisper = ui_add_bitmap_button(wnd, UI_SKIN "basic_interface\\direct_chat.bmp",
                                         UI_SKIN "basic_interface\\direct_chat.bmp",
                                         UI_SKIN "basic_interface\\direct_chat.bmp", 0x46, wnd->height - 0x18,
                                         LIST_ID_WHISPER);

    form = ui_text_button_create();
    self->form_button = &form->button.base;
    ui_text_button_set_text(self->form_button, msg_str(MSG_LIST_FORM_BUTTON), 0, 0, 0xc, 0, 0);
    ((struct UITextButton *)self->form_button)->hover_colour = 0xefb700;
    ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)self->form_button, msg_str(MSG_LIST_FORM_TOOLTIP));
    ui_window_move_base(self->form_button, 0xa, wnd->height - 0x16);
    self->form_button->id = LIST_ID_FORM;
    ui_window_add_child(wnd, self->form_button);

    self->help = ui_add_bitmap_button(wnd, UI_SKIN "battle_field\\btn_help.bmp", UI_SKIN "battle_field\\btn_help_a.bmp",
                                      UI_SKIN "battle_field\\btn_help_b.bmp", wnd->width - 0x6e,
                                      wnd->height - 0x18, LIST_ID_HELP);

    self->top = 0;
    self->selected = -1;
    self->per_row = 1;
    self->rows = LIST_ROWS;
    self->scrollable = 0;
    self->menu_row = 0;
    load_regions(self);
    load_jobs(self);
    update_scroll(self);
}

static void draw_row(struct UISeekPartyListWnd *self, const struct PartySearchRow *r, int index, int y)
{
    struct UIWindow *wnd = &self->frame.base;

    ui_window_fill_rect(wnd, LIST_ROW_LEFT, y - 1, LIST_ROW_WIDTH, LIST_ROW_HEIGHT, 0xcecece);
    if (r->is_more_row) {
        ui_window_fill_rect(wnd, 0x15, y, 0x1fc, index + 2 == ((int)g_session1.partyBooking.rows.size) ? 0x26 : 0x27, 0xffffff);
        ui_window_draw_bitmap(wnd, 0x16, y + 1, ui_skin_bitmap(UI_SKIN "basic_interface\\more_info.bmp"), 0);
        return;
    }
    ui_window_fill_rect(wnd, 0x15, y, 0x1fc, index + 1 == ((int)g_session1.partyBooking.rows.size) ? 0x26 : 0x27, 0xffffff);
    if (self->selected == index)
        ui_window_fill_rect(wnd, LIST_ROW_LEFT, y - 1, LIST_ROW_WIDTH, LIST_ROW_HEIGHT, 0xef9c73);
    ui_window_text_out(wnd, 0x16, y + 2, owned_text_cstr(&r->time), 0, 0, 0xc, 0, 0, 0);
    {
        struct String shortened = {0};
        ui_window_text_out(wnd, 0x4c, y + 2, ui_window_shorten_text(wnd, &shortened, owned_text_cstr(&r->leader), 0x82, 0, 0xc, 0),
                         0, 0, 0xc, 0, 0, 0);
        string_free(&shortened);
    }
    ui_window_text_out(wnd, 0xd7, y + 2, owned_text_cstr(&r->levels), 0, 0, 0xc, 0, 0, 0);
    ui_window_text_out(wnd, 0x118, y + 2, owned_text_cstr(&r->place), 0, 0, 0xc, 0, 0, 0);
    ui_window_text_out(wnd, 0x16, y + 0x16, owned_text_cstr(&r->jobs), 0, 0, 0xc, 0, 0, 0);
}

void ui_seek_party_list_wnd_on_draw(struct UIWindow *window)
{
    struct UISeekPartyListWnd *self = (struct UISeekPartyListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int r, g, b;

    ui_window_draw_title_bar(wnd, 1, "", 0);
    if (self->minimised)
        return;
    ui_skin_color_chip(&g_skinMgr, 2, 2, &r, &g, &b);
    ui_window_fill_rect(wnd, 0, 0x11, wnd->width, wnd->height - 0x26, RGB(r, g, b));
    if (g_session1.partyBooking.rows.size) {
        const struct PartySearchEntry *entry = party_search_rows_at(&g_session1.partyBooking.rows, 0);
        int index = 0, shown = 0, y = LIST_TOP + 5;
        while (entry && shown < LIST_ROWS) {
            int key = entry->key;
            if (index >= (long long)self->top * self->per_row) {
                struct PartySearchRow copy = {0};
                if (!party_search_row_copy(&copy, &entry->row))
                    break;
                draw_row(self, &copy, index, y);
                ++shown;
                y += LIST_ROW_HEIGHT;
                party_search_row_clear(&copy);
            }
            entry = party_search_rows_after(&g_session1.partyBooking.rows, key);
            ++index;
        }
        update_scroll(self);
    }
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

static short search_job(const struct UISeekPartyListWnd *self)
{
    static const int first_job[LIST_CATEGORIES] = { 0, 1, 5, 0xb, 0x18, 0x1e, 0x25, 0x2b, 0x38 };

    if (self->category == 0)
        return -1;
    return (short)(first_job[self->category] - 1 + self->job);
}

static void title_append(struct String *title, const char *a, struct String *b)
{
    string_append_cstr(title, a);
    string_append(title, string_cstr(b), b->size);
}

static void search(struct UISeekPartyListWnd *self, int command)
{
    struct PacketPartyBookingSearch packet;
    struct String text, title;
    size_t k;

    packet.id = CZ_PARTY_BOOKING_REQ_SEARCH;
    packet.level = (short)atoi(ui_edit_ctrl_get_text(self->level));
    string_init(&text);
    string_assign(&text, ui_edit_ctrl_get_text(self->level),
                      strlen(ui_edit_ctrl_get_text(self->level)));
    for (k = 0; k < text.size; k++) {
        char c = string_cstr(&text)[k];

        if (c < '/' || c > '9') {
            ui_message_box(msg_str(MSG_LIST_BAD_LEVEL));
            string_free(&text);
            return;
        }
    }
    if (packet.level < 0 || packet.level > g_session1.baseLevel3rd) {
        char buf[0x104];

        memset(buf, 0, sizeof buf);
        _snprintf_s(buf, sizeof buf, (size_t)-1, msg_str(MSG_LIST_LEVEL_RANGE), g_session1.baseLevel3rd);
        ui_message_box(buf);
        string_free(&text);
        return;
    }
    packet.where = self->region == 0 ? 0 : (short)((WORD)self->region << 8 | (WORD)self->map);
    packet.job = search_job(self);

    if (command == LIST_ID_FIND) {
        if (self->last_job == packet.job && self->last_level == packet.level && self->last_where == packet.where) {
            string_free(&text);
            return;
        }
        packet.last_index = 0;
        party_booking_clear_rows(&g_session1.partyBooking);
    } else if (command == LIST_ID_MORE) {
        if (!g_session1.partyBooking.queue_new) {
            party_booking_take_pending(&g_session1.partyBooking);
            string_free(&text);
            return;
        }
        packet.last_index = g_session1.partyBooking.last_id;
    }
    packet.count = LIST_RESULTS_ASKED;
    ui_send_packet(&packet);
    self->last_job = packet.job;
    self->last_level = packet.level;
    self->last_where = packet.where;

    string_init(&title);
    string_assign_cstr(&title, msg_str(MSG_LIST_SEARCHED));
    if (packet.level > 0) {
        struct String number;

        string_init(&number);
        string_format(&number, "%d", packet.level);
        title_append(&title, " Level: ", &number);
        string_free(&number);
    }
    if (self->region != 0) {
        struct String map, region;

        ui_combo_box_def_wnd_get_label_text(self->map_combo, &map);
        ui_combo_box_def_wnd_get_label_text(self->region_combo, &region);
        title_append(&title, " Map: ", &region);
        title_append(&title, " ", &map);
        string_free(&region);
        string_free(&map);
    }
    if (self->category != 0) {
        struct String job;

        ui_combo_box_def_wnd_get_label_text(self->job_combo, &job);
        title_append(&title, " Job: ", &job);
        string_free(&job);
    }
    ui_static_text_set_text_ellipsis(self->title, string_cstr(&title));
    party_booking_set_filter(&g_session1.partyBooking, self->last_level, self->last_where, self->last_job);
    ui_window_invalidate_base(&self->frame.base);
    string_free(&title);
    string_free(&text);
}

static void whisper(struct UISeekPartyListWnd *self)
{
    const struct PartySearchEntry *entry;
    struct PartySearchRow copy = {0};
    int key;
    if (self->selected < 0 || self->selected < (long long)self->top * self->per_row)
        return;
    entry = party_search_rows_at(&g_session1.partyBooking.rows, (unsigned)self->selected);
    if (!entry || !party_search_row_copy(&copy, &entry->row))
        return;
    key = entry->key;
    if (copy.is_more_row) {
        ui_seek_party_list_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, LIST_ID_MORE, 0, 0, 0);
        party_booking_remove_row(&g_session1.partyBooking, key);
    } else {
        const char *player = session_character_name(&g_session1);
        if (strlen(player) != copy.leader.size ||
            memcmp(player, owned_text_cstr(&copy.leader), copy.leader.size))
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_WHISPER, (int)owned_text_cstr(&copy.leader), 0, 0, 0);
    }
    party_search_row_clear(&copy);
}

int ui_seek_party_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4)
{
    struct UISeekPartyListWnd *self = (struct UISeekPartyListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case LIST_ID_CLOSE:
            g_session1.partyBooking.alarm = self->checkbox->state != 0;
            ui_close_window(SEEK_PARTY_LIST_WND_ID);
            break;
        case LIST_ID_MINIMISE:
            if (self->minimised) {
                ui_window_resize_base(&self->frame.base, wnd->width, self->full_height);
            } else {
                self->full_height = wnd->height;
                ui_window_resize_base(&self->frame.base, wnd->width, LIST_MINIMISED_HEIGHT);
            }
            self->minimised = !self->minimised;
            ui_window_invalidate_base(&self->frame.base);
            break;
        case LIST_ID_REDRAW:
            ui_window_invalidate_base(&self->frame.base);
            break;
        case LIST_ID_HELP:
            ui_window_open(PARTY_BOOKING_HELP_WND_ID);
            break;
        case LIST_ID_FIND:
        case LIST_ID_MORE:
            search(self, val1);
            break;
        case LIST_ID_WHISPER:
            whisper(self);
            break;
        }
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN: {
        int old = self->top;

        if (msg == UI_MSG_PAGE_UP)
            self->top = old - self->rows + 1;
        else if (msg == UI_MSG_PAGE_DOWN)
            self->top = self->rows - 1 + old;
        else
            self->top = val1 + old;
        if (old != self->top) {
            update_scroll(self);
            ui_window_invalidate_base(&self->frame.base);
        }
        return self->top - old;
    }
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!list_info_ok(self->info))
            list_info_reset(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        ui_seek_party_list_wnd_send_msg(&self->frame.base, NULL, UI_MSG_RESIZE, 3, self->info->width, self->info->height, 0);
        ui_seek_party_list_wnd_send_msg(&self->frame.base, NULL, UI_MSG_REBUILD, 0, 0, 0, 0);
        return 0;
    case COMBO_MSG_CHOSEN:
        if (sender == self->region_combo) {
            region_chosen(self, val1 + 1);
            self->region = val1;
        } else if (sender == self->map_combo) {
            self->map = val1;
        } else if (sender == self->category_combo) {
            category_chosen(self, val1);
        } else if (sender == self->job_combo) {
            self->job = val1 + 1;
        }
        return 0;
    case UI_MSG_TEXT_BUTTON:
        if (val1 == LIST_ID_FORM && !ui_close_window(SEEK_PARTY_MB_WND_ID))
            ui_window_open(SEEK_PARTY_MB_WND_ID);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

#define MSG_PARTY_BOOKING_NEW 0x6d9
#define PARTY_ALARM_COLOUR 0xffff00
#define UIWM_MSG_NOTICE 1

char party_booking_wnd_matches(struct UISeekPartyListWnd *self, struct PartyBookingPost post)
{
    int i;

    if (self->last_level > 0 && (self->last_level > post.level || post.level - self->last_level > 0xf))
        return 0;
    if (self->last_where > 0 && self->last_where != post.map)
        return 0;
    if (self->last_job <= 0)
        return 1;
    for (i = 0; i < PARTY_BOOKING_JOBS; i++)
        if (self->last_job == post.jobs[i])
            return 1;
    return 0;
}

void party_booking_wnd_alarm(struct UISeekPartyListWnd *self)
{
    if (!self->checkbox->state)
        return;
    sound_play_wave("party_alarm.wav", 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_NOTICE, (int)msg_str(MSG_PARTY_BOOKING_NEW), PARTY_ALARM_COLOUR,
                          0xe, 0);
}
