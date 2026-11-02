#include "world/ground_api.h"
#include "world/pathfinder_state.h"
#include "world/path_info.h"
#ifndef NAVI_H
#define NAVI_H

#include "base.h"

#include <string.h>

#define NAVI_TYPE_NPC           0x65
#define NAVI_TYPE_MOB           0x12c
#define NAVI_TYPE_MOB_BOSS      0x12d
#define NAVI_TYPE_LINK_FIRST    0xc8
#define NAVI_TYPE_LINK_NPC_SCRIPT           0xc9
#define NAVI_TYPE_LINK_KAFRA_DUNGEON           0xca
#define NAVI_TYPE_LINK_COOL_EVENT_DUNGEON           0xcb
#define NAVI_TYPE_LINK_NPC_WARPER           0xcc
#define NAVI_TYPE_LINK_AIRPORT           0xcd
#define NAVI_TYPE_WARP_SCROLL   0x190

#define NAVI_MAP_TYPE_DEFAULT   0x1389
#define NAVI_MAP_TYPE_SCROLL    0x138c

struct NaviMap;
struct NaviLink;
struct Texture;

struct NaviPoint {
    int x, y;
};

struct NaviPass {
    struct NaviLink *link;
    int distance;
};

enum NaviObjectKind {
    NAVI_OBJECT_ENTITY,
    NAVI_OBJECT_LINK
};

struct NaviObject {
    enum NaviObjectKind kind;
    int id;
    int type;
    int sprite;
    struct String name;
    struct String alias;
    struct NaviMap *map;
    int x, y;
    struct Vec passes;
    int pickNpc[9];
};

struct NaviLink {
    struct NaviObject base;
    struct NaviMap *destMap;
    int destX, destY;
    int depth;
    struct Vec exits;
};

struct NaviMap {
    int id;
    int mapType;
    struct String name;
    struct String displayName;
    int width;
    int height;
    unsigned char visited;
    struct Vec npcs;
    struct Vec mobs;
    struct Vec links;
};
OFFSET_ASSERT(NaviMap, name, 0x08);

struct NaviRouteNode {
    int prev;
    struct NaviLink *link;
    int distance;
};

struct NaviManagerBase {
    struct Vec maps;
    struct Vec links;
    struct NaviMap *targetMap;
    int targetX, targetY;
    int bestDistance;
    struct Vec routeNodes;
    struct Vec fromPasses;
    struct Vec targetPasses;
};

#define NAVI_RESULT_PLACE   0
#define NAVI_RESULT_MAP     1
#define NAVI_RESULT_NPC     2
#define NAVI_RESULT_MOB     3
#define NAVI_RESULT_ROUTE_LINK 6
#define NAVI_SEARCH_ALL     0

#define NAVI_POS_UNDER_MAP  (-1000)
#define NAVI_SPRITE_NPC     10000

#define NAVI_PICTURE_MAP    1
#define NAVI_PICTURE_SPRITE 2

#define NAVI_GUIDE_OFF      0
#define NAVI_GUIDE_IDLE     1
#define NAVI_GUIDE_ACTIVE   2

struct SearchResult {
    int kind;
    void *obj;
    int x, y;
    struct String name;
};

static __inline void search_result_init(struct SearchResult *r, int kind)
{
    string_init(&r->name);
    r->kind = kind;
    r->obj = NULL;
    r->x = -1;
    r->y = -1;
}

static __inline void search_result_destroy(struct SearchResult *r)
{
    r->kind = -1;
    r->obj = NULL;
    r->x = -1;
    r->y = -1;
    string_clear(&r->name);
    string_free(&r->name);
}

struct NaviManager {
    struct NaviManagerBase base;
    struct PathFinder pathFinder;
    struct Vec results;
    struct SearchResult guided;
    struct SearchResult destination;
    int walkDistance;
    struct Vec routeLinks;
    unsigned char arriveNearTarget;
    int routeStep;
    struct PathInfo path;
    unsigned char showPath;
    struct String gridImages[8];
    struct C3dGround15 *ground;
    struct NaviMap *currentMap;
    unsigned char windowHidden;
    int window_x, window_y;
    unsigned char windowCollapsed;
    int guideState;
    int windowPage;
    int searchFilter;
    struct String searchText;
    unsigned char routeUseKafra;
    unsigned char routeUseScrolls;
    unsigned char routeUseAirship;
    int routePrNpcState;
    int pictureKind;
    struct Texture *picture;
    int minimapX, minimapY;
    struct Vec pathDots;
    void *actRes;
    void *sprRes;
    struct Vec infoLines;
    unsigned char showMarker;
    struct Vec resultGroups;
    int selectedGroup;
    int selectedMember;
    int roadIcon;
    struct String texts[5];
};
extern struct NaviManager g_navi;

void navi_object_free(struct NaviObject *object);
int navi_object_type(const struct NaviObject *object);
int navi_object_sprite(const struct NaviObject *object);
int navi_object_sprite_high(const struct NaviObject *object);
const char *navi_object_name(const struct NaviObject *object);
const char *navi_object_alias(const struct NaviObject *object);
struct NaviPoint *navi_object_position(const struct NaviObject *object, struct NaviPoint *out);
struct NaviMap *navi_object_map(const struct NaviObject *object);

struct NaviObject *navi_object_construct(struct NaviObject *self);
struct NaviObject *navi_object_construct_with(struct NaviObject *self,
        int type, int sprite, const char *name, const char *alias, int x, int y, struct NaviMap *map);
void navi_object_destruct(struct NaviObject *self);
unsigned char navi_object_id_is(struct NaviObject *self, const int *id);

void navi_string_assign_cstr(struct String *s, const char *text);

struct NaviLink *navi_link_construct_with(struct NaviLink *self,
        int type, int sprite, const char *name, const char *alias, int x, int y, struct NaviMap *map,
        struct NaviMap *destMap, int destX, int destY);
void navi_link_destruct(struct NaviLink *self);
struct NaviMap *navi_link_dest_map(struct NaviLink *self);
const char *navi_link_dest_map_name(struct NaviLink *self);
struct NaviPoint *navi_link_dest_pos(struct NaviLink *self, struct NaviPoint *out);
void navi_link_set_depth(struct NaviLink *self, int depth);
int navi_link_depth(struct NaviLink *self);
void navi_link_add_exit(struct NaviLink *self, const struct NaviPass *exit);
struct NaviPass **navi_link_exits_begin(struct NaviLink *self, struct NaviPass **out);
struct NaviPass **navi_link_exits_end(struct NaviLink *self, struct NaviPass **out);
void navi_link_clear_exits(struct NaviLink *self);

struct NaviMap *navi_map_construct(struct NaviMap *self, int id,
        const char *name, const char *displayName, int width, int height, int mapType);
void navi_map_destruct(struct NaviMap *self);
const char *navi_map_name(struct NaviMap *self);
const char *navi_map_display_name(struct NaviMap *self);
unsigned char navi_map_name_equals(struct NaviMap *self, const struct String *name);
unsigned char navi_map_add_npc(struct NaviMap *self, struct NaviObject *npc);
unsigned char navi_map_add_mob(struct NaviMap *self, struct NaviObject *mob);
unsigned char navi_map_add_link(struct NaviMap *self, struct NaviLink *link);
struct NaviObject *navi_map_npc(struct NaviMap *self, int key, unsigned char byId);
struct NaviLink *navi_map_link(struct NaviMap *self, int key, unsigned char byId);
struct NaviObject *navi_map_npc_at(struct NaviMap *self, int x, int y);
struct NaviObject *navi_map_mob_by_sprite(struct NaviMap *self, int sprite);

int navi_map_map_type(struct NaviMap *self);
int navi_map_width(struct NaviMap *self);
int navi_map_height(struct NaviMap *self);
unsigned char navi_map_visited(struct NaviMap *self);
void navi_map_set_visited(struct NaviMap *self, unsigned char visited);
struct NaviLink ***navi_map_links_begin(struct NaviMap *self, struct NaviLink ***out);
struct NaviLink ***navi_map_links_end(struct NaviMap *self, struct NaviLink ***out);
int navi_map_link_count(struct NaviMap *self);

struct NaviMap *navi_manager_current_map(struct NaviManager *self);

void navi_manager_clear(struct NaviManager *self);
unsigned char navi_manager_add_map(struct NaviManager *self,
        struct NaviMap *map);
unsigned char navi_manager_add_scroll_link(struct NaviManager *self,
        struct NaviLink *link);
struct NaviMap *navi_manager_map_by_name(struct NaviManager *self,
        const char *name);
struct NaviLink *navi_manager_scroll_link(struct NaviManager *self,
        int key, unsigned char byId);

unsigned char navi_manager_set_target(struct NaviManager *self,
        struct NaviMap *map, int x, int y);
int navi_manager_map_count(struct NaviManager *self);
void navi_manager_reset_link_depths(struct NaviManager *self);

unsigned char navi_manager_links_into(struct NaviManager *self,
        struct NaviMap *destMap, struct Vec *out);
int navi_manager_find_route(struct NaviManager *self, struct Vec *path,
        unsigned char arriveIsEnough, unsigned char useKafra, unsigned char useScrolls,
        unsigned char useAirship, int prNpcState);

unsigned char navi_manager_load_tables(struct NaviManager *self, void *lua);

#endif
