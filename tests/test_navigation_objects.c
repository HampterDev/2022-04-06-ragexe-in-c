#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "core/fatal.h"

static struct Allocation { unsigned char *raw; size_t size; } blocks[256];
static int live, record_attempts, fail_record, depth_resets;
static void *allocate(size_t size)
{
    int i;
    for (i = 0; i < 256 && blocks[i].raw; ++i) {}
    assert(i < 256 && size <= SIZE_MAX - 16);
    blocks[i].raw = malloc(size + 16); assert(blocks[i].raw);
    blocks[i].size = size; ++live;
    memset(blocks[i].raw, 0xa5, size + 16);
    return blocks[i].raw + 8;
}
static void release(void *ptr)
{
    int i, j;
    if (!ptr) return;
    for (i = 0; i < 256 && (!blocks[i].raw || blocks[i].raw + 8 != ptr); ++i) {}
    assert(i < 256);
    for (j = 0; j < 8; ++j) {
        assert(blocks[i].raw[j] == 0xa5);
        assert(blocks[i].raw[8 + blocks[i].size + j] == 0xa5);
    }
    free(blocks[i].raw); blocks[i].raw = NULL; --live;
}
static void *allocate_record(size_t size)
{
    if (++record_attempts == fail_record) return NULL;
    return allocate(size);
}
void *mem_alloc(size_t size) { return allocate(size); }
void mem_free(void *ptr)
{  release(ptr); }
__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }

struct NaviManager g_navi;
struct Session g_session1;
HWND g_hMainWnd;
void navi_manager_reset_link_depths(struct NaviManager *manager)
{ assert(manager == &g_navi); ++depth_resets; }

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...)
{
    va_list args;
    int row, i;
    const char *outputs = strchr(format, '>') + 1;
    const char *strings[4] = {0};
    int numbers[8] = {0};
    int ns = 0, ni = 0;
    assert(lua == (struct ClientLua *)&g_navi);
    va_start(args, format); row = va_arg(args, int);
    if (!strcmp(function, "queryNavi_MapInfo") && row == 1) {
        strings[0] = "prontera"; strings[1] = "Prontera city";
        numbers[0] = NAVI_MAP_TYPE_DEFAULT; numbers[1] = 300; numbers[2] = 400;
    } else if ((!strcmp(function, "queryNavi_NpcInfo") || !strcmp(function, "queryNavi_MobInfo")) && row == 1) {
        strings[0] = "prontera"; strings[1] = "An owned navigation object name"; strings[2] = "An owned navigation alias";
        numbers[0] = 7; numbers[1] = !strcmp(function, "queryNavi_NpcInfo") ? NAVI_TYPE_NPC : NAVI_TYPE_MOB;
        numbers[2] = 0x12345678; numbers[3] = 20; numbers[4] = 30;
    } else if (!strcmp(function, "queryNavi_LinkInfo") && row == 1) {
        strings[0] = strings[3] = "prontera"; strings[1] = "A navigation link with owned text"; strings[2] = "Link alias";
        numbers[0] = 9; numbers[1] = NAVI_TYPE_LINK_NPC_SCRIPT; numbers[2] = 321;
        numbers[3] = 40; numbers[4] = 50; numbers[5] = 60; numbers[6] = 70;
    } else if (!strcmp(function, "queryNavi_Scroll")) {
        strings[0] = row == 1 ? "A warp scroll with owned text" : "NULL";
        strings[1] = row == 1 ? "prontera" : "NULL";
        numbers[0] = 11; numbers[1] = 654; numbers[2] = 80; numbers[3] = 90;
    } else if (!strcmp(function, "queryNavi_Scroll_Pass")) {
        (void)va_arg(args, int);
    } else {
        assert(!strcmp(function, "queryNavi_MapInfo") || !strcmp(function, "queryNavi_NpcInfo") ||
               !strcmp(function, "queryNavi_MobInfo") || !strcmp(function, "queryNavi_LinkInfo") ||
               !strcmp(function, "queryNavi_PickNpc") || !strcmp(function, "queryNavi_Distance_Map") ||
               !strcmp(function, "queryNavi_NpcDistance_Map"));
    }
    for (i = 0; outputs[i]; ++i) {
        if (outputs[i] == 's') { assert(ns < 4); *va_arg(args, const char **) = strings[ns++]; }
        else { assert(outputs[i] == 'd'); *va_arg(args, int *) = ni < 8 ? numbers[ni++] : 0; }
    }
    va_end(args);
    return 1;
}

#define malloc allocate_record
#define free release
#include "../src/navi/navi_objects.c"
#include "../src/navi/navi_links.c"
#include "../src/navi/navi_maps.c"
#include "../src/navi/navi_base.c"
#include "../src/navi/navi_lua_tables.c"
#undef malloc
#undef free
#include "../src/navi/navi_accessors.c"
#include "../src/navi/navi_object_queries.c"

static void object_queries_and_copies(void)
{
    struct NaviMap source, destination;
    struct NaviObject entity;
    struct NaviLink link;
    struct NaviPass pass;
    struct NaviPoint point;
    int i;
    navi_map_construct(&source, 1, "prontera", "Source", 100, 100, NAVI_MAP_TYPE_DEFAULT);
    navi_map_construct(&destination, 2, "izlude", "Destination", 100, 100, NAVI_MAP_TYPE_DEFAULT);
    navi_object_construct_with(&entity, NAVI_TYPE_MOB, 0x89abcdef, "A long entity name to copy", "A long entity alias to copy", 10, 20, &source);
    assert(entity.kind == NAVI_OBJECT_ENTITY && entity.id == -1);
    assert(navi_object_type(&entity) == NAVI_TYPE_MOB);
    assert(navi_object_sprite(&entity) == 0xcdef && navi_object_sprite_high(&entity) == 0x89ab);
    entity.type = NAVI_TYPE_MOB_BOSS; assert(navi_object_sprite(&entity) == 0xcdef);
    entity.type = NAVI_TYPE_NPC; assert((unsigned)navi_object_sprite(&entity) == 0x89abcdef);
    assert(navi_object_position(&entity, &point) == &point && point.x == 10 && point.y == 20);
    assert(navi_object_map(&entity) == &source);
    navi_link_construct_with(&link, NAVI_TYPE_MOB, 0x89abcdef, "A long link name to copy", "A long link alias to copy", 30, 40, &source, &destination, 50, 60);
    assert(link.base.kind == NAVI_OBJECT_LINK && (unsigned)navi_object_sprite(&link.base) == 0x89abcdef);
    pass.link = &link; pass.distance = 123;
    for (i = 0; i < 40; ++i) { vec_push(&link.base.passes, &pass, sizeof pass); navi_link_add_exit(&link, &pass); }
    vec_clear(&link.base.passes);
    assert(link.base.passes.first == link.base.passes.last && link.base.passes.first != NULL);
    navi_link_destruct(&link); navi_object_destruct(&entity);
    navi_map_destruct(&source); navi_map_destruct(&destination);
    navi_object_free(NULL); assert(live == 0);
}

static void catalog_ownership(void)
{
    int failure;
    memcpy(&g_session1.mapName, "prontera", 9);
    for (failure = 0; failure <= 5; ++failure) {
        struct NaviMap *map;
        struct NaviLink *scroll, *link;
        struct NaviPass pass;
        memset(&g_navi, 0, sizeof g_navi);
        navi_manager_base_construct(&g_navi);
        record_attempts = depth_resets = 0; fail_record = failure;
        assert(navi_manager_load_tables(&g_navi, &g_navi) == !failure);
        if (failure) {
            assert(record_attempts == failure && depth_resets == 0);
            assert(!g_navi.currentMap && g_navi.base.maps.first == g_navi.base.maps.last && g_navi.base.links.first == g_navi.base.links.last);
            assert(navi_manager_current_map(&g_navi) == NULL);
        } else {
            assert(record_attempts == 5 && depth_resets == 1);
            map = navi_manager_map_by_name(&g_navi, "prontera"); assert(map == g_navi.currentMap);
            assert(navi_map_npc(map, 7, 1)->id == 7 && navi_map_mob_by_sprite(map, 0x5678));
            link = navi_map_link(map, 9, 1); scroll = navi_manager_scroll_link(&g_navi, 11, 1);
            assert(link && scroll && link->base.kind == NAVI_OBJECT_LINK && scroll->base.kind == NAVI_OBJECT_LINK);
            assert(navi_object_map(&scroll->base) == map);
            memcpy(&g_session1.mapName, "unknown", 8); assert(!navi_object_map(&scroll->base));
            memcpy(&g_session1.mapName, "prontera", 9); assert(navi_object_map(&scroll->base) == map);
            pass.link = link; pass.distance = 10;
            vec_push(&scroll->base.passes, &pass, sizeof pass); navi_link_add_exit(scroll, &pass);
            navi_manager_clear(&g_navi); assert(!g_navi.currentMap);
            record_attempts = 0; assert(navi_manager_load_tables(&g_navi, &g_navi));
        }
        navi_manager_base_destruct(&g_navi); navi_manager_base_destruct(&g_navi);
        assert(live == 0);
    }
}

int main(void)
{
    object_queries_and_copies();
    catalog_ownership();
    puts("Navigation objects: typed queries, sprite words, borrowed passes, warp maps, C ownership, reload and all five record allocation failures passed");
    return 0;
}
