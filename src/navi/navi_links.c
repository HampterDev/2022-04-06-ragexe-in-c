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

static void link_construct_base(struct NaviLink *self)
{
    navi_object_construct(&self->base);
    self->base.kind = NAVI_OBJECT_LINK;
    self->destX = 0;
    self->destY = 0;
    self->exits.first = NULL;
    self->exits.last = NULL;
    self->exits.end = NULL;
}

struct NaviLink *navi_link_construct_with(struct NaviLink *self,
        int type, int sprite, const char *name, const char *alias, int x, int y, struct NaviMap *map,
        struct NaviMap *destMap, int destX, int destY)
{
    link_construct_base(self);
    self->base.type = type;
    self->base.sprite = sprite;
    if (name)
        string_assign_cstr(&self->base.name, name);
    if (alias)
        string_assign_cstr(&self->base.alias, alias);
    self->base.x = x;
    self->base.y = y;
    self->base.map = map;
    self->destMap = destMap;
    self->destX = destX;
    self->destY = destY;
    self->depth = 0;
    return self;
}

void navi_link_destruct(struct NaviLink *self)
{
    self->base.kind = NAVI_OBJECT_LINK;
    self->base.type = NAVI_TYPE_LINK_FIRST;
    self->destMap = NULL;
    vec_free(&self->exits);
    navi_object_destruct(&self->base);
}

struct NaviMap *navi_link_dest_map(struct NaviLink *self)
{
    return self->destMap;
}

const char *navi_link_dest_map_name(struct NaviLink *self)
{
    return string_cstr(&self->destMap->name);
}

struct NaviPoint *navi_link_dest_pos(struct NaviLink *self, struct NaviPoint *out)
{
    out->x = self->destX;
    out->y = self->destY;
    return out;
}

void navi_link_set_depth(struct NaviLink *self, int depth)
{
    self->depth = depth;
}

int navi_link_depth(struct NaviLink *self)
{
    return self->depth;
}

void navi_link_add_exit(struct NaviLink *self, const struct NaviPass *exit)
{
    vec_push(&self->exits, exit, sizeof *exit);
}

struct NaviPass **navi_link_exits_begin(struct NaviLink *self, struct NaviPass **out)
{
    *out = (struct NaviPass *)self->exits.first;
    return out;
}

struct NaviPass **navi_link_exits_end(struct NaviLink *self, struct NaviPass **out)
{
    *out = (struct NaviPass *)self->exits.last;
    return out;
}

void navi_link_clear_exits(struct NaviLink *self)
{
    self->exits.last = self->exits.first;
}
