#include "resources/world_res.h"
#include "resources/world_format.h"
#include "fileio/file.h"
#include "render/math3d.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void (*g_worldResLight)(const float ambient[3], const float diffuse[3]);

#define RSW_AT_LEAST_1(major, minor, n) ((major) == 1 ? (minor) >= (n) : (major) >= 2)

static void world_res_defaults(struct C3dWorldRes *self)
{
    memset(self->gndFile, 0, sizeof self->gndFile);
    memset(self->gatFile, 0, sizeof self->gatFile);
    memset(self->srcFile, 0, sizeof self->srcFile);
    self->waterLevel = 0;
    self->waterType = 0;
    self->waveHeight = 1;
    self->waveSpeed = 2;
    self->wavePitch = 50;
    self->waterAnimSpeed = 3;
    self->lightLatitude = self->lightLongitude = 45;
    memset(self->lightDir, 0, sizeof self->lightDir);
    self->diffuse[0] = self->diffuse[1] = self->diffuse[2] = 1;
    self->ambient[0] = self->ambient[1] = self->ambient[2] = 0.3f;
    self->shadowOpacity = 0.5f;
    self->major = self->minor = self->twoPassAlpha = 0;
    self->bounds[0] = self->bounds[2] = -500;
    self->bounds[1] = self->bounds[3] = 500;
}

static void world_res_init(struct C3dWorldRes *self)
{
    memset(self, 0, sizeof *self);
    res_construct(&self->res);
    self->res.ops = &world_resource_ops;
    world_res_defaults(self);
}

struct Res *world_resource_create(void)
{
    struct C3dWorldRes *self = calloc(1, sizeof *self);
    if (!self)
        return NULL;
    world_res_init(self);
    return &self->res;
}

void world_resource_clear(struct C3dWorldRes *self)
{
    free(self->models);
    free(self->effects);
    free(self->sounds);
    free(self->tree);
    self->models = NULL;
    self->effects = NULL;
    self->sounds = NULL;
    self->tree = NULL;
    self->modelCount = self->effectCount = self->soundCount = 0;
    world_res_defaults(self);
}

void world_resource_finish(struct C3dWorldRes *self)
{
    world_resource_clear(self);
    res_destruct(&self->res);
}

static void *grow_records(void *records, size_t *capacity, size_t element_size)
{
    size_t maximum = SIZE_MAX / element_size;
    size_t next;
    void *grown;
    if (*capacity == maximum)
        return NULL;
    next = !*capacity ? 16 : *capacity > maximum / 2 ? maximum : *capacity * 2;
    grown = realloc(records, next * element_size);
    if (grown)
        *capacity = next;
    return grown;
}

static void copy_text(char *out, const char *data, size_t width)
{
    memcpy(out, data, width);
    out[width] = 0;
}

static int read_name(struct File *file, char *out, int width)
{
    if (!file_read(file, out, width))
        return 0;
    out[width] = 0;
    return 1;
}

BYTE c3d_world_res_load(struct C3dWorldRes *self, const char *name)
{
    struct File fp;
    char magic[4], message[256];
    BYTE major, minor;
    int build = 0, count, i, type;
    size_t model_capacity = 0, effect_capacity = 0, sound_capacity = 0;
    float matrix[12];

    world_resource_clear(self);
    file_construct(&fp);
    if (!file_open(&fp, name, 0, 1, 0))
        goto fail;
    if (!file_read(&fp, magic, sizeof magic) || memcmp(magic, "GRSW", sizeof magic)) {
        error_msg("Invalid Format", 0);
        goto fail;
    }
    if (!file_read(&fp, &major, 1) || !file_read(&fp, &minor, 1))
        goto fail;
    if (major > 2 || (major == 2 && minor > 6)) {
        snprintf(message, sizeof message,
            "Unsupported Version\n current version: %d.%d < %d.%d", 2, 6, major, minor);
        error_msg(message, 0);
        goto fail;
    }
    self->major = major;
    self->minor = minor;
    if (major == 2 && minor >= 5 && !file_read(&fp, &build, sizeof build))
        goto fail;
    if (major == 2 && minor >= 2 && !file_read(&fp, &self->twoPassAlpha, 1))
        goto fail;
    if (!file_consume(&fp, 1, 40) || !read_name(&fp, self->gndFile, 40))
        goto fail;
    if (RSW_AT_LEAST_1(major, minor, 4) && !read_name(&fp, self->gatFile, 40))
        goto fail;
    if (!read_name(&fp, self->srcFile, 40))
        goto fail;

    if (!(major == 2 && minor >= 6)) {
        if (RSW_AT_LEAST_1(major, minor, 3) && !file_read(&fp, &self->waterLevel, 4))
            goto fail;
        if (RSW_AT_LEAST_1(major, minor, 8)
                && (!file_read(&fp, &self->waterType, 4)
                    || !file_read(&fp, &self->waveHeight, 4)
                    || !file_read(&fp, &self->waveSpeed, 4)
                    || !file_read(&fp, &self->wavePitch, 4)))
            goto fail;
        if (RSW_AT_LEAST_1(major, minor, 9) && !file_read(&fp, &self->waterAnimSpeed, 4))
            goto fail;
    }
    if (RSW_AT_LEAST_1(major, minor, 5)
            && (!file_read(&fp, &self->lightLatitude, 4)
                || !file_read(&fp, &self->lightLongitude, 4)
                || !file_read(&fp, self->diffuse, sizeof self->diffuse)
                || !file_read(&fp, self->ambient, sizeof self->ambient)))
        goto fail;
    mat_set_rotate_x(matrix, (float)self->lightLongitude / 180.0f * 3.141592f);
    mat_rotate_y(matrix, (float)self->lightLatitude / 180.0f * 3.141592f);
    self->lightDir[0] = matrix[0] * 0.0f + matrix[3] + matrix[6] * 0.0f + matrix[9];
    self->lightDir[1] = matrix[1] * 0.0f + matrix[4] + matrix[7] * 0.0f + matrix[10];
    self->lightDir[2] = matrix[2] * 0.0f + matrix[5] + matrix[8] * 0.0f + matrix[11];
    if (RSW_AT_LEAST_1(major, minor, 7) && !file_read(&fp, &self->shadowOpacity, 4))
        goto fail;
    if (RSW_AT_LEAST_1(major, minor, 6) && !file_read(&fp, self->bounds, sizeof self->bounds))
        goto fail;
    if (!file_read(&fp, &count, 4) || count < 0 || count > (fp.size - fp.position) / 4)
        goto fail;
    for (i = 0; i < count; ++i) {
        if (!file_read(&fp, &type, 4))
            goto fail;
        switch (type) {
        case 1: {
            struct RswModel model = {0};
            struct RswModelRec record;
            model.animSpeed = 1;
            if (RSW_AT_LEAST_1(major, minor, 3)) {
                if (!read_name(&fp, model.name, 40)
                        || !file_read(&fp, &model.animType, 4)
                        || !file_read(&fp, &model.animSpeed, 4)
                        || !file_read(&fp, &model.blockType, 4))
                    goto fail;
                if (major == 2 && minor >= 6 && (unsigned)build >= 186
                        && !file_read(&fp, &model.flag, 1))
                    goto fail;
                model.flag = model.flag != 0;
                if (!isfinite(model.animSpeed) || model.animSpeed > 100 || model.animSpeed <= 0)
                    model.animSpeed = 1;
            }
            if (!file_read(&fp, &record, sizeof record))
                goto fail;
            copy_text(model.fileName, record.fileName, sizeof record.fileName);
            copy_text(model.nodeName, record.nodeName, sizeof record.nodeName);
            memcpy(model.pos, record.pos, sizeof model.pos);
            memcpy(model.rot, record.rot, sizeof model.rot);
            memcpy(model.scale, record.scale, sizeof model.scale);
            if (self->modelCount == model_capacity) {
                void *storage = grow_records(self->models, &model_capacity, sizeof *self->models);
                if (!storage) goto fail;
                self->models = storage;
            }
            self->models[self->modelCount++] = model;
            break;
        }
        case 2:
            if (!file_consume(&fp, 1, sizeof(struct RswLightRec)))
                goto fail;
            break;
        case 3: {
            struct RswSound sound;
            struct RswSoundRec record = {0};
            if (!file_read(&fp, &record, major >= 2 ? sizeof record : offsetof(struct RswSoundRec, cycle)))
                goto fail;
            if (major < 2) record.cycle = 4;
            copy_text(sound.name, record.name, sizeof record.name);
            copy_text(sound.waveName, record.waveName, sizeof record.waveName);
            memcpy(sound.pos, record.pos, sizeof sound.pos);
            sound.volume = record.volume;
            sound.width = record.width; sound.height = record.height;
            sound.range = record.range; sound.cycle = record.cycle;
            if (self->soundCount == sound_capacity) {
                void *storage = grow_records(self->sounds, &sound_capacity, sizeof *self->sounds);
                if (!storage) goto fail;
                self->sounds = storage;
            }
            self->sounds[self->soundCount++] = sound;
            break;
        }
        case 4: {
            struct RswEffect effect;
            struct RswEffectRec record;
            if (!file_read(&fp, &record, sizeof record))
                goto fail;
            copy_text(effect.name, record.name, sizeof record.name);
            memcpy(effect.pos, record.pos, sizeof effect.pos);
            effect.id = record.id; effect.emitSpeed = record.emitSpeed;
            memcpy(effect.param, record.param, sizeof effect.param);
            if (self->effectCount == effect_capacity) {
                void *storage = grow_records(self->effects, &effect_capacity, sizeof *self->effects);
                if (!storage) goto fail;
                self->effects = storage;
            }
            self->effects[self->effectCount++] = effect;
            break;
        }
        default:
            break;
        }
    }
    if (major == 2 && minor >= 1) {
        const BYTE *boxes = file_consume(&fp, RSW_TREE_NODE_COUNT, sizeof *self->tree);
        if (!boxes)
            goto fail;
        self->tree = calloc(RSW_TREE_NODE_COUNT, sizeof *self->tree);
        if (!self->tree)
            goto fail;
        memcpy(self->tree, boxes, RSW_TREE_NODE_COUNT * sizeof *self->tree);
    }
    file_destruct(&fp);
    if (g_worldResLight)
        g_worldResLight(self->ambient, self->diffuse);
    return 1;
fail:
    file_destruct(&fp);
    world_resource_clear(self);
    return 0;
}

static void world_resource_destroy(struct Res *resource)
{
    struct C3dWorldRes *self = (struct C3dWorldRes *)resource;
    world_resource_finish(self);
    free(self);
}
static int world_resource_load(struct Res *resource, const char *name)
{
    return c3d_world_res_load((struct C3dWorldRes *)resource, name);
}
const struct ResourceOps world_resource_ops = {
    .destroy = world_resource_destroy,
    .load = world_resource_load,
};
