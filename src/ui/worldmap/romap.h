#include "std/string.h"
#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "render/texture_types.h"
#include "resources/animation.h"
#include "session/character_name.h"
#include "ui/worldmap/route_view.h"
#include "navi/navi_manager.h"
#include "session/map_names.h"
#include "ui/window_manager_state.h"
#ifndef UI_WORLDMAP_ROMAP_H
#define UI_WORLDMAP_ROMAP_H

#include "render/math3d.h"
#include "std/list.h"

#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include "ui/quests/world_view.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "ui/render/render.h"

#include "ui/worldmap/kafra_services.h"

struct NaviManager;
struct NaviMap;
#define WND_NAVIGATION 0xcb
#define WND_ROMAP 0x8c

struct RoMapEntry {
    int region;
    int kind;
    struct WorldViewRect place_rect;
    struct String name;
    struct WorldViewRect rect;
    struct List members;
};

struct RouteStop {
    struct String name;
    struct WorldViewRect rect;
    float cx, cy;
    float width, height;
};
struct KafraMoveMapServiceInfo {
    struct Vec stops;
    struct RouteStop origin;
    struct String map;
    float progress;
};
struct FlyingShipRouteInfo {
    struct Vec stops;
    int leg;
    float leg_t;
    float leg_length;
    float travelled;
    BYTE at_start;
    int route;
    DWORD colour;
    BYTE active;
    BYTE loaded;
    struct String bitmap;
};
#define FLYING_ROUTE_IZLUDE 0
#define FLYING_ROUTE_EINBROCH 1

struct RoMapBox {
    int a, b, c, d;
    int flag;
};

struct UIRoMapWnd {
    struct UIFrameWnd frame;
    int origin_x, origin_y;
    struct List entries;
    struct RoMapEntry *hover;
    struct RoMapEntry *spot;
    struct RoMapBox *members_box;
    struct RoMapBox *minimap_box;
    BYTE hover_shown;
    void *minimap;
    int minimap_state;
    BYTE show_dungeons;
    DWORD frame_time;
    BYTE first_frame;
    struct KafraMoveMapServiceInfo kafra;
    struct FlyingShipRouteInfo ship[2];
    int mouse_x, mouse_y;
    int combo_height;
    struct UIWindow *combo;
    struct UIWindow *dungeon_check;
    struct Vec results;
    int place_index;
    struct UIBitmapButton *search;
    struct UIBitmapButton *navi;
    struct UIWindow *edit;
    RECT kafra_rect;
};
OFFSET_ASSERT(UIRoMapWnd, kafra, 0xcc);

#define ROMAP_ID_DUNGEONS 0xb8
#define ROMAP_ID_NAVI 0xc0
#define ROMAP_ID_SEARCH 0x139
#define ROMAP_MSG_SELECT 0x27
#define ROMAP_MSG_RESET 0x3c

void route_stops_clear(struct Vec *v);
void route_stops_free(struct Vec *v);

#define ROUTE_PRIM_LIST 0x201

void kafra_move_map_service_info_update(struct KafraMoveMapServiceInfo *self, struct UIRoMapWnd *wnd, float seconds);
void kafra_move_map_service_info_draw(struct KafraMoveMapServiceInfo *self, struct UIRoMapWnd *wnd);
void ui_ro_map_wnd_process_ship(struct UIRoMapWnd *self, struct FlyingShipRouteInfo *ship, float seconds);
void flying_ship_route_info_stop(struct FlyingShipRouteInfo *self);
void flying_ship_route_info_advance(struct FlyingShipRouteInfo *self, struct UIRoMapWnd *wnd, float seconds);
void ui_ro_map_wnd_draw_level_marks(void);
void ui_ro_map_wnd_draw_player_mark(void);
void ui_ro_map_wnd_draw_minimap(struct UIRoMapWnd *self);
void ui_ro_map_wnd_draw_spot(struct UIRoMapWnd *self);
float ui_ro_map_wnd_frame_seconds(struct UIRoMapWnd *self);
void ui_ro_map_wnd_render_overlay(struct UIRoMapWnd *self);

struct UIRoMapWnd *ui_ro_map_wnd_construct(struct UIRoMapWnd *self);
struct UIWindow *ui_ro_map_wnd_new(void);

#endif
