#include "effects/render_object.h"
#include "game/context.h"
#include "std/list.h"

int effect_add_world_object(struct World *expected_world, struct RenderObject *object)
{
    struct List *objects;

    if (!object)
        return 0;
    objects = game_current_object_list();
    if (!expected_world || expected_world != game_current_world() || !objects || !objects->head) {
        game_object_destroy((struct GameObject *)object);
        return 0;
    }
    list_push_back(objects, &object, sizeof object);
    return 1;
}
