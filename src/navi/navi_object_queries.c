#include "navi/navi.h"

int navi_object_type(const struct NaviObject *self)
{
    return self->type;
}

int navi_object_sprite(const struct NaviObject *self)
{
    if (self->kind == NAVI_OBJECT_ENTITY &&
        (self->type == NAVI_TYPE_MOB || self->type == NAVI_TYPE_MOB_BOSS))
        return (unsigned short)self->sprite;
    return self->sprite;
}

int navi_object_sprite_high(const struct NaviObject *self)
{
    return (unsigned int)self->sprite >> 16;
}

const char *navi_object_name(const struct NaviObject *self)
{
    return string_cstr(&self->name);
}

const char *navi_object_alias(const struct NaviObject *self)
{
    return string_cstr(&self->alias);
}

struct NaviPoint *navi_object_position(const struct NaviObject *self, struct NaviPoint *out)
{
    out->x = self->x;
    out->y = self->y;
    return out;
}

struct NaviMap *navi_object_map(const struct NaviObject *self)
{
    if (self->kind == NAVI_OBJECT_LINK && self->type == NAVI_TYPE_WARP_SCROLL)
        return navi_manager_current_map(&g_navi);
    return self->map;
}
