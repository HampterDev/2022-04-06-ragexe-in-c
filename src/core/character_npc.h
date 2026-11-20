#include "actors/actor_ops.h"
#include "actors/object_ops.h"
#ifndef CORE_CHARACTER_NPC_H
#define CORE_CHARACTER_NPC_H

#include "platform/win_types.h"
#include "base.h"
#include "std/std.h"
#include "actors/types.h"
#include "ui/render/character_render.h"

struct CharacterNpc {
    CNPC_MEMBERS
    struct CharacterRender *model;
};

struct CharacterNpc *character_npc_construct(struct CharacterNpc *self);
void character_npc_destruct(struct CharacterNpc *self);

#endif
