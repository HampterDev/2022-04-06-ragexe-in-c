#include "navi/navi.h"
#include "game/cells.h"
#include "render/texture_cache.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "navi/navi_search.h"
#include "resources/attr.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "platform/win_types.h"
#include "fileio/resource_api.h"
#include <string.h>
#include "navi/navi_manager.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"
#include <stdlib.h>

struct NaviObject *navi_object_construct(struct NaviObject *self)
{
    self->kind = NAVI_OBJECT_ENTITY;
    string_init(&self->name);
    string_init(&self->alias);
    self->x = 0;
    self->y = 0;
    self->passes.first = NULL;
    self->passes.last = NULL;
    self->passes.end = NULL;
    memset(self->pickNpc, 0, sizeof(self->pickNpc));
    self->id = -1;
    self->type = NAVI_TYPE_NPC;
    self->sprite = 0;
    self->map = NULL;
    return self;
}

struct NaviObject *navi_object_construct_with(struct NaviObject *self,
        int type, int sprite, const char *name, const char *alias, int x, int y, struct NaviMap *map)
{
    self->kind = NAVI_OBJECT_ENTITY;
    string_init(&self->name);
    string_init(&self->alias);
    self->x = 0;
    self->y = 0;
    self->passes.first = NULL;
    self->passes.last = NULL;
    self->passes.end = NULL;
    memset(self->pickNpc, 0, sizeof(self->pickNpc));
    self->type = type;
    self->id = -1;
    self->sprite = sprite;
    if (name)
        string_assign_cstr(&self->name, name);
    if (alias)
        string_assign_cstr(&self->alias, alias);
    self->x = x;
    self->y = y;
    self->map = map;
    return self;
}

void navi_object_destruct(struct NaviObject *self)
{
    self->kind = NAVI_OBJECT_ENTITY;
    self->id = -1;
    self->type = NAVI_TYPE_NPC;
    self->sprite = -1;
    string_clear(&self->name);
    string_clear(&self->alias);
    self->x = 0;
    self->y = 0;
    self->map = NULL;
    vec_free(&self->passes);
    string_free(&self->alias);
    string_free(&self->name);
}

void navi_object_free(struct NaviObject *self)
{
    if (!self) return;
    if (self->kind == NAVI_OBJECT_LINK)
        navi_link_destruct((struct NaviLink *)self);
    else
        navi_object_destruct(self);
    free(self);
}

unsigned char navi_object_id_is(struct NaviObject *self, const int *id)
{
    return self->id == *id;
}
