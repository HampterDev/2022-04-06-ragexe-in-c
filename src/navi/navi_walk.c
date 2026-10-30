#include "render/texture_cache.h"
#include "world/ground_api.h"
#include "navi/grid_uv.h"

#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "navi/navi_manager.h"
#include "game/cells.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "navi/navi_search.h"
#include "resources/attr.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <windows.h>
#include <mmsystem.h>
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

#include <string.h>

const float g_naviCellUVs[8] = {0, 0, 1, 0, 0, 1, 1, 1};

unsigned g_naviMarkerTime;

int g_naviMarkerFrame = 1;

#define NAVI_MINIMAP_SIZE       126.0f
#define NAVI_PATH_DRAW_RANGE    10
#define NAVI_MARKER_FRAME_MS    150
#define NAVI_MARKER_FRAMES      4

float navi_manager_fit_minimap(struct NaviManager *self, int *ox, int *oy,
        int width, int height, float size)
{
    float scale;

    if (width > height) {
        scale = size / (float)width;
        *oy = (int)((float)(width - height) * 0.5f * scale);
        *ox = 0;
    } else if (width < height) {
        scale = size / (float)height;
        *ox = (int)((float)(height - width) * 0.5f * scale);
        *oy = 0;
    } else {
        *oy = 0;
        scale = size / (float)width;
        *ox = 0;
    }
    return scale;
}

struct NaviPoint *navi_manager_minimap_pos(struct NaviManager *self, struct NaviPoint *out,
        struct NaviMap *map, int x, int y, float size)
{
    out->x = -1;
    out->y = -1;
    if (map) {
        int ox, oy;
        float scale = navi_manager_fit_minimap(self, &ox, &oy, navi_map_width(map),
                                             navi_map_height(map), size);

        out->x = (int)((float)x * scale + (float)ox);
        out->y = (int)((float)(int)size - ((float)y * scale + (float)oy));
    }
    return out;
}

struct NaviPoint *navi_manager_minimap_pos_by_name(struct NaviManager *self,
        struct NaviPoint *out, const char *name, int x, int y, float size)
{
    navi_manager_minimap_pos(self, out, navi_map_named(self, name), x, y, size);
    return out;
}

static void add_path_dot(struct NaviManager *self, struct NaviPoint dot)
{
    struct NaviPoint *p, *end = (struct NaviPoint *)self->pathDots.last;

    for (p = (struct NaviPoint *)self->pathDots.first; p != end; p++)
        if (p->x == dot.x && p->y == dot.y)
            return;
    vec_push(&self->pathDots, &dot, sizeof dot);
}

unsigned char navi_manager_bind_world(struct NaviManager *self)
{
    struct C3dAttr *attr = game_current_map_attr();

    if (!attr)
        return 0;
    self->ground = game_current_ground();
    path_finder_set_attr(&self->pathFinder, attr);
    return 1;
}

unsigned char navi_manager_find_path(struct NaviManager *self, int sx, int sy,
        int tx, int ty, unsigned char draw)
{
    const struct PathCell *cell, *cells_end;
    unsigned char found;
    struct NaviMap *here;
    int ox, oy;
    float scale;

    path_info_clear(&self->path);
    found = path_finder_find_path(&self->pathFinder, 0, sx, sy, 8, 8, tx, ty, 1, &self->path, 0xf);
    path_finder_clear_search(&self->pathFinder);
    if (!draw)
        return found;

    self->pathDots.last = self->pathDots.first;
    if (!found)
        return found;
    here = navi_map_named(self, g_session1.mapName);
    if (!here)
        return found;

    scale = navi_manager_fit_minimap(self, &ox, &oy, navi_map_width(here), navi_map_height(here),
                                   NAVI_MINIMAP_SIZE);
    cells_end = self->path.cells + ((int)self->path.count);
    for (cell = self->path.cells; cell != cells_end; cell++) {
        struct NaviPoint dot;

        dot.x = (int)((float)cell->x * scale + (float)ox);
        dot.y = (int)(NAVI_MINIMAP_SIZE - ((float)cell->y * scale + (float)oy));
        add_path_dot(self, dot);
    }
    ui_navigation_invalidate();
    return found;
}

void navi_manager_load_grid_images(struct NaviManager *self)
{
    char path[0x104];
    int i;

    for (i = 0; i < 8; i++) {
        memset(path, 0, 0x104);
        sprintf(path, NAVI_UI_DIR "navigation_interface3\\navi_grid%d_%d.tga", self->roadIcon, i);
        string_assign_cstr(&self->gridImages[i], path);
    }
}

static int draw_path_cells(struct NaviManager *self, void *view, int px, int py)
{
    const struct PathCell *cells = self->path.cells;
    int last = ((int)self->path.count) - 1;
    unsigned phase = timeGetTime() % 1000 / 100;
    int drawn = 0, i;

    for (i = 1; i < last; i += 2) {
        const struct PathCell *cell = &cells[i];
        unsigned colour = phase == (unsigned)i % 10 ? 0xeeffffff : 0x99ffffff;

        if (px + NAVI_PATH_DRAW_RANGE < cell->x || px - NAVI_PATH_DRAW_RANGE > cell->x)
            continue;
        if (py + NAVI_PATH_DRAW_RANGE < cell->y || py - NAVI_PATH_DRAW_RANGE > cell->y)
            continue;
        if (self->ground) {
            void *tex = texture_cache_get(&g_textureCache, string_cstr(&self->gridImages[cell->dir]), 0, 0);

            c3d_ground15_draw_attr_quad_raised(self->ground, view, cell->x, cell->y, colour, tex, g_naviCellUVs);
        }
        drawn++;
    }
    return drawn;
}

static void draw_marker(struct NaviManager *self, void *view)
{
    unsigned now = timeGetTime();
    char file[100];
    int frame = g_naviMarkerFrame;
    struct C3dGround15 *ground;
    struct NaviPoint targetForY, targetForX;

    memset(file, 0, 0x64);
    if ((int)(now - g_naviMarkerTime) >= NAVI_MARKER_FRAME_MS) {
        frame++;
        g_naviMarkerTime = now;
        if (frame > NAVI_MARKER_FRAMES)
            frame = 1;
        g_naviMarkerFrame = frame;
    }
    sprintf(file, NAVI_UI_DIR "navigation_interface3\\location%d.tga", frame);
    ground = game_current_ground();
    if (!ground)
        return;
    navi_manager_target_pos(self, &targetForY);
    navi_manager_target_pos(self, &targetForX);
    c3d_ground15_draw_attr_marker(ground, view, targetForX.x, targetForY.y, -1,
                                         texture_cache_get(&g_textureCache, file, 0, 0));
}

void navi_manager_draw_path(struct NaviManager *self, void *view)
{
    if (self->showPath) {
        int count = ((int)self->path.count);
        int px = 0, py = 0;
        struct GameMode *gm;

        if (count < 3)
            return;
        gm = game_current_mode();
        if (gm)
            game_mode_get_player_cell(gm, &px, &py);
        if (!draw_path_cells(self, view, px, py)) {
            const struct PathCell *end = self->path.cells + count;

            navi_manager_find_path(self, px, py, end[-1].x, end[-1].y, 1);
        }
    }

    if (self->showMarker && !strcmp(g_session1.mapName, navi_map_name(navi_manager_target_map(self))))
        draw_marker(self, view);
}
