#include "session/state.h"
#include "../src/effects/rag/prim_kinds_116_128.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

float g_degreeTables[6 * 361] = { [4 * 361] = 1.0f };
struct Session g_session1;
static struct EffectPrim prim;
static struct RenderObject actor;
static struct RagEffect spawned;
static int actor_present, player_present, actor_queries, heading_calls, launches, messages;
static int expected_effect;

void prim_draw_quad(struct EffectPrim *self, int *count, short alpha, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                    const float *matrix, short blend, char texture, unsigned char in_world,
                    unsigned char r, unsigned char g, unsigned char b, float v_p2p3, float v_p0p1,
                    float u_p0p3, float u_p1p2, unsigned char mode)
{
    assert(!"target processing does not draw quads");
}

void rag_effect_send_msg(struct RagEffect *self, struct RenderObject *sender,
                                     RenderMsgArg msg, RenderMsgArg position, RenderMsgArg beat, RenderMsgArg last)
{
    const Vec3 *at = (const Vec3 *)(size_t)position;
    assert(self == &spawned && sender == NULL && msg == RAG_MSG_OFFSET && last == 0);
    assert(at->x == 5.0f && at->z == 9.0f);
    ++messages;
}

struct Player *game_current_player(void)
{
    return player_present ? (struct Player *)&actor : NULL;
}

struct GameActor *game_find_actor(int aid)
{
    assert(aid == 99);
    ++actor_queries;
    return actor_present ? (struct GameActor *)&actor : NULL;
}

void prim_heading_degrees(float x, float z, short *out)
{
    assert(x == 10.0f && z == 20.0f);
    ++heading_calls;
    *out = 0;
}

struct RagEffect *game_actor_launch_effect(struct GameActor *owner, int type, Vec3 offset, float rotation)
{
    assert((struct RenderObject *)owner == &actor && player_present && type == expected_effect);
    assert(offset.x == 0.0f && offset.y == 0.0f && offset.z == 0.0f && rotation == 0.0f);
    ++launches;
    return &spawned;
}

static void prepare(void)
{
    struct PrimFlatSquare *square;
    memset(&prim, 0, sizeof prim);
    actor.pos.x = 10.0f;
    actor.pos.z = 20.0f;
    prim.delta.y = 99.0f;
    square = &prim.records.flat_squares[0];
    square->active = 1;
    square->alpha = 50;
    heading_calls = actor_queries = 0;
    actor_present = player_present = 0;
}

int main(void)
{
    g_session1.ownAid = 42;
    struct PrimFlatSquare *square;
    prepare();
    square = &prim.records.flat_squares[0];
    square->state = SQUARE_FADING;
    prim_homing_square_process(&prim);
    assert(square->frame == 1 && square->state == SQUARE_FADING && heading_calls == 0);

    prepare();
    prim_homing_square_process(&prim);
    assert(square->state == SQUARE_RETURNING && square->frame == 1 && actor_queries == 1);

    prepare();
    actor_present = 1;
    prim_homing_square_process(&prim);
    assert(heading_calls == 1 && square->heading == 90 && square->frame == 1);
    actor_present = 0;
    prim_homing_square_process(&prim);
    assert(square->state == SQUARE_RETURNING && square->frame == 2);

    prepare();
    prim.delta.y = (float)g_session1.ownAid;
    player_present = 1;
    prim_homing_square_process(&prim);
    assert(heading_calls == 1 && actor_queries == 0);
    player_present = 0;
    prim_homing_square_process(&prim);
    assert(square->state == SQUARE_RETURNING);

    prepare();
    prim_veering_square_process(&prim);
    assert(square->frame == 1 && square->state == SQUARE_FADING && heading_calls == 0);
    square->frame = 5;
    prim_veering_square_process(&prim);
    assert(square->alpha == 53 && square->frame == 6);

    prepare();
    prim.records.effect_spawners[0].mode = SPAWN_EVERY_FRAME;
    prim_spawn_effects_render(&prim);
    prim.records.effect_spawners[0].mode = SPAWN_EVERY_8;
    prim_spawn_effects_render(&prim);
    assert(launches == 0);
    prim.records.effect_spawners[0].at.x = 5.0f;
    prim.records.effect_spawners[0].at.z = 9.0f;
    player_present = 1;
    expected_effect = EFFECT_SPAWNED_0;
    prim_spawn_effects_render(&prim);
    assert(launches == 1 && messages == 1);
    prim.records.effect_spawners[0].mode = SPAWN_EVERY_FRAME;
    expected_effect = EFFECT_SPAWNED_1;
    prim_spawn_effects_render(&prim);
    assert(launches == 2 && messages == 2);
    puts("effect targets: missing, active and removed targets, own player and absent spawner passed");
    return 0;
}
