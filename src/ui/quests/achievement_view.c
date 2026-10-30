#include "ui/controls/listbox.h"
#include "ui/controls/buttons.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "contents/connection_api.h"
#include "ui/quests/achievement_internal.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_achievements.h"
#include "session/session_lua.h"

struct UIAchViewWnd;
void ui_ach_view_wnd_destroy(struct UIWindow *window);
void ui_ach_view_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_ach_view_wnd_on_draw(struct UIWindow *self);
int ui_ach_view_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_ach_view_wnd_ops = {
    .destroy = ui_ach_view_wnd_destroy,
    .resize = ui_window_resize_base,
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
    .on_create = ui_ach_view_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_ach_view_wnd_on_draw,
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
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_ach_view_wnd_send_msg,
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
};

struct UIAchSummaryWnd;
void ui_ach_summary_wnd_destroy(struct UIWindow *window);
void ui_ach_summary_wnd_set_show(struct UIWindow *self, int visible);
void ui_ach_summary_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_ach_summary_wnd_on_draw(struct UIWindow *self);
int ui_ach_summary_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_ach_summary_wnd_ops = {
    .destroy = ui_ach_summary_wnd_destroy,
    .resize = ui_window_resize_base,
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
    .set_show = ui_ach_summary_wnd_set_show,
    .on_create = ui_ach_summary_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_ach_summary_wnd_on_draw,
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
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_ach_summary_wnd_send_msg,
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
};

#define VIEW_BULLET ACH_DIR "detail_bullet.bmp"

struct UIAchViewWnd *ui_ach_view_wnd_construct(struct UIAchViewWnd *self)
{
    int i;

    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_ach_view_wnd_ops;
    self->id = -1;
    self->info = NULL;
    self->receive = NULL;
    self->text = NULL;
    for (i = 0; i < 10; i++) {
        self->reqs[i] = NULL;
        self->gauges[i] = NULL;
    }
    for (i = 0; i < 3; i++)
        self->rewards[i] = NULL;
    return self;
}

static void ui_ach_view_wnd_destruct(struct UIAchViewWnd *self)
{
    self->base.ops = &ui_ach_view_wnd_ops;
    ui_window_destruct(&self->base);
}

void ui_ach_view_wnd_destroy(struct UIWindow *window)
{
    struct UIAchViewWnd *self = (struct UIAchViewWnd *)window;
    ui_ach_view_wnd_destruct(self);
    mem_free(self);
}

static void view_set_bitmap(struct UIBitmapButton *button, const struct String *base, const char *suffix,
                            int state)
{
    struct String path;
    size_t len = strlen(suffix);

    string_init(&path);
    string_reserve(&path, base->size + len);
    string_append(&path, string_cstr(base), base->size);
    string_append(&path, suffix, len);
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
    string_free(&path);
}

void ui_ach_view_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIAchViewWnd *self = (struct UIAchViewWnd *)window;
    struct String base;
    struct UIBitmapButton *button;
    int i, y;

    string_init(&base);
    string_assign(&base, ACH_DIR "btn_receive", sizeof(ACH_DIR "btn_receive") - 1);
    button = mem_alloc(sizeof *button);
    self->receive = button ? ui_bitmap_button_construct(button) : NULL;
    view_set_bitmap(self->receive, &base, "_out.bmp", 0);
    view_set_bitmap(self->receive, &base, "_over.bmp", 1);
    view_set_bitmap(self->receive, &base, "_press.bmp", 2);
    view_set_bitmap(self->receive, &base, "_disable.bmp", 3);
    ui_window_create((struct UIWindow *)self->receive, self->receive->bitmap_width,
                     self->receive->bitmap_height);
    ui_window_move_base(&self->receive->button.base, 0x5a, 0x174);
    self->receive->button.base.id = ACH_ID_RECEIVE;
    ui_button_show((struct UIButton *)(self->receive), 0);
    self->receive->redraw_parent = 1;
    ui_window_add_child(&self->base, (struct UIWindow *)self->receive);
    self->text = ach_new_tv(0);
    ui_list_box_create(&self->text->viewer.list, 0xc, 0x22, 0x12c, 0x3c, 1);
    self->text->colour = ACH_DONE_COLOUR;
    self->text->outline = 0xffffff;
    ui_window_add_child(&self->base, &self->text->viewer.list.base);
    for (i = 0, y = 0x63; y < 0x121; i++, y += 0x13) {
        self->reqs[i] = ach_new_tv(1);
        ui_list_box_create(&self->reqs[i]->viewer.list, 0x1c, y, 0x118, 0x28, 1);
        ui_window_set_show_base(&self->reqs[i]->viewer.list.base, 0);
        ui_window_add_child(&self->base, &self->reqs[i]->viewer.list.base);
    }
    for (i = 0; i < 10; i++) {
        self->gauges[i] = ach_new_gauge(0xfe, 0xa);
        self->gauges[i]->bar_colour = 0xc78a71;
        self->gauges[i]->back_colour = 0xffffff;
        self->gauges[i]->outer_colour = 0xffffff;
        self->gauges[i]->inner_colour = 0xdaa896;
        ui_window_set_show_base(&self->gauges[i]->base, 0);
        ui_window_add_child(&self->base, &self->gauges[i]->base);
    }
    for (i = 0; i < 3; i++) {
        struct UIToolTipBitmapWnd *reward = mem_try_alloc(sizeof *reward);

        self->rewards[i] = reward ? ui_tool_tip_bitmap_wnd_construct(reward) : NULL;
        ui_window_set_show_base(&self->rewards[i]->base, 0);
        ui_window_add_child(&self->base, &self->rewards[i]->base);
    }
    string_free(&base);
}

void ui_ach_view_wnd_on_draw(struct UIWindow *window)
{
    struct UIAchViewWnd *self = (struct UIAchViewWnd *)window;
    struct SessionAchievementState state;
    struct AchievementInfo *info;
    unsigned i;

    ach_draw(&self->base, 0, 0, ACH_DIR "bg_detail.bmp", 0);
    if (self->id == -1 || !self->info)
        return;
    if (ach_state(&state, self->id)->completed == 1)
        ach_draw(&self->base, 0x108, 3, ACH_DIR "detail_stamp.bmp", 1);
    info = self->info;
    ui_window_text_out(&self->base, 0xf, 0xe, string_cstr(&info->text[0]), 0, 0, 0xc, ACH_TEXT_COLOUR, 1, 0);
    for (i = 0; i < VEC_COUNT(&self->info->resources, struct AchievementResource); i++) {
        if (self->info->ui_type == 0)
            ach_draw(&self->base, 0x14, 0x67 + 0x13 * i, VIEW_BULLET, 1);
        else if (self->info->ui_type == 1)
            ach_draw(&self->base, 0x14, 0x6f + 0x24 * i, VIEW_BULLET, 1);
    }
    if (ach_has_reward(self->info)) {
        int width = 0, shown = 0, x;

        for (i = 0; i < 3; i++)
            if (self->rewards[i]->base.visible == 1) {
                width += self->rewards[i]->base.width;
                shown++;
            }
        x = (int)((self->base.width - shown * 8 - width + 8) * 0.5);
        for (i = 0; i < 3; i++)
            if (self->rewards[i]->base.visible == 1) {
                ui_window_move_base(&self->rewards[i]->base, x, 0x130);
                x += 0x46;
            }
    }
}

int ui_ach_view_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UIAchViewWnd *self = (struct UIAchViewWnd *)window;
#pragma pack(push, 1)
    struct {
        WORD id;
        int ach_id;
    } packet;
#pragma pack(pop)

    if (msg == UI_MSG_COMMAND) {
        if (val1 == ACH_ID_RECEIVE) {
            packet.id = PACKET_CZ_REQ_ACH_REWARD;
            packet.ach_id = self->id;
            rag_connection_send_packet(rag_connection_instance(),
                                      rag_connection_get_packet_length(rag_connection_instance(),
                                                                     PACKET_CZ_REQ_ACH_REWARD),
                                      &packet);
        }
    } else if (msg == ACH_MSG_GO_TO && val1 != -1 && self->base.parent) {
        ui_achievement_wnd_go_to((struct UIAchievementWnd *)self->base.parent, val1);
    }
    return 0;
}

void ui_ach_view_wnd_refresh(struct UIAchViewWnd *self)
{
    struct SessionAchievementState state;
    struct AchievementInfo *info;
    struct String text;
    unsigned i;
    int y_plain, y_counted;

    ui_list_box_clear(&self->text->viewer.list);
    ui_window_invalidate_base(&self->text->viewer.list.base);
    for (i = 0; i < 10; i++) {
        ui_list_box_clear(&self->reqs[i]->viewer.list);
        ui_window_invalidate_base(&self->reqs[i]->viewer.list.base);
        ui_window_set_show_base(&self->reqs[i]->viewer.list.base, 0);
        ui_window_set_show_base(&self->gauges[i]->base, 0);
    }
    for (i = 0; i < 3; i++)
        ui_window_set_show_base(&self->rewards[i]->base, 0);
    ui_button_show((struct UIButton *)(self->receive), 0);
    if (self->info) {
        ach_state(&state, self->id);
        ui_text_viewer_send_msg(&self->text->viewer.list.base, NULL, UI_MSG_ADD_TEXT, (int)string_cstr(&self->info->text[2]), 0x32, 0, 0);
        if (ach_has_reward(self->info) == 1 && !state.rewarded) {
            ui_button_show((struct UIButton *)(self->receive), 1);
            self->receive->button.disabled = 1;
            ui_window_invalidate_base(&self->receive->button.base);
            if (state.completed == 1) {
                self->receive->button.disabled = 0;
                ui_window_invalidate_base(&self->receive->button.base);
            }
        }
        info = self->info;
        y_plain = 0x61;
        y_counted = 0x7b;
        for (i = 0; i < VEC_COUNT(&info->resources, struct AchievementResource); i++) {
            struct AchievementResource *req = &VEC_AT(&self->info->resources, struct AchievementResource, i);
            struct UIAchTV *line = self->reqs[i];

            ui_window_set_show_base(&line->viewer.list.base, 1);
            line->viewer.list.base.id = req->goto_id;
            if (self->info->ui_type == 1) {
                int count = state.objectives[i], target = req->target;

                string_init(&text);
                string_format(&text, "%s ( %d / %d )", string_cstr(&req->text), count, target);
                ui_text_viewer_send_msg(&line->viewer.list.base, NULL, UI_MSG_ADD_TEXT, (int)string_cstr(&text), 0x38, 0, 0);
                ui_window_set_show_base(&self->gauges[i]->base, 1);
                self->gauges[i]->value = count;
                self->gauges[i]->max = target;
                ui_window_invalidate_base(&self->gauges[i]->base);
                string_free(&text);
            } else {
                ui_text_viewer_send_msg(&line->viewer.list.base, NULL, UI_MSG_ADD_TEXT, (int)string_cstr(&req->text), 0x38, 0, 0);
                ui_window_set_show_base(&self->gauges[i]->base, 0);
            }
            if (self->info->ui_type == 0) {
                if (state.objectives[i] > 0) {
                    line->colour = ACH_DONE_COLOUR;
                    line->underline = 1;
                } else {
                    line->colour = 0x995736;
                }
            } else if (self->info->ui_type == 1) {
                if (state.objectives[i] == req->target) {
                    line->colour = ACH_DONE_COLOUR;
                    line->underline = 1;
                } else if (state.objectives[i] == 0) {
                    line->colour = 0x995736;
                } else {
                    line->colour = 0x4189e6;
                }
            }
            if (self->info->ui_type == 0) {
                ui_window_move_base(&line->viewer.list.base, 0x1c, y_plain);
            } else if (self->info->ui_type == 1) {
                ui_window_move_base(&line->viewer.list.base, 0x1c, y_counted - 0x12);
                ui_window_move_base(&self->gauges[i]->base, 0x1c, y_counted);
            }
            info = self->info;
            y_plain += 0x13;
            y_counted += 0x24;
        }
        if (ach_has_reward(info)) {
            if (info->reward_title > 0) {
                struct String path, name;

                string_init(&path);
                string_assign(&path, ACH_DIR "reward_title.bmp", sizeof(ACH_DIR "reward_title.bmp") - 1);
                session_get_title_string(&g_session1, &name, self->info->reward_title);
                ui_tool_tip_bitmap_wnd_set(self->rewards[0], string_cstr(&path), string_cstr(&name), 0);
                ui_window_set_show_base(&self->rewards[0]->base, 1);
                string_free(&name);
                string_free(&path);
                info = self->info;
            }
            if (info->reward_buff > 0) {
                struct String path;
                const char *descript = NULL;

                string_init(&path);
                string_assign(&path, ACH_DIR "reward_buff.bmp", sizeof(ACH_DIR "reward_buff.bmp") - 1);

                client_lua_call(g_session1.luaState, "GetStateIconDescript", "dd>s", self->info->reward_buff, 1, &descript);
                ui_tool_tip_bitmap_wnd_set(self->rewards[1], string_cstr(&path), descript, 0);
                ui_window_set_show_base(&self->rewards[1]->base, 1);
                string_free(&path);
                info = self->info;
            }
            if (info->reward_item > 0) {
                struct String path;

                string_init(&path);
                string_assign(&path, ACH_DIR "reward_item.bmp", sizeof(ACH_DIR "reward_item.bmp") - 1);
                ui_tool_tip_bitmap_wnd_set(self->rewards[2], string_cstr(&path), NULL, self->info->reward_item);
                ui_window_set_show_base(&self->rewards[2]->base, 1);
                string_free(&path);
            }
        }
    }
    ui_window_invalidate_base(&self->base);
}

#define SUMMARY_LINK_COLOUR 0x997836
#define SUMMARY_LINK_HOVER 0x635aa5

struct UIAchSummaryWnd *ui_ach_summary_wnd_construct(struct UIAchSummaryWnd *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_ach_summary_wnd_ops;
    self->gauge_count = 7;
    vec_init(&self->gauges);
    vec_init(&self->recent);
    return self;
}

static void ui_ach_summary_wnd_destruct(struct UIAchSummaryWnd *self)
{
    self->base.ops = &ui_ach_summary_wnd_ops;
    ach_pairs_free(&self->recent);
    vec_free(&self->gauges);
    ui_window_destruct(&self->base);
}

void ui_ach_summary_wnd_destroy(struct UIWindow *window)
{
    struct UIAchSummaryWnd *self = (struct UIAchSummaryWnd *)window;
    ui_ach_summary_wnd_destruct(self);
    mem_free(self);
}

#define SUMMARY_GAUGE(self, i) VEC_AT(&(self)->gauges, struct UICustomGageBar *, i)

void ui_ach_summary_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIAchSummaryWnd *self = (struct UIAchSummaryWnd *)window;
    int i, x;

    for (i = 0; i < self->gauge_count; i++) {
        struct UICustomGageBar *gauge = mem_try_alloc(sizeof *gauge);

        if (gauge)
            gauge = ui_custom_gage_bar_construct(gauge);
        if (i == 0) {
            ui_window_on_create_base(&gauge->base, 0x22e, 0xa);
            ui_window_resize_base(&gauge->base, 0x22e, 0xa);
            gauge->value = i;
            gauge->max = 100;
            ui_window_move_base(&gauge->base, 0x2b, 0x31);
        } else {
            ui_window_on_create_base(&gauge->base, 0xfe, 0xa);
            ui_window_resize_base(&gauge->base, 0xfe, 0xa);
            gauge->value = 0;
            gauge->max = 100;
            ui_window_move_base(&gauge->base, (i - 1) % 2 * 0x130 + 0x2b, (i - 1) / 2 * 0x32 + 0x7b);
        }
        gauge->bar_colour = 0xc78a71;
        gauge->back_colour = 0xffffff;
        gauge->outer_colour = 0xffffff;
        gauge->inner_colour = 0xdaa896;
        ui_window_add_child(&self->base, &gauge->base);
        vec_push(&self->gauges, &gauge, sizeof(struct UICustomGageBar *));
    }
    for (i = 0, x = 0x3e; x < 0x2b6; i++, x += 0x13c) {
        struct UITextButton *button = ui_text_button_create();

        self->recent_buttons[i] = button;
        ui_text_button_set_text((struct UIWindow *)self->recent_buttons[i], "",
                             SUMMARY_LINK_COLOUR, 0, 0xc, 1, 0);
        self->recent_buttons[i]->hover_colour = SUMMARY_LINK_HOVER;
        ui_window_move_base(&self->recent_buttons[i]->button.base, x, 0x145);
        self->recent_buttons[i]->button.base.id = -1;
        ui_window_add_child(&self->base, (struct UIWindow *)self->recent_buttons[i]);
    }
}

static void summary_ratio(struct UIAchSummaryWnd *self, int x, int y, struct UICustomGageBar *gauge)
{
    struct String text;

    string_init(&text);
    string_format(&text, "%d/%d", gauge->value, gauge->max);
    ui_window_text_out_right(&self->base, x, y, string_cstr(&text), 0, 0, 0xb, SUMMARY_LINK_HOVER, 0);
    string_free(&text);
}

static const int g_summaryNames[6] = { 0xa61, 0xa65, 0xa6c, 0xa6f, 0xa72, 0xa76 };

void ui_ach_summary_wnd_on_draw(struct UIWindow *window)
{
    struct UIAchSummaryWnd *self = (struct UIAchSummaryWnd *)window;
    struct AchPair *it;
    int i, x;

    ach_draw(&self->base, 0, 0, ACH_DIR "bg_summary.bmp", 0);
    ui_window_text_out(&self->base, 0x52, 0x1b, msg_str(0xa77), 0, 0, 0xc, ACH_TEXT_COLOUR, 1, 0);
    summary_ratio(self, 0x258, 0x1c, SUMMARY_GAUGE(self, 0));
    for (i = 0; i < 6; i++) {
        int row = i / 2 * 0x32, col = i % 2;

        ui_window_text_out(&self->base, col * 0x12e + 0x52, row + 0x6b, msg_str(g_summaryNames[i]), 0, 0, 0xc,
                         ACH_TEXT_COLOUR, 1, 0);
        summary_ratio(self, col * 0x130 + 0x128, row + 0x6c, SUMMARY_GAUGE(self, i + 1));
    }
    ui_window_text_out(&self->base, 9, 0x124, msg_str(0xa5b), 0, 0, 0xc, 0x9c6b5a, 1, 0);
    for (it = self->recent.last, x = 0x104; it != (struct AchPair *)self->recent.first; it--, x += 0x13c) {
        int id = it[-1].id;
        struct String path, text;
        struct SessionAchievementState state;

        achievement_ui_text(g_achievementDb, &path, id);
        string_insert(&path, 0, ACH_DIR "icon_", sizeof(ACH_DIR "icon_") - 1);
        string_append_cstr(&path, ".bmp");
        ui_window_draw_bitmap(&self->base, x - 0xf2, 0x13a, ui_skin_bitmap(string_cstr(&path)), 1);
        ach_draw(&self->base, x, 0x13c, ACH_DIR "badge_complete.bmp", 1);
        string_init(&text);
        string_format(&text, "%d", achievement_info_number(g_achievementDb, id));
        ui_window_text_out(&self->base, x + 0x17, 0x145, string_cstr(&text), 0, 0, 0xc, ACH_TEXT_COLOUR, 1,
                         0);
        string_free(&text);
        achievement_text1(g_achievementDb, &text, id);
        ui_window_text_out(&self->base, x - 0xf1, 0x16d, string_cstr(&text), 0, 0, 0xc, ACH_DONE_COLOUR, 0,
                         0);
        string_free(&text);
        ach_state(&state, id);
        if (achievement_has_reward(g_achievementDb, id) == 1 && !state.rewarded)
            ach_draw(&self->base, x + 0xc, 0x168, ACH_DIR "list_rewardbox_not_receive.bmp", 1);
        string_format(&text, "%4d.%2d.%2d", state.year, state.month, state.day);
        ui_window_text_out_outlined(&self->base, x - 3, 0x16e, string_cstr(&text), 0, ACH_DATE_COLOUR,
                                 0xffffff, 0, 0xa, 0, 0);
        string_free(&text);
        string_free(&path);
    }
}

void ui_ach_summary_wnd_set_show(struct UIWindow *window, int visible)
{
    struct UIAchSummaryWnd *self = (struct UIAchSummaryWnd *)window;
    int i;

    self->base.visible = visible;
    for (i = 0; i < 2; i++)
        ui_text_button_on_mouse_move(&self->recent_buttons[i]->button.base, -1, -1);
}

int ui_ach_summary_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UIAchSummaryWnd *self = (struct UIAchSummaryWnd *)window;
    if (msg == UI_MSG_TEXT_BUTTON && val1 != -1 && self->base.parent)
        ui_achievement_wnd_go_to((struct UIAchievementWnd *)self->base.parent, val1);
    ui_window_invalidate_base(&self->base);
    return 0;
}

static void summary_add_recent(struct UIAchSummaryWnd *self, struct AchievementInfo *info, int *id,
                               struct UITextButton *button)
{
    struct String title;

    if (achievement_get_info(g_achievementDb, info, *id)) {
        struct AchPair pair;

        ach_pair_copy_construct(&pair, id, info);
        vec_push(&self->recent, &pair, sizeof pair);
    }
    string_init(&title);
    string_assign_cstr(&title, string_cstr(&info->text[0]));
    string_assign(&button->button.text, string_cstr(&title), title.size);
    string_free(&title);
    button->button.base.id = *id;
    ui_text_button_fit_to_caption((struct UITextButton *)&button->button);
}

void ui_ach_summary_wnd_refresh(struct UIAchSummaryWnd *self)
{
    struct AchievementInfo info;
    int second = 0, first = 0, i;

    ach_pairs_clear(&self->recent);
    session_get_latest_achievements(&g_session1, &second, &first);
    achievement_info_init(&info);
    summary_add_recent(self, &info, &second, self->recent_buttons[1]);
    summary_add_recent(self, &info, &first, self->recent_buttons[0]);
    achievement_info_destruct(&info);
    for (i = 0; i < self->gauge_count; i++) {
        SUMMARY_GAUGE(self, i)->value = achievement_tab_count_flagged(g_achievementDb, i, -1);
        SUMMARY_GAUGE(self, i)->max = achievement_tab_count(g_achievementDb, i, -1);
    }
    ui_window_invalidate_base(&self->base);
}
