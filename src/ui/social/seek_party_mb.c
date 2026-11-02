#include "render/rp_pool.h"
#include "contents/party_booking_regions.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/social/seek_party.h"
#include "ui/social/seek_party_mb.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "ui/ui_std.h"

#define SEEK_PARTY_MB_WND_ID 0xa8
#define SEEK_PARTY_WND_ID 0xa4
#define JOB_LIST_WND_ID 0xaa

#define SEEK_PARTY_MB_ID_OK 0xb0
#define SEEK_PARTY_MB_ID_CANCEL 0xb1
#define SEEK_PARTY_MB_ID_MINIMISE 0xc2

#define MSG_SEEK_PARTY_MB_TITLE 0x698
#define MSG_SEEK_PARTY_MB_LEVEL_HINT 0x79d
#define MSG_SEEK_PARTY_MB_FIRST_JOBS 0x6b0
#define MSG_SEEK_PARTY_MB_JOBS 0x65d
#define MSG_SEEK_PARTY_MB_CATEGORY 0x6db
#define MSG_SEEK_PARTY_MB_NO_JOB 0x6e8
#define MSG_SEEK_PARTY_MB_TOO_MANY_JOBS 0x6e9
#define MSG_SEEK_PARTY_MB_BAD_LEVEL 0x6ea
#define MSG_SEEK_PARTY_MB_LEVEL_RANGE 0xa29

#define SEEK_PARTY_MB_CATEGORIES 8
#define SEEK_PARTY_MB_MAX_JOBS 6
#define SEEK_PARTY_MB_MINIMISED_HEIGHT 0x11

#define CZ_SEEK_PARTY 0x802

#define UI_MSG_COMBO_SELECT 0x27
#define UI_MSG_REBUILD 0x17

struct JobBoxPair {
    short job;
    struct UIWindow *box;
};

static int job_less(const void *a, const void *b)
{
    return *(const unsigned short *)a < *(const unsigned short *)b;
}

#pragma pack(push, 1)
struct SeekPartyForm {
    short level;
    short where;
    short jobs[SEEK_PARTY_MB_MAX_JOBS];
};
struct PacketSeekParty {
    short id;
    struct SeekPartyForm form;
};
#pragma pack(pop)

struct UISeekPartyMBWnd {
    struct UIFrameWnd frame;
    struct UIWindow *title;
    struct UIWindow *labels[3];
    struct UIWindow *level;
    struct UIWindow *region_combo;
    struct UIWindow *map_combo;
    struct UIWindow *category_combo;
    struct Tree jobs[SEEK_PARTY_MB_CATEGORIES + 1];
    struct PartySearchRegions regions;
    struct UIBitmapButton *cancel;
    struct UIBitmapButton *ok;
    int category;
    short region;
    short map;
    int full_height;
    BYTE minimised;
    struct SeekPartyForm form;
    struct WindowInfo *info;
};
void ui_seek_party_mb_wnd_destroy(struct UIWindow *window);
void ui_seek_party_mb_wnd_store_info(struct UIWindow *self);
void ui_seek_party_mb_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_seek_party_mb_wnd_on_draw(struct UIWindow *self);
intptr_t ui_seek_party_mb_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_seek_party_mb_wnd_ops = {
    .destroy = ui_seek_party_mb_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_seek_party_mb_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_seek_party_mb_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_seek_party_mb_wnd_on_draw,
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
    .send_msg = ui_seek_party_mb_wnd_send_msg,
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

const short *ui_seek_party_mb_jobs(const struct UIWindow *wnd)
{
    return ((const struct UISeekPartyMBWnd *)wnd)->form.jobs;
}

extern int(*_imp__atoi)(const char *text);

static void combo_add(struct UIWindow *combo, const char *text, int value)
{
    struct String item;

    string_init(&item);
    string_assign_cstr(&item, text);
    ui_combo_box_def2_wnd_add_item(combo, item, value);
}

struct UISeekPartyMBWnd *ui_seek_party_mb_wnd_construct(struct UISeekPartyMBWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_seek_party_mb_wnd_ops;
    self->title = NULL;
    for (i = 0; i < 3; i++)
        self->labels[i] = NULL;
    self->level = NULL;
    self->region_combo = NULL;
    self->map_combo = NULL;
    self->category_combo = NULL;
    for (i = 0; i < SEEK_PARTY_MB_CATEGORIES + 1; i++)
        tree_init(&self->jobs[i]);
    self->regions.items = NULL;
    self->regions.size = 0;
    self->cancel = NULL;
    self->ok = NULL;
    self->category = -1;
    self->map = 0;
    self->full_height = 0;
    self->minimised = 0;
    return self;
}

struct UIWindow *ui_seek_party_mb_wnd_new(void)
{
    return (struct UIWindow *)ui_seek_party_mb_wnd_construct(mem_alloc(sizeof(struct UISeekPartyMBWnd)));
}

static void ui_seek_party_mb_wnd_destruct(struct UISeekPartyMBWnd *self)
{
    int i;

    self->frame.base.ops = &ui_seek_party_mb_wnd_ops;
    party_search_regions_clear(&self->regions);
    for (i = SEEK_PARTY_MB_CATEGORIES; i >= 0; i--)
        tree_free(&self->jobs[i], NULL);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_seek_party_mb_wnd_destroy(struct UIWindow *window)
{
    struct UISeekPartyMBWnd *self = (struct UISeekPartyMBWnd *)window;
    ui_seek_party_mb_wnd_destruct(self);
    mem_free(self);
}

static void show_category(struct UISeekPartyMBWnd *self, int category)
{
    struct TreeNode *node;
    int i;

    if (category == self->category)
        return;
    if (self->category != -1) {
        for (i = 0; i < SEEK_PARTY_MB_CATEGORIES; i++) {
            struct TreeNode *head = self->jobs[i].head;

            if (head->left == head)
                return;
            for (node = head->left; node != head; node = tree_next(node)) {
                ui_check_box_set_hidden(TREE_VALUE(node, struct JobBoxPair)->box, 1);
                ((struct UICheckBox *)TREE_VALUE(node, struct JobBoxPair)->box)->disabled = 1;
            }
        }
    }
    {
        struct TreeNode *head = self->jobs[category].head;

        if (head->left == head)
            return;
        for (node = head->left; node != head; node = tree_next(node)) {
            ui_check_box_set_hidden(TREE_VALUE(node, struct JobBoxPair)->box, 0);
            ((struct UICheckBox *)TREE_VALUE(node, struct JobBoxPair)->box)->disabled = 0;
        }
    }
    self->category = category;
    ui_window_invalidate_base(&self->frame.base);
}

static void add_job_box(struct UISeekPartyMBWnd *self, struct Tree *map, short job, const char *label, int row)
{
    struct UIWindow *box = mem_alloc(sizeof(struct UICheckBox));
    struct JobBoxPair pair;

    if (box)
        box = (struct UIWindow *)ui_check_box_construct(box);
    ui_window_create(box, 0x6e, 0xc);
    ui_check_box_set_bitmaps(box, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp", label);
    if (row < 8)
        ui_window_move_base(box, 0x36, 0x63 + 0x19 * row);
    else
        ui_window_move_base(box, 0xb4, -0x65 + 0x19 * row);
    ui_check_box_set_check(box, 0);
    ui_check_box_set_hidden(box, 1);
    ((struct UICheckBox *)box)->disabled = 1;
    ui_window_add_child(&self->frame.base, box);
    pair.job = job;
    pair.box = box;
    tree_add(map, &pair, sizeof pair, job_less, NULL);
}

static void make_job_boxes(struct UISeekPartyMBWnd *self)
{
    static const int count[SEEK_PARTY_MB_CATEGORIES] = { 4, 6, 0xd, 6, 7, 6, 0xd, 8 };
    static const int first_job[SEEK_PARTY_MB_CATEGORIES] = { 1, 5, 0xb, 0x18, 0x1e, 0x25, 0x2b, 0x38 };
    int label = 0;
    int c, j;

    for (j = 0; j < count[0]; j++)
        add_job_box(self, &self->jobs[0], (short)(j + 1), msg_str(MSG_SEEK_PARTY_MB_FIRST_JOBS + j), j);
    for (c = 1; c < SEEK_PARTY_MB_CATEGORIES; c++)
        for (j = 0; j < count[c]; j++)
            add_job_box(self, &self->jobs[c], (short)(first_job[c] + j), msg_str(MSG_SEEK_PARTY_MB_JOBS + label++),
                        j);
    show_category(self, 0);
}

static void fill_categories(struct UISeekPartyMBWnd *self)
{
    int i;

    for (i = 0; i < SEEK_PARTY_MB_CATEGORIES; i++)
        combo_add(self->category_combo, msg_str(MSG_SEEK_PARTY_MB_CATEGORY + i), 0);
    ui_combo_box_def_wnd_select(self->category_combo, 0);
}

void seek_party_load_regions(struct UIWindow *region_combo, struct PartySearchRegions *regions, int first)
{
    struct TextArray names = {0};
    size_t i;
    if (!party_search_read_regions(g_session1.luaState, first, &names, regions))
        return;
    ui_combo_box_def_wnd_clear_list(region_combo);
    for (i = 0; i < names.size; ++i)
        combo_add(region_combo, owned_text_cstr(&names.items[i]), 0);
    text_array_clear(&names);
}

int seek_party_show_maps(struct UIWindow *combo, const struct PartySearchRegions *regions, int key)
{
    struct PartySearchMaps copy = {0};
    const struct PartySearchMaps *maps;
    unsigned i;
    ui_combo_box_def_wnd_clear_list(combo);
    maps = party_search_region_maps(regions, key);
    if (!maps || !party_search_maps_copy(&copy, maps))
        return 0;
    for (i = 0; i < copy.size; ++i) {
        struct String item;
        const char *name = owned_text_cstr(&copy.items[i].name);
        string_init(&item);
        string_assign_cstr(&item, name);
        ui_combo_box_def2_wnd_add_item(combo, item, (int)copy.items[i].color);
    }
    party_search_maps_clear(&copy);
    return 1;
}

static void load_regions(struct UISeekPartyMBWnd *self)
{
    seek_party_load_regions(self->region_combo, &self->regions, 2);
    ui_combo_box_def_wnd_select(self->region_combo, 0);
    ui_seek_party_mb_wnd_send_msg(&self->frame.base, self->region_combo, UI_MSG_COMBO_SELECT, 0, 0, 0, 0);
}

static struct UIWindow *add_label(struct UIWindow *wnd, const char *text, int y)
{
    struct UIWindow *label = mem_alloc(sizeof(struct UIStaticText));

    if (label)
        label = ui_static_text_construct(label);
    ui_window_create(label, wnd->width - 0xa, 0x1e);
    ui_static_text_set_text(label, text, 1);
    ui_window_move(label, 0xa, y);
    ((struct UIStaticText *)label)->white_shadow = 1;
    ui_window_add_child(wnd, label);
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

void ui_seek_party_mb_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UISeekPartyMBWnd *self = (struct UISeekPartyMBWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    char title[0x40];

    memset(title, 0, sizeof title);
    sprintf(title, msg_str(MSG_SEEK_PARTY_MB_TITLE));
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

    self->labels[0] = add_label(wnd, "LEVEL", 0x17);
    self->labels[1] = add_label(wnd, "MAP", 0x2d);
    self->labels[2] = add_label(wnd, "JOB", 0x46);

    self->ok = ui_add_bitmap_button(wnd, UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp",
                                    wnd->width - 0x5d, wnd->height - 0x18, SEEK_PARTY_MB_ID_OK);
    self->cancel = ui_add_bitmap_button(wnd, UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp",
                                        UI_SKIN "btn_cancel_b.bmp", wnd->width - 0x2e, wnd->height - 0x18,
                                        SEEK_PARTY_MB_ID_CANCEL);

    make_job_boxes(self);

    self->level = mem_alloc(sizeof(struct UIEditCtrl));
    if (self->level)
        self->level = ui_edit_ctrl_construct(self->level);
    ui_window_create(self->level, 0x28, 0x10);
    ui_window_move(self->level, 0x32, 0x14);
    ((struct UIEditCtrl *)self->level)->max_chars = 5;
    ui_edit_ctrl_set_back_colour(self->level, 0xe8, 0xe8, 0xe8);
    ui_edit_ctrl_set_border_colour(self->level, 0x64, 0x64, 0x64);
    ((struct UIEditCtrl *)self->level)->no_border = 0;
    {
        const char *hint = msg_str(MSG_SEEK_PARTY_MB_LEVEL_HINT);

        if (hint)
            string_assign_cstr(&((struct UIEditCtrl *)self->level)->tooltip, hint);
    }
    ui_window_add_child(wnd, self->level);

    self->category_combo = add_combo(wnd, 0x96, 0x32, 0x44);
    self->region_combo = add_combo(wnd, 0x64, 0x32, 0x2b);
    self->map_combo = add_combo(wnd, 0xbe, 0x9b, 0x2b);

    load_regions(self);
    fill_categories(self);
}

void ui_seek_party_mb_wnd_on_draw(struct UIWindow *window)
{
    struct UISeekPartyMBWnd *self = (struct UISeekPartyMBWnd *)window;
    static const RECT outer = { 0x32, 0x5f, 0x136, 0x125 };
    static const RECT inner = { 0x33, 0x60, 0x135, 0x124 };
    struct UIWindow *wnd = &self->frame.base;
    int r, g, b;

    ui_window_draw_title_bar(wnd, 1, "", 0);
    if (self->minimised)
        return;
    ui_skin_color_chip(&g_skinMgr, 2, 2, &r, &g, &b);
    ui_window_fill_rect(wnd, 0, 0x11, wnd->width, wnd->height - 0x26, RGB(r, g, b));
    ui_window_fill_canvas(wnd, 0, &outer);
    ui_window_fill_canvas(wnd, 0xf0f0f0, &inner);
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

void ui_seek_party_mb_wnd_store_info(struct UIWindow *window)
{
    struct UISeekPartyMBWnd *self = (struct UISeekPartyMBWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}

static int seek_party_mb_info_ok(const struct WindowInfo *info)
{
    if (info->width < 0x168 || info->height < 0x15e)
        return 0;
    if (info->x < 0 || info->x > g_renderer->width - info->width)
        return 0;
    if (info->y < 0 || info->y > g_renderer->height - info->height)
        return 0;
    return 1;
}

static void seek_party_mb_info_reset(struct WindowInfo *info)
{
    info->x = 0x1fe;
    info->y = 0x168;
    info->width = 0x168;
    info->height = 0x15e;
}

static void message_box(const char *text)
{
    ui_window_mgr_message_box(&g_windowMgr, text, UI_MSGBOX_OK, 1, 0, 0, "메시지", 0x118, 0x78, 0);
}

static void region_chosen(struct UISeekPartyMBWnd *self, int index)
{
    seek_party_show_maps(self->map_combo, &self->regions, index + 2);
    self->region = (short)index;
    ui_combo_box_def_wnd_select(self->map_combo, 0);
    self->map = 0;
}

static void submit(struct UISeekPartyMBWnd *self)
{
    struct String text;
    int checked = 0;
    int i;
    size_t k;

    for (i = 0; i < SEEK_PARTY_MB_MAX_JOBS; i++)
        self->form.jobs[i] = -1;
    for (i = 0; i < SEEK_PARTY_MB_CATEGORIES; i++) {
        struct TreeNode *head = self->jobs[i].head, *node;

        if (head->left == head)
            break;
        for (node = head->left; node != head; node = tree_next(node)) {
            const struct JobBoxPair *job_box = TREE_VALUE(node, const struct JobBoxPair);

            if (!job_box->box->state)
                continue;
            if (checked < SEEK_PARTY_MB_MAX_JOBS)
                self->form.jobs[checked] = job_box->job;
            checked++;
        }
    }
    if (checked == 0) {
        message_box(msg_str(MSG_SEEK_PARTY_MB_NO_JOB));
        return;
    }
    if (checked > SEEK_PARTY_MB_MAX_JOBS) {
        char buf[0x64];

        memset(buf, 0, sizeof buf);
        sprintf(buf, msg_str(MSG_SEEK_PARTY_MB_TOO_MANY_JOBS), checked);
        message_box(buf);
        return;
    }

    {
        const char *level = ui_edit_ctrl_get_text(self->level);

        string_init(&text);
        string_assign_cstr(&text, level);
    }
    for (k = 0; k < text.size; k++) {
        char c = string_cstr(&text)[k];

        if (c < '/' || c > '9') {
            message_box(msg_str(MSG_SEEK_PARTY_MB_BAD_LEVEL));
            string_free(&text);
            return;
        }
    }
    self->form.level = (short)_imp__atoi(ui_edit_ctrl_get_text(self->level));
    g_session1.partyBookingSearchLevel = self->form.level;
    if (self->form.level < 1 || self->form.level > g_session1.baseLevel3rd) {
        char buf[0x104];

        memset(buf, 0, sizeof buf);
        snprintf(buf, sizeof buf, msg_str(MSG_SEEK_PARTY_MB_LEVEL_RANGE),
                     g_session1.baseLevel3rd);
        message_box(buf);
        string_free(&text);
        return;
    }
    self->form.where = (short)((short)(self->region + 1) * 0x100 | self->map);
    if (!ui_close_window(SEEK_PARTY_WND_ID)) {
        struct PacketSeekParty packet;

        packet.id = CZ_SEEK_PARTY;
        packet.form = self->form;
        ui_send_packet(&packet);
    }
    string_free(&text);
}

intptr_t ui_seek_party_mb_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                        intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UISeekPartyMBWnd *self = (struct UISeekPartyMBWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case UI_MSG_COMBO_SELECT:
        if (sender == self->region_combo)
            region_chosen(self, val1);
        else if (sender == self->map_combo)
            self->map = (short)val1;
        else if (sender == self->category_combo)
            show_category(self, val1);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!seek_party_mb_info_ok(self->info))
            seek_party_mb_info_reset(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        ui_seek_party_mb_wnd_send_msg(&self->frame.base, NULL, UI_MSG_RESIZE, 3, self->info->width, self->info->height, 0);
        ui_seek_party_mb_wnd_send_msg(&self->frame.base, NULL, UI_MSG_REBUILD, 0, 0, 0, 0);
        return 0;
    case UI_MSG_COMMAND:
        switch (val1) {
        case SEEK_PARTY_MB_ID_OK:
            submit(self);
            break;
        case SEEK_PARTY_MB_ID_CANCEL:
            ui_window_mgr_post_quit1(&g_windowMgr, SEEK_PARTY_MB_WND_ID);
            ui_window_mgr_post_quit1(&g_windowMgr, JOB_LIST_WND_ID);
            break;
        case SEEK_PARTY_MB_ID_MINIMISE:
            if (self->minimised) {
                ui_window_resize_base(&self->frame.base, wnd->width, self->full_height);
            } else {
                self->full_height = wnd->height;
                ui_window_resize_base(&self->frame.base, wnd->width, SEEK_PARTY_MB_MINIMISED_HEIGHT);
            }
            self->minimised = !self->minimised;
            ui_window_invalidate_base(&self->frame.base);
            break;
        }
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
