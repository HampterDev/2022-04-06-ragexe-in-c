#include "actor/appearance.h"
#include "actor/actor_ops.h"
#include "actor/actor.h"
#include "actor/pc.h"
#include "actor/granny_pc.h"
#include "session/state.h"
#include "game/actor_entry.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

struct Session g_session1;
static void *current_actor;
static int job_queries, query_job_offset;
static int order[32], order_count, prepare_ok, prepare_calls, weapon_view;
static int64_t arguments[60];

static void record(void *actor, int slot, int64_t value)
{
    assert(actor == current_actor && order_count < 32);
    order[order_count++] = slot;
    arguments[slot] = value;
}
#define INT_SLOT(name, n) static void fake_##name(struct GameActor *actor, int value) { record(actor, n, value); }
#define VOID_SLOT(name, n) static void fake_##name(struct GameActor *actor) { record(actor, n, 0); }
INT_SLOT(set_head, 22) INT_SLOT(set_accessory, 25) INT_SLOT(set_accessory2, 26) INT_SLOT(set_accessory3, 27)
INT_SLOT(set_shield, 28) INT_SLOT(set_body_palette, 33)
VOID_SLOT(set_imf_name, 30) VOID_SLOT(set_head_pal_name, 31) VOID_SLOT(set_body_pal_name, 32)
static void fake_set_weapon(struct GameActor *actor, int64_t value) { record(actor, 23, value); }
#define GRANNY_INT(name, n) void granny_pc_##name(struct GameActor *actor, int value) { record(actor, n, value); }
#define GRANNY_VOID(name, n) void granny_pc_##name(struct GameActor *actor) { record(actor, n, 0); }
GRANNY_INT(set_head, 22) GRANNY_INT(set_accessory, 25) GRANNY_INT(set_accessory2, 26)
GRANNY_INT(set_accessory3, 27) GRANNY_INT(set_shield, 28)
GRANNY_VOID(set_imf_name, 30) GRANNY_VOID(set_head_pal_name, 31)
void granny_pc_set_weapon(struct GameActor *actor, int64_t value) { record(actor, 23, value); }
static void fake_set_weapon_look(struct GameActor *actor, int64_t value) { record(actor, 24, value); }

void game_actor_set_sex(struct GameActor *actor, int sex) { actor->sex = sex; }
void game_actor_set_job(struct GameActor *actor, int job) { actor->job = job; }
int game_actor_get_job(struct GameActor *actor)
{ ++job_queries; return actor->job + query_job_offset; }
int session_get_mounted_job(int job)
{ return job + 100; }
int session_get_weapon_view_id(int weapon)
{ (void)weapon; return weapon_view; }
int granny_pc_replace_models(struct GrannyPc *actor)
{ assert(actor == current_actor); ++prepare_calls; return prepare_ok; }

static void begin(void *actor, struct GameActorOps *table)
{
    static const struct GameActorOps callbacks = {
        .set_head = fake_set_head, .set_weapon = fake_set_weapon, .set_weapon_look = fake_set_weapon_look,
        .set_accessory = fake_set_accessory, .set_accessory2 = fake_set_accessory2, .set_accessory3 = fake_set_accessory3,
        .set_shield = fake_set_shield, .set_imf_name = fake_set_imf_name, .set_head_pal_name = fake_set_head_pal_name,
        .set_body_pal_name = fake_set_body_pal_name, .set_body_palette = fake_set_body_palette
    };
    *table = callbacks;
    ((struct RenderObject *)actor)->ops = &table->base.base;
    current_actor = actor;
    order_count = 0;
    memset(arguments, 0, sizeof arguments);
}

static void expect_order(const int *expected, size_t count)
{
    assert((size_t)order_count == count);
    assert(!memcmp(order, expected, count * sizeof *expected));
}

static void sprite_look(void)
{
    static const int full[] = {22,23,25,26,27,28,30,31,33,32};
    static const int plain[] = {22,24,25,26,27,28,30,31,32};
    static const int transformed[] = {22,25,26,27,30,31,32};
    static const int dual_jobs[] = {0xc,0xfad,0xfc3,0xfdb,0xfe1,0x1005};
    static const int ids[] = {INT_MIN,-1,0,1,INT_MAX};
    static const int states[] = {1 << 12,1 << 16,1 << 18,1 << 27,1 << 28};
    static const int drawn[] = {0x16,0x1a,0x1b,0x1c,0x1d};
    struct ActorLook look = {7,1,11,0,{12,13,14},15,16,17}, saved;
    struct Pc pc, expected;
    struct GameActorOps table;
    unsigned i, j;

    for (i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
        for (j = 0; j < sizeof ids / sizeof ids[0]; ++j) {
            memset(&pc, 0, sizeof pc);
            begin(&pc, &table); table.set_look = pc_set_spr_act;
            look.weapon = actor_weapon_pair(ids[i], ids[j]); saved = look;
            assert((uint32_t)look.weapon == (uint32_t)ids[i]);
            assert((uint32_t)((uint64_t)look.weapon >> 32) == (uint32_t)ids[j]);
            expected = pc;
            expected.look_job = expected.job = look.job; expected.sex = look.sex;
            expected.head = look.head; expected.weapon = look.weapon;
            memcpy(expected.accessory, look.accessory, sizeof look.accessory);
            expected.shield = look.shield; expected.head_palette = look.head_palette;
            expected.body_palette = look.body_palette;
            actor_set_look(&pc, &look);
            assert(!memcmp(&pc, &expected, sizeof pc) && !memcmp(&look, &saved, sizeof look));
            expect_order(full, sizeof full / sizeof full[0]);
            assert(arguments[23] == look.weapon && arguments[28] == look.shield);

            begin(&pc, &table); table.set_look_with_state = pc_set_spr_act_with_state;
            weapon_view = -1;
            actor_set_look_with_state(&pc, &look, 0);
            expect_order(plain, sizeof plain / sizeof plain[0]);
            assert(arguments[24] == ids[i] && arguments[28] == ids[j]);
            assert(pc.weapon == look.weapon && pc.shield == ids[j]);
            assert(!memcmp(&look, &saved, sizeof look));
        }
    }
    look.weapon = actor_weapon_pair(-123, -456);
    for (i = 0; i < sizeof dual_jobs / sizeof dual_jobs[0]; ++i) {
        begin(&pc, &table); table.set_look_with_state = pc_set_spr_act_with_state;
        look.job = dual_jobs[i]; weapon_view = 0;
        actor_set_look_with_state(&pc, &look, 0);
        assert(arguments[24] == look.weapon && pc.shield == 0);
    }
    look.job = 7;
    for (i = 0; i < sizeof states / sizeof states[0]; ++i) {
        begin(&pc, &table); table.set_look_with_state = pc_set_spr_act_with_state;
        actor_set_look_with_state(&pc, &look, states[i]);
        expect_order(transformed, sizeof transformed / sizeof transformed[0]);
        assert(pc.job == drawn[i] && pc.look_job == look.job);
    }
    begin(&pc, &table); table.set_look_with_state = pc_set_spr_act_with_state;
    pc.mounted = 1;
    actor_set_look_with_state(&pc, &look, 0);
    assert(pc.job == look.job + 100);
    expect_order(transformed, sizeof transformed / sizeof transformed[0]);
    pc.mounted = 0;
    for (i = 0; i < 3; ++i) {
        begin(&pc, &table); table.set_look_with_state = pc_set_spr_act_with_state;
        look.job = 0xfa1; look.body_palette = (int)i;
        actor_set_look_with_state(&pc, &look, 0);
        assert(pc.body_palette == (i < 2 ? 1 : 2));
    }
}

static void model_look(void)
{
    static const int full[] = {22,23,25,26,27,28,30,31};
    struct ActorLook look = {7,1,11,0,{12,13,14},15,16,17}, saved;
    struct GrannyPc pc = {0}, expected;
    struct GameActorOps table;

    look.weapon = actor_weapon_pair(INT_MIN, -123); saved = look;
    begin(&pc, &table); table.set_look = granny_pc_set_spr_act;
    table.set_look_with_state = granny_pc_set_spr_act_with_state;
    expected = pc; prepare_ok = 0; prepare_calls = 0;
    actor_set_look(&pc, &look);
    assert(prepare_calls == 1 && !order_count && !memcmp(&pc, &expected, sizeof pc));
    prepare_ok = 1;
    actor_set_look(&pc, &look);
    expected.job = look.job; expected.sex = look.sex; expected.head = look.head;
    expected.weapon = look.weapon; expected.accessory = look.accessory[0];
    expected.accessory2 = look.accessory[1]; expected.accessory3 = look.accessory[2];
    expected.shield = look.shield; expected.head_palette = look.head_palette;
    expected.body_palette = look.body_palette;
    assert(!memcmp(&pc, &expected, sizeof pc));
    expect_order(full, sizeof full / sizeof full[0]);
    assert(arguments[23] == look.weapon);
    order_count = 0; pc.shield = expected.shield = 999;
    actor_set_look_with_state(&pc, &look, INT_MIN);
    assert(!memcmp(&pc, &expected, sizeof pc) && arguments[28] == 999);
    assert(!memcmp(&look, &saved, sizeof look));
    expect_order(full, sizeof full / sizeof full[0]);
}

static void entry_appearance(void)
{
    struct ActorEntryInfo standing = {0};
    struct ActorMoveEntryInfo moving = {0};
    struct GrannyPc pc = {0};
    struct ActorLook look;
    struct GameActorOps table;
    int pass;

    standing.job = moving.job = 0xfad;
    standing.sex = moving.sex = 1;
    standing.head = moving.head = 22;
    standing.weapon[0] = moving.weapon[0] = INT_MIN;
    standing.weapon[1] = moving.weapon[1] = -456;
    standing.accessory = moving.accessory = 33;
    standing.accessory2 = moving.accessory2 = 44;
    standing.accessory3 = moving.accessory3 = 55;
    standing.shield = moving.shield = 66;
    standing.head_palette = moving.head_palette = 77;
    standing.body_palette = moving.body_palette = 88;
    for (pass = 0; pass < 2; ++pass) {
        look = pass ? actor_look_from_move_entry(&moving) : actor_look_from_entry(&standing);
        begin(&pc, &table); table.set_look = granny_pc_set_spr_act;
        prepare_ok = 1;
        actor_set_look(&pc, &look);
        assert(pc.job == 0xfad && pc.sex == 1 && pc.head == 22);
        assert(pc.weapon == actor_weapon_pair(INT_MIN, -456));
        assert(pc.accessory == 33 && pc.accessory2 == 44 && pc.accessory3 == 55);
        assert(pc.shield == 66 && pc.head_palette == 77 && pc.body_palette == 88);
        assert(arguments[23] == pc.weapon && arguments[28] == 66);
    }
}

static void query_appearance(void)
{
    static struct Player player;
    struct PlayerAppearance appearance, untouched;
    struct ActorScreenPosition screen = { -1, -2, -3.0f };

    memset(&appearance, 0x5a, sizeof appearance);
    untouched = appearance;
    assert(!player_get_appearance(NULL, &appearance));
    assert(memcmp(&appearance, &untouched, sizeof appearance) == 0);
    assert(job_queries == 0);
    assert(player_state(NULL) == -1);
    assert(!actor_get_screen_position(NULL, &screen));
    assert(screen.x == -1 && screen.y == -2 && screen.scale == -3.0f);

    player.job = 7;
    player.look_job = 8;
    player.sex = 1;
    player.head = 34;
    player.body_palette = 2;
    player.head_palette = 5;
    player.accessory[0] = 1001;
    player.accessory[1] = 1002;
    player.accessory[2] = 1003;
    player.robe = 15;
    player.body_style = 4;
    player.mounted = 2;
    player.state = 3;
    player.screen_x = 320;
    player.screen_y = 200;
    player.scale = 1.25f;
    assert(player_get_appearance(&player, &appearance));
    assert(appearance.job == 107 && appearance.look_job == 8);
    assert(appearance.sex == 1 && appearance.head == 34);
    assert(appearance.body_palette == 2 && appearance.head_palette == 5);
    assert(appearance.accessory[0] == 1001 && appearance.accessory[1] == 1002 && appearance.accessory[2] == 1003);
    assert(appearance.robe == 15 && appearance.body_style == 4 && appearance.mounted == 2);
    assert(job_queries == 1 && player_state(&player) == 3);
    assert(actor_get_screen_position((struct GameActor *)&player, &screen));
    assert(screen.x == 320 && screen.y == 200 && screen.scale == 1.25f);

    player.head = 99;
    player.accessory[1] = 2002;
    assert(appearance.head == 34 && appearance.accessory[1] == 1002);
    assert(player_get_appearance(&player, &appearance));
    assert(appearance.head == 99 && appearance.accessory[1] == 2002);
    puts("actor presentation: absent actors, appearance copies and screen position passed");
}

int main(void)
{
    query_job_offset = 100;
    query_appearance();
    query_job_offset = 0;
    sprite_look();
    model_look();
    entry_appearance();
    puts("Actor appearance: whole weapon values, ordered callbacks, shields, palettes, effect states and preparation failure passed");
    return 0;
}
