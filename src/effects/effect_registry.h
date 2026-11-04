#ifndef EFFECTS_EFFECT_REGISTRY_H
#define EFFECTS_EFFECT_REGISTRY_H

#include "effects/effect_types.h"

/* Every effect class's type table, in registration order (effects/effect_registry.c, which
   documents the order). The effect manager's row in the manager table (core/client_managers.c)
   passes it to effect_mgr_create. */
extern const struct EffectTypeSources g_effectTypes;

#endif
