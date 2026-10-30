#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#if defined(SKILL_EFFECT_CALLS_REFERENCE) || defined(SKILL_ACTIONS_REFERENCE)
#include "skills/skill_action.h"
#define SKILL_CALLBACK(fn, ...) fn(NULL, __VA_ARGS__)
#else
#include "skills/skill_action_internal.h"
#define SKILL_CALLBACK(fn, ...) fn(__VA_ARGS__)
#endif

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "skill effect check failed at line %d: %s\n", __LINE__, #c);           \
            exit(2);                                                                               \
        }                                                                                          \
    } while (0)

static int skill_effect_test_rand(void)
{
    return 0;
}
#define rand skill_effect_test_rand
#ifdef SKILL_ACTIONS_REFERENCE
#elif defined(SKILL_EFFECT_CALLS_REFERENCE)
#else
#include "../src/skills/hyper_novice.c"
#include "../src/skills/night_watch.c"
#include "../src/skills/shinkiro_shiranui.c"
#include "../src/skills/sky_emperor.c"
#include "../src/skills/spirit_handler.c"
#endif
#undef rand
#include "skill_effect_calls_unreached.h"

struct Observation {
    struct Effect effect;
    char name[96];
    int aid, type, target, argument, key;
    unsigned char reuse;
    unsigned spins, stops, zooms, delays;
    float spin, stop, zoom;
    int delay;
};
static struct Observation events[256];
static unsigned event_count, spawn_attempts, wrong_calls, launches, live_blocks;
static int fail_spawn, has_view, last_launch_type, last_launch_value;
static SkillActor *last_launch_actor;
static uint32_t digest = UINT32_C(2166136261);
static FILE *trace_file;
struct Session g_session1;
struct EffectMgr *g_effectMgr;

static void trace(const void *data, size_t size)
{
    const unsigned char *p = data;
    if (trace_file)
        CHECK(fwrite(data, 1, size, trace_file) == size);
    while (size--)
        digest = (digest ^ *p++) * UINT32_C(16777619);
}

static struct Observation *observation(struct Effect *effect)
{
    unsigned i;
    for (i = 0; i < event_count; ++i)
        if (&events[i].effect == effect)
            return &events[i];
    CHECK(0);
    return NULL;
}

static void set_spin(struct Effect *effect, float value)
{
    struct Observation *e = observation(effect);
    CHECK(!e->spins++);
    e->spin = value;
}
static void set_stop(struct Effect *effect, float value)
{
    struct Observation *e = observation(effect);
    CHECK(!e->stops++);
    e->stop = value;
}
static void set_zoom(struct Effect *effect, float value)
{
    struct Observation *e = observation(effect);
    CHECK(!e->zooms++);
    e->zoom = value;
}
static void set_delay(struct Effect *effect, int value)
{
    struct Observation *e = observation(effect);
    CHECK(!e->delays++);
    e->delay = value;
}
static void wrong_name(struct Effect *effect, const char *value)
{
    ++wrong_calls;
}
static void wrong_alpha(struct Effect *effect, float value)
{
    ++wrong_calls;
}
static const struct EffectOps operations = {.set_stop_frame = set_stop,
                                              .set_spin = set_spin,
                                              .set_zoom = set_zoom,
                                              .set_start_delay = set_delay,
                                              .set_str_name = wrong_name,
                                              .set_alpha_scale = wrong_alpha};

static struct Effect *spawn(const char *name, int aid, int type, Vec3 pos, int target,
                             int argument, unsigned char reuse, int key)
{
    struct Observation *e;
    ++spawn_attempts;
    if (fail_spawn && spawn_attempts == (unsigned)fail_spawn)
        return NULL;
    CHECK(event_count < sizeof events / sizeof events[0]);
    e = &events[event_count++];
    e->effect.ops = &operations;
    e->effect.pos = pos;
    CHECK(strlen(name) < sizeof e->name);
    strcpy(e->name, name);
    e->aid = aid;
    e->type = type;
    e->target = target;
    e->argument = argument;
    e->reuse = reuse;
    e->key = key;
    return &e->effect;
}

struct Effect *skill_spawn_named(const char *name, int aid, Vec3 pos, int target, int argument,
                                  unsigned char reuse, int key)
{
    return spawn(name, aid, 0, pos, target, argument, reuse, key);
}

struct Effect *effect_mgr_spawn(struct EffectMgr *manager, int aid, int type, Vec3 pos,
                                  int target, int argument, unsigned char reuse, int key)
{
    CHECK(manager == g_effectMgr);
    return spawn("numeric", aid, type, pos, target, argument, reuse, key);
}

struct RagEffect *game_actor_launch_effect_timed(struct GameActor *actor, int type, int value)
{
    ++launches;
    last_launch_actor = (SkillActor *)actor;
    last_launch_type = type;
    last_launch_value = value;
    return NULL;
}

int game_copy_view_matrix(float matrix[12])
{
    unsigned i;
    for (i = 0; i < 12; ++i)
        matrix[i] = (float)i;
    return has_view;
}

float effect_screen_angle(Vec3 from, Vec3 to, const float *matrix)
{
    CHECK(matrix[0] == 0.0f && matrix[11] == 11.0f);
    CHECK(from.x == 10.0f && to.x == 40.0f);
    return -37.25f;
}
float effect_screen_angle_quadrant(Vec3 from, Vec3 to)
{
    CHECK(from.x == 10.0f && to.x == 40.0f);
    return 90.0f;
}

void *mem_alloc(size_t size)
{
    void *p = malloc(size);
    CHECK(p != NULL);
    ++live_blocks;
    return p;
}
void mem_free(void *p)
{
    if (p) {
        CHECK(live_blocks);
        --live_blocks;
        free(p);
    }
}
__declspec(noreturn) void fatal_error(const char *message)
{
    CHECK(0);
    exit(2);
}

struct Vec *skill_shuffled_indices(struct Vec *out, int count)
{
    int i, *values = mem_alloc((size_t)count * sizeof *values);
    for (i = 0; i < count; ++i)
        values[i] = count - 1 - i;
    out->first = values;
    out->last = out->end = values + count;
    return out;
}

int skill_format_effect_id(const char *format, int number)
{
    CHECK(number >= 0 && number < 3);
    if (!strcmp(format, "hitouakumu_%02d"))
        return 100 + number;
    if (!strcmp(format, "hasty_fire_in_the_hole_%d"))
        return 200 + number;
    CHECK(!strcmp(format, "hasty_fire_in_the_hole_bottom_%d"));
    return 300 + number;
}

struct Ez2StrEffectMgr *ez2str_effect_mgr_instance(void)
{
    return NULL;
}
int ez2_str_effect_mgr_get_effect_id(struct Ez2StrEffectMgr *mgr, const struct String *name)
{
    CHECK(!strcmp(string_cstr(name), "hells_drive_00"));
    return 400;
}

static void reset(void)
{
    unsigned i;
    CHECK(!live_blocks);
    for (i = 0; i < event_count; ++i) {
        struct Observation *e = &events[i];
        trace(e->name, strlen(e->name) + 1);
        trace(&e->effect.pos, sizeof e->effect.pos);
        trace(&e->spins, sizeof e->spins);
        trace(&e->stops, sizeof e->stops);
        trace(&e->zooms, sizeof e->zooms);
        trace(&e->delays, sizeof e->delays);
        trace(&e->spin, sizeof e->spin);
        trace(&e->stop, sizeof e->stop);
        trace(&e->zoom, sizeof e->zoom);
        trace(&e->delay, sizeof e->delay);
    }
    memset(events, 0, sizeof events);
    memset(&g_session1, 0, sizeof g_session1);
    event_count = spawn_attempts = wrong_calls = launches = 0;
    fail_spawn = 0;
    has_view = 1;
}

static void spin_calls(SkillActor *caster, SkillActor *target)
{
    unsigned light;
    for (light = 0; light < 2; ++light) {
        reset();
        g_session1.map_rule_flags[0] = (unsigned char)light;
        SKILL_CALLBACK(hn_spiral_pierce_max_use, 1, caster, target);
        CHECK(!wrong_calls && event_count == 3 - light);
        CHECK(events[event_count - 1].spins == 1);
        CHECK(events[event_count - 1].spin == -37.25f);
        CHECK(!strcmp(events[event_count - 1].name, "spiral_pierce_max"));
    }
    reset();
    SKILL_CALLBACK(nw_only_one_bullet_damage_hit, caster, target);
    CHECK(!wrong_calls && event_count == 1 && events[0].spins == 1);
    CHECK(events[0].spin == -37.25f && events[0].aid == target->aid);
    reset();
    spawn_turned_quadrant("quadrant", caster, target->pos);
    CHECK(!wrong_calls && event_count == 1 && events[0].spins == 1 && events[0].spin == 90.0f);
    reset();
    spawn_turned("exact", caster, target->pos);
    CHECK(!wrong_calls && event_count == 1 && events[0].spins == 1 && events[0].spin == -37.25f);
    reset();
    SKILL_CALLBACK(sh_hyun_rok_cannon_use, 1, caster, target);
    CHECK(!wrong_calls && event_count == 1 && events[0].spins == 1 && events[0].spin == -37.25f);
}

static void cast_calls(SkillActor *caster)
{
    static const unsigned times[] = {0, 15, 16, 65537, UINT32_MAX};
    unsigned i, light;
    for (i = 0; i < sizeof times / sizeof times[0]; ++i) {
        for (light = 0; light < 2; ++light) {
            reset();
            g_session1.map_rule_flags[0] = (unsigned char)light;
            SKILL_CALLBACK(hn_jack_frost_nova_cast, times[i], caster);
            CHECK(!wrong_calls && event_count == 1 && events[0].stops == 1);
            CHECK(events[0].stop == (float)(double)times[i]);
            CHECK(events[0].type == (light ? 0x607 : 0x5a6));
            CHECK(launches == 1 && last_launch_actor == caster && last_launch_type == 0x5a9);
            CHECK(last_launch_value == (int)(times[i] >> 4));
        }
        reset();
        SKILL_CALLBACK(ss_shinkirou_cast, times[i], caster);
        CHECK(!wrong_calls && event_count == 1 && events[0].stops == 1);
        CHECK(events[0].stop == (float)(double)times[i] && events[0].type == 0x6ee);
        CHECK(launches == 1 && last_launch_actor == caster && last_launch_type == 0x945);
        CHECK(last_launch_value == (int)(times[i] >> 4));
    }
}

static void repeated_calls(SkillActor *caster, SkillActor *target)
{
    unsigned i;
    reset();
    SKILL_CALLBACK(nw_vigilante_gatling_use, 1, caster, target);
    CHECK(!wrong_calls && event_count == 8);
    for (i = 1; i < 8; ++i) {
        CHECK(events[i].zooms == 1 && events[i].zoom == 0.5f);
        CHECK(events[i].delays == 1 && events[i].delay == (int)(i - 1) * 32);
    }
    reset();
    SKILL_CALLBACK(nw_vigilante_shotgun_use, 1, caster, target);
    CHECK(!wrong_calls && event_count == 5);
    for (i = 1; i < 5; ++i) {
        CHECK(events[i].zooms == 1 && events[i].zoom == (float)70 * 0.01f);
        CHECK(events[i].delays == 1 && events[i].delay == (int)(i - 1) * 32);
    }
    reset();
    SKILL_CALLBACK(ss_hitouakumu_use, 1, caster, target);
    CHECK(!wrong_calls && event_count == 46 && !live_blocks);
    for (i = 1; i < 46; ++i) {
        unsigned blade = (i - 1) % 15, ring = (i - 1) / 15;
        CHECK(events[i].type == (int)(100 + ring));
        CHECK(events[i].spins == 1 && events[i].spin == (float)blade * 24.0f);
        CHECK(events[i].delays == 1 && events[i].delay == (int)(14 - blade + ring * 10) * 32);
    }
    reset();
    SKILL_CALLBACK(hn_hells_drive_use, 1, caster, target);
    CHECK(event_count == 6 && !live_blocks);
    for (i = 0; i < 6; ++i)
        CHECK(events[i].delays == 1 && events[i].delay == (int)(i / 2) * 32);
    reset();
    SKILL_CALLBACK(nw_hasty_fire_in_the_hole_use_on_ground, target->pos, 1, caster);
    CHECK(event_count == 6);
    for (i = 0; i < 6; ++i)
        CHECK(events[i].delays == 1 && events[i].delay == (int)(i / 2) * 0x120);
    reset();
    SKILL_CALLBACK(ss_kunaikaiten_use, 1, caster, target);
    CHECK(event_count == 16);
    for (i = 0; i < 16; ++i)
        CHECK(events[i].delays == 1 && events[i].delay == (int)(i / 4) * 0x1e0);
    reset();
    SKILL_CALLBACK(ss_reiketsuhou_use_on_ground, target->pos, 1, caster);
    CHECK(event_count == 6);
    for (i = 1; i < 6; ++i)
        CHECK(events[i].delays == 1 && events[i].delay == (int)(i - 1) * 0x50);
    reset();
    SKILL_CALLBACK(ske_sunset_blast_use, 1, caster, target);
    CHECK(event_count == 7);
    for (i = 1; i < 7; ++i)
        CHECK(events[i].delays == 1 && events[i].delay == (int)(i - 1) * 32);
}

static void guards(SkillActor *caster, SkillActor *target)
{
    reset();
    SKILL_CALLBACK(hn_spiral_pierce_max_use, 1, NULL, target);
    SKILL_CALLBACK(hn_spiral_pierce_max_use, 1, caster, NULL);
    SKILL_CALLBACK(hn_jack_frost_nova_cast, 10, NULL);
    SKILL_CALLBACK(ss_shinkirou_cast, 10, NULL);
    SKILL_CALLBACK(nw_only_one_bullet_damage_hit, NULL, target);
    SKILL_CALLBACK(nw_only_one_bullet_damage_hit, caster, NULL);
    SKILL_CALLBACK(sh_hyun_rok_cannon_use, 1, NULL, target);
    SKILL_CALLBACK(sh_hyun_rok_cannon_use, 1, caster, NULL);
    SKILL_CALLBACK(ss_hitouakumu_use, 1, NULL, target);
    CHECK(!spawn_attempts && !launches);
    reset();
    has_view = 0;
    SKILL_CALLBACK(nw_only_one_bullet_damage_hit, caster, target);
    SKILL_CALLBACK(sh_hyun_rok_cannon_use, 1, caster, target);
    spawn_turned("missing_view", caster, target->pos);
    CHECK(!spawn_attempts);
    SKILL_CALLBACK(hn_spiral_pierce_max_use, 1, caster, target);
    CHECK(event_count == 2 && !events[0].spins && !events[1].spins);
    reset();
    fail_spawn = 1;
    SKILL_CALLBACK(hn_jack_frost_nova_cast, 10, caster);
    CHECK(!event_count && launches == 1 && !wrong_calls);
    reset();
    fail_spawn = 1;
    SKILL_CALLBACK(ss_shinkirou_cast, 10, caster);
    CHECK(!event_count && launches == 1 && !wrong_calls);
    reset();
    fail_spawn = 1;
    SKILL_CALLBACK(nw_only_one_bullet_damage_hit, caster, target);
    CHECK(!event_count && !wrong_calls);
    reset();
    fail_spawn = 1;
    SKILL_CALLBACK(sh_hyun_rok_cannon_use, 1, caster, target);
    CHECK(!event_count && !wrong_calls);
    reset();
    fail_spawn = 3;
    SKILL_CALLBACK(nw_vigilante_gatling_use, 1, caster, target);
    CHECK(event_count == 2 && spawn_attempts == 3 && events[1].zooms == 1);
    reset();
    fail_spawn = 2;
    SKILL_CALLBACK(ss_hitouakumu_use, 1, caster, target);
    CHECK(event_count == 45 && spawn_attempts == 46 && !live_blocks && !wrong_calls);
}

static void status_reapply(SkillActor *actor)
{
    reset();
    SKILL_CALLBACK(EfstRisingSun_On, 0x569, 9, actor);
    CHECK(event_count == 2);
    CHECK(!strcmp(events[0].name, "ske_rising_sun_00") &&
          !strcmp(events[1].name, "ske_rising_sun_01"));
    reset();
    SKILL_CALLBACK(EfstRisingSun_Reapply, 0x569, 9, actor);
    CHECK(event_count == 1 && !strcmp(events[0].name, "ske_rising_sun_01"));
    reset();
#if defined(SKILL_EFFECT_CALLS_REFERENCE) || defined(SKILL_ACTIONS_REFERENCE)
    {
        struct CEfstAction *action = new_CEfstAction_RULEBREAK();
        action->ops->use_action(action, 0x568, 9, actor);
        mem_free(action);
    }
#else
    CHECK(efst_action_rulebreak_ops.on == efst_action_rulebreak_ops.reapply);
    efst_action_rulebreak_ops.reapply(0x568, 9, actor);
#endif
    CHECK(event_count == 1 && !strcmp(events[0].name, "rulebreak_buff") && !live_blocks);
}

int main(int argc, char **argv)
{
    const char *trace_path = getenv("RAGEXE_SKILL_ACTION_TRACE");
    SkillActor caster = {0}, target = {0};
    if (trace_path) {
        trace_file = fopen(trace_path, "wb");
        CHECK(trace_file != NULL);
    }
    caster.aid = 42;
    caster.pos.x = 10;
    caster.pos.y = 20;
    caster.pos.z = 30;
    target.aid = -42;
    target.pos.x = 40;
    target.pos.y = 50;
    target.pos.z = 60;
    if (argc == 1 || !strcmp(argv[1], "spin"))
        spin_calls(&caster, &target);
    if (argc == 1 || !strcmp(argv[1], "cast"))
        cast_calls(&caster);
    if (argc == 1 || !strcmp(argv[1], "repeated"))
        repeated_calls(&caster, &target);
    if (argc == 1) {
        guards(&caster, &target);
        status_reapply(&caster);
    }
    reset();
    printf("Skill effect calls: rotation, cast duration, zoom, delays and guarded paths passed; "
           "digest %08x.\n",
           digest);
    if (trace_file)
        CHECK(fclose(trace_file) == 0);
    return 0;
}
