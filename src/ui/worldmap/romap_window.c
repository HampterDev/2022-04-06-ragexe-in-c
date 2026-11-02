#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "session/character_name.h"
#include "navi/navi_manager.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "ui/window_access.h"
#include "game/context.h"
#include <string.h>
#include "ui/worldmap/romap.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"
#include "session/session_items.h"
#include "ui/quests/navigation.h"

struct Vec;
struct UIRoMapWnd;
struct UIWindow;
void ui_ro_map_wnd_destroy(struct UIWindow *window);
void ui_ro_map_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_ro_map_wnd_on_draw(struct UIWindow *self);
void ui_ro_map_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_ro_map_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_ro_map_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_ro_map_wnd_on_r_btn_up(struct UIWindow *self, int x, int y);
intptr_t ui_ro_map_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_ro_map_wnd_ops = {
    .destroy = ui_ro_map_wnd_destroy,
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
    .on_create = ui_ro_map_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_ro_map_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_ro_map_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_ro_map_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_ro_map_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_ro_map_wnd_on_r_btn_up,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_ro_map_wnd_send_msg,
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

#define MSG_ROMAP_DUNGEONS 0x988
#define MSG_ROMAP_SEARCH_TIP 0x894
#define MSG_ROMAP_NAVI_TIP 0xc8d
#define MSG_ROMAP_EDIT_HINT 0x8a2

void route_stops_clear(struct Vec *v)
{
    struct RouteStop *stop;

    for (stop = v->first; stop != (struct RouteStop *)v->last; stop++)
        string_free(&stop->name);
    vec_clear(v);
}

void route_stops_free(struct Vec *v)
{
    route_stops_clear(v);
    vec_free(v);
}

void flying_ship_route_info_destruct(struct FlyingShipRouteInfo *self)
{
    string_free(&self->bitmap);
    route_stops_free(&self->stops);
}

void kafra_move_map_service_info_destruct(struct KafraMoveMapServiceInfo *self)
{
    string_free(&self->map);
    string_free(&self->origin.name);
    route_stops_free(&self->stops);
}

static void entry_init(struct RoMapEntry *entry)
{
    memset(entry, 0, sizeof *entry);
    string_init(&entry->name);
    list_init(&entry->members);
}

static void destroy_string(void *value) { string_free(value); }

static void entry_destruct(struct RoMapEntry *entry)
{
    list_free(&entry->members, destroy_string);
    string_free(&entry->name);
}

static void destroy_entry(void *value)
{
    entry_destruct(value);
}

static void members_push(struct List *members, const struct String *name)
{
    struct String copy;

    string_copy(&copy, name);
    list_push_back(members, &copy, sizeof copy);
}

static void entry_copy(struct RoMapEntry *dst, const struct RoMapEntry *src)
{
    const struct ListNode *node;

    *dst = *src;
    string_copy(&dst->name, &src->name);
    list_init(&dst->members);
    for (node = src->members.head->next; node != src->members.head; node = node->next)
        members_push(&dst->members, LIST_VALUE(node, const struct String));
}

void ui_ro_map_wnd_destruct(struct UIRoMapWnd *self)
{
    self->frame.base.ops = &ui_ro_map_wnd_ops;
    mem_free(self->minimap_box);
    if (self->spot) {
        entry_destruct(self->spot);
        mem_free(self->spot);
    }
    self->origin_x = 0;
    self->origin_y = 0;
    self->minimap = NULL;
    self->minimap_state = 0;
    string_vec_free(&self->results);
    flying_ship_route_info_destruct(&self->ship[1]);
    flying_ship_route_info_destruct(&self->ship[0]);
    kafra_move_map_service_info_destruct(&self->kafra);
    list_free(&self->entries, destroy_entry);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_ro_map_wnd_destroy(struct UIWindow *window)
{
    struct UIRoMapWnd *self = (struct UIRoMapWnd *)window;
    ui_ro_map_wnd_destruct(self);
    mem_free(self);
}

int ui_ro_map_wnd_centre_x(struct UIWindow *self, struct String text, int font, int arg,
                                  int left, int right)
{
    int width = ui_window_text_width(self, string_cstr(&text), 0, 0, font, arg, 0);
    int x = abs((int)((float)(width - left - right) * 0.5f));

    string_free(&text);
    return x;
}

static int centre_x(struct UIWindow *wnd, const struct String *text, int font, int arg, int left, int right)
{
    struct String copy;

    string_copy(&copy, text);
    return ui_ro_map_wnd_centre_x(wnd, copy, font, arg, left, right);
}

static const struct WorldViewRect *place_label_rect(const struct WorldViewArea *place)
{
    if (!place->map_count)
        return &place->rect;
    return &place->maps[0].rect;
}

static void draw_field_names(struct UIRoMapWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    int font = g_worldView.dungeon_font_size[g_worldView.resolution];
    COLORREF colour = g_worldView.dungeon_font_colour[0];
    COLORREF shadow = g_worldView.dungeon_font_colour[1];
    const struct WorldViewWorld *world = world_view_current_world(&g_worldView);
    size_t i;
    struct String text;

    if (!world)
        return;
    string_init(&text);
    for (i = 0; i < world->area_count; ++i) {
        const struct WorldViewArea *place = &world->areas[i];
        const struct WorldViewRect *rect;
        int x, y;

        if (place->id >= WORLD_VIEW_FIELD_ID)
            continue;
        rect = place_label_rect(place);
        string_assign_cstr(&text, place->name);
        if (text.size == 0)
            continue;
        x = centre_x(wnd, &text, font, 0, rect->left, rect->right);
        y = rect->top + 2;
        ui_window_text_out(wnd, x + 1, y + 1, string_cstr(&text), 0, 0, font, shadow, 0, 0);
        ui_window_text_out(wnd, x, y, string_cstr(&text), 0, 0, font, colour, 0, 0);
    }
    string_free(&text);
}

static void draw_dungeons(struct UIRoMapWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    int fi = g_worldView.resolution;
    int font_a = g_worldView.dungeon_font_size[fi];
    int font_b = g_worldView.mob_font_size[fi];
    int font_c = g_worldView.mob_level_font_size[fi];
    const struct WorldViewWorld *world = world_view_current_world(&g_worldView);
    size_t i, j;
    struct String text;
    BYTE at_top = 0;

    if (!self->show_dungeons || !world)
        return;
    string_init(&text);
    for (i = 0; i < world->area_count; ++i) {
        const struct WorldViewArea *place = &world->areas[i];

        if (place->id < WORLD_VIEW_FIELD_ID) {
            const struct WorldViewRect *rect = place_label_rect(place);
            int x, y;

            if (!self->show_dungeons)
                continue;
            string_assign_cstr(&text, place->text);
            if (text.size == 0)
                continue;
            x = centre_x(wnd, &text, font_a, 0, rect->left, rect->right);
            y = font_a + 2 + rect->top;
            ui_window_text_out(wnd, x + 1, y + 1, string_cstr(&text), 0, 0, font_a, g_worldView.dungeon_font_colour[1], 0,
                             0);
            ui_window_text_out(wnd, x, y, string_cstr(&text), 0, 0, font_a, g_worldView.dungeon_font_colour[0], 0, 0);
        } else {
            for (j = 0; j < place->map_count; ++j) {
                const struct WorldViewMap *area = &place->maps[j];
                const struct WorldViewRect *r = &area->rect;
                int x, y, sub_y;

                string_assign_cstr(&text, area->text_a);
                if (text.size == 0)
                    continue;
                ui_window_draw_round_frame(wnd, r->left, r->top, r->right - r->left, r->bottom - r->top,
                                        g_worldView.box_line_colour);
                if (at_top) {
                    y = r->top + 3;
                    sub_y = y + font_b + 3;
                    at_top = 0;
                } else {
                    y = r->bottom - font_b - 3;
                    sub_y = y - font_c - 3;
                    at_top = 1;
                }
                x = centre_x(wnd, &text, font_b, 0, r->left, r->right);
                ui_window_text_out(wnd, x + 1, y + 1, string_cstr(&text), 0, 0, font_b, g_worldView.mob_colour[1],
                                 0, 0);
                ui_window_text_out(wnd, x - 1, y - 1, string_cstr(&text), 0, 0, font_b, g_worldView.mob_colour[1],
                                 0, 0);
                ui_window_text_out(wnd, x, y, string_cstr(&text), 0, 0, font_b, g_worldView.mob_colour[0], 0, 0);
                string_assign_cstr(&text, area->text_b);
                if (text.size == 0)
                    continue;
                x = centre_x(wnd, &text, font_c, 1, r->left, r->right);
                ui_window_text_out_outlined(wnd, x, sub_y, string_cstr(&text), 0, g_worldView.mob_level_colour[0],
                                         g_worldView.mob_level_colour[1], 0, font_c, 1, 0);
            }
        }
    }
    string_free(&text);
}

static void draw_members(struct UIRoMapWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct WorldViewRect rect;
    struct ListNode *node;
    struct RoMapBox *box;
    int line, x, y, bx, width = 0, height, count = 0;
    BYTE left_side = 0;

    if (self->members_box) {
        mem_free(self->members_box);
        self->members_box = NULL;
    }
    if (!self->hover || !self->minimap_box)
        return;
    line = ui_window_text_height(wnd, "", 0, 0, 0xc, 1, 0) + 3;
    rect = self->hover->rect;
    x = self->minimap_box->c;
    if (x < rect.right) {
        x = self->minimap_box->a - 10;
        left_side = 1;
    } else {
        x += 10;
    }
    y = self->minimap_box->b;
    for (node = self->hover->members.head->next; node != self->hover->members.head; node = node->next) {
        int w = ui_window_text_width(wnd, string_cstr(LIST_VALUE(node, struct String)), 0, 0, 0xc, 1, 0);

        if (width < w)
            width = w;
        count++;
    }
    width += 10;
    height = count * line + 7;
    bx = left_side ? x - width : x;
    if (bx + width > g_renderer->width) {
        bx = rect.left - width - 10;
    } else if (!left_side) {
        y += 0x11;
    }
    if (height + y > g_renderer->height)
        y = g_renderer->height - height + 10;

    {
        int tx = bx + 5, ty = y + 5;

        for (node = self->hover->members.head->next; node != self->hover->members.head; node = node->next) {
            const char *name = string_cstr(LIST_VALUE(node, struct String));
            COLORREF colour = strcmp(name, session_character_name(&g_session1)) == 0 ? 0x8080f0 : 0xffffff;

            ui_window_text_out_outlined(wnd, tx, ty, name, 0, colour, 0, 0, 0xc, 1, 0);
            ty += line;
        }
    }
    box = mem_alloc(sizeof *box);
    if (box) {
        box->a = bx;
        box->b = y;
        box->c = width;
        box->d = height;
        box->flag = 1;
    }
    self->members_box = box;
    ui_window_draw_round_frame(wnd, bx, y, width, height, 0xffffff);
}

static void draw_spot(struct UIRoMapWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct RoMapEntry *spot = self->spot;
    int x, y, top;
    struct String name, path;
    struct RoMapBox *box, *old;
    RECT clear;

    if (!spot)
        return;
    if (spot->kind < WORLD_VIEW_FIELD_ID)
        ui_window_draw_round_frame(wnd, spot->place_rect.left, spot->place_rect.top,
                                spot->place_rect.right - spot->place_rect.left,
                                spot->place_rect.bottom - spot->place_rect.top, g_worldView.dungeon_select_colour[1]);
    ui_window_draw_round_frame(wnd, spot->rect.left, spot->rect.top, spot->rect.right - spot->rect.left,
                            spot->rect.bottom - spot->rect.top, g_worldView.dungeon_select_colour[1]);

    x = spot->rect.right + 10;
    if (x + 0x82 > g_renderer->width)
        x = spot->rect.left - 0x8c;
    top = spot->rect.top;
    y = top + 0x8c > g_renderer->height ? g_renderer->height - 0x82 : top + 10;
    old = self->minimap_box;
    box = mem_alloc(sizeof *box);
    if (box) {
        box->c = x + 0x82;
        box->flag = 1;
        box->a = x;
        box->b = y;
        box->d = y + 0x82;
    }
    self->minimap_box = box;
    if (old)
        mem_free(old);
    clear.left = x;
    clear.top = y;
    clear.right = x + 0x82;
    clear.bottom = y + 0x82;
    ui_window_fill_canvas(wnd, UI_TRANSPARENT, &clear);

    string_copy(&name, &spot->name);
    if (name.size > 4)
        string_resize(&name, name.size - 4, 0);
    ui_window_text_out_outlined(wnd, x + 3, y + 0x73, string_cstr(&name), 0, 0xffffff, 0x5c5c5c, 0, 0xc, 1, 0);
    string_init(&path);
    string_assign(&path, UI_SKIN "map\\", 0x13);
    string_append(&path, string_cstr(&name), name.size);
    string_append_cstr(&path, ".bmp");
    self->minimap = texture_cache_get(&g_textureCache, string_cstr(&path), 0, 1);
    string_free(&path);
    {
        const char *shown = session_map_display_name(string_cstr(&self->spot->name));

        string_assign_cstr(&name, shown);
    }
    ui_window_text_out_outlined(wnd, x + 3, y + 3, string_cstr(&name), 0, 0xd2ff, 0, 0, 0xc, 1, 0);
    string_free(&name);
}

void ui_ro_map_wnd_on_draw(struct UIWindow *window)
{
    struct UIRoMapWnd *self = (struct UIRoMapWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct ListNode *node;
    struct String map_name;
    const char *current;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    draw_field_names(self);
    current = game_current_map_name();
    string_init(&map_name);
    string_assign_cstr(&map_name, current);
    for (node = self->entries.head->next; node != self->entries.head; node = node->next) {
        struct RoMapEntry *e = LIST_VALUE(node, struct RoMapEntry);

        if (g_worldView.current_world != e->region)
            continue;
        ui_window_draw_round_frame(wnd, e->rect.left, e->rect.top, e->rect.right - e->rect.left,
                                e->rect.bottom - e->rect.top,
                                string_equal(&map_name, &e->name) ? 0x23aee8 : 0x7ee2f7);
    }
    string_free(&map_name);
    draw_dungeons(self);
    draw_members(self);
    draw_spot(self);
    ui_window_fill_canvas(wnd, UI_TRANSPARENT, &self->kafra_rect);
    ui_window_draw_title_bar(wnd, 1, "", 0);
    ui_window_fill_rect(wnd, 0, 0x10, wnd->width, 2, 0xf7dfc6);
}

void ui_ro_map_wnd_on_l_btn_down(struct UIWindow *self, int x, int y)
{
}

static void track_spot(struct UIRoMapWnd *self, int x, int y)
{
    const struct WorldViewWorld *region;
    const struct WorldViewMap *found;
    int index = 0;

    if (self->spot) {
        entry_destruct(self->spot);
        mem_free(self->spot);
        self->spot = NULL;
    }
    region = world_view_current_world(&g_worldView);
    if (!region)
        return;
    found = world_view_map_at(region, &index, x, y);
    if (found) {
        const struct WorldViewArea *place = &region->areas[index];
        int kind = place->id;
        struct RoMapEntry *spot = mem_alloc(sizeof *spot);

        if (!spot)
            return;
        entry_init(spot);
        self->spot = spot;
        spot->region = g_worldView.current_world;
        self->spot->kind = kind;
        string_assign_cstr(&self->spot->name, found->name);
        self->spot->rect = found->rect;
        place = &region->areas[index];
        self->spot->place_rect = place->rect;
        ui_window_invalidate_base(&self->frame.base);
    }
}

void ui_ro_map_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIRoMapWnd *self = (struct UIRoMapWnd *)window;
    struct ListNode *node;

    if (&self->frame.base == ui_window_mgr_get_capture(&g_windowMgr))
        return;
    if (self->mouse_x == x && self->mouse_y == y)
        return;
    self->mouse_x = x;
    self->mouse_y = y;
    self->hover = NULL;
    for (node = self->entries.head->next; node != self->entries.head; node = node->next) {
        struct RoMapEntry *e = LIST_VALUE(node, struct RoMapEntry);

        if (g_worldView.current_world == e->region && e->rect.left < x && e->rect.right > x && e->rect.top < y
            && e->rect.bottom > y) {
            self->hover = e;
            break;
        }
    }
    ui_window_invalidate_base(&self->frame.base);
    track_spot(self, x, y);
}

static void navi_show_results(void)
{
    ui_navigation_v4_wnd_fill_entries((struct UINavigationV4Wnd *)g_windowMgr.navigation_v4_wnd, 0);
    ui_navigation_v4_wnd_pick_entry((struct UINavigationV4Wnd *)g_windowMgr.navigation_v4_wnd, 0);
}

void ui_ro_map_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIRoMapWnd *self = (struct UIRoMapWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String *s;
    const struct WorldViewArea *place;
    int index = 0;

    string_vec_clear(&self->results);
    place = world_view_field_at(world_view_current_world(&g_worldView), &index, x, y);
    if (place) {
        size_t i;

        self->place_index = index;
        for (i = 0; i < place->map_count; ++i) {
            const struct WorldViewMap *area = &place->maps[i];

            string_vec_push_text(&self->results, area->text_a, strlen(area->text_a));
        }
    }
    if (self->results.first != self->results.last) {
        struct UIWindow *combo;
        int widest = 0, height, cx, cy;

        if (g_windowMgr.combo_box_wnd)
            ui_close_window(UI_WND_COMBO_BOX);
        if (self->results.first == self->results.last)
            return;
        combo = ui_make_window(UI_WND_COMBO_BOX);
        for (s = self->results.first; s != (struct String *)self->results.last; s++) {
            int w = ui_window_text_width(wnd, string_cstr(s), 0, 0, 0xe, 0, 0);

            if (widest < w)
                widest = w;
            ui_window_send_msg(combo, NULL, 0x1f, (intptr_t)string_cstr(s), 0, 0, 0);
        }
        height = (int)((struct String *)self->results.last - (struct String *)self->results.first) * 0x10;
        if ((unsigned)height > 0xa0)
            height = 0xa0;
        ui_window_send_msg(combo, NULL, 0x28, 0, 0, 0, 0);
        ui_window_resize(combo, widest, height);
        ui_window_send_msg(combo, NULL, 0x53, 0x8c, 0, 0, 0);
        cy = wnd->height - (y + 1) - height < 0 ? wnd->height - height : y + 1;
        cx = wnd->width - (x + 2) - widest < 0 ? wnd->width - widest : x + 2;
        ui_window_move(combo, cx, cy);
    } else if (self->spot) {
        struct String text;

        string_copy(&text, &self->spot->name);
        if (text.size > 4)
            string_resize(&text, text.size - 4, 0);
        g_navi.windowCollapsed = 0;
        navi_open_search(&g_navi, string_cstr(&text), text.size, 0, 0);
        if (g_windowMgr.navigation_v4_wnd)
            navi_show_results();
        ui_ro_map_wnd_place_search(self);
        string_free(&text);
    }
}

void ui_ro_map_wnd_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIRoMapWnd *self = (struct UIRoMapWnd *)window;
    if (g_windowMgr.combo_box_wnd)
        ui_close_window(UI_WND_COMBO_BOX);
    self->place_index = 0;
}

void ui_ro_map_wnd_place_search(struct UIRoMapWnd *self)
{
    if (g_windowMgr.navigation_v4_wnd && !g_navi.windowCollapsed) {
        ui_window_move_base(&self->search->button.base, -100, -100);
        ui_window_move_base(&self->navi->button.base, -100, -100);
        ui_window_move(self->edit, -100, -100);
    } else {
        ui_window_move_base(&self->search->button.base, 0x14f, 0);
        ui_window_move_base(&self->navi->button.base, 0x177, 3);
        ui_window_move(self->edit, 0x83, 0);
        g_navi.windowHidden = 1;
    }
    ui_window_invalidate_base(&self->frame.base);
}

static BYTE entry_locate(struct RoMapEntry *entry)
{
    struct String name;
    unsigned i;
    BYTE found = 0;

    string_copy(&name, &entry->name);
    for (i = 0; i < g_worldView.world_count; i++) {
        const struct WorldViewMap *spot = world_view_find_map(&g_worldView.worlds[i], string_cstr(&name));

        if (spot) {
            entry->region = i;
            entry->rect = spot->rect;
            found = 1;
            break;
        }
    }
    string_free(&name);
    return found;
}

static void build_marks(struct UIRoMapWnd *self)
{
    int count = session_get_party_member_count(&g_session1);
    int i;

    if (count > 0) {
        for (i = 0; i < count; i++) {
            struct PartyMember member;

            session_party_member_at(&g_session1, &member, i);
            if (member.found && !member.state) {
                struct String name, map;
                struct ListNode *node;
                BYTE added = 0;

                string_copy(&name, &member.name);
                string_copy(&map, &member.map_name);
                for (node = self->entries.head->next; node != self->entries.head; node = node->next) {
                    struct RoMapEntry *e = LIST_VALUE(node, struct RoMapEntry);

                    if (string_equal(&e->name, &map)) {
                        members_push(&e->members, &name);
                        added = 1;
                        break;
                    }
                }
                string_free(&map);
                string_free(&name);
                if (!added) {
                    struct RoMapEntry entry;

                    entry_init(&entry);
                    string_assign(&entry.name, string_cstr(&member.map_name), member.map_name.size);
                    members_push(&entry.members, &member.name);
                    if (entry_locate(&entry)) {
                        struct RoMapEntry copy;

                        entry_copy(&copy, &entry);
                        list_push_back(&self->entries, &copy, sizeof copy);
                    }
                    entry_destruct(&entry);
                }
            }
            string_free(&member.map_name);
            string_free(&member.name);
        }
    } else {
        const char *current = game_current_map_name();
        const char *me;
        struct String map, name;
        struct RoMapEntry entry;

        if (!*current)
            return;
        string_init(&map);
        string_assign_cstr(&map, current);
        entry_init(&entry);
        string_assign(&entry.name, string_cstr(&map), map.size);
        me = session_character_name(&g_session1);
        string_init(&name);
        string_assign_cstr(&name, me);
        list_push_back(&entry.members, &name, sizeof name);
        if (entry_locate(&entry)) {
            struct RoMapEntry copy;

            entry_copy(&copy, &entry);
            list_push_back(&self->entries, &copy, sizeof copy);
        }
        entry_destruct(&entry);
        string_free(&map);
    }
}

static void reset_marks(struct UIRoMapWnd *self)
{
    list_clear(&self->entries, destroy_entry);
    self->minimap = NULL;
    self->members_box = NULL;
    self->minimap_box = NULL;
    self->spot = NULL;
    self->hover = NULL;
    self->hover_shown = 0;
}

static struct UIBitmapButton *add_button(struct UIRoMapWnd *self, const char *up, const char *over,
                                         const char *down, int id, const char *tip)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, up, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, over, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, down, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, -100, -100);
    button->button.base.id = id;
    ui_bitmap_button_set_tool_tip(button, tip);
    ui_window_add_child(&self->frame.base, &button->button.base);
    return button;
}

void ui_ro_map_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIRoMapWnd *self = (struct UIRoMapWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *combo = mem_alloc(sizeof(struct UIComboBoxDefWnd));
    struct UIBitmapButton *close;
    struct String checked, unchecked, guided;
    struct UIWindow *check, *edit;
    const char *hint;
    int right, i, count, selected;

    if (combo)
        combo = ui_combo_box_def_wnd_construct(combo);
    self->combo = combo;
    ((struct UIComboBoxDefWnd *)combo)->list_height = 0x43;
    ui_window_create(self->combo, 0x6e, self->combo_height);
    ui_window_move(self->combo, 0x11, 0);
    combo = self->combo;
    ((struct UIComboBoxDefWnd *)combo)->border_r = 0x64;
    ((struct UIComboBoxDefWnd *)combo)->border_g = 0x64;
    ((struct UIComboBoxDefWnd *)combo)->border_b = 0x64;
    ((struct UIComboBoxDefWnd *)combo)->back_r = 0xe8;
    ((struct UIComboBoxDefWnd *)combo)->back_g = 0xe8;
    ((struct UIComboBoxDefWnd *)combo)->back_b = 0xe8;
    ui_window_add_child(wnd, self->combo);
    count = (int)g_worldView.world_count;
    for (i = 0; i < count; i++) {
        struct String item;

        string_init(&item);
        string_assign_cstr(&item, g_worldView.worlds[i].name);
        ui_combo_box_def2_wnd_add_item(self->combo, item, 0);
    }
    selected = world_view_select_for_map(&g_worldView, game_current_map_name());
    if (count)
        ui_combo_box_def_wnd_select(self->combo, selected);
    world_view_prepare(&g_worldView);
    ui_window_invalidate_base(wnd);

    navi_manager_guided_map_name(&g_navi, &guided);
    if (guided.size != 0) {
        string_append_cstr(&guided, ".rsw");
        world_view_set_current(&g_worldView, string_cstr(&guided));
    } else {
        world_view_clear_current(&g_worldView);
    }
    string_free(&guided);
    ui_ro_map_wnd_load_result_maps(self);
    ui_ro_map_wnd_load_route_maps(self);

    right = wnd->width - 0x12;
    close = mem_alloc(sizeof *close);
    if (close)
        close = ui_bitmap_button_construct(close);
    ui_bitmap_button_set_bitmap(close, UI_SKIN "basic_interface\\world_close_off.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(close, UI_SKIN "basic_interface\\world_close_on.bmp", UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(close, UI_SKIN "basic_interface\\world_close_on.bmp", UI_BTN_DOWN, 0);
    ui_window_create(&close->button.base, close->bitmap_width, close->bitmap_height);
    ui_window_move_base(&close->button.base, right, 0);
    close->button.base.id = UI_ID_CLOSE;
    ui_bitmap_button_set_tool_tip(close, "ctrl+`");
    ui_window_add_child(wnd, &close->button.base);

    string_init(&checked);
    string_assign(&checked, UI_SKIN "checkbox_1.bmp", 0x1d);
    string_init(&unchecked);
    string_assign(&unchecked, UI_SKIN "checkbox_0.bmp", 0x1d);
    check = mem_try_alloc(sizeof(struct UICheckBox));
    if (check)
        check = (struct UIWindow *)ui_check_box_construct(check);
    self->dungeon_check = check;
    ui_window_create(check, 0x50, 0xc);
    ui_check_box_set_bitmaps(self->dungeon_check, string_cstr(&checked), string_cstr(&unchecked),
                          msg_str(MSG_ROMAP_DUNGEONS));
    self->dungeon_check->id = ROMAP_ID_DUNGEONS;
    ui_window_move(self->dungeon_check, right - 0x52, 3);
    ui_check_box_set_check(self->dungeon_check, self->show_dungeons);
    ui_check_box_set_tool_tip(self->dungeon_check, "Tab");
    ui_window_add_child(wnd, self->dungeon_check);

    self->search = add_button(self, UI_SKIN "navigation_interface\\NaviSearch_A.bmp",
                              UI_SKIN "navigation_interface\\NaviSearch_B.bmp",
                              UI_SKIN "navigation_interface\\NaviSearch_C.bmp", ROMAP_ID_SEARCH,
                              msg_str(MSG_ROMAP_SEARCH_TIP));
    self->navi = add_button(self, UI_SKIN "scroll0down.bmp", UI_SKIN "scroll0down.bmp", UI_SKIN "scroll0down.bmp",
                            ROMAP_ID_NAVI, msg_str(MSG_ROMAP_NAVI_TIP));
    edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    self->edit = edit;
    ui_window_create(edit, 0xc8, 0x12);
    ui_window_move(self->edit, -100, -100);
    ((struct UIEditCtrl *)self->edit)->max_chars = 0x3c;
    ui_edit_ctrl_set_back_colour(self->edit, 0xe8, 0xe8, 0xe8);
    ui_edit_ctrl_set_border_colour(self->edit, 0xc8, 0x64, 0x64);
    ((struct UIEditCtrl *)self->edit)->no_border = 0;
    hint = msg_str(MSG_ROMAP_EDIT_HINT);
    edit = self->edit;
    if (hint)
        string_assign_cstr(&((struct UIEditCtrl *)edit)->tooltip, hint);
    ui_window_add_child(wnd, edit);

    ui_ro_map_wnd_place_search(self);
    string_free(&unchecked);
    string_free(&checked);
    build_marks(self);
}

intptr_t ui_ro_map_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                  intptr_t val3, intptr_t val4)
{
    struct UIRoMapWnd *self = (struct UIRoMapWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case 0:
        if (self->edit != sender)
            break;
        if (ui_edit_ctrl_get_text_size(self->edit) <= 0)
            break;
        ui_ro_map_wnd_send_msg(wnd, self->edit, UI_MSG_COMMAND, ROMAP_ID_SEARCH, 0, 0, 0);
        return 0;

    case UI_MSG_COMMAND:
        switch (val1) {
        case ROMAP_ID_DUNGEONS:
            self->show_dungeons = !self->show_dungeons;
            self->dungeon_check->state = self->show_dungeons;
            ui_window_invalidate(self->dungeon_check);
            ui_window_invalidate_base(wnd);
            return 0;
        case ROMAP_ID_NAVI:
            ui_make_window(WND_NAVIGATION);
            ui_window_send_msg(g_windowMgr.navigation_v4_wnd, NULL, UI_MSG_COMMAND, ROMAP_ID_NAVI, 0, 0, 0);
            ui_ro_map_wnd_place_search(self);
            return 0;
        case UI_ID_CLOSE:
            ui_close_window(WND_ROMAP);
            return 0;
        case ROMAP_ID_SEARCH:
            if (ui_edit_ctrl_get_text_size(self->edit)) {
                struct String text;

                string_init(&text);
                string_assign_cstr(&text, ui_edit_ctrl_get_text(self->edit));
                if (navi_open_search(&g_navi, string_cstr(&text), text.size, 0, 1) == 1)
                    ui_ro_map_wnd_load_result_maps(self);
                string_free(&text);
            }
            return 0;
        }
        return 0;

    case ROMAP_MSG_SELECT:
        if (self->combo == sender) {
            world_view_select(&g_worldView, val1);
            ui_window_invalidate_base(wnd);
            return 0;
        }
        if (val1 >= 0
            && (unsigned)val1 < (unsigned)((struct String *)self->results.last
                                           - (struct String *)self->results.first)) {
            const struct WorldViewWorld *world = world_view_current_world(&g_worldView);
            const struct WorldViewArea *place;
            const struct WorldViewMap *area;
            struct String text;
            if (!world || self->place_index < 0 || (size_t)self->place_index >= world->area_count)
                return 0;
            place = &world->areas[self->place_index];
            if ((size_t)val1 >= place->map_count)
                return 0;
            area = &place->maps[val1];
            string_init(&text);
            string_assign_cstr(&text, area->name);
            if (text.size > 4)
                string_resize(&text, text.size - 4, 0);
            g_navi.windowCollapsed = 0;
            navi_open_search(&g_navi, string_cstr(&text), text.size, 0, 0);
            if (ui_window_find(WND_NAVIGATION))
                navi_show_results();
            ui_ro_map_wnd_place_search(self);
            string_free(&text);
            ui_window_invalidate_base(wnd);
        }
        return 0;

    case ROMAP_MSG_RESET:
        reset_marks(self);
        build_marks(self);
        ui_window_invalidate_base(wnd);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_ro_map_wnd_load_guided_map(struct UIRoMapWnd *self)
{
    struct String guided;

    ui_window_invalidate_base(&self->frame.base);
    navi_manager_guided_map_name(&g_navi, &guided);
    if (guided.size != 0) {
        string_append_cstr(&guided, ".rsw");
        world_view_set_current(&g_worldView, string_cstr(&guided));
    } else {
        world_view_clear_current(&g_worldView);
    }
    string_free(&guided);
}

void ui_ro_map_wnd_load_result_maps(struct UIRoMapWnd *self)
{
    int count, i;
    struct String map;

    world_view_clear_level_marks(&g_worldView);
    count = navi_manager_result_count(&g_navi);
    if (count <= 0)
        return;
    string_init(&map);
    for (i = 0; i < count; i++) {
        struct String got;
        int sprite = 0;

        navi_manager_result_map_name(&g_navi, &got, i, &sprite);
        string_free(&map);
        map = got;
        if (map.size != 0) {
            string_append_cstr(&map, ".rsw");
            world_view_add_level_mark(&g_worldView, string_cstr(&map), sprite);
        }
    }
    ui_window_invalidate_base(&self->frame.base);
    string_free(&map);
}

void ui_ro_map_wnd_load_route_maps(struct UIRoMapWnd *self)
{
    struct String map;
    const char *name;
    int i;

    world_view_clear_marks(&g_worldView);
    if (g_navi.guideState != 2)
        return;
    string_init(&map);
    for (i = 0; i < navi_manager_route_link_count(&g_navi); i++) {
        struct NaviLink *link = navi_manager_route_link_at(&g_navi, i);

        if (!link)
            continue;
        name = navi_map_name(navi_object_map(&link->base));
        string_assign_cstr(&map, name);
        string_append_cstr(&map, ".rsw");
        world_view_add_mark(&g_worldView, string_cstr(&map));
    }
    name = navi_map_name(navi_manager_target_map2(&g_navi));
    string_assign_cstr(&map, name);
    string_append_cstr(&map, ".rsw");
    world_view_add_mark(&g_worldView, string_cstr(&map));
    ui_window_invalidate_base(&self->frame.base);
    string_free(&map);
}

static const char airship_bitmap[] = "유저인터페이스\\worldview_interface\\wv_Airf3.bmp";
struct FlyingShipRouteInfo *flying_ship_route_info_construct(struct FlyingShipRouteInfo *self)
{
    memset(&self->stops, 0, sizeof self->stops);
    self->leg = 0;
    self->leg_t = 0.0f;
    self->leg_length = 0.0f;
    self->travelled = 0.0f;
    self->at_start = 1;
    self->route = 0;
    self->colour = 0;
    self->active = self->loaded = 0;
    string_init(&self->bitmap);
    return self;
}

struct KafraMoveMapServiceInfo *kafra_move_map_service_info_construct(struct KafraMoveMapServiceInfo *self)
{
    self->stops.first = self->stops.last = self->stops.end = NULL;
    string_init(&self->origin.name);
    memset(&self->origin.rect, 0, sizeof self->origin.rect);
    string_init(&self->map);
    self->progress = 0;
    kafra_services_load(g_session1.luaState);
    return self;
}

struct UIRoMapWnd *ui_ro_map_wnd_construct(struct UIRoMapWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_ro_map_wnd_ops;
    list_init(&self->entries);
    kafra_move_map_service_info_construct(&self->kafra);
    flying_ship_route_info_construct(&self->ship[0]);
    flying_ship_route_info_construct(&self->ship[1]);
    self->results.first = self->results.last = self->results.end = NULL;
    list_clear(&self->entries, destroy_entry);
    self->minimap = NULL;
    self->members_box = NULL;
    self->minimap_box = NULL;
    self->spot = NULL;
    self->hover = NULL;
    self->hover_shown = 0;
    self->origin_x = self->origin_y = 0;
    self->minimap_state = 0;
    self->show_dungeons = 0;
    self->frame_time = 0;
    self->first_frame = 1;
    string_assign(&self->ship[0].bitmap, airship_bitmap, sizeof airship_bitmap - 1);
    self->ship[0].colour = 0xaaffffff;
    self->ship[0].route = 1;
    string_assign(&self->ship[1].bitmap, airship_bitmap, sizeof airship_bitmap - 1);
    self->ship[1].colour = 0xaaffffff;
    self->ship[1].route = 0;
    self->combo_height = 0x12;
    self->mouse_x = self->mouse_y = 0;
    self->kafra_rect.left = self->kafra_rect.top = self->kafra_rect.right = self->kafra_rect.bottom = 0;
    return self;
}

struct UIWindow *ui_ro_map_wnd_new(void)
{
    return (struct UIWindow *)ui_ro_map_wnd_construct(mem_alloc(sizeof(struct UIRoMapWnd)));
}
