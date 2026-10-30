#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "session/state.h"
#include "session/job_sounds.h"
#include "job_sound_reference.h"

struct Session g_session1;
static struct Session session;
static const char *expected[JOB_HIT_SOUND_COUNT];
static int weapon_calls;
static int real_weapon = -1;

int session_get_real_weapon_id(int weapon)
{
    assert(weapon == -1);
    weapon_calls++;
    return real_weapon;
}

int session_get_drawn_job(struct Session *self)
{
    (void)self;
    assert(!"Empty weapon catalog must return before querying the drawn job");
    return 0;
}

static void expect_fallback(int job)
{
    const char *sound;
    int calls = weapon_calls;
    sound = session_get_hit_wave_name(&session, job);
    assert(weapon_calls == calls + 1);
    assert(sound != NULL);
    assert(!strcmp(sound, "_enemy_hit_normal1.wav") || !strcmp(sound, "_enemy_hit_normal2.wav")
        || !strcmp(sound, "_enemy_hit_normal3.wav") || !strcmp(sound, "_enemy_hit_normal4.wav"));
}

int main(void)
{
    struct JobHitSounds sounds = {0}, first;
    unsigned i;
    int job;

    assert(sizeof(struct JobHitSounds) == 12);
    for (i = 0; i < JOB_HIT_SOUND_COUNT; i++) {
        expected[i] = "player_clothes.wav";
        assert(job_hit_sound(&sounds, i) == NULL);
    }
    for (i = 0; i < sizeof(legacy_job_sounds) / sizeof(legacy_job_sounds[0]); i++)
        expected[legacy_job_sounds[i].index] = legacy_job_sounds[i].name;
    job_hit_sounds_init(&sounds);
    first = sounds;
    for (i = 0; i < JOB_HIT_SOUND_COUNT; i++) {
        const char *sound = job_hit_sound(&sounds, i);
        assert(sound == sounds.clothes || sound == sounds.wooden || sound == sounds.metal);
        assert(!memcmp(sound, expected[i], strlen(expected[i]) + 1));
    }
    job_hit_sounds_init(&sounds);
    assert(sounds.clothes == first.clothes && sounds.wooden == first.wooden && sounds.metal == first.metal);
    assert(job_hit_sound(&sounds, JOB_HIT_SOUND_COUNT) == NULL);
    assert(job_hit_sound(&sounds, UINT_MAX) == NULL);
    assert(session_get_hit_wave_name(&session, 0) == NULL);

    session_init_job_hit_wave_name(&session);
    for (job = 0; job <= 32000; job++) {
        if (job <= 30 || job == 1000 || (job >= 3999 && job <= 20020) || job == 32000) {
            unsigned index = job >= 3999 ? (unsigned)(job - 3950) : (unsigned)job;
            int calls = weapon_calls;
            assert(!strcmp(session_get_hit_wave_name(&session, job), expected[index]));
            assert(weapon_calls == calls);
        } else {
            expect_fallback(job);
        }
    }
    expect_fallback(-1);
    expect_fallback(INT_MIN);
    assert(session_get_hit_wave_name(&session, 32001) == NULL);
    assert(session_get_hit_wave_name(&session, INT_MAX) == NULL);
    real_weapon = 5;
    assert(session_get_hit_wave_name(&session, 31) == NULL);
    assert(session_get_weapon_hit_wave(&session, -1) == NULL);
    session_init_job_hit_wave_name(&session);
    assert(!strcmp(session_get_hit_wave_name(&session, 1), "player_metal.wav"));
    puts("Job hit sounds: all 28051 legacy entries, 32001 job routes, borrowed storage, repeated initialization, negative/large inputs and weapon fallback passed");
    return 0;
}
