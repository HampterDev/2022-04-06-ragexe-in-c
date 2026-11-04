#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "ui/window_access.h"
#include "ui/quests/achievement.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_achievements.h"
#include "ui/wndmgr/wndmgr.h"

struct UIAchievementWnd;
struct UIWindow;
void ui_achievement_wnd_destroy(struct UIWindow *window);
void ui_achievement_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_achievement_wnd_on_draw(struct UIWindow *self);
intptr_t ui_achievement_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_achievement_wnd_ops = {
    .destroy = ui_achievement_wnd_destroy,
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
    .on_create = ui_achievement_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_achievement_wnd_on_draw,
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
    .send_msg = ui_achievement_wnd_send_msg,
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

struct UIAchTracingWnd;
void ui_ach_tracing_wnd_destroy(struct UIWindow *window);
BYTE ui_ach_tracing_wnd_is_click_through(struct UIWindow *self);
void ui_ach_tracing_wnd_store_info(struct UIWindow *self);
void ui_ach_tracing_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_ach_tracing_wnd_on_draw(struct UIWindow *self);
intptr_t ui_ach_tracing_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_ach_tracing_wnd_ops = {
    .destroy = ui_ach_tracing_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_ach_tracing_wnd_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_ach_tracing_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_ach_tracing_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_ach_tracing_wnd_on_draw,
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
    .send_msg = ui_ach_tracing_wnd_send_msg,
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

#define ACH_TITLE_MSG 0xa56
#define ACH_SUMMARY_MSG 0xa77

static const struct {
    int main, sub;
} ach_tab_names[] = {
    {0, -1},
    {1, -1}, {1, 0}, {1, 1}, {1, 2},
    {2, -1}, {2, 0}, {2, 1}, {2, 2}, {2, 3}, {2, 4}, {2, 5},
    {3, -1}, {3, 0}, {3, 1},
    {4, -1}, {4, 0}, {4, 1},
    {5, -1}, {5, 0}, {5, 1}, {5, 2},
    {6, -1},
};
#define ACH_TAB_FIRST_MSG 0xa60

struct UIAchievementWnd *ui_achievement_wnd_construct(struct UIAchievementWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_achievement_wnd_ops;
    tree_init(&self->achs);
    self->upper = NULL;
    self->tab = NULL;
    self->list = NULL;
    self->main = 0;
    self->sub = -1;
    return self;
}

struct UIWindow *ui_achievement_wnd_new(void)
{
    return (struct UIWindow *)ui_achievement_wnd_construct(mem_alloc(sizeof(struct UIAchievementWnd)));
}

void ui_achievement_wnd_destruct(struct UIAchievementWnd *self)
{
    self->frame.base.ops = &ui_achievement_wnd_ops;
    tree_free(&self->achs, achievement_info_entry_destroy);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_achievement_wnd_destroy(struct UIWindow *window)
{
    struct UIAchievementWnd *self = (struct UIAchievementWnd *)window;
    ui_achievement_wnd_destruct(self);
    mem_free(self);
}

static void ui_achievement_wnd_show_category(struct UIAchievementWnd *self, int main, int sub)
{
    struct UIAchTab *tab;
    struct UIAchUpperWnd *upper;

    if (main == 0) {
        ui_window_set_show_base(&self->view->base, 0);
        ui_window_set_show_base(&self->list->base, 0);
        ui_ach_summary_wnd_set_show(&self->summary->base, 1);
    } else {
        ui_window_set_show_base(&self->view->base, 1);
        ui_window_set_show_base(&self->list->base, 1);
        ui_ach_summary_wnd_set_show(&self->summary->base, 0);
    }
    tab = self->tab;
    self->main = main;
    self->sub = sub;
    tab->sel_sub = sub;
    tab->sel_main = main;
    ui_window_invalidate_base(&tab->base);
    upper = self->upper;
    if (main == 0) {
        struct String caption;
        const char *text = msg_str(ACH_SUMMARY_MSG);

        string_init(&caption);
        string_assign_cstr(&caption, text);
        string_assign(&upper->caption, string_cstr(&caption), caption.size);
        string_free(&caption);
    } else {
        struct String name;

        ui_ach_tab_get_name(self->tab, &name, self->main, self->sub);
        string_assign(&upper->caption, string_cstr(&name), name.size);
        string_free(&name);
    }
    ui_window_invalidate_base(&self->upper->base);
    if (achievement_collect_tab(g_achievementDb, &self->achs, self->main, self->sub) == 1)
        ui_ach_list_wnd_fill(self->list, &self->achs, self->upper->order);
    ui_achievement_wnd_send_msg(&self->frame.base, NULL, ACH_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_achievement_wnd_go_to(struct UIAchievementWnd *self, int id)
{
    int main = -1, sub = -1;

    if (achievement_find_tab(g_achievementDb, id, &main, &sub) != 1)
        return;
    ui_achievement_wnd_show_category(self, main, sub);
    ui_ach_list_wnd_select_id(self->list, id);
}

void ui_achievement_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIAchievementWnd *self = (struct UIAchievementWnd *)window;
    struct UIAchUpperWnd *upper;
    struct UIAchTab *tab;
    struct UIAchListWnd *list;
    struct UIAchViewWnd *view;
    struct UIAchSummaryWnd *summary;
    unsigned i;

    ui_window_add_close_button(&self->frame.base, "", 0);

    upper = mem_try_alloc(sizeof *upper);
    upper = upper ? ui_ach_upper_wnd_construct(upper) : NULL;
    self->upper = upper;
    ui_window_create(&upper->base, self->frame.base.width, 0x2e);
    ui_window_move_base(&self->upper->base, 0, 0x11);
    ui_window_add_child(&self->frame.base, &self->upper->base);

    tab = mem_try_alloc(sizeof *tab);
    tab = tab ? ui_ach_tab_construct(tab) : NULL;
    self->tab = tab;
    ui_ach_tab_init(tab, 0x66, 0x28, 0x1a, 0, 0, 0, 0, 0);
    ui_window_move_base(&self->tab->base, 0, 0x3f);
    ui_window_add_child(&self->frame.base, &self->tab->base);
    for (i = 0; i < sizeof ach_tab_names / sizeof ach_tab_names[0]; i++)
        ui_ach_tab_set_name(self->tab, msg_str(ACH_TAB_FIRST_MSG + i), ach_tab_names[i].main, ach_tab_names[i].sub);

    list = mem_try_alloc(sizeof *list);
    list = list ? ui_ach_list_wnd_construct(list) : NULL;
    self->list = list;
    ui_ach_list_wnd_init(list, 0x140, 0x195, 5);
    ui_window_move_base(&self->list->base, 0x66, 0x3f);
    ui_window_add_child(&self->frame.base, &self->list->base);

    view = mem_alloc(sizeof *view);
    view = view ? ui_ach_view_wnd_construct(view) : NULL;
    self->view = view;
    ui_window_create(&view->base, 0x148, 0x195);
    ui_window_move_base(&self->view->base, 0x1a6, 0x3f);
    ui_window_add_child(&self->frame.base, &self->view->base);

    summary = mem_alloc(sizeof *summary);
    summary = summary ? ui_ach_summary_wnd_construct(summary) : NULL;
    self->summary = summary;
    ui_window_create(&summary->base, 0x288, 0x194);
    ui_window_move_base(&self->summary->base, 0x66, 0x3f);
    ui_window_add_child(&self->frame.base, &self->summary->base);

    ui_achievement_wnd_show_category(self, self->main, self->sub);
}

void ui_achievement_wnd_on_draw(struct UIWindow *window)
{
    struct UIAchievementWnd *self = (struct UIAchievementWnd *)window;
    ui_window_clear_canvas(&self->frame.base, 0xffffffff);
    ui_window_draw_title_bar(&self->frame.base, 1, msg_str(ACH_TITLE_MSG), 0);
}

intptr_t ui_achievement_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                        intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIAchievementWnd *self = (struct UIAchievementWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CLOSE) {
            ui_window_mgr_post_quit1(&g_windowMgr, ACHIEVEMENT_WND_ID);
            ui_window_invalidate_base(&self->frame.base);
            return 0;
        }
        break;
    case ACH_MSG_SELECT_TAB:
        if (val1 == -1 && val2 == -1)
            ui_achievement_wnd_show_category(self, self->main, self->sub);
        else
            ui_achievement_wnd_show_category(self, val1, val2);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    case ACH_MSG_REFRESH: {
        struct UIAchUpperWnd *upper;
        int count, max;

        ui_ach_view_wnd_refresh(self->view);
        ui_ach_summary_wnd_refresh(self->summary);
        count = achievement_tab_count_flagged(g_achievementDb, self->main, self->sub);
        max = achievement_tab_count(g_achievementDb, self->main, self->sub);
        ui_ach_upper_wnd_set_count(self->upper, count, max);
        upper = self->upper;
        ui_ach_upper_wnd_set_points(upper, g_session1.achCurrentPoints, g_session1.achNextPoints);
        ui_window_invalidate_base(&self->upper->base);
        ui_window_invalidate_base(&self->frame.base);
        break;
    }
    case ACH_MSG_SHOW: {
        struct AchievementInfoEntry *found;
        struct UIAchViewWnd *view;
        int id = (int)val1;

        found = tree_find(&self->achs, &id, tree_less_int);
        view = self->view;
        if (!found) {
            view->id = -1;
            view->info = NULL;
        } else {
            view = self->view;
            view->id = val1;
            view->info = &found->info;
        }
        ui_ach_view_wnd_refresh(view);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    }
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
    ui_window_invalidate_base(&self->frame.base);
    return 0;
}

#define TRACE_LINE_ID 0x1d6
#define TRACE_ARROW_ID 0x1d7
#define TRACE_DONE_COLOUR 0xdff6f
#define TRACE_TEXT_COLOUR 0xffffff

#define TRACE_COUNT(v) VEC_COUNT(v, struct TrackedAchievement)

struct UIAchTracingWnd *ui_ach_tracing_wnd_construct(struct UIAchTracingWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_ach_tracing_wnd_ops;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_ach_tracing_wnd_new(void)
{
    return (struct UIWindow *)ui_ach_tracing_wnd_construct(mem_alloc(sizeof(struct UIAchTracingWnd)));
}

void ui_ach_tracing_wnd_destruct(struct UIAchTracingWnd *self)
{
    self->frame.base.ops = &ui_ach_tracing_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_ach_tracing_wnd_destroy(struct UIWindow *window)
{
    struct UIAchTracingWnd *self = (struct UIAchTracingWnd *)window;
    ui_ach_tracing_wnd_destruct(self);
    mem_free(self);
}

BYTE ui_ach_tracing_wnd_is_click_through(struct UIWindow *self)
{
    return 1;
}

void ui_ach_tracing_wnd_store_info(struct UIWindow *window)
{
    struct UIAchTracingWnd *self = (struct UIAchTracingWnd *)window;
    if (!self->info)
        return;
    self->info->x = self->frame.base.x;
    self->info->y = self->frame.base.y;
}

static struct UITextButton *trace_new_button(struct UIAchTracingWnd *self, const char *text, int id)
{
    struct UITextButton *button = ui_text_button_create();
    ui_text_button_set_text(&button->button.base, text, 0, 0, 0xc, 1, 0);
    button->button.base.id = id;
    ui_window_add_child(&self->frame.base, &button->button.base);
    return button;
}

void ui_ach_tracing_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIAchTracingWnd *self = (struct UIAchTracingWnd *)window;
    int i;

    for (i = 0; i < 3; i++) {
        self->lines[i] = trace_new_button(self, "", TRACE_LINE_ID);
        self->arrows[i] = trace_new_button(self, "\xe2\x96\xb6" /* "▶" */, TRACE_ARROW_ID);
    }
}

static void trace_draw_count(struct UIAchTracingWnd *self, struct UITextButton *line, const struct TrackedAchievement *trace,
                             int y)
{
    struct String text;

    string_init(&text);
    string_format(&text, "(+%d)", (int)VEC_COUNT(&trace->lines, struct TrackedLine));
    ui_window_text_out(&self->frame.base, line->button.base.width + 0x10, y + 1, string_cstr(&text), 0, 0,
                     0xc, TRACE_TEXT_COLOUR, 1, 0);
    string_free(&text);
    string_format(&text, "(+%d)", (int)VEC_COUNT(&trace->lines, struct TrackedLine));
    ui_window_text_out(&self->frame.base, line->button.base.width + 0xf, y, string_cstr(&text), 0, 0, 0xc, 0,
                     1, 0);
    string_free(&text);
}

void ui_ach_tracing_wnd_on_draw(struct UIWindow *window)
{
    struct UIAchTracingWnd *self = (struct UIAchTracingWnd *)window;
    int i, pass, y = 0xa;
    unsigned index;

    ui_window_clear_canvas(&self->frame.base, 0xffff00ff);
    for (i = 0; i < 3; i++) {
        ui_button_set_show((struct UIButton *)(self->lines[i]), 0);
        ui_button_set_show((struct UIButton *)(self->arrows[i]), 0);
    }
    for (pass = 0; pass < 2; pass++) {
        for (index = 0; index < TRACE_COUNT(&g_session1.trackedAchievements); index++) {
            struct TrackedAchievement *trace = &VEC_AT(&g_session1.trackedAchievements, struct TrackedAchievement, index);
            struct UITextButton *line = self->lines[index], *arrow = self->arrows[index];
            struct String text;

            if (pass == 0 ? trace->completed == 1 : trace->completed != 1)
                continue;
            ui_button_set_show((struct UIButton *)(line), 1);
            ui_button_set_show((struct UIButton *)(arrow), 1);
            string_init(&text);
            string_format(&text, "\xe2\x97\x8f [%s]" /* "● [%s]" */, string_cstr(&trace->name));
            string_assign(&line->button.text, string_cstr(&text), text.size);
            string_free(&text);
            ui_text_button_fit_to_caption((struct UITextButton *)&line->button);
            ui_window_move_base(&line->button.base, 0xa, y);
            line->value = trace->id;
            ui_window_move_base(&arrow->button.base, self->frame.base.width - 0x14, y);
            arrow->value = trace->id;
            if (!trace->expanded)
                trace_draw_count(self, line, trace, y);
            y += 0xf;
            if (trace->expanded == 1) {
                struct TrackedLine *cond;

                for (cond = trace->lines.first; cond != (struct TrackedLine *)trace->lines.last; cond++) {
                    ui_window_text_out(&self->frame.base, 0x15, y + 1, string_cstr(&cond->text), 0, 0, 0xc, 0,
                                     0, 0);
                    ui_window_text_out(&self->frame.base, 0x14, y, string_cstr(&cond->text), 0, 0, 0xc,
                                     cond->done ? TRACE_DONE_COLOUR : TRACE_TEXT_COLOUR, 0, 0);
                    y += 0xf;
                }
            }
            y += 5;
        }
    }
}

intptr_t ui_ach_tracing_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                       intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIAchTracingWnd *self = (struct UIAchTracingWnd *)window;
    if (msg == UI_MSG_SET_INFO) {
        self->info = (POINT *)val1;
        if (!ach_tracing_info_is_on_screen(self->info))
            ach_tracing_info_set_default(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
    } else if (msg == UI_MSG_TEXT_BUTTON) {
        if (val1 == TRACE_LINE_ID) {
            struct TrackedAchievement *trace, *last = g_session1.trackedAchievements.last;

            for (trace = g_session1.trackedAchievements.first; trace != last; trace++)
                if (trace->id == val2)
                    break;
            if (trace != (struct TrackedAchievement *)g_session1.trackedAchievements.last)
                trace->expanded = !trace->expanded;
        } else if (val1 == TRACE_ARROW_ID) {
            struct UIWindow *wnd = ui_window_open(ACHIEVEMENT_WND_ID);

            if (wnd)
                ui_achievement_wnd_go_to((struct UIAchievementWnd *)wnd, val2);
        }
    } else {
        return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
    }
    ui_window_invalidate_base(&self->frame.base);
    return 0;
}
