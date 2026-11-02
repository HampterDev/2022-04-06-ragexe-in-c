#include "ui/window_access.h"
#include "navi/navi_manager.h"
#include "navi/navi_search.h"
#include "navi/navigation.h"
#include "game/navigation.h"
#include "ui/quests/navigation.h"
#include "ui/navigation_view.h"
#include "world_state_memory.h"
#include <limits.h>
#include <stdio.h>

struct NaviManager g_navi;
struct UIWindowMgr g_windowMgr;
static struct NaviMap map;
static struct UINavigationV4Wnd window;
static struct UIWindowOps window_table;
static int opens, raised, invalidated, relayouts, list_layouts, refreshed;
static int chat_lines, boxes, commands, guided_maps, result_maps, route_maps;
static int guide_status = 11, guide_calls, guide_x, guide_y, updates, searches, open_ok = 1;
static struct NaviManager *searched_manager;
static char searched_text[256];

char *msg_str(int id) { return id == 0x90b ? "%s" : "Navigation status"; }
struct NaviMap *navi_manager_map_by_name(struct NaviManager *manager, const char *name)
{ assert(manager); return !strcmp(name, string_cstr(&map.name)) ? &map : NULL; }
struct NaviMap *navi_map_named(struct NaviManager *manager, const char *name)
{ return !strcmp(name, "map_alias") || !strcmp(name, "an_alias_owned_by_the_manager") ? &map : navi_manager_map_by_name(manager, name); }
const char *navi_map_name(struct NaviMap *value) { assert(value == &map); return string_cstr(&map.name); }
int navi_manager_guide_to(struct NaviManager *manager, int x, int y)
{ assert(manager == &g_navi); ++guide_calls; guide_x = x; guide_y = y; return guide_status; }
unsigned char navi_manager_update_window(struct NaviManager *manager) { assert(manager); ++updates; return 1; }
unsigned char navi_manager_search(struct NaviManager *manager, struct String text, int kind, struct Vec *results)
{
    assert(kind == 0 && results == &manager->results);
    assert(text.size < sizeof searched_text); memcpy(searched_text, string_cstr(&text), text.size + 1);
    searched_manager = manager; ++searches; string_free(&text); return 0;
}
static void invalidate(struct UIWindow *value) { assert(value == &window.frame.base); ++invalidated; }
struct UIWindow *ui_window_open(int id)
{
    assert(id == NAVI_WINDOW_ID); ++opens;
    if (open_ok) g_windowMgr.navigation_v4_wnd = &window.frame.base;
    return g_windowMgr.navigation_v4_wnd;
}
intptr_t ui_window_send(struct UIWindow *value, struct UIWindow *sender, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{ assert(value == &window.frame.base && !sender && message == UI_MSG_COMMAND && a == NAVI_ID_MAXIMIZE && !b && !c && !d); ++commands; return 0; }
void ui_window_mgr_raise_window(struct UIWindowMgr *owner, struct UIWindow *value)
{ assert(owner == &g_windowMgr && value == g_windowMgr.navigation_v4_wnd); ++raised; }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *owner, int id) { (void)owner; (void)id; abort(); }
intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *owner, int message, intptr_t text, intptr_t colour, intptr_t c, intptr_t d)
{ assert(owner == &g_windowMgr && message == 1 && text && colour == 0x62b0ff && !c && !d); ++chat_lines; return 1; }
int ui_window_mgr_message_box(struct UIWindowMgr *owner, const char *text, int type, int a, int b, int c, const char *title, int width, int height, int d)
{ assert(owner == &g_windowMgr && text && !type && a == 1 && !b && !c && !strcmp(title, "메시지") && width == 280 && height == 120 && !d); ++boxes; return 0; }
void ui_navigation_v4_wnd_relayout(struct UINavigationV4Wnd *value) { assert(value == &window); ++relayouts; }
void ui_navigation_v4_wnd_layout_list_page(struct UINavigationV4Wnd *value) { assert(value == &window); ++list_layouts; }
void ui_navigation_v4_wnd_fill_results(struct UINavigationV4Wnd *value) { assert(value == &window); ++refreshed; }
void ui_ro_map_wnd_load_guided_map(struct UIRoMapWnd *value) { assert((void *)value == g_windowMgr.kafra_move_map_service_info_wnd); ++guided_maps; }
void ui_ro_map_wnd_load_result_maps(struct UIRoMapWnd *value) { assert((void *)value == g_windowMgr.kafra_move_map_service_info_wnd); ++result_maps; }
void ui_ro_map_wnd_load_route_maps(struct UIRoMapWnd *value) { assert((void *)value == g_windowMgr.kafra_move_map_service_info_wnd); ++route_maps; }

static void reset(void)
{
    memset(&g_navi, 0, sizeof g_navi); memset(&g_windowMgr, 0, sizeof g_windowMgr);
    memset(&window, 0, sizeof window); memset(&window_table, 0, sizeof window_table);
    window_table.invalidate = invalidate; window.frame.base.ops = &window_table;
    string_init(&map.name); string_assign_cstr(&map.name, "prontera");
    string_init(&g_navi.searchText); search_result_init(&g_navi.guided, -1); search_result_init(&g_navi.destination, -1);
    opens = raised = invalidated = relayouts = list_layouts = refreshed = 0;
    chat_lines = boxes = commands = guided_maps = result_maps = route_maps = 0;
    guide_calls = updates = searches = 0; guide_status = 11; open_ok = 1;
}
static void cleanup(void)
{
    string_free(&map.name); string_free(&g_navi.searchText);
    search_result_destroy(&g_navi.guided); search_result_destroy(&g_navi.destination);
    assert(!live_blocks);
}

static void command_paths(void)
{
    const char *borrowed = "map_alias"; int before;
    g_windowMgr.kafra_move_map_service_info_wnd = (void *)2;
    assert(navi_navigate(&g_navi, borrowed, NAVI_RESULT_MAP, 5, 1, 17, 29, 1002));
    assert(guide_x == 17 && guide_y == 29 && !opens && g_navi.windowHidden == 1);
    assert(g_navi.guided.obj == &map && g_navi.destination.obj == &map);
    assert(g_navi.routeUseKafra == 1 && g_navi.routeUseScrolls == 0 && g_navi.routeUseAirship == 1);
    assert(guided_maps == 1 && result_maps == 1 && route_maps == 1);
    string_assign(&g_navi.searchText, "an_alias_owned_by_the_manager", sizeof "an_alias_owned_by_the_manager" - 1);
    assert(navi_navigate(&g_navi, string_cstr(&g_navi.searchText), NAVI_RESULT_MAP, 5, 1, 17, 29, 0));
    g_windowMgr.navigation_v4_wnd = &window.frame.base;
    assert(navi_navigate(&g_navi, "prontera", NAVI_RESULT_MAP, 2, 1, 7, 8, 0));
    assert(!g_navi.windowHidden && relayouts == 1);
    guide_status = -94;
    assert(!navi_navigate(&g_navi, borrowed, NAVI_RESULT_MAP, 5, 0, 1, 2, 0)); guide_status = 11;
    assert(!navi_navigate(&g_navi, borrowed, 4, 5, 0, 1, 2, 0));
    assert(!navi_navigate(&g_navi, borrowed, 5, 5, 0, 1, 2, 0));
    assert(!navi_navigate(&g_navi, NULL, 1, 5, 0, 1, 2, 0));
    assert(!navi_show_map(&g_navi, borrowed, 1, 2));
    assert(navi_show_map(&g_navi, "prontera", 1000, 10));
    assert(g_navi.guided.kind == NAVI_RESULT_MAP && g_navi.guided.x == -1 && g_navi.guided.y == -1);
    assert(window.frame.base.alpha_target == 255 && list_layouts == 1 && invalidated == 1);
    assert(navi_show_map(&g_navi, "prontera", INT_MIN, INT_MAX));
    assert(!strcmp(string_cstr(&g_navi.guided.name), "prontera -2147483648,2147483647"));
    {
        char long_name[301];
        memset(long_name, 'a', 300); long_name[300] = 0; string_assign(&map.name, long_name, 300);
        assert(navi_show_map(&g_navi, long_name, INT_MIN, INT_MAX));
        assert(g_navi.guided.name.size == 323 && !memcmp(string_cstr(&g_navi.guided.name), long_name, 300));
        string_assign_cstr(&map.name, "prontera");
    }
    assert(navi_open_search(&g_navi, "a search query held by the caller", sizeof "a search query held by the caller" - 1, 3, 0));
    assert(!strcmp(searched_text, "a search query held by the caller") && commands == 1 && raised == 1);
    assert(g_navi.searchFilter == 3 && searched_manager == &g_navi);
    before = searches; assert(!navi_open_search(&g_navi, "test", 4, 5, 0) && before == searches);
    assert(!navi_open_search(&g_navi, NULL, 1, 0, 0));
    assert(!navi_open_search(&g_navi, "test", SIZE_MAX, 0, 0));
    {
        struct NaviManager other = {0}; string_init(&other.searchText);
        assert(navi_open_search(&other, "other", 5, 0, 1) && searched_manager == &other);
        string_free(&other.searchText);
    }
    g_windowMgr.navigation_v4_wnd = NULL; open_ok = 0;
    ui_navigation_show_map(); ui_navigation_show_search(); ui_navigation_invalidate(); ui_navigation_refresh_results();
    assert(!ui_navigation_is_open()); open_ok = 1; ui_navigation_open(); assert(ui_navigation_is_open());
    ui_navigation_refresh_results(); assert(refreshed == 1);
    ui_navigation_message_box("status"); assert(boxes == 1);
}
static void readers(void)
{
    int before;
    {
        struct PacketNavigationActive packet = {0}; int digits;
        packet.type = NAVI_RESULT_MAP; packet.hide_window = 1;
        memcpy(packet.map_name, "abcdefghijklmnop", 16); packet.x = -32768; packet.y = 32767; packet.mob_id = 1002;
        string_assign(&map.name, packet.map_name, 16);
        for (digits = 0; digits < 256; ++digits) {
            int rest = digits >= 100 ? digits - 100 : digits;
            packet.flag = (unsigned char)digits; game_mode_on_navigation_active(NULL, &packet);
            assert(guide_x == -32768 && guide_y == 32767);
            assert(g_navi.routeUseKafra == (digits >= 100));
            assert(g_navi.routeUseScrolls == (rest >= 10));
            if (rest >= 10) rest -= 10;
            assert(g_navi.routeUseAirship == (rest >= 1));
        }
        before = guide_calls; game_mode_on_navigation_active(NULL, NULL); assert(guide_calls == before);
    }
}
int main(void)
{
    reset(); command_paths(); readers(); cleanup();
    puts("navigation events: borrowed commands, full-width packets, long labels, search ownership and typed UI operations passed");
    return 0;
}
