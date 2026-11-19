#include "effects/render_object.h"
#include "game/context.h"
#include "world/world.h"

int effect_add_world_object(struct World *expected_world, struct RenderObject *object)
{
    if (!object)
        return 0;
    if (!expected_world || expected_world != game_current_world() || !world_add_object(expected_world, object)) {
        game_object_destroy((struct GameObject *)object);
        return 0;
    }
    return 1;
}
