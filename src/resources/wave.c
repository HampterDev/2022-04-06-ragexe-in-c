#include "resources/wave.h"
#include "resources/resource_factories.h"
#include "resources/resource_ops.h"
#include "fileio/res_mgr.h"
#include "fileio/file.h"
#include "platform/audio.h"
#include <stdlib.h>

struct Res *wave_resource_create(void)
{
    struct Wave *wave = calloc(1, sizeof *wave);
    if (!wave)
        return NULL;
    res_construct(&wave->res);
    wave->res.ops = &wave_resource_ops;
    return &wave->res;
}

void wave_resource_reset(struct Wave *wave)
{
    audio_sound_free(wave->sound);
    wave->sound = NULL;
}

static int wave_resource_load(struct Res *resource, const char *name)
{
    struct Wave *wave = (struct Wave *)resource;
    struct File file;
    int ok = 0;
    if (!name)
        return 0;
    file_construct(&file);
    if (file_open(&file, name, CFILE_MODE_READ, 1, 0) && file.size >= 12 && file.buffer) {
        wave_resource_reset(wave);
        wave->sound = audio_sound_decode(file.buffer, (size_t)file.size);
        ok = 1;
    }
    file_destruct(&file);
    return ok;
}

static void wave_resource_destroy(struct Res *resource)
{
    wave_resource_reset((struct Wave *)resource);
    free(resource);
}

const struct ResourceOps wave_resource_ops = {
    .destroy = wave_resource_destroy,
    .load = wave_resource_load,
};
