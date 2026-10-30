#include "effects/render_object.h"
#include "game/context.h"
#include "std/list.h"
#include <assert.h>
#include <stdio.h>

static char world_tokens[2];
static struct World *world;
static struct List objects;
static struct ListNode head;
static struct RenderObject object;
static struct RenderObjectOps methods;
static int list_present, inserts, deletes;

struct World *game_current_world(void) { return world; }
struct List *game_current_object_list(void) { return list_present ? &objects : NULL; }

static void destroy(struct GameObject *self)
{
    assert(self == (struct GameObject *)&object && inserts == 0);
    ++deletes;
}

void *list_insert(struct List *list, struct ListNode *where, const void *value, size_t size)
{
    assert(list == &objects && where == &head);
    assert(size == sizeof(struct RenderObject *));
    assert(*(struct RenderObject *const *)value == &object && deletes == 0);
    ++inserts;
    return NULL;
}

int main(void)
{
    struct World *expected = (struct World *)&world_tokens[0];
    methods.base.destroy = destroy;
    object.ops = &methods.base;
    objects.head = &head;

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
