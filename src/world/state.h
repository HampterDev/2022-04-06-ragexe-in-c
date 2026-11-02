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

struct Player;
struct GameMode;
struct RswBounds;

struct World {
    struct GameMode *mode;
    struct List gameObjects;
    struct List actors;
    struct List items;
    struct List skills;
    struct C3dGround15 *ground;
    struct Player *player;
    struct C3dAttr *attr;
    struct WorldModels models;
    int is_pvp, is_siege, is_battleground, is_event_pvp;
    struct SceneGraphNode root;
    const struct RswBounds *prebuilt;
    struct List pendingSkills;
    struct Tree idsByCell;
    struct List idsNowhere;
    BYTE nearFlag;
    int nearRange;
    struct WorldLoader *modelLoader;
};

#endif
