#ifndef CONTENTS_CHAMPION_MONSTER_EFFECTS_H
#define CONTENTS_CHAMPION_MONSTER_EFFECTS_H
struct ChampionMonsterMgr;
struct GameActor;

int champion_monster_apply_effects(struct ChampionMonsterMgr *manager, int job,
                                  struct GameActor *actor, int actor_id);

#endif
