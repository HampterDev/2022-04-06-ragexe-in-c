#ifndef ACTOR_ACTOR_SPAWNS_H
#define ACTOR_ACTOR_SPAWNS_H

#include <stddef.h>
#include "render/vec3.h"

struct Corpse;
struct GameMode;

/* What the actors ask the game mode to put into its world: the arrows and plant attacks of
   their ranged attacks and the corpses they leave. There is one instance, g_actorSpawns
   (actor/actor_spawns.c); client_install_hooks() (core/client_init.c) fills it in with the
   game mode's functions (game/world_objects.c). The actor_spawn_* wrappers below call them;
   with a member NULL nothing is launched or made, and actor_spawn_corpse returns NULL. */
struct ActorSpawns {
    void (*launch_arrow)(struct GameMode *mode, const struct Vec3 *from, unsigned target,
                         float speed, int job);
    void (*launch_plant_attack)(struct GameMode *mode, unsigned target_id, int job, float delay);
    struct Corpse *(*make_corpse)(struct GameMode *mode, Vec3 pos, const char *act,
                                  const char *spr, int action, int motion, float facing);
    void (*make_granny_corpse)(struct GameMode *mode, Vec3 pos, float facing, const char *name);
};

extern struct ActorSpawns g_actorSpawns;

static __inline void actor_spawn_arrow(struct GameMode *mode, const struct Vec3 *from,
                                       unsigned target, float speed, int job)
{
    if (g_actorSpawns.launch_arrow)
        g_actorSpawns.launch_arrow(mode, from, target, speed, job);
}

static __inline void actor_spawn_plant_attack(struct GameMode *mode, unsigned target_id, int job,
                                              float delay)
{
    if (g_actorSpawns.launch_plant_attack)
        g_actorSpawns.launch_plant_attack(mode, target_id, job, delay);
}

static __inline struct Corpse *actor_spawn_corpse(struct GameMode *mode, Vec3 pos, const char *act,
                                                  const char *spr, int action, int motion,
                                                  float facing)
{
    if (!g_actorSpawns.make_corpse)
        return NULL;
    return g_actorSpawns.make_corpse(mode, pos, act, spr, action, motion, facing);
}

static __inline void actor_spawn_granny_corpse(struct GameMode *mode, Vec3 pos, float facing,
                                               const char *name)
{
    if (g_actorSpawns.make_granny_corpse)
        g_actorSpawns.make_granny_corpse(mode, pos, facing, name);
}

#endif
