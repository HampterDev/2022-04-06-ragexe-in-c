#include "ui/controls/listbox.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "navi/snapshot.h"
#include "replay/replay.h"
#include "render/texture_image.h"
#include "navi/navi_manager.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include <string.h>
#include "ui/worldmap/navi_wnd.h"
#include <mmsystem.h>
#include "core/file_util.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "ui/wndmgr/wndmgr.h"
#include "core/app.h"

struct Vec;
struct UINavigationWnd;
struct UIWindow;
void ui_navigation_wnd_destroy(struct UIWindow *window);
void ui_navigation_wnd_store_info(struct UIWindow *self);
void ui_navigation_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_navigation_wnd_on_draw(struct UIWindow *self);
void ui_navigation_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_navigation_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_navigation_wnd_ops = {
    .destroy = ui_navigation_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_navigation_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_navigation_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_navigation_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_navigation_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_navigation_wnd_send_msg,
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

#define SKIN_LEN 0xf
#define OFF_SCREEN (-1000)
#define NAVI_LIST(w) ((struct UIListBoxNavi *)(w))

void ui_navigation_wnd_destruct(struct UINavigationWnd *self)
{
    self->frame.base.ops = &ui_navigation_wnd_ops;
    string_vec_free(&self->files);
    string_free(&self->folded_text);
    string_free(&self->title);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_navigation_wnd_destroy(struct UIWindow *window)
{
    struct UINavigationWnd *self = (struct UINavigationWnd *)window;
    ui_navigation_wnd_destruct(self);
    mem_free(self);
}

static struct UIWindow *add_list(struct UINavigationWnd *self, int id)
{
    struct UIListBoxNavi *list = mem_alloc(sizeof *list);
    struct UIWindow *wnd;

    if (list)
        list = ui_list_box_navi_construct(list);
    wnd = &list->list.base;
    ui_window_create(wnd, 0xd7, 0x60);
    ((struct UIListBox *)wnd)->r = 0xd2;
    ((struct UIListBox *)wnd)->g = 0xd2;
    ((struct UIListBox *)wnd)->b = 0xd2;
    wnd->id = id;
    ui_window_move_base(wnd, OFF_SCREEN, OFF_SCREEN);
    return wnd;
}

static void button_bitmap(struct UIBitmapButton *button, const struct String *dir, const char *name,
                          const char *suffix, int state)
{
    struct String path;

    string_copy(&path, dir);
    string_append_cstr(&path, name);
    if (suffix)
        string_append_cstr(&path, suffix);
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
    string_free(&path);
}

static struct UIWindow *add_check(struct UINavigationWnd *self, struct String *dir, const struct String *skin,
                                  int label, BYTE checked, int tip)
{
    struct UIWindow *check = mem_alloc(sizeof(struct UICheckBox));
    struct String unchecked, on;

    if (check)
        check = (struct UIWindow *)ui_check_box_construct(check);
    ui_window_create(check, ui_window_text_width(&self->frame.base, msg_str(label), 0xa, 0, 0xc, 0, 0), 0xc);
    string_assign(dir, string_cstr(skin), skin->size);
    string_copy(&unchecked, dir);
    string_append_cstr(&unchecked, "checkbox_0.bmp");
    string_copy(&on, dir);
    string_append_cstr(&on, "checkbox_1.bmp");
    ui_check_box_set_bitmaps(check, string_cstr(&on), string_cstr(&unchecked), msg_str(label));
    string_free(&on);
    string_free(&unchecked);
    ui_window_move_base(check, OFF_SCREEN, OFF_SCREEN);
    ui_check_box_set_check(check, checked);
    ui_check_box_set_tool_tip(check, msg_str(tip));
    ui_window_add_child(&self->frame.base, check);
    return check;
}

void ui_navigation_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UINavigationWnd *self = (struct UINavigationWnd *)window;
    static const char *const names[6] = {"NaviSearch", "NaviStart", "NaviRoute", "NaviList", "Navisyschange2",
                                         "navifolder"};
    static const int ids[6] = {NAVI_ID_SEARCH, NAVI_ID_START, NAVI_ID_ROUTE, NAVI_ID_LIST, NAVI_ID_WIDE,
                               NAVI_ID_FOLDER};
    struct UIWindow *wnd = &self->frame.base;
    struct String skin, dir, path;
    const char *tips[6];
    struct UIResizer *resizer;
    struct UIWindow *combo, *edit;
    const char *hint;
    int i;

    self->results = add_list(self, NAVI_ID_RESULTS);
    ui_window_add_child(wnd, self->results);
    self->route = add_list(self, NAVI_ID_ROUTE_LIST);
    ui_list_box_navi_set_flag_colour(NAVI_LIST(self->route), 0x1414f0);
    ui_window_add_child(wnd, self->route);

    string_init(&skin);
    string_assign(&skin, UI_SKIN, SKIN_LEN);
    resizer = mem_alloc(sizeof *resizer);
    if (resizer)
        resizer = ui_resizer_construct(resizer);
    self->resizer = resizer;
    ui_window_create(&resizer->base, 0xe, 0xe);
    ui_resizer_set_edge(self->resizer, UI_EDGE_BOTTOM_RIGHT);
    string_copy(&path, &skin);
    string_append_cstr(&path, "btn_resize.bmp");
    ui_resizer_set_bitmap(self->resizer, string_cstr(&path));
    string_free(&path);
    ui_resizer_set_colours(self->resizer, UI_TRANSPARENT, UI_TRANSPARENT);
    ui_window_move_base(&self->resizer->base, OFF_SCREEN, OFF_SCREEN);
    ui_window_add_child(wnd, &self->resizer->base);

    string_copy(&dir, &skin);
    string_append_cstr(&dir, "navigation_interface\\");
    for (i = 0; i < 6; i++)
        tips[i] = msg_str(0x894 + i);
    for (i = 0; i < 6; i++) {
        struct UIBitmapButton *button = mem_alloc(sizeof *button);

        if (button)
            button = ui_bitmap_button_construct(button);
        self->buttons[i] = button;
        button_bitmap(button, &dir, names[i], "_A.bmp", UI_BTN_UP);
        button_bitmap(self->buttons[i], &dir, names[i], "_B.bmp", UI_BTN_HOVER);
        button_bitmap(self->buttons[i], &dir, names[i], "_C.bmp", UI_BTN_DOWN);
        button = self->buttons[i];
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&self->buttons[i]->button.base, OFF_SCREEN, OFF_SCREEN);
        self->buttons[i]->button.base.id = ids[i];
        ui_bitmap_button_set_tool_tip(self->buttons[i], tips[i]);
        ui_window_add_child(wnd, &self->buttons[i]->button.base);
    }

    self->close = ui_window_add_close_button(wnd, msg_str(0x89a), 0);
    self->mini = ui_window_add_mini_button(wnd, "", 0);
    self->help = mem_alloc(sizeof *self->help);
    if (self->help)
        self->help = ui_bitmap_button_construct(self->help);
    button_bitmap(self->help, &dir, "NaviHelp_off.bmp", NULL, UI_BTN_UP);
    button_bitmap(self->help, &dir, "NaviHelp_on.bmp", NULL, UI_BTN_HOVER);
    button_bitmap(self->help, &dir, "NaviHelp_on.bmp", NULL, UI_BTN_DOWN);
    ui_window_create(&self->help->button.base, self->help->bitmap_width, self->help->bitmap_height);
    ui_window_move_base(&self->help->button.base, wnd->width - 0x2a, 3);
    self->help->button.base.id = NAVI_ID_HELP;
    ui_bitmap_button_set_tool_tip(self->help, msg_str(0x89d));
    ui_window_add_child(wnd, &self->help->button.base);

    combo = mem_alloc(sizeof(struct UIComboBoxDefWnd));
    if (combo)
        combo = ui_combo_box_def_wnd_construct(combo);
    self->mode_combo = combo;
    ((struct UIComboBoxDefWnd *)combo)->list_height = 0x43;
    ui_window_create(self->mode_combo, 0x38, 0x12);
    ui_window_move(self->mode_combo, OFF_SCREEN, OFF_SCREEN);
    combo = self->mode_combo;
    ((struct UIComboBoxDefWnd *)combo)->border_r = 0x64;
    ((struct UIComboBoxDefWnd *)combo)->border_g = 0x64;
    ((struct UIComboBoxDefWnd *)combo)->border_b = 0x64;
    ((struct UIComboBoxDefWnd *)combo)->back_r = 0xe8;
    ((struct UIComboBoxDefWnd *)combo)->back_g = 0xe8;
    ((struct UIComboBoxDefWnd *)combo)->back_b = 0xe8;
    ui_window_add_child(wnd, self->mode_combo);
    for (i = 0x89e; i <= 0x8a1; i++)
        ui_combo_add(self->mode_combo, msg_str(i));

    edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    self->edit = edit;
    ui_window_create(edit, 0xe2, 0x12);
    ui_window_move(self->edit, OFF_SCREEN, OFF_SCREEN);
    ((struct UIEditCtrl *)self->edit)->max_chars = 0x3c;
    ui_edit_ctrl_set_back_colour(self->edit, 0xe8, 0xe8, 0xe8);
    ui_edit_ctrl_set_border_colour(self->edit, 0xc8, 0x64, 0x64);
    ((struct UIEditCtrl *)self->edit)->no_border = 0;
    hint = msg_str(0x8a2);
    edit = self->edit;
    if (hint)
        string_assign_cstr(&((struct UIEditCtrl *)edit)->tooltip, hint);
    ui_window_add_child(wnd, edit);
    if (g_replayPlaying) {
        ((struct UIEditCtrl *)self->edit)->disabled = 1;
        ui_window_invalidate(self->edit);
    }

    self->check_scrolls = add_check(self, &dir, &skin, 0x8a3, g_navi.routeUseScrolls, 0x8a4);
    self->check_kafra = add_check(self, &dir, &skin, 0x8a5, g_navi.routeUseKafra, 0x8a6);
    self->check_airship = add_check(self, &dir, &skin, 0x8a7, g_navi.routeUseAirship, 0x8a8);
    string_free(&dir);
    string_free(&skin);
}

void ui_navigation_wnd_show_list(struct UINavigationWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *button;
    int x;

    ui_window_move_base(self->results, OFF_SCREEN, OFF_SCREEN);
    ui_window_move_base(self->route, OFF_SCREEN, OFF_SCREEN);
    ui_window_move_base(&self->buttons[2]->button.base, OFF_SCREEN, OFF_SCREEN);
    ui_window_move_base(&self->buttons[3]->button.base, OFF_SCREEN, OFF_SCREEN);
    x = wnd->width - 0xde;
    if (!g_navi.windowCollapsed) {
        if (g_navi.windowPage == 1) {
            ui_window_move_base(self->route, x, 0x25);
            ui_window_invalidate_base(self->route);
            button = &self->buttons[3]->button.base;
        } else {
            ui_window_move_base(self->results, x, 0x25);
            ui_window_invalidate_base(self->results);
            button = &self->buttons[2]->button.base;
        }
        ui_window_move_base(button, x, wnd->height - 0x29);
    }
    ui_window_invalidate_base(wnd);
}

void ui_navigation_wnd_place(struct UINavigationWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    int i, x, y;

    if (g_navi.windowCollapsed) {
        ui_window_resize_base(wnd, 0x86, 0x3d);
        x = wnd->x;
        y = wnd->y;
        if (g_renderer->width - 0x110 <= x)
            x = g_renderer->width - wnd->width;
        if (g_renderer->height - 0x152 <= y)
            y = g_renderer->height - wnd->height;
        ui_window_move(wnd, x < 0 ? 0 : x, y < 0 ? 0 : y);
        ui_window_move_base(&self->resizer->base, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(&self->help->button.base, wnd->width - 0x2a, 3);
        ui_window_move_base(self->close, wnd->width - 0xe, 3);
        ui_window_move_base(self->mini, wnd->width - 0x1c, 3);
        ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)self->mini, msg_str(0x89b));
        for (i = 0; i < 6; i++)
            ui_window_move_base(&self->buttons[i]->button.base, OFF_SCREEN, OFF_SCREEN);
        ui_navigation_wnd_show_list(self);
        ui_window_move(self->mode_combo, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(self->check_kafra, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(self->check_scrolls, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(self->check_airship, OFF_SCREEN, OFF_SCREEN);
        ui_window_move(self->edit, OFF_SCREEN, OFF_SCREEN);
    } else {
        int xs[6], ys[6];

        if (self->info->is_screen) {
            self->info->rect.width = 0x110;
            ui_window_resize(self->edit, 0xe2, 0x12);
        } else {
            self->info->rect.width = 0xe4;
            ui_window_resize(self->edit, 0xb6, 0x12);
        }
        ui_window_resize_base(wnd, self->info->rect.width, self->info->rect.height);
        x = g_renderer->width - self->info->rect.width;
        if (x > wnd->x)
            x = wnd->x;
        y = g_renderer->height - self->info->rect.height;
        if (y > wnd->y)
            y = wnd->y;
        ui_window_move(wnd, x < 0 ? 0 : x, y < 0 ? 0 : y);
        ui_list_box_resize(self->results, 0xd7, wnd->height - 0x4e);
        ui_list_box_resize(self->route, 0xd7, wnd->height - 0x4e);
        ui_window_move_base(&self->resizer->base, wnd->width - 0xe, wnd->height - 0xe);
        ui_window_move_base(&self->help->button.base, wnd->width - 0x2a, 3);
        ui_window_move_base(self->close, wnd->width - 0xe, 3);
        ui_window_move_base(self->mini, wnd->width - 0x1c, 3);
        ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)self->mini, msg_str(0x89c));
        xs[0] = wnd->width - 0x42;
        ys[0] = 0x12;
        xs[1] = wnd->width - 0x4d;
        ys[1] = wnd->height - 0x25;
        xs[2] = OFF_SCREEN;
        ys[2] = OFF_SCREEN;
        xs[3] = OFF_SCREEN;
        ys[3] = OFF_SCREEN;
        xs[4] = wnd->width - 0xe0;
        ys[4] = wnd->height - 0x16;
        xs[5] = wnd->width - 0x1f;
        ys[5] = 0x12;
        for (i = 0; i < 6; i++)
            ui_window_move_base(&self->buttons[i]->button.base, xs[i], ys[i]);
        ui_navigation_wnd_show_list(self);
        ui_window_move(self->mode_combo, 6, 0x12);
        ui_window_move_base(self->check_kafra, wnd->width - 0x88, wnd->height - 0x25);
        ui_window_move_base(self->check_scrolls, wnd->width - 0xba, wnd->height - 0x25);
        ui_window_move_base(self->check_airship, wnd->width - 0x88, wnd->height - 0x15);
        ui_window_move(self->edit, 0x41, 0x12);
        return;
    }
}

void ui_navigation_wnd_set_title(struct UINavigationWnd *self, int state)
{
    char line[0x104], where[0x104];
    const char *text;

    if (!g_navi.windowCollapsed) {
        text = msg_str(0x8ad);
        string_assign_cstr(&self->title, text);
        text = msg_str(0x8ac);
        string_assign_cstr(&self->folded_text, text);
    } else if (state == 2) {
        memset(line, 0, sizeof line);
        memset(where, 0, sizeof where);
        if (navi_manager_describe_destination(&g_navi, where, 0x104))
            sprintf(line, ">>%s", where);
        else
            sprintf(line, msg_str(0x8a9));
        text = msg_str(0x8aa);
        string_assign_cstr(&self->title, text);
        string_assign_cstr(&self->folded_text, line);
    } else {
        text = msg_str(0x8ab);
        string_assign_cstr(&self->title, text);
        text = msg_str(0x8ac);
        string_assign_cstr(&self->folded_text, text);
    }
    ui_window_invalidate_base(&self->frame.base);
}

void ui_navigation_wnd_store_info(struct UIWindow *window)
{
    struct UINavigationWnd *self = (struct UINavigationWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct NavigationWndInfo *info = self->info;
    int limit, x, y;

    if (!info)
        return;
    info->rect.width = wnd->width;
    self->info->rect.height = wnd->height;
    info = self->info;
    limit = info->is_screen ? 0x110 : 0xe4;
    x = wnd->x;
    if (g_renderer->width - limit < x) {
        wnd->x = g_renderer->width - limit;
        x = wnd->x;
    }
    if (x < 0) {
        wnd->x = 0;
        x = 0;
    }
    info->rect.x = x;
    y = wnd->y;
    if (g_renderer->height - 0x152 < y) {
        wnd->y = g_renderer->height - 0x152;
        y = wnd->y;
    }
    if (y < 0) {
        wnd->y = 0;
        y = 0;
    }
    self->info->rect.y = y;
}

void ui_navigation_wnd_on_draw(struct UIWindow *window)
{
    struct UINavigationWnd *self = (struct UINavigationWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct SearchResult guided;
    int mark_x, mark_y, kind, size, px, py;
    BYTE cross = 1;
    struct NaviUiRecord *line;
    int count;

    ui_window_draw_title_bar(wnd, 1, string_cstr(&self->title), 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
    if (g_navi.windowCollapsed) {
        ui_window_text_out(wnd, 6, 0x14, string_cstr(&self->folded_text), 0, 0, 0xc, 0, 0, 0);
        return;
    }
    if (!self->info->is_screen)
        return;
    mark_x = g_navi.minimapX;
    mark_y = g_navi.minimapY;
    kind = navi_manager_guided(&g_navi, &guided)->kind;
    search_result_destroy(&guided);
    ui_window_fill_rect(wnd, 6, 0x25, 0x82, 0x82, 0x646464);
    size = 0x80;

    if (g_navi.pictureKind == 2) {
        int sprite_width = 0x80;

        if (kind == NAVI_RESULT_NPC) {
            sprite_width = 0x80 - 0x3d;
            size = 0x3a;
            if (mark_x > -1 && mark_y > -1) {
                mark_x = (int)((float)mark_x * 0.453125f);
                mark_y = (int)((float)mark_y * 0.453125f);
            }
        } else if (kind == NAVI_RESULT_MOB) {
            size = 0x20;
        }
        ui_window_draw_actor_sprite(wnd, g_navi.sprRes, g_navi.actRes, 7, 0x26, sprite_width, 0x80, 0x5dc, 1, 0);
    }
    px = 0x87 - size;
    py = 0xa6 - size;
    ui_window_fill_rect(wnd, px, py, size, size, 0);
    if (g_navi.picture) {
        struct Texture *tex = g_navi.picture;
        int image_width = texture_image_width(tex), image_height = texture_image_height(tex);
        if (image_width > 0 && image_height > 0) {
            float scale = (float)size / (float)(double)image_width;
            dc_bitmap_draw_texels_scaled(wnd->canvas, px, py, size, size, tex, image_width, image_height, scale, scale, 0xffffffff, 0, 0, 0, 0, 0);
        }
    }

    if (g_navi.guideState == 2 && navi_manager_guided_on_map(&g_navi, g_session1.mapName)) {
        float f = (float)size * 0.0078125f;
        struct NaviPoint *dot;
        int cell_x = 0, cell_y = 0;

        cross = g_navi.windowPage == 1 ? 0 : 1;
        for (dot = g_navi.pathDots.first; dot != (struct NaviPoint *)g_navi.pathDots.last; dot++)
            ui_window_fill_rect(wnd, (int)((float)dot->x * f + (float)px), (int)((float)dot->y * f + (float)py),
                              2, 2, 0xf0f00a);
        if (navi_manager_player_cell(&g_navi, &cell_x, &cell_y)) {
            struct NaviPoint at;

            navi_manager_minimap_pos_by_name(&g_navi, &at, g_session1.mapName, cell_x, cell_y, (float)size);
            ui_window_fill_rect(wnd, at.x + px - 1, at.y + py - 1, 3, 3, 0xff);
            ui_window_fill_rect(wnd, at.x + px, at.y + py, 1, 1, 0xffff);
        }
    }

    if (mark_x >= 0 && mark_y >= 0) {
        if (cross) {
            ui_window_fill_rect(wnd, px, mark_y - 1 + py, size, 2, 0x1414c8);
            ui_window_fill_rect(wnd, px + mark_x - 1, py, 2, size, 0x1414c8);
        }
        mark_x += px;
        ui_window_fill_rect(wnd, mark_x - 2, mark_y - 2 + py, 4, 4, 0xff);
        ui_window_fill_rect(wnd, mark_x - 1, mark_y - 1 + py, 2, 2, 0xffff);
    }

    kind = g_navi.pictureKind;
    line = g_navi.infoLines.first;
    count = (int)((struct NaviUiRecord *)g_navi.infoLines.last - line);
    for (; count > 0; count--, line++) {
        if (line->style != kind)
            continue;
        ui_window_text_out(wnd, line->x + 1, line->y + 1, string_cstr(&line->text), 0, 0, 0xc, 0x1e1e1e,
                         0, 0);
        ui_window_text_out(wnd, line->x, line->y, string_cstr(&line->text), 0, 0, 0xc, 0xe6e6e6, 0, 0);
    }
}

void ui_navigation_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UINavigationWnd *self = (struct UINavigationWnd *)window;
    if ((unsigned)(x - 8) <= 0x7e && (unsigned)(y - 0x27) <= 0x7e)
        ui_window_send_msg(&self->frame.base, &self->frame.base, UI_MSG_COMMAND, NAVI_ID_PICTURE, 0, 0, 0);
    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
}

static void add_line(struct UINavigationWnd *self, int list, const char *text)
{
    ui_window_send_msg(&self->frame.base, NULL, NAVI_MSG_ADD_LINE, list, (int)text, 0, 0);
}

void ui_navigation_wnd_fill_results(struct UINavigationWnd *self)
{
    char line[0x40];
    int i;

    ui_window_invalidate_base(self->results);
    ui_list_box_navi_clear_items((struct UIListBox *)self->results);
    if (!navi_manager_result_count(&g_navi))
        return;
    memset(line, 0, sizeof line);
    sprintf(line, msg_str(0x8ae), navi_manager_result_count(&g_navi));
    add_line(self, NAVI_ID_RESULTS, line);
    ui_list_box_navi_set_row_base(NAVI_LIST(self->results), 1);
    for (i = 0; i < navi_manager_result_count(&g_navi); i++) {
        struct SearchResult r;

        memset(line, 0, sizeof line);
        navi_manager_result_at(&g_navi, &r, i);
        if (r.kind == NAVI_RESULT_NPC) {
            const char *name = navi_object_name(r.obj);

            sprintf(line, msg_str(0x8af), navi_map_name(navi_object_map(r.obj)), name);
        } else if (r.kind == NAVI_RESULT_MOB) {
            void *mob = r.obj;
            int level = navi_object_sprite_high(mob);

            if (level > 0) {
                struct String rank;
                const char *name;
                int msg = 0;

                string_init(&rank);
                string_assign(&rank, "없음", 4);
                if (level > 0x63)
                    msg = 0x991;
                else if (level > 0x31)
                    msg = 0x992;
                else if ((unsigned)(level - 0x14) <= 0x1d)
                    msg = 0x993;
                else if ((unsigned)(level - 5) <= 0xe)
                    msg = 0x994;
                else if ((unsigned)(level - 1) <= 3)
                    msg = 0x995;
                if (msg) {
                    const char *text = msg_str(msg);

                    string_assign_cstr(&rank, text);
                }
                name = navi_object_name(mob);
                sprintf(line, msg_str(0x98f), navi_map_name(navi_object_map(mob)), name,
                            string_cstr(&rank));
                string_free(&rank);
            } else {
                const char *name = navi_object_name(mob);

                sprintf(line, msg_str(0x8b0), navi_map_name(navi_object_map(mob)), name);
            }
        } else if (r.kind == NAVI_RESULT_MAP) {
            sprintf(line, msg_str(0x8b1), navi_map_name(r.obj));
        } else if (r.kind == NAVI_RESULT_PLACE) {
            if (r.name.size == 0)
                sprintf(line, msg_str(0x975), navi_map_name(r.obj), r.x, r.y);
            else
                sprintf(line, msg_str(0x976), string_cstr(&r.name), navi_map_name(r.obj), r.x, r.y);
        }
        add_line(self, NAVI_ID_RESULTS, line);
        search_result_destroy(&r);
    }
}

static void add_map_line(struct UINavigationWnd *self, char *line, struct NaviMap *map)
{
    const char *name = navi_map_name(map);

    sprintf(line, msg_str(0x8b4), navi_map_display_name(map), name);
    add_line(self, NAVI_ID_ROUTE_LIST, line);
}

void ui_navigation_wnd_fill_route(struct UINavigationWnd *self)
{
    struct SearchResult dest;
    char line[0x64];
    int links, first_line, i;
    void *prev = NULL;

    ui_list_box_navi_clear_items((struct UIListBox *)self->route);
    ui_list_box_navi_clear_flags(NAVI_LIST(self->route));
    ui_window_invalidate_base(self->route);
    links = navi_manager_route_link_count(&g_navi);
    if (g_navi.guideState != 2)
        return;
    navi_manager_destination(&g_navi, &dest);
    if (!dest.obj)
        goto done;
    memset(line, 0, sizeof line);
    add_line(self, NAVI_ID_ROUTE_LIST, msg_str(0x8b2));
    memset(line, 0, sizeof line);
    sprintf(line, msg_str(0x8b3), navi_manager_walk_distance(&g_navi), links);
    add_line(self, NAVI_ID_ROUTE_LIST, line);

    switch (dest.kind) {
    case NAVI_RESULT_NPC: {
        struct NaviPoint a, b;
        int y;

        memset(line, 0, sizeof line);
        add_map_line(self, line, navi_object_map(dest.obj));
        memset(line, 0, sizeof line);
        y = navi_object_position((struct NaviObject *)dest.obj, &a)->y;
        sprintf(line, msg_str(0x8b5), navi_object_name(dest.obj), navi_object_position((struct NaviObject *)dest.obj, &b)->x, y);
        add_line(self, NAVI_ID_ROUTE_LIST, line);
        first_line = 5;
        break;
    }
    case NAVI_RESULT_MOB: {
        struct String where;

        memset(line, 0, sizeof line);
        add_map_line(self, line, navi_object_map(dest.obj));
        string_init(&where);
        string_assign_cstr(&where,
                               msg_str(navi_object_type(dest.obj) == NAVI_TYPE_MOB_BOSS ? 0x8b6 : 0x8b7));
        memset(line, 0, sizeof line);
        sprintf(line, msg_str(0x8b8), navi_object_name(dest.obj), string_cstr(&where));
        add_line(self, NAVI_ID_ROUTE_LIST, line);
        string_free(&where);
        first_line = 5;
        break;
    }
    case NAVI_RESULT_MAP:
        memset(line, 0, sizeof line);
        add_map_line(self, line, dest.obj);
        first_line = 4;
        break;
    case NAVI_RESULT_PLACE: {
        struct NaviPoint a, b;
        int y;

        memset(line, 0, sizeof line);
        add_map_line(self, line, dest.obj);
        memset(line, 0, sizeof line);
        y = navi_manager_target_pos2(&g_navi, &a)->y;
        sprintf(line, msg_str(0x8b9), navi_manager_target_pos2(&g_navi, &b)->x, y);
        add_line(self, NAVI_ID_ROUTE_LIST, line);
        first_line = 5;
        break;
    }
    default:
        goto done;
    }
    add_line(self, NAVI_ID_ROUTE_LIST, msg_str(0x8ba));
    ui_list_box_navi_set_row_base(NAVI_LIST(self->route), first_line);

    for (i = 0; i < links; i++) {
        void *link;

        memset(line, 0, 0x40);
        link = navi_manager_route_link_at(&g_navi, i);
        if (i == 0) {
            if (navi_object_type(link) == NAVI_TYPE_WARP_SCROLL) {
                const char *to = navi_link_dest_map_name(link);

                sprintf(line, msg_str(0x8bb), 0, navi_object_name(link), to);
            } else {
                struct NaviPoint a, b;
                int cell_x = 0, cell_y = 0, y;

                navi_manager_player_cell(&g_navi, &cell_x, &cell_y);
                y = navi_object_position((struct NaviObject *)link, &a)->y;
                sprintf(line, msg_str(0x8bc), 0, navi_map_name(navi_object_map(link)), cell_x, cell_y,
                            navi_object_position((struct NaviObject *)link, &b)->x, y);
            }
        } else {
            struct NaviPoint a, b, c, d;
            int to_y = navi_object_position((struct NaviObject *)link, &a)->y;
            int to_x = navi_object_position((struct NaviObject *)link, &b)->x;
            int from_y = navi_link_dest_pos(prev, &c)->y;
            int from_x = navi_link_dest_pos(prev, &d)->x;

            sprintf(line, msg_str(0x8bc), i, navi_map_name(navi_object_map(link)), from_x, from_y, to_x,
                        to_y);
        }
        add_line(self, NAVI_ID_ROUTE_LIST, line);
        prev = link;
    }

    {
        struct SearchResult again;
        struct String target;
        struct NaviPoint a, b;
        int kind, from_x, from_y, to_x, to_y;
        const char *name;

        kind = navi_manager_destination(&g_navi, &again)->kind;
        search_result_destroy(&again);
        name = navi_map_name(navi_manager_target_map2(&g_navi));
        string_init(&target);
        string_assign_cstr(&target, name);
        if (links == 0) {
            navi_manager_player_cell(&g_navi, &from_x, &from_y);
        } else {
            from_x = navi_link_dest_pos(prev, &a)->x;
            from_y = navi_link_dest_pos(prev, &a)->y;
        }
        to_x = navi_manager_target_pos2(&g_navi, &a)->x;
        to_y = navi_manager_target_pos2(&g_navi, &b)->y;
        if (kind != NAVI_RESULT_NPC && kind != NAVI_RESULT_PLACE)
            sprintf(line, msg_str(0x8be), links, string_cstr(&target));
        else
            sprintf(line, msg_str(0x8bd), links, string_cstr(&target), from_x, from_y, to_x, to_y);
        add_line(self, NAVI_ID_ROUTE_LIST, line);
        if (navi_manager_route_step(&g_navi) != -1) {
            ui_list_box_navi_set_flag(NAVI_LIST(self->route), navi_manager_route_step(&g_navi), 1);
            ui_window_invalidate_base(self->route);
        } else if (navi_manager_arrives_near_target(&g_navi)) {
            ui_list_box_navi_set_flag(NAVI_LIST(self->route), links, 1);
            ui_window_invalidate_base(self->route);
        }
        string_free(&target);
    }
done:
    search_result_destroy(&dest);
}

BYTE ui_navigation_wnd_load_place_file(struct UINavigationWnd *self, int index)
{
    struct String name, path;
    unsigned count;
    FILE *file;
    BYTE done;

    if (self->files.first == self->files.last)
        return 0;
    count = (unsigned)((struct String *)self->files.last - (struct String *)self->files.first);
    if ((unsigned)index > count || index < 0)
        return 0;
    {
        const char *text = string_cstr(&((struct String *)self->files.first)[index]);

        string_init(&name);
        string_assign_cstr(&name, text);
    }
    if (name.size == 0) {
        string_free(&name);
        return 0;
    }
    string_init(&path);
    string_format(&path, "%s\\Navigationdata\\%s", g_clientDir, string_cstr(&name));
    file = fopen(string_cstr(&path), "rb");
    if (!file) {
        done = 0;
    } else {
        static const char delims_init[6] = "\t ,\t\n";
        char delims[6];
        char buf[0x400];
        struct Vec words = {0};

        navi_manager_clear_results(&g_navi);
        memcpy(delims, delims_init, sizeof delims);
        memset(buf, 0, sizeof buf);
        while (!feof(file)) {
            char *word;

            fgets(buf, 0x400, file);
            for (word = strtok(buf, delims); word; word = strtok(NULL, delims))
                string_vec_push_text(&words, word, strlen(word));
            navi_manager_add_place(&g_navi, &words);
            string_vec_clear(&words);
        }
        fclose(file);
        if (navi_manager_result_count(&g_navi) > 0) {
            struct ReplayBuffer bytes = {0};

            if (!navi_results_save(&g_navi, &bytes) && replay_recorder_instance()->state == 2)
                replay_recorder_instance()->writeFailed = 1;
            ui_window_record_bytes(&self->frame.base, 0x38f, bytes.data, bytes.size);
            replay_buffer_destroy(&bytes);
        }
        ui_navigation_wnd_fill_results(self);
        ui_window_send_msg(&self->frame.base, &self->frame.base, UI_MSG_COMMAND, NAVI_ID_LIST, 0, 0, 0);
        done = 1;
        string_vec_free(&words);
    }
    string_free(&path);
    string_free(&name);
    return done;
}

static void list_place_files(struct UINavigationWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct String *s, dir;
    struct UIWindow *combo;
    int count, height, x, y;

    if (g_windowMgr.combo_box_wnd)
        ui_close_window(UI_WND_COMBO_BOX);
    for (s = self->files.first; s != (struct String *)self->files.last; s++) {
        string_clear(s);
    }
    for (s = self->files.first; s != (struct String *)self->files.last; s++)
        string_free(s);
    self->files.last = self->files.first;
    string_init(&dir);
    string_format(&dir, "%s\\Navigationdata\\", g_clientDir);
    dir_list_names_vector(&self->files, string_cstr(&dir), "*.txt");
    count = (int)((struct String *)self->files.last - (struct String *)self->files.first);
    if (count) {
        combo = ui_make_window(UI_WND_COMBO_BOX);
        for (s = self->files.first; s != (struct String *)self->files.last; s++)
            ui_window_send_msg(combo, NULL, 0x1f, (int)string_cstr(s), 0, 0, 0);
        height = count << 4;
        if (height > 0xa0)
            height = 0xa0;
        ui_window_send_msg(combo, NULL, 0x28, 0, 0, 0, 0);
        ui_window_resize(combo, 0xa0, height);
        ui_window_send_msg(combo, NULL, 0x53, WND_NAVIGATION, 0, 0, 0);
        x = wnd->width - 0xa5;
        y = 0x24;
        ui_window_to_screen(wnd, &x, &y);
        ui_window_move(combo, x, y);
    }
    string_free(&dir);
}

static void pick_line(struct UINavigationWnd *self, struct UIWindow *list, int id, int line, int val3, int val4)
{
    int index;
    unsigned char guided;

    if (!g_replayPlaying)
        line = ((struct UIListBox *)list)->selected;
    if (line == -1)
        return;
    ui_window_record_command(&self->frame.base, id, line, val3, val4);
    index = line - ui_list_box_navi_row_base(NAVI_LIST(list));
    if (id == NAVI_ID_RESULTS) {
        if (index < 0)
            return;
        guided = navi_manager_guide_result(&g_navi, index);
    } else {
        guided = navi_manager_guide_link(&g_navi, index);
    }
    if (!guided)
        return;
    if (g_windowMgr.kafra_move_map_service_info_wnd)
        ui_ro_map_wnd_load_guided_map((struct UIRoMapWnd *)g_windowMgr.kafra_move_map_service_info_wnd);
    ui_window_invalidate_base(&self->frame.base);
}

static void command(struct UINavigationWnd *self, struct UIWindow *sender, int id, int val2, int val3, int val4)
{
    struct UIWindow *wnd = &self->frame.base;

    if (id != NAVI_ID_RESULTS && id != NAVI_ID_ROUTE_LIST)
        ui_window_record_command(wnd, id, val2, val3, val4);
    switch (id) {
    case NAVI_ID_START: {
        int status;

        ui_window_invalidate_base(wnd);
        status = navi_manager_destination_from_guided(&g_navi);
        if (!status) {
            struct SearchResult dest;
            struct SearchResult *d = navi_manager_destination(&g_navi, &dest);

            status = navi_manager_guide_to(&g_navi, d->x, d->y);
            search_result_destroy(&dest);
            if (status >= 0) {
                ui_window_send_msg(wnd, wnd, UI_MSG_COMMAND, NAVI_ID_ROUTE, 0, 0, 0);
                ui_navigation_wnd_fill_route(self);
                if (g_windowMgr.kafra_move_map_service_info_wnd)
                    ui_ro_map_wnd_load_route_maps((struct UIRoMapWnd *)g_windowMgr.kafra_move_map_service_info_wnd);
            }
        }
        navi_manager_report_status(&g_navi, status, 1);
        return;
    }
    case NAVI_ID_ROUTE:
        g_navi.windowPage = 1;
        ui_navigation_wnd_show_list(self);
        return;
    case NAVI_ID_LIST:
        g_navi.windowPage = 0;
        ui_navigation_wnd_show_list(self);
        return;
    case NAVI_ID_FOLDER:
        list_place_files(self);
        return;
    case UI_ID_CLOSE:
        if (g_navi.guideState == 2) {
            if (ui_window_mgr_message_box(&g_windowMgr, msg_str(0x8bf), UI_MSGBOX_QUESTION, 1, 0, 0, "메시지",
                                       0x118, 0x78, 0) != UI_MSGBOX_ANSWER_YES)
                return;
            navi_manager_set_guide_state(&g_navi, 0);
        }
        world_view_clear_marks(&g_worldView);
        world_view_clear_level_marks(&g_worldView);
        world_view_clear_current(&g_worldView);
        navi_manager_reset(&g_navi, 1);
        g_navi.windowCollapsed = 0;
        g_navi.windowHidden = 0;
        if (g_windowMgr.kafra_move_map_service_info_wnd) {
            g_navi.windowCollapsed = 1;
            ui_ro_map_wnd_place_search((struct UIRoMapWnd *)g_windowMgr.kafra_move_map_service_info_wnd);
            return;
        }
        ui_window_mgr_post_quit2(&g_windowMgr, wnd);
        g_windowMgr.navigation_v4_wnd = NULL;
        return;
    case NAVI_ID_FOLD:
        g_navi.windowCollapsed = !g_navi.windowCollapsed;
        ui_navigation_wnd_place(self);
        ui_navigation_wnd_set_title(self, g_navi.guideState);
        if (g_navi.windowCollapsed && g_windowMgr.kafra_move_map_service_info_wnd)
            ui_ro_map_wnd_place_search((struct UIRoMapWnd *)g_windowMgr.kafra_move_map_service_info_wnd);
        return;
    case NAVI_ID_CHECK:
        if (sender == self->check_kafra)
            g_navi.routeUseKafra = val2 != 0;
        else if (sender == self->check_scrolls)
            g_navi.routeUseScrolls = val2 != 0;
        else if (sender == self->check_airship)
            g_navi.routeUseAirship = val2 != 0;
        return;
    case NAVI_ID_WIDE:
        self->info->is_screen = !self->info->is_screen;
        ui_navigation_wnd_place(self);
        ui_window_invalidate_base(wnd);
        return;
    case NAVI_ID_SEARCH:
        if (!ui_edit_ctrl_get_text_size(self->edit))
            return;
        string_assign_cstr(&g_navi.searchText, ui_edit_ctrl_get_text(self->edit));
        navi_manager_search_again(&g_navi);
        ui_window_send_msg(wnd, wnd, UI_MSG_COMMAND, NAVI_ID_LIST, 0, 0, 0);
        if (g_windowMgr.kafra_move_map_service_info_wnd)
            ui_ro_map_wnd_load_result_maps((struct UIRoMapWnd *)g_windowMgr.kafra_move_map_service_info_wnd);
        return;
    case NAVI_ID_RESULTS:
        pick_line(self, self->results, id, val2, val3, val4);
        return;
    case NAVI_ID_ROUTE_LIST:
        pick_line(self, self->route, id, val2, val3, val4);
        return;
    case NAVI_ID_HELP:
        if (!ui_close_window(WND_NAVI_HELP))
            ui_make_window(WND_NAVI_HELP);
        return;
    case NAVI_ID_PICTURE: {
        struct SearchResult guided;
        int kind;

        if (g_navi.windowCollapsed || !self->info->is_screen)
            return;
        kind = navi_manager_guided(&g_navi, &guided)->kind;
        search_result_destroy(&guided);
        if (kind == 1 || kind == 6 || kind == 0)
            return;
        if (g_navi.pictureKind == 1)
            g_navi.pictureKind = 2;
        else if (g_navi.pictureKind == 2)
            g_navi.pictureKind = 1;
        ui_window_invalidate_base(wnd);
        return;
    }
    }
}

static void enter_or_resize(struct UINavigationWnd *self, struct UIWindow *sender, int val1, int val3)
{
    if (self->edit == sender && ui_edit_ctrl_get_text_size(self->edit) > 0) {
        ui_window_send_msg(&self->frame.base, self->edit, UI_MSG_COMMAND, NAVI_ID_SEARCH, 0, 0, 0);
        return;
    }
    if (val1 == UI_EDGE_BOTTOM_RIGHT) {
        int h = val3 >= 0x152 ? val3 : 0x152;

        self->info->rect.height = h > 0x152 ? 0x152 : h;
        ui_navigation_wnd_place(self);
    }
}

int ui_navigation_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UINavigationWnd *self = (struct UINavigationWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case 0:
        enter_or_resize(self, sender, val1, val3);
        return 0;
    case UI_MSG_COMMAND:
        command(self, sender, val1, val2, val3, val4);
        return 0;
    case UI_MSG_RESIZE:
        if (val1 == UI_EDGE_BOTTOM_RIGHT) {
            int h = val3 >= 0x152 ? val3 : 0x152;

            self->info->rect.height = h > 0x152 ? 0x152 : h;
            ui_navigation_wnd_place(self);
        }
        return 0;
    case NAVI_MSG_ADD_LINE:
        if (val1 == NAVI_ID_RESULTS)
            ui_list_box_navi_add_item((struct UIListBox *)self->results, (const char *)val2);
        else if (val1 == NAVI_ID_ROUTE_LIST)
            ui_list_box_navi_add_item((struct UIListBox *)self->route, (const char *)val2);
        enter_or_resize(self, sender, val1, val3);
        return 0;
    case NAVI_MSG_INFO:
        self->info = (struct NavigationWndInfo *)val1;
        navi_window_info_clamp(self->info);
        ui_window_move(wnd, self->info->rect.x, self->info->rect.y);
        ui_window_invalidate_base(wnd);
        return 0;
    case NAVI_MSG_SELECT:
        if (self->mode_combo == sender) {
            g_navi.searchFilter = val1;
            ui_window_invalidate_base(wnd);
            if (g_replayPlaying)
                ui_window_send_msg(self->mode_combo, NULL, 0x39, val1, 0, 0, 0);
        } else if (!g_replayPlaying) {
            ui_navigation_wnd_load_place_file(self, val1);
        }
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
