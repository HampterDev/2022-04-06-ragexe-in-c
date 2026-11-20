#include "render/screen_bounds.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resources/granny_res.h"
#include "owned_storage_test_allocator.h"

void *model_test_calloc(size_t count, size_t size)
{
    void *p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
void model_test_free(void *memory) { checked_free(memory); }
#define malloc checked_malloc
#define calloc model_test_calloc
#define free checked_free
#include "../src/resources/granny_playback.c"
#include "../src/actors/granny_model_storage.c"
#undef malloc
#undef calloc
#undef free
#include "../src/actors/granny_model.c"
#include "granny_sdk_test_support.h"

struct ModelFixture {
    struct C3dGrannyModelRes resource;
    struct GrannyResModel model;
    granny_model sdk_model;
    granny_skeleton skeleton;
};
static struct ModelFixture fixtures[3];
static struct C3dGrannyModelRes *loaded_resource;

static void fixture_init(struct ModelFixture *f)
{
    memset(f, 0, sizeof *f);
    f->resource.model = &f->model; f->resource.grannyModel = &f->sdk_model;
    f->resource.skeleton = &f->skeleton; f->skeleton.BoneCount = 3;
    f->resource.bounds.half_width = f->resource.bounds.half_height = 10.0f;
    f->sdk_model.Skeleton = &f->skeleton;
}
void res_lock(struct Res *res) { res->lockCount++; }
void res_unlock(struct Res *res) { assert(res->lockCount > 0); res->lockCount--; }
struct ResMgr *g_resMgr(void) { return (struct ResMgr *)1; }
void *res_mgr_get(struct ResMgr *mgr, const char *name)
{
    assert(mgr == (struct ResMgr *)1 && name);
    return loaded_resource;
}
void game_actor_set_job(struct GameActor *actor, int job) { actor->job = job; }
void game_actor_set_sex(struct GameActor *actor, int sex) { actor->sex = sex; }

static void assert_clean(void)
{
    int i;
    assert(!outstanding); sdk_assert_empty();
    for (i = 0; i < 3; i++) assert(!fixtures[i].resource.res.lockCount);
}

/* A model holds its resource and its playback while it lives; every allocation or SDK call that
   fails leaves nothing behind, and a resource without a model or skeleton makes none. */
static void test_create_failures(void)
{
    struct GrannyModelState *model;
    unsigned i, count, before;
    assert(!granny_model_state_create(NULL));
    fixture_init(&fixtures[0]);
    before = allocation_calls;
    model = granny_model_state_create(&fixtures[0].resource); assert(model);
    count = allocation_calls - before;
    assert(model->resource == &fixtures[0].resource && model->playback && fixtures[0].resource.res.lockCount == 1);
    assert(model->bounds.half_width == 10.0f);
    granny_model_state_destroy(model); assert_clean();
    for (i = 1; i <= count; i++) {
        fail_at = allocation_calls + i;
        assert(!granny_model_state_create(&fixtures[0].resource));
        fail_at = 0; assert_clean();
    }
    before = sdk_calls;
    model = granny_model_state_create(&fixtures[0].resource); assert(model);
    count = sdk_calls - before;
    granny_model_state_destroy(model); assert_clean();
    for (i = 1; i <= count; i++) {
        sdk_fail_at = sdk_calls + i;
        assert(!granny_model_state_create(&fixtures[0].resource));
        sdk_fail_at = 0; assert_clean();
    }
    fixtures[0].resource.model = NULL;
    assert(!granny_model_state_create(&fixtures[0].resource));
    fixture_init(&fixtures[0]);
    fixtures[0].resource.skeleton = NULL;
    assert(!granny_model_state_create(&fixtures[0].resource));
    granny_model_state_destroy(NULL);
    assert_clean();
}

/* An actor's model is replaced whole or not at all; the treasure boxes and their kin have bounds of
   their own; a corpse takes and gives up its model. */
static void test_replacement(void)
{
    struct C3dGrannyGameActor actor = {0};
    struct GrannyCorpse3d corpse = {0};
    struct GrannyModelState *old;
    unsigned i, calls, before, old_allocations;
    int j;
    for (j = 0; j < 3; j++) fixture_init(&fixtures[j]);
    loaded_resource = &fixtures[0].resource;
    c3d_granny_game_actor_set_job_model((struct GameActor *)&actor, 1, 0); assert(actor.model);
    old = actor.model; before = allocation_calls;
    c3d_granny_game_actor_set_job_model((struct GameActor *)&actor, 2, 1); assert(actor.model != old);
    calls = allocation_calls - before; old = actor.model; old_allocations = outstanding;
    for (i = 1; i <= calls; i++) {
        fail_at = allocation_calls + i;
        c3d_granny_game_actor_set_job_model((struct GameActor *)&actor, 3, 0); fail_at = 0;
        assert(actor.model == old && actor.job == 2 && actor.sex == 1 && outstanding == old_allocations);
    }
    c3d_granny_game_actor_set_job_model((struct GameActor *)&actor, 0x49d, 0);
    assert(actor.model->bounds.half_width == 30.0f && fixtures[0].resource.bounds.half_width == 10.0f);
    assert(actor.model_kind == 1);
    c3d_granny_game_actor_set_job_model((struct GameActor *)&actor, 0x2d2, 0);
    assert(actor.model->bounds.half_width == 10.0f && actor.model_kind == 2);
    granny_model_state_destroy(actor.model); assert_clean();
    granny_corpse3d_load_model(&corpse); assert(corpse.model);
    granny_corpse3d_load_model(&corpse);
    granny_corpse3d_free_model(&corpse); granny_corpse3d_free_model(&corpse); assert_clean();
}

/* Two models of one resource pose apart. */
static void test_pose(void)
{
    struct GrannyModelState *model, *other;
    granny_world_pose *pose;
    fixture_init(&fixtures[0]);
    model = granny_model_state_create(&fixtures[0].resource); assert(model);
    assert(granny_playback_play(model->playback, 2, 0));
    pose = granny_playback_pose(model->playback, 0.25f);
    assert(pose == granny_playback_world_pose(model->playback));
    other = granny_model_state_create(&fixtures[0].resource); assert(other);
    granny_playback_pose(other->playback, 90.0f);
    assert(pose->time == 0.25f && fixtures[0].resource.res.lockCount == 2);
    granny_model_state_destroy(other);
    granny_model_state_destroy(model); assert_clean();
}

int main(void)
{
    test_create_failures();
    test_replacement();
    test_pose();
    puts("Granny models: complete allocation/SDK rollback, atomic actor model replacement and owned poses passed");
    return 0;
}
