#include "ui/window_access.h"
#include "navi/navi_manager.h"
#include "navi/navi_search.h"
#include "navi/navigation.h"
#include "navi/replay_events.h"
#include "game/navigation.h"
#include "game/replay_navigation.h"
#include "replay/replay.h"
#include "ui/quests/navigation.h"
#include "ui/navigation_view.h"
#include "world_state_memory.h"
#include <limits.h>
#include <stdio.h>

struct NaviManager g_navi;
struct UIWindowMgr g_windowMgr;
static struct ReplayRecorder recorder;
static struct NaviMap map;
static struct UINavigationV4Wnd window;
static struct UIWindowOps window_table;
static unsigned char packet_bytes[128];
static size_t packet_size;
static int writes, write_ok = 1, opens, raised, invalidated, relayouts, list_layouts, refreshed;
static int chat_lines, boxes, commands, guided_maps, result_maps, route_maps;
static int guide_status = 11, guide_calls, guide_x, guide_y, updates, searches, open_ok = 1;
static struct NaviManager *searched_manager;
static char searched_text[256];

struct ReplayRecorder *replay_recorder_instance(void) { return &recorder; }
BYTE replay_recorder_write_packet(struct ReplayRecorder *owner, unsigned short size, const void *data, int mode, DWORD time)
{
    assert(owner == &recorder && mode == REPLAY_TIME_ZERO && time == 0 && size <= sizeof packet_bytes);
    memcpy(packet_bytes, data, size); packet_size = size; ++writes;
    return (BYTE)write_ok;
}
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
int ui_window_send(struct UIWindow *value, struct UIWindow *sender, int message, int a, int b, int c, int d)
{ assert(value == &window.frame.base && !sender && message == UI_MSG_COMMAND && a == NAVI_ID_MAXIMIZE && !b && !c && !d); ++commands; return 0; }
void ui_window_mgr_raise_window(struct UIWindowMgr *owner, struct UIWindow *value)
{ assert(owner == &g_windowMgr && value == g_windowMgr.navigation_v4_wnd); ++raised; }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *owner, int id) { (void)owner; (void)id; abort(); }
int ui_window_mgr_send_msg_(struct UIWindowMgr *owner, int message, int text, int colour, int c, int d)
{ assert(owner == &g_windowMgr && message == 1 && text && colour == 0x62b0ff && !c && !d); ++chat_lines; return 1; }
int ui_window_mgr_message_box(struct UIWindowMgr *owner, const char *text, int type, int a, int b, int c, const char *title, int width, int height, int d)
{ assert(owner == &g_windowMgr && text && !type && a == 1 && !b && !c && !strcmp(title, "Message") && width == 280 && height == 120 && !d); ++boxes; return 0; }
void ui_navigation_v4_wnd_relayout(struct UINavigationV4Wnd *value) { assert(value == &window); ++relayouts; }
void ui_navigation_v4_wnd_layout_list_page(struct UINavigationV4Wnd *value) { assert(value == &window); ++list_layouts; }
void ui_navigation_v4_wnd_fill_results(struct UINavigationV4Wnd *value) { assert(value == &window); ++refreshed; }
void ui_ro_map_wnd_load_guided_map(struct UIRoMapWnd *value) { assert((void *)value == g_windowMgr.kafra_move_map_service_info_wnd); ++guided_maps; }
void ui_ro_map_wnd_load_result_maps(struct UIRoMapWnd *value) { assert((void *)value == g_windowMgr.kafra_move_map_service_info_wnd); ++result_maps; }
void ui_ro_map_wnd_load_route_maps(struct UIRoMapWnd *value) { assert((void *)value == g_windowMgr.kafra_move_map_service_info_wnd); ++route_maps; }
int navi_results_load(struct NaviManager *manager, const void *data, size_t size) { (void)manager; (void)data; (void)size; abort(); }

static void reset(void)
{
    memset(&g_navi, 0, sizeof g_navi); memset(&g_windowMgr, 0, sizeof g_windowMgr); memset(&recorder, 0, sizeof recorder);
    memset(&window, 0, sizeof window); memset(&window_table, 0, sizeof window_table);
    window_table.invalidate = invalidate; window.frame.base.ops = &window_table;
    string_init(&map.name); string_assign_cstr(&map.name, "prontera");
    string_init(&g_navi.searchText); search_result_init(&g_navi.guided, -1); search_result_init(&g_navi.destination, -1);
    writes = opens = raised = invalidated = relayouts = list_layouts = refreshed = 0;
    chat_lines = boxes = commands = guided_maps = result_maps = route_maps = 0;
    guide_calls = updates = searches = 0; guide_status = 11; write_ok = open_ok = 1;
}
static void cleanup(void)
{
    string_free(&map.name); string_free(&g_navi.searchText);
    search_result_destroy(&g_navi.guided); search_result_destroy(&g_navi.destination);
    assert(!live_blocks);
}

#ifdef NAVI_EVENT_ORACLE
#include "navigation_event_oracle.h"
static void emit(const char *name)
{
    char path[100]; FILE *file;
    sprintf(path, "%s.bin", name); file = fopen(path, "wb"); assert(file);
    assert(fwrite(packet_bytes, 1, packet_size, file) == packet_size); assert(!fclose(file));
}
int main(void)
{
    struct String name;
    recorder.state = 2;
    string_init(&name);
    string_assign_cstr(&name, "prontera"); NaviManager_RecordNavigate(&g_navi, name, 3, 0x105, 1, -12, 32767, 1002); emit("route_short");
    string_init(&name);
    string_assign_cstr(&name, "abcdefghijklmnopqrstuvwxyz1234"); NaviManager_RecordNavigate(&g_navi, name, 1, 7, 0, INT_MIN, INT_MAX, -1); emit("route_full");
    string_init(&name);
    string_assign_cstr(&name, "프론테라"); NaviManager_RecordNavigate(&g_navi, name, 0, 5, 1, 27, 31, 1002); emit("route_korean");
    string_init(&name);
    string_assign_cstr(&name, "prontera"); NaviManager_RecordShowMap(&g_navi, name, 1000, -1); emit("map_short");
    string_init(&name);
    string_assign_cstr(&name, "abcdefghijklmnopqrstuvwxyz1234"); NaviManager_RecordShowMap(&g_navi, name, INT_MIN, INT_MAX); emit("map_full");
    assert(!live_blocks); puts("archived navigation event writer fixtures generated"); return 0;
}
#else
#include "fixtures/navigation_events_wire.h"
static void equals(const unsigned char *expected, size_t size)
{ assert(packet_size == size && !memcmp(packet_bytes, expected, size)); }
static void writers(void)
{
    unsigned allocations = live_blocks;
    recorder.state = 2;
    assert(navi_record_route("prontera", 8, 3, 0x105, 1, -12, 32767, 1002) == 54); equals(route_short, sizeof route_short);
    assert(navi_record_route("abcdefghijklmnopqrstuvwxyz1234", 30, 1, 7, 0, INT_MIN, INT_MAX, -1) == 54); equals(route_full, sizeof route_full);
    assert(navi_record_route("프론테라", strlen("프론테라"), 0, 5, 1, 27, 31, 1002) == 54); equals(route_korean, sizeof route_korean);
    assert(navi_record_map("prontera", 8, 1000, -1) == 44); equals(map_short, sizeof map_short);
    assert(navi_record_map("abcdefghijklmnopqrstuvwxyz1234", 30, INT_MIN, INT_MAX) == 44); equals(map_full, sizeof map_full);
    assert(live_blocks == allocations);
    assert(navi_record_map("abcdefghijklmnopqrstuvwxyz12345", 31, 1, 2) == -1 && recorder.writeFailed);
    recorder.writeFailed = 0; assert(navi_record_route("abcdefghijklmnopqrstuvwxyz12345", 31, 0, 0, 0, 0, 0, 0) == -1 && recorder.writeFailed);
    recorder.writeFailed = 0; assert(navi_record_map(NULL, 0, 0, 0) == -1 && recorder.writeFailed);
    recorder.writeFailed = 0; assert(navi_record_route(NULL, 0, 0, 0, 0, 0, 0, 0) == -1 && recorder.writeFailed);
    recorder.writeFailed = 0; recorder.state = 0;
    assert(!navi_record_map(NULL, SIZE_MAX, 0, 0) && !recorder.writeFailed);
    assert(!navi_record_route(NULL, SIZE_MAX, 0, 0, 0, 0, 0, 0) && !recorder.writeFailed);
    recorder.state = 2; write_ok = 0; assert(navi_record_map("prontera", 8, 1, 2) == -1); write_ok = 1;
}
static void command_paths(void)
{
    const char *borrowed = "map_alias"; int before;
    recorder.state = 2; g_windowMgr.kafra_move_map_service_info_wnd = (void *)2;
    before = writes;
    assert(navi_navigate(&g_navi, borrowed, NAVI_RESULT_MAP, 5, 1, 17, 29, 1002));
    assert(guide_x == 17 && guide_y == 29 && writes == before + 1 && !opens && g_navi.windowHidden == 1);
    assert(g_navi.guided.obj == &map && g_navi.destination.obj == &map && !strcmp((const char *)packet_bytes + 6, borrowed));
    assert(g_navi.routeUseKafra == 1 && g_navi.routeUseScrolls == 0 && g_navi.routeUseAirship == 1);
    assert(guided_maps == 1 && result_maps == 1 && route_maps == 1);
    string_assign(&g_navi.searchText, "an_alias_owned_by_the_manager", sizeof "an_alias_owned_by_the_manager" - 1);
    assert(navi_navigate(&g_navi, string_cstr(&g_navi.searchText), NAVI_RESULT_MAP, 5, 1, 17, 29, 0));
    assert(!strcmp((const char *)packet_bytes + 6, "an_alias_owned_by_the_manager"));
    g_windowMgr.navigation_v4_wnd = &window.frame.base;
    assert(navi_navigate(&g_navi, "prontera", NAVI_RESULT_MAP, 2, 1, 7, 8, 0));
    assert(!g_navi.windowHidden && relayouts == 1);
    before = writes; guide_status = -94;
    assert(!navi_navigate(&g_navi, borrowed, NAVI_RESULT_MAP, 5, 0, 1, 2, 0) && writes == before); guide_status = 11;
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
        recorder.state = 0; assert(navi_show_map(&g_navi, long_name, INT_MIN, INT_MAX));
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
    unsigned char copy[sizeof route_full + 1]; size_t i; int before;
    recorder.state = 0;
    assert(!game_replay_navigation_route(NULL, sizeof route_full));
    assert(!game_replay_navigation_map(NULL, sizeof map_full));
    memcpy(copy, route_full, sizeof route_full); before = guide_calls;
    for (i = 0; i < sizeof route_full; ++i) assert(!game_replay_navigation_route(copy, i));
    assert(!game_replay_navigation_route(copy, sizeof copy) && guide_calls == before);
    string_assign_cstr(&map.name, "abcdefghijklmnopqrstuvwxyz1234");
    assert(game_replay_navigation_route(copy, sizeof route_full) && guide_x == INT_MIN && guide_y == INT_MAX);
    assert(!memcmp(copy, route_full, sizeof route_full));
    for (i = 0; i < 6; ++i) {
        memcpy(copy, route_full, sizeof route_full); copy[i] ^= 1;
        assert(!game_replay_navigation_route(copy, sizeof route_full));
    }
    memcpy(copy, map_full, sizeof map_full); before = updates;
    assert(!game_replay_navigation_map(copy, sizeof map_full + 1));
    for (i = 0; i < sizeof map_full; ++i) assert(!game_replay_navigation_map(copy, i));
    assert(updates == before && game_replay_navigation_map(copy, sizeof map_full));
    assert(!memcmp(copy, map_full, sizeof map_full));
    for (i = 0; i < 6; ++i) {
        memcpy(copy, map_full, sizeof map_full); copy[i] ^= 1;
        assert(!game_replay_navigation_map(copy, sizeof map_full));
    }
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
    reset(); writers(); command_paths(); readers(); cleanup();
    puts("navigation events: archived bytes, bounded names, borrowed commands, full-width packets, malformed headers, long labels, search ownership and typed UI operations passed");
    return 0;
}
#endif
