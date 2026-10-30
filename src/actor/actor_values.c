#include "actor/render_object.h"
#include "std/integer_map.h"
#include "std/tree.h"

unsigned char game_actor_update_sphere_count(struct GameActor *self, int64_t count)
{
    unsigned char above = count > self->sphere_count;
    self->sphere_count = count;
    return above;
}

void game_actor_set_instant_var(struct GameActor *self, int key, long long value)
{
    struct IntInt64Pair *var = tree_get(&self->vars, &key, sizeof key, sizeof *var, tree_less_int);

    var->value = value;
}
