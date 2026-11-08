#include "ui/quests/world_view.h"
#include "contents/client_lua.h"
#include "session/state.h"
#include "session/map_names.h"
#include "core/msg_str.h"
#include <lauxlib.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define SEARCH_DIR "유저인터페이스\\worldview_interface\\wv_Sreach"
#define BOSS_LEVEL 10000

struct WorldView g_worldView = { .current = { .name = "" } };

void world_view_initialize(struct WorldView *view)
{
    static const char *const icons[WORLD_VIEW_ICONS] = {
        SEARCH_DIR "Map.bmp", SEARCH_DIR "Mob1.bmp", SEARCH_DIR "Mob2.bmp",
        SEARCH_DIR "Mob3.bmp", SEARCH_DIR "Mob4.bmp", SEARCH_DIR "Mob5.bmp",
        SEARCH_DIR "NoImage.bmp", SEARCH_DIR "NoImage.bmp", SEARCH_DIR "NoImage.bmp",
        SEARCH_DIR "NoImage.bmp", SEARCH_DIR "Npc.bmp", SEARCH_DIR "Npc_Mob1.bmp",
        SEARCH_DIR "Npc_Mob2.bmp", SEARCH_DIR "Npc_Mob3.bmp", SEARCH_DIR "Npc_Mob4.bmp",
        SEARCH_DIR "Npc_Mob5.bmp"
    };
    size_t i;
    memset(view, 0, sizeof *view);
    view->current.name = "";
    view->dungeon_font_colour[0] = view->mob_colour[0] = view->mob_level_colour[0] = 0xffffffff;
    view->dungeon_font_colour[1] = view->mob_colour[1] = view->mob_level_colour[1] = 0xff000000;
    view->dungeon_select_colour[0] = view->dungeon_select_colour[1] = 0xffffffff;
    view->box_line_colour = 0x282828;
    view->run_font_colour[0] = 0xff000000;
    for (i = 0; i < 4; ++i) {
        view->dungeon_font_size[i] = view->mob_font_size[i] = 10;
        view->mob_level_font_size[i] = view->run_font_size[i] = 10;
    }
    for (i = 0; i < WORLD_VIEW_ICONS; ++i) {
        view->search_colour[i] = 0xeeffffff;
        view->search_icon[i] = icons[i];
    }
}

void world_view_clear(struct WorldView *view)
{
    size_t i, j, k;
    for (i = 0; i < view->world_count; ++i) {
        struct WorldViewWorld *world = &view->worlds[i];
        for (j = 0; j < world->area_count; ++j) {
            struct WorldViewArea *area = &world->areas[j];
            for (k = 0; k < area->map_count; ++k) {
                free(area->maps[k].name);
                free(area->maps[k].text_a);
                free(area->maps[k].text_b);
            }
            free(area->maps);
            free(area->name);
            free(area->text);
        }
        free(world->areas);
        free(world->name);
        free(world->texture_name);
    }
    free(view->worlds);
    free(view->marks);
    free(view->level_marks);
    world_view_initialize(view);
}

static void *grow(void *data, size_t *capacity, size_t count, size_t size)
{
    size_t next;
    void *replacement;
    if (count < *capacity)
        return data;
    if (count >= INT_MAX || count >= (size_t)-1 / size)
        return NULL;
    next = *capacity + *capacity / 2 + 8;
    if (next > (size_t)-1 / size)
        next = count + 1;
    replacement = realloc(data, next * size);
    if (replacement)
        *capacity = next;
    return replacement;
}

static char *copy_text(const char *text)
{
    size_t length = strlen(text ? text : "") + 1;
    char *copy = malloc(length);
    if (copy)
        memcpy(copy, text ? text : "", length);
    return copy;
}

const struct WorldViewWorld *world_view_world_at(const struct WorldView *view, size_t index)
{
    return index < view->world_count ? &view->worlds[index] : NULL;
}

const struct WorldViewWorld *world_view_current_world(const struct WorldView *view)
{
    return world_view_world_at(view, (size_t)view->current_world);
}

int world_view_select(struct WorldView *view, int index)
{
    if (index < 0 || !view->world_count)
        index = 0;
    else if ((size_t)index >= view->world_count)
        index = (int)view->world_count - 1;
    return view->current_world = index;
}

const struct WorldViewMap *world_view_find_map(const struct WorldViewWorld *world, const char *name)
{
    size_t i, j;
    if (!world || !name)
        return NULL;
    for (i = 0; i < world->area_count; ++i)
        for (j = 0; j < world->areas[i].map_count; ++j)
            if (strcmp(world->areas[i].maps[j].name, name) == 0)
                return &world->areas[i].maps[j];
    return NULL;
}

int world_view_select_for_map(struct WorldView *view, const char *name)
{
    size_t i;
    for (i = 0; i < view->world_count; ++i)
        if (world_view_find_map(&view->worlds[i], name))
            return view->current_world = (int)i;
    return view->current_world = 0;
}

static int contains(const struct WorldViewRect *rect, int x, int y)
{
    return rect->left < x && x < rect->right && rect->top < y && y < rect->bottom;
}

static int area_hit(const struct WorldViewArea *area, int x, int y)
{
    const struct WorldViewRect *rect = area->id < WORLD_VIEW_FIELD_ID && area->map_count
        ? &area->maps[0].rect : &area->rect;
    return contains(rect, x, y);
}

const struct WorldViewMap *world_view_map_at(const struct WorldViewWorld *world, int *area_index, int x, int y)
{
    size_t i, j;
    if (!world)
        return NULL;
    for (i = 0; i < world->area_count; ++i) {
        const struct WorldViewArea *area = &world->areas[i];
        if (area_hit(area, x, y)) {
            *area_index = (int)i;
            for (j = 0; j < area->map_count; ++j)
                if (contains(&area->maps[j].rect, x, y))
                    return &area->maps[j];
            return NULL;
        }
    }
    return NULL;
}

const struct WorldViewArea *world_view_field_at(const struct WorldViewWorld *world, int *area_index, int x, int y)
{
    size_t i;
    if (!world)
        return NULL;
    for (i = 0; i < world->area_count; ++i)
        if (world->areas[i].id < WORLD_VIEW_FIELD_ID && area_hit(&world->areas[i], x, y)) {
            *area_index = (int)i;
            return &world->areas[i];
        }
    return NULL;
}

static int find_entry(const struct WorldView *view, const char *name, int level, struct WorldViewEntry *entry)
{
    size_t i, j, k;
    if (!name)
        return 0;
    for (i = 0; i < view->world_count; ++i) {
        const struct WorldViewWorld *world = &view->worlds[i];
        for (j = 0; j < world->area_count; ++j) {
            const struct WorldViewArea *area = &world->areas[j];
            for (k = 0; k < area->map_count; ++k) {
                const struct WorldViewMap *map = &area->maps[k];
                if (strcmp(map->name, name) != 0)
                    continue;
                if (area->id >= WORLD_VIEW_FIELD_ID)
                    goto next_world;
                *entry = (struct WorldViewEntry){(int)i, (int)j, (int)k, level, map->name, map->rect};
                return 1;
            }
        }
next_world:;
    }
    return 0;
}

void world_view_clear_current(struct WorldView *view)
{
    memset(&view->current, 0, sizeof view->current);
    view->current.name = "";
}

int world_view_set_current(struct WorldView *view, const char *name)
{
    world_view_clear_current(view);
    return find_entry(view, name, 0, &view->current);
}

static struct WorldViewEntry *find_mark(struct WorldViewEntry *marks, size_t count, const char *name)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (strcmp(marks[i].name, name) == 0)
            return &marks[i];
    return NULL;
}

static int append_mark(struct WorldView *view, const char *name, int level, struct WorldViewEntry **marks,
                       size_t *count, size_t *capacity)
{
    struct WorldViewEntry entry, *replacement;
    if (!find_entry(view, name, level, &entry))
        return 0;
    replacement = grow(*marks, capacity, *count, sizeof **marks);
    if (!replacement)
        return 0;
    *marks = replacement;
    (*marks)[(*count)++] = entry;
    return 1;
}

int world_view_add_mark(struct WorldView *view, const char *name)
{
    if (!name)
        return 0;
    if (find_mark(view->marks, view->mark_count, name))
        return 1;
    return append_mark(view, name, 0, &view->marks, &view->mark_count, &view->mark_capacity);
}

void world_view_clear_marks(struct WorldView *view) { view->mark_count = 0; }
void world_view_clear_level_marks(struct WorldView *view) { view->level_mark_count = 0; }

const struct WorldViewEntry *world_view_mark(const struct WorldView *view, int index, int current_only)
{
    if (index < 0 || (size_t)index >= view->mark_count ||
        (current_only && view->marks[index].world != view->current_world))
        return NULL;
    return &view->marks[index];
}

const struct WorldViewRect *world_view_mark_rect(const struct WorldView *view, int index, int current_only)
{
    const struct WorldViewEntry *entry = world_view_mark(view, index, current_only);
    return entry ? &entry->rect : NULL;
}

int world_view_add_level_mark(struct WorldView *view, const char *name, int level)
{
    struct WorldViewEntry *entry;
    int old;
    if (!name)
        return 0;
    entry = find_mark(view->level_marks, view->level_mark_count, name);
    if (!entry)
        return append_mark(view, name, level, &view->level_marks, &view->level_mark_count, &view->level_mark_capacity);
    old = entry->level;
    if (level == 0)
        return 1;
    if (level == BOSS_LEVEL) {
        if (old < BOSS_LEVEL)
            entry->level = old + BOSS_LEVEL;
    } else if (old >= BOSS_LEVEL) {
        if (level > old - BOSS_LEVEL) {
            if (level > INT_MAX - BOSS_LEVEL)
                return 0;
            entry->level = level + BOSS_LEVEL;
        }
    } else if (level > old) {
        entry->level = level;
    }
    return 1;
}

const struct WorldViewRect *world_view_level_mark_rect(const struct WorldView *view, int *icon, int index, int current_only)
{
    const struct WorldViewEntry *entry;
    int level, base, step;
    if (index < 0 || (size_t)index >= view->level_mark_count)
        return NULL;
    entry = &view->level_marks[index];
    if (current_only && entry->world != view->current_world)
        return NULL;
    level = entry->level;
    base = level >= BOSS_LEVEL ? 10 : *icon;
    level %= BOSS_LEVEL;
    step = level > 99 ? 5 : level > 49 ? 4 : level >= 20 ? 3 : level >= 5 ? 2 : level >= 1 ? 1 : 0;
    *icon = base + step;
    return &entry->rect;
}

static int scaled_rect(struct WorldViewRect *out, const struct WorldViewRect *source, float sx, float sy)
{
    double values[4] = {
        (double)((float)source->left * sx) + 0.5, (double)((float)source->top * sy) + 0.5,
        (double)((float)source->right * sx) + 0.5, (double)((float)source->bottom * sy) + 0.5
    };
    size_t i;
    for (i = 0; i < 4; ++i)
        if (!(values[i] >= INT_MIN && values[i] <= INT_MAX - ((i & 1) ? WORLD_VIEW_TOP : 0)))
            return 0;
    *out = (struct WorldViewRect){(int)values[0], (int)values[1] + WORLD_VIEW_TOP,
                                 (int)values[2], (int)values[3] + WORLD_VIEW_TOP};
    return 1;
}

static void refresh_entry(const struct WorldView *view, struct WorldViewEntry *entry)
{
    if (*entry->name)
        entry->rect = view->worlds[entry->world].areas[entry->area].maps[entry->map].rect;
}

void world_view_layout(struct WorldView *view, int width, int height)
{
    size_t i, j, k;
    view->resolution = width <= 640 ? 0 : width <= 800 ? 1 : width <= 1024 ? 2 : 3;
    view->layout_width = width;
    view->layout_height = height;
    if (width <= 0 || height <= WORLD_VIEW_TOP)
        return;
    for (i = 0; i < view->world_count; ++i) {
        struct WorldViewWorld *world = &view->worlds[i];
        float sx, sy;
        int pass;
        if (world->image_width <= 0 || world->image_height <= 0)
            continue;
        sx = (float)width / (float)world->image_width;
        sy = (float)(height - WORLD_VIEW_TOP) / (float)world->image_height;
        for (pass = 0; pass < 2; ++pass) {
            for (j = 0; j < world->area_count; ++j) {
                struct WorldViewArea *area = &world->areas[j];
                struct WorldViewRect rect;
                if (!scaled_rect(&rect, &area->source_rect, sx, sy))
                    goto next_world;
                if (pass)
                    area->rect = rect;
                for (k = 0; k < area->map_count; ++k) {
                    if (!scaled_rect(&rect, &area->maps[k].source_rect, sx, sy))
                        goto next_world;
                    if (pass)
                        area->maps[k].rect = rect;
                }
            }
        }
next_world:;
    }
    refresh_entry(view, &view->current);
    for (i = 0; i < view->mark_count; ++i)
        refresh_entry(view, &view->marks[i]);
    for (i = 0; i < view->level_mark_count; ++i)
        refresh_entry(view, &view->level_marks[i]);
}

struct LoadContext {
    struct WorldView next;
    struct ClientLua *lua;
    char *map_table, *field_table;
    int ok;
};

static int query(struct LoadContext *load, const char *name, const char *table, int row, int arguments, int results, int optional)
{
    lua_State *L = load->lua->L;
    lua_getglobal(L, name);
    if (optional && lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    if (table)
        lua_pushstring(L, table);
    if (arguments)
        lua_pushinteger(L, row);
    if (lua_pcall(L, arguments, results, 0)) {
        const char *error = lua_tostring(L, -1);
        client_lua_error_box(load->lua, name, "%s\n", error ? error : "Lua query failed");
        lua_pop(L, 1);
        return -1;
    }
    return 1;
}

static int integer(lua_State *L, int index, int *out)
{
    double value = lua_tonumber(L, index);
    if (lua_isnil(L, index)) {
        *out = 0;
        return 1;
    }
    if (!lua_isnumber(L, index) || !(value >= INT_MIN && value <= INT_MAX))
        return 0;
    *out = (int)value;
    return 1;
}

static int numbers(struct LoadContext *load, const char *name, int row, int arguments, int *values, int count)
{
    lua_State *L = load->lua->L;
    int i, result = query(load, name, NULL, row, arguments, count, 1);
    if (result <= 0)
        return result;
    for (i = 0; i < count; ++i) {
        if (!integer(L, i - count, &values[i])) {
            lua_pop(L, count);
            return -1;
        }
    }
    lua_pop(L, count);
    return 1;
}

static uint32_t rgb(int r, int g, int b)
{
    return (uint32_t)(uint8_t)r << 16 | (uint32_t)(uint8_t)g << 8 | (uint8_t)b;
}

static int read_style(struct LoadContext *load)
{
    struct WorldView *view = &load->next;
    int v[7], result, i;
#define NUMBERS(name, count) \
    result = numbers(load, name, 0, 0, v, count); \
    if (result < 0) return 0; \
    if (result)
    NUMBERS("Get_Worldview_info_Dungeon", 4) {
        view->dungeon_colour = rgb(v[0], v[1], v[2]) + ((uint32_t)v[3] << 24);
        view->dungeon_alpha = ((uint32_t)v[3] << 24) + 0xffffff;
    }
    NUMBERS("Get_Worldview_info_Dungeon_FontColor", 6) {
        view->dungeon_font_colour[0] = rgb(v[2], v[1], v[0]);
        view->dungeon_font_colour[1] = rgb(v[5], v[4], v[3]);
    }
    NUMBERS("Get_Worldview_info_Dungeon_FontSize", 4) { memcpy(view->dungeon_font_size, v, sizeof view->dungeon_font_size); }
    NUMBERS("Get_Worldview_info_Dungeon_SelectColor", 3) {
        view->dungeon_select_colour[0] = rgb(v[0], v[1], v[2]);
        view->dungeon_select_colour[1] = rgb(v[2], v[1], v[0]);
    }
    NUMBERS("Get_Worldview_info_Mob_Color", 6) {
        view->mob_colour[0] = rgb(v[2], v[1], v[0]);
        view->mob_colour[1] = rgb(v[5], v[4], v[3]);
    }
    NUMBERS("Get_Worldview_info_Mob_font", 4) { memcpy(view->mob_font_size, v, sizeof view->mob_font_size); }
    NUMBERS("Get_Worldview_info_MobLv_Color", 6) {
        view->mob_level_colour[0] = rgb(v[2], v[1], v[0]);
        view->mob_level_colour[1] = rgb(v[5], v[4], v[3]);
    }
    NUMBERS("Get_Worldview_info_MobLv_font", 4) { memcpy(view->mob_level_font_size, v, sizeof view->mob_level_font_size); }
    NUMBERS("Get_Worldview_info_BoxLine_Color", 3) { view->box_line_colour = rgb(v[0], v[1], v[2]); }
    NUMBERS("Get_Worldview_info_Sreach_Alpha", 7) {
        for (i = 0; i < 6; ++i)
            view->search_colour[i] = (uint32_t)v[i] << 24;
        view->search_colour[10] = (uint32_t)v[6] << 24;
        for (i = 0; i < WORLD_VIEW_ICONS; ++i) {
            result = numbers(load, "Get_Worldview_info_Sreach_Color", i, 1, v, 3);
            if (result < 0) return 0;
            if (result) view->search_colour[i] += rgb(v[0], v[1], v[2]);
        }
        for (i = 1; i < 6; ++i)
            view->search_colour[10 + i] = view->search_colour[i];
    }
    NUMBERS("Get_Worldview_info_Run", 4) { view->run_colour = rgb(v[0], v[1], v[2]) + ((uint32_t)v[3] << 24); }
    NUMBERS("Get_Worldview_info_Run_fontColor", 6) {
        view->run_font_colour[0] = rgb(v[2], v[1], v[0]);
        view->run_font_colour[1] = rgb(v[5], v[4], v[3]);
    }
    NUMBERS("Get_Worldview_info_Run_font", 4) { memcpy(view->run_font_size, v, sizeof view->run_font_size); }
    NUMBERS("Get_Worldview_info_SelectBox_Color", 4) { view->select_box_colour = rgb(v[0], v[1], v[2]) + ((uint32_t)v[3] << 24); }
#undef NUMBERS
    return 1;
}

static struct WorldViewArea *area_by_id(struct WorldViewWorld *world, int id)
{
    size_t i;
    for (i = 0; i < world->area_count; ++i)
        if (world->areas[i].id == id)
            return &world->areas[i];
    return NULL;
}

static int add_map(struct WorldViewWorld *world, int id, const struct WorldViewRect *rect,
                   const char *name, const char *title, const char *subtitle)
{
    struct WorldViewArea *area = area_by_id(world, id);
    struct WorldViewMap *map, *maps;
    if (!area) {
        struct WorldViewArea *areas = grow(world->areas, &world->area_capacity, world->area_count, sizeof *areas);
        if (!areas) return 0;
        world->areas = areas;
        area = &world->areas[world->area_count++];
        memset(area, 0, sizeof *area);
        area->id = id;
        area->source_rect = *rect;
        area->name = copy_text(name);
        area->text = copy_text(name);
        if (!area->name || !area->text) return 0;
    }
    if (area->source_rect.left > rect->left) area->source_rect.left = rect->left;
    if (area->source_rect.top > rect->top) area->source_rect.top = rect->top;
    if (area->source_rect.right < rect->right) area->source_rect.right = rect->right;
    if (area->source_rect.bottom < rect->bottom) area->source_rect.bottom = rect->bottom;
    area->rect = area->source_rect;
    maps = grow(area->maps, &area->map_capacity, area->map_count, sizeof *maps);
    if (!maps) return 0;
    area->maps = maps;
    map = &area->maps[area->map_count++];
    memset(map, 0, sizeof *map);
    map->source_rect = map->rect = *rect;
    map->name = copy_text(name);
    map->text_a = copy_text(title);
    map->text_b = copy_text(subtitle);
    return map->name && map->text_a && map->text_b;
}

static int read_maps(struct LoadContext *load, struct WorldViewWorld *world)
{
    lua_State *L = load->lua->L;
    int row, id, coordinates[4], i;
    const char *name, *title, *subtitle;
    struct WorldViewRect rect;
    if (load->map_table && *load->map_table) {
        for (row = 1; row < WORLD_VIEW_ROWS; ++row) {
            if (query(load, "querymaptable_load", load->map_table, row, 2, 8, 0) != 1) return 0;
            name = lua_tostring(L, -7);
            if (!name) { lua_pop(L, 8); break; }
            if (!integer(L, -8, &id)) return 0;
            for (i = 0; i < 4; ++i)
                if (!integer(L, i - 6, &coordinates[i])) return 0;
            rect = (struct WorldViewRect){coordinates[0], coordinates[1], coordinates[2], coordinates[3]};
            title = lua_tostring(L, -2);
            subtitle = lua_tostring(L, -1);
            if (strcmp(msg_str(0xbb), session_map_display_name(name)) != 0 &&
                !add_map(world, id, &rect, name, title, subtitle)) return 0;
            lua_pop(L, 8);
        }
    }
    if (load->field_table && *load->field_table) {
        for (row = 1; row < WORLD_VIEW_ROWS; ++row) {
            struct WorldViewArea *area;
            if (query(load, "queryfieldtable_load", load->field_table, row, 2, 7, 0) != 1) return 0;
            name = lua_tostring(L, -2);
            if (!name) { lua_pop(L, 7); break; }
            if (!integer(L, -7, &id)) return 0;
            for (i = 0; i < 4; ++i)
                if (!integer(L, i - 6, &coordinates[i])) return 0;
            area = area_by_id(world, id);
            if (area) {
                const char *source = lua_tostring(L, -1);
                char *label = copy_text(name), *text = copy_text(source);
                if (!label || !text) { free(label); free(text); return 0; }
                free(area->name); free(area->text);
                area->name = label; area->text = text;
                area->source_rect = area->rect = (struct WorldViewRect){coordinates[0], coordinates[1], coordinates[2], coordinates[3]};
            }
            lua_pop(L, 7);
        }
    }
    return 1;
}

static int read_worlds(lua_State *L)
{
    static const char *const scripts[] = {
        "Lua Files\\worldviewdata\\worldviewdata_Language", "Lua Files\\worldviewdata\\worldviewdata_list",
        "Lua Files\\worldviewdata\\worldviewdata_table", "Lua Files\\worldviewdata\\worldviewdata_f",
        "Lua Files\\worldviewdata\\worldviewdata_Info"
    };
    struct LoadContext *load = lua_touserdata(L, 1);
    int i, row;
    for (i = 0; i < 5; ++i)
        if (!client_lua_load_file(load->lua, scripts[i], 1, 0)) return 0;
    for (row = 1; row < WORLD_VIEW_ROWS; ++row) {
        struct WorldViewWorld *world, *worlds;
        const char *name, *texture;
        if (query(load, "queryWorld_List", NULL, row, 1, 4, 0) != 1) return 0;
        name = lua_tostring(L, -4);
        if (!name) { lua_pop(L, 4); break; }
        texture = lua_tostring(L, -1);
        if (!texture) return 0;
        worlds = grow(load->next.worlds, &load->next.world_capacity, load->next.world_count, sizeof *worlds);
        if (!worlds) return 0;
        load->next.worlds = worlds;
        world = &worlds[load->next.world_count++];
        memset(world, 0, sizeof *world);
        world->name = copy_text(name);
        world->texture_name = copy_text(texture);
        load->map_table = copy_text(lua_tostring(L, -3));
        load->field_table = copy_text(lua_tostring(L, -2));
        if (!world->name || !world->texture_name || !load->map_table || !load->field_table) return 0;
        lua_pop(L, 4);
        if (!read_maps(load, world)) return 0;
        free(load->map_table); load->map_table = NULL;
        free(load->field_table); load->field_table = NULL;
    }
    load->ok = read_style(load);
    return 0;
}

int world_view_load(struct WorldView *view, struct ClientLua *lua)
{
    struct LoadContext load = {0};
    int base, result;
    if (!lua || !lua->L) return 0;
    world_view_initialize(&load.next);
    load.lua = lua;
    base = lua_gettop(lua->L);
    result = lua_cpcall(lua->L, read_worlds, &load);
    lua_settop(lua->L, base);
    free(load.map_table);
    free(load.field_table);
    if (result || !load.ok) {
        world_view_clear(&load.next);
        return 0;
    }
    world_view_clear(view);
    *view = load.next;
    return 1;
}
