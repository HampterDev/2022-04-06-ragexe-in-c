#include "core/object_type_registry.h"
#include "core/object_type_tables.h"

/* The object types of src/actor/: the actor classes. core/object_type.h describes the table.
   In the order of the class names. */
static const struct ObjectTypeRow k_rows[] = {
    {&c3d_granny_game_actor_ops, OBJECT_TYPE(GAME_ACTOR), "C3dGrannyGameActor"},
    {&blur_pc_ops, OBJECT_TYPE(PC) | OBJECT_TYPE(GAME_ACTOR), "CBlurPC"},
    {&g_characterNpcVtbl_ops, OBJECT_TYPE(GAME_ACTOR), "CCharacterNpc"},  /* core/character_npc.c */
    {&game_actor_ops, OBJECT_TYPE(GAME_ACTOR), "CGameActor"},
    {&hallu_npc_ops, OBJECT_TYPE(GAME_ACTOR), "CHalluNPC"},
    {&hallu_pc_ops, OBJECT_TYPE(PC) | OBJECT_TYPE(GAME_ACTOR), "CHalluPC"},
    {&hallu_pc2_ops, OBJECT_TYPE(PC) | OBJECT_TYPE(GAME_ACTOR), "CHalluPC2"},
    {&hallu_plant_ops, OBJECT_TYPE(GAME_ACTOR), "CHalluPlant"},
    {&npc_ops, OBJECT_TYPE(GAME_ACTOR), "CNpc"},
    {&pc_ops, OBJECT_TYPE(PC) | OBJECT_TYPE(GAME_ACTOR), "CPc"},
    {&player_ops, OBJECT_TYPE(PC) | OBJECT_TYPE(GAME_ACTOR), "CPlayer"},
};

const struct ObjectTypeRows g_actorObjectTypes = OBJECT_TYPE_ROWS(k_rows);
