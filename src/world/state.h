#ifndef WORLD_STATE_H
#define WORLD_STATE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "platform/win_types.h"
#include "base.h"
#include "std/container_types.h"
#include "world/scene_types.h"
#include "world/models.h"
#include "world/id_index.h"

struct Player;
struct GameMode;
struct RswBounds;
struct WorldGpu;
struct GameActor;
struct Skill;

/* custom: what a node of the world's actors holds (struct World, actors): the actor, first, as the
   nodes of the other lists hold their objects, then its link in their index by aid
   (world/aid_index.h) */
struct ActorEntry {
    struct GameActor *actor;
    struct IdLink aidLink;
};

/* custom: what a node of the world's skill units holds (struct World, skills): the unit, first, as
   the nodes of the other lists hold their objects, then its link in their index by aid, and its
   node on the world's pendingSkills while the unit has left and stays only to end the hits it
   plays, NULL otherwise (world/aid_index.h) */
struct SkillEntry {
    struct Skill *skill;
    struct IdLink aidLink;
    struct ListNode *pendingNode;
};

struct World {
    struct GameMode *mode;
    struct List gameObjects;
    struct List actors; /* of struct ActorEntry */
    /* custom: the actors of `actors` by their aid (world_get_game_actor_by_aid) */
    struct IdIndex actorIndex;
    struct List items;
    struct List skills; /* of struct SkillEntry */
    /* custom: the skill units of `skills` by their aid (world_get_skill_by_aid) */
    struct IdIndex skillIndex;
    struct C3dGround15 *ground;
    struct Player *player;
    struct C3dAttr *attr;
    struct WorldModels models;
    int is_pvp, is_siege, is_battleground, is_event_pvp;
    struct SceneGraphNode root;
    const struct RswBounds *prebuilt;
    /* custom: of struct SkillEntry *, the units that left while they played hits, in the order they
       left; each goes when its hits end (world_process_leaving_skills). The original kept the aids
       here and never took one off. */
    struct List pendingSkills;
    struct Tree idsByCell;
    struct List idsNowhere;
    BYTE nearFlag;
    int nearRange;
    struct WorldLoader *modelLoader;
    /* custom: the map's static world on the device (world/world_gpu.c) */
    struct WorldGpu *gpu;
};

#endif
