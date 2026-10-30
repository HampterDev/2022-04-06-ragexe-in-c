#ifndef CONTENTS_PET_EVOLUTION_H
#define CONTENTS_PET_EVOLUTION_H
#include "std/std.h"

struct PetMaterial {
    int item_id;
    int count;
};
struct PetRecipe {
    int evolved_egg_id;
    struct Vec materials;
};
struct PetEggRecipes {
    int egg_id;
    struct Vec recipes;
};
struct PetEvolutionMgr {
    struct Tree recipes;
    struct Vec auto_feeding;
};
struct PetFoundList {
    char found;
    struct Vec list;
};
extern struct PetEvolutionMgr *g_petTable;

char pet_evolution_create(void);
void pet_evolution_destroy(void);
struct PetFoundList *pet_evolution_targets(struct PetEvolutionMgr *self,
                                         struct PetFoundList *out, int egg_id);
struct PetFoundList *pet_evolution_materials(struct PetEvolutionMgr *self,
                                           struct PetFoundList *out, int egg_id, int evolved_egg_id);
char pet_evolution_is_auto_feeding(struct PetEvolutionMgr *self, int egg_id);
#endif
