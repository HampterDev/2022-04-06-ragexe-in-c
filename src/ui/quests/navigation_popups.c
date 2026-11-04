#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "navi/navi_manager.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include "ui/quests/quests.h"
#include "ui/quests/navigation.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define NAVI_GUIDE_WND_ID 0x13a
#define NAVI_ROAD_ICON_WND_ID 0x132

#define MSG_CLOSE 0xbe5
#define MSG_NAVI_GUIDE_TITLE 0xc84
#define MSG_NAVI_ROAD_ICON_TITLE 0xc83

#define NAVI_LINE_FIRST 100
#define NAVI_LINE_STEP 200
#define NAVI_LINE_DESTINATION 300

#define NAVI_GUIDE_LIST_ID 0x148
#define NAVI_GUIDE_MSG_REFRESH 0x3c

struct Vec;
struct UINavigationRuideWnd;
void ui_navigation_ruide_wnd_destroy(struct UIWindow *window);
void ui_navigation_ruide_wnd_on_create(struct UIWindow *self, int width, int height_unused);
void ui_navigation_ruide_wnd_on_draw(struct UIWindow *self);
intptr_t ui_navigation_ruide_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_navigation_ruide_wnd_ops = {
    .destroy = ui_navigation_ruide_wnd_destroy,
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
    .on_create = ui_navigation_ruide_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_navigation_ruide_wnd_on_draw,
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
    .send_msg = ui_navigation_ruide_wnd_send_msg,
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

struct UINavigationRuideWnd {
    struct UIFrameWnd frame;
    struct UIListBox_NaviSearch *list;
};

struct UINavigationRuideWnd *ui_navigation_ruide_wnd_construct(struct UINavigationRuideWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_navigation_ruide_wnd_ops;
    return self;
}

struct UIWindow *ui_navigation_ruide_wnd_new(void)
{
    return (struct UIWindow *)ui_navigation_ruide_wnd_construct(mem_alloc(sizeof(struct UINavigationRuideWnd)));
}

static void ui_navigation_ruide_wnd_destruct(struct UINavigationRuideWnd *self)
{
    self->frame.base.ops = &ui_navigation_ruide_wnd_ops;
    ui_list_box_navi_search_clear_items(&self->list->list);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_navigation_ruide_wnd_destroy(struct UIWindow *window)
{
    struct UINavigationRuideWnd *self = (struct UINavigationRuideWnd *)window;
    ui_navigation_ruide_wnd_destruct(self);
    mem_free(self);
}

static void navi_guide_add_line(struct UIListBox_NaviSearch *list, const char *text, int kind)
{
    struct Vec *kinds = &list->kinds;

    vec_push(kinds, &kind, sizeof(int));
    ui_list_box_add_item(&list->list, text);
    ui_window_invalidate_base(&list->list.base);
}

static void ui_navigation_ruide_wnd_fill(struct UINavigationRuideWnd *self)
{
    char line[0x32] = { 0 };
    struct NaviPoint pos;
    int i;

    ui_list_box_navi_search_clear_items(&self->list->list);
    for (i = 0; i < navi_manager_route_link_count(&g_navi); i++) {
        struct NaviLink *step = navi_manager_route_link_at(&g_navi, i);
        struct NaviPoint cell;
        int y = navi_object_position(&step->base, &cell)->y;
        int x = navi_object_position(&step->base, &cell)->x;

        sprintf(line, "%s (%d %d)", navi_map_name(navi_object_map(&(step)->base)), x, y);
        navi_guide_add_line(self->list, line, i == 0 ? NAVI_LINE_FIRST : NAVI_LINE_STEP);
    }
    if (navi_manager_target_pos2(&g_navi, &pos)->x > 0) {
        int y = navi_manager_target_pos2(&g_navi, &pos)->y;
        int x = navi_manager_target_pos2(&g_navi, &pos)->x;

        sprintf(line, "%s (%d %d)", navi_map_name(navi_manager_target_map2(&g_navi)), x, y);
    } else {
        sprintf(line, "%s", navi_map_name(navi_manager_target_map2(&g_navi)));
    }
    navi_guide_add_line(self->list, line, NAVI_LINE_DESTINATION);
}

void ui_navigation_ruide_wnd_on_create(struct UIWindow *window, int width, int height_unused)
{
    struct UINavigationRuideWnd *self = (struct UINavigationRuideWnd *)window;
    struct UIListBox_NaviSearch *list;

    ui_window_add_close_button(&self->frame.base, msg_str(MSG_CLOSE), 0);
    list = mem_alloc(sizeof *list);
    if (list) {
        ui_list_box_construct(&list->list);
        list->list.base.ops = &ui_list_box_navi_search_ops.base;
        list->kinds.first = list->kinds.last = list->kinds.end = NULL;
    }
    self->list = list;
    ui_window_create(&list->list.base, width - 4, 0x125);
    self->list->list.r = 0xff;
    self->list->list.g = 0xff;
    self->list->list.b = 0xff;
    self->list->list.base.id = NAVI_GUIDE_LIST_ID;
    ui_window_move_base(&self->list->list.base, 2, 0x14);
    ui_window_add_child(&self->frame.base, &self->list->list.base);
    ui_navigation_ruide_wnd_fill(self);
}

void ui_navigation_ruide_wnd_on_draw(struct UIWindow *window)
{
    struct UINavigationRuideWnd *self = (struct UINavigationRuideWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_fill_rect(wnd, 0, 0, 0x110, 0x152, RGB(0xcc, 0xcc, 0xcc));
    ui_window_draw_body(wnd, ui_window_draw_title_bar(wnd, 1, msg_str(MSG_NAVI_GUIDE_TITLE), 0), 0, "");
    ui_window_draw_button_bar(wnd, 0, 0, 1);
}

intptr_t ui_navigation_ruide_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UINavigationRuideWnd *self = (struct UINavigationRuideWnd *)window;
    if (msg != UI_MSG_COMMAND && msg != NAVI_GUIDE_MSG_REFRESH)
        return 0;
    if (msg == UI_MSG_COMMAND && val1 == UI_ID_CLOSE)
        ui_window_mgr_post_quit1(&g_windowMgr, NAVI_GUIDE_WND_ID);
    ui_navigation_ruide_wnd_fill(self);
    ui_window_invalidate_base(&self->frame.base);
    return 0;
}

#define ROAD_ICON_SELECT_BMP UI_SKIN "navigation_interface3\\img_roadIocn_bg_select.bmp"
#define ROAD_ICON_BG_BMP UI_SKIN "navigation_interface3\\bg_roadIocn_select.bmp"

#define NAVI_MSG_ROAD_ICON 0x28

struct UINavigationroadiconWnd;
void ui_navigationroadicon_wnd_destroy(struct UIWindow *window);
void ui_navigationroadicon_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_navigationroadicon_wnd_on_draw(struct UIWindow *self);
void ui_navigationroadicon_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
intptr_t ui_navigationroadicon_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_navigationroadicon_wnd_ops = {
    .destroy = ui_navigationroadicon_wnd_destroy,
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
    .on_create = ui_navigationroadicon_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_navigationroadicon_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_navigationroadicon_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_navigationroadicon_wnd_send_msg,
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

struct UINavigationroadiconWnd {
    struct UIFrameWnd frame;
    struct UIWindow *highlight;
};

static void road_icon_move_highlight(struct UIWindow *highlight, int icon)
{
    int column = icon <= 4 ? icon : icon - 4;
    int x = (unsigned)(icon - 1) <= 7 ? column * 0x24 - 0x1a : 0;

    ui_window_move(highlight, x, icon < 5 ? 0x18 : 0x3c);
}

struct UINavigationroadiconWnd *ui_navigationroadicon_wnd_construct(struct UINavigationroadiconWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_navigationroadicon_wnd_ops;
    return self;
}

struct UIWindow *ui_navigationroadicon_wnd_new(void)
{
    return (struct UIWindow *)ui_navigationroadicon_wnd_construct(mem_alloc(sizeof(struct UINavigationroadiconWnd)));
}

static void ui_navigationroadicon_wnd_destruct(struct UINavigationroadiconWnd *self)
{
    self->frame.base.ops = &ui_navigationroadicon_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_navigationroadicon_wnd_destroy(struct UIWindow *window)
{
    struct UINavigationroadiconWnd *self = (struct UINavigationroadiconWnd *)window;
    ui_navigationroadicon_wnd_destruct(self);
    mem_free(self);
}

void ui_navigationroadicon_wnd_on_create(struct UIWindow *window, int width_unused,
                                                 int height_unused)
{
    struct UINavigationroadiconWnd *self = (struct UINavigationroadiconWnd *)window;
    struct UIWindow *highlight;

    ui_window_add_close_button(&self->frame.base, msg_str(MSG_CLOSE), 0);
    highlight = mem_alloc(sizeof(struct UIStaticBitmap));
    if (highlight)
        highlight = (struct UIWindow *)ui_static_bitmap_construct((struct UIStaticBitmap *)highlight);
    self->highlight = highlight;
    ui_static_bitmap_set_bitmap((struct UIStaticBitmap *)highlight, ROAD_ICON_SELECT_BMP);
    ui_window_create(self->highlight, self->highlight->width, self->highlight->height);
    road_icon_move_highlight(self->highlight, g_navi.roadIcon);
    ui_window_set_show_base(self->highlight, 1);
    ui_window_add_child(&self->frame.base, self->highlight);
}

void ui_navigationroadicon_wnd_on_draw(struct UIWindow *window)
{
    struct UINavigationroadiconWnd *self = (struct UINavigationroadiconWnd *)window;
    struct BitmapRes *bg = ui_skin_bitmap(ROAD_ICON_BG_BMP);
    int top = ui_window_draw_title_bar(&self->frame.base, 1, msg_str(MSG_NAVI_ROAD_ICON_TITLE), 0);

    ui_window_draw_bitmap(&self->frame.base, 0, top, bg, 0);
}

static int road_icon_hit_test(int x, int y)
{
    int row, column;

    if ((unsigned)(y - 0x19) <= 0x1e)
        row = 0;
    else if ((unsigned)(y - 0x3f) <= 0x1c)
        row = 1;
    else
        return 0;
    for (column = 0; column < 4; column++)
        if ((unsigned)(x - 0xb - column * 0x24) <= 0x1e)
            return row * 4 + column + 1;
    return 0;
}

void ui_navigationroadicon_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UINavigationroadiconWnd *self = (struct UINavigationroadiconWnd *)window;
    int icon = road_icon_hit_test(x, y);

    if (icon) {
        if (g_windowMgr.navigation_v4_wnd)
            ui_window_send_msg(g_windowMgr.navigation_v4_wnd, NULL, NAVI_MSG_ROAD_ICON, icon, 0, 0, 0);
        road_icon_move_highlight(self->highlight, icon);
    }
    ui_window_invalidate_base(&self->frame.base);
    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
}

intptr_t ui_navigationroadicon_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender,
                                               int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    if (msg == UI_MSG_COMMAND && val1 == UI_ID_CLOSE)
        ui_window_mgr_post_quit1(&g_windowMgr, NAVI_ROAD_ICON_WND_ID);
    return 0;
}

#define NAVI_HELP_WND_ID 0xe5
#define MSG_NAVI_HELP_TITLE 0x8c0
#define MSG_NAVI_HELP_CLOSE 0x89a
#define NAVI_HELP_TEXT_LAYOUT 0x30
#define NAVI_HELP_BLANK 0
#define NAVI_HELP_BACKGROUND 0xd2

struct UINavigationHelpWnd;
void ui_navigation_help_wnd_destroy(struct UIWindow *window);
void ui_navigation_help_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_navigation_help_wnd_on_draw(struct UIWindow *self);
intptr_t ui_navigation_help_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_navigation_help_wnd_ops = {
    .destroy = ui_navigation_help_wnd_destroy,
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
    .on_create = ui_navigation_help_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_navigation_help_wnd_on_draw,
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
    .send_msg = ui_navigation_help_wnd_send_msg,
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

struct UINavigationHelpWnd {
    struct UIFrameWnd frame;
    struct UITextViewer *text;
    int close_button;
};

static const int navi_help_lines[] = {
    0x8c1, NAVI_HELP_BLANK, 0x8c2, 0x8c3, 0x8c4, NAVI_HELP_BLANK, 0x8c5, 0x8c6, 0x8c7, NAVI_HELP_BLANK,
    0x8c8, NAVI_HELP_BLANK, 0x8c9, NAVI_HELP_BLANK, 0x8ca, NAVI_HELP_BLANK, 0x8cb, NAVI_HELP_BLANK,
    0x8cc, NAVI_HELP_BLANK, 0x8cd, 0x8ce, NAVI_HELP_BLANK, 0x8cf, 0x8d0, NAVI_HELP_BLANK, 0xc90,
    NAVI_HELP_BLANK, 0xc91, 0xc92,
};

struct UINavigationHelpWnd *ui_navigation_help_wnd_construct(struct UINavigationHelpWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_navigation_help_wnd_ops;
    self->text = NULL;
    self->close_button = 0;
    return self;
}

struct UIWindow *ui_navigation_help_wnd_new(void)
{
    return (struct UIWindow *)ui_navigation_help_wnd_construct(mem_alloc(sizeof(struct UINavigationHelpWnd)));
}

static void ui_navigation_help_wnd_destruct(struct UINavigationHelpWnd *self)
{
    self->frame.base.ops = &ui_navigation_help_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_navigation_help_wnd_destroy(struct UIWindow *window)
{
    struct UINavigationHelpWnd *self = (struct UINavigationHelpWnd *)window;
    ui_navigation_help_wnd_destruct(self);
    mem_free(self);
}

void ui_navigation_help_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UINavigationHelpWnd *self = (struct UINavigationHelpWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *text = mem_alloc(sizeof(struct UITextViewer));
    unsigned i;

    if (text)
        text = ui_text_viewer_construct(text);
    self->text = (struct UITextViewer *)text;
    ui_list_box_create_window(&self->text->list, 5, 0x16, width - 10, height - 0x1b, 0);
    self->text->list.b = NAVI_HELP_BACKGROUND;
    self->text->list.g = NAVI_HELP_BACKGROUND;
    self->text->list.r = NAVI_HELP_BACKGROUND;
    ui_window_add_child(wnd, &self->text->list.base);
    for (i = 0; i < sizeof navi_help_lines / sizeof navi_help_lines[0]; i++) {
        const char *line = navi_help_lines[i] == NAVI_HELP_BLANK ? " " : msg_str(navi_help_lines[i]);

        ui_window_send_msg(&self->text->list.base, NULL, UI_MSG_ADD_TEXT, (intptr_t)line, NAVI_HELP_TEXT_LAYOUT, 0, 0);
    }
    ui_window_add_close_button(wnd, msg_str(MSG_NAVI_HELP_CLOSE), 0);
}

void ui_navigation_help_wnd_on_draw(struct UIWindow *window)
{
    struct UINavigationHelpWnd *self = (struct UINavigationHelpWnd *)window;
    ui_window_draw_title_bar(&self->frame.base, 1, msg_str(MSG_NAVI_HELP_TITLE), 0);
    ui_window_draw_body(&self->frame.base, 0x11, 0, "");
}

intptr_t ui_navigation_help_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                           intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    if (msg == UI_MSG_COMMAND && val1 == UI_ID_CLOSE)
        ui_close_window(NAVI_HELP_WND_ID);
    return 0;
}
