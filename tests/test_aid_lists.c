/* world/aid_lists.c, the world's actors and skill units found by their aid, against walks of the
   lists it replaced and a model of the units that leave while they play hits.

   Seeded runs of random steps: actors added (their aid set before or after they join, several
   sharing one), given other aids, ended by their processing (world/objects.c) or removed by aid,
   destroyed while their destructor looks itself up; units added with an aid or with aid 0 and given
   one later (packet 0x1c9, graffiti), given other aids, removed with and without hits left, shown
   again (world_keep_skill), their hits ending, processed. After each step every aid seen is looked
   up and must give the first object with it in the list's order, every node must hold its object's
   link under its object's aid, and the units that left must be the model's, in the order they left.
   Then the cases by name: a unit that leaves view while its hits play and comes back with the same
   aid, before its hits end and after it went; a unit replaced by one of the same aid while it plays
   hits; units sharing an aid; and removals from a destructor. */
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "diag/perf.h"
#include "effects/skills/skill_state.h"
#include "world/world.h"
#include "world/world_imports.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MSG_LEAVE 0x35

struct perf_counters g_perf;
struct EffectMgr *g_effectMgr;
static struct World world;
static long allocations;
static uint64_t rng;

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
    --allocations;
    free(p);
}

void world_models_process(struct World *w) { assert(w == &world); }
void world_forget_id_in_cell_map(struct World *w, int id, BYTE notify) { (void)id; assert(w == &world && notify); }
void world_forget_id_in_list_ec(struct World *w, int id, BYTE notify) { (void)id; assert(w == &world && notify); }
void effect_mgr_reset(struct EffectMgr *mgr) { (void)mgr; }
void cell_ids_destroy(void *value) { (void)value; abort(); }

static uint32_t next_random(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return (uint32_t)(rng >> 32);
}

/* ---- the test's objects ---- */

struct TestActor {
    struct GameActor actor;
    int ends;     /* its processing ends it */
    int self_aid; /* the aid it must find itself by while its destructor runs, or 0 */
};

struct TestUnit {
    struct Skill unit;
    int hits;          /* hits it still plays */
    int leaves;        /* messages 0x35 */
    int self_aid;      /* as for actors */
    int remove_aid;    /* an aid whose unit its destructor removes, or 0 */
    int *gone;         /* set when destroyed */
};

static int actors_destroyed, units_destroyed;

static void destroy_actor(struct GameObject *object)
{
    struct TestActor *a = (struct TestActor *)object;

    assert(!object->world_node);
    if (a->self_aid)
        assert(world_get_game_actor_by_aid(&world, (unsigned)a->self_aid) == &a->actor);
    ++actors_destroyed;
    memset(a, 0xdd, sizeof *a);
    mem_free(a);
}

static unsigned char process_actor(struct GameObject *object)
{
    return !((struct TestActor *)object)->ends;
}

static void destroy_unit(struct GameObject *object)
{
    struct TestUnit *u = (struct TestUnit *)object;

    assert(!object->world_node);
    if (u->self_aid)
        assert(world_get_skill_by_aid(&world, u->self_aid) == &u->unit);
    if (u->remove_aid)
        world_remove_skill(&world, u->remove_aid);
    if (u->gone)
        *u->gone = 1;
    ++units_destroyed;
    memset(u, 0xdd, sizeof *u);
    mem_free(u);
}

static unsigned char process_unit(struct GameObject *object)
{
    (void)object;
    return 1; /* as skill_on_process: a unit never ends by itself */
}

static void unit_message(struct GameObject *object, struct GameObject *sender, RenderMsgArg msg,
                         RenderMsgArg a, RenderMsgArg b, RenderMsgArg c, RenderMsgArg d)
{
    (void)sender; (void)a; (void)b; (void)c; (void)d;
    assert(msg == MSG_LEAVE);
    ((struct TestUnit *)object)->leaves++;
}

static void no_message(struct GameObject *object, struct GameObject *sender, RenderMsgArg msg,
                       RenderMsgArg a, RenderMsgArg b, RenderMsgArg c, RenderMsgArg d)
{
    (void)object; (void)sender; (void)msg; (void)a; (void)b; (void)c; (void)d;
    abort();
}

static const struct GameObjectOps actor_ops = { destroy_actor, process_actor, no_message, NULL, NULL };
static const struct GameObjectOps unit_ops = { destroy_unit, process_unit, unit_message, NULL, NULL };

unsigned char game_actor_has_pending_hits(struct GameActor *self)
{
    assert(self->ops == &unit_ops);
    return ((struct TestUnit *)self)->hits ? 1 : 0;
}

static struct TestActor *new_actor(int aid)
{
    struct TestActor *a = mem_alloc(sizeof *a);

    memset(a, 0, sizeof *a);
    a->actor.ops = &actor_ops;
    a->actor.aid = aid;
    return a;
}

static struct TestUnit *new_unit(int aid)
{
    struct TestUnit *u = mem_alloc(sizeof *u);

    memset(u, 0, sizeof *u);
    u->unit.base.ops = &unit_ops;
    u->unit.unit_aid = aid;
    return u;
}

static void world_start(void)
{
    memset(&world, 0, sizeof world);
    list_init(&world.gameObjects);
    list_init(&world.items);
    list_init(&world.idsNowhere);
    tree_init(&world.idsByCell);
    world_init_aid_lists(&world);
}

static void world_end(void)
{
    world_clear_objects(&world);
    assert(!world.gameObjects.size && !world.actors.size && !world.skills.size && !world.pendingSkills.size);
    assert(!world.actorIndex.ids && !world.skillIndex.ids);
    list_free(&world.gameObjects, NULL);
    list_free(&world.items, NULL);
    list_free(&world.idsNowhere, NULL);
    tree_free(&world.idsByCell, NULL);
    world_free_aid_lists(&world);
    assert(!allocations);
}

/* ---- the walks the indexes replaced ---- */

static struct GameActor *walk_actor(unsigned aid)
{
    struct ListNode *n;
    for (n = world.actors.head->next; n != world.actors.head; n = n->next)
        if ((unsigned)(*LIST_VALUE(n, struct GameActor *))->aid == aid)
            return *LIST_VALUE(n, struct GameActor *);
    return NULL;
}

static struct Skill *walk_unit(int aid)
{
    struct ListNode *n;
    for (n = world.skills.head->next; n != world.skills.head; n = n->next)
        if ((*LIST_VALUE(n, struct Skill *))->unit_aid == aid)
            return *LIST_VALUE(n, struct Skill *);
    return NULL;
}

/* every node holds its object, under its object's aid, and the object its node; the world's list
   of objects holds every actor and unit */
static void check_nodes(void)
{
    struct ListNode *n;
    for (n = world.actors.head->next; n != world.actors.head; n = n->next) {
        struct ActorEntry *e = LIST_VALUE(n, struct ActorEntry);
        assert(e->actor->actor_node == n && e->actor->world_node);
        assert(e->aidLink.indexed && e->aidLink.id == (uint32_t)e->actor->aid);
    }
    for (n = world.skills.head->next; n != world.skills.head; n = n->next) {
        struct SkillEntry *e = LIST_VALUE(n, struct SkillEntry);
        assert(e->skill->unit_node == n && e->skill->base.world_node);
        assert(e->aidLink.indexed && e->aidLink.id == (uint32_t)e->skill->unit_aid);
        assert(!e->pendingNode || *LIST_VALUE(e->pendingNode, struct SkillEntry *) == e);
    }
    for (n = world.pendingSkills.head->next; n != world.pendingSkills.head; n = n->next)
        assert((*LIST_VALUE(n, struct SkillEntry *))->pendingNode == n);
}

static void check_aid(int aid)
{
    assert(world_get_game_actor_by_aid(&world, (unsigned)aid) == walk_actor((unsigned)aid));
    assert(world_get_skill_by_aid(&world, aid) == walk_unit(aid));
}

/* ---- the units that left, as a model ---- */

#define MAX_UNITS 4096
static struct TestUnit *leaving[MAX_UNITS];
static size_t nleaving;

static int model_leaving(const struct TestUnit *u)
{
    size_t i;
    for (i = 0; i < nleaving; i++)
        if (leaving[i] == u)
            return 1;
    return 0;
}

static void model_drop(const struct TestUnit *u)
{
    size_t i;
    for (i = 0; i < nleaving; i++)
        if (leaving[i] == u) {
            memmove(leaving + i, leaving + i + 1, (nleaving - i - 1) * sizeof *leaving);
            nleaving--;
            return;
        }
}

static void check_leaving(void)
{
    struct ListNode *n;
    size_t i = 0;
    for (n = world.pendingSkills.head->next; n != world.pendingSkills.head; n = n->next, i++) {
        assert(i < nleaving);
        assert(&leaving[i]->unit == (*LIST_VALUE(n, struct SkillEntry *))->skill);
    }
    assert(i == nleaving);
}

/* ---- the random steps ---- */

static int random_aid(int aids)
{
    /* aid 0 and -1 (0xffffffff) among them */
    return (int)(next_random() % (uint32_t)aids) - 1;
}

static struct GameActor *any_actor(void)
{
    size_t k;
    struct ListNode *n;
    if (!world.actors.size)
        return NULL;
    k = next_random() % world.actors.size;
    for (n = world.actors.head->next; k--; n = n->next)
        ;
    return *LIST_VALUE(n, struct GameActor *);
}

static struct TestUnit *any_unit(void)
{
    size_t k;
    struct ListNode *n;
    if (!world.skills.size)
        return NULL;
    k = next_random() % world.skills.size;
    for (n = world.skills.head->next; k--; n = n->next)
        ;
    return (struct TestUnit *)*LIST_VALUE(n, struct Skill *);
}

/* world_remove_skill as the model says it goes */
static void remove_unit_by_aid(int aid)
{
    struct TestUnit *u = (struct TestUnit *)walk_unit(aid);
    int leaves = u ? u->leaves : 0, gone = 0, already = u && model_leaving(u);
    BYTE removed;

    if (u)
        u->gone = &gone;
    removed = world_remove_skill(&world, aid);
    if (!u) {
        assert(!removed);
        return;
    }
    if (gone) {
        assert(removed);
        model_drop(u);
        return;
    }
    assert(!removed && u->hits && u->leaves == leaves + 1);
    u->gone = NULL;
    if (!already)
        leaving[nleaving++] = u;
}

static void process(void)
{
    struct TestUnit *expect[MAX_UNITS];
    size_t i, n = nleaving, kept = 0;
    int gone[MAX_UNITS];

    for (i = 0; i < n; i++) {
        gone[i] = 0;
        leaving[i]->gone = &gone[i];
        expect[i] = leaving[i];
    }
    world_process_actors(&world);
    for (i = 0; i < n; i++) {
        if (gone[i])
            continue;
        assert(expect[i]->hits);
        expect[i]->gone = NULL;
        leaving[kept++] = expect[i];
    }
    nleaving = kept;
}

/* an aid that some actor or unit has, most of the time, else any */
static int listed_aid(int aids, int units)
{
    struct GameActor *a;
    struct TestUnit *u;

    if (next_random() % 4) {
        if (units && (u = any_unit()) != NULL)
            return u->unit.unit_aid;
        if (!units && (a = any_actor()) != NULL)
            return a->aid;
    }
    return random_aid(aids);
}

static void run(unsigned seed, int aids, int steps)
{
    int step, aid, k;

    rng = 0x9e3779b97f4a7c15ull * (seed + 1);
    world_start();
    nleaving = 0;
    for (step = 0; step < steps; step++) {
        unsigned what = next_random() % 16;
        struct GameActor *a;
        struct TestUnit *u;

        switch (what) {
        case 0: case 1: /* an actor joins with its aid */
            if (world.actors.size < 400)
                world_add_actor(&world, &new_actor(random_aid(aids))->actor);
            break;
        case 2: /* an actor joins and takes its aid after (moveentry10, the store assistant) */
            if (world.actors.size < 400) {
                a = &new_actor(0)->actor;
                world_add_actor(&world, a);
                world_set_actor_aid(&world, a, random_aid(aids));
            }
            break;
        case 3: /* an actor's aid changes (an entry again, the vanish's -1) */
            if ((a = any_actor()) != NULL)
                world_set_actor_aid(&world, a, next_random() % 4 ? random_aid(aids) : -1);
            break;
        case 4: /* actors end by their processing */
            if ((a = any_actor()) != NULL) {
                ((struct TestActor *)a)->ends = 1;
                ((struct TestActor *)a)->self_aid = 0;
            }
            process();
            break;
        case 5: /* an actor removed by its aid finds itself while its destructor runs */
            aid = listed_aid(aids, 0);
            a = walk_actor((unsigned)aid);
            if (a && aid)
                ((struct TestActor *)a)->self_aid = aid;
            assert(world_remove_actor_by_aid(&world, (unsigned)aid) == (a != NULL));
            break;
        case 6: case 7: /* a unit joins with its aid */
            if (world.skills.size < 400) {
                u = new_unit(random_aid(aids));
                u->hits = next_random() % 3 ? 0 : 1 + next_random() % 4;
                world_add_skill(&world, &u->unit);
            }
            break;
        case 8: /* a unit joins with aid 0 and takes its aid after (packet 0x1c9, graffiti) */
            if (world.skills.size < 400) {
                u = new_unit(0);
                world_add_skill(&world, &u->unit);
                world_set_skill_aid(&world, &u->unit, random_aid(aids));
            }
            break;
        case 9: /* a unit's aid changes */
            if ((u = any_unit()) != NULL)
                world_set_skill_aid(&world, &u->unit, random_aid(aids));
            break;
        case 10: case 11: /* the server says a unit left */
            remove_unit_by_aid(listed_aid(aids, 1));
            break;
        case 12: /* the server shows a unit again */
            if ((u = any_unit()) != NULL) {
                world_keep_skill(&world, &u->unit);
                model_drop(u);
            }
            break;
        case 13: case 14: /* hits end */
            if ((u = any_unit()) != NULL && u->hits)
                u->hits--;
            break;
        case 15:
            process();
            break;
        }
        check_nodes();
        check_leaving();
        if (aids <= 16 || step % 256 == 0)
            for (aid = -1; aid < aids - 1; aid++)
                check_aid(aid);
        else
            for (k = 0; k < 16; k++)
                check_aid(next_random() % 2 ? listed_aid(aids, (int)(next_random() % 2)) : random_aid(aids));
    }
    nleaving = 0;
    world_end();
}

/* ---- the cases by name ---- */

static struct TestUnit *add_unit(int aid, int hits)
{
    struct TestUnit *u = new_unit(aid);
    u->hits = hits;
    world_add_skill(&world, &u->unit);
    return u;
}

/* A unit leaves view while it plays hits and comes back with the same aid. Shown again before its
   hits end, it stays when they end; gone after they ended, the unit that comes then stays. The
   original kept the aid, not the unit, and so removed both. */
static void unit_leaves_and_comes_back(void)
{
    struct TestUnit *u, *again;
    int gone = 0;

    world_start();
    u = add_unit(7, 2);
    assert(!world_remove_skill(&world, 7) && u->leaves == 1 && world.pendingSkills.size == 1);
    world_process_actors(&world);
    assert(u->leaves == 2 && world_get_skill_by_aid(&world, 7) == &u->unit);
    world_keep_skill(&world, &u->unit); /* the server shows it again */
    assert(!world.pendingSkills.size);
    u->hits = 0;
    world_process_actors(&world);
    assert(u->leaves == 2 && world_get_skill_by_aid(&world, 7) == &u->unit);

    u->hits = 1;
    u->gone = &gone;
    assert(!world_remove_skill(&world, 7) && world.pendingSkills.size == 1);
    u->hits = 0;
    world_process_actors(&world);
    assert(gone && !world.pendingSkills.size && !world.skills.size && !world_get_skill_by_aid(&world, 7));
    again = add_unit(7, 0);
    world_process_actors(&world);
    world_process_actors(&world);
    assert(world_get_skill_by_aid(&world, 7) == &again->unit && again->leaves == 0);
    world_end();
}

/* A unit replaced by one of the same aid while it plays hits (game_mode_spawn_unit_double): the
   one that left is found first until it goes, the new one stays after. */
static void unit_replaced_while_it_plays_hits(void)
{
    struct TestUnit *old, *made;
    int gone = 0;

    world_start();
    old = add_unit(9, 1);
    old->gone = &gone;
    assert(!world_remove_skill(&world, 9));
    made = add_unit(9, 0);
    assert(world_get_skill_by_aid(&world, 9) == &old->unit);
    world_process_actors(&world);
    assert(!gone && world_get_skill_by_aid(&world, 9) == &old->unit);
    old->hits = 0;
    world_process_actors(&world);
    assert(gone && world_get_skill_by_aid(&world, 9) == &made->unit && made->leaves == 0);
    world_process_actors(&world);
    assert(world_get_skill_by_aid(&world, 9) == &made->unit && world.skills.size == 1);
    world_end();
}

/* Units that share an aid (graffiti sent twice) are found in the list's order, also after the
   first is given another aid and back, and after it goes. */
static void units_share_an_aid(void)
{
    struct TestUnit *a, *b, *c;

    world_start();
    a = add_unit(5, 0);
    b = add_unit(5, 0);
    c = add_unit(0, 0);
    assert(world_get_skill_by_aid(&world, 5) == &a->unit);
    world_set_skill_aid(&world, &a->unit, 6);
    assert(world_get_skill_by_aid(&world, 5) == &b->unit && world_get_skill_by_aid(&world, 6) == &a->unit);
    world_set_skill_aid(&world, &c->unit, 5);
    assert(world_get_skill_by_aid(&world, 5) == &b->unit);
    world_set_skill_aid(&world, &a->unit, 5);
    assert(world_get_skill_by_aid(&world, 5) == &a->unit);
    assert(world_remove_skill(&world, 5) && world_get_skill_by_aid(&world, 5) == &b->unit);
    assert(world_remove_skill(&world, 5) && world_get_skill_by_aid(&world, 5) == &c->unit);
    check_nodes();
    world_end();
}

/* A unit is found by its aid while its destructor runs, and a destructor that removes another unit
   (one that left among them) leaves the lists right; so does an actor's. */
static void removals_from_a_destructor(void)
{
    struct TestUnit *a, *b, *c;
    struct TestActor *x, *y;

    world_start();
    a = add_unit(1, 0);
    b = add_unit(2, 1);
    c = add_unit(3, 0);
    assert(!world_remove_skill(&world, 2)); /* b plays a hit: it stays */
    a->self_aid = 1;
    a->remove_aid = 3;
    assert(world_remove_skill(&world, 1));
    assert(!world_get_skill_by_aid(&world, 1) && !world_get_skill_by_aid(&world, 3));
    assert(world.skills.size == 1 && world.pendingSkills.size == 1);
    b->hits = 0;
    b->self_aid = 2;
    world_process_actors(&world);
    assert(!world.skills.size && !world.pendingSkills.size && units_destroyed >= 3);
    (void)c;

    x = new_actor(11);
    y = new_actor(11);
    world_add_actor(&world, &x->actor);
    world_add_actor(&world, &y->actor);
    x->self_aid = 11;
    assert(world_remove_actor_by_aid(&world, 11));
    assert(world_get_game_actor_by_aid(&world, 11) == &y->actor);
    y->ends = 1;
    world_process_actors(&world);
    assert(!world.actors.size && !world_get_game_actor_by_aid(&world, 11));
    check_nodes();
    world_end();
}

int main(void)
{
    unsigned seed;

    unit_leaves_and_comes_back();
    unit_replaced_while_it_plays_hits();
    units_share_an_aid();
    removals_from_a_destructor();
    for (seed = 0; seed < 8; seed++)
        run(seed, seed & 1 ? 8 : 300, 10000);
    printf("Aid lists: %d actors and %d units destroyed; lookups equal the walks, the units that left the "
           "model's\n", actors_destroyed, units_destroyed);
    return 0;
}
