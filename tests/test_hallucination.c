#include "actor/actor_ops.h"
#include "core/memory.h"
#include "actor/hallucination.h"
#include "actor/actor.h"
#include "actor/pc.h"
#include "game/context.h"
#include <assert.h>
#include <stdio.h>

static struct Pc source, temporary;
static struct HalluPC figure;
static struct List *objects;
static int fail_pc, fail_figure, allocations, destructions, frees, inserted;
const struct GameActorOps pc_ops = {0};
const struct GameActorOps player_ops = {0};
const struct GameActorOps blur_pc_ops = {0};
const struct GameActorOps hallu_pc_ops = {0};
const struct GameActorOps hallu_pc2_ops = {0};

struct List *game_current_object_list(void) { return objects; }

void *mem_try_alloc(size_t bytes)
{
    assert(bytes == sizeof temporary);
    ++allocations;
    return fail_pc ? NULL : &temporary;
}

void *mem_alloc(size_t bytes)
{
    assert(bytes == sizeof figure);
    ++allocations;
    return fail_figure ? NULL : &figure;
}

void mem_free(void *memory)
{
    assert(memory == &temporary);
    assert(destructions == frees + 1);
    ++frees;
}

struct Pc *pc_construct(struct Pc *pc)
{
    assert(pc == &temporary);
    memset(pc, 0, sizeof *pc);
    return pc;
}

void pc_destruct(struct Pc *pc) { assert(pc == &temporary); ++destructions; }
int game_actor_get_job(struct GameActor *actor) { assert((void *)actor == &source); return 0x1092; }
void game_actor_set_job(struct GameActor *actor, int job)
{
    assert((void *)actor == &temporary && job == 0x1092);
}

void pc_set_head(struct GameActor *actor, int head)
{
    struct Pc *pc = (struct Pc *)actor;

    assert(pc == &temporary && head == 13);
    pc->head = head;
}
void pc_set_imf_name(struct GameActor *actor)
{
    struct Pc *pc = (struct Pc *)actor;

    assert(pc == &temporary && pc->sex == 1 && pc->head == 13);
}
void pc_set_body_palette(struct GameActor *actor, int palette)
{
    struct Pc *pc = (struct Pc *)actor;

    assert(pc == &temporary && palette == 29);
    pc->body_palette = palette;
}
void pc_set_head_palette(struct GameActor *actor, int palette)
{
    struct Pc *pc = (struct Pc *)actor;

    assert(pc == &temporary && palette == 47);
    pc->head_palette = palette;
}
void pc_set_body_style(struct GameActor *actor, int style, unsigned char apply)
{
    struct Pc *pc = (struct Pc *)actor;

    assert(pc == &temporary && style == 5 && apply == 0);
    pc->body_style = style;
}

struct RenderObject *hallu_pc_construct(void *memory, struct RenderObject *actor)
{
    assert(memory == &figure && (void *)actor == &temporary);
    assert(temporary.sex == 1 && temporary.head == 13 && temporary.body_palette == 29);
    assert(temporary.head_palette == 47 && temporary.body_style == 5);
    memset(&figure, 0, sizeof figure);
    figure.argb = 0x37abcdef;
    figure.owner = (struct GameActor *)&temporary;
    return (struct RenderObject *)&figure;
}

void *list_insert(struct List *list, struct ListNode *where, const void *value, size_t size)
{
    assert(list == objects && where == objects->head);
    assert(size == sizeof(void *) && *(void *const *)value == &figure);
    assert(figure.owner == NULL);
    assert(figure.pos.x == -100 && figure.pos.y == 7 && figure.pos.z == 230);
    ++inserted;
    return NULL;
}

int main(void)
{
    struct List world_objects = {0};
    struct Npc npc = {0};
    struct GameActor other_pc = {0};
    const float position[3] = {-100, 7, 230};
    source.sex = 3;
    source.ops = &pc_ops.base.base;
    source.head = 13;
    source.body_palette = 29;
    source.head_palette = 47;
    source.body_style = 5;

    actor_spawn_hallu_copy((struct GameActor *)&source, position, 0xb4);
    objects = &world_objects;
    actor_spawn_hallu_copy(NULL, position, 0xb4);
    actor_spawn_hallu_copy((struct GameActor *)&npc, position, 0xb4);
    other_pc.is_pc = 1;
    actor_spawn_hallu_copy(&other_pc, position, 0xb4);
    assert(allocations == 0 && inserted == 0);
    fail_pc = 1;
    actor_spawn_hallu_copy((struct GameActor *)&source, position, 0xb4);
    assert(allocations == 1 && destructions == 0 && inserted == 0);
    fail_pc = 0;
    fail_figure = 1;
    actor_spawn_hallu_copy((struct GameActor *)&source, position, 0xb4);
    assert(allocations == 3 && destructions == 1 && frees == 1 && inserted == 0);
    fail_figure = 0;
    actor_spawn_hallu_copy((struct GameActor *)&source, position, 0xb4);
    assert(figure.argb == 0xb4ffffff && inserted == 1 && frees == 2);
    source.ops = &player_ops.base.base;
    actor_spawn_hallu_copy((struct GameActor *)&source, position, -1);
    assert(figure.argb == 0x37ffffff && inserted == 2 && frees == 3);
    assert(source.sex == 3 && source.head == 13 && source.body_palette == 29);
    assert(source.head_palette == 47 && source.body_style == 5);
    puts("hallucination: named appearance fields, alpha, world ownership and allocation cleanup passed");
    return 0;
}
