#ifndef EFFECTS_EFFECT_RENDER_HOOKS_H
#define EFFECTS_EFFECT_RENDER_HOOKS_H

/* The effect manager's side of g_renderBeforeClearLists (render/rp_pool.h):
   effect_mgr_destroy_removed on g_effectMgr. */
void effect_mgr_destroy_removed_effects(void);

#endif
