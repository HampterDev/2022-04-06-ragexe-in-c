#include "contents/map_info_api.h"
#include "contents/map_info.h"
#include "core/msg_str.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct MapInfo {
    char *map;
    char *display_name;
    char *sign_line1;
    char *sign_line2;
    char *ornament;
    char show_banner;
};

struct MapInfoMgr {
    struct MapInfo *maps;
    size_t count;
    size_t capacity;
    char *default_name;
    struct MapNameDisplay *banner;
};

struct MapInfoMgr *g_mapInfo;

static const char *text_or_empty(const char *text)
{
    return text ? text : "";
}

static char *copy_text(const char *text)
{
    size_t size = strlen(text) + 1;
    char *copy = malloc(size);
    if (copy)
        memcpy(copy, text, size);
    return copy;
}

static void free_banner(struct MapInfoMgr *self)
{
    if (self->banner) {
        map_name_display_destruct(self->banner);
        free(self->banner);
        self->banner = NULL;
    }
}

static struct MapInfo *info_at(struct MapInfoMgr *self, const char *map)
{
    size_t first = 0, count = self->count;
    char *key;
    while (count) {
        size_t half = count / 2, at = first + half;
        int order = strcmp(self->maps[at].map, map);
        if (!order)
            return &self->maps[at];
        if (order < 0) {
            first = at + 1;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    key = copy_text(map);
    if (!key)
        return NULL;
    if (self->count == self->capacity) {
        size_t limit = SIZE_MAX / sizeof *self->maps;
        size_t capacity = self->capacity ? self->capacity : 32;
        struct MapInfo *maps;
        if (self->count == limit) {
            free(key);
            return NULL;
        }
        capacity = capacity > limit / 2 ? limit : capacity * 2;
        maps = realloc(self->maps, capacity * sizeof *maps);
        if (!maps) {
            free(key);
            return NULL;
        }
        self->maps = maps;
        self->capacity = capacity;
    }
    memmove(self->maps + first + 1, self->maps + first,
            (self->count - first) * sizeof *self->maps);
    self->maps[first] = (struct MapInfo){0};
    self->maps[first].map = key;
    ++self->count;
    return &self->maps[first];
}

static int compare_map(const char *key, const char *map, size_t skip)
{
    size_t i = 0;
    for (;;) {
        unsigned char a = (unsigned char)*key++;
        unsigned char b;
        if (i == skip)
            i += 3;
        b = (unsigned char)map[i++];
        if (a != b || !a)
            return (int)a - (int)b;
    }
}

static const struct MapInfo *find_info(const struct MapInfoMgr *self, const char *map)
{
    const char *at = strrchr(map, '@');
    size_t skip = at && at - map >= 4 ? (size_t)(at - map) - 4 : SIZE_MAX;
    size_t first = 0, count = self ? self->count : 0;
    while (count) {
        size_t half = count / 2, index = first + half;
        int order = compare_map(self->maps[index].map, map, skip);
        if (!order)
            return &self->maps[index];
        if (order < 0) {
            first = index + 1;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    return NULL;
}

int map_info_set_display_name(struct MapInfoMgr *self, const char *map,
                              const char *name, char show_banner)
{
    char *copy;
    struct MapInfo *info;
    if (!self)
        return 0;
    copy = copy_text(name);
    if (!copy)
        return 0;
    info = info_at(self, map);
    if (!info) {
        free(copy);
        return 0;
    }
    free(info->display_name);
    info->display_name = copy;
    info->show_banner = show_banner;
    return 1;
}

const char *map_info_display_name(const struct MapInfoMgr *self, const char *map)
{
    const struct MapInfo *info = find_info(self, map);
    return text_or_empty(info ? info->display_name : self ? self->default_name : NULL);
}

void map_info_show_banner(struct MapInfoMgr *self, const char *map)
{
    const struct MapInfo *info = find_info(self, map);
    if (info && info->show_banner) {
        const char *ornament = text_or_empty(info->ornament);
        const char *line1 = text_or_empty(info->sign_line1);
        const char *line2 = text_or_empty(info->sign_line2);
        struct MapNameDisplay *banner = malloc(sizeof *banner);
        if (!banner)
            return;
        if (!*ornament)
            ornament = "noname";
        if (!*line1 && !*line2)
            line2 = text_or_empty(self->default_name);
        if (!map_name_display_construct(banner, line1, line2, ornament)) {
            free(banner);
            return;
        }
        free_banner(self);
        self->banner = banner;
    }
}

void map_info_render_banner(struct MapInfoMgr *self)
{
    if (self && self->banner && !map_name_display_render(self->banner))
        free_banner(self);
}

char map_info_load(struct MapInfoMgr *self)
{
    char *name;
    if (!self)
        return 0;
    name = copy_text(msg_str(0xbb));
    if (!name)
        return 0;
    free(self->default_name);
    self->default_name = name;
    return 0;
}

char map_info_create(void)
{
    if (g_mapInfo)
        return 0;
    g_mapInfo = calloc(1, sizeof *g_mapInfo);
    return g_mapInfo != NULL;
}

void map_info_destroy(void)
{
    struct MapInfoMgr *self = g_mapInfo;
    if (!self)
        return;
    g_mapInfo = NULL;
    free_banner(self);
    for (size_t i = 0; i < self->count; ++i) {
        struct MapInfo *info = &self->maps[i];
        free(info->map);
        free(info->display_name);
        free(info->sign_line1);
        free(info->sign_line2);
        free(info->ornament);
    }
    free(self->maps);
    free(self->default_name);
    free(self);
}
