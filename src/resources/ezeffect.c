#include "render/texture_cache.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fileio/file.h"
#include "resources/ezeffect.h"

static void str_release_textures(struct StrAnim *anim)
{
    int i, j;
    for (i = STR_LAYERS; i-- > 0;) {
        struct StrLayer *layer = &anim->layers[i];
        for (j = 0; j < STR_LAYER_TEXTURES; ++j) {
            if (layer->textures[j]) {
                texture_unlock(layer->textures[j]);
                layer->textures[j] = NULL;
            }
        }
    }
}

static void ezeffect_init(struct EzEffectRes *res)
{
    res_construct(&res->res);
    res->res.ops = &effect_resource_ops;
    file_construct(&res->file);
    memset(&res->anim, 0, sizeof res->anim);
    file_close(&res->file);
}

struct Res *effect_resource_create(void)
{
    struct EzEffectRes *res = malloc(sizeof *res);

    if (!res)
        return NULL;
    ezeffect_init(res);
    return &res->res;
}

BYTE e_zeffect_res_load(struct EzEffectRes *self, const char *name)
{
    struct File *fp = &self->file;
    int magic, version, metadata, i;

    effect_resource_clear(self);
    if (!file_open(fp, name, 0, 1, 0)) {
        char text[512];
        snprintf(text, sizeof text, "Str::Cannot find File : %s", name ? name : "");
        error_msg(text, 0);
        return 0;
    }
    if (!file_read(fp, &magic, 4) || !file_read(fp, &version, 4)
            || !file_read(fp, &metadata, 4)
            || !file_read(fp, &self->anim.frame_count, 4)
            || !file_read(fp, &self->anim.layer_count, 4)
            || !file_seek(fp, 0x10, FILE_CURRENT))
        goto invalid;
    if (version != STR_VERSION) {
        error_msg("LoadAniClip: Version is incorrect", 0);
        goto invalid;
    }
    if (self->anim.frame_count < 0 || self->anim.layer_count < 0
            || self->anim.layer_count > STR_LAYERS)
        goto invalid;
    for (i = 0; i < self->anim.layer_count; i++) {
        struct StrLayer *layer = &self->anim.layers[i];
        const BYTE *names;
        int j;

        if (!file_read(fp, &layer->texture_count, 4)
                || layer->texture_count < 0 || layer->texture_count > STR_LAYER_TEXTURES)
            goto invalid;
        names = file_consume(fp, (size_t)layer->texture_count, 0x80);
        if (!names)
            goto invalid;
        for (j = 0; j < layer->texture_count; j++) {
            const char *texture = (const char *)names + j * 0x80;
            if (!memchr(texture, 0, 0x80))
                goto invalid;
            layer->texture_names[j] = texture;
        }
        for (; j < STR_LAYER_TEXTURES; j++)
            layer->texture_names[j] = "1.bmp";
        if (!file_read(fp, &layer->key_count, 4) || layer->key_count < 0)
            goto invalid;
        if (layer->key_count) {
            layer->keys = (const struct StrKey *)file_consume(fp, (size_t)layer->key_count, sizeof *layer->keys);
            if (!layer->keys)
                goto invalid;
        }
    }
    return 1;
invalid:
    effect_resource_clear(self);
    return 0;
}

void effect_resource_clear(struct EzEffectRes *self)
{
    str_release_textures(&self->anim);
    memset(&self->anim, 0, sizeof self->anim);
    file_close(&self->file);
}

void effect_resource_finish(struct EzEffectRes *self)
{
    file_destruct(&self->file);
    str_release_textures(&self->anim);
    res_destruct(&self->res);
}

static void *layer_fetch(struct StrLayer *layer, int index, const char *folder)
{
    char path[256];
    int length = snprintf(path, sizeof path, "effect\\%s%s", folder, layer->texture_names[index]);
    if (length < 0 || (size_t)length >= sizeof path)
        return &g_whiteTexture;
    if (!strchr(path, '.')) {
        if ((size_t)length + sizeof ".bmp" > sizeof path)
            return &g_whiteTexture;
        memcpy(path + length, ".bmp", sizeof ".bmp");
    }
    layer->textures[index] = texture_cache_get(&g_textureCache, path, 1, 0);
    if (!layer->textures[index])
        return &g_whiteTexture;
    texture_lock(layer->textures[index]);
    return layer->textures[index];
}

void *str_layer_get_texture(struct StrLayer *self, int index)
{
    if (index < 0 || index >= self->texture_count || index >= STR_LAYER_TEXTURES)
        return &g_whiteTexture;
    if (self->textures[index])
        return self->textures[index];
    return layer_fetch(self, index, "");
}

void *str_layer_get_set_texture(struct StrLayer *self, int index, const char *folder)
{
    if (index < 0 || index >= self->texture_count || index >= STR_LAYER_TEXTURES)
        return &g_whiteTexture;
    if (self->textures[index])
        return self->textures[index];
    return layer_fetch(self, index, folder ? folder : "");
}

static void effect_resource_destroy(struct Res *resource)
{
    struct EzEffectRes *self = (struct EzEffectRes *)resource;
    effect_resource_finish(self);
    free(self);
}
static int effect_resource_load(struct Res *resource, const char *name)
{
    return e_zeffect_res_load((struct EzEffectRes *)resource, name);
}
const struct ResourceOps effect_resource_ops = {
    .destroy = effect_resource_destroy,
    .load = effect_resource_load,
};
