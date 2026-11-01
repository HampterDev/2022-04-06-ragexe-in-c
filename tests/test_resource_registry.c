#include "resources/resource_registry.h"
#include "resources/resource_factories.h"
#include "fileio/resource_types.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

enum Factory {
    FACTORY_ACT, FACTORY_RMA, FACTORY_ATTR, FACTORY_BITMAP, FACTORY_GIF, FACTORY_EMBLEM, FACTORY_EFFECT, FACTORY_GROUND, FACTORY_IMF,
    FACTORY_MODEL1, FACTORY_MODEL2, FACTORY_ANIM_MODEL, FACTORY_GRANNY_MODEL, FACTORY_PALETTE, FACTORY_SPRITE, FACTORY_WAVE, FACTORY_WORLD,
    FACTORY_COUNT
};
static struct Res resources[FACTORY_COUNT];
static int calls[FACTORY_COUNT];
static int failing_factory = -1;

#define FACTORY(name, id) \
    struct Res *name(void) { \
        ++calls[id]; \
        return failing_factory == id ? NULL : &resources[id]; \
    }
FACTORY(animation_resource_create, FACTORY_ACT)
FACTORY(rma_resource_create, FACTORY_RMA)
FACTORY(attribute_resource_create, FACTORY_ATTR)
FACTORY(bitmap_resource_create, FACTORY_BITMAP)
FACTORY(gif_resource_create, FACTORY_GIF)
FACTORY(emblem_resource_create, FACTORY_EMBLEM)
FACTORY(effect_resource_create, FACTORY_EFFECT)
FACTORY(ground_resource_create, FACTORY_GROUND)
FACTORY(imf_resource_create, FACTORY_IMF)
FACTORY(model_resource_v1_create, FACTORY_MODEL1)
FACTORY(model_resource_v2_create, FACTORY_MODEL2)
FACTORY(animated_model_resource_create, FACTORY_ANIM_MODEL)
FACTORY(granny_model_resource_create, FACTORY_GRANNY_MODEL)
FACTORY(palette_resource_create, FACTORY_PALETTE)
FACTORY(sprite_resource_create, FACTORY_SPRITE)
FACTORY(wave_resource_create, FACTORY_WAVE)
FACTORY(world_resource_create, FACTORY_WORLD)
#undef FACTORY

int main(void)
{
    static const struct {
        const char *extension, *directory;
        enum Factory factory;
    } expected[] = {
        { "act", "sprite\\", FACTORY_ACT }, { "rma", "model\\", FACTORY_RMA },
        { "gat", "", FACTORY_ATTR }, { "bmp", "texture\\", FACTORY_BITMAP },
        { "tga", "texture\\", FACTORY_BITMAP }, { "jpg", "texture\\", FACTORY_BITMAP },
        { "png", "texture\\", FACTORY_BITMAP }, { "gif", "texture\\", FACTORY_GIF },
        { "ebm", "..\\_tmpEmblem\\", FACTORY_EMBLEM },
        { "str", "texture\\effect\\", FACTORY_EFFECT }, { "gnd", "", FACTORY_GROUND },
        { "imf", "imf\\", FACTORY_IMF }, { "rsm", "model\\", FACTORY_MODEL1 },
        { "rsm2", "model\\", FACTORY_MODEL2 }, { "rsx", "model\\", FACTORY_ANIM_MODEL },
        { "gr2", "model\\3dmob\\", FACTORY_GRANNY_MODEL },
        { "pal", "palette\\", FACTORY_PALETTE }, { "spr", "sprite\\", FACTORY_SPRITE },
        { "wav", "wav\\", FACTORY_WAVE }, { "rsw", "", FACTORY_WORLD },
    };
    static const int invalid[] = { INT_MIN, -1, RESOURCE_TYPE_COUNT, INT_MAX };
    int i, j;

    assert(sizeof expected / sizeof expected[0] == RESOURCE_TYPE_COUNT);
    for (i = 0; i < RESOURCE_TYPE_COUNT; ++i) {
        int type = resource_type_index(expected[i].extension);
        assert(type == i);
        assert(strcmp(resource_type_directory(type), expected[i].directory) == 0);
        memset(calls, 0, sizeof calls);
        assert(resource_create(type) == &resources[expected[i].factory]);
        for (j = 0; j < FACTORY_COUNT; ++j)
            assert(calls[j] == (j == expected[i].factory));
        failing_factory = expected[i].factory;
        assert(resource_create(type) == NULL);
        assert(calls[failing_factory] == 2);
        failing_factory = -1;
    }
    assert(resource_type_index(NULL) == -1);
    assert(resource_type_index("") == -1);
    assert(resource_type_index("unknown") == -1);
    assert(resource_type_index("bik") == -1);
    assert(resource_type_index("BMP") == -1);
    assert(resource_type_index(".bmp") == -1);
    assert(resource_type_index("rsm2extra") == -1);
    memset(calls, 0, sizeof calls);
    for (i = 0; i < (int)(sizeof invalid / sizeof invalid[0]); ++i) {
        assert(resource_type_directory(invalid[i]) == NULL);
        assert(resource_create(invalid[i]) == NULL);
    }
    for (i = 0; i < FACTORY_COUNT; ++i)
        assert(calls[i] == 0);
    puts("resource registry: all types, directories, factory dispatch, failure and bounds passed");
    return 0;
}
