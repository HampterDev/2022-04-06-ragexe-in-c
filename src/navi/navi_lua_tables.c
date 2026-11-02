#include "contents/client_lua_calls.h"
#include "navi/navi.h"
#include "game/cells.h"
#include "render/texture_cache.h"
#include "std/string.h"
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
#include <string.h>
#include "navi/navi_manager.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "core/app.h"
#include "platform/window.h"

#define NAVI_MAX_MAPS       0x270f
#define NAVI_MAX_OBJECTS    0x1869f
#define NAVI_MAX_DISTANCE   0xf423f
#define NAVI_MAX_SCROLLS    0xc8

static void error_box(char *text, const char *title)
{
    platform_message_box(g_hMainWnd, text, title, 0);
}

unsigned char navi_manager_load_tables(struct NaviManager *self, void *lua)
{
    char message[0x104];
    char scrollMessage[0x104];
    const char *mapName, *name, *alias, *destName, *kind;
    int id, type, sprite, x, y, destX, destY, count, linkId, distance;
    int pick[9];
    int i, j, k;

    memset(message, 0, sizeof(message));
    navi_manager_clear(self);
    x = 0;
    y = 0;
    destX = 0;
    destY = 0;
    linkId = -1;
    distance = 0;

    for (i = 1; i < NAVI_MAX_MAPS; i++) {
        struct NaviMap *map, *p;

        mapName = NULL;
        name = NULL;
        if (!client_lua_call(lua, "queryNavi_MapInfo", "d>ssddd",
                               i, &mapName, &name, &id, &x, &y)) {
            sprintf(message, "Map 루아 읽기 실패 %d", i);
            error_box(message, "Error");
            break;
        }
        if (!mapName)
            break;
        p = malloc(sizeof *p);
        if (!p) goto allocation_failed;
        map = navi_map_construct(p, navi_manager_map_count(self), mapName, name, x, y, id);
        navi_manager_add_map(self, map);
    }

    for (i = 1; i < NAVI_MAX_OBJECTS; i++) {
        struct NaviMap *map;
        struct NaviObject *npc, *p;

        type = 100;
        alias = NULL;
        name = NULL;
        mapName = NULL;
        y = 0;
        x = 0;
        sprite = 0;
        if (!client_lua_call(lua, "queryNavi_NpcInfo", "d>sdddssdd",
                               i, &mapName, &id, &type, &sprite, &name, &alias, &x, &y)) {
            sprintf(message, "Npc 루아 읽기 실패 %d", i);
            error_box(message, "Error");
            break;
        }
        if (!mapName)
            break;
        map = navi_manager_map_by_name(self, mapName);
        if (!map)
            continue;
        p = malloc(sizeof *p);
        if (!p) goto allocation_failed;
        npc = navi_object_construct_with(p, type, sprite, name, alias, x, y, map);
        npc->id = id;
        navi_map_add_npc(map, npc);
    }

    for (i = 1; i < NAVI_MAX_OBJECTS; i++) {
        struct NaviMap *map;
        struct NaviObject *mob, *p;

        type = 100;
        alias = NULL;
        name = NULL;
        mapName = NULL;
        y = 0;
        x = 0;
        sprite = 0;
        if (!client_lua_call(lua, "queryNavi_MobInfo", "d>sdddssdd",
                               i, &mapName, &id, &type, &sprite, &name, &alias, &x, &y)) {
            sprintf(message, "몬스터 루아 읽기 실패 %d", i);
            error_box(message, "Error");
            break;
        }
        if (!mapName)
            break;
        map = navi_manager_map_by_name(self, mapName);
        if (!map)
            continue;
        p = malloc(sizeof *p);
        if (!p) goto allocation_failed;
        mob = navi_object_construct_with(p, type, sprite, name, alias, x, y, map);
        mob->id = id;
        navi_map_add_mob(map, mob);
    }

    for (i = 1; i < NAVI_MAX_OBJECTS; i++) {
        struct NaviMap *map, *destMap;
        struct NaviLink *link, *p;

        type = 100;
        destName = NULL;
        alias = NULL;
        name = NULL;
        mapName = NULL;
        destY = 0;
        destX = 0;
        y = 0;
        x = 0;
        sprite = 0;
        if (!client_lua_call(lua, "queryNavi_LinkInfo", "d>sdddssddsdd",
                               i, &mapName, &id, &type, &sprite, &name, &alias, &x, &y,
                               &destName, &destX, &destY)) {
            sprintf(message, "Link 루아 읽기 실패 %d", i);
            error_box(message, "Error");
            break;
        }
        if (!mapName)
            break;
        map = navi_manager_map_by_name(self, mapName);
        destMap = navi_manager_map_by_name(self, destName);
        if (!map || !destMap)
            continue;
        p = malloc(sizeof *p);
        if (!p) goto allocation_failed;
        link = navi_link_construct_with(p, type, sprite, name, alias, x, y, map,
                                          destMap, destX, destY);
        link->base.id = id;
        navi_map_add_link(map, link);
    }

    memset(pick, 0, sizeof(pick));
    for (i = 1; i < NAVI_MAX_MAPS; i++) {
        struct NaviMap *map;
        struct NaviObject *npc;

        mapName = NULL;
        y = 0;
        x = 0;
        sprite = 0;
        if (!client_lua_call(lua, "queryNavi_PickNpc", "d>sddddddddddd",
                               i, &mapName, &x, &y, &pick[0], &pick[1], &pick[2], &pick[3],
                               &pick[4], &pick[5], &pick[6], &pick[7], &pick[8])) {
            snprintf(message, 0x104, "Failed to read Link lua %d", i);
            error_box(message, "Error queryNavi_PickNpc ");
            break;
        }
        if (!mapName)
            break;
        map = navi_manager_map_by_name(self, mapName);
        if (!map)
            continue;
        npc = navi_map_npc_at(map, x, y);
        if (!npc)
            continue;
        memcpy(npc->pickNpc, pick, sizeof(pick));
    }

    for (i = 1; i < NAVI_MAX_DISTANCE; i += 3) {
        struct NaviMap *map;

        mapName = NULL;
        count = 0;
        if (!client_lua_call(lua, "queryNavi_Distance_Map", "d>sd",
                               i, &mapName, &count)) {
            sprintf(message, "워프 길 정보 읽기 실패 %d", i);
            error_box(message, "Error");
            break;
        }
        if (!mapName)
            break;
        map = navi_manager_map_by_name(self, mapName);
        if (!map)
            continue;
        for (j = 1; j < count + 1; j++) {
            struct NaviLink *link;
            int linkIdOfMap = -1;

            if (!client_lua_call(lua, "queryNavi_Distance_Link", "dd>d",
                                   i, j, &linkIdOfMap))
                continue;
            link = navi_map_link(map, linkIdOfMap, 1);
            if (!link)
                continue;
            for (k = 1; ; k++) {
                struct NaviLink *to;
                struct NaviPass pass;

                kind = NULL;
                linkId = -1;
                distance = 0;
                if (!client_lua_call(lua, "queryNavi_Distance_Pass", "ddd>sdd",
                                       i, j, k, &kind, &linkId, &distance))
                    continue;
                if (strcmp("NULL", kind) == 0)
                    break;
                if (strcmp(kind, "P") == 0) {
                    to = navi_map_link(map, linkId, 1);
                    if (to) {
                        pass.link = to;
                        pass.distance = distance;
                        vec_push(&link->base.passes, &pass, sizeof pass);
                    }
                } else if (strcmp(kind, "E") == 0) {
                    to = navi_map_link(navi_link_dest_map(link), linkId, 1);
                    if (to) {
                        pass.link = to;
                        pass.distance = distance;
                        navi_link_add_exit(link, &pass);
                    }
                }
            }
        }
    }

    for (i = 1; i < NAVI_MAX_DISTANCE; i += 3) {
        struct NaviMap *map;

        mapName = NULL;
        count = 0;
        if (!client_lua_call(lua, "queryNavi_NpcDistance_Map", "d>sd",
                               i, &mapName, &count)) {
            sprintf(message, "Npc 워프 길 정보 읽기 실패 %d", i);
            error_box(message, "Error");
            break;
        }
        if (!mapName)
            break;
        map = navi_manager_map_by_name(self, mapName);
        if (!map)
            continue;
        for (j = 1; j < count + 1; j++) {
            struct NaviObject *npc;
            int npcId = -1;

            if (!client_lua_call(lua, "queryNavi_NpcDistance_Link", "dd>d",
                                   i, j, &npcId))
                continue;
            npc = navi_map_npc(map, npcId, 1);
            if (!npc)
                continue;
            for (k = 1; ; k++) {
                struct NaviMap *linkMap;
                struct NaviLink *to;
                struct NaviPass pass;

                kind = NULL;
                linkId = -1;
                distance = 0;
                if (!client_lua_call(lua, "queryNavi_NpcDistance_Pass", "ddd>sdd",
                                       i, j, k, &kind, &linkId, &distance))
                    continue;
                if (linkId == 0 && distance == 0)
                    break;
                linkMap = navi_manager_map_by_name(self, kind);
                if (!linkMap)
                    to = navi_manager_scroll_link(self, linkId, 1);
                else
                    to = navi_map_link(linkMap, linkId, 1);
                if (to) {
                    pass.link = to;
                    pass.distance = distance;
                    vec_push(&npc->passes, &pass, sizeof pass);
                }
            }
        }
    }

    for (i = 1; i < NAVI_MAX_SCROLLS; i++) {
        struct NaviMap *destMap;
        struct NaviLink *link, *p;

        destName = NULL;
        name = NULL;
        destY = 0;
        destX = 0;
        sprite = 0;
        id = 0;
        if (!client_lua_call(lua, "queryNavi_Scroll", "d>sddsdd",
                               i, &name, &id, &sprite, &destName, &destX, &destY)) {
            memset(scrollMessage, 0, sizeof(scrollMessage));
            sprintf(scrollMessage, "Scroll 정보 읽기 실패 %d", i);
            error_box(scrollMessage, "Error");
            break;
        }
        if (strcmp("NULL", name) == 0 && strcmp("NULL", destName) == 0)
            break;
        destMap = navi_manager_map_by_name(self, destName);
        if (!destMap)
            continue;
        p = malloc(sizeof *p);
        if (!p) goto allocation_failed;
        link = navi_link_construct_with(p, NAVI_TYPE_WARP_SCROLL, sprite, name, NULL, 0, 0, NULL,
                                          destMap, destX, destY);
        link->base.id = id;
        navi_manager_add_scroll_link(self, link);
        navi_link_clear_exits(link);
        for (k = 1; ; k++) {
            struct NaviLink *to;
            struct NaviPass pass;

            linkId = -1;
            distance = 0;
            if (!client_lua_call(lua, "queryNavi_Scroll_Pass", "dd>dd",
                                   i, k, &linkId, &distance))
                continue;
            if (linkId == 0 && distance == 0)
                break;
            to = navi_map_link(destMap, linkId, 1);
            if (!to)
                continue;
            pass.link = to;
            pass.distance = distance;
            navi_link_add_exit(link, &pass);
        }
    }

    self->currentMap = navi_manager_map_by_name(self, "prontera");
    navi_manager_reset_link_depths(self);
    return 1;

allocation_failed:
    navi_manager_clear(self);
    return 0;
}
