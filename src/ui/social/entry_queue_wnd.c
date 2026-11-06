#include "game/tips.h"
#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "ui/social/entry_queue_windows.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include <stdio.h>
#include <string.h>
#include "ui/social/social.h"
#include "ui/social/entry_queue_state.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_party.h"

#define BATTLE_FIELD UI_SKIN "battle_field\\"

struct EntryQueueLine {
    struct String text;
    int id;
};

static int line_less(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

static void destroy_line(void *value)
{
    string_free(&((struct EntryQueueLine *)value)->text);
}

struct UIEntryQueueListBox {
    struct UIListBox list;
    struct Tree ids;
};
OFFSET_ASSERT(UIEntryQueueListBox, ids, 0xd0);
SIZE_ASSERT(UIEntryQueueListBox, 0xd8);
struct UIWindow;
void ui_entry_queue_list_box_destroy(struct UIWindow *window);
void ui_entry_queue_list_box_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_entry_queue_list_box_add_item(struct UIEntryQueueListBox *self, const char *text, int id);
int ui_entry_queue_list_box_get_item_id(struct UIEntryQueueListBox *self, int index);
const struct UIListBoxOps ui_entry_queue_list_box_ops = {
    .base = {
        .destroy = ui_entry_queue_list_box_destroy,
        .resize = ui_list_box_resize,
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
        .on_create = ui_list_box_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_list_box_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_list_box_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_entry_queue_list_box_on_l_btn_up,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_list_box_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .get_records = ui_window_get_records_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .create = ui_list_box_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_list_box_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

struct UIEntryQueueListBox *ui_entry_queue_list_box_construct(struct UIEntryQueueListBox *self)
{
    ui_list_box_construct(&self->list);
    self->list.base.ops = &ui_entry_queue_list_box_ops.base;
    tree_init(&self->ids);
    return self;
}

static void ui_entry_queue_list_box_destruct(struct UIEntryQueueListBox *self)
{
    self->list.base.ops = &ui_entry_queue_list_box_ops.base;
    tree_free(&self->ids, destroy_line);
    ui_list_box_destruct(&self->list);
}

void ui_entry_queue_list_box_destroy(struct UIWindow *window)
{
    struct UIEntryQueueListBox *self = (struct UIEntryQueueListBox *)window;
    ui_entry_queue_list_box_destruct(self);
    mem_free(self);
}

void ui_entry_queue_list_box_add_item(struct UIEntryQueueListBox *self, const char *text, int id)
{
    struct EntryQueueLine line;
    int added;

    string_init(&line.text);
    string_assign_cstr(&line.text, text);
    line.id = id;
    tree_add(&self->ids, &line, sizeof line, line_less, &added);
    if (added)
        ui_list_box_add_item(&self->list, text);
    else
        string_free(&line.text);
}

int ui_entry_queue_list_box_get_item_id(struct UIEntryQueueListBox *self, int index)
{
    const struct String *key = (const struct String *)self->list.items.first + index;
    const struct EntryQueueLine *line = tree_find(&self->ids, key, line_less);

    return line ? line->id : -1;
}

void ui_entry_queue_list_box_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIEntryQueueListBox *self = (struct UIEntryQueueListBox *)window;
    struct UIWindow *wnd = &self->list.base;

    if (self->list.selected != -1)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_LIST_SELECT, wnd->id, self->list.selected, 0, 0);
}

#define MSG_ENTRY_QUEUE_TITLE 0x603
#define MSG_ENTRY_QUEUE_LABEL_FIRST 0x85b
#define MSG_ENTRY_QUEUE_CONFIRM_ALONE 0x862
#define MSG_ENTRY_QUEUE_CONFIRM_PARTY 0x863
#define MSG_ENTRY_QUEUE_CONFIRM_GUILD 0x864
#define MSG_ENTRY_QUEUE_LIST_CAPTION 0x865
#define MSG_ENTRY_QUEUE_ROW_LIMIT 0x866
#define MSG_ENTRY_QUEUE_ROW_KIND1 0x867
#define MSG_ENTRY_QUEUE_ROW_KIND2 0x868
#define MSG_ENTRY_QUEUE_ROW_KIND3 0x869
#define MSG_ENTRY_QUEUE_ROW_KIND_OTHER 0x86a
#define MSG_ENTRY_REFUSED_MAP 0x86b
#define MSG_ENTRY_REFUSED_DELAY 0x86c
#define MSG_ENTRY_REFUSED_NO_PARTY 0x86d
#define MSG_ENTRY_REFUSED_PARTY_FLAG 0x86e
#define MSG_ENTRY_REFUSED_PARTY_SIZE 0x86f
#define MSG_ENTRY_REFUSED_NO_GUILD 0x870
#define MSG_ENTRY_REFUSED_GUILD_FLAG 0x871
#define MSG_ENTRY_REFUSED_GUILD_SIZE 0x872

#define ENTRY_QUEUE_ID_ALONE 0
#define ENTRY_QUEUE_ID_PARTY 1
#define ENTRY_QUEUE_ID_GUILD 2
#define ENTRY_QUEUE_ID_HELP 3
#define ENTRY_QUEUE_ROWS 7

#define UI_MSG_REBUILD 0x17

struct UIEntryQueueWnd {
    struct UIFrameWnd frame;
    struct UIBitmapButton *apply[3];
    struct UIBitmapButton *help;
    struct UIWindow *labels[ENTRY_QUEUE_ROWS];
    struct UIWindow *values[ENTRY_QUEUE_ROWS];
    struct UIEntryQueueListBox *list;
    struct WindowInfo *info;
};
OFFSET_ASSERT(UIEntryQueueWnd, labels, 0xa4);
OFFSET_ASSERT(UIEntryQueueWnd, values, 0xc0);
OFFSET_ASSERT(UIEntryQueueWnd, list, 0xdc);
SIZE_ASSERT(UIEntryQueueWnd, 0xe4);
void ui_entry_queue_wnd_destroy(struct UIWindow *window);
void ui_entry_queue_wnd_store_info(struct UIWindow *self);
void ui_entry_queue_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_entry_queue_wnd_on_draw(struct UIWindow *self);
int ui_entry_queue_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_entry_queue_wnd_ops = {
    .destroy = ui_entry_queue_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_entry_queue_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_entry_queue_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_entry_queue_wnd_on_draw,
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
    .send_msg = ui_entry_queue_wnd_send_msg,
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

#define EFST_ENTRY_QUEUE_APPLY_DELAY 0x29c

static int entry_queue_info_usable(const struct WindowInfo *info)
{
    if (info->width <= 0 || info->height <= 0)
        return 0;
    if (info->x < 0 || info->x > g_renderer->width - info->width)
        return 0;
    if (info->y < 0 || info->y > g_renderer->height - info->height)
        return 0;
    return 1;
}

static void entry_queue_info_reset(struct WindowInfo *info)
{
    info->width = 0x118;
    info->height = 0x168;
    info->x = 0x15e;
    info->y = 0x96;
}

static BYTE refuse(int msg)
{
    ui_window_mgr_message_box(&g_windowMgr, msg_str(msg), UI_MSGBOX_OK, 0, 0, 0, "Message", 0x118, 0x78, 0);
    return 0;
}

static BYTE may_apply(void)
{
    if (!g_entryQueueMgr || !g_entryQueueMgr->current) return 0;
    if (session_map_is_battlefield(&g_session1) == 1)
        return refuse(MSG_ENTRY_REFUSED_MAP);
    if (game_mode_has_state_icon(game_current_mode(), EFST_ENTRY_QUEUE_APPLY_DELAY))
        return refuse(MSG_ENTRY_REFUSED_DELAY);
    return 1;
}

static BYTE may_apply_alone(void)
{
    return may_apply();
}

static BYTE may_apply_party(void)
{
    if (!may_apply())
        return 0;
    if (session_get_party_member_count(&g_session1) == 0)
        return refuse(MSG_ENTRY_REFUSED_NO_PARTY);
    if (g_session1.partyLeader == 0)
        return refuse(MSG_ENTRY_REFUSED_PARTY_FLAG);
    if ((unsigned)g_entryQueueMgr->current->member_limit < (unsigned)session_count_online_party_members(&g_session1))
        return refuse(MSG_ENTRY_REFUSED_PARTY_SIZE);
    return 1;
}

static BYTE may_apply_guild(void)
{
    if (!may_apply())
        return 0;
    if (g_guildInfo.guild_id < 1)
        return refuse(MSG_ENTRY_REFUSED_NO_GUILD);
    if (g_guildInfo.is_master == 0)
        return refuse(MSG_ENTRY_REFUSED_GUILD_FLAG);
    if ((unsigned)g_entryQueueMgr->current->member_limit < g_guildInfo.online_count)
        return refuse(MSG_ENTRY_REFUSED_GUILD_SIZE);
    return 1;
}

static BYTE confirm(const char *question)
{
    struct EntryQueueInfo *queue = g_entryQueueMgr ? g_entryQueueMgr->current : NULL;
    char text[0x104];

    if (!queue)
        return 0;
    memset(text, 0, sizeof text);
    sprintf_s(text, sizeof text, "%s %s", queue->title, question);
    return ui_window_mgr_message_box(&g_windowMgr, text, UI_MSGBOX_QUESTION, 0, 0, 0, "Message", 0x118, 0x78,
                                  0) == UI_MSGBOX_ANSWER_YES;
}

static void show_queue(struct UIEntryQueueWnd *self, struct EntryQueueInfo *queue)
{
    struct UIWindow *wnd = &self->frame.base;
    char text[0x104];
    int row;

    if (!g_entryQueueMgr || !queue) return;
    g_entryQueueMgr->current = queue;
    memset(text, 0, sizeof text);
    for (row = 0; row < ENTRY_QUEUE_ROWS; row++) {
        struct UIWindow *value = self->values[row], *label = self->labels[row];

        if (!value)
            continue;
        switch (row) {
        case 0:
            strcpy_s(text, sizeof text, queue->title);
            break;
        case 1:
            strcpy_s(text, sizeof text, queue->text_row1);
            break;
        case 2:
            sprintf_s(text, sizeof text, msg_str(MSG_ENTRY_QUEUE_ROW_LIMIT), queue->member_limit, queue->enemy_limit);
            break;
        case 3:
            if (queue->kind == 1)
                sprintf_s(text, sizeof text, msg_str(MSG_ENTRY_QUEUE_ROW_KIND1), queue->kind_arg1);
            else if (queue->kind == 2)
                sprintf_s(text, sizeof text, msg_str(MSG_ENTRY_QUEUE_ROW_KIND2), queue->kind_arg1);
            else if (queue->kind == 3)
                sprintf_s(text, sizeof text, msg_str(MSG_ENTRY_QUEUE_ROW_KIND3), queue->kind_arg1, queue->kind_arg2);
            else
                sprintf_s(text, sizeof text, msg_str(MSG_ENTRY_QUEUE_ROW_KIND_OTHER));
            break;
        case 4:
            strcpy_s(text, sizeof text, queue->text_row4);
            break;
        case 5:
            strcpy_s(text, sizeof text, queue->text_row5);
            break;
        case 6:
            strcpy_s(text, sizeof text, queue->text_row6);
            break;
        }
        ui_window_resize(value, ui_window_text_width(wnd, text, 0, 0, 0xc, 0, 0), 0x19);
        ui_static_text_set_text(value, text, 0);
        ui_window_move(value, label->x + label->width + 5, label->y);
        ui_window_invalidate(value);
    }
    ui_window_invalidate_base(wnd);
}

struct UIEntryQueueWnd *ui_entry_queue_wnd_construct(struct UIEntryQueueWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_entry_queue_wnd_ops;
    self->list = NULL;
    self->info = NULL;
    memset(self->apply, 0, (BYTE *)&self->list - (BYTE *)self->apply);
    return self;
}

struct UIWindow *ui_entry_queue_wnd_new(void)
{
    return (struct UIWindow *)ui_entry_queue_wnd_construct(mem_alloc(sizeof(struct UIEntryQueueWnd)));
}

static void ui_entry_queue_wnd_destruct(struct UIEntryQueueWnd *self)
{
    self->frame.base.ops = &ui_entry_queue_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_entry_queue_wnd_destroy(struct UIWindow *window)
{
    struct UIEntryQueueWnd *self = (struct UIEntryQueueWnd *)window;
    ui_entry_queue_wnd_destruct(self);
    mem_free(self);
}

static const char *const apply_bitmaps[3][3] = {
    { BATTLE_FIELD "apply_individual.bmp", BATTLE_FIELD "apply_individual_a.bmp",
      BATTLE_FIELD "apply_individual_b.bmp" },
    { BATTLE_FIELD "apply_party.bmp", BATTLE_FIELD "apply_party_a.bmp", BATTLE_FIELD "apply_party_b.bmp" },
    { BATTLE_FIELD "apply_guild.bmp", BATTLE_FIELD "apply_guild_a.bmp", BATTLE_FIELD "apply_guild_b.bmp" },
};

void ui_entry_queue_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIEntryQueueWnd *self = (struct UIEntryQueueWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIEntryQueueListBox *list;
    size_t queue_index;
    const char *labels[ENTRY_QUEUE_ROWS];
    int row, i;

    ui_window_resize_base(wnd, 0x15e, 0x15e);
    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "", 0);

    list = mem_alloc(sizeof *list);
    if (list)
        list = ui_entry_queue_list_box_construct(list);
    self->list = list;
    ui_window_create(&list->list.base, 0x82, 0x10e);
    ui_window_move_base(&list->list.base, 0xa, 0x28);
    list->list.r = 0xf0;
    list->list.g = 0xf0;
    list->list.b = 0xf0;
    ui_window_add_child(wnd, &list->list.base);
    for (queue_index = 0; queue_index < entry_queue_count(); ++queue_index) {
        struct EntryQueueInfo *queue = entry_queue_at(queue_index);
        ui_entry_queue_list_box_add_item(self->list, queue->title, queue->id);
    }

    if (g_entryQueueMgr) g_entryQueueMgr->current = entry_queue_at(0);

    for (row = 0; row < ENTRY_QUEUE_ROWS; row++)
        labels[row] = msg_str(MSG_ENTRY_QUEUE_LABEL_FIRST + row);
    for (row = 0; row < ENTRY_QUEUE_ROWS; row++) {
        struct UIWindow *label = mem_alloc(sizeof(struct UIStaticText));

        if (label)
            label = ui_static_text_construct(label);
        self->labels[row] = label;
        if (!label)
            continue;
        ui_window_create(label, ui_window_text_width(wnd, labels[row], 0, 0, 0xc, 0, 0), 0x19);
        ui_static_text_set_text(label, labels[row], 0);
        ui_window_move(label, 0x9d, 0xa5 + row * 0x14);
        ui_window_add_child(wnd, label);
    }
    for (row = 0; row < ENTRY_QUEUE_ROWS; row++) {
        struct UIWindow *value = mem_alloc(sizeof(struct UIStaticText));

        if (value)
            value = ui_static_text_construct(value);
        self->values[row] = value;
        if (!self->labels[row])
            continue;
        ui_window_create(value, 0x9d, 0x19);
        ui_window_add_child(wnd, value);
    }
    show_queue(self, entry_queue_at(0));

    for (i = 0; i < 3; i++)
        self->apply[i] = ui_add_bitmap_button(wnd, apply_bitmaps[i][0], apply_bitmaps[i][1], apply_bitmaps[i][2],
                                    8 + i * 0x5d, height - 0x18, ENTRY_QUEUE_ID_ALONE + i);
    self->help = ui_add_bitmap_button(wnd, BATTLE_FIELD "btn_help.bmp", BATTLE_FIELD "btn_help_a.bmp",
                            BATTLE_FIELD "btn_help_b.bmp", width - 0x3e, height - 0x18, ENTRY_QUEUE_ID_HELP);
}

void ui_entry_queue_wnd_on_draw(struct UIWindow *window)
{
    struct UIEntryQueueWnd *self = (struct UIEntryQueueWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct EntryQueueInfo *queue = g_entryQueueMgr ? g_entryQueueMgr->current : NULL;
    const char *caption;

    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_ENTRY_QUEUE_TITLE), 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    if (queue) {
        struct String path;
        void *res;

        string_init(&path);
        string_assign(&path, BATTLE_FIELD, sizeof BATTLE_FIELD - 1);
        string_append_cstr(&path, queue->picture);
        res = ui_skin_bitmap(string_cstr(&path));
        string_free(&path);
        if (res)
            ui_window_draw_bitmap(wnd, 0x9d, 0x19, res, 0);
    }
    caption = msg_str(MSG_ENTRY_QUEUE_LIST_CAPTION);
    ui_window_draw_shadow_text(wnd, 0xa, 0x17, msg_str(MSG_ENTRY_QUEUE_LIST_CAPTION), (int)strlen(caption), 0,
                            0xc0c0c0, 0, 0xc, 0);
}

int ui_entry_queue_wnd_get_selected(struct UIEntryQueueWnd *self)
{
    return self->list ? self->list->list.selected : -1;
}

void ui_entry_queue_wnd_store_info(struct UIWindow *window)
{
    struct UIEntryQueueWnd *self = (struct UIEntryQueueWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
}

int ui_entry_queue_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UIEntryQueueWnd *self = (struct UIEntryQueueWnd *)window;
    struct EntryQueueInfo *queue = g_entryQueueMgr ? g_entryQueueMgr->current : NULL;

    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case ENTRY_QUEUE_ID_ALONE:
            if (may_apply_alone() && confirm(msg_str(MSG_ENTRY_QUEUE_CONFIRM_ALONE)))
                entry_queue_mgr_apply(g_entryQueueMgr, ENTRY_APPLY_ALONE, queue->name);
            break;
        case ENTRY_QUEUE_ID_PARTY:
            if (may_apply_party() && confirm(msg_str(MSG_ENTRY_QUEUE_CONFIRM_PARTY)))
                entry_queue_mgr_apply(g_entryQueueMgr, ENTRY_APPLY_PARTY, queue->name);
            break;
        case ENTRY_QUEUE_ID_GUILD:
            if (may_apply_guild() && confirm(msg_str(MSG_ENTRY_QUEUE_CONFIRM_GUILD)))
                entry_queue_mgr_apply(g_entryQueueMgr, ENTRY_APPLY_GUILD, queue->name);
            break;
        case ENTRY_QUEUE_ID_HELP:
            if (!ui_close_window(ENTRY_QUEUE_HELP_WND_ID))
                ui_make_window(ENTRY_QUEUE_HELP_WND_ID);
            break;
        case UI_ID_CLOSE:
            ui_window_record_command(&self->frame.base, val1, val2, val3, val4);
            ui_window_mgr_post_quit1(&g_windowMgr, ENTRY_QUEUE_WND_ID);
            break;
        }
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!entry_queue_info_usable(self->info))
            entry_queue_info_reset(self->info);
        ui_entry_queue_wnd_send_msg(&self->frame.base, NULL, UI_MSG_REBUILD, 0, 0, 0, 0);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;
    case UI_MSG_LIST_SELECT:
        if (self->list) {
            unsigned id = (unsigned)ui_entry_queue_list_box_get_item_id(self->list, val2);
            struct EntryQueueInfo *chosen = entry_queue_find(id);

            if (chosen)
                show_queue(self, chosen);
        }
        return 0;
    }
    return 0;
}
