#include "core/memory.h"
#include "actor/hallucination.h"
#include "actor/actor.h"
#include "actor/pc.h"
#include "game/context.h"

static const struct Pc *sprite_player(const struct GameActor *actor)
{
    const void *table;
    if (!actor)
        return NULL;
    table = actor->ops;
    if (table != &pc_ops && table != &player_ops &&
        table != &blur_pc_ops && table != &hallu_pc_ops && table != &hallu_pc2_ops)
        return NULL;
    return (const struct Pc *)actor;
}

void actor_spawn_hallu_copy(const struct GameActor *actor, const float position[3], int alpha)
{
    struct List *objects = game_current_object_list();
    const struct Pc *source = sprite_player(actor);
    struct Pc *pc;
    struct HalluPC *figure;
    void *memory;
    int job, sex, head, body_palette, head_palette, body_style;

    if (!source || !objects)
        return;
    job = game_actor_get_job((struct GameActor *)actor);
    sex = source->sex;
    head = source->head;
    body_palette = source->body_palette;
    head_palette = source->head_palette;
    body_style = source->body_style;
    memory = mem_try_alloc(sizeof *pc);
    pc = memory ? pc_construct(memory) : NULL;
    if (!pc)
        return;

    pc->sex = sex & 1;
    game_actor_set_job((struct GameActor *)pc, job);
    pc_set_head((struct GameActor *)pc, head);
    pc_set_imf_name((struct GameActor *)pc);
    pc_set_body_palette((struct GameActor *)pc, body_palette);
    pc_set_head_palette((struct GameActor *)pc, head_palette);
    pc_set_body_style((struct GameActor *)pc, body_style, 0);

    memory = mem_alloc(sizeof *figure);
    figure = memory ? (struct HalluPC *)hallu_pc_construct(memory, (struct RenderObject *)pc) : NULL;
    if (figure) {
        figure->pos.x = position[0];
        figure->pos.y = position[1];
        figure->pos.z = position[2];
        figure->argb = (alpha == -1 ? figure->argb & 0xff000000u : (unsigned)(unsigned char)alpha << 24)
                       | 0x00ffffffu;
        figure->owner = NULL;
        list_push_back(objects, &figure, sizeof figure);
    }
    pc_destruct(pc);
    mem_free(pc);
}
