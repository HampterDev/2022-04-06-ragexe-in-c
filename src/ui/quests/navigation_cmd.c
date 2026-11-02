#include "ui/controls/listbox.h"
#include "ui/window_access.h"
#include "ui/window_manager_state.h"
#include "core/markup_tags.h"
#include "actor/appearance.h"
#include "game/context.h"
#include "navi/navi.h"
#include "ui/quests/navigation.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/ui_std.h"


#include <stdio.h>
#include "ui/chat/chat_window_parts.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "ui/chat/chat_window.h"
#include "ui/wndmgr/wndmgr.h"
#include "core/app.h"
#include "platform/clock.h"

#define MSG_NAVI_STOP_GUIDE 0x8bf
#define MSG_NAVI_MOB_VERY_LOW 0x995
#define MSG_NAVI_MOB_VERY_HIGH 0x991
#define MSG_NAVI_MOB_HIGH 0x992
#define MSG_NAVI_MOB_AVERAGE 0x993
#define MSG_NAVI_MOB_LOW 0x994
#define MSG_AIRPLANE_GUIDED_MOB 0xd3b
#define MSG_AIRPLANE_BAD_START 0xd03
#define MSG_AIRPLANE_BAD_END 0xd04
#define MSG_AIRPLANE_SAME_MAP 0xd19
#define MSG_AIRPLANE_DEAD 0xd1a
#define AIRPLANE_MSG_NOTICE 0x9a
#define AIRPLANE_MSG_DISABLE_ALL 0x23
#define AIRPLANE_RESULT_CLOSED 0xb4

#define COMBO_MSG_CHOSEN 0x27

#define NAVI_ROUTE_MSG_REFRESH 0x3c
#define NAVI_CHAT_MSG_LINK 0x99

struct String *ui_navigation_v4_wnd_result_name(struct UINavigationV4Wnd *self, struct String *out,
                                                         int kind, void *obj)
{
    const char *name;

    string_init(out);
    if (kind == 2 || kind == 3) {
        name = navi_object_name(obj);
    } else if (kind == 1 || kind == 0) {
        name = navi_map_name(obj);
    } else {
        return out;
    }
    string_assign_cstr(out, name);
    return out;
}

static void mob_count_text(char *buf, int count)
{
    struct String word;
    const char *text;

    if (count <= 0)
        return;
    text = msg_str(MSG_NAVI_MOB_VERY_LOW);
    string_init(&word);
    string_assign_cstr(&word, text);
    if (count > 99) {
        text = msg_str(MSG_NAVI_MOB_VERY_HIGH);
        string_assign_cstr(&word, text);
    } else if (count > 49) {
        string_assign_cstr(&word, msg_str(MSG_NAVI_MOB_HIGH));
    } else if ((unsigned)(count - 20) <= 29) {
        string_assign_cstr(&word, msg_str(MSG_NAVI_MOB_AVERAGE));
    } else if ((unsigned)(count - 5) <= 14) {
        string_assign_cstr(&word, msg_str(MSG_NAVI_MOB_LOW));
    }
    sprintf(buf, " (%s)", string_cstr(&word));
    string_free(&word);
}

struct String *ui_navigation_v4_wnd_member_text(struct UINavigationV4Wnd *self, struct String *out,
                                                         int kind, void *obj)
{
    struct NaviObject *object = obj;
    struct NaviPoint pos;
    const char *name;
    char buf[0x34];
    int y;

    memset(buf, 0, 0x32);
    string_init(out);
    if (kind == 2) {
        name = navi_map_name(navi_object_map(object));
        string_assign_cstr(out, name);
        y = navi_object_position(object, &pos)->y;
        sprintf(buf, " [%d,%d]", navi_object_position(object, &pos)->x, y);
        string_append_cstr(out, buf);
    } else if (kind == 3) {
        name = navi_map_name(navi_object_map(object));
        string_assign_cstr(out, name);
        mob_count_text(buf, navi_object_sprite_high(object));
        string_append_cstr(out, buf);
    } else if (kind == 1 || kind == 0) {
        name = navi_map_display_name(obj);
        string_assign_cstr(out, name);
    }
    return out;
}

struct String *ui_navigation_v4_wnd_mob_text(struct UINavigationV4Wnd *self, struct String *out,
                                                      void *mob)
{
    struct NaviObject *object = mob;
    const char *name;
    char buf[0x34];

    memset(buf, 0, 0x32);
    string_init(out);
    name = navi_object_name(object);
    string_assign_cstr(out, name);
    mob_count_text(buf, navi_object_sprite_high(object));
    string_append_cstr(out, buf);
    return out;
}

void ui_navigation_v4_wnd_set_road_icon(struct UINavigationV4Wnd *self, int icon)
{
    char normal[0x64], over[0x64], press[0x64];

    if ((unsigned)(icon - 1) > 7)
        return;
    g_navi.roadIcon = icon;
    memset(normal, 0, sizeof normal);
    memset(over, 0, sizeof over);
    memset(press, 0, sizeof press);
    sprintf(normal, NAVI_DIR "btn_roadIocn_select%d_normal.bmp", icon);
    sprintf(over, NAVI_DIR "btn_roadIocn_select%d_over.bmp", g_navi.roadIcon);
    sprintf(press, NAVI_DIR "btn_roadIocn_select%d_press.bmp", g_navi.roadIcon);
    ui_bitmap_button_set_bitmap(self->road_icon, normal, 0, 0);
    ui_bitmap_button_set_bitmap(self->road_icon, over, 1, 0);
    ui_bitmap_button_set_bitmap(self->road_icon, press, 2, 0);
    ui_window_invalidate_base(&self->road_icon->button.base);
    navi_manager_load_grid_images(&g_navi);
}

void ui_navigation_v4_wnd_put_link(struct UINavigationV4Wnd *self, struct String map, int x, int y)
{
    struct String xs = {0}, ys = {0}, link = {0};
    struct UIWindow *focus;

    navi_links_to_base62(&xs, x >= 0 ? (unsigned)x : 1000);
    navi_links_to_base62(&ys, y >= 0 ? (unsigned)y : 1000);
    string_assign_cstr(&link, "<NAVIL>");
    string_append(&link, string_cstr(&map), map.size);
    string_append(&link, string_cstr(&xs), xs.size);
    string_append(&link, string_cstr(&ys), ys.size);
    string_append_cstr(&link, "</NAVIL>");
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) &&
        ui_window_mgr_get_focus_edit(&g_windowMgr)->id != NAVI_ID_SEARCH_EDIT) {
        if (ui_chat_window_edit(g_windowMgr.new_chat_wnd) == ui_window_mgr_get_focus_edit(&g_windowMgr) && !g_windowMgr.chat_bar_shown)
            ui_new_chat_wnd_on_enter((struct UINewChatWnd *)g_windowMgr.new_chat_wnd);
    } else if (!g_windowMgr.chat_bar_shown) {
        ui_new_chat_wnd_on_enter((struct UINewChatWnd *)g_windowMgr.new_chat_wnd);
    } else {
        ui_window_mgr_set_focus_edit(&g_windowMgr, ui_chat_window_edit(g_windowMgr.new_chat_wnd));
    }
    focus = ui_window_mgr_get_focus_edit(&g_windowMgr);
    ui_window_send_msg(focus, NULL, NAVI_CHAT_MSG_LINK, (int)string_cstr(&link), 0, 0, 0);
    string_free(&link);
    string_free(&ys);
    string_free(&xs);
    string_free(&map);
}

void ui_navigation_v4_wnd_pick_entry(struct UINavigationV4Wnd *self, int entry)
{
    if (entry == -1)
        return;
    g_navi.selectedMember = entry;
    if (!navi_manager_guide_selected(&g_navi)) {
        g_navi.selectedMember = 0;
        navi_manager_guide_selected(&g_navi);
    }
    self->entries->list.selected = g_navi.selectedMember;
    ui_window_invalidate_base(&self->entries->list.base);
    if (g_windowMgr.kafra_move_map_service_info_wnd)
        ui_ro_map_wnd_load_guided_map((struct UIRoMapWnd *)g_windowMgr.kafra_move_map_service_info_wnd);
    ui_window_invalidate_base(&self->frame.base);
}

static void find_route(struct UINavigationV4Wnd *self)
{
    struct SearchResult result;
    struct UIWindow *route;
    int status;

    ui_window_invalidate_base(&self->frame.base);
    status = navi_manager_destination_from_guided(&g_navi);
    if (status == 0) {
        navi_manager_destination(&g_navi, &result);
        status = navi_manager_guide_to(&g_navi, result.x, result.y);
        search_result_destroy(&result);
        if (status >= 0) {
            g_navi.windowPage = NAVI_VIEW_ROUTE;
            navi_manager_guide_link(&g_navi, navi_manager_route_step(&g_navi));
            if (g_windowMgr.kafra_move_map_service_info_wnd)
                ui_ro_map_wnd_load_route_maps((struct UIRoMapWnd *)g_windowMgr.kafra_move_map_service_info_wnd);
        }
    }
    navi_manager_report_status(&g_navi, status, 1);
    ui_navigation_v4_wnd_relayout(self);
    route = ui_get_window(NAVI_ROUTE_WND_ID);
    if (route)
        ui_window_send_msg(route, NULL, NAVI_ROUTE_MSG_REFRESH, 0, 0, 0, 0);
}

static void run_search(struct UINavigationV4Wnd *self)
{
    if (!ui_edit_ctrl_get_text_size(&self->search->edit.base))
        return;
    string_assign_cstr(&g_navi.searchText, ui_edit_ctrl_get_text(&self->search->edit.base));
    navi_manager_run_search(&g_navi);
    ui_list_box_navi_search_clear_items(&self->entries->list);
    ui_window_invalidate_base(&self->entries->list.base);
    navi_manager_stop_guide(&g_navi);
    navi_manager_update_window(&g_navi);
    ui_navigation_v4_wnd_fill_results(self);
    g_navi.windowPage = NAVI_VIEW_SEARCH;
    if (g_windowMgr.kafra_move_map_service_info_wnd)
        ui_ro_map_wnd_load_result_maps((struct UIRoMapWnd *)g_windowMgr.kafra_move_map_service_info_wnd);
    ui_navigation_v4_wnd_layout_list_page(self);
}

static void go_back(struct UINavigationV4Wnd *self)
{
    struct SearchResult result;
    void *dest;

    dest = navi_manager_destination(&g_navi, &result)->obj;
    search_result_destroy(&result);
    if (!dest) {
        if (g_navi.windowPage != NAVI_VIEW_TARGET)
            return;
        if (g_navi.guideState == 2) {
            ui_navigation_v4_wnd_layout_route(self);
            g_navi.windowPage = NAVI_VIEW_ROUTE;
            navi_manager_guide_link(&g_navi, navi_manager_route_step(&g_navi));
        } else {
            ui_navigation_v4_wnd_layout_list_page(self);
            g_navi.windowPage = NAVI_VIEW_SEARCH;
            navi_manager_guide_selected(&g_navi);
        }
        return;
    }
    if (g_navi.windowPage == NAVI_VIEW_ROUTE || (g_navi.windowPage == NAVI_VIEW_TARGET && g_navi.guideState != 2)) {
        g_navi.windowPage = NAVI_VIEW_SEARCH;
        navi_manager_guide_selected(&g_navi);
    } else {
        g_navi.windowPage = NAVI_VIEW_ROUTE;
        navi_manager_guide_link(&g_navi, navi_manager_route_step(&g_navi));
    }
    ui_navigation_v4_wnd_relayout(self);
}

static void share(struct UINavigationV4Wnd *self)
{
    struct SearchResult result;
    struct NaviObject *obj;
    struct String map;
    int kind, x, y;

    navi_manager_guided_map_name(&g_navi, &map);
    x = map.size;
    string_free(&map);
    if (x == 0)
        return;
    kind = navi_manager_guided(&g_navi, &result)->kind;
    search_result_destroy(&result);
    if (kind <= 1) {
        ui_navigation_v4_wnd_layout_route(self);
        g_navi.windowPage = NAVI_VIEW_TARGET;
        self->hint_until = platform_ticks_ms() + 5000;
        return;
    }
    navi_manager_guided(&g_navi, &result);
    x = y = 1000;
    obj = result.obj;
    if (kind == 2) {
        struct NaviPoint pos;

        x = navi_object_position(obj, &pos)->x;
        y = navi_object_position(obj, &pos)->y;
    }
    string_assign_cstr(&map, navi_map_name(navi_object_map(obj)));
    ui_navigation_v4_wnd_put_link(self, map, x, y);
    search_result_destroy(&result);
}

static void close_navigation(struct UINavigationV4Wnd *self)
{
    if (g_navi.guideState == 2) {
        if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_NAVI_STOP_GUIDE), UI_MSGBOX_QUESTION, 1, 0, 0,
                                   "메시지", 0x118, 0x78, 0) != UI_MSGBOX_ANSWER_YES)
            return;
        navi_manager_set_guide_state(&g_navi, 1);
        g_navi.windowPage = NAVI_VIEW_SEARCH;
        navi_manager_reset_guide(&g_navi);
        ui_close_window(NAVI_ROUTE_WND_ID);
        ui_navigation_v4_wnd_relayout(self);
        world_view_clear_marks(&g_worldView);
        return;
    }
    if (g_windowMgr.kafra_move_map_service_info_wnd)
        ui_navigation_v4_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, NAVI_ID_MINIMIZE, 0, 0, 0);
    g_navi.windowPage = NAVI_VIEW_SEARCH;
    g_navi.windowCollapsed = 0;
    navi_manager_reset(&g_navi, 1);
    world_view_clear_marks(&g_worldView);
    world_view_clear_level_marks(&g_worldView);
    world_view_clear_current(&g_worldView);
    ui_close_window(NAVI_ROUTE_WND_ID);
    ui_close_window(NAVI_WINDOW_ID);
}

static unsigned find_rsw(const struct String *s)
{
    const char *text = string_cstr(s);
    unsigned i;

    if (s->size < 4)
        return (unsigned)-1;
    for (i = 0; i + 4 <= s->size; i++)
        if (memcmp(text + i, ".rsw", 4) == 0)
            return i;
    return (unsigned)-1;
}

static void airship_refuse(struct UIWindow *wnd, int msg)
{
    ui_window_send_msg(wnd, NULL, AIRPLANE_MSG_NOTICE, (int)msg_str(msg), 0, 0, 0);
    ui_window_send_msg(wnd, NULL, AIRPLANE_MSG_DISABLE_ALL, 0, 0, 0, 0);
}

static void open_airship(struct UINavigationV4Wnd *self)
{
    struct UIWindow *wnd;
    struct String here, there, part, joined, copy;
    const char *map_name;
    unsigned pos;

    if (!game_current_player() || ui_get_window(NAVI_AIRSHIP_WND_ID))
        return;
    wnd = (struct UIWindow *)ui_private_airplane_wnd_new();
    ui_window_create(wnd, 0xd0, 0x6e);
    ui_frame_wnd_move(wnd, (int)((double)(g_renderer->width - 0xd0) * 0.5),
             (int)((double)(g_renderer->height - 0x6e) * 0.5));
    ui_window_mgr_add_window(&g_windowMgr, wnd);
    if (navi_manager_guided_kind(&g_navi) == 2)
        ui_private_airplane_wnd_send_msg(wnd, NULL, AIRPLANE_MSG_NOTICE, (int)msg_str(MSG_AIRPLANE_GUIDED_MOB), 0, 0, 0);

    map_name = game_current_map_name();
    string_init(&here);
    string_assign_cstr(&here, map_name);
    pos = find_rsw(&here);
    string_init(&part);
    string_assign(&part, string_cstr(&here), here.size < pos ? here.size : pos);
    string_append_cstr(&part, ".gat");
    string_move(&joined, &part);
    string_free(&here);
    string_move(&here, &joined);
    string_free(&part);
    string_copy(&copy, &here);
    if (!private_airplane_can_start(g_airshipTickets, copy))
        airship_refuse(wnd, MSG_AIRPLANE_BAD_START);

    navi_manager_guided_map_name(&g_navi, &part);
    string_append_cstr(&part, ".gat");
    string_move(&there, &part);
    string_copy(&copy, &there);
    if (!private_airplane_can_end(g_airshipTickets, copy))
        airship_refuse(wnd, MSG_AIRPLANE_BAD_END);

    if (string_compare(&here, &there) == 0)
        airship_refuse(wnd, MSG_AIRPLANE_SAME_MAP);
    if (player_state(game_current_player()) == 3)
        airship_refuse(wnd, MSG_AIRPLANE_DEAD);

    g_windowMgr.private_airplane_wnd = wnd;
    if (ui_window_mgr_do_modal(&g_windowMgr, wnd) == AIRPLANE_RESULT_CLOSED)
        g_windowMgr.private_airplane_wnd = NULL;
    string_free(&there);
    string_free(&here);
}

static void command(struct UINavigationV4Wnd *self, int id, int val2, int val3, int val4)
{
    int row;

    switch (id) {
    case NAVI_ID_ROAD_ICON:
        if (!ui_close_window(NAVI_ROAD_ICON_WND_ID))
            ui_make_window(NAVI_ROAD_ICON_WND_ID);
        break;
    case NAVI_ID_MAXIMIZE:
        g_navi.windowCollapsed = 0;
        self->frame.base.alpha_target = 0xff;
        self->frame.base.alpha_time = platform_ticks_ms();
        g_navi.windowHidden = 0;
        ui_navigation_v4_wnd_relayout(self);
        break;
    case UI_ID_CLOSE:
        close_navigation(self);
        break;
    case NAVI_ID_MINIMIZE:
        g_navi.windowCollapsed = 1;
        self->frame.base.alpha_target = 0xaa;
        self->frame.base.alpha_time = platform_ticks_ms();
        if (g_windowMgr.kafra_move_map_service_info_wnd)
            ui_ro_map_wnd_place_search((struct UIRoMapWnd *)g_windowMgr.kafra_move_map_service_info_wnd);
        ui_navigation_v4_wnd_relayout(self);
        break;
    case NAVI_ID_SERVICE:
        g_navi.routeUseScrolls = 0;
        g_navi.routeUseAirship = val2 != 0;
        g_navi.routeUseKafra = val2 != 0;
        break;
    case NAVI_ID_BACK:
        go_back(self);
        break;
    case NAVI_ID_SEARCH:
        run_search(self);
        break;
    case NAVI_ID_RESULTS:
        row = self->results->list.selected;
        ui_navigation_v4_wnd_fill_entries(self, row);
        if (row != -1) {
            g_navi.selectedMember = 0;
            if (!navi_manager_guide_selected(&g_navi)) {
                g_navi.selectedMember = 0;
                navi_manager_guide_selected(&g_navi);
            }
            self->entries->list.selected = g_navi.selectedMember;
            ui_window_invalidate_base(&self->entries->list.base);
            if (g_windowMgr.kafra_move_map_service_info_wnd)
                ui_ro_map_wnd_load_guided_map((struct UIRoMapWnd *)g_windowMgr.kafra_move_map_service_info_wnd);
            ui_window_invalidate_base(&self->frame.base);
        }
        ui_window_invalidate_base(&self->frame.base);
        break;
    case NAVI_ID_ENTRIES:
        row = self->entries->list.selected;
        if (row != -1) {
            ui_navigation_v4_wnd_pick_entry(self, row);
        }
        ui_window_invalidate_base(&self->frame.base);
        break;
    case NAVI_ID_HELP:
        if (!ui_close_window(NAVI_HELP_WND_ID))
            ui_make_window(NAVI_HELP_WND_ID);
        break;
    case NAVI_ID_SHARE:
        share(self);
        break;
    case NAVI_ID_FIND:
        find_route(self);
        break;
    case NAVI_ID_ROUTE_LIST:
        if (!ui_close_window(NAVI_ROUTE_WND_ID) && navi_manager_route_link_count(&g_navi))
            ui_make_window(NAVI_ROUTE_WND_ID);
        break;
    case NAVI_ID_AIRSHIP:
        open_airship(self);
        break;
    }
}

int ui_navigation_v4_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UINavigationV4Wnd *self = (struct UINavigationV4Wnd *)window;
    switch (msg) {
    case UI_MSG_DEFAULT_BUTTON:
        if (&self->search->edit.base == sender && ui_edit_ctrl_get_text_size(sender) > 0)
            ui_navigation_v4_wnd_send_msg(&self->frame.base, &self->search->edit.base, UI_MSG_COMMAND, NAVI_ID_SEARCH, 0, 0, 0);
        break;
    case UI_MSG_COMMAND:
        command(self, val1, val2, val3, val4);
        break;
    case UI_MSG_SET_INFO:
        self->info = (struct NavigationWndInfo *)val1;
        navi_window_info_clamp(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->rect.x, self->info->rect.y);
        ui_window_invalidate_base(&self->frame.base);
        break;
    case COMBO_MSG_CHOSEN:
        if ((struct UIWindow *)self->kind_box == sender) {
            g_navi.searchFilter = val1;
            navi_manager_build_groups(&g_navi);
            ui_navigation_v4_wnd_fill_results(self);
            ui_window_invalidate_base(&self->frame.base);
        }
        break;
    case NAVI_MSG_ROAD_ICON:
        ui_navigation_v4_wnd_set_road_icon(self, val1);
        break;
    }
    return 0;
}
