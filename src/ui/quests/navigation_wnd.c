#include "ui/controls/listbox.h"
#include "ui/controls/edit.h"
#include "core/text_array.h"
#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "resources/palette.h"
#include "session/state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "navi/navi.h"
#include "ui/quests/navigation.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/ui_std.h"

#include <mmsystem.h>
#include <math.h>
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_names.h"

struct Vec;
struct UINavigationV4Wnd;
struct UIWindow;
void ui_navigation_v4_wnd_destroy(struct UIWindow *window);
void ui_navigation_v4_wnd_store_info(struct UIWindow *self);
void ui_navigation_v4_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_navigation_v4_wnd_on_process(struct UIWindow *self);
void ui_navigation_v4_wnd_on_draw(struct UIWindow *self);
void ui_navigation_v4_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
const struct UIWindowOps ui_navigation_v4_wnd_ops = {
    .destroy = ui_navigation_v4_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_navigation_v4_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_navigation_v4_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_navigation_v4_wnd_on_process,
    .on_draw = ui_navigation_v4_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_navigation_v4_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_navigation_v4_wnd_send_msg,
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

#define NAVI_HIDE (-1000)
#define NAVI_WIDTH 0x110
#define NAVI_HEIGHT 0x152
#define NAVI_MINI_WIDTH 0x86
#define NAVI_MINI_HEIGHT 0x3d

#define MSG_NAVI_TITLE 0x8ad
#define MSG_NAVI_FOUND 0xc8a
#define MSG_NAVI_TARGET_HINT 0xc8f

#define NAVI_DOT_SCALE 2.078125f
#define NAVI_PICTURE_X 3.0f
#define NAVI_PICTURE_Y 46.0f

#define MOVE(child, x, y) ui_window_move(UI_WINDOW((child)), (x), (y))

struct UINavigationV4Wnd *ui_navigation_v4_wnd_construct(struct UINavigationV4Wnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_navigation_v4_wnd_ops;
    self->files.first = NULL;
    self->files.last = NULL;
    self->files.end = NULL;
    self->hint_until = 0;
    self->airship = NULL;
    self->anim_period = 2000;
    return self;
}

struct UIWindow *ui_navigation_v4_wnd_new(void)
{
    return (struct UIWindow *)ui_navigation_v4_wnd_construct(mem_alloc(sizeof(struct UINavigationV4Wnd)));
}

void ui_navigation_v4_wnd_destruct(struct UINavigationV4Wnd *self)
{
    self->frame.base.ops = &ui_navigation_v4_wnd_ops;
    string_vec_free(&self->files);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_navigation_v4_wnd_destroy(struct UIWindow *window)
{
    struct UINavigationV4Wnd *self = (struct UINavigationV4Wnd *)window;
    ui_navigation_v4_wnd_destruct(self);
    mem_free(self);
}

void ui_navigation_v4_wnd_layout_route(struct UINavigationV4Wnd *self)
{
    int width;

    MOVE(self->mini_close, NAVI_HIDE, NAVI_HIDE);
    MOVE(self->maximize, NAVI_HIDE, NAVI_HIDE);
    width = self->frame.base.width;
    MOVE(self->help, width - 0x2a, 3);
    width = self->frame.base.width;
    MOVE(self->close_button, width - 0xe, 3);
    width = self->frame.base.width;
    MOVE(self->mini_button, width - 0x1c, 3);
    MOVE(self->kind_box, 0x1d, 0x15);
    MOVE(self->search, 0x5e, 0x15);
    MOVE(self->search_btn, 0xcf, 0x15);
    MOVE(self->navidata, 0xec, 0x15);
    MOVE(self->back, 4, 0x15);
    MOVE(self->road_icon, 0xed, 0x13d);
    MOVE(self->minimize_box, 9, 0x141);
    MOVE(self->route_list, 0xcb, 0x13d);
    MOVE(self->count_text, NAVI_HIDE, NAVI_HIDE);
    MOVE(self->results, NAVI_HIDE, NAVI_HIDE);
    MOVE(self->entries, NAVI_HIDE, NAVI_HIDE);
    MOVE(self->service, NAVI_HIDE, NAVI_HIDE);
    MOVE(self->share, NAVI_HIDE, NAVI_HIDE);
    MOVE(self->find, NAVI_HIDE, NAVI_HIDE);
    MOVE(self->airship, NAVI_HIDE, NAVI_HIDE);
    ui_window_invalidate_base(&self->frame.base);
}

void ui_navigation_v4_wnd_layout_list_page(struct UINavigationV4Wnd *self)
{
    int width;

    MOVE(self->mini_close, NAVI_HIDE, NAVI_HIDE);
    MOVE(self->maximize, NAVI_HIDE, NAVI_HIDE);
    width = self->frame.base.width;
    MOVE(self->help, width - 0x2a, 3);
    width = self->frame.base.width;
    MOVE(self->close_button, width - 0xe, 3);
    width = self->frame.base.width;
    MOVE(self->mini_button, width - 0x1c, 3);
    MOVE(self->kind_box, 0x1d, 0x15);
    MOVE(self->search, 0x5e, 0x15);
    MOVE(self->search_btn, 0xcf, 0x15);
    MOVE(self->navidata, 0xec, 0x15);
    MOVE(self->back, 4, 0x15);
    MOVE(self->road_icon, 0xed, 0x13d);
    MOVE(self->minimize_box, 9, 0x141);
    MOVE(self->route_list, 0xcb, 0x13d);
    MOVE(self->count_text, 0x14, 0x30);
    MOVE(self->results, 1, 0x46);
    MOVE(self->entries, 0x8b, 0xa6);
    MOVE(self->service, 0x92, 0xf7);
    MOVE(self->share, 0x91, 0x107);
    MOVE(self->find, 0xcf, 0x107);
    MOVE(self->airship, 0x91, 0x120);
    ui_window_invalidate_base(&self->frame.base);
}

void ui_navigation_v4_wnd_relayout(struct UINavigationV4Wnd *self)
{
    struct String text;

    if (g_navi.windowCollapsed == 1) {
        ui_window_resize_base(&self->frame.base, NAVI_MINI_WIDTH, NAVI_MINI_HEIGHT);
        MOVE(self->mini_close, self->frame.base.width - 0xf, 2);
        MOVE(self->maximize, self->frame.base.width - 0x20, 2);
        MOVE(self->help, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->close_button, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->mini_button, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->kind_box, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->search, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->search_btn, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->navidata, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->back, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->road_icon, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->minimize_box, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->count_text, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->results, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->entries, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->service, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->share, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->find, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->route_list, NAVI_HIDE, NAVI_HIDE);
        MOVE(self->airship, NAVI_HIDE, NAVI_HIDE);
    } else {
        ui_window_resize_base(&self->frame.base, NAVI_WIDTH, NAVI_HEIGHT);
        if (g_navi.windowPage == NAVI_VIEW_SEARCH)
            ui_navigation_v4_wnd_layout_list_page(self);
        else
            ui_navigation_v4_wnd_layout_route(self);
    }
    ui_window_set_show_base(&self->frame.base, g_navi.windowHidden == 0);
    ui_navigation_v4_wnd_fill_results(self);
    ui_navigation_v4_wnd_fill_entries(self, g_navi.selectedGroup);
    navi_manager_update_window(&g_navi);
    navi_manager_step_map_name(&g_navi, &text);
    string_free(&text);
    navi_manager_step_text(&g_navi, &text);
    string_free(&text);
    navi_manager_target_label(&g_navi, &text);
    string_free(&text);
    navi_manager_step_text_dots(&g_navi, &text);
    string_free(&text);
    navi_manager_target_chat_text(&g_navi, &text);
    string_free(&text);
    ui_window_invalidate_base(&self->frame.base);
}

static struct UIBitmapButton *new_button(void)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    return button ? ui_bitmap_button_construct(button) : NULL;
}

static void set_bitmaps(struct UIBitmapButton *button, const char *normal, const char *over, const char *press)
{
    ui_bitmap_button_set_bitmap(button, normal, 0, 0);
    ui_bitmap_button_set_bitmap(button, over, 1, 0);
    ui_bitmap_button_set_bitmap(button, press, 2, 0);
}

static void place_button(struct UINavigationV4Wnd *self, struct UIBitmapButton *button, int id, int tip)
{
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    button->button.base.id = id;
    if (tip)
        ui_bitmap_button_set_tool_tip(button, msg_str(tip));
    ui_window_add_child(&self->frame.base, &button->button.base);
}

static struct UIWindow *new_check_box(void)
{
    struct UIWindow *box = mem_alloc(sizeof(struct UICheckBox));

    return box ? (struct UIWindow *)ui_check_box_construct(box) : NULL;
}

static struct UIListBox_NaviSearch *new_list(void)
{
    struct UIListBox_NaviSearch *list = mem_alloc(sizeof *list);

    return list ? navi_list_construct(list) : NULL;
}

void ui_navigation_v4_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UINavigationV4Wnd *self = (struct UINavigationV4Wnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIListBox_NaviSearch *list;
    struct UIBitmapEditCtrl *search;
    struct UIComboBoxBmpWnd *kind_box;
    struct UIWindow *text;

    self->mini_close = new_button();
    set_bitmaps(self->mini_close, NAVI_DIR "btn_close_normal.bmp", NAVI_DIR "btn_close_over.bmp",
                NAVI_DIR "btn_close_press.bmp");
    place_button(self, self->mini_close, UI_ID_CLOSE, 0xbe5);

    self->maximize = new_button();
    set_bitmaps(self->maximize, NAVI_DIR "btn_maximize_normal.bmp", NAVI_DIR "btn_maximize_over.bmp",
                NAVI_DIR "btn_maximize_press.bmp");
    place_button(self, self->maximize, NAVI_ID_MAXIMIZE, 0x89b);

    self->minimize_box = new_check_box();
    ui_window_create(self->minimize_box, 0x6e, 0xc);
    ui_check_box_set_bitmaps(self->minimize_box, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp", "Minimize");
    ui_check_box_set_check(self->minimize_box, 0);
    ui_check_box_set_tool_tip(self->minimize_box, msg_str(0x89c));
    ui_window_add_child(wnd, self->minimize_box);

    self->road_icon = new_button();
    ui_navigation_v4_wnd_set_road_icon(self, g_navi.roadIcon);
    place_button(self, self->road_icon, NAVI_ID_ROAD_ICON, 0xc8b);

    self->route_list = new_button();
    set_bitmaps(self->route_list, NAVI_DIR "btn_roadlistwnd_normal.bmp", NAVI_DIR "btn_roadlistwnd_over.bmp",
                NAVI_DIR "btn_roadlistwnd_press.bmp");
    place_button(self, self->route_list, NAVI_ID_ROUTE_LIST, 0xc85);

    list = new_list();
    self->results = list;
    ui_window_create(&list->list.base, 0x8a, 0xf4);
    self->results->list.r = 0xff;
    self->results->list.g = 0xff;
    self->results->list.b = 0xff;
    self->results->list.base.id = NAVI_ID_RESULTS;
    ui_window_add_child(wnd, &self->results->list.base);

    list = new_list();
    self->entries = list;
    ui_window_create(&list->list.base, 0x84, 0x50);
    self->entries->list.r = 0xf0;
    self->entries->list.g = 0xf4;
    self->entries->list.b = 0xfd;
    self->entries->list.base.id = NAVI_ID_ENTRIES;
    ui_window_add_child(wnd, &self->entries->list.base);

    self->service = new_check_box();
    ui_window_create(self->service, 0x6e, 0xc);
    ui_check_box_set_bitmaps(self->service, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp", "service");
    self->service->id = NAVI_ID_SERVICE;
    ui_check_box_set_check(self->service, g_navi.routeUseKafra);
    ui_check_box_set_tool_tip(self->service, msg_str(0xc8c));
    ui_window_add_child(wnd, self->service);

    self->share = new_button();
    set_bitmaps(self->share, UI_SKIN "private_airplane\\btn_share_normal.bmp",
                UI_SKIN "private_airplane\\btn_share_over.bmp", UI_SKIN "private_airplane\\btn_share_press.bmp");
    place_button(self, self->share, NAVI_ID_SHARE, 0xc86);

    self->find = new_button();
    set_bitmaps(self->find, UI_SKIN "private_airplane\\btn_find_normal.bmp",
                UI_SKIN "private_airplane\\btn_find_over.bmp", UI_SKIN "private_airplane\\btn_find_press.bmp");
    place_button(self, self->find, NAVI_ID_FIND, 0x895);

    self->airship = new_button();
    set_bitmaps(self->airship, UI_SKIN "private_airplane\\btn_move_normal.bmp",
                UI_SKIN "private_airplane\\btn_move_over.bmp", UI_SKIN "private_airplane\\btn_move_press.bmp");
    place_button(self, self->airship, NAVI_ID_AIRSHIP, 0);

    text = mem_alloc(sizeof(struct UIStaticText));
    text = text ? ui_static_text_construct(text) : NULL;
    self->count_text = text;
    ui_window_create(text, 0x64, 0x14);
    ui_window_add_child(wnd, self->count_text);
    ui_static_text_set_text(self->count_text, msg_str(0xc87), 0);
    ui_static_text_set_text_colour(self->count_text, 0, 0, 0);

    self->close_button = ui_window_add_close_button(wnd, msg_str(0xbe5), 0);
    self->mini_button = ui_window_add_mini_button(wnd, msg_str(0x89c), 0);

    self->help = new_button();
    set_bitmaps(self->help, UI_SKIN "navigation_interface\\NaviHelp_off.bmp",
                UI_SKIN "navigation_interface\\NaviHelp_on.bmp", UI_SKIN "navigation_interface\\NaviHelp_on.bmp");
    place_button(self, self->help, NAVI_ID_HELP, 0x89d);

    self->navidata = new_button();
    set_bitmaps(self->navidata, NAVI_DIR "btn_Navidata_Open_normal.bmp", NAVI_DIR "btn_Navidata_Open_over.bmp",
                NAVI_DIR "btn_Navidata_Open_press.bmp");
    place_button(self, self->navidata, NAVI_ID_NAVIDATA, 0x899);

    search = mem_alloc(sizeof *search);
    if (search) {
        ui_edit_ctrl_construct(&search->edit.base);
        search->edit.base.ops = &ui_bitmap_edit_ctrl_ops.base;
        string_init(&search->bitmap);
    }
    self->search = search;
    ui_window_create(&search->edit.base, 0x71, 0x12);
    self->search->edit.max_chars = 0x1e;
    ui_bitmap_edit_ctrl_set_bitmap(self->search, NAVI_DIR "bg_search.bmp");
    self->search->edit.base.id = NAVI_ID_SEARCH_EDIT;
    ui_window_add_child(wnd, &self->search->edit.base);

    self->search_btn = new_button();
    ui_bitmap_button_set_bitmap(self->search_btn, NAVI_DIR "btn_searchbar_normal.bmp", UI_BMP_DISABLED, 0);
    set_bitmaps(self->search_btn, NAVI_DIR "btn_searchbar_normal.bmp", NAVI_DIR "btn_searchbar_over.bmp",
                NAVI_DIR "btn_searchbar_press.bmp");
    place_button(self, self->search_btn, NAVI_ID_SEARCH, 0x894);

    self->back = new_button();
    ui_bitmap_button_set_bitmap(self->back, NAVI_DIR "bt_back.bmp", UI_BMP_DISABLED, 0);
    set_bitmaps(self->back, NAVI_DIR "bt_back.bmp", NAVI_DIR "bt_back_on.bmp", NAVI_DIR "bt_back_on.bmp");
    place_button(self, self->back, NAVI_ID_BACK, 0xc88);

    kind_box = mem_alloc(sizeof *kind_box);
    kind_box = kind_box ? ui_combo_box_bmp_wnd_construct(kind_box) : NULL;
    self->kind_box = kind_box;
    kind_box->def.list_height = 0x43;
    ui_combo_box_bmp_wnd_create(self->kind_box, 0x3d, 0x12, NAVI_DIR "dropdown_on", NAVI_DIR "bg_combo.bmp");
    kind_box = self->kind_box;
    kind_box->def.border_r = 100;
    kind_box->def.border_g = 100;
    kind_box->def.border_b = 100;
    kind_box->def.back_r = 0xe8;
    kind_box->def.back_g = 0xe8;
    kind_box->def.back_b = 0xe8;
    ui_window_add_child(wnd, &self->kind_box->def.base);
    ui_combo_add(&self->kind_box->def.base, msg_str(0x89e));
    ui_combo_add(&self->kind_box->def.base, msg_str(0x89f));
    ui_combo_add(&self->kind_box->def.base, msg_str(0x8a0));
    ui_combo_add(&self->kind_box->def.base, msg_str(0x8a1));
    ui_combo_box_def_wnd_select(&self->kind_box->def.base, g_navi.searchFilter);

    ui_navigation_v4_wnd_relayout(self);
}

void ui_navigation_v4_wnd_draw_picture(struct UINavigationV4Wnd *self, int x, int y, int size)
{
    void *picture;
    float scale;

    if (!g_navi.picture)
        return;
    ui_window_fill_rect(&self->frame.base, x, y, size, size, 0);
    picture = g_navi.picture;
    scale = (float)size / (float)(double)UI_TEXTURE_W(picture);
    ui_canvas_draw_image(self->frame.base.canvas, x, y, size, size, picture, UI_TEXTURE_W(picture),
                         UI_TEXTURE_H(picture), scale, scale, 0xffffffff);
}

static INIT_ONCE s_body_guard, s_acc_guard[3];
static DWORD s_body_time, s_acc_time[3];
static int s_body_frame, s_acc_frame[3];

static void advance_clock(struct UINavigationV4Wnd *self, const struct SpriteAction *motions, DWORD now, DWORD *time)
{
    int count = (int)motions->motion_count;
    int per;
    if (!count || self->anim_period <= 0) {
        *time = now;
        return;
    }
    per = self->anim_period / count;
    if (!per) per = 1;

    if ((int)(now - *time) / per >= count)
        *time = now;
}

static void *res_of(struct String *path)
{
    return res_mgr_get_string(g_resMgr(), *path);
}

static void draw_accessory(struct UINavigationV4Wnd *self, const int *look, int item, int slot, struct ActRes **acts,
                           struct SprRes **sprs, const struct Motion *body, DWORD now, int x, int y, int w, int h, int ox, int oy,
                           float scale)
{
    struct String path;
    struct SpriteAction *motions;
    struct Motion *motion;
    int k = slot - 2;

    acts[slot] = res_of(session_get_res_name0(&path, look[0], look[1], look[3], item));
    sprs[slot] = res_of(session_get_res_name1(&path, look[0], look[1], look[3], item));
    if (!sprs[slot] || !acts[slot])
        return;
    motions = act_res_get_action(acts[slot], 0);
    if (!motions->motion_count || !animation_attachment(body, 0)
            || !animation_attachment(&motions->motions[0], 0))
        return;
    advance_clock(self, motions, now, &s_acc_time[k]);
    motion = motions->motions;
    s_acc_frame[k] = 0;
    dc_bitmap_draw_sprite(self->frame.base.canvas, x + (animation_attachment(body, 0)->x - animation_attachment(motion, 0)->x),
                       y + (animation_attachment(body, 0)->y - animation_attachment(motion, 0)->y), w, h, sprs[slot], motion, ox,
                       oy, scale, NULL);
}

void ui_navigation_v4_wnd_draw_guided_npc(struct UINavigationV4Wnd *self, int x, int y, int w, int h,
                                                struct BitmapRes *background)
{
    struct UIWindow *wnd = &self->frame.base;
    struct SearchResult result;
    struct NaviObject *obj;
    struct ActRes *acts[9] = {0};
    struct SprRes *sprs[9] = {0};
    struct SpriteAction *motions, *other;
    struct String path;
    void *palette;
    struct Motion *body, *motion;
    int look[9];
    int ox, oy;
    float scale;
    DWORD now;

    navi_manager_guided(&g_navi, &result);
    obj = result.obj;
    search_result_destroy(&result);
    if (!obj)
        return;
    navi_manager_guided(&g_navi, &result);
    obj = result.obj;
    search_result_destroy(&result);
    memcpy(look, obj->pickNpc, sizeof look);
    if (look[0] == 0) {
        ui_window_draw_actor_sprite(wnd, g_navi.sprRes, g_navi.actRes, x, y, w, h, 0x5dc, 1, background);
        return;
    }

    ox = 0;
    oy = 0;
    scale = 1.0f;
    acts[0] = res_of(session_get_body_spr_name_variant0(&path, look[0], look[1], 0, 0));
    sprs[0] = res_of(session_get_body_spr_name_variant1(&path, look[0], look[1], 0, 0));
    if (!sprs[0] || !acts[0])
        goto done;
    motions = act_res_get_action(acts[0], 0);
    if (!motions || !motions->motion_count)
        goto done;
    if (!ui_window_fit_motion(motions->motions, sprs[0], w, h, &ox, &oy, &scale))
        goto done;
    oy += 10;
    scale = 1.0f;
    if (background)
        dc_bitmap_update(wnd->canvas, x + ox - background->width / 2, y + oy - background->height / 2,
                           background->width, background->height, background->pixels, 1);

    motions = act_res_get_action(acts[0], 0);
    if (!motions)
        goto done;
    now = timeGetTime();
    if (once_begin(&s_body_guard)) {
        s_body_time = now;
        once_complete(&s_body_guard);
    }
    if (once_begin(&s_acc_guard[0])) {
        s_acc_time[0] = now;
        once_complete(&s_acc_guard[0]);
    }
    if (once_begin(&s_acc_guard[1])) {
        s_acc_time[1] = now;
        once_complete(&s_acc_guard[1]);
    }
    if (once_begin(&s_acc_guard[2])) {
        s_acc_time[2] = now;
        once_complete(&s_acc_guard[2]);
    }
    s_body_frame = 0;
    advance_clock(self, motions, now, &s_body_time);

    if (obj->pickNpc[8]) {
        acts[8] = res_of(session_get_robe_act_name(&path, look[0], look[1], look[8], 0));
        sprs[8] = res_of(session_get_robe_spr_name(&path, look[0], look[1], look[8], 0));
        if (sprs[8] && acts[8] && (other = act_res_get_action(acts[8], 0)) != NULL &&
            other->motion_count) {
            motion = &other->motions[(size_t)s_body_frame < other->motion_count ? s_body_frame : 0];
            if (motion)
                dc_bitmap_draw_sprite(wnd->canvas, x, y, w, h, sprs[8], motion, ox, oy, scale, NULL);
        }
    }

    if (!motions->motion_count)
        goto done;
    palette = NULL;
    if (look[2] > 0) {
        session_get_body_pal_name(&path, look[0], look[1], look[2], 0, 0);
        palette = palette_resource_colors(res_mgr_get(g_resMgr(), string_cstr(&path)), NULL);
        string_free(&path);
    }
    body = &motions->motions[(size_t)s_body_frame < motions->motion_count ? s_body_frame : 0];
    dc_bitmap_draw_sprite(wnd->canvas, x, y, w, h, sprs[0], body, ox, oy, 1.0f, palette);

    acts[1] = res_of(session_get_head_spr_name0(&path, look[0], look[1], &look[3]));
    sprs[1] = res_of(session_get_head_spr_name1(&path, look[0], look[1], &look[3]));
    if (sprs[1] && acts[1] && (other = act_res_get_action(acts[1], 0)) != NULL && other->motion_count) {
        int dx, dy;

        motion = &other->motions[(size_t)s_body_frame < other->motion_count ? s_body_frame : 0];
        palette = NULL;
        dx = dy = 0;
        if (animation_attachment(body, 0) && animation_attachment(motion, 0)) {
            dx = body->attachments[0].x - motion->attachments[0].x;
            dy = body->attachments[0].y - motion->attachments[0].y;
        }
        if (look[4] > 0) {
            session_get_head_pal_name(&path, look[3], look[0], look[1], look[4]);
            palette = palette_resource_colors(res_mgr_get(g_resMgr(), string_cstr(&path)), NULL);
            string_free(&path);
        }
        dc_bitmap_draw_sprite(wnd->canvas, x + dx, y + dy, w, h, sprs[1], motion, ox, oy, scale, palette);
    }

    if (obj->pickNpc[5])
        draw_accessory(self, look, look[5], 2, acts, sprs, body, now, x, y, w, h, ox, oy, scale);
    if (obj->pickNpc[6])
        draw_accessory(self, look, look[6], 3, acts, sprs, body, now, x, y, w, h, ox, oy, scale);
    if (obj->pickNpc[7])
        draw_accessory(self, look, look[7], 4, acts, sprs, body, now, x, y, w, h, ox, oy, scale);

done:
    return;
}

static void draw_route(struct UINavigationV4Wnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct NaviPoint *dot, *end, pos;
    int cx, cy;

    ui_window_fill_rect(wnd, 1, 0x2c, 0x10e, 0x10e, 0xcccccc);
    ui_navigation_v4_wnd_draw_picture(self, 3, 0x2e, 0x10a);
    ui_window_text_out(wnd, 0xa, 0x30, string_cstr(&g_navi.texts[0]), 0, 0, 0xc, 0xffffff, 1, 0);
    ui_window_text_out(wnd, 0xa, 0x44, string_cstr(&g_navi.texts[1]), 0, 0, 0xc, 0xffffff, 1, 0);
    dot = g_navi.pathDots.first;
    end = g_navi.pathDots.last;
    if (dot != end) {
        for (; dot != end; dot++)
            ui_window_fill_rect(wnd, (int)((float)dot->x * NAVI_DOT_SCALE + NAVI_PICTURE_X),
                              (int)((float)dot->y * NAVI_DOT_SCALE + NAVI_PICTURE_Y), 2, 2, 0xf0f00a);
        ui_window_draw_bitmap(wnd, (int)((float)end[-1].x * NAVI_DOT_SCALE + NAVI_PICTURE_X - 8.0f),
                            (int)((float)end[-1].y * NAVI_DOT_SCALE + NAVI_PICTURE_Y - 21.0f),
                            ui_skin_bitmap(NAVI_DIR "ico_destination.bmp"), 1);
    }
    cx = 0;
    cy = 0;
    if (navi_manager_player_cell(&g_navi, &cx, &cy)) {
        navi_manager_minimap_pos_by_name(&g_navi, &pos, g_session1.mapName, cx, cy, 266.0f);
        ui_window_draw_bitmap(wnd, pos.x - 6, pos.y + 0x1d, ui_skin_bitmap(NAVI_DIR "ico_location.bmp"), 1);
    }
    ui_window_text_out(wnd, 0xa, wnd->height - 0x2a, string_cstr(&g_navi.texts[2]), 0, 0, 0xc, 0xffffff, 1, 0);
}

static void draw_target(struct UINavigationV4Wnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct SearchResult result;
    struct NaviMap *map;
    int mx, my;

    ui_window_fill_rect(wnd, 1, 0x2c, 0x10e, 0x10e, 0xcccccc);
    ui_navigation_v4_wnd_draw_picture(self, 3, 0x2e, 0x10a);
    map = navi_manager_guided(&g_navi, &result)->obj;
    search_result_destroy(&result);
    if (map) {
        ui_window_text_out_outlined(wnd, 9, 0x32, navi_map_display_name(map), 0, 0xffffff, 0, 0, 0xb, 1, 0);
        ui_window_text_out_outlined(wnd, 9, 0x41, navi_map_name(map), 0, 0xffffff, 0, 0, 0xb, 1, 0);
    }
    if (self->hint_until > timeGetTime()) {
        struct String hint;
        struct TextArray lines = {0};
        void *tex;
        int y, n, i;

        tex = texture_cache_get(&g_textureCache, NAVI_DIR "back.bmp", 0, 0);
        if (tex)
            ui_canvas_draw_image(wnd->canvas, 0xc, 0x110, wnd->width - 0x18, 0x1e, tex, UI_TEXTURE_W(tex),
                                 UI_TEXTURE_H(tex), 1.0f, 1.0f, 0x66ffffff);
        tex = texture_cache_get(&g_textureCache, NAVI_DIR "icon_mouse.bmp", 0, 0);
        if (tex)
            ui_canvas_draw_image(wnd->canvas, 0x12, 0x115, UI_TEXTURE_W(tex), UI_TEXTURE_H(tex), tex,
                                 UI_TEXTURE_W(tex), UI_TEXTURE_H(tex), 1.0f, 1.0f, 0xffffffff);
        string_init(&hint);
        string_assign_cstr(&hint, msg_str(MSG_NAVI_TARGET_HINT));
        text_split_lines(string_cstr(&hint), &lines, 0x26);
        n = (int)lines.size;
        y = (41 - n) * 7;
        for (i = 0; i < (int)lines.size; i++, y += 0xe)
            ui_window_text_out_outlined(wnd, 0x2a, y, text_array_at(&lines, i).data, 0, 0xffffff, 0,
                                     0, 0xb, 1, 0);
        text_array_clear(&lines);
        string_free(&hint);
    }
    mx = g_mouse.x - wnd->x;
    if (mx > 3 && mx < 0x10d)
        ui_window_fill_rect(wnd, mx, 0x2e, 2, 0x10a, 0xffaa2200);
    my = g_mouse.y - wnd->y;
    if (my > 0x2e && my < 0x138)
        ui_window_fill_rect(wnd, 3, my, 0x10a, 2, 0xffaa2200);
}

static void draw_search(struct UINavigationV4Wnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct SearchResult result;
    struct NaviUiRecord *rec;
    int x, y, size, style, n, i;

    ui_window_fill_rect(wnd, 0x8a, 0x2c, 1, 0x10e, 0xcccccc);
    ui_window_fill_rect(wnd, 0x8b, 0xf2, 0x84, 1, 0xcccccc);
    ui_window_draw_bitmap(wnd, 5, 0x2e, ui_skin_bitmap(NAVI_DIR "bg_dot.bmp"), 0);
    x = 0x9b;
    size = 0x64;
    y = 0x3c;
    navi_manager_guided(&g_navi, &result);
    if (result.kind == 2) {
        x = 0xe3;
        size = 0x28;
        y = 0x78;
        ui_navigation_v4_wnd_draw_guided_npc(self, 0x93, 0x2d, 0x74, 0x74, ui_skin_bitmap(UI_SKIN "Anibackground.bmp"));
    } else if (result.kind == 3) {
        x = 0xe3;
        size = 0x28;
        y = 0x78;
        ui_window_draw_actor_sprite(wnd, g_navi.sprRes, g_navi.actRes, 0x93, 0x2d, 0x74, 0x74, 0x5dc, 1,
                                 ui_skin_bitmap(UI_SKIN "Anibackground.bmp"));
    }
    ui_navigation_v4_wnd_draw_picture(self, x, y, size);
    style = g_navi.pictureKind;
    n = (int)VEC_COUNT(&g_navi.infoLines, struct NaviUiRecord);
    for (i = 0; i < n; i++) {
        rec = (struct NaviUiRecord *)g_navi.infoLines.first + i;
        if (rec->style != style)
            continue;
        x = rec->x;
        y = rec->y;
        ui_window_text_out(wnd, x + 1, y + 1, string_cstr(&rec->text), 0, 0, 0xc, rec->colour, 0, 0);
        rec = (struct NaviUiRecord *)g_navi.infoLines.first + i;
        ui_window_text_out(wnd, x, y, string_cstr(&rec->text), 0, 0, 0xc, rec->colour2, 0, 0);
    }
    search_result_destroy(&result);
}

void ui_navigation_v4_wnd_on_draw(struct UIWindow *window)
{
    struct UINavigationV4Wnd *self = (struct UINavigationV4Wnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int top;

    if (g_navi.windowCollapsed == 1) {
        ui_window_fill_round_rect(wnd, 0, 0, NAVI_MINI_WIDTH, NAVI_MINI_HEIGHT, 0xffffff);
        ui_window_fill_round_rect(wnd, 1, 0x11, 0x84, 0x2b, 0);
        ui_window_text_out(wnd, 3, 3, msg_str(MSG_NAVI_TITLE), 0, 0, 0xc, 0, 0, 0);
        if (g_navi.texts[1].size != 0) {
            ui_window_draw_bitmap(wnd, 2, 0x12, ui_skin_bitmap(NAVI_DIR "ico_destination.bmp"), 1);
            ui_window_text_out(wnd, 5, 0x15, string_cstr(&g_navi.texts[3]), 0, 0, 0xc, 0xffffff, 1, 0);
            ui_window_text_out(wnd, 5, 0x28, string_cstr(&g_navi.texts[4]), 0, 0, 0xc, 0xffffff, 1, 0);
        }
        return;
    }
    top = ui_window_draw_title_bar(wnd, 1, msg_str(MSG_NAVI_TITLE), 0);
    ui_window_draw_body(wnd, top, 0, "");
    ui_window_draw_button_bar(wnd, 0, 0, 1);
    ui_window_fill_rect(wnd, 0, 0x2b, NAVI_WIDTH, 1, 0xcccccc);
    ui_window_draw_bitmap(wnd, 0, wnd->height - 0x18, ui_skin_bitmap(NAVI_DIR "bg_down.bmp"), 0);
    if (g_navi.guideState == 2 && g_navi.windowPage == NAVI_VIEW_ROUTE)
        draw_route(self);
    else if (g_navi.windowPage == NAVI_VIEW_TARGET)
        draw_target(self);
    else
        draw_search(self);
}

void ui_navigation_v4_wnd_store_info(struct UIWindow *window)
{
    struct UINavigationV4Wnd *self = (struct UINavigationV4Wnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct NavigationWndInfo *info = self->info;
    int width, limit, x, y;

    if (!info)
        return;
    info->rect.width = wnd->width;
    self->info->rect.height = wnd->height;
    width = 0x110;
    info = self->info;
    if (!info->is_screen)
        width = 0xe4;
    limit = g_renderer->width - width;
    x = wnd->x;
    if (limit < x) {
        wnd->x = limit;
        x = limit;
    }
    if (x < 0) {
        wnd->x = 0;
        x = 0;
    }
    info->rect.x = x;
    y = wnd->y;
    limit = g_renderer->height - NAVI_HEIGHT;
    if (limit < y) {
        wnd->y = limit;
        y = limit;
    }
    if (y < 0) {
        wnd->y = 0;
        y = 0;
    }
    self->info->rect.y = y;
}

void ui_navigation_v4_wnd_on_process(struct UIWindow *window)
{
    struct UINavigationV4Wnd *self = (struct UINavigationV4Wnd *)window;
    if (g_navi.windowPage == NAVI_VIEW_TARGET)
        ui_window_invalidate_base(&self->frame.base);
}

void ui_navigation_v4_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UINavigationV4Wnd *self = (struct UINavigationV4Wnd *)window;
    struct SearchResult result;
    struct NaviMap *map;
    struct String name;
    int px, py, width, height, size, diff, offx, offy, cx, cy;
    float scale;
    double half;

    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
    if (g_navi.windowCollapsed) {
        self->frame.base.alpha_target = 0xaa;
        self->frame.base.alpha_time = timeGetTime();
        return;
    }
    py = y - 0x2e;
    if ((unsigned)py > 0x10a)
        return;
    px = x - 3;
    if ((unsigned)px > 0x10a || g_navi.windowPage != NAVI_VIEW_TARGET)
        return;
    navi_manager_guided(&g_navi, &result);
    map = result.obj;
    width = navi_map_width(map);
    height = navi_map_height(map);
    size = width > height ? width : height;
    diff = width - height;
    scale = (float)size / 265.0f;
    half = (double)diff;
    offx = (int)fabs(diff > 0 ? 0.0 : half * 0.5);
    offy = (int)fabs(diff > 0 ? half * 0.5 : 0.0);
    cx = (int)((float)px * scale - (float)offx);
    cy = (int)((float)size - (float)py * scale - (float)offy);
    if (cx >= 0 && cy >= 0 && width >= cx && height >= cy) {
        string_init(&name);
        string_assign_cstr(&name, navi_map_name(map));
        ui_navigation_v4_wnd_put_link(self, name, cx, cy);
    }
    search_result_destroy(&result);
}

void ui_navigation_v4_wnd_fill_results(struct UINavigationV4Wnd *self)
{
    struct SearchResult result;
    struct String name;
    struct Vec *group;
    char line[0x34], count[0x34];
    unsigned i;
    int members;

    ui_list_box_navi_search_clear_items(&self->results->list);
    ui_window_invalidate_base(&self->results->list.base);
    for (i = 0; i < VEC_COUNT(&g_navi.resultGroups, struct Vec); i++) {
        group = (struct Vec *)g_navi.resultGroups.first + i;
        navi_manager_result_at(&g_navi, &result, *(int *)group->first);
        group = (struct Vec *)g_navi.resultGroups.first + i;
        members = (int)VEC_COUNT(group, int);
        ui_navigation_v4_wnd_result_name(self, &name, result.kind, result.obj);
        if (result.kind == 0 && result.name.size != 0)
            string_assign(&name, string_cstr(&result.name), result.name.size);
        memset(line, 0, 0x32);
        sprintf(line, "[%d]%s", members, string_cstr(&name));
        navi_list_add(self->results, result.kind, line);
        string_free(&name);
        search_result_destroy(&result);
    }
    memset(count, 0, 0x32);
    sprintf(count, msg_str(MSG_NAVI_FOUND), (int)VEC_COUNT(&g_navi.resultGroups, struct Vec));
    ui_static_text_set_text(self->count_text, count, 0);
    ui_window_invalidate_base(&self->frame.base);
}

void ui_navigation_v4_wnd_fill_entries(struct UINavigationV4Wnd *self, int group)
{
    struct SearchResult result;
    struct String text, made;
    struct Vec *members;
    int count, i;

    ui_window_invalidate_base(&self->entries->list.base);
    ui_list_box_navi_search_clear_items(&self->entries->list);
    if (g_navi.resultGroups.first == g_navi.resultGroups.last)
        return;
    g_navi.selectedGroup = group;
    if (group == -1)
        return;
    members = (struct Vec *)g_navi.resultGroups.first + group;
    count = (int)VEC_COUNT(members, int);
    string_init(&text);
    for (i = 0; i < count; i++) {
        members = (struct Vec *)g_navi.resultGroups.first + group;
        navi_manager_result_at(&g_navi, &result, ((int *)members->first)[i]);
        if (result.x == -1000 && result.y == -1000) {
            ui_navigation_v4_wnd_mob_text(self, &made, result.obj);
            string_free(&text);
            string_move(&text, &made);
        } else {
            ui_navigation_v4_wnd_member_text(self, &made, result.kind, result.obj);
            string_free(&text);
            string_move(&text, &made);
            if (result.kind == 0) {
                char cell[0x20];

                memset(cell, 0, 0x1e);
                sprintf(cell, " [%d,%d]", result.x, result.y);
                string_append_cstr(&text, cell);
            }
        }
        navi_list_add(self->entries, result.kind, string_cstr(&text));
        search_result_destroy(&result);
    }
    ui_window_invalidate_base(&self->entries->list.base);
    string_free(&text);
}

BYTE ui_navigation_v4_wnd_minimize_checked(struct UINavigationV4Wnd *self)
{
    return self->minimize_box->state != 0;
}
