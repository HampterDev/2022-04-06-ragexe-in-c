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
#include <stdlib.h>

int navi_manager_map_count(struct NaviManager *manager)
{ return (int)VEC_COUNT(&manager->base.maps, void *); }
struct GameMode *game_current_mode(void) { abort(); }
struct ResMgr *g_resMgr(void) { abort(); }
char *res_mgr_resolve_map_name(struct ResMgr *manager, const char *name) { (void)manager; (void)name; abort(); }
struct NaviMap *navi_manager_result_map(const struct SearchResult *result)
{  (void)result; abort(); }
void game_mode_get_player_cell(struct GameMode *mode, int *x, int *y) { (void)mode; (void)x; (void)y; abort(); }
void std_sort(void *a, void *b, unsigned size, unsigned char (*less)(const void *, const void *, void *), void *context)
{ (void)a; (void)b; (void)size; (void)less; (void)context; abort(); }
unsigned char navi_result_name_greater(struct SearchResult *a, const struct SearchResult *b) { (void)a; (void)b; abort(); }
