#include "actor/types.h"
#include "actor/render_object.h"
#include "actor/actor_ops.h"
#include "fileio/resource_api.h"

/* A render object's sprite and act resources: loading (locked while the object holds them)
   and reading them. Apart from actor/game_object.c and actor/game_actor.c, whose method
   tables and actor code reach the whole actor model, so that the actors and effects that
   load a sprite or an act do not. */

void render_object_load_spr_res(struct RenderObject *self, const char *name)
{
    if (self->spr)
        res_unlock((struct Res *)self->spr);
    self->spr = res_mgr_get(g_resMgr(), name);
    if (self->spr)
        res_lock((struct Res *)self->spr);
}

void render_object_load_act_res(struct RenderObject *self, const char *name)
{
    if (self->act)
        res_unlock((struct Res *)self->act);
    self->act = res_mgr_get(g_resMgr(), name);
    res_lock((struct Res *)self->act);
}

void *render_object_get_act_res(struct RenderObject *self)
{
    return self->act;
}

void *render_object_get_spr_res(struct RenderObject *self)
{
    return self->spr;
}
