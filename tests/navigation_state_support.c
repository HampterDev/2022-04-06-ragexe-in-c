#include "ui/window_messages.h"
#include "game/cells.h"
#include "render/texture_cache.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "navi/navi_search.h"
#include "resources/attr.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "fileio/resource_api.h"
#include "navi/navi_manager.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"
#include "core/numeric.h"
#include <stdlib.h>

struct Session g_session1;
void ui_navigation_open(void) { abort(); }
void ui_navigation_relayout(void) { abort(); }
void ui_navigation_invalidate(void) { abort(); }
void ui_navigation_chat(const char *text) { (void)text; abort(); }
struct UIWindowMgr g_windowMgr;

struct GameMode;
struct PathFinder;
struct ResMgr;
struct NaviLink;
struct NaviManager;
struct NaviMap;
struct NaviObject;
struct NaviPoint;
struct NaviRouteNode;
struct SearchResult;
struct String;
struct Vec;
struct WString;
struct UINavigationV4Wnd;
struct UIWindow;

void path_finder_construct(struct PathFinder * arg0) { abort(); }
void path_finder_destruct(struct PathFinder * arg0) { abort(); }
void game_mode_get_player_cell(struct GameMode *arg0, int *arg1, int *arg2) { abort(); }
char * msg_str(int arg0) { abort(); }
struct NaviLink * navi_link_construct_with(struct NaviLink * arg0, int arg1, int arg2, const char * arg3, const char * arg4, int arg5, int arg6, struct NaviMap * arg7, struct NaviMap * arg8, int arg9, int arg10) { abort(); }
const char * navi_link_dest_map_name(struct NaviLink * arg0) { abort(); }
struct NaviPoint * navi_link_dest_pos(struct NaviLink * arg0, struct NaviPoint * arg1) { abort(); }
void navi_link_set_depth(struct NaviLink * arg0, int arg1) { abort(); }
struct NaviManager * navi_manager_base_construct(struct NaviManager * arg0) { abort(); }
void navi_manager_base_destruct(struct NaviManager * arg0) { abort(); }
unsigned char navi_manager_bind_world(struct NaviManager * arg0) { abort(); }
void navi_manager_clear_groups(struct NaviManager * arg0) { abort(); }
void navi_manager_clear_info_lines(struct NaviManager * arg0) { abort(); }
void navi_manager_clear_target(struct NaviManager * arg0) { abort(); }
unsigned char navi_manager_fill_from_passes(struct NaviManager * arg0) { abort(); }
unsigned char navi_manager_fill_target_passes(struct NaviManager * arg0, int arg1, int arg2) { abort(); }
unsigned char navi_manager_find_path(struct NaviManager * arg0, int arg1, int arg2, int arg3, int arg4, unsigned char arg5) { abort(); }
int navi_manager_find_route(struct NaviManager * arg0, struct Vec * arg1, unsigned char arg2, unsigned char arg3, unsigned char arg4, unsigned char arg5, int arg6) { abort(); }
void navi_manager_load_grid_images(struct NaviManager * arg0) { abort(); }
struct NaviMap * navi_manager_map_by_name(struct NaviManager * arg0, const char * arg1) { abort(); }
void navi_manager_report_status(struct NaviManager * arg0, int arg1, int arg2) { abort(); }
struct NaviMap * navi_manager_target_map(struct NaviManager * arg0) { abort(); }
struct NaviPoint * navi_manager_target_pos(struct NaviManager * arg0, struct NaviPoint * arg1) { abort(); }
struct NaviLink *** navi_map_links_begin(struct NaviMap * arg0, struct NaviLink *** arg1) { abort(); }
struct NaviLink *** navi_map_links_end(struct NaviMap * arg0, struct NaviLink *** arg1) { abort(); }
struct NaviObject * NaviMap_MobByName(struct NaviMap * arg0, const char * arg1) { abort(); }
const char * navi_map_name(struct NaviMap * arg0) { abort(); }
struct NaviObject * navi_map_npc_at(struct NaviMap * arg0, int arg1, int arg2) { abort(); }
char *res_mgr_resolve_map_name(struct ResMgr *mgr, const char *name) { abort(); }
void ui_navigation_v4_wnd_relayout(struct UINavigationV4Wnd * arg0) { abort(); }
int ui_window_mgr_send_msg_(struct UIWindowMgr * arg0, int arg1, int arg2, int arg3, int arg4, int arg5) { abort(); }
float float_sqrt(float arg0) { abort(); }
struct ResMgr * g_resMgr(void) { abort(); }
struct GameMode * game_current_mode(void) { abort(); }
unsigned char navi_result_name_greater(struct SearchResult * arg0, const struct SearchResult * arg1) { abort(); }
unsigned int number_truncate_low32(double arg0) { abort(); }
void std_sort(void * arg0, void * arg1, unsigned int arg2, unsigned char (*arg3)(const void *, const void *, void *), void * arg4) { abort(); }
void std_vector_bytes_resize_fill(struct Vec * arg0, unsigned int arg1, const char * arg3) { abort(); }
struct UIWindow * ui_window_open(int arg0) { abort(); }

struct NaviMap *navi_manager_current_map(struct NaviManager *manager) { (void)manager; abort(); }
