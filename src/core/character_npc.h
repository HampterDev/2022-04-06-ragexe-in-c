#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#ifndef CORE_CHARACTER_NPC_H
#define CORE_CHARACTER_NPC_H

#include <windows.h>
#include "base.h"
#include "std/std.h"
#include "actor/types.h"
#include "ui/render/character_render.h"

struct CharacterNpc {
    CNPC_MEMBERS
    struct CharacterRender *model;
};

struct CharacterNpc *character_npc_construct(struct CharacterNpc *self);
void character_npc_destruct(struct CharacterNpc *self);

#endif
