#include "ui/controls/scroll.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "contents/connection_api.h"
#include <stdio.h>
#include "ui/quests/quests.h"
#include "ui/quests/quest_journal_layout.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define QUEST_WND_ID 0x141
#define QUEST_SIMPLE_VIEW_WND_ID 0xcc

#define QUEST_UI_DIR UI_SKIN "renew_questui\\"
#define QUEST_ICON_NONE QUEST_UI_DIR "ico_nq.bmp"

#define QUEST_ROWS 9
#define QUEST_ROW_HEIGHT 0x2e
#define QUEST_LIST_X 0x26
#define QUEST_LIST_Y 0x18
#define QUEST_LIST_W 0x14a
#define QUEST_LIST_H 0x19e
#define QUEST_DRAG_HEIGHT 0x11

#define QUEST_ROW_ID_FIRST 0x17d
#define QUEST_SIMPLE_VIEW_BOX_ID UI_ID_CHECKBOX

#define QUEST_BOX_SHOWN 0
#define QUEST_BOX_LOCK 1

#define MSG_QUEST_WND_TITLE 0xd48
#define MSG_QUEST_SIMPLE_VIEW 0x7e3
#define MSG_QUEST_SHOWN_TIP 0xd51
#define MSG_QUEST_LOCK_TIP 0xd4f

#define QUEST_MSG_SET_TAB 0x16
#define QUEST_MSG_REFRESH 0x17
#define UI_TAB_MSG_SELECT 0xd

#define CZ_ACTIVE_QUEST_ID      0x2b6

#pragma pack(push, 1)
struct PACKET_CZ_ACTIVE_QUEST {
    WORD packet_type;
    DWORD quest_id;
    BYTE active;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_CZ_ACTIVE_QUEST, 7);

struct UIRenewQuestWnd;
struct UIWindow;
void ui_renew_quest_wnd_destroy(struct UIWindow *window);
void ui_renew_quest_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_renew_quest_wnd_on_draw(struct UIWindow *self);
void ui_renew_quest_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_renew_quest_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_renew_quest_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_renew_quest_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_renew_quest_wnd_ops = {
    .destroy = ui_renew_quest_wnd_destroy,
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
    .on_create = ui_renew_quest_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_renew_quest_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_renew_quest_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_renew_quest_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_renew_quest_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_renew_quest_wnd_send_msg,
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

struct UIRenewQuestWnd {
    struct UIFrameWnd frame;
    struct UIWindow *tabs;
    struct UIScrollBar *vbar;
    struct UIWindow *simple_view_box;
    struct UIWindow *close;
    struct UIWindow *shown_boxes[QUEST_ROWS];
    struct UIWindow *lock_boxes[QUEST_ROWS];
    struct UIWindow *icons[QUEST_ROWS];
    int tab;
    int top;
    int selected;
    BYTE scrollable;
    int pressed;
    struct Vec lists[QUEST_TAB_COUNT];
};
OFFSET_ASSERT(UIRenewQuestWnd, tab, 0x110);
OFFSET_ASSERT(UIRenewQuestWnd, lists, 0x124);
SIZE_ASSERT(UIRenewQuestWnd, 0x148);

static unsigned quest_wnd_count(struct UIRenewQuestWnd *self, int tab)
{
    return VEC_COUNT(&self->lists[tab], int);
}

static int quest_wnd_id(struct UIRenewQuestWnd *self, int tab, int index)
{
    return ((int *)self->lists[tab].first)[index];
}

static void quest_wnd_show_selected(struct UIRenewQuestWnd *self)
{
    struct UIRenewDetailQuestInfoWnd *detail =
        (struct UIRenewDetailQuestInfoWnd *)ui_make_window(QUEST_DETAIL_WND_ID);
    int id;

    if (!detail)
        return;
    id = self->lists[self->tab].first == self->lists[self->tab].last ? 0 : quest_wnd_id(self, self->tab, self->selected);
    ui_renew_detail_quest_info_wnd_show_quest(detail, id);
}

struct UIRenewQuestWnd *ui_renew_quest_wnd_construct(struct UIRenewQuestWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_renew_quest_wnd_ops;
    self->tabs = NULL;
    self->vbar = NULL;
    self->simple_view_box = NULL;
    self->close = NULL;
    self->tab = 0;
    self->top = 0;
    self->selected = 0;
    self->scrollable = 0;
    self->pressed = 0;
    for (i = 0; i < QUEST_TAB_COUNT; i++)
        self->lists[i].first = self->lists[i].last = self->lists[i].end = NULL;
    memset(self->shown_boxes, 0, sizeof self->shown_boxes + sizeof self->lock_boxes + sizeof self->icons);
    vec_clear(&self->lists[0]);
    vec_clear(&self->lists[1]);
    vec_clear(&self->lists[2]);
    return self;
}

struct UIWindow *ui_renew_quest_wnd_new(void)
{
    return (struct UIWindow *)ui_renew_quest_wnd_construct(mem_alloc(sizeof(struct UIRenewQuestWnd)));
}

static void ui_renew_quest_wnd_destruct(struct UIRenewQuestWnd *self)
{
    int i;

    self->frame.base.ops = &ui_renew_quest_wnd_ops;
    ui_window_mgr_post_quit1(&g_windowMgr, QUEST_DETAIL_WND_ID);
    for (i = QUEST_TAB_COUNT - 1; i >= 0; i--)
        vec_free(&self->lists[i]);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_renew_quest_wnd_destroy(struct UIWindow *window)
{
    struct UIRenewQuestWnd *self = (struct UIRenewQuestWnd *)window;
    ui_renew_quest_wnd_destruct(self);
    mem_free(self);
}

static struct UIWindow *quest_wnd_add_close_button(struct UIRenewQuestWnd *self)
{
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, UI_SKIN "basic_interface\\btn_close.bmp", 0, 0);
    ui_bitmap_button_set_bitmap(button, UI_SKIN "basic_interface\\btn_close_a.bmp", 1, 0);
    ui_bitmap_button_set_bitmap(button, UI_SKIN "basic_interface\\btn_close_b.bmp", 2, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, 0x147, 0x1ba);
    button->button.base.id = UI_ID_CLOSE;
    ui_window_add_child(&self->frame.base, &button->button.base);
    return &button->button.base;
}

static struct UIScrollBar *quest_wnd_add_scroll_bar(struct UIRenewQuestWnd *self)
{
    struct UIScrollBar *bar = mem_alloc(sizeof(struct UIScrollBar));

    if (bar) {
        ui_scroll_bar_construct(bar);
        bar->base.ops = &ui_renew_quest_scroll_bar_ops;
        bar->button_size = 3;
        bar->thickness = 7;
    }
    ui_scroll_bar_create(bar, 0x176, 0x11, 1, 0x1a6);
    self->top = 0;
    ui_window_add_child(&self->frame.base, &bar->base);
    return bar;
}

void ui_renew_quest_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIRenewQuestWnd *self = (struct UIRenewQuestWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *tabs = mem_alloc(sizeof(struct UITab));
    int i, row_y;

    if (tabs)
        tabs = (struct UIWindow *)ui_tab_construct((struct UITab *)tabs);
    self->tabs = tabs;
    ui_window_create(tabs, QUEST_TAB_WIDTH, QUEST_TAB_HEIGHT * QUEST_TAB_COUNT);
    ((struct UITab *)(self->tabs))->count = QUEST_TAB_COUNT;
    ui_window_move_base(self->tabs, 0, QUEST_TAB_Y);
    ((struct UITab *)(self->tabs))->kind = -1;
    ui_tab_send_msg(self->tabs, wnd, UI_TAB_MSG_SELECT, self->tab, 0, 0, 0);
    ui_window_add_child(wnd, self->tabs);

    if (!g_windowMgr.quest_display_wnd)
        ui_make_window(QUEST_SIMPLE_VIEW_WND_ID);
    self->simple_view_box = ui_window_add_check_box(wnd, QUEST_SIMPLE_VIEW_BOX_ID, 0xa, 0xc,
                                                 msg_str(MSG_QUEST_SIMPLE_VIEW), 0x26, 0x1bf,
                                                 g_windowMgr.quest_display_wnd->visible != 0, NULL, 0xc, 0x212121,
                                                 QUEST_UI_DIR "checkbox_on.bmp", QUEST_UI_DIR "checkbox_off.bmp");
    self->close = quest_wnd_add_close_button(self);
    self->vbar = quest_wnd_add_scroll_bar(self);

    for (i = 0, row_y = 0; i < QUEST_ROWS; i++, row_y += QUEST_ROW_HEIGHT) {
        struct UIWindow *box, *icon;

        box = ui_window_add_check_box(wnd, QUEST_ROW_ID_FIRST + i, 0xe, 0xe, NULL, 0x15e, 0x1b + row_y, 1, NULL,
                                   0, 0, QUEST_UI_DIR "bt_check_on.bmp", QUEST_UI_DIR "bt_check_off.bmp");
        self->shown_boxes[i] = box;
        ((struct UICheckBox *)(box))->notice_value = QUEST_BOX_SHOWN;
        ui_check_box_set_tool_tip(self->shown_boxes[i], msg_str(MSG_QUEST_SHOWN_TIP));
        ui_check_box_set_hidden(self->shown_boxes[i], 1);

        box = ui_window_add_check_box(wnd, QUEST_ROW_ID_FIRST + i, 0x14, 0x11, NULL, 0x158, 0x2e + row_y, 1, NULL,
                                   0, 0, QUEST_UI_DIR "bt_lock_open.bmp", QUEST_UI_DIR "bt_lock.bmp");
        self->lock_boxes[i] = box;
        ((struct UICheckBox *)(box))->notice_value = QUEST_BOX_LOCK;
        ui_check_box_set_hidden(self->lock_boxes[i], 1);

        icon = mem_alloc(sizeof(struct UIStaticBitmap));
        if (icon)
            icon = (struct UIWindow *)ui_static_bitmap_construct((struct UIStaticBitmap *)icon);
        self->icons[i] = icon;
        ui_static_bitmap_set_bitmap((struct UIStaticBitmap *)icon, QUEST_ICON_NONE);
        ui_window_create(self->icons[i], self->icons[i]->width, self->icons[i]->height);
        ui_window_move(self->icons[i], 0x2e, 0x1f + row_y);
        ui_window_set_show(self->icons[i], 0);
        ui_window_add_child(wnd, self->icons[i]);
    }
    ui_renew_quest_wnd_send_msg(&self->frame.base, wnd, QUEST_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_renew_quest_wnd_on_draw(struct UIWindow *window)
{
    struct UIRenewQuestWnd *self = (struct UIRenewQuestWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int i, row_y;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    quest_journal_draw_background(wnd, self->tab);
    ui_window_text_out(wnd, 0x2d, 2, msg_str(MSG_QUEST_WND_TITLE), 0, 0, 13, 0, 0, 0);

    for (i = 0, row_y = 0; row_y < QUEST_ROWS * QUEST_ROW_HEIGHT; i++, row_y += QUEST_ROW_HEIGHT) {
        int index = self->top + i;
        const struct RenewQuestBase *info;
        const struct RenewQuestB *quest;
        COLORREF title_colour;
        int y = row_y;

        if ((unsigned)index >= quest_wnd_count(self, self->tab))
            return;
        quest = renew_quest_find_ongoing(g_questTable, quest_wnd_id(self, self->tab, index));
        if (!quest)
            return;
        info = &quest->base;

        if (index == self->selected) {
            y += self->pressed;
            ui_window_draw_bitmap(wnd, QUEST_LIST_X, QUEST_LIST_Y + y,
                                ui_skin_bitmap(self->pressed ? QUEST_UI_DIR "bg_questList_press.bmp"
                                                            : QUEST_UI_DIR "bg_questList_check.bmp"),
                                1);
            title_colour = 0xe47b2e;
        } else {
            ui_window_draw_bitmap(wnd, QUEST_LIST_X, QUEST_LIST_Y + y, ui_skin_bitmap(QUEST_UI_DIR "bg_questList.bmp"),
                                1);
            title_colour = 0x955322;
        }
        ui_window_move(self->icons[i], 0x2e, 0x1f + y);
        ui_window_text_out(wnd, 0x50, 0x1e + y, string_cstr(&info->text[RENEW_QUEST_TEXT_TITLE]), 0, 0, 12, title_colour, 1, 0);
        ui_window_text_out(wnd, 0x50, 0x30 + y, string_cstr(&info->text[RENEW_QUEST_TEXT_SUMMARY]), 0, 0, 12, 0x38302e, 0, 0);
    }
}

void ui_renew_quest_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIRenewQuestWnd *self = (struct UIRenewQuestWnd *)window;
    int index;

    if (y < QUEST_DRAG_HEIGHT) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if ((unsigned)(x - QUEST_LIST_X) > QUEST_LIST_W || (unsigned)(y - QUEST_LIST_Y) > QUEST_LIST_H)
        return;
    index = (y - QUEST_LIST_Y) / QUEST_ROW_HEIGHT + self->top;
    if (quest_wnd_count(self, self->tab) <= (unsigned)index)
        return;
    self->pressed = 1;
    self->selected = index;
    ui_window_invalidate_base(&self->frame.base);
    quest_wnd_show_selected(self);
}

void ui_renew_quest_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIRenewQuestWnd *self = (struct UIRenewQuestWnd *)window;
    self->pressed = 0;
    ui_window_invalidate_base(&self->frame.base);
    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
}

void ui_renew_quest_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIRenewQuestWnd *self = (struct UIRenewQuestWnd *)window;
    if (!self->scrollable)
        return;
    ui_renew_quest_wnd_send_msg(&self->frame.base, &self->vbar->base, UI_MSG_SCROLL_V, delta < 0 ? 3 : -3, 0, 0, 0);
}

static void quest_wnd_set_icon(struct UIWindow *icon, const struct RenewQuestBase *info)
{
    char path[MAX_PATH];

    if (!info->text[RENEW_QUEST_TEXT_ICON].size) {
        strcpy(path, QUEST_ICON_NONE);
    } else {
        _snprintf(path, sizeof path, QUEST_UI_DIR "%s", string_cstr(&info->text[RENEW_QUEST_TEXT_ICON]));
        path[sizeof path - 1] = 0;
    }
    ui_static_bitmap_set_bitmap((struct UIStaticBitmap *)icon, path);
    ui_static_bitmap_set_tool_tip(icon, string_cstr(&info->text[RENEW_QUEST_TEXT_ICON_TIP]));
    ui_window_set_show(icon, 1);
    ui_window_invalidate(icon);
}

static void quest_wnd_set_box_state(struct UIWindow *box, BYTE checked)
{
    box->state = checked;
    ui_window_invalidate(box);
}

static void ui_renew_quest_wnd_update_rows(struct UIRenewQuestWnd *self)
{
    int i;

    for (i = 0; i < QUEST_ROWS; i++) {
        unsigned index = self->top + i;
        struct UIWindow *shown = self->shown_boxes[i], *lock = self->lock_boxes[i];
        struct RenewQuestB *info;

        if (index >= quest_wnd_count(self, self->tab)) {
            ui_check_box_set_hidden(shown, 1);
            ui_check_box_set_hidden(lock, 1);
            ui_window_set_show(self->icons[i], 0);
            continue;
        }
        info = renew_quest_ongoing_info(g_questTable, quest_wnd_id(self, self->tab, index));

        if (self->tab == QUEST_TAB_OTHER) {
            ui_check_box_set_hidden(shown, 1);
            ui_check_box_set_hidden(lock, 1);
        } else {
            quest_wnd_set_box_state(lock, info->base.active);
            ui_check_box_set_tool_tip(lock, msg_str(MSG_QUEST_LOCK_TIP));
            ui_check_box_set_hidden(lock, 0);
            quest_wnd_set_box_state(shown, info->shown);
            ui_check_box_set_hidden(shown, 0);
        }
        quest_wnd_set_icon(self->icons[i], &info->base);
    }
}

static void ui_renew_quest_wnd_update_scroll(struct UIRenewQuestWnd *self)
{
    unsigned count = quest_wnd_count(self, self->tab);

    if (count && self->vbar) {
        int range = (int)count - QUEST_ROWS > 0 ? (int)count - QUEST_ROWS : 0;

        self->scrollable = range > 0;
        if (range > 0) {
            ui_scroll_bar_set_range(self->vbar, range, QUEST_ROWS, count);
            if (self->top < 0)
                self->top = 0;
            if (self->top > range)
                self->top = range;
            ui_scroll_bar_set_pos(self->vbar, self->top);
        } else {
            self->top = 0;
            ui_scroll_bar_set_range(self->vbar, 0, QUEST_ROWS, count);
        }
        ui_window_invalidate_base(&self->vbar->base);
    }
    ui_renew_quest_wnd_update_rows(self);
}

static void ui_renew_quest_wnd_rebuild_lists(struct UIRenewQuestWnd *self, int quest_id)
{
    struct RenewQuestMgr *table = g_questTable;
    struct TreeNode *node;
    unsigned index, count;

    vec_clear(&self->lists[QUEST_TAB_ACTIVE]);
    vec_clear(&self->lists[QUEST_TAB_PAUSED]);
    vec_clear(&self->lists[QUEST_TAB_OTHER]);
    for (node = tree_first(&table->quests_b); node != table->quests_b.head; node = tree_next(node)) {
        struct OngoingQuestEntry *entry = TREE_VALUE(node, struct OngoingQuestEntry);
        int tab;

        if (entry->quest.cool_time_quest == 1 && entry->quest.cool_time_quest_high == 0)
            tab = QUEST_TAB_OTHER;
        else if (entry->quest.base.active == 1)
            tab = QUEST_TAB_ACTIVE;
        else
            tab = QUEST_TAB_PAUSED;
        vec_push(&self->lists[tab], &entry->id, sizeof(int));
    }

    if (!quest_id) {
        self->selected = 0;
        return;
    }
    count = quest_wnd_count(self, self->tab);
    for (index = 0; index < count && quest_wnd_id(self, self->tab, index) != quest_id; index++)
        ;

    self->selected = index;
    if ((int)index >= QUEST_ROWS) {
        self->top = index - (QUEST_ROWS - 1);
        ui_renew_quest_wnd_update_scroll(self);
        ui_window_invalidate_base(&self->frame.base);
    }
    quest_wnd_show_selected(self);
}

static void quest_wnd_on_row_box(struct UIRenewQuestWnd *self, struct UIWindow *box, int row, int checked)
{
    int quest_id = quest_wnd_id(self, self->tab, self->top + row);

    if (((struct UICheckBox *)(box))->notice_value == QUEST_BOX_SHOWN) {
        renew_quest_set_shown(g_questTable, quest_id, checked != 0);
        renew_quest_save_simple_view_list(g_questTable);
        if (g_windowMgr.quest_display_wnd)
            ui_window_invalidate(g_windowMgr.quest_display_wnd);
    } else {
        struct PACKET_CZ_ACTIVE_QUEST packet;
        int len;

        packet.packet_type = CZ_ACTIVE_QUEST_ID;
        packet.quest_id = quest_id;
        packet.active = checked != 0;
        len = rag_connection_get_packet_length(rag_connection_instance(), CZ_ACTIVE_QUEST_ID);
        rag_connection_send_packet(rag_connection_instance(), len, &packet);
    }
}

static int quest_wnd_refresh(struct UIRenewQuestWnd *self, int quest_id)
{
    if (quest_id) {
        int tab;
        unsigned i;

        for (tab = QUEST_TAB_ACTIVE; tab < QUEST_TAB_COUNT; tab++) {
            for (i = 0; i < quest_wnd_count(self, tab); i++)
                if (quest_wnd_id(self, tab, i) == quest_id)
                    break;
            if (i < quest_wnd_count(self, tab))
                break;
        }
        if (tab == QUEST_TAB_COUNT) {
            ui_window_mgr_message_box(&g_windowMgr, "not found quest info", 0, 1, 0, 0,
                                   "메시지", 0x118, 0x78, 0);
            return 0;
        }
        ui_renew_quest_wnd_send_msg(&self->frame.base, &self->frame.base, QUEST_MSG_SET_TAB, tab, 0, 0, 0);
    }
    ui_renew_quest_wnd_rebuild_lists(self, quest_id);
    ui_renew_quest_wnd_update_scroll(self);
    ui_window_invalidate_base(&self->frame.base);
    return 0;
}

int ui_renew_quest_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UIRenewQuestWnd *self = (struct UIRenewQuestWnd *)window;
    int old_top;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CLOSE) {
            ui_window_mgr_post_quit1(&g_windowMgr, QUEST_WND_ID);
        } else if (val1 == QUEST_SIMPLE_VIEW_BOX_ID) {
            if (g_windowMgr.quest_display_wnd)
                ui_window_set_show(g_windowMgr.quest_display_wnd, val2);
        } else if (val1 >= QUEST_ROW_ID_FIRST && val1 < QUEST_ROW_ID_FIRST + QUEST_ROWS && sender) {
            quest_wnd_on_row_box(self, sender, val1 - QUEST_ROW_ID_FIRST, val2);
        }
        return 0;

    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        old_top = self->top;
        if (msg == UI_MSG_PAGE_UP)
            self->top = old_top - (QUEST_ROWS - 1);
        else if (msg == UI_MSG_PAGE_DOWN)
            self->top = old_top + (QUEST_ROWS - 1);
        else
            self->top = old_top + val1;
        if (self->top == old_top)
            return 0;
        ui_renew_quest_wnd_update_scroll(self);
        ui_window_invalidate_base(&self->frame.base);
        return self->top - old_top;

    case QUEST_MSG_SET_TAB:
        if (self->tab == val1)
            return 0;
        self->tab = val1;
        if (val1 < 0)
            self->tab = 0;
        else if (val1 > QUEST_TAB_COUNT - 1)
            self->tab = QUEST_TAB_COUNT - 1;
        self->selected = 0;
        self->top = 0;
        ui_renew_quest_wnd_update_scroll(self);
        ui_window_invalidate_base(&self->frame.base);
        return 0;

    case QUEST_MSG_REFRESH:
        return quest_wnd_refresh(self, val2);
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
