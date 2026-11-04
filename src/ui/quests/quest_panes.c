#include "ui/controls/scroll.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include "ui/quests/quests.h"
#include "ui/quests/quests_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define QUEST_MSG_REFRESH 0x17

#define QUEST_UI_DIR UI_SKIN "renew_questui\\"
#define QUEST_HEADING_BMP QUEST_UI_DIR "img_poring.bmp"
#define QUEST_LINE_BMP QUEST_UI_DIR "img_line.bmp"
#define QUEST_REWARD_BMP QUEST_UI_DIR "img_questIocn.bmp"
#define ITEM_BMP_FMT UI_SKIN "item\\%s.bmp"
#define ITEM_FALLBACK_RESOURCE "사과"

#define MSG_QUEST_DESCRIPTION 0xd49
#define MSG_QUEST_HUNTS 0xd4a
#define MSG_QUEST_REWARD_HEADING 0xd4b
#define MSG_QUEST_BASE_EXP 0xd4c
#define MSG_QUEST_JOB_EXP 0xd4d
#define MSG_QUEST_REWARDS 0xd4e
#define MSG_QUEST_HUNT_DONE 0x7ee
#define MSG_REWARD_COUNT 0xb6

#define HUNT_UP_TO_FMT "%s Lv.%d▼"
#define HUNT_FROM_FMT "%s Lv.%d▲"

#define QUEST_TEXT_R 0x1d
#define QUEST_TEXT_G 0x55
#define QUEST_TEXT_B 0xa0

struct Vec;
struct UIOngoingQuestScrollWnd;
struct UIScrollWnd;
struct UIWindow;
void ui_ongoing_quest_scroll_wnd_destroy(struct UIWindow *window);
void ui_ongoing_quest_scroll_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_ongoing_quest_scroll_wnd_on_draw(struct UIWindow *self);
void ui_ongoing_quest_scroll_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_ongoing_quest_scroll_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_ongoing_quest_scroll_wnd_ops = {
    .destroy = ui_ongoing_quest_scroll_wnd_destroy,
    .resize = ui_scroll_wnd_resize,
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
    .on_create = ui_ongoing_quest_scroll_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_ongoing_quest_scroll_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_ongoing_quest_scroll_wnd_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_ongoing_quest_scroll_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_scroll_wnd_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_scroll_wnd_make_quad,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
struct UIRecommededQuestScrollWnd;
void ui_recommeded_quest_scroll_wnd_destroy(struct UIWindow *window);
void ui_recommeded_quest_scroll_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_recommeded_quest_scroll_wnd_on_draw(struct UIWindow *self);
void ui_recommeded_quest_scroll_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_recommeded_quest_scroll_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_recommeded_quest_scroll_wnd_ops = {
    .destroy = ui_recommeded_quest_scroll_wnd_destroy,
    .resize = ui_scroll_wnd_resize,
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
    .on_create = ui_recommeded_quest_scroll_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_recommeded_quest_scroll_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_recommeded_quest_scroll_wnd_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_recommeded_quest_scroll_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_scroll_wnd_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_scroll_wnd_make_quad,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIOngoingQuestScrollWnd {
    struct UIScrollWnd scroll;
    struct Vec hunt_rows;
    int quest_id;
    int line_height;
};
SIZE_ASSERT(UIOngoingQuestScrollWnd, 0xc4);

struct UIRecommededQuestScrollWnd {
    struct UIScrollWnd scroll;
    int quest_id;
    int line_height;
};
SIZE_ASSERT(UIRecommededQuestScrollWnd, 0xb8);

static int pane_text_width(struct UIScrollWnd *pane, const char *text)
{
    return ui_window_text_width(&pane->base, text, 0, 0, 12, 1, 0);
}

static struct UIWindow *pane_add_bitmap(struct UIScrollWnd *pane, const char *bitmap, int x, int y)
{
    struct UIWindow *pic = mem_alloc(sizeof(struct UIStaticBitmap));

    if (pic)
        pic = (struct UIWindow *)ui_static_bitmap_construct((struct UIStaticBitmap *)pic);
    ui_static_bitmap_set_bitmap((struct UIStaticBitmap *)pic, bitmap);
    ui_window_create(pic, pic->width, pic->height);
    ui_window_move_base(pic, x, y);
    ui_scroll_wnd_add_child(pane, pic);
    return pic;
}

static struct UIWindow *pane_add_label(struct UIScrollWnd *pane, const char *text, int x, int y)
{
    struct UIWindow *label = mem_alloc(sizeof(struct UIStaticText));

    if (label)
        label = ui_static_text_construct(label);
    ui_window_create(label, pane_text_width(pane, text) + 4, 12);
    ui_static_text_set_text(label, text, 0);
    ui_window_move(label, x, y);
    ui_scroll_wnd_add_child(pane, label);
    return label;
}

static void pane_add_blue_label(struct UIScrollWnd *pane, const char *text, int x, int y)
{
    struct UIWindow *label = pane_add_label(pane, text, x, y);
    ui_static_text_set_text_colour(label, QUEST_TEXT_R, QUEST_TEXT_G, QUEST_TEXT_B);
}

static int pane_add_heading(struct UIScrollWnd *pane, int msg, int y)
{
    pane_add_bitmap(pane, QUEST_HEADING_BMP, 9, y);
    pane_add_label(pane, msg_str(msg), 0x18, y);
    return y + 0x16;
}

static int pane_add_divider(struct UIScrollWnd *pane, int y)
{
    struct UIWindow *line = pane_add_bitmap(pane, QUEST_LINE_BMP, -1, y);
    return y + line->height + 8;
}

static struct UIWindow *pane_make_npc(const struct RenewQuestBase *npc)
{
    struct UINpcSprWnd *spr = ui_npc_spr_wnd_new();
    struct String copy;

    string_copy(&copy, &npc->text[RENEW_QUEST_TEXT_NPC_SPRITE]);
    ui_npc_spr_wnd_create(spr, 0x4b, 0x91, copy);
    return (struct UIWindow *)spr;
}

static void pane_set_npc_target(struct UIScrollWnd *pane, struct UIWindow *spr, const struct RenewQuestBase *npc)
{
    if (npc->text[RENEW_QUEST_TEXT_NPC_MAP].size) {
        struct String copy;
        string_copy(&copy, &npc->text[RENEW_QUEST_TEXT_NPC_MAP]);
        ui_npc_spr_wnd_set_target((struct UINpcSprWnd *)spr, copy, npc->npc_x, npc->npc_y);
    }
    ui_scroll_wnd_add_child(pane, spr);
}

static struct UITagTextViewer *pane_add_viewer(int width, int height, int x, int y)
{
    struct UITagTextViewer *view = ui_tag_text_viewer_create(width, height);

    ui_window_move_base(&view->base, x, y);
    return view;
}

static void viewer_add_lines(struct UITagTextViewer *view, const struct Vec *lines)
{
    const struct String *line;

    for (line = lines->first; line != lines->last; line++)
        ui_tag_text_viewer_append_text(view, string_cstr(line));
}

static void pane_on_wheel(struct UIScrollWnd *pane, int delta)
{
    if (pane->vscroll_on != 1)
        return;
    ui_window_send_msg(&pane->base, &pane->vbar->base, UI_MSG_SCROLL_V, delta < 0 ? 15 : -15, 0, 0, 0);
}

static void pane_on_draw(struct UIScrollWnd *pane)
{
    ui_window_clear_canvas(&pane->base, 0xffff00ff);
    if (pane->base.parent)
        ui_window_invalidate(pane->base.parent);
}

static int pane_send_msg(struct UIScrollWnd *pane, struct UIWindow *sender, int msg, int val1, int val2, int val3,
                        int val4)
{
    if (msg == UI_MSG_COMMAND && val1 == UI_ID_CLOSE)
        ui_close_window(QUEST_DETAIL_WND_ID);
    return ui_scroll_wnd_send_msg((struct UIWindow *)(pane), sender, msg, val1, val2, val3, val4);
}

static struct UIOngoingQuestScrollWnd *ui_ongoing_quest_scroll_wnd_construct(struct UIOngoingQuestScrollWnd *self,
                                                                             int width, int height)
{
    ui_scroll_wnd_construct(&self->scroll, width, height, 0);
    self->scroll.base.ops = &ui_ongoing_quest_scroll_wnd_ops;
    self->hunt_rows.first = self->hunt_rows.last = self->hunt_rows.end = NULL;
    self->quest_id = 0;
    self->line_height = 0x12;
    ui_ongoing_quest_scroll_wnd_on_create(&self->scroll.base, width, height);
    return self;
}

struct UIOngoingQuestScrollWnd *ui_ongoing_quest_scroll_wnd_new(int width, int height)
{
    struct UIOngoingQuestScrollWnd *self = mem_alloc(sizeof *self);

    return self ? ui_ongoing_quest_scroll_wnd_construct(self, width, height) : NULL;
}

static void ui_ongoing_quest_scroll_wnd_destruct(struct UIOngoingQuestScrollWnd *self)
{
    self->scroll.base.ops = &ui_ongoing_quest_scroll_wnd_ops;
    vec_free(&self->hunt_rows);
    ui_scroll_wnd_destruct(&self->scroll);
}

void ui_ongoing_quest_scroll_wnd_destroy(struct UIWindow *window)
{
    struct UIOngoingQuestScrollWnd *self = (struct UIOngoingQuestScrollWnd *)window;
    ui_ongoing_quest_scroll_wnd_destruct(self);
    mem_free(self);
}

void ui_ongoing_quest_scroll_wnd_on_create(struct UIWindow *window, int width_unused,
                                                 int height_unused)
{
    struct UIOngoingQuestScrollWnd *self = (struct UIOngoingQuestScrollWnd *)window;
    self->scroll.vbar_width = 9;
}

void ui_ongoing_quest_scroll_wnd_on_draw(struct UIWindow *window)
{
    struct UIOngoingQuestScrollWnd *self = (struct UIOngoingQuestScrollWnd *)window;
    pane_on_draw(&self->scroll);
}

void ui_ongoing_quest_scroll_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIOngoingQuestScrollWnd *self = (struct UIOngoingQuestScrollWnd *)window;
    pane_on_wheel(&self->scroll, delta);
}

static void ongoing_hunt_text(const struct RenewQuestB *info, int i, char *out, size_t size)
{
    char what[512];
    short min = info->plain.hunt_min_level[i], max = info->plain.hunt_max_level[i];

    if (info->plain.hunt_mob[i] == QUEST_HUNT_ANY_MOB) {
        struct String kinds;
        renew_quest_condition_text(g_questTable, &kinds, info->plain.hunt_flags[i], min, max);
        _snprintf(what, sizeof what, "%s", string_cstr(&kinds));
        string_free(&kinds);
    } else if (min < 0 || max < 0 || (min == 0 && max == 0)) {
        _snprintf(what, sizeof what, "%s", info->plain.hunt_name[i]);
    } else if (min == 0) {
        _snprintf(what, sizeof what, HUNT_UP_TO_FMT, info->plain.hunt_name[i], max);
    } else if (max == 0) {
        _snprintf(what, sizeof what, HUNT_FROM_FMT, info->plain.hunt_name[i], min);
    } else {
        _snprintf(what, sizeof what, "%s Lv.%d~Lv.%d", info->plain.hunt_name[i], min, max);
    }
    what[sizeof what - 1] = 0;

    if (info->plain.hunt_goal[i] <= 0)
        _snprintf(out, size, "%s", what);
    else if (info->plain.hunt_done[i] != info->plain.hunt_goal[i])
        _snprintf(out, size, "%s ( %d / %d )", what, info->plain.hunt_done[i], info->plain.hunt_goal[i]);
    else
        _snprintf(out, size, "%s ( %s )", what, msg_str(MSG_QUEST_HUNT_DONE));
    out[size - 1] = 0;
}

void ui_ongoing_quest_scroll_wnd_update_hunts(struct UIOngoingQuestScrollWnd *self)
{
    struct RenewQuestB *info = renew_quest_ongoing_info(g_questTable, self->quest_id);
    int i;

    for (i = 0; i < info->plain.hunt_count; i++) {
        struct UIWindow *row = ((struct UIWindow **)self->hunt_rows.first)[i];
        char text[640];

        ongoing_hunt_text(info, i, text, sizeof text);
        ui_window_resize(row, pane_text_width(&self->scroll, text) + 4, 0xe);
        ui_static_text_set_text(row, text, 0);
    }
    ui_window_invalidate_base(&self->scroll.base);
}

static void ongoing_add_hunt_row(struct UIOngoingQuestScrollWnd *self, int y)
{
    struct UIWindow *row = mem_alloc(sizeof(struct UIStaticText));
    struct Vec *rows = &self->hunt_rows;

    if (row)
        row = ui_static_text_construct(row);
    ui_window_create(row, 0xa, 0xe);
    ui_window_move(row, 0x20, y);
    ui_scroll_wnd_add_child(&self->scroll, row);
    vec_push(rows, &row, sizeof(struct UIWindow *));
}

static int ongoing_add_reward(struct UIOngoingQuestScrollWnd *self, const struct RenewQuestReward *reward, int y)
{
    struct UIScrollWnd *pane = &self->scroll;
    struct UIWindow *box = pane_add_bitmap(pane, QUEST_REWARD_BMP, 0x6d, y);
    const struct ItemTableEntry *item;
    struct UIWindow *name;
    char text[MAX_PATH];

    if (!reward->item_id)
        return y;
    item = item_table_get(reward->item_id);
    _snprintf(text, sizeof text, ITEM_BMP_FMT, item->resName ? item->resName : ITEM_FALLBACK_RESOURCE);
    text[sizeof text - 1] = 0;
    pane_add_bitmap(pane, text, 0x74, y + 7);

    name = pane_add_label(pane, item->name, 0x9a, y + 5);
    ui_static_text_set_text_colour(name, 0, 0, 0);
    _snprintf(text, sizeof text, msg_str(MSG_REWARD_COUNT), reward->count);
    text[sizeof text - 1] = 0;
    pane_add_blue_label(pane, text, 0x9a, y + 0x14);
    return y + 3 + box->height;
}

void ui_ongoing_quest_scroll_wnd_set_quest(struct UIOngoingQuestScrollWnd *self, int quest_id)
{
    struct UIScrollWnd *pane = &self->scroll;
    struct RenewQuestB *info = renew_quest_ongoing_info(g_questTable, quest_id);
    struct UITagTextViewer *desc;
    struct RenewQuestReward *reward;
    struct UIScrollBar *bar;
    int y, height, i;

    ui_scroll_wnd_remove_all(pane);
    pane->hbar = NULL;
    pane->vbar = NULL;
    self->hunt_rows.last = self->hunt_rows.first;
    self->quest_id = quest_id;

    pane_add_bitmap(pane, QUEST_HEADING_BMP, 9, 9);
    pane_add_label(pane, msg_str(MSG_QUEST_DESCRIPTION), 0x18, 9);
    desc = pane_add_viewer(0x122, 0xc8, 0x20, 0x1e);
    desc->style.colour = 0x38302e;
    desc->style.font_height = 12;
    desc->style.bold = desc->style.italic = 0;
    ui_scroll_wnd_add_child(pane, &desc->base);
    if (info->description.first != info->description.last) {
        viewer_add_lines(desc, &info->description);
        height = ui_tag_text_viewer_content_height(desc);
    } else {
        height = self->line_height;
    }
    ui_window_resize_base(&desc->base, 0x122, height);
    y = pane_add_divider(pane, height + 0x1e + self->line_height);

    y = pane_add_heading(pane, MSG_QUEST_HUNTS, y);
    for (i = 0; i < info->plain.hunt_count; i++, y += 0x11)
        ongoing_add_hunt_row(self, y);
    ui_ongoing_quest_scroll_wnd_update_hunts(self);
    y = pane_add_divider(pane, y);

    if (info->base.text[RENEW_QUEST_TEXT_NPC_SPRITE].size) {
        struct UIWindow *spr = pane_make_npc(&info->base);
        ui_window_move_base(spr, pane->base.width - spr->width - 0x1e, y);
        pane_set_npc_target(pane, spr, &info->base);
    }
    y = pane_add_heading(pane, MSG_QUEST_REWARD_HEADING, y);
    pane_add_label(pane, msg_str(MSG_QUEST_BASE_EXP), 0x20, y);
    pane_add_blue_label(pane, string_cstr(&info->reward_exp), 0x6d, y);
    y += 0x14;
    pane_add_label(pane, msg_str(MSG_QUEST_JOB_EXP), 0x20, y);
    pane_add_blue_label(pane, string_cstr(&info->reward_job_exp), 0x6d, y);
    y += 0x14;

    pane_add_label(pane, msg_str(MSG_QUEST_REWARDS), 0x20, y);
    for (reward = info->rewards.first; reward != info->rewards.last; reward++)
        y = ongoing_add_reward(self, reward, y);

    bar = mem_alloc(sizeof(struct UIScrollBar));
    if (bar)
        bar = ui_scroll_bar_construct(bar);
    pane->hbar = bar;
    ui_scroll_bar_create(bar, 0, pane->base.height - pane->hbar_height, 0, pane->base.width - pane->vbar_width);
    ui_window_add_child(&pane->base, &pane->hbar->base);
    bar = mem_alloc(sizeof(struct UIScrollBar));
    if (bar)
        bar = ui_renew_quest_scroll_bar_construct(bar);
    pane->vbar = bar;
    ui_scroll_bar_create(bar, pane->base.width - pane->vbar_width, 0, 1, pane->base.height - pane->hbar_height);
    ui_window_add_child(&pane->base, &pane->vbar->base);
    ui_scroll_wnd_update_bars(pane);
    ui_window_invalidate_base(&pane->base);
}

int ui_ongoing_quest_scroll_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                               int val1, int val2, int val3, int val4)
{
    struct UIOngoingQuestScrollWnd *self = (struct UIOngoingQuestScrollWnd *)window;
    if (msg == QUEST_MSG_REFRESH)
        ui_ongoing_quest_scroll_wnd_update_hunts(self);
    return pane_send_msg(&self->scroll, sender, msg, val1, val2, val3, val4);
}

static struct UIRecommededQuestScrollWnd *ui_recommeded_quest_scroll_wnd_construct(struct UIRecommededQuestScrollWnd *self, int width, int height)
{
    ui_scroll_wnd_construct(&self->scroll, width, height, 0);
    self->scroll.base.ops = &ui_recommeded_quest_scroll_wnd_ops;
    self->quest_id = 0;
    self->line_height = 0xe;
    ui_recommeded_quest_scroll_wnd_on_create((struct UIWindow *)(self), width, height);
    return self;
}

struct UIRecommededQuestScrollWnd *ui_recommeded_quest_scroll_wnd_new(int width, int height)
{
    struct UIRecommededQuestScrollWnd *self = mem_alloc(sizeof *self);

    return self ? ui_recommeded_quest_scroll_wnd_construct(self, width, height) : NULL;
}

static void ui_recommeded_quest_scroll_wnd_destruct(struct UIRecommededQuestScrollWnd *self)
{
    self->scroll.base.ops = &ui_recommeded_quest_scroll_wnd_ops;
    ui_scroll_wnd_destruct(&self->scroll);
}

void ui_recommeded_quest_scroll_wnd_destroy(struct UIWindow *window)
{
    struct UIRecommededQuestScrollWnd *self = (struct UIRecommededQuestScrollWnd *)window;
    ui_recommeded_quest_scroll_wnd_destruct(self);
    mem_free(self);
}

void ui_recommeded_quest_scroll_wnd_on_create(struct UIWindow *window, int width_unused,
                                                    int height_unused)
{
    struct UIRecommededQuestScrollWnd *self = (struct UIRecommededQuestScrollWnd *)window;
    self->scroll.vbar_width = 9;
}

void ui_recommeded_quest_scroll_wnd_on_draw(struct UIWindow *window)
{
    struct UIRecommededQuestScrollWnd *self = (struct UIRecommededQuestScrollWnd *)window;
    pane_on_draw(&self->scroll);
}

void ui_recommeded_quest_scroll_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIRecommededQuestScrollWnd *self = (struct UIRecommededQuestScrollWnd *)window;
    pane_on_wheel(&self->scroll, delta);
}

static struct UITagTextViewer *recommended_add_box(int width, int height, int x, int y)
{
    struct UITagTextViewer *view = pane_add_viewer(width, height, x, y);

    view->bg_argb = 0xffff00ff;
    ui_window_invalidate_base(&view->base);
    return view;
}

void ui_recommeded_quest_scroll_wnd_set_quest(struct UIRecommededQuestScrollWnd *self, int quest_id)
{
    struct UIScrollWnd *pane = &self->scroll;
    struct RenewQuestA *info = renew_quest_recommended_info(g_questTable, quest_id);
    struct UITagTextViewer *box[3];
    int i;

    ui_scroll_wnd_remove_all(pane);
    pane->hbar = NULL;
    pane->vbar = NULL;
    self->quest_id = quest_id;

    box[0] = recommended_add_box(0x132, 0x79, 0x12, 0xc);
    ui_scroll_wnd_add_child(pane, &box[0]->base);
    box[1] = recommended_add_box(0xd5, 0x3c, 0x10, 0x9d);
    box[1]->style.colour = 0x2a2a2a;
    box[1]->style.font_height = 12;
    box[1]->style.bold = 1;
    box[1]->style.italic = 0;
    box[1]->outlined = 1;
    box[1]->outline_colour = 0xffffff;
    box[1]->line_spacing = 4;
    ui_scroll_wnd_add_child(pane, &box[1]->base);
    box[2] = recommended_add_box(0xc7, 0x53, 0x12, 0xe1);
    ui_scroll_wnd_add_child(pane, &box[2]->base);
    for (i = 0; i < 3; i++)
        viewer_add_lines(box[i], &info->lists[i]);

    if (info->base.text[RENEW_QUEST_TEXT_NPC_SPRITE].size) {
        struct UIWindow *spr = pane_make_npc(&info->base);
        ui_window_move_base(spr, pane->base.width - spr->width - 0x28, pane->base.height - spr->height - 0xa);
        pane_set_npc_target(pane, spr, &info->base);
    }
    ui_window_invalidate_base(&pane->base);
}

int ui_recommeded_quest_scroll_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                                  int msg, int val1, int val2, int val3, int val4)
{
    struct UIRecommededQuestScrollWnd *self = (struct UIRecommededQuestScrollWnd *)window;
    return pane_send_msg(&self->scroll, sender, msg, val1, val2, val3, val4);
}
