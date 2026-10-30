#include "core/thread_text.h"
#include "session/item_predicates.h"
#include "session/state.h"
#include "core/memory.h"
#include "contents/game_session.h"
#include "contents/lua_data.h"
#include "std/std.h"
#include "contents/pet_evolution.h"

static void destroy_egg_recipes(void *value);

struct PetEvolutionMgr *g_petTable;

static struct PetEvolutionMgr *pet_evolution_construct(struct PetEvolutionMgr *self)
{
    tree_init(&self->recipes);
    self->auto_feeding.first = self->auto_feeding.last = self->auto_feeding.end = 0;
    return self;
}

static void pet_evolution_destruct(struct PetEvolutionMgr *self)
{
    tree_free(&self->recipes, destroy_egg_recipes);
    vec_free(&self->auto_feeding);
}

static struct LuaDataResult *insert_auto_feeding(struct PetEvolutionMgr *self,
                                                 struct LuaDataResult *out, int egg_id)
{
    const int *egg;

    for (egg = self->auto_feeding.first; egg != self->auto_feeding.last; egg++)
        if (*egg == egg_id)
            return lua_data_result(out, 1, "PetEggITID already exist in AutoFeedingPetList");
    vec_push(&self->auto_feeding, &egg_id, sizeof(int));
    return lua_data_result(out, 1, "good");
}

static struct LuaDataResult *insert_recipe(struct PetEvolutionMgr *self, struct LuaDataResult *out,
                                           int egg_id, int evolved_egg_id, int material_id, int count)
{
    struct PetEggRecipes *egg = tree_get(&self->recipes, &egg_id, sizeof egg_id, sizeof *egg, tree_less_int);
    struct Vec *recipes = &egg->recipes;
    struct PetRecipe *recipe;
    struct PetMaterial material, *have;

    material.item_id = material_id;
    material.count = count;
    for (recipe = recipes->first; recipe != recipes->last; recipe++)
        if (recipe->evolved_egg_id == evolved_egg_id)
            break;

    if (recipe == recipes->last) {
        struct PetRecipe added;

        added.evolved_egg_id = evolved_egg_id;
        vec_init(&added.materials);
        vec_push(&added.materials, &material, sizeof material);
        vec_push(recipes, &added, sizeof added);
        return lua_data_result(out, 1, "good");
    }
    for (have = recipe->materials.first; have != recipe->materials.last; have++)
        if (have->item_id == material_id)
            return lua_data_result(out, 0, "duplicate material ITID");
    vec_push(&recipe->materials, &material, sizeof material);
    return lua_data_result(out, 1, "good");
}

struct PetFoundList *pet_evolution_targets(struct PetEvolutionMgr *self,
                                                      struct PetFoundList *out, int egg_id)
{
    struct Vec targets = { 0, 0, 0 };
    const struct PetEggRecipes *egg = tree_find(&self->recipes, &egg_id, tree_less_int);
    const struct PetRecipe *recipe;

    if (!egg) {
        out->found = 0;
    } else {
        const struct Vec *recipes = &egg->recipes;

        for (recipe = recipes->first; recipe != recipes->last; recipe++)
            vec_push(&targets, &recipe->evolved_egg_id, sizeof(int));
        out->found = 1;
    }
    vec_copy(&out->list, &targets, sizeof(int));
    vec_free(&targets);
    return out;
}

struct PetFoundList *pet_evolution_materials(struct PetEvolutionMgr *self,
                                                        struct PetFoundList *out, int egg_id,
                                                        int evolved_egg_id)
{
    struct Vec materials = { 0, 0, 0 };
    const struct PetEggRecipes *egg = tree_find(&self->recipes, &egg_id, tree_less_int);
    const struct PetRecipe *recipe = 0;
    const struct PetMaterial *material;

    out->found = 0;
    if (egg) {
        const struct Vec *recipes = &egg->recipes;

        for (recipe = recipes->first; recipe != recipes->last; recipe++)
            if (recipe->evolved_egg_id == evolved_egg_id)
                break;
        if (recipe != recipes->last) {
            for (material = recipe->materials.first; material != recipe->materials.last; material++)
                vec_push(&materials, material, sizeof *material);
            out->found = 1;
        }
    }
    vec_copy(&out->list, &materials, sizeof(struct PetMaterial));
    vec_free(&materials);
    return out;
}

char pet_evolution_is_auto_feeding(struct PetEvolutionMgr *self, int egg_id)
{
    const int *egg;

    for (egg = self->auto_feeding.first; egg != self->auto_feeding.last; egg++)
        if (*egg == egg_id)
            return 1;
    return 0;
}

static int lua_insert_evolution_recipe(lua_State *L)
{
    struct LuaDataResult result;
    int egg_id, evolved_egg_id, material_id, count;

    if (!lua_isnumber(L, -4))
        return lua_data_reply(L, 0, "argment 1st is must number");
    if (!lua_isnumber(L, -3))
        return lua_data_reply(L, 0, "argment 2nd is must number");
    if (!lua_isnumber(L, -2))
        return lua_data_reply(L, 0, "argment 3th is must number");
    if (!lua_isnumber(L, -1))
        return lua_data_reply(L, 0, "argment 4th is must number");
    egg_id = (int)lua_tonumber(L, -4);
    evolved_egg_id = (int)lua_tonumber(L, -3);
    material_id = (int)lua_tonumber(L, -2);
    count = (int)lua_tonumber(L, -1);

    if (!session_is_pet_egg_id(&g_session1, egg_id)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, thread_text_format("PetEgg Item %d not found or not petegg", egg_id));
        return 2;
    }
    if (!session_is_pet_egg_id(&g_session1, evolved_egg_id)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, thread_text_format("Revolutuion PetEgg Item %d not found or not petegg",
                                             evolved_egg_id));
        return 2;
    }
    if (count <= 0) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, thread_text_format("Material Item Cnt %d Error", count));
        return 2;
    }
    insert_recipe(g_petTable, &result, egg_id, evolved_egg_id, material_id, count);
    return lua_data_reply_result(L, &result, "InsertRecipe : %s");
}

static int lua_insert_pet_auto_feeding(lua_State *L)
{
    struct LuaDataResult result;
    int egg_id;

    if (!lua_isnumber(L, -1))
        return lua_data_reply(L, 0, "argment 1st is must number");
    egg_id = (int)lua_tonumber(L, -1);
    if (!session_is_pet_egg_id(&g_session1, egg_id)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, thread_text_format("PetEgg Item %d not found or not petegg", egg_id));
        return 2;
    }
    insert_auto_feeding(g_petTable, &result, egg_id);
    return lua_data_reply_result(L, &result, "InsertPetAutoFeeding : %s");
}

static const struct LuaDataFunction pet_evolution_functions[] = {
    { "InsertEvolutionRecipeLGU", lua_insert_evolution_recipe },
    { "InsertPetAutoFeeding", lua_insert_pet_auto_feeding },
};

static char pet_evolution_load(void)
{
    return lua_data_run("system\\PetEvolutionCln_true.lub", "CPetEvolutionMgr", pet_evolution_functions,
                        sizeof pet_evolution_functions / sizeof pet_evolution_functions[0],
                        LUA_DATA_ERROR_FORMAT);
}

char pet_evolution_create(void)
{
    struct PetEvolutionMgr *mgr;

    if (g_petTable)
        return 0;
    mgr = (struct PetEvolutionMgr *)mem_try_alloc(sizeof *mgr);
    if (mgr)
        pet_evolution_construct(mgr);
    g_petTable = mgr;
    if (!mgr)
        return 0;
    if (pet_evolution_load())
        return 1;
    pet_evolution_destroy();
    return 0;
}

void pet_evolution_destroy(void)
{
    if (!g_petTable)
        return;
    pet_evolution_destruct(g_petTable);
    mem_free(g_petTable);
    g_petTable = 0;
}

static void pet_recipe_destruct(struct PetRecipe *recipe)
{
    vec_free(&recipe->materials);
}

static void pet_recipes_free(struct Vec *recipes)
{
    struct PetRecipe *recipe;

    for (recipe = recipes->first; recipe != recipes->last; recipe++)
        pet_recipe_destruct(recipe);
    vec_free(recipes);
}

static void destroy_egg_recipes(void *value)
{
    struct PetEggRecipes *egg = value;

    pet_recipes_free(&egg->recipes);
}
