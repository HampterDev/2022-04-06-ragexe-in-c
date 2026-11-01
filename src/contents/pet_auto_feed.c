#include "core/thread_text.h"
#include "session/item_predicates.h"
#include "session/state.h"
#include "core/memory.h"
#include "contents/game_session.h"
#include "contents/lua_data.h"
#include "std/std.h"
#include "contents/pet_auto_feed.h"

struct PetAutoFeedMgr *g_petAutoFeed;

static struct PetAutoFeedMgr *pet_auto_feed_construct(struct PetAutoFeedMgr *self)
{
    self->auto_feeding.first = self->auto_feeding.last = self->auto_feeding.end = 0;
    return self;
}

static void pet_auto_feed_destruct(struct PetAutoFeedMgr *self)
{
    vec_free(&self->auto_feeding);
}

static struct LuaDataResult *insert_auto_feeding(struct PetAutoFeedMgr *self,
                                                 struct LuaDataResult *out, int egg_id)
{
    const int *egg;

    for (egg = self->auto_feeding.first; egg != self->auto_feeding.last; egg++)
        if (*egg == egg_id)
            return lua_data_result(out, 1, "PetEggITID already exist in AutoFeedingPetList");
    vec_push(&self->auto_feeding, &egg_id, sizeof(int));
    return lua_data_result(out, 1, "good");
}

char pet_auto_feed_is_allowed(struct PetAutoFeedMgr *self, int egg_id)
{
    const int *egg;

    if (!self)
        return 0;
    for (egg = self->auto_feeding.first; egg != self->auto_feeding.last; egg++)
        if (*egg == egg_id)
            return 1;
    return 0;
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
    insert_auto_feeding(g_petAutoFeed, &result, egg_id);
    return lua_data_reply_result(L, &result, "InsertPetAutoFeeding : %s");
}

/* Older combined catalogs still call this while loading auto-feed eligibility. */
static int ignore_pet_recipe(lua_State *L)
{
    return lua_data_reply(L, 1, "good");
}

static const struct LuaDataFunction pet_auto_feed_functions[] = {
    { "InsertEvolutionRecipeLGU", ignore_pet_recipe },
    { "InsertPetAutoFeeding", lua_insert_pet_auto_feeding },
};

static char pet_auto_feed_load(void)
{
    return lua_data_run("system\\PetEvolutionCln_true.lub", "CPetAutoFeedMgr", pet_auto_feed_functions,
                        sizeof pet_auto_feed_functions / sizeof pet_auto_feed_functions[0],
                        LUA_DATA_ERROR_FORMAT);
}

char pet_auto_feed_create(void)
{
    struct PetAutoFeedMgr *mgr;

    if (g_petAutoFeed)
        return 0;
    mgr = (struct PetAutoFeedMgr *)mem_try_alloc(sizeof *mgr);
    if (mgr)
        pet_auto_feed_construct(mgr);
    g_petAutoFeed = mgr;
    if (!mgr)
        return 0;
    if (pet_auto_feed_load())
        return 1;
    pet_auto_feed_destroy();
    return 0;
}

void pet_auto_feed_destroy(void)
{
    if (!g_petAutoFeed)
        return;
    pet_auto_feed_destruct(g_petAutoFeed);
    mem_free(g_petAutoFeed);
    g_petAutoFeed = 0;
}
