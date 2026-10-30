#include "resources/resource_factories.h"
#include "resources/resource_ops.h"
#include "fileio/res_mgr.h"
#include "fileio/file.h"
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct RmaAction {
    int index;
    int a, b, c;
    int *frames;
    size_t frame_count;
    size_t name_index;
};
struct RmaData {
    char *text;
    char **names;
    size_t name_count;
    struct RmaAction *actions;
    size_t action_count;
    float interior_zoom, shadow_zoom;
    int interior_offset_x, interior_offset_y;
    int interior_frames[2];
};
struct RmaRes {
    struct Res res;
    struct RmaData data;
};

static void rma_data_clear(struct RmaData *data)
{
    size_t i;
    for (i = 0; i < data->action_count; ++i)
        free(data->actions[i].frames);
    free(data->actions);
    free(data->names);
    free(data->text);
    memset(data, 0, sizeof *data);
    data->interior_zoom = data->shadow_zoom = 1.0f;
}

struct Res *rma_resource_create(void)
{
    struct RmaRes *resource = calloc(1, sizeof *resource);
    if (!resource)
        return NULL;
    res_construct(&resource->res);
    resource->res.ops = &rma_resource_ops;
    resource->data.interior_zoom = resource->data.shadow_zoom = 1.0f;
    return &resource->res;
}

static void *rma_reserve(void *items, size_t *capacity, size_t count, size_t size)
{
    size_t next;
    void *grown;
    if (count <= *capacity)
        return items;
    if (count > SIZE_MAX / size)
        return NULL;
    next = *capacity ? *capacity : 4;
    while (next < count) {
        if (next > SIZE_MAX / size / 2) {
            next = count;
            break;
        }
        next *= 2;
    }
    grown = realloc(items, next * size);
    if (grown)
        *capacity = next;
    return grown;
}

static int rma_space(unsigned char c)
{
    return c < 128 && isspace(c);
}

static char *rma_word(char **cursor)
{
    char *start, *at = *cursor;
    while (*at && rma_space((unsigned char)*at))
        ++at;
    start = at;
    while (*at && !rma_space((unsigned char)*at)) {
        if (at[0] == '/' && at[1] == '/') {
            *start = 0;
            *cursor = start;
            return NULL;
        }
        ++at;
    }
    if (*at)
        *at++ = 0;
    *cursor = at;
    return *start ? start : NULL;
}

static int rma_integer_word(const char *word, int *out)
{
    char *end;
    long value;
    if (!word)
        return 0;
    errno = 0;
    value = strtol(word, &end, 10);
    if (word == end || *end || errno == ERANGE || value < INT_MIN || value > INT_MAX)
        return 0;
    *out = (int)value;
    return 1;
}

static int rma_integer(char **cursor, int *out)
{
    return rma_integer_word(rma_word(cursor), out);
}

static int rma_number(char **cursor, float *out)
{
    char *word = rma_word(cursor), *end;
    double value;
    if (!word)
        return 0;
    errno = 0;
    value = strtod(word, &end);
    if (word == end || *end || errno == ERANGE || !isfinite(value) || fabs(value) > FLT_MAX)
        return 0;
    *out = (float)value;
    return 1;
}

static struct RmaAction *rma_find_action(struct RmaData *data, int index)
{
    size_t i;
    for (i = 0; i < data->action_count; ++i)
        if (data->actions[i].index == index)
            return &data->actions[i];
    return NULL;
}

static int rma_action(struct RmaData *data, char *cursor, size_t *capacity)
{
    struct RmaAction value = {0}, *action;
    size_t frames_capacity = 0;
    if (!data->name_count || !rma_integer(&cursor, &value.index) || value.index < 0
            || !rma_integer(&cursor, &value.c) || !rma_integer(&cursor, &value.a)
            || !rma_integer(&cursor, &value.b))
        return 0;
    for (;;) {
        char *word = rma_word(&cursor);
        int frame, *frames;
        if (!word)
            break;
        if (!rma_integer_word(word, &frame)) {
            free(value.frames);
            return 0;
        }
        frames = rma_reserve(value.frames, &frames_capacity, value.frame_count + 1, sizeof *frames);
        if (!frames) {
            free(value.frames);
            return 0;
        }
        value.frames = frames;
        value.frames[value.frame_count++] = frame;
    }
    value.name_index = data->name_count - 1;
    action = rma_find_action(data, value.index);
    if (action) {
        if (action->a != -1)
            error_msg("중복되는 액션이 있음", 0);
        if (!value.frame_count) {
            value.frames = action->frames;
            value.frame_count = action->frame_count;
        } else {
            free(action->frames);
        }
    } else {
        struct RmaAction *actions = rma_reserve(data->actions, capacity, data->action_count + 1, sizeof *actions);
        if (!actions) {
            free(value.frames);
            return 0;
        }
        data->actions = actions;
        action = &actions[data->action_count++];
    }
    *action = value;
    return 1;
}

static int rma_parse(struct RmaData *data, const char *text, size_t size)
{
    char *line;
    size_t names_capacity = 0, actions_capacity = 0;
    if ((!text && size) || size == SIZE_MAX)
        return 0;
    data->text = malloc(size + 1);
    if (!data->text)
        return 0;
    if (size)
        memcpy(data->text, text, size);
    data->text[size] = 0;
    line = data->text;
    while (*line) {
        char *next = strchr(line, '\n'), *cursor = line, *directive;
        if (next)
            *next++ = 0;
        directive = rma_word(&cursor);
        if (!directive) {
        } else if (strcmp(directive, "#rsxname") == 0) {
            char *name = rma_word(&cursor), **names;
            if (!name)
                return 0;
            names = rma_reserve(data->names, &names_capacity, data->name_count + 1, sizeof *names);
            if (!names)
                return 0;
            data->names = names;
            names[data->name_count++] = name;
        } else if (strcmp(directive, "#action") == 0) {
            if (!rma_action(data, cursor, &actions_capacity))
                return 0;
        } else if (strcmp(directive, "#interiorzoom") == 0) {
            if (!rma_number(&cursor, &data->interior_zoom)) return 0;
        } else if (strcmp(directive, "#shadowzoom") == 0) {
            if (!rma_number(&cursor, &data->shadow_zoom)) return 0;
        } else if (strcmp(directive, "#interioroffsetx") == 0) {
            if (!rma_integer(&cursor, &data->interior_offset_x)) return 0;
        } else if (strcmp(directive, "#interioroffsety") == 0) {
            if (!rma_integer(&cursor, &data->interior_offset_y)) return 0;
        } else if (strcmp(directive, "#interiorframes") == 0) {
            if (!rma_integer(&cursor, &data->interior_frames[0])
                    || !rma_integer(&cursor, &data->interior_frames[1])) return 0;
        } else {
            return 0;
        }
        if (!next)
            break;
        line = next;
    }
    return 1;
}

static int rma_resource_load(struct Res *base, const char *name)
{
    struct RmaRes *resource = (struct RmaRes *)base;
    struct RmaData replacement = {0};
    struct File file;
    int opened, loaded = 0;
    if (!name)
        return 0;
    replacement.interior_zoom = replacement.shadow_zoom = 1.0f;
    file_construct(&file);
    opened = file_open(&file, name, CFILE_MODE_READ, 1, 0);
    if (opened && file.size >= 0)
        loaded = rma_parse(&replacement, (const char *)file.buffer, (size_t)file.size);
    file_destruct(&file);
    if (loaded) {
        rma_data_clear(&resource->data);
        resource->data = replacement;
    } else {
        rma_data_clear(&replacement);
        if (opened)
            error_msg("Syntax Error in Rma", 0);
    }
    return loaded;
}

static void rma_resource_destroy(struct Res *base)
{
    rma_data_clear(&((struct RmaRes *)base)->data);
    free(base);
}

const struct ResourceOps rma_resource_ops = {
    .destroy = rma_resource_destroy,
    .load = rma_resource_load,
};
