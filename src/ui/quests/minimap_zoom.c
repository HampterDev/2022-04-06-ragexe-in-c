#include "game/cells.h"
#include "game/mode_misc.h"
#include "contents/town_info.h"
#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "navi/navi_manager.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/draw.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_state.h"
#include "std/tree.h"
#include "ui/wndmgr/wndmgr.h"

#define MZ_MSG_TOGGLE_ZOOM 0x1ee

struct UIMinimapZoomWnd {
    struct UIFrameWnd frame;
    struct Tree buttons;
    struct Vec row;
};
OFFSET_ASSERT(UIMinimapZoomWnd, buttons, 0x94);
OFFSET_ASSERT(UIMinimapZoomWnd, row, 0x9c);
struct ZoomButton {
    int index;
    struct UIWindow *button;
};

#define MZ_CMD_MINI 0xb5
#define MZ_CMD_PLUS 0xd0
#define MZ_CMD_MINUS 0xd1
#define MZ_CMD_OBJECT 0x1bc
#define MZ_CMD_VIEWON 0x1be
#define MZ_MSG_PLACE 0x1bd
#define MZ_MSG_HIDE 0x1bf
#define MZ_TOWN_ID_FIRST 1000
#define MZ_BIG_MAP_WND 0x111
#define MZ_WORLD_MAP_WND 0x8c

static void track_facilities(struct UIMinimapZoomWnd *self)
{
    self->frame.base.redraws_tree = g_windowMgr.minimap_show_navigation && self->buttons.size;
}

void ui_minimap_zoom_wnd_on_create(struct UIWindow *self, int width, int height)
{
    static const int ids[5] = { 0x1bc, 0xd0, 0xd1, 0xb5, 0x1be };
    static const char *const names[5] = { "object", "plus", "minus", "mini", "viewon" };
    struct Vec *row = &((struct UIMinimapZoomWnd *)self)->row;
    int xs[5], ys[5];
    const char *tips[5];
    struct String prefix;
    int i;

    (void)width;
    (void)height;
    for (i = 0; i < 5; i++)
        ys[i] = 0x80;
    xs[0] = 1;
    xs[1] = self->width - 0x37;
    xs[2] = self->width - 0x29;
    xs[3] = self->width - 0x1b;
    xs[4] = self->width - 0xd;
    for (i = 0; i < 5; i++)
        tips[i] = msg_str(0xb27 + i);
    string_init(&prefix);
    string_assign_cstr(&prefix, "minimap\\i_");
    row->last = row->first;
    for (i = 0; i < 5; i++) {
        struct String name;
        struct UIWindow *button;

        string_copy(&name, &prefix);
        string_append_cstr(&name, names[i]);
        button = &ui_window_add_numbered_button(self, ids[i], xs[i], ys[i], string_cstr(&name), tips[i], 1)->button.base;
        vec_push(row, &button, sizeof button);
        string_free(&name);
    }
    ui_window_send_msg(self, self, 6, MZ_MSG_TOGGLE_ZOOM, 0, 0, 0);

    for (i = 0;; i++) {
        struct String map, tooltip, icon;
        struct TownInfo info;
        void *mem;
        struct UIBitmapButton *button;
        struct ZoomButton entry;

        game_mode_get_map_name(game_current_mode(), &map);
        town_info_get(g_townInfo, &info, map, i);
        if (!info.icon.size) {
            string_free(&info.icon);
            string_free(&info.name);
            break;
        }
        string_copy(&tooltip, &info.name);
        string_copy(&icon, &info.icon);
        mem = mem_alloc(sizeof(struct UIBitmapButton));
        button = mem ? ui_bitmap_button_construct(mem) : NULL;
        ui_bitmap_button_set_bitmap(button, string_cstr(&icon), 0, 0);
        ui_bitmap_button_set_bitmap(button, string_cstr(&icon), 1, 0);
        ui_bitmap_button_set_bitmap(button, string_cstr(&icon), 2, 0);
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, 0, 0);
        ui_button_show((struct UIButton *)(button), 0);
        button->button.base.id = i + 1000;
        ui_bitmap_button_set_tool_tip(button, string_cstr(&tooltip));
        ui_window_add_child(self, &button->button.base);
        entry.index = i;
        entry.button = &button->button.base;
        tree_add(&((struct UIMinimapZoomWnd *)self)->buttons, &entry, sizeof entry, tree_less_int, NULL);
        string_free(&icon);
        string_free(&tooltip);
        string_free(&info.icon);
        string_free(&info.name);
    }
    string_free(&prefix);
    track_facilities((struct UIMinimapZoomWnd *)self);
}

void ui_minimap_zoom_wnd_on_draw(struct UIWindow *self)
{
    struct String text;
    int x = 0, y = 0;

    ui_window_clear_canvas(self, 0xffff00ff);
    game_mode_get_player_cell(game_current_mode(), &x, &y);
    string_init(&text);
    string_format(&text, " %d  %d", x, y);
    ui_window_text_out(self, 0x13, 0x80, string_cstr(&text), 0, 0, 0xc, 0, 0, 0);
    ui_window_text_out(self, 0x14, 0x80, string_cstr(&text), 0, 0, 0xc, 0xffffff, 0, 0);
    string_free(&text);
}

static void show_facility(struct UIWindow *wnd, struct UIWindow *button, BYTE show)
{
    if (button->visible == show && ((struct UIButton *)button)->disabled == (show ^ 1))
        return;
    ui_button_show((struct UIButton *)(button), show);
    ui_window_invalidate_base(wnd);
}

static void place_facility(struct UIWindow *wnd, struct UIWindow *button, int x, int y)
{
    int old_x = button->x, old_y = button->y;

    ui_window_move(button, x, y);
    if (button->x != old_x || button->y != old_y)
        ui_window_invalidate_base(wnd);
}

static struct UIWindow *facility_button(struct UIMinimapZoomWnd *self, int index)
{
    struct ZoomButton *entry = tree_find(&self->buttons, &index, tree_less_int);

    return entry ? entry->button : NULL;
}

static void toggle_window(int window_id)
{
    if (!ui_close_window(window_id))
        ui_window_open(window_id);
}

static void navigate_to_facility(int id)
{
    struct String map, arg;
    struct TownInfo info;

    game_mode_get_map_name(game_current_mode(), &map);
    string_copy(&arg, &map);
    town_info_get(g_townInfo, &info, arg, id - MZ_TOWN_ID_FIRST);
    if (info.icon.size != 0) {
        size_t dot = string_find_first_of(&map, ".", 0);

        if (dot != (size_t)-1) {
            string_truncate(&map, dot);
        }
        navi_navigate(&g_navi, string_cstr(&map), 0, 5, 1, info.x, info.y, 0x3ea);
    }
    string_free(&info.icon);
    string_free(&info.name);
    string_free(&map);
}

int ui_minimap_zoom_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                        int msg, int val1, int val2, int val3, int val4)
{
    struct UIMinimapZoomWnd *self = (struct UIMinimapZoomWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *button;
    struct TreeNode *it;
    struct UIWindow **p;
    float zoom;

    if (msg != UI_MSG_COMMAND)
        return ui_window_send_msg_base(wnd, sender, msg, val1, val2, val3, val4);

    switch (val1) {
    case MZ_CMD_PLUS:
        zoom = g_windowMgr.minimap_zoom;
        g_windowMgr.minimap_zoom = (zoom / 3.0f + zoom) * 0.25f + zoom;
        if (g_windowMgr.minimap_zoom > 4.0f)
            g_windowMgr.minimap_zoom = 4.0f;
        return 0;
    case MZ_CMD_MINUS:
        zoom = g_windowMgr.minimap_zoom;
        g_windowMgr.minimap_zoom = zoom - zoom * 0.25f;
        if (1.0f > g_windowMgr.minimap_zoom)
            g_windowMgr.minimap_zoom = 1.0f;
        return 0;
    case MZ_CMD_MINI:
        if (!session_is_not_minimap_exception(&g_session1)) {
            ui_message_box(msg_str(0xacf));
            return 0;
        }
        toggle_window(MZ_BIG_MAP_WND);
        return 0;
    case MZ_CMD_OBJECT:
        g_windowMgr.minimap_show_navigation = g_windowMgr.minimap_show_navigation == 0;
        for (it = tree_first(&self->buttons); it != self->buttons.head; it = tree_next(it))
            ui_button_show((struct UIButton *)(TREE_VALUE(it, struct ZoomButton)->button),
                           (BYTE)g_windowMgr.minimap_show_navigation);
        track_facilities(self);
        ui_window_invalidate_base(wnd);
        return 0;
    case MZ_MSG_PLACE:
        button = facility_button(self, val2);
        if (button) {
            show_facility(wnd, button, 1);
            place_facility(wnd, button, val3 - wnd->x, val4 - wnd->y);
        }
        return 0;
    case MZ_CMD_VIEWON:
        toggle_window(MZ_WORLD_MAP_WND);
        return 0;
    case MZ_MSG_HIDE:
        button = facility_button(self, val2);
        if (button)
            show_facility(wnd, button, 0);
        return 0;
    case MZ_MSG_TOGGLE_ZOOM:
        for (p = (struct UIWindow **)self->row.first; p != (struct UIWindow **)self->row.last; p++)
            ui_button_show((struct UIButton *)(*p), g_session1.minimapButtons != 0);
        ui_window_invalidate_base(wnd);
        navigate_to_facility(val1);
        return 0;
    default:
        navigate_to_facility(val1);
        return 0;
    }
}

struct UIMinimapZoomWnd;
struct UIWindow;
void ui_minimap_zoom_wnd_destroy(struct UIWindow *window);
void ui_minimap_zoom_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_minimap_zoom_wnd_on_draw(struct UIWindow *self);
int ui_minimap_zoom_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_minimap_zoom_wnd_ops = {
    .destroy = ui_minimap_zoom_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_minimap_zoom_wnd_claims_mouse,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_minimap_zoom_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_minimap_zoom_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_minimap_zoom_wnd_on_l_btn_down,
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
    .send_msg = ui_minimap_zoom_wnd_send_msg,
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

struct UIWindow *ui_minimap_zoom_wnd_new(void)
{
    struct UIMinimapZoomWnd *self = mem_alloc(sizeof *self);

    if (!self)
        return NULL;
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_minimap_zoom_wnd_ops;
    tree_init(&self->buttons);
    self->row.first = NULL;
    self->row.last = NULL;
    self->row.end = NULL;
    return &self->frame.base;
}

void ui_minimap_zoom_wnd_destruct(struct UIMinimapZoomWnd *self)
{
    self->frame.base.ops = &ui_minimap_zoom_wnd_ops;
    vec_free(&self->row);
    tree_free(&self->buttons, NULL);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_minimap_zoom_wnd_destroy(struct UIWindow *window)
{
    struct UIMinimapZoomWnd *self = (struct UIMinimapZoomWnd *)window;
    ui_minimap_zoom_wnd_destruct(self);
    mem_free(self);
}
