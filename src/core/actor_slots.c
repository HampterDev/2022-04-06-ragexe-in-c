#include "actor/actor_ops.h"
#include "actor/render_ops.h"
#include "actor/appearance.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include <windows.h>
#include "base.h"
#include "std/std.h"
#include "actor/job.h"

int game_actor_get_effect_state(struct RenderObject *self) { return ((struct GameActor *)self)->effect_state; }
int game_actor_get_guild_id(struct GameActor * self)
{ return self->guild_id; }
void game_actor_set_guild_id(struct GameActor * self, int value)
{ self->guild_id = value; }
void game_actor_set_mounted(struct GameActor * self, unsigned char value)
{ self->mounted = value; }

int game_actor_get_npc_location_radius(struct GameActor * self)
{
    int radius = 0, job;

    job = game_actor_get_job(self);

    client_lua_call(g_session1.luaState, "GetNpcLocationRadius", "d>d", job, &radius);
    return radius;
}
