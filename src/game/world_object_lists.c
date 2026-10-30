#include "game/gamemode.h"
#include "world/world_imports.h"

void actor_list_push_back(struct List *list, void *actor)
{
    actor_list_generation++;
    list_push_back(list, &actor, sizeof actor);
}
