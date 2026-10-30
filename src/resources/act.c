#include "resources/animation.h"
#include "resources/resource_factories.h"
#include "resources/resource_ops.h"
#include "resources/sprite.h"
#include "fileio/res_mgr.h"
#include "fileio/resource_api.h"
#include "fileio/file.h"
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct AnimationData {
    struct SpriteAction *actions;
    size_t action_count;
    char (*events)[41];
    size_t event_count;
};
struct ActRes {
    struct Res res;
    struct AnimationData data;
};
struct ActReader {
    struct File *file;
    unsigned version;
    const char *sprite_name;
    struct SprRes *sprite;
    int sprite_queried;
};

static void animation_data_clear(struct AnimationData *data)
{
    size_t a, m;
    for (a = 0; a < data->action_count; ++a) {
        struct SpriteAction *action = &data->actions[a];
        for (m = 0; m < action->motion_count; ++m) {
            free(action->motions[m].clips);
            free(action->motions[m].attachments);
        }
        free(action->motions);
    }
    free(data->actions);
    free(data->events);
    memset(data, 0, sizeof *data);
}

struct Res *animation_resource_create(void)
{
    struct ActRes *resource = calloc(1, sizeof *resource);
    if (!resource)
        return NULL;
    res_construct(&resource->res);
    resource->res.ops = &action_resource_ops;
    return &resource->res;
}

static int animation_count(struct File *file, size_t *out, size_t minimum_record)
{
    int32_t count;
    if (!file_read(file, &count, sizeof count) || count < 0
            || (size_t)count > (size_t)(file->size - file->position) / minimum_record)
        return 0;
    *out = (size_t)count;
    return 1;
}

static int animation_center(int value, int size, float scale, int *out)
{
    float result;
    if (size < 0 || !isfinite(scale))
        return 0;
    if (scale == 0.0f) {
        *out = 0;
        return 1;
    }
    result = ((float)value - (float)(size / 2 + size % 2) * scale) / scale;
    if (!isfinite(result) || result < (float)INT_MIN || result >= 2147483648.0f)
        return 0;
    *out = (int)result;
    return 1;
}

static int animation_read_clip(struct ActReader *reader, struct SprClip *clip)
{
    struct File *file = reader->file;
    int32_t values[11] = {0};
    size_t bytes = reader->version < 0x200 ? 16 : reader->version < 0x204 ? 32 : reader->version < 0x205 ? 36 : 44;
    int width, height;
    if (!file_read(file, values, (int)bytes))
        return 0;
    clip->x = values[0]; clip->y = values[1]; clip->sprIndex = values[2];
    clip->flags = (unsigned)values[3];
    if (reader->version < 0x200) {
        memset(clip->color, 255, sizeof clip->color);
        clip->zoomX = clip->zoomY = 1.0f;
        return 1;
    }
    memcpy(clip->color, &values[4], 4);
    memcpy(&clip->zoomX, &values[5], 4);
    if (reader->version < 0x204) {
        clip->zoomY = clip->zoomX;
        clip->angle = values[6]; clip->clipType = values[7];
    } else {
        memcpy(&clip->zoomY, &values[6], 4);
        clip->angle = values[7]; clip->clipType = values[8];
    }
    if (!isfinite(clip->zoomX) || !isfinite(clip->zoomY))
        return 0;
    if (clip->sprIndex < 0)
        return 1;
    if (reader->version >= 0x205) {
        width = values[9]; height = values[10];
    } else {
        struct SprImg *image;
        if (!reader->sprite_queried) {
            reader->sprite = res_mgr_get(g_resMgr(), reader->sprite_name);
            reader->sprite_queried = 1;
        }
        image = sprite_resource_image(reader->sprite, clip->clipType, clip->sprIndex);
        if (!image)
            return 1;
        width = image->width * (image->widthHalved ? 2 : 1);
        height = image->height * (image->heightHalved ? 2 : 1);
    }
    return animation_center(clip->x, width, clip->zoomX, &clip->x)
        && animation_center(clip->y, height, clip->zoomY, &clip->y);
}

static int animation_read_motion(struct ActReader *reader, struct Motion *motion)
{
    struct File *file = reader->file;
    size_t count, i;
    size_t clip_bytes = reader->version < 0x200 ? 16 : reader->version < 0x204 ? 32 : reader->version < 0x205 ? 36 : 44;
    motion->eventId = -1;
    if (!file_read(file, &motion->range1, sizeof motion->range1)
            || !file_read(file, &motion->range2, sizeof motion->range2)
            || !animation_count(file, &count, clip_bytes) || count > SIZE_MAX / sizeof *motion->clips)
        return 0;
    if (count) {
        motion->clips = calloc(count, sizeof *motion->clips);
        if (!motion->clips)
            return 0;
        motion->clip_count = count;
        for (i = 0; i < count; ++i)
            if (!animation_read_clip(reader, &motion->clips[i])) return 0;
    }
    if (reader->version >= 0x200) {
        if (!file_read(file, &motion->eventId, 4)) return 0;
        if (reader->version == 0x200) motion->eventId = -1;
    }
    if (reader->version >= 0x203) {
        if (!animation_count(file, &count, 16) || count > SIZE_MAX / sizeof *motion->attachments)
            return 0;
        if (count) {
            motion->attachments = calloc(count, sizeof *motion->attachments);
            if (!motion->attachments) return 0;
            motion->attachment_count = count;
            for (i = 0; i < count; ++i) {
                int32_t record[4];
                if (!file_read(file, record, sizeof record)) return 0;
                motion->attachments[i].x = record[1];
                motion->attachments[i].y = record[2];
                motion->attachments[i].attr = record[3];
            }
        }
    }
    return 1;
}

static int animation_parse(struct AnimationData *data, struct File *file, const char *name)
{
    uint16_t header[8];
    struct ActReader reader = {0};
    char *sprite_name = NULL;
    size_t a, m, count;
    int ok = 0;
    if (!file_read(file, header, sizeof header)) return 0;
    if (header[0] != 0x4341) {
        error_msg("Illegal file format", 0); error_msg(name, 0);
        return 0;
    }
    if (header[1] < 0x100 || header[1] > 0x205) {
        error_msg("Unsupported Version. Content may be corrupted", 0);
        return 0;
    }
    reader.file = file; reader.version = header[1];
    if (reader.version >= 0x200 && reader.version < 0x205) {
        size_t size = strlen(name);
        size_t stem = size >= 4 ? size - 4 : size;
        if (stem > SIZE_MAX - 5) return 0;
        sprite_name = malloc(stem + 5);
        if (!sprite_name) return 0;
        memcpy(sprite_name, name, stem); memcpy(sprite_name + stem, ".spr", 5);
        reader.sprite_name = sprite_name;
    }
    count = header[2];
    if (count > (size_t)(file->size - file->position) / 4) goto done;
    if (count) {
        data->actions = calloc(count, sizeof *data->actions);
        if (!data->actions) goto done;
        data->action_count = count;
        for (a = 0; a < count; ++a) {
            struct SpriteAction *action = &data->actions[a];
            size_t motions, minimum = reader.version < 0x200 ? 36 : reader.version < 0x203 ? 40 : 44;
            action->delay = 4.0f;
            if (!animation_count(file, &motions, minimum) || motions > SIZE_MAX / sizeof *action->motions) goto done;
            if (!motions) continue;
            action->motions = calloc(motions, sizeof *action->motions);
            if (!action->motions) goto done;
            action->motion_count = motions;
            for (m = 0; m < motions; ++m)
                if (!animation_read_motion(&reader, &action->motions[m])) goto done;
        }
    }
    if (reader.version >= 0x201) {
        if (!animation_count(file, &count, 40) || count > SIZE_MAX / sizeof *data->events) goto done;
        if (count) {
            data->events = calloc(count, sizeof *data->events);
            if (!data->events) goto done;
            data->event_count = count;
            for (a = 0; a < count; ++a)
                if (!file_read(file, data->events[a], 40)) goto done;
        }
    }
    if (reader.version >= 0x202) {
        for (a = 0; a < data->action_count; ++a) {
            float delay;
            if (!file_read(file, &delay, 4) || !isfinite(delay) || delay <= 0) goto done;
            data->actions[a].delay = delay;
        }
    }
    ok = 1;
done:
    free(sprite_name);
    return ok;
}

static int action_resource_load(struct Res *base, const char *name)
{
    struct ActRes *resource = (struct ActRes *)base;
    struct AnimationData replacement = {0};
    struct File file;
    int ok = 0;
    if (!name) return 0;
    file_construct(&file);
    if (file_open(&file, name, CFILE_MODE_READ, 1, 0)) {
        if (animation_parse(&replacement, &file, name)) {
            animation_data_clear(&resource->data);
            resource->data = replacement;
            memset(&replacement, 0, sizeof replacement);
            ok = 1;
        }
    } else {
        size_t size = strlen(name);
        const char prefix[] = "Cannot find File : ";
        if (size <= SIZE_MAX - sizeof prefix) {
            char *message = malloc(size + sizeof prefix);
            if (message) {
                memcpy(message, prefix, sizeof prefix - 1);
                memcpy(message + sizeof prefix - 1, name, size + 1);
                error_msg(message, 0);
                free(message);
            }
        }
    }
    animation_data_clear(&replacement);
    file_destruct(&file);
    return ok;
}

static void action_resource_destroy(struct Res *base)
{
    animation_data_clear(&((struct ActRes *)base)->data);
    free(base);
}

size_t animation_action_count(const struct ActRes *resource)
{
    return resource ? resource->data.action_count : 0;
}

struct SpriteAction *act_res_get_action(const struct ActRes *resource, unsigned action)
{
    static struct SpriteAction empty = {NULL, 0, 4.0f};
    return resource && action < resource->data.action_count ? &resource->data.actions[action] : &empty;
}

struct Motion *act_res_get_motion(const struct ActRes *resource, unsigned action, unsigned frame)
{
    static struct Motion empty = {.eventId = -1};
    struct SpriteAction *value = act_res_get_action(resource, action);
    if (!value->motion_count) return &empty;
    return &value->motions[frame < value->motion_count ? frame : 0];
}

struct SprClip *motion_get_clip(const struct Motion *motion, unsigned index)
{
    static struct SprClip empty = {.sprIndex = -1, .color = {255,255,255,255}, .zoomX = 1.0f, .zoomY = 1.0f};
    return motion && index < motion->clip_count ? &motion->clips[index] : &empty;
}

float act_res_get_delay(const struct ActRes *resource, unsigned action)
{
    return act_res_get_action(resource, action)->delay;
}

const char *act_res_get_event_name(const struct ActRes *resource, unsigned event)
{
    return resource && event < resource->data.event_count ? resource->data.events[event] : "";
}

unsigned actres_frame_count(const struct ActRes *resource, unsigned action)
{
    size_t count = act_res_get_action(resource, action)->motion_count;
    return count ? (unsigned)count : 1;
}

const struct ResourceOps action_resource_ops = {
    .destroy = action_resource_destroy,
    .load = action_resource_load,
};
