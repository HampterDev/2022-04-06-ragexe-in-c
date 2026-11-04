#include "effects/rag/rag_effect.h"
#include "actor/render_object.h"
#include "actor/granny_actor.h"
#include "game/world_objects.h"
#include "core/memory.h"
#include <string.h>
#include "game/objects.h"
#include "core/character_npc.h"
#include "effects/skills/arrow_effect.h"
#include "effects/skills/mandra_attack_effect.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "world/world.h"
#include <stdint.h>

static void world_add(struct GameMode *self, void *object)
{
    list_push_back(&self->world->gameObjects, &object, sizeof object);
}

void game_mode_launch_arrow(struct GameMode *self, const struct Vec3 *from, unsigned target, float speed,
                                     int job)
{
    void *arrow;

    if (target == 0)
        return;
    arrow = arrow_effect_new();
    if (arrow == 0)
        return;
    world_add(self, arrow);
    render_object_send(arrow, NULL, 0xe, (intptr_t)from, (int64_t)target, (intptr_t)&speed, (int64_t)job);
}

void game_mode_launch_arrow_to_point(struct GameMode *self, const float *from, const float *to, float speed)
{
    void *arrow = arrow_effect_new();

    if (arrow == 0)
        return;
    world_add(self, arrow);
    render_object_send(arrow, NULL, 0x40, (intptr_t)from, (intptr_t)to, (intptr_t)&speed, 0);
}

void game_mode_launch_plant_attack(struct GameMode *self, unsigned target_id, int job, float delay)
{
    void *attack = mandra_attack_effect_new();

    if (attack == 0)
        return;
    world_add(self, attack);
    render_object_send(attack, NULL, 0xe, (int64_t)target_id, (intptr_t)&delay, (int64_t)job, 0);
}

struct Corpse *game_mode_make_corpse(struct GameMode *self, Vec3 pos,
                                  const char *act_name, const char *spr_name,
                                  int action, int motion, float facing)
{
    struct Corpse *corpse = mem_try_alloc(sizeof *corpse);

    if (!corpse)
        return NULL;
    corpse_construct(corpse);
    world_add(self, corpse);
    corpse->pos = pos;
    render_object_load_act_res((struct RenderObject *)corpse, act_name);
    render_object_load_spr_res((struct RenderObject *)corpse, spr_name);
    corpse->action = action;
    corpse->base_action = action;
    corpse->motion = motion;
    corpse->roty = facing;
    return corpse;
}

void game_mode_make_granny_corpse(struct GameMode *self, Vec3 pos, float facing,
                              const char *name)
{
    struct GrannyCorpse3d *corpse = mem_alloc(sizeof *corpse);

    if (!corpse)
        return;
    granny_corpse3d_construct(corpse, name);
    corpse->pos = pos;
    corpse->facing = facing;
    world_add(self, corpse);
}

void game_mode_make_mouse_pointer(struct GameMode *self)
{
    void *mem = mem_alloc(sizeof(struct MousePointer));
    void *pointer = mem != 0 ? mouse_pointer_construct(mem) : 0;

    self->mouse_pointer = pointer;
    world_add(self, pointer);
}

struct GameActor *game_mode_new_actor_for_job(struct GameMode *self, int job)
{
    const char *name;
    void *mem;

    if (session_is_pc_job(&g_session1, job) == 1) {
        mem = mem_alloc(sizeof(struct CharacterNpc));
        return mem != 0 ? (struct GameActor *)character_npc_construct(mem) : 0;
    }
    name = session_get_job_name(&g_session1, job, -1);
    if (strstr(name, ".gr2") != 0) {
        mem = mem_alloc(sizeof(struct C3dGrannyGameActor));
        return mem != 0 ? (struct GameActor *)c3d_granny_game_actor_construct(mem, name) : 0;
    }
    mem = mem_alloc(sizeof(struct Npc));
    return mem != 0 ? (struct GameActor *)npc_construct(mem) : 0;
}

char game_mode_launch_effect_at(struct GameMode *self, int effect_id, struct GameActor *caster,
                                        struct GameActor *target, int level)
{
    struct Vec3 pos, at;
    struct Effect *spawned;
    struct RagEffect *effect;

    if (caster == 0 || target == 0)
        return 0;
    if (effect_mgr_is_registered(g_effectMgr, effect_id)) {
        pos.x = caster->pos.x;
        pos.y = caster->pos.y;
        pos.z = caster->pos.z;
        spawned = effect_mgr_spawn(g_effectMgr, caster->aid, effect_id, pos, target->aid, -1, 0, 0);
        if (spawned == 0)
            return 0;
        if (level != -1)
            spawned->variant = level;
        if (effect_id == 0x21b) {
            pos.x = caster->pos.x;
            pos.y = caster->pos.y;
            pos.z = caster->pos.z;
            effect_mgr_spawn(g_effectMgr, caster->aid, 0x3d4, pos, target->aid, -1, 0, 0);
        }
        return 1;
    }
    pos.x = pos.y = pos.z = 0.0f;
    effect = game_actor_launch_effect(caster, effect_id, pos, 0);
    if (effect == 0)
        return 0;
    at.x = target->pos.x;
    at.y = target->pos.y;
    at.z = target->pos.z;
    rag_effect_send_msg(effect, NULL, 0xe, (intptr_t)&at, 0, 0);
    if (level != -1)
        rag_effect_send_msg(effect, NULL, 0x2c, (int64_t)level, 0, 0);
    return 1;
}
