#include "resources/resource_registry.h"
#include "resources/resource_factories.h"
#include <stddef.h>
#include <string.h>
#include "resources/anim_model_res.h"
#include "resources/attr.h"

struct ResourceType {
    const char *extension;
    const char *directory;
    struct Res *(*create)(void);
};

static const struct ResourceType resource_types[RESOURCE_TYPE_COUNT] = {
    { "act", "sprite\\", animation_resource_create },
    { "rma", "model\\", rma_resource_create },
    { "gat", "", attribute_resource_create },
    { "bmp", "texture\\", bitmap_resource_create },
    { "tga", "texture\\", bitmap_resource_create },
    { "jpg", "texture\\", bitmap_resource_create },
    { "png", "texture\\", bitmap_resource_create },
    { "gif", "texture\\", gif_resource_create },
    { "ebm", "..\\_tmpEmblem\\", emblem_resource_create },
    { "str", "texture\\effect\\", effect_resource_create },
    { "gnd", "", ground_resource_create },
    { "imf", "imf\\", imf_resource_create },
    { "rsm", "model\\", model_resource_v1_create },
    { "rsm2", "model\\", model_resource_v2_create },
    { "rsx", "model\\", animated_model_resource_create },
    { "gr2", "model\\3dmob\\", granny_model_resource_create },
    { "pal", "palette\\", palette_resource_create },
    { "spr", "sprite\\", sprite_resource_create },
    { "wav", "wav\\", wave_resource_create },
    { "rsw", "", world_resource_create },
};

int resource_type_index(const char *extension)
{
    int i;
    if (!extension)
        return -1;
    for (i = 0; i < RESOURCE_TYPE_COUNT; ++i)
        if (strcmp(extension, resource_types[i].extension) == 0)
            return i;
    return -1;
}

const char *resource_type_directory(int type)
{
    if (type < 0 || type >= RESOURCE_TYPE_COUNT)
        return NULL;
    return resource_types[type].directory;
}

struct Res *resource_create(int type)
{
    if (type < 0 || type >= RESOURCE_TYPE_COUNT)
        return NULL;
    return resource_types[type].create();
}
