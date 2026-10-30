#include "navi/navi_manager.h"
#include "navi/navi_search.h"
#include "std/vec.h"
#include <stdlib.h>

struct NaviMap *navi_link_dest_map(struct NaviLink *link) { (void)link; abort(); }
const char *navi_link_dest_map_name(struct NaviLink *link) { (void)link; abort(); }
const char *navi_map_display_name(struct NaviMap *map) { (void)map; abort(); }
struct NaviObject *navi_map_npc_at(struct NaviMap *map, int x, int y)
{ (void)map; (void)x; (void)y; abort(); }
struct NaviObject *navi_map_mob_by_sprite(struct NaviMap *map, int sprite)
{ (void)map; (void)sprite; abort(); }
void navi_manager_build_groups(struct NaviManager *manager) { (void)manager; abort(); }
struct NaviPoint *navi_manager_target_pos(struct NaviManager *manager, struct NaviPoint *out)
{ (void)manager; (void)out; abort(); }
struct NaviMap *navi_manager_target_map(struct NaviManager *manager) { (void)manager; abort(); }
void navi_results_sort(struct SearchResult *first, struct SearchResult *last)
{ (void)first; (void)last; abort(); }
void search_results_push_copy(struct Vec *value, const struct SearchResult *result)
{ (void)value; (void)result; abort(); }

struct NaviMap *navi_manager_current_map(struct NaviManager *manager) { (void)manager; abort(); }
