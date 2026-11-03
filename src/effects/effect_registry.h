#ifndef EFFECTS_EFFECT_REGISTRY_H
#define EFFECTS_EFFECT_REGISTRY_H

#include "effects/effect_types.h"

/* Every effect class's type table, in registration order (effects/effect_registry.c, which
   documents the order). app_init_managers in core/startup.c passes it to effect_mgr_create. */
extern const struct EffectTypeSources g_effectTypes;

#endif
