#include "effects/effect_render_hooks.h"
#include "effects/effect_mgr.h"

void effect_mgr_destroy_removed_effects(void)
{
    effect_mgr_destroy_removed(g_effectMgr);
}
