#include "effects/render_object.h"
#include "game/context.h"
#include <assert.h>
#include <stdio.h>

static char world_tokens[2];
static struct World *world;
static struct RenderObject object;
static struct RenderObjectOps methods;
static int list_present, inserts, deletes;

struct World *game_current_world(void) { return world; }

static void destroy(struct GameObject *self)
{
    assert(self == (struct GameObject *)&object && inserts == 0);
    ++deletes;
}

/* the world's list takes the object while it is there (world/object_list.c) */
int world_add_object(struct World *self, void *value)
{
    assert(self == world && value == &object && deletes == 0);
    if (!list_present)
        return 0;
    ++inserts;
    return 1;
}

int main(void)
{
    struct World *expected = (struct World *)&world_tokens[0];
    methods.base.destroy = destroy;
    object.ops = &methods.base;

    assert(!effect_add_world_object(NULL, NULL));
    assert(inserts == 0 && deletes == 0);
    assert(!effect_add_world_object(NULL, &object));
    assert(inserts == 0 && deletes == 1);

    deletes = 0;
    world = expected;
    assert(!effect_add_world_object(expected, &object));
    assert(inserts == 0 && deletes == 1);

    deletes = 0;
    list_present = 1;
    world = (struct World *)&world_tokens[1];
    assert(!effect_add_world_object(expected, &object));
    assert(inserts == 0 && deletes == 1);

    deletes = 0;
    world = NULL;
    assert(!effect_add_world_object(expected, &object));
    assert(inserts == 0 && deletes == 1);

    deletes = 0;
    world = expected;
    assert(effect_add_world_object(expected, &object));
    assert(inserts == 1 && deletes == 0);
    puts("effect objects: world ownership transfer, rejected objects and map changes passed");
    return 0;
}
