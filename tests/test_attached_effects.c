/* An actor's attached message effects as they leave its list (actors/attached.h): each effect keeps
   its node on the list (effects/skills/msg_effect.c, msg_effect_attach) and gives it with message
   0x30 from its destructor (msg_effect_leave_owner); the actor takes the node off only where it
   holds the sender (game_actor_detach). The test's actor answers 0x30 as the client's does.

   Seeded runs: effects attached (some as the actor's marks), given up in random order, damage
   totals (style 0x16) shown again and again (actors/show_number.c, which takes the previous total's
   digits off itself), the owner gone with effects still attached (0x31); after each step the list
   must hold exactly the live attached effects in the order they came, each effect's node must be
   its own on that list, and the marks must name live effects. Then the guards: a 0x30 whose node
   holds another object, or no node, changes nothing. */
#include "actors/attached.h"
#include "actors/game_actor.h"
#include "actors/object_ops.h"
#include "actors/render_ops.h"
#include "actors/render_object.h"
#include "effects/skills/msg_effect.h"
#include "game/context.h"
#include "render/math3d.h"
#include "resources/animation.h"
#include "resources/attr_queries.h"
#include "session/state.h"
#include "world/world.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MSG_SET_STYLE 0x16
#define MSG_OWNER_LEAVES 0x30
#define MSG_OWNER_GONE 0x31
#define MSG_SET_OWNER 0x32

struct Session g_session1;
static struct World world;
static struct GameActor actor;
static long allocations;
static uint64_t rng, serial;
static int forgotten, detaches;

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

/* ---- what msg_effect.c and show_number.c reach beyond this test ---- */

struct GameMode *game_current_mode(void) { return (struct GameMode *)&world; }
struct World *game_current_world(void) { return &world; }
void game_remove_object(void *object) { world_remove_object(&world, object); }
void game_actor_forget_handle(struct GameActor *self, struct GameObject *handle)
{
    assert(self == &actor && handle);
    ++forgotten;
}
struct AbleToMakeEffect *able_to_make_effect_construct(struct AbleToMakeEffect *self)
{
    memset(self, 0, sizeof *self);
    self->serial = ++serial;
    return self;
}
void able_to_make_effect_dc_able_to_make_effect(struct AbleToMakeEffect *self) { (void)self; }
void able_to_make_effect_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg a, RenderMsgArg b, RenderMsgArg c, RenderMsgArg d)
{
    (void)object; (void)source; (void)a; (void)b; (void)c; (void)d;
    assert(msg == 0x18); /* show_number's word to the total it takes off */
}
void render_object_load_spr_res(struct RenderObject *self, const char *name) { (void)self; (void)name; }
void render_object_load_act_res(struct RenderObject *self, const char *name) { (void)self; (void)name; }
uint32_t platform_ticks_ms(void) { return 0; }

/* what the effects' other messages, their following and their method table reach, which this test
   does not */
void render_object_set_argb(struct RenderObject *self, int a, int r, int g, int b)
{ (void)self; (void)a; (void)r; (void)g; (void)b; abort(); }
int game_object_dir_of(struct GameObject *object, float roty) { (void)object; (void)roty; abort(); }
unsigned char msg_effect_on_process(struct GameObject *object) { (void)object; abort(); }
void render_object_update_motion_base(struct RenderObject *object) { (void)object; abort(); }
void render_object_set_action_base(struct RenderObject *object, int action, int type)
{ (void)object; (void)action; (void)type; abort(); }
void game_object_set_render_info(struct RenderObject *object, const struct ScreenBounds *bounds, float x, float y)
{ (void)object; (void)bounds; (void)x; (void)y; abort(); }
int render_object_return_zero(struct RenderObject *object) { (void)object; abort(); }
float float_sqrt(float x) { (void)x; abort(); }
float get_cos_deg(int deg) { (void)deg; abort(); }
float get_sin_deg(int deg) { (void)deg; abort(); }
unsigned actres_frame_count(const struct ActRes *self, unsigned action) { (void)self; (void)action; abort(); }
float c3d_attr_get_height(struct C3dAttr *self, float x, float z) { (void)self; (void)x; (void)z; abort(); }
void *render_object_get_act_res(struct RenderObject *self) { (void)self; abort(); }
float render_object_calc_ground_dist(struct RenderObject *self, float x, float z)
{ (void)self; (void)x; (void)z; abort(); }
void render_object_turn_toward(struct RenderObject *self, float x, float z) { (void)self; (void)x; (void)z; abort(); }
int render_object_camera_dir(float roty, int rounded) { (void)roty; (void)rounded; abort(); }
struct C3dAttr *game_current_map_attr(void) { abort(); }

/* ---- the actor: 0x30 as game_actor_handle_msg answers it ---- */

static void actor_message(struct GameObject *object, struct GameObject *sender, RenderMsgArg msg,
                          RenderMsgArg a, RenderMsgArg b, RenderMsgArg c, RenderMsgArg d)
{
    (void)b; (void)c; (void)d;
    assert(object == (struct GameObject *)&actor);
    if (msg == MSG_OWNER_LEAVES) {
        ++detaches;
        game_actor_detach(&actor, MSG_PTR(a), sender);
    }
}

static void no_destroy(struct GameObject *object) { (void)object; abort(); }
static const struct GameObjectOps actor_ops = { no_destroy, NULL, actor_message, NULL, NULL };

static uint32_t next_random(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return (uint32_t)(rng >> 32);
}

/* ---- the model: the attached effects in the order they came ---- */

#define MAX_ATTACHED 512
static struct MsgEffect *model[MAX_ATTACHED];
static size_t nmodel;

static void model_drop(const struct MsgEffect *e)
{
    size_t i;
    for (i = 0; i < nmodel; i++)
        if (model[i] == e) {
            memmove(model + i, model + i + 1, (nmodel - i - 1) * sizeof *model);
            nmodel--;
            return;
        }
    abort();
}

static int in_model(const void *e)
{
    size_t i;
    for (i = 0; i < nmodel; i++)
        if (model[i] == e)
            return 1;
    return 0;
}

static void check(void)
{
    struct ListNode *n;
    size_t i = 0;
    struct GameObject *const *marks[] = { &actor.cart, &actor.falcon, &actor.emotion_mark, &actor.frozen_mark,
        &actor.curse_mark, &actor.sleep_mark, &actor.stun_mark, &actor.msg_mark_1, &actor.msg_mark_2,
        &actor.emotion_mark_a, &actor.emotion_mute, &actor.cursor_mark };

    assert(actor.attached.size == nmodel);
    for (n = actor.attached.head->next; n != actor.attached.head; n = n->next, i++) {
        struct MsgEffect *e = *LIST_VALUE(n, struct MsgEffect *);
        assert(e == model[i] && e->attached_node == n && e->owner == (struct RenderObject *)&actor);
        assert(e->base.world_node);
    }
    for (i = 0; i < sizeof marks / sizeof marks[0]; i++)
        assert(!*marks[i] || in_model(*marks[i]));
}

/* an effect made, put on the world's objects and attached as the actor's code does it */
static struct MsgEffect *attach_new(int style)
{
    struct MsgEffect *e = msg_effect_construct(mem_alloc(sizeof *e));

    render_object_send(e, NULL, MSG_SET_STYLE, actor.aid, style, 0, 0);
    world_add_object(&world, e);
    msg_effect_attach(e, &actor.attached);
    render_object_send(e, &actor, MSG_SET_OWNER, 0, 0, 0, 0);
    model[nmodel++] = e;
    return e;
}

/* the damage total's digits on the list, from show_number's rebuilds */
static void model_from_list(void)
{
    struct ListNode *n;
    nmodel = 0;
    for (n = actor.attached.head->next; n != actor.attached.head; n = n->next)
        model[nmodel++] = *LIST_VALUE(n, struct MsgEffect *);
}

static void run(unsigned seed, int steps)
{
    int step;

    rng = 0x9e3779b97f4a7c15ull * (seed + 1);
    for (step = 0; step < steps; step++) {
        unsigned what = next_random() % 8;
        struct MsgEffect *e;

        switch (what) {
        case 0: case 1:
            if (nmodel < MAX_ATTACHED - 16) {
                e = attach_new(next_random() % 4 ? 0xe : 0x2c);
                switch (next_random() % 4) {
                case 0: actor.emotion_mute = (struct GameObject *)e; break;
                case 1: actor.cart = (struct GameObject *)e; break;
                case 2: actor.stun_mark = (struct GameObject *)e; break;
                }
            }
            break;
        case 2: case 3: /* an attached effect goes: its destructor gives its node */
            if (nmodel) {
                e = model[next_random() % nmodel];
                model_drop(e);
                world_remove_object(&world, e);
            }
            break;
        case 4: /* the emotion mark goes as message 0x76 takes it (actor_sendmsg.c) */
            if (actor.emotion_mute) {
                model_drop((struct MsgEffect *)actor.emotion_mute);
                game_remove_object(actor.emotion_mute);
                assert(!actor.emotion_mute);
            }
            break;
        case 5: { /* a damage total again: the previous total's digits go, the new ones come */
            int value = (int)(next_random() % 100000), digits = 0, v = value;
            size_t others = 0, i;
            struct ListNode *n;

            do digits++; while ((v /= 10) != 0);
            for (i = 0; i < nmodel; i++)
                others += model[i]->style != 0x16;
            game_actor_show_number(&actor, value, 0xffffffff, 0x16);
            model_from_list();
            assert(actor.attached.size == others + (size_t)digits);
            for (n = actor.attached.head->next, i = 0; n != actor.attached.head; n = n->next)
                i += (*LIST_VALUE(n, struct MsgEffect *))->style == 0x16;
            assert(i == (size_t)digits);
            break;
        }
        case 6: { /* a 0x30 whose node holds another object, or none, changes nothing */
            size_t before = actor.attached.size;
            if (nmodel >= 2) {
                struct MsgEffect *a = model[0], *b = model[1];
                render_object_send(&actor, b, MSG_OWNER_LEAVES, (intptr_t)a->attached_node, 0, 0, 0);
            }
            render_object_send(&actor, nmodel ? model[0] : NULL, MSG_OWNER_LEAVES, 0, 0, 0, 0);
            assert(actor.attached.size == before);
            break;
        }
        case 7: /* rarely, the owner goes with effects attached (release_attached) */
            if (next_random() % 64 == 0) {
                size_t i, n = nmodel;
                struct MsgEffect *gone[MAX_ATTACHED];
                int sent = detaches;

                memcpy(gone, model, n * sizeof *model);
                for (i = 0; i < n; i++)
                    render_object_send(gone[i], &actor, MSG_OWNER_GONE, 0, 0, 0, 0);
                list_clear(&actor.attached, NULL);
                memset(&actor.cart, 0, (char *)&actor.attached - (char *)&actor.cart);
                nmodel = 0;
                for (i = 0; i < n; i++) {
                    assert(!gone[i]->attached_node && !gone[i]->owner);
                    world_remove_object(&world, gone[i]);
                }
                assert(detaches == sent);
            }
            break;
        }
        check();
    }
}

int main(void)
{
    unsigned seed;
    struct MsgEffect *a, *b;
    struct ListNode *node;

    list_init(&world.gameObjects);
    memset(&actor, 0, sizeof actor);
    actor.ops = &actor_ops;
    actor.aid = 77;
    list_init(&actor.attached);

    /* the guards by name */
    a = attach_new(0xe);
    b = attach_new(0xe);
    actor.cart = (struct GameObject *)a;
    node = a->attached_node;
    render_object_send(&actor, b, MSG_OWNER_LEAVES, (intptr_t)node, 0, 0, 0);
    assert(actor.attached.size == 2 && actor.cart == (struct GameObject *)a && a->attached_node == node);
    render_object_send(&actor, a, MSG_OWNER_LEAVES, 0, 0, 0, 0);
    assert(actor.attached.size == 2 && actor.cart == (struct GameObject *)a);
    world_remove_object(&world, a);
    model_drop(a);
    assert(actor.attached.size == 1 && !actor.cart && b->attached_node == actor.attached.head->next);
    check();

    for (seed = 0; seed < 8; seed++)
        run(seed, 20000);
    while (nmodel) {
        a = model[nmodel - 1];
        model_drop(a);
        world_remove_object(&world, a);
    }
    check();
    assert(!world.gameObjects.size && !actor.attached.size);
    list_free(&world.gameObjects, NULL);
    list_free(&actor.attached, NULL);
    assert(!allocations);
    printf("Attached effects: %d left their actor by their own node, %d digits forgotten as totals came "
           "again\n", detaches, forgotten);
    return 0;
}
