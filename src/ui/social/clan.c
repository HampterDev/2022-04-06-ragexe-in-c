#include "render/rp_pool.h"
#include "session/map_names.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/social/social.h"
#include "contents/clan_info.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define CLAN_WND_ID 0xed

#define MSG_CLAN_CAPTION 0x933
#define MSG_CLAN_LABEL_NAME 0x935
#define MSG_CLAN_LABEL_LEVEL 0x934
#define MSG_CLAN_LABEL_LEADER 0x93a
#define MSG_CLAN_LABEL_ONLINE 0x93b
#define MSG_CLAN_LABEL_TERRITORY 0x93c
#define MSG_CLAN_EMBLEM_TITLE 0x936
#define MSG_CLAN_KIND0_TITLE 0x937
#define MSG_CLAN_KIND1_TITLE 0x938

#define UI_MSG_INVALIDATE_CHILDREN 0x3c

#define CLAN_WND_WIDTH 0x190
#define CLAN_WND_HEIGHT 0x13d

#define CLAN_ILLUST_DEFAULT UI_SKIN "illust\\펫_포포링.bmp"

struct UIClanWnd {
    struct UIFrameWnd frame;
    struct WindowInfo *info;
    struct UIWindow *caption;
};
SIZE_ASSERT(UIClanWnd, 0x9c);
void ui_clan_wnd_destroy(struct UIWindow *window);
void ui_clan_wnd_store_info(struct UIWindow *self);
void ui_clan_wnd_on_draw(struct UIWindow *self);
int ui_clan_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_clan_wnd_ops = {
    .destroy = ui_clan_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_clan_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_abstract_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_clan_wnd_on_draw,
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
    .send_msg = ui_clan_wnd_send_msg,
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

static void ui_clan_wnd_construct(struct UIClanWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_clan_wnd_ops;
    self->info = NULL;
    self->caption = NULL;
}

void ui_clan_wnd_destruct(struct UIClanWnd *self)
{
    self->frame.base.ops = &ui_clan_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_clan_wnd_destroy(struct UIWindow *window)
{
    struct UIClanWnd *self = (struct UIClanWnd *)window;
    ui_clan_wnd_destruct(self);
    mem_free(self);
}

static DWORD skin_rgb(int x, int y)
{
    int r, g, b;

    ui_skin_color_chip(&g_skinMgr, x, y, &r, &g, &b);
    return RGB(r, g, b);
}

void ui_clan_wnd_draw_tab_band(struct UIClanWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;

    ui_window_fill_rect(wnd, 0, 0x11, wnd->width, 0x18, skin_rgb(0xa, 2));
    ui_window_fill_rect(wnd, 1, 0x12, 0x40, 0x17, skin_rgb(2, 2));
}

void ui_clan_wnd_on_draw(struct UIWindow *window)
{
    struct UIClanWnd *self = (struct UIClanWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_clan_wnd_draw_tab_band(self);
    ui_window_draw_title_bar(wnd, 0, "", 0);
    ui_window_fill_rect(wnd, 0, 0x29, wnd->width, wnd->height - 0x29, skin_rgb(0xe, 6));
    ui_window_fill_rect(wnd, 1, 0x29, wnd->width - 2, wnd->height - 0x29, skin_rgb(2, 2));
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

void ui_clan_wnd_store_info(struct UIWindow *window)
{
    struct UIClanWnd *self = (struct UIClanWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
}

static int clan_info_usable(const struct WindowInfo *info)
{
    if (info->width != CLAN_WND_WIDTH || info->height != CLAN_WND_HEIGHT)
        return 0;
    if (info->x < 0 || info->x > g_renderer->width - CLAN_WND_WIDTH)
        return 0;
    if (info->y < 0 || info->y > g_renderer->height - CLAN_WND_HEIGHT)
        return 0;
    return 1;
}

static void clan_frame_reset(struct WindowInfo *info)
{
    info->x = 0;
    info->y = 0x22;
    info->width = CLAN_WND_WIDTH;
    info->height = CLAN_WND_HEIGHT;
}

static void clan_take_info(struct UIClanWnd *self, struct WindowInfo *info)
{
    self->info = info;
    if (!clan_info_usable(info))
        clan_frame_reset(info);
    ui_frame_wnd_move(&self->frame.base, info->x, info->y);
}

int ui_clan_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                 int val3, int val4)
{
    struct UIClanWnd *self = (struct UIClanWnd *)window;
    if (msg == UI_MSG_SET_INFO) {
        clan_take_info(self, (struct WindowInfo *)val1);
        return 0;
    }
    if (msg == UI_MSG_INVALIDATE_CHILDREN) {
        ui_window_invalidate_children(&self->frame.base);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIClanInfoManageWnd {
    struct UIClanWnd clan;
    int selected_kind0;
    int selected_kind1;
    void *emblem;
};
SIZE_ASSERT(UIClanInfoManageWnd, 0xa8);
void ui_clan_info_manage_wnd_destroy(struct UIWindow *window);
void ui_clan_info_manage_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_clan_info_manage_wnd_on_draw(struct UIWindow *self);
void ui_clan_info_manage_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_clan_info_manage_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_clan_info_manage_wnd_ops = {
    .destroy = ui_clan_info_manage_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_clan_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_clan_info_manage_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_clan_info_manage_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_clan_info_manage_wnd_on_l_btn_down,
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
    .send_msg = ui_clan_info_manage_wnd_send_msg,
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

struct UIClanInfoManageWnd *ui_clan_info_manage_wnd_construct(struct UIClanInfoManageWnd *self)
{
    ui_clan_wnd_construct(&self->clan);
    self->clan.frame.base.ops = &ui_clan_info_manage_wnd_ops;
    self->emblem = NULL;
    return self;
}

struct UIWindow *ui_clan_info_manage_wnd_new(void)
{
    return (struct UIWindow *)ui_clan_info_manage_wnd_construct(mem_alloc(sizeof(struct UIClanInfoManageWnd)));
}

static void ui_clan_info_manage_wnd_destruct(struct UIClanInfoManageWnd *self)
{
    self->clan.frame.base.ops = &ui_clan_info_manage_wnd_ops;
    ui_clan_wnd_destruct(&self->clan);
}

void ui_clan_info_manage_wnd_destroy(struct UIWindow *window)
{
    struct UIClanInfoManageWnd *self = (struct UIClanInfoManageWnd *)window;
    ui_clan_info_manage_wnd_destruct(self);
    mem_free(self);
}

static void clan_load_emblem(struct UIClanInfoManageWnd *self)
{
    struct ClanInfo *clan = g_clanInfo;
    char path[0x100];

    if (clan->in_clan != 1)
        return;
    memset(path, 0, sizeof path);
    sprintf(path, UI_SKIN "clan_system\\clan_emblem%02d.bmp", clan->emblem);
    self->emblem = res_mgr_get(g_resMgr(), ui_skin_path(path));
    if (!self->emblem)
        self->emblem = res_mgr_get(g_resMgr(), ui_skin_path(UI_SKIN "clan_system\\clan_emblem01.bmp"));
}

void ui_clan_wnd_make_caption(struct UIClanWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;

    if (!self->caption) {
        const char *text = msg_str(MSG_CLAN_CAPTION);
        struct UIWindow *caption = mem_alloc(sizeof(struct UIStaticText));

        if (caption)
            caption = ui_static_text_construct(caption);
        self->caption = caption;
        ui_window_create(caption, 0x3e, 0xf);
        ui_static_text_set_text_ellipsis(self->caption, text);
        ui_window_move(self->caption, 3, 0x16);
        ui_window_add_child(wnd, self->caption);
        ui_window_add_close_button(wnd, "Ctrl+G", 0);
    }
}

void ui_clan_info_manage_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIClanInfoManageWnd *self = (struct UIClanInfoManageWnd *)window;
    ui_clan_wnd_make_caption(&self->clan);
    self->selected_kind0 = -1;
    self->selected_kind1 = -1;
    if (g_clanInfo->in_clan)
        g_clanInfo->level = 1;
    clan_load_emblem(self);
}

static void text_at(struct UIWindow *wnd, int x, int y, const char *text)
{
    ui_window_text_out(wnd, x, y, text, 0, 0, 0xc, 0, 0, 0);
}

static void draw_others(struct UIClanInfoManageWnd *self, int kind, int top, int selected)
{
    struct UIWindow *wnd = &self->clan.frame.base;
    struct ListNode *node;
    int row = 0, y = top + 2;

    ui_window_fill_rect(wnd, 0xc8, top, 0xa8, 0x30, skin_rgb(0xe, 6));
    for (node = g_clanInfo->relations.head->next; node != g_clanInfo->relations.head && y < top + 0x32;
         node = node->next) {
        const struct ClanRelation *relation = LIST_VALUE(node, struct ClanRelation);

        if (relation->relation != kind)
            continue;
        if (row == selected)
            ui_window_fill_rect(wnd, 0xcc, y, 0x50, 0x10, skin_rgb(6, 2));
        text_at(wnd, 0xcc, y, string_cstr(&relation->name));
        row++;
        y += 0x10;
    }
}

void ui_clan_info_manage_wnd_on_draw(struct UIWindow *window)
{
    struct UIClanInfoManageWnd *self = (struct UIClanInfoManageWnd *)window;
    struct UIWindow *wnd = &self->clan.frame.base;
    struct ClanInfo *clan;
    struct String map;
    char line[0x400];
    void *illust;

    ui_clan_wnd_on_draw((struct UIWindow *)(&self->clan));
    memset(line, 0, sizeof line);

    clan = g_clanInfo;
    sprintf(line, "%s : %s", msg_str(MSG_CLAN_LABEL_NAME), string_cstr(&clan->name));
    text_at(wnd, 8, 0x31, line);
    sprintf(line, "%s : %d", msg_str(MSG_CLAN_LABEL_LEVEL), g_clanInfo->level);
    text_at(wnd, 8, 0x41, line);
    clan = g_clanInfo;
    sprintf(line, "%s : %s", msg_str(MSG_CLAN_LABEL_LEADER), string_cstr(&clan->master));
    text_at(wnd, 8, 0x51, line);
    sprintf(line, "%s : ", msg_str(MSG_CLAN_LABEL_ONLINE));
    text_at(wnd, 8, 0x61, line);
    sprintf(line, "%s", UI_SKIN "basic_interface\\grp_online.bmp");
    ui_window_draw_bitmap(wnd, 0x80, 0x5f, res_mgr_get(g_resMgr(), ui_skin_path(line)), 1);
    sprintf(line, "   %d", g_clanInfo->online_count);
    text_at(wnd, 0x8a, 0x61, line);

    string_copy(&map, &g_clanInfo->map_name);
    if (map.size > 4) {
        struct String rsw;

        string_init(&rsw);
        string_assign_cstr(&rsw, ".rsw");
        string_replace(&map, string_find(&map, ".gat", 0), rsw.size, ".rsw", 4);
        string_free(&rsw);
    }
    {
        const char *name = session_map_display_name(string_cstr(&map));

        string_assign_cstr(&map, name);
    }
    sprintf(line, "%s : %s", msg_str(MSG_CLAN_LABEL_TERRITORY), string_cstr(&map));
    text_at(wnd, 8, 0x71, line);
    string_free(&map);

    if (g_clanInfo->emblem) {
        char path[0x100];

        memset(path, 0, sizeof path);
        sprintf(path, UI_SKIN "clan_system\\clan_illust%02d.bmp", g_clanInfo->emblem);
        illust = res_mgr_get(g_resMgr(), ui_skin_path(path));
        if (!illust)
            illust = res_mgr_get(g_resMgr(), ui_skin_path(CLAN_ILLUST_DEFAULT));
        ui_window_draw_bitmap(wnd, 8, 0x91, illust, 1);
    } else {
        illust = res_mgr_get(g_resMgr(), ui_skin_path(CLAN_ILLUST_DEFAULT));
        ui_window_draw_bitmap(wnd, 0x32, 0x91, illust, 1);
    }

    sprintf(line, "%s", msg_str(MSG_CLAN_EMBLEM_TITLE));
    text_at(wnd, 0xc8, 0x41, line);
    ui_window_fill_rect(wnd, 0x117, 0x40, 0x1a, 0x1a, skin_rgb(6, 2));
    if (g_clanInfo->emblem)
        ui_window_draw_bitmap(wnd, 0x118, 0x41, self->emblem, 1);

    sprintf(line, "%s", msg_str(MSG_CLAN_KIND0_TITLE));
    text_at(wnd, 0xc8, 0x8b, line);
    sprintf(line, "%s", msg_str(MSG_CLAN_KIND1_TITLE));
    text_at(wnd, 0xc8, 0xd5, line);
    draw_others(self, 0, 0x9b, self->selected_kind0);
    draw_others(self, 1, 0xe5, self->selected_kind1);
}

void ui_clan_info_manage_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIClanInfoManageWnd *self = (struct UIClanInfoManageWnd *)window;
    ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->clan.frame), x, y);
}

int ui_clan_info_manage_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    struct UIClanInfoManageWnd *self = (struct UIClanInfoManageWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CLOSE)
            ui_window_mgr_post_quit1(&g_windowMgr, CLAN_WND_ID);
        return 0;
    case UI_MSG_INVALIDATE_CHILDREN:
        clan_load_emblem(self);
        ui_window_invalidate_children(&self->clan.frame.base);
        return 0;
    case UI_MSG_SET_INFO:
        clan_take_info(&self->clan, (struct WindowInfo *)val1);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->clan.frame), sender, msg, val1, val2, val3, val4);
}
