#include "actor/object_ops.h"
#include "core/abstract_method.h"
#include "core/fatal.h"
#include "core/memory.h"
#include "world/world.h"
#include "effects/effect_mgr.h"
#include "effects/skills/dummy_effect_launcher.h"
#include "effects/rag/effect_prim.h"
#include <mmsystem.h>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TestObject {
    struct GameActor actor;
    unsigned char alive;
    int processed, destroyed;
};

struct EffectMgr *g_effectMgr;
static struct World world;
static int allocations, releases, forgotten_cell, forgotten_list, actor_nodes;
static int model_passes, index_clears, effects_clears, tree_clears;
static struct GameObject *root_allocation;
static jmp_buf fatal_jump;
static int expect_fatal;
static struct DummyEffectLauncher *dummy;
static int dummy_cleanup, forwarded_messages, spawned_effects, launched_effects;

void *mem_alloc(size_t size)
{
    void *p = malloc(size);
    assert(p);
    ++allocations;
    return p;
}

void mem_free(void *p)
{
    assert(p);
    if (p == root_allocation) {
        assert(root_allocation->ops == &game_object_ops);
        root_allocation = NULL;
    }
    if (p == dummy) {
        assert(dummy_cleanup == 1);
        dummy = NULL;
    }
    ++releases;
    free(p);
}

void fatal_error(const char *message)
{
    assert(expect_fatal && !strcmp(message, "abstract method called"));
    longjmp(fatal_jump, 1);
}

void able_to_make_effect_dc_able_to_make_effect(struct AbleToMakeEffect *object)
{
    assert(object == &dummy->base);
    ++dummy_cleanup;
}
void able_to_make_effect_send_msg(struct GameObject *object, struct GameObject *sender,
    RenderMsgArg msg, RenderMsgArg a, RenderMsgArg b, RenderMsgArg c, RenderMsgArg d)
{
    assert(object == (struct GameObject *)dummy && sender == object);
    assert(msg == INT64_MAX && a == INT64_MIN && b == -1 && c == INT64_MAX && d == 7);
    ++forwarded_messages;
}
struct Effect *effect_mgr_spawn(struct EffectMgr *mgr, int aid, int type, Vec3 pos,
    int target, int level, unsigned char reuse, int instance)
{
    assert(mgr == g_effectMgr && aid == 0 && type == 123 && target == 0);
    assert(level == -1 && !reuse && !instance && pos.x == 1 && pos.y == 2 && pos.z == 3);
    ++spawned_effects;
    return NULL;
}
struct RagEffect *game_actor_launch_effect(struct GameActor *object, int id, Vec3 at, float rotation)
{
    assert(object == (struct GameActor *)dummy && id == 123);
    assert(at.x == 0 && at.y == 0 && at.z == 0 && rotation == 0);
    ++launched_effects;
    return NULL;
}
int game_object_dir_of(struct GameObject *object, float roty)
{
    assert(object && roty == 90.0f);
    return 7;
}
void world_models_process(struct World *w) { assert(w == &world); ++model_passes; }
void world_video_list_update(struct WorldVideoList *list) { (void)list; abort(); }
void rag_effect_send_msg(struct RagEffect *self, struct RenderObject *sender,
    RenderMsgArg msg, RenderMsgArg a, RenderMsgArg b, RenderMsgArg c)
{
    (void)self; (void)sender; (void)msg; (void)a; (void)b; (void)c;
    abort();
}
void world_forget_id_in_cell_map(struct World *w, int id, BYTE notify)
{
    assert(w == &world && id == 42 && notify == 1);
    ++forgotten_cell;
}
void world_forget_id_in_list_ec(struct World *w, int id, BYTE notify)
{
    assert(w == &world && id == 42 && notify == 1);
    ++forgotten_list;
}
void actor_index_drop_node(const struct ListNode *node) { (void)node; ++actor_nodes; }
void actor_index_clear(void) { ++index_clears; }
void effect_mgr_reset(struct EffectMgr *mgr) { assert(mgr == g_effectMgr); ++effects_clears; }
void cell_ids_destroy(void *value)
{
    struct CellIds *cell = value;
    assert(cell->cell == 7.0f && !cell->ids.size);
    list_free(&cell->ids, NULL);
    ++tree_clears;
}
static int cell_less(const void *a, const void *b)
{
    return *(const float *)a < *(const float *)b;
}
BYTE game_actor_has_pending_hits(void *skill) { (void)skill; abort(); }

static unsigned char process_object(struct GameObject *base)
{
    struct TestObject *object = (struct TestObject *)base;
    ++object->processed;
    return object->alive;
}

static void destroy_object(struct GameObject *base)
{
    struct TestObject *object = (struct TestObject *)base;
    if (object->actor.aid == 42)
        assert(forgotten_cell == 1 && forgotten_list == 1 && actor_nodes == 1);
    assert(object->destroyed == 0);
    ++object->destroyed;
}

static const struct GameObjectOps test_ops = {
    destroy_object, process_object, game_object_send_msg, game_object_render_base, game_object_dir_of
};

static void append(struct List *list, void *object)
{
    list_push_back(list, &object, sizeof object);
}

static void world_lifetime(void)
{
    struct TestObject actor = {0}, expired = {0}, survivor = {0};
    struct CellIds cell;
    struct List *lists[] = { &world.gameObjects, &world.actors, &world.items,
        &world.skills, &world.pendingSkills, &world.idsNowhere };
    unsigned i;

    for (i = 0; i < sizeof lists / sizeof lists[0]; ++i)
        list_init(lists[i]);
    tree_init(&world.idsByCell);
    cell.cell = 7.0f;
    list_init(&cell.ids);
    tree_add(&world.idsByCell, &cell, sizeof cell, cell_less, NULL);
    actor.actor.ops = expired.actor.ops = survivor.actor.ops = &test_ops;
    actor.actor.aid = 42;
    survivor.alive = 1;
    append(&world.gameObjects, &actor);
    append(&world.gameObjects, &expired);
    append(&world.gameObjects, &survivor);
    append(&world.actors, &actor);

    world_process_actors(&world);
    assert(model_passes == 1 && world.gameObjects.size == 1 && world.actors.size == 0);
    assert(actor.processed == 1 && expired.processed == 1 && survivor.processed == 1);
    assert(actor.destroyed == 1 && expired.destroyed == 1 && survivor.destroyed == 0);
    assert(*LIST_VALUE(world.gameObjects.head->next, void *) == &survivor);

    world_remove_object(&world, &expired);
    world_remove_object(&world, &survivor);
    world_remove_object(&world, &survivor);
    assert(expired.destroyed == 1 && survivor.destroyed == 1 && world.gameObjects.size == 0);

    survivor.destroyed = 0;
    append(&world.gameObjects, NULL);
    append(&world.gameObjects, &survivor);
    append(&world.items, &survivor);
    world.player = (struct Player *)&survivor;
    world_clear_objects(&world);
    assert(survivor.destroyed == 1 && !world.player);
    assert(index_clears == 1 && effects_clears == 1 && tree_clears == 1 && !world.idsByCell.size);
    tree_free(&world.idsByCell, NULL);
    for (i = 0; i < sizeof lists / sizeof lists[0]; ++i) {
        assert(lists[i]->size == 0 && lists[i]->head->next == lists[i]->head);
        assert(lists[i]->head->prev == lists[i]->head);
        list_free(lists[i], NULL);
    }
}

static void defaults(void)
{
    struct GameObject object = {0}, before;
    struct GameObjectOps render_ops = game_object_ops;
    float view[16] = {0};

    object.ops = &game_object_ops;
    object.serial = 123;
    before = object;
    assert(game_object_process(&object) == 1);
    game_object_send(&object, &object, INT64_MIN, INT64_MAX, -1, 0, 9);
    game_object_render(&object, view);
    assert(game_object_direction(&object, 90.0f) == 7);
    assert(!memcmp(&before, &object, sizeof object));
    render_ops.on_process = render_object_on_process;
    render_ops.send_msg = render_object_send_msg;
    object.ops = &render_ops;
    assert(game_object_process(&object) == 0);
    game_object_send(&object, NULL, 1, 2, 3, 4, 5);

    root_allocation = mem_alloc(sizeof *root_allocation);
    root_allocation->ops = &render_ops;
    game_object_destroy(root_allocation);
    assert(!root_allocation);

    expect_fatal = 1;
    if (setjmp(fatal_jump) == 0) {
        game_object_abstract_process(&object);
        abort();
    }
    expect_fatal = 0;
}

static void dummy_lifetime(void)
{
    struct GameObjectOps ops = game_object_ops;
    struct GameObject *object;
    Vec3 at = {1, 2, 3};

    dummy = mem_alloc(sizeof *dummy);
    memset(dummy, 0, sizeof *dummy);
    ops.destroy = dummy_effect_launcher_deleting_destruct;
    ops.on_process = dummy_effect_launcher_on_process;
    ops.send_msg = dummy_effect_launcher_send_msg;
    dummy->base.ops = &ops;
    object = (struct GameObject *)dummy;
    game_object_send(object, object, INT64_MAX, INT64_MIN, -1, INT64_MAX, 7);
    assert(forwarded_messages == 1);
    game_object_send(object, NULL, 0xe, MSG_ARG_PTR(&at), 123, 0, 0);
    assert(dummy->frames == 300 && dummy->base.visible == 0);
    assert(spawned_effects == 1 && launched_effects == 1);
    dummy->base.start_tick = timeGetTime();
    dummy->frames = INT_MAX;
    assert(game_object_process(object) == 1);
    dummy->frames = -1;
    assert(game_object_process(object) == 0);
    game_object_destroy(object);
    assert(!dummy && dummy_cleanup == 1);
}

static struct RagEffect message_effect;
static struct EffectPrim message_prim;
static int root_messages;

static void root_message(struct GameObject *object, struct GameObject *sender,
    RenderMsgArg msg, RenderMsgArg a, RenderMsgArg b, RenderMsgArg c, RenderMsgArg d)
{
    assert(object == (struct GameObject *)&message_effect || object == (struct GameObject *)&message_prim);
    assert(sender == (struct GameObject *)&message_effect);
    assert(msg == INT64_MIN && a == INT64_MAX && b == -1 && c == 7 && d == -9);
    ++root_messages;
}

static void effect_dispatch(void)
{
    static const struct RenderObjectOps effect_ops = {.base = {.send_msg = root_message}};

    message_effect.base.ops = &effect_ops.base;
    message_prim.base.ops = &effect_ops.base;
    render_object_send(&message_effect, &message_effect, INT64_MIN, INT64_MAX, -1, 7, -9);
    render_object_send(&message_prim, &message_effect, INT64_MIN, INT64_MAX, -1, 7, -9);
    assert(root_messages == 2);
    puts("Effect dispatch: messages to effects and primitives reach their object operation with all 64-bit values.");
}

int main(void)
{
    defaults();
    world_lifetime();
    dummy_lifetime();
    effect_dispatch();
    assert(allocations == releases);
    puts("Object operations: defaults, fatal policy, frame removal, map cleanup and dummy lifetime passed");
    return 0;
}
