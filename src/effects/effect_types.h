#ifndef EFFECTS_EFFECT_TYPES_H
#define EFFECTS_EFFECT_TYPES_H

#include <stddef.h>

struct Effect;
struct EffectMgr;

/* Effect-type tables: which effect class the effect manager creates for an effect type.

   Each effect class owns its rows. They are a table in the class's own file, defined right
   after its constructor and declared in effects/effect_allocator.h, for example
   heal_effect_types in effects/skills/heal_effect.c. A row says that the types first..last
   are created by `create`; the rows of a table are registered in their order.

   effects/effect_registry.c assembles the tables in one fixed order (g_effectTypes there),
   and app_init_managers (core/startup.c) hands that list to effect_mgr_create. The effect
   manager itself names no effect class.

   A type that no row lists is not an effect-manager type: effect_mgr_spawn creates nothing
   for it, and the actor code that asks effect_mgr_is_registered takes its other path for it
   (the RagEffect list). The fixed rows of all tables are pairwise distinct; a type listed
   twice makes the registration fail, and with it effect_mgr_create and the client start. */
struct EffectTypeRow {
    int first, last;
    struct Effect *(*create)(void);
};

struct EffectTypeTable {
    const struct EffectTypeRow *rows;
    size_t count;
};

#define EFFECT_TYPE(type, create)         { type, type, create }
#define EFFECT_TYPES(first, last, create) { first, last, create }
#define EFFECT_TYPE_TABLE(rows)           { rows, sizeof(rows) / sizeof((rows)[0]) }

/* One entry of the composition list: a class's fixed table, or a function that registers
   types known only at startup (it returns 0 to stop the registration). */
struct EffectTypeSource {
    const struct EffectTypeTable *table;
    unsigned char (*add)(struct EffectMgr *mgr);
};

struct EffectTypeSources {
    const struct EffectTypeSource *items;
    size_t count;
};

#endif
