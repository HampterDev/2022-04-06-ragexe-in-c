#ifndef UI_QUESTS_WORLD_VIEW_H
#define UI_QUESTS_WORLD_VIEW_H

#include <stddef.h>
#include <stdint.h>

struct ClientLua;
struct Texture;

#define WORLD_VIEW_FIELD_ID 100
#define WORLD_VIEW_ROWS 400
#define WORLD_VIEW_ICONS 16
#define WORLD_VIEW_TOP 17

struct WorldViewRect { int left, top, right, bottom; };

struct WorldViewMap {
    struct WorldViewRect source_rect, rect;
    char *name, *text_a, *text_b;
};

struct WorldViewArea {
    char *name, *text;
    int id;
    struct WorldViewRect source_rect, rect;
    struct WorldViewMap *maps;
    size_t map_count, map_capacity;
};

struct WorldViewWorld {
    char *name, *texture_name;
    struct Texture *texture;
    int image_width, image_height;
    struct WorldViewArea *areas;
    size_t area_count, area_capacity;
};

struct WorldViewEntry {
    int world, area, map, level;
    const char *name;
    struct WorldViewRect rect;
};

struct WorldView {
    struct WorldViewWorld *worlds;
    size_t world_count, world_capacity;
    struct WorldViewEntry *marks, *level_marks;
    size_t mark_count, mark_capacity, level_mark_count, level_mark_capacity;
    struct WorldViewEntry current;
    int current_world, resolution, layout_width, layout_height;
    uint32_t dungeon_colour, dungeon_alpha;
    uint32_t dungeon_font_colour[2], dungeon_select_colour[2];
    int dungeon_font_size[4];
    uint32_t mob_colour[2], mob_level_colour[2];
    int mob_font_size[4], mob_level_font_size[4];
    uint32_t box_line_colour, search_colour[WORLD_VIEW_ICONS];
    const char *search_icon[WORLD_VIEW_ICONS];
    uint32_t run_colour, run_font_colour[2], select_box_colour;
    int run_font_size[4];
};

extern struct WorldView g_worldView;

void world_view_initialize(struct WorldView *view);
void world_view_clear(struct WorldView *view);
int world_view_load(struct WorldView *view, struct ClientLua *lua);

const struct WorldViewWorld *world_view_world_at(const struct WorldView *view, size_t index);
const struct WorldViewWorld *world_view_current_world(const struct WorldView *view);
int world_view_select(struct WorldView *view, int index);
int world_view_select_for_map(struct WorldView *view, const char *name);
const struct WorldViewMap *world_view_find_map(const struct WorldViewWorld *world, const char *name);
const struct WorldViewMap *world_view_map_at(const struct WorldViewWorld *world, int *area, int x, int y);
const struct WorldViewArea *world_view_field_at(const struct WorldViewWorld *world, int *area, int x, int y);

int world_view_set_current(struct WorldView *view, const char *name);
void world_view_clear_current(struct WorldView *view);
int world_view_add_mark(struct WorldView *view, const char *name);
void world_view_clear_marks(struct WorldView *view);
const struct WorldViewEntry *world_view_mark(const struct WorldView *view, int index, int current_only);
const struct WorldViewRect *world_view_mark_rect(const struct WorldView *view, int index, int current_only);
int world_view_add_level_mark(struct WorldView *view, const char *name, int level);
void world_view_clear_level_marks(struct WorldView *view);
const struct WorldViewRect *world_view_level_mark_rect(const struct WorldView *view, int *icon, int index, int current_only);

void world_view_layout(struct WorldView *view, int width, int height);
void world_view_prepare(struct WorldView *view);
void world_view_draw(struct WorldView *view, int x, int y, int width, int height);

#endif
