#ifndef CONTENTS_CHAMPION_MONSTER_H
#define CONTENTS_CHAMPION_MONSTER_H
#include "std/std.h"
#include "std/tree.h"
#include "contents/champion_monster_effects.h"

struct ChampionMonsterData {
    float size;
    struct Vec effects;
};
struct ChampionMonster {
    int job;
    struct ChampionMonsterData data;
};
struct ChampionMonsterMgr {
    struct Tree jobs;
};
extern struct ChampionMonsterMgr *g_championMonster;

char champion_monster_create(void);
void champion_monster_destroy(void);
char champion_monster_load(void);
float champion_monster_size(struct ChampionMonsterMgr *self, int job);
char champion_monster_is_champion(struct ChampionMonsterMgr *self, int job);
#endif
