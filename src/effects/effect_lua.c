#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include "effects/effect_mgr.h"
#include "effects/skills/emitter_effect.h"
#include "effects/skills/evils_paw_effect.h"
#include "effects/skills/wind_effect.h"

#define EFFECT_EMITTER           0x3ce
#define EFFECT_MAGIC_FLOOR       0x401
#define EFFECT_EVILS_PAW         0x40f
#define EFFECT_ANIMATED_EMITTER  0x431
#define EFFECT_EZ2STR            0x584
#define EFFECT_WIND              0x927

enum EffectInfoProp {
    INFO_RESTART = 0,
    INFO_VELOCITY_MIN = 1,
    INFO_VELOCITY_MAX = 2,
    INFO_ACCELERATION = 3,
    INFO_POSITION = 4,
    INFO_SPAWN_BOX = 5,
    INFO_COLOR = 6,
    INFO_RATE = 7,
    INFO_SIZE = 8,
    INFO_LIFE = 9,
    INFO_TEXTURE = 10,
    INFO_SPEED = 11,
    INFO_SRC_BLEND = 12,
    INFO_DEST_BLEND = 13,
    INFO_COUNT = 14,
    INFO_BASE_AREA = 16,
    INFO_BASE_ROLL = 17,
    INFO_BASE_RISE_ANGLE = 18,
    INFO_BASE_ALPHA = 19,
    INFO_BASE_FULL_DISPLAY_ANGLE = 20,
    INFO_BASE_VALUE_FIRST = 21,
    INFO_BASE_VALUE_LAST = 41,
    INFO_BASE_SCALE = 42,
    INFO_WITH_DEPTH = 43,
    INFO_OFFSET = 44,
    INFO_FIXED_FACING = 45,
    INFO_TURN = 46,
    INFO_STR_FILE = 47,
    INFO_STR_LIST = 48,
    INFO_STR_ZOOM = 49,
    INFO_STR_ALPHA = 50,
    INFO_KEEP_PARTICLES = 51,
    INFO_PARTICLE_SCALE = 52,
    INFO_STRAND_COUNT = 53,
};

static struct Effect *find_effect(int effect_id, int index)
{
    return effect_mgr_tracked_at(g_effectMgr, effect_id, (unsigned)index);
}

static void restart(struct Effect *e)
{
    e->ops->restart(e);
}

static void set_effect_param(lua_State *L, struct Effect *e, int prop, int arg)
{
    struct EffectParams *params = &e->params;

    switch (prop) {
    case INFO_RESTART:
        restart(e);
        break;
    case INFO_BASE_AREA:
        params->area = (float)luaL_checknumber(L, arg);
        break;
    case INFO_BASE_SCALE:
        params->scale = (float)luaL_checknumber(L, arg);
        break;
    case INFO_BASE_ROLL:
        params->roll = (int)luaL_checknumber(L, arg);
        break;
    case INFO_BASE_RISE_ANGLE:
        params->rise_angle = (float)(int)luaL_checknumber(L, arg);
        break;
    case INFO_BASE_ALPHA:
        params->alpha = (int)luaL_checknumber(L, arg);
        break;
    case INFO_BASE_FULL_DISPLAY_ANGLE:
        params->full_display_angle = (int)luaL_checknumber(L, arg);
        break;
    default:
        if (prop >= INFO_BASE_VALUE_FIRST && prop <= INFO_BASE_VALUE_LAST)
            params->values[prop - INFO_BASE_VALUE_FIRST] = (float)(int)luaL_checknumber(L, arg);
        break;
    }
}

void emitter_effect_set_texture(struct EmitterEffect *self, struct String name)
{
    string_assign(&self->texture, string_cstr(&name), name.size);
    string_free(&name);
}

static int set_emitter_info(lua_State *L, int effect_id, int all_props)
{
    struct EmitterEffect *e;
    int prop;

    e = (struct EmitterEffect *)find_effect(effect_id, (int)luaL_checknumber(L, 1));
    if (e == NULL)
        return 0;
    prop = (int)luaL_checknumber(L, 2);

    switch (prop) {
    case INFO_RESTART:
        restart(&e->base);
        break;

    case INFO_VELOCITY_MIN: case INFO_VELOCITY_MAX: case INFO_ACCELERATION:
    case INFO_POSITION: case INFO_SPAWN_BOX: {
        Vec3 *v;
        float x = (float)luaL_checknumber(L, 3);
        float y = (float)luaL_checknumber(L, 4);
        float z = (float)luaL_checknumber(L, 5);
        if (prop == INFO_VELOCITY_MIN)      v = &e->velocity_min;
        else if (prop == INFO_VELOCITY_MAX) v = &e->velocity_max;
        else if (prop == INFO_ACCELERATION) v = &e->acceleration;
        else if (prop == INFO_POSITION)     v = &e->base.pos;
        else                                v = &e->spawn_box;
        v->x = x;
        v->y = y;
        v->z = z;
        break;
    }

    case INFO_COLOR: {
        int r, g, b, alpha;
        r = (int)luaL_checknumber(L, 3);
        if (r > 255) r = 255;
        g = (int)luaL_checknumber(L, 4);
        if (g > 255) g = 255;
        b = (int)luaL_checknumber(L, 5);
        if (b > 255) b = 255;
        alpha = (int)luaL_checknumber(L, 6);
        if (alpha > 255) alpha = 255;
        e->alpha = alpha;
        e->r = r;
        e->g = g;
        e->b = b;
        break;
    }

    case INFO_RATE: case INFO_SIZE: case INFO_LIFE: {
        float lo = (float)luaL_checknumber(L, 3);
        float hi = (float)luaL_checknumber(L, 4);
        if (prop == INFO_RATE) {
            e->rate_min = lo;
            e->rate_max = hi;
        } else if (prop == INFO_SIZE) {
            e->size_min = lo;
            e->size_max = hi;
        } else {
            e->life_min = lo;
            e->life_max = hi;
        }
        break;
    }

    case INFO_TEXTURE: {
        const char *name = luaL_checklstring(L, 3, NULL);
        struct String s;
        string_init(&s);
        string_assign_cstr(&s, name);
        emitter_effect_set_texture(e, s);
        break;
    }

    case INFO_SPEED:
        emitter_effect_set_particle_speed(e, (float)luaL_checknumber(L, 3));
        break;
    case INFO_SRC_BLEND:
        e->src_blend = (int)luaL_checknumber(L, 3);
        break;
    case INFO_DEST_BLEND:
        e->dest_blend = (int)luaL_checknumber(L, 3);
        break;
    case INFO_COUNT:
        emitter_effect_set_count(e, (int)luaL_checknumber(L, 3));
        break;
    case INFO_WITH_DEPTH:
        e->with_depth = (int)luaL_checknumber(L, 3) != 0;
        break;

    default:
        if (!all_props)
            break;
        switch (prop) {
        case INFO_FIXED_FACING:
            e->fixed_facing = (int)luaL_checknumber(L, 3) != 0;
            break;
        case INFO_TURN: {
            float x = (float)luaL_checknumber(L, 3);
            float y = (float)luaL_checknumber(L, 4);
            float z = (float)luaL_checknumber(L, 5);
            e->turn.x = x;
            e->turn.y = y;
            e->turn.z = z;
            break;
        }
        case INFO_KEEP_PARTICLES:
            e->keep_particles = (int)luaL_checknumber(L, 3) == 1;
            break;
        case INFO_PARTICLE_SCALE: {
            float x = (float)luaL_checknumber(L, 3);
            float y = (float)luaL_checknumber(L, 4);
            e->scale_x = x;
            e->scale_y = y;
            break;
        }
        }
        break;
    }
    return 0;
}

int effect_lua_set_emitter_info(lua_State *L)
{
    return set_emitter_info(L, EFFECT_EMITTER, 1);
}

int effect_lua_set_animated_emitter_info(lua_State *L)
{
    return set_emitter_info(L, EFFECT_ANIMATED_EMITTER, 0);
}

int effect_lua_set_magic_floor_info(lua_State *L)
{
    int index = (int)luaL_checknumber(L, 1);
    double prop = luaL_checknumber(L, 2);
    struct Effect *e = find_effect(EFFECT_MAGIC_FLOOR, index);

    if (e != NULL)
        set_effect_param(L, e, (int)prop, 3);
    return 0;
}

int effect_lua_set_other_effect_info(lua_State *L)
{
    int effect_id = (int)luaL_checknumber(L, 1);
    int index = (int)luaL_checknumber(L, 2);
    struct Effect *e = find_effect(effect_id, index);

    if (e != NULL)
        set_effect_param(L, e, (int)luaL_checknumber(L, 3), 4);
    return 0;
}

int effect_lua_set_evils_paw_info(lua_State *L)
{
    int index = (int)luaL_checknumber(L, 1);
    double prop = luaL_checknumber(L, 2);
    struct EvilsPawEffect *e = (struct EvilsPawEffect *)find_effect(EFFECT_EVILS_PAW, index);
    float x, y, z;

    if (e == NULL)
        return 0;
    switch ((int)prop) {
    case INFO_RESTART:
        restart(&e->base);
        break;
    case INFO_SIZE:
        e->particle_scale = (int)(float)luaL_checknumber(L, 3);
        break;
    case INFO_SPEED:
        e->frame_delay = (int)(float)luaL_checknumber(L, 3);
        break;
    case INFO_OFFSET:
        x = (float)luaL_checknumber(L, 3);
        y = (float)luaL_checknumber(L, 4);
        z = (float)luaL_checknumber(L, 5);
        e->offset.x = x;
        e->offset.y = y;
        e->offset.z = z;
        break;
    }
    return 0;
}

int effect_lua_set_ez2str_info(lua_State *L)
{
    int index = (int)luaL_checknumber(L, 1);
    double prop = luaL_checknumber(L, 2);
    struct Effect *e = find_effect(EFFECT_EZ2STR, index);
    float alpha;

    if (e == NULL)
        return 0;
    switch ((int)prop) {
    case INFO_STR_FILE:
        e->ops->set_str_name(e, luaL_checklstring(L, 3, NULL));
        break;
    case INFO_STR_LIST:
        e->ops->set_list_flags(e, (unsigned)luaL_checknumber(L, 3));
        break;
    case INFO_STR_ZOOM:
        e->ops->set_zoom(e, (float)luaL_checknumber(L, 3));
        break;
    case INFO_STR_ALPHA:
        alpha = (float)luaL_checknumber(L, 3);
        if (alpha >= 1.0f)
            alpha = 1.0f;
        else if (0.0f > alpha)
            alpha = 0.0f;
        e->ops->set_alpha_scale(e, alpha);
        break;
    }
    return 0;
}

int effect_lua_set_wind_effect_info(lua_State *L)
{
    int index = (int)luaL_checknumber(L, 1);
    double prop = luaL_checknumber(L, 2);
    struct WindEffect *e = (struct WindEffect *)find_effect(EFFECT_WIND, index);
    float x, y, z;

    if (e == NULL)
        return 0;
    switch ((int)prop) {
    case INFO_RESTART:
        restart(&e->base);
        break;
    case INFO_POSITION:
        x = (float)luaL_checknumber(L, 3);
        y = (float)luaL_checknumber(L, 4);
        z = (float)luaL_checknumber(L, 5);
        e->base.pos.x = x;
        e->base.pos.y = y;
        e->base.pos.z = z;
        break;
    case INFO_COLOR: {
        double alpha = luaL_checknumber(L, 3);
        double r = luaL_checknumber(L, 4);
        double g = luaL_checknumber(L, 5);
        double b = luaL_checknumber(L, 6);
        e->max_alpha = (unsigned char)(int)alpha;
        e->r = (unsigned char)(int)r;
        e->g = (unsigned char)(int)g;
        e->b = (unsigned char)(int)b;
        break;
    }
    case INFO_SIZE:
        e->radius = (float)luaL_checknumber(L, 3);
        break;
    case INFO_TEXTURE: {
        const char *name = luaL_checklstring(L, 3, NULL);
        string_assign_cstr(&e->texture, name);
        break;
    }
    case INFO_SPEED:
        e->spin = (float)luaL_checknumber(L, 3);
        break;
    case INFO_SRC_BLEND:
        e->src_blend = (int)(__int64)luaL_checknumber(L, 3);
        break;
    case INFO_DEST_BLEND:
        e->dest_blend = (int)(__int64)luaL_checknumber(L, 3);
        break;
    case INFO_BASE_RISE_ANGLE:
        e->band_width = (float)luaL_checknumber(L, 3);
        break;
    case INFO_BASE_FULL_DISPLAY_ANGLE:
        e->arc = (unsigned short)(int)luaL_checknumber(L, 3);
        break;
    case INFO_BASE_VALUE_FIRST:
        e->height = (float)luaL_checknumber(L, 3);
        break;
    case INFO_TURN:
        x = (float)luaL_checknumber(L, 3);
        z = (float)luaL_checknumber(L, 4);
        e->tilt_x = x;
        e->tilt_y = 0.0f;
        e->tilt_z = z;
        break;
    case INFO_STRAND_COUNT:
        e->rings = (unsigned short)(int)luaL_checknumber(L, 3);
        break;
    }
    return 0;
}
