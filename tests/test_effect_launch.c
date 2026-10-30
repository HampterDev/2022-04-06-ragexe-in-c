#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/memory.h"
#include "actor/actor.h"
#include "game/context.h"
#include "ui/misc/rolling_cutter.h"
#include "std/list.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static struct List *objects;
static struct GameActor actor;
static union {
    struct RagEffect effect;
    struct RollingCutterRagEffect cutter;
} storage;
static int allocations, initializations, world_insertions, actor_insertions;
static int fail_allocation, constructed_type, initialized_id;
static Vec3 initialized_offset;
struct Session g_session1;
extern unsigned g_launchedEffectIds[32];

struct List *game_current_object_list(void) { return objects; }

void *mem_alloc(size_t size)
{
    assert(size == sizeof(struct RagEffect) || size == sizeof(struct RollingCutterRagEffect));
    ++allocations;
    return fail_allocation ? NULL : &storage;
}

void *mem_try_alloc(size_t size)
{
    return mem_alloc(size);
}

struct RagEffect *rag_effect_construct(struct RagEffect *self)
{
    assert(self == &storage.effect);
    constructed_type = 0;
    return self;
}

struct RollingCutterRagEffect *rolling_cutter_rag_effect_construct(struct RollingCutterRagEffect *self)
{
    assert(self == &storage.cutter);
    constructed_type = 0x345;
    return self;
}

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...)
{
    va_list args;
    int id;
    assert(lua == g_session1.luaState && strcmp(function, "IsForceRenderEffect") == 0);
    assert(strcmp(format, "d>b") == 0);
    va_start(args, format);
    id = va_arg(args, int);
    *va_arg(args, unsigned char *) = (unsigned char)(id == 0xa6);
    va_end(args);
    return 1;
}

float get_sin_deg(int degrees) { assert(degrees == 90); return 1.0f; }
float get_cos_deg(int degrees) { assert(degrees == 90); return 0.0f; }

void rag_effect_init(struct RagEffect *effect, struct RenderObject *owner, int id, Vec3 offset)
{
    assert(effect == &storage.effect && (void *)owner == &actor);
    ++initializations;
    initialized_id = id;
    initialized_offset = offset;
}

void rag_effect_send_msg(struct RagEffect *self, struct RenderObject *sender, RenderMsgArg msg,
    RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3)
{
    (void)self; (void)sender; (void)msg; (void)par1; (void)par2; (void)par3;
    assert(!"launching sends no effect message");
}

void *list_insert(struct List *list, struct ListNode *where, const void *value, size_t size)
{
    assert(size == sizeof(void *) && *(void *const *)value == &storage);
    if (list == objects) {
        assert(where == objects->head && initializations == world_insertions + 1);
        ++world_insertions;
    } else {
        assert(list == &actor.effects && where == actor.effects.head);
        assert(world_insertions == actor_insertions + 1);
        ++actor_insertions;
    }
    return NULL;
}

int main(void)
{
    struct List world_objects = {0};
    Vec3 offset = {2.0f, 3.0f, 4.0f};

    assert(!game_actor_launch_effect(&actor, 1, offset, 90));
    objects = &world_objects;
    assert(!game_actor_launch_effect(NULL, 1, offset, 90));
    assert(!game_actor_launch_effect(&actor, -1, offset, 90));
    assert(!game_actor_launch_effect(&actor, 60001, offset, 90));
    assert(!game_actor_launch_effect(&actor, 0xa6, offset, 90));
    assert(allocations == 0 && initializations == 0 && actor_insertions == 0);

    fail_allocation = 1;
    assert(!game_actor_launch_effect(&actor, 1, offset, 90));
    assert(allocations == 1 && initializations == 0 && actor_insertions == 0);
    fail_allocation = 0;
    actor.is_pc = 1;
    assert(game_actor_launch_effect(&actor, 0xa6, offset, 90) == &storage.effect);
    assert(constructed_type == 0 && initialized_id == 0xa6);
    assert(initialized_offset.x == -4 && initialized_offset.y == 3 && initialized_offset.z == 2);
    assert(storage.effect.always_draw == 1 && g_launchedEffectIds[0] == 0xa6);
    assert(world_insertions == 1 && actor_insertions == 1);

    assert(game_actor_launch_effect(&actor, 0x345, offset, 90) == &storage.effect);
    assert(constructed_type == 0x345 && storage.effect.always_draw == 0);
    assert(game_actor_launch_effect(&actor, 0x7f, offset, 90) == &storage.effect);
    assert(g_launchedEffectIds[0] == 0x7f && g_launchedEffectIds[1] == 0x345);
    assert(world_insertions == 3 && actor_insertions == 3);
    objects = NULL;
    assert(!game_actor_launch_effect(&actor, 1, offset, 90));
    assert(world_insertions == 3 && allocations == 4);
    puts("effect launch: rejected requests, allocation failure, rotation and ownership passed");
    return 0;
}
