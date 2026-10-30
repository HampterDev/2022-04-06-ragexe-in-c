#include "resources/imf.h"
#include "resources/resource_factories.h"
#include "resources/resource_ops.h"
#include "fileio/res_mgr.h"
#include "fileio/file.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct ImfFrame {
    int order;
    POINT position;
};
struct ImfAction {
    struct ImfFrame *frames;
    size_t count;
};
struct ImfLayer {
    struct ImfAction *actions;
    size_t count;
};
struct ImfRes {
    struct Res res;
    struct ImfLayer *layers;
    size_t count;
};

struct Res *imf_resource_create(void)
{
    struct ImfRes *resource = calloc(1, sizeof *resource);
    if (!resource)
        return NULL;
    res_construct(&resource->res);
    resource->res.ops = &imf_resource_ops;
    return &resource->res;
}

static void imf_resource_reset(struct ImfRes *resource)
{
    size_t layer;
    for (layer = 0; layer < resource->count; ++layer) {
        struct ImfLayer *item = &resource->layers[layer];
        size_t action;
        for (action = 0; action < item->count; ++action)
            free(item->actions[action].frames);
        free(item->actions);
    }
    free(resource->layers);
    resource->layers = NULL;
    resource->count = 0;
}

static int imf_count(struct File *file, size_t *count, size_t record_size)
{
    int32_t value;
    if (!file_read(file, &value, sizeof value) || value < 0
            || (size_t)value > (size_t)(file->size - file->position) / record_size)
        return 0;
    *count = (size_t)value;
    return 1;
}

static int imf_parse(struct ImfRes *resource, struct File *file)
{
    struct { float version; int32_t checksum, last_layer; } header;
    size_t layers, layer;
    if (!file_read(file, &header, sizeof header) || !isfinite(header.version)
            || header.version < 1.01f || header.last_layer < -1)
        return 0;
    layers = (size_t)(header.last_layer + (int64_t)1);
    if (layers > (size_t)(file->size - file->position) / sizeof(int32_t)
            || layers > SIZE_MAX / sizeof *resource->layers)
        return 0;
    if (!layers)
        return 1;
    resource->layers = calloc(layers, sizeof *resource->layers);
    if (!resource->layers)
        return 0;
    resource->count = layers;
    for (layer = 0; layer < layers; ++layer) {
        struct ImfLayer *item = &resource->layers[layer];
        size_t actions, action;
        if (!imf_count(file, &actions, sizeof(int32_t)) || actions > SIZE_MAX / sizeof *item->actions)
            return 0;
        if (!actions)
            continue;
        item->actions = calloc(actions, sizeof *item->actions);
        if (!item->actions)
            return 0;
        item->count = actions;
        for (action = 0; action < actions; ++action) {
            struct ImfAction *clip = &item->actions[action];
            size_t frames, frame;
            const BYTE *data;
            if (!imf_count(file, &frames, 12) || frames > SIZE_MAX / sizeof *clip->frames)
                return 0;
            if (!frames)
                continue;
            data = file_consume(file, frames, 12);
            if (!data)
                return 0;
            clip->frames = calloc(frames, sizeof *clip->frames);
            if (!clip->frames)
                return 0;
            clip->count = frames;
            for (frame = 0; frame < frames; ++frame, data += 12) {
                int32_t values[3];
                memcpy(values, data, sizeof values);
                clip->frames[frame].order = values[0];
                clip->frames[frame].position.x = values[1];
                clip->frames[frame].position.y = values[2];
            }
        }
    }
    return 1;
}

static int imf_resource_load(struct Res *base, const char *name)
{
    struct ImfRes *resource = (struct ImfRes *)base, replacement = {0};
    struct File file;
    int ok = 0;
    if (!name)
        return 0;
    file_construct(&file);
    if (file_open(&file, name, CFILE_MODE_READ, 1, 0) && imf_parse(&replacement, &file)) {
        imf_resource_reset(resource);
        resource->layers = replacement.layers;
        resource->count = replacement.count;
        replacement.layers = NULL;
        replacement.count = 0;
        ok = 1;
    }
    imf_resource_reset(&replacement);
    file_destruct(&file);
    return ok;
}

static void imf_resource_destroy(struct Res *base)
{
    imf_resource_reset((struct ImfRes *)base);
    free(base);
}

static const struct ImfFrame *imf_frame(const struct ImfRes *resource, int layer, int action, int frame)
{
    const struct ImfLayer *item;
    const struct ImfAction *clip;
    if (!resource || layer < 0 || (size_t)layer >= resource->count || action < 0 || frame < 0)
        return NULL;
    item = &resource->layers[layer];
    if ((size_t)action >= item->count)
        return NULL;
    clip = &item->actions[action];
    return (size_t)frame < clip->count ? &clip->frames[frame] : NULL;
}

int imf_resource_layer(const struct ImfRes *resource, int order, int action, int frame)
{
    size_t layer;
    if (!imf_frame(resource, order, action, frame))
        return -1;
    for (layer = 0; layer < resource->count; ++layer) {
        const struct ImfFrame *value = imf_frame(resource, (int)layer, action, frame);
        if (value && value->order == order)
            return (int)layer;
    }
    return -1;
}

POINT imf_resource_position(const struct ImfRes *resource, int layer, int action, int frame)
{
    const struct ImfFrame *value = imf_frame(resource, layer, action, frame);
    POINT zero = {0, 0};
    return value ? value->position : zero;
}

const struct ResourceOps imf_resource_ops = {
    .destroy = imf_resource_destroy,
    .load = imf_resource_load,
};
