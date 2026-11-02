#include "render/texture_cache.h"
#include "render/texture_types.h"
#include "game/context.h"
#include <string.h>
#include "ui/worldmap/romap.h"
#include "render/screen_draw.h"
#include "platform/clock.h"

#define WORLDMAP_DIR UI_SKIN "worldview_interface\\"
#define MINIMAP_SIZE 0x82

static void draw_route_marks(void)
{
    int count = (int)g_worldView.mark_count;
    int i;

    for (i = 0; i < count; i++) {
        const struct WorldViewRect *r = world_view_mark_rect(&g_worldView, i, 1);
        int x, y, width, height;

        if (!r)
            continue;
        x = r->left + 1;
        y = r->top + 1;
        width = r->right - r->left - 2;
        height = r->bottom - r->top - 2;
        draw_box_screen(x, y, width, height, g_worldView.run_colour);
        if (i != 0) {
            const struct WorldViewRect *p = world_view_mark_rect(&g_worldView, i - 1, 1);

            if (p) {
                float x0 = (float)(p->right - p->left) * 0.5f + (float)p->left;
                float y0 = (float)(p->bottom - p->top) * 0.5f + (float)p->top;
                float x1 = (float)(r->right - r->left) * 0.5f + (float)r->left;
                float y1 = (float)(r->bottom - r->top) * 0.5f + (float)r->top;

                screen_thick_line(x0, y0, x1, y1, 5,
                                  texture_cache_get(&g_textureCache, WORLDMAP_DIR "wv_Airf2.bmp", 0, 0),
                                  0x99ffffff);
            }
        }
        if (count - 1 == i) {
            const struct WorldViewEntry *last = world_view_mark(&g_worldView, i, 1);
            struct String target;
            const char *name;
            struct Texture *tex;

            if (!last)
                continue;
            name = navi_map_name(navi_manager_target_map2(&g_navi));
            string_init(&target);
            string_assign_cstr(&target, name);
            string_append_cstr(&target, ".rsw");
            if (strcmp(string_cstr(&target), last->name) == 0) {
                tex = texture_cache_get(&g_textureCache, WORLDMAP_DIR "wv_End.bmp", 0, 0);
                if (tex)
                    screen_box_textured((int)((float)width * 0.5f + (float)x - (float)((unsigned)tex->reqWidth)),
                                        (int)((float)height * 0.5f + (float)y - (float)((unsigned)tex->reqHeight)),
                                        ((unsigned)tex->reqWidth), ((unsigned)tex->reqHeight), 0xccffffff, tex);
            }
            string_free(&target);
        }
    }
}

static void draw_member_faces(struct UIRoMapWnd *self)
{
    const char *current = game_current_map_name();
    struct String map, face;
    struct ListNode *node;
    int half, size;

    string_init(&map);
    string_assign_cstr(&map, current);
    half = g_worldView.resolution * 2 + 8;
    size = half * 2;
    string_init(&face);
    string_assign(&face, WORLDMAP_DIR "wv_face1.bmp", 0x2f);
    for (node = self->entries.head->next; node != self->entries.head; node = node->next) {
        struct RoMapEntry *e = LIST_VALUE(node, struct RoMapEntry);
        int x, y;

        if (g_worldView.current_world != e->region)
            continue;
        x = (int)((float)(e->rect.left - half) + (float)(e->rect.right - e->rect.left) * 0.5f);
        y = (int)((float)(e->rect.top - half) + (float)(e->rect.bottom - e->rect.top) * 0.5f);
        if (string_equal(&map, &e->name))
            string_assign(&face, WORLDMAP_DIR "wv_face1.bmp", 0x2f);
        else
            string_assign(&face, WORLDMAP_DIR "wv_face.bmp", 0x2e);
        screen_box_textured(x, y, size, size, 0xeeffffff,
                            texture_cache_get(&g_textureCache, string_cstr(&face), 0, 0));
    }
    string_free(&face);
    string_free(&map);
}

static BYTE rect_is(const struct WorldViewRect *a, const struct WorldViewRect *b)
{
    return a->left == b->left && a->right == b->right && a->top == b->top && a->bottom == b->bottom;
}

static void draw_dungeon_places(struct UIRoMapWnd *self)
{
    struct Texture *pos_tex = texture_cache_get(&g_textureCache, WORLDMAP_DIR "wv_dunPos.bmp", 0, 1);
    struct Texture *select_tex = texture_cache_get(&g_textureCache, WORLDMAP_DIR "wv_dunSelect.bmp", 0, 1);
    DWORD frame = g_worldView.dungeon_alpha;
    DWORD line = g_worldView.dungeon_colour;
    DWORD select = g_worldView.dungeon_select_colour[0];
    const struct WorldViewWorld *world = world_view_current_world(&g_worldView);
    size_t i;
    if (!world) return;

    for (i = 0; i < world->area_count; i++) {
        const struct WorldViewArea *p = &world->areas[i];
        const struct WorldViewRect *box;
        BYTE sel;

        if (p->id >= WORLD_VIEW_FIELD_ID)
            continue;
        if (p->map_count != 0) {
            const struct WorldViewMap *a = &p->maps[0];
            float x0, y0, x1, y1;

            sel = self->spot && rect_is(&a->rect, &self->spot->rect);
            x0 = (float)(a->rect.right - a->rect.left) * 0.5f + (float)a->rect.left;
            y0 = (float)(a->rect.bottom - a->rect.top) * 0.5f + (float)a->rect.top;
            x1 = (float)(p->rect.right - p->rect.left) * 0.5f + (float)p->rect.left;
            y1 = (float)(p->rect.bottom - p->rect.top) * 0.5f + (float)p->rect.top + 1.0f;
            screen_thick_line(x0, y0, x1, y1, 1, &g_whiteTexture,
                              sel ? (select & 0xffffff) - 0x1000000 : line);
            screen_box_textured(p->rect.left, p->rect.top, p->rect.right - p->rect.left,
                                p->rect.bottom - p->rect.top, sel ? 0xffffffff : frame, pos_tex);
            box = &a->rect;
        } else {
            sel = self->spot && rect_is(&p->rect, &self->spot->rect);
            box = &p->rect;
        }
        screen_box_textured(box->left, box->top, box->right - box->left, box->bottom - box->top,
                            sel ? 0xffffffff : frame, select_tex);
    }
}

void ui_ro_map_wnd_draw_level_marks(void)
{
    int count = (int)g_worldView.level_mark_count;
    int i;

    for (i = 0; i < count; i++) {
        int icon = 0;
        const struct WorldViewRect *r = world_view_level_mark_rect(&g_worldView, &icon, i, 1);
        int x, y, width, height;

        if (!r)
            continue;
        x = r->left + 1;
        y = r->top + 1;
        width = r->right - r->left - 2;
        height = r->bottom - r->top - 2;
        draw_box_screen(x, y, width, height, g_worldView.search_colour[icon]);
        screen_box_textured(x, y, width, height, 0xffffffff,
                            texture_cache_get(&g_textureCache, g_worldView.search_icon[icon], 0,
                                             0));
    }
}

void ui_ro_map_wnd_draw_player_mark(void)
{
    if (*g_worldView.current.name && g_worldView.current_world == g_worldView.current.world) {
        struct WorldViewRect r = g_worldView.current.rect;

        screen_box_textured(r.left, r.top, r.right - r.left, r.bottom - r.top, 0xeeffffff,
                            texture_cache_get(&g_textureCache, WORLDMAP_DIR "wv_Select.bmp", 0, 0));
    }
}

void ui_ro_map_wnd_draw_minimap(struct UIRoMapWnd *self)
{
    if (!self->minimap)
        return;
    draw_box_screen(self->minimap_box->a, self->minimap_box->b, MINIMAP_SIZE, MINIMAP_SIZE, 0x88000000);
    texture_draw_surface_stretch(self->minimap, self->minimap_box->a, self->minimap_box->b, MINIMAP_SIZE, MINIMAP_SIZE);
}

void ui_ro_map_wnd_draw_spot(struct UIRoMapWnd *self)
{
    if (self->spot && self->minimap_box) {
        struct RoMapEntry *spot = self->spot;

        draw_box_screen(spot->rect.left, spot->rect.top, spot->rect.right - spot->rect.left,
                      spot->rect.bottom - spot->rect.top, g_worldView.select_box_colour);
        ui_ro_map_wnd_draw_minimap(self);
    }
    if (self->members_box)
        draw_box_screen(self->members_box->a, self->members_box->b, self->members_box->c, self->members_box->d,
                      0x66000000);
}

float ui_ro_map_wnd_frame_seconds(struct UIRoMapWnd *self)
{
    DWORD now, elapsed;

    if (self->first_frame == 1) {
        self->frame_time = platform_ticks_ms();
        self->first_frame = 0;
    }
    now = platform_ticks_ms();
    elapsed = now - self->frame_time;
    self->frame_time = now;
    return (float)elapsed / 1000.0f;
}

void ui_ro_map_wnd_render_overlay(struct UIRoMapWnd *self)
{
    float seconds;

    world_view_draw(&g_worldView, 0, 0x11, self->frame.base.width,
                        self->frame.base.height - 0x11);
    ui_ro_map_wnd_draw_level_marks();
    draw_route_marks();
    ui_ro_map_wnd_draw_player_mark();
    draw_member_faces(self);
    draw_dungeon_places(self);
    ui_ro_map_wnd_draw_spot(self);
    seconds = ui_ro_map_wnd_frame_seconds(self);
    kafra_move_map_service_info_update(&self->kafra, self, seconds);
    kafra_move_map_service_info_draw(&self->kafra, self);
    ui_ro_map_wnd_process_ship(self, &self->ship[0], seconds);
    ui_ro_map_wnd_process_ship(self, &self->ship[1], seconds);
}
