#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "fileio/res_mgr.h"
#include "platform/thread.h"
#include <process.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int allocation_failure, allocations, registration_failure, registrations;
static int event_failure, thread_failure;
static void (*cleanup)(void);
static void *guard_alloc(size_t size)
{
    unsigned char *raw;
    if (allocation_failure)
        return NULL;
    raw = malloc(size + 24);
    assert(raw);
    *(size_t *)raw = size;
    memset(raw + 8, 0x93, 8);
    memset(raw + 16 + size, 0x57, 8);
    ++allocations;
    return raw + 16;
}
static void guard_free(void *pointer)
{
    unsigned char *raw;
    size_t size, i;
    if (!pointer)
        return;
    raw = (unsigned char *)pointer - 16;
    size = *(size_t *)raw;
    for (i = 0; i < 8; ++i) {
        assert(raw[8 + i] == 0x93);
        assert(raw[16 + size + i] == 0x57);
    }
    memset(raw, 0xdd, size + 24);
    free(raw);
    --allocations;
}
static void *guard_calloc(size_t count, size_t size)
{
    void *p = guard_alloc(count * size);
    if (p)
        memset(p, 0, count * size);
    return p;
}
static int register_cleanup(void (*function)(void))
{
    if (registration_failure)
        return -1;
    cleanup = function;
    ++registrations;
    return 0;
}
static struct PlatformEvent *create_event(void)
{
    return event_failure ? NULL : platform_event_create();
}
static struct PlatformThread *begin_thread(void (*entry)(void *context), void *context)
{
    return thread_failure ? NULL : platform_thread_create(entry, context);
}

#define malloc guard_alloc
#define calloc guard_calloc
#define free guard_free
#define atexit register_cleanup
#define platform_event_create create_event
#define platform_thread_create begin_thread
#include "../src/contents/sound.c"
#undef malloc
#undef calloc
#undef free
#undef atexit
#undef platform_event_create
#undef platform_thread_create

/* The audio module, as sound.c sees it: an opened device, voices and one music stream. */
#define VOICES 64
uint32_t g_soundMode;
static int device_open, device_mono, fail_open, fail_music, music_open, music_done_answer;
static int open_count, close_count, music_attempts, voice_starts, voice_limit, wave_missing, undecodable;
static int music_loops;
static float last_gain, voice_gain[VOICES];
static int voice_live[VOICES], voice_playing[VOICES];
static struct AudioSound *decoded = (struct AudioSound *)&decoded;
static char last_music[128], last_wave[128];
static struct Wave wave_resource;
static struct ResMgr resource_manager;
static HANDLE gain_entered, gain_release;
static volatile LONG block_gain, music_gain;

struct ResMgr *g_resMgr(void) { return &resource_manager; }
void *res_mgr_get(struct ResMgr *mgr, const char *name)
{
    assert(mgr == &resource_manager);
    strcpy_s(last_wave, sizeof last_wave, name);
    wave_resource.sound = undecodable ? NULL : decoded;
    return wave_missing ? NULL : &wave_resource;
}

int audio_open(int mono)
{
    assert(!device_open);
    ++open_count;
    if (fail_open)
        return 0;
    device_open = 1;
    device_mono = mono;
    return 1;
}
void audio_close(void)
{
    int i;
    assert(device_open && !music_open);
    for (i = 0; i < VOICES; ++i)
        assert(!voice_live[i]);
    device_open = 0;
    ++close_count;
}
struct AudioVoice *audio_voice_play(const struct AudioSound *sound, float gain)
{
    int i;
    assert(device_open && sound == decoded && gain >= 0.0f && gain <= 1.0f);
    for (i = 0; i < voice_limit; ++i)
        if (!voice_live[i]) {
            voice_live[i] = voice_playing[i] = 1;
            voice_gain[i] = last_gain = gain;
            ++voice_starts;
            return (struct AudioVoice *)&voice_live[i];
        }
    return NULL;
}
static int voice_index(struct AudioVoice *voice)
{
    int i = (int)((int *)voice - voice_live);
    assert(i >= 0 && i < VOICES && voice_live[i]);
    return i;
}
int audio_voice_playing(struct AudioVoice *voice)
{
    return voice && voice_playing[voice_index(voice)];
}
void audio_voice_stop(struct AudioVoice *voice)
{
    if (voice)
        voice_live[voice_index(voice)] = voice_playing[voice_index(voice)] = 0;
}
struct AudioMusic *audio_music_play(const char *path, int loop_count, float gain)
{
    assert(device_open && !music_open && path);
    ++music_attempts;
    strcpy_s(last_music, sizeof last_music, path);
    if (fail_music)
        return NULL;
    music_open = 1;
    music_loops = loop_count;
    InterlockedExchange(&music_gain, (LONG)lrintf(gain * SOUND_VOLUME_MAX));
    return (struct AudioMusic *)&music_open;
}
void audio_music_set_gain(struct AudioMusic *music, float gain)
{
    if (!music)
        return;
    assert(music == (struct AudioMusic *)&music_open && music_open && gain >= 0.0f && gain <= 1.0f);
    InterlockedExchange(&music_gain, (LONG)lrintf(gain * SOUND_VOLUME_MAX));
    if (InterlockedCompareExchange(&block_gain, 0, 1) == 1) {
        SetEvent(gain_entered);
        assert(WaitForSingleObject(gain_release, 5000) == WAIT_OBJECT_0);
        assert(music_open && device_open);
    }
}
int audio_music_done(struct AudioMusic *music)
{
    if (!music)
        return 0;
    assert(music == (struct AudioMusic *)&music_open && music_open);
    return music_done_answer;
}
void audio_music_stop(struct AudioMusic *music)
{
    if (!music)
        return;
    assert(music == (struct AudioMusic *)&music_open && music_open);
    music_open = 0;
}

static void reset(int mode)
{
    sound_mgr_destroy();
    assert(!allocations && !device_open && !music_open);
    allocation_failure = event_failure = thread_failure = registration_failure = 0;
    fail_open = fail_music = wave_missing = undecodable = music_done_answer = 0;
    open_count = close_count = music_attempts = voice_starts = 0;
    voice_limit = VOICES;
    g_soundMode = mode;
}

static void test_failures(void)
{
    reset(1); allocation_failure = 1; assert(!sound_mgr_create()); assert(!allocations && !open_count);
    reset(1); event_failure = 1; assert(!sound_mgr_create()); assert(!allocations && !open_count);
    reset(1); fail_open = 1; assert(!sound_mgr_create()); assert(!allocations && open_count == 1 && !close_count);
    reset(2); registration_failure = 1; assert(!sound_mgr_create()); assert(!allocations && close_count == 1);
    reset(0); assert(sound_mgr_create()); assert(!open_count && registrations == 1);
    sound_play_bgm("track"); sound_play_wave("one.wav", 1.0f); assert(!music_attempts && !voice_starts);
    sound_mgr_destroy(); sound_mgr_destroy(); assert(!allocations && !close_count);
    sound_bgm_fade_in(); sound_bgm_fade_out(); sound_stop_stream(); sound_end_samples();
    assert(!sound_stream_done() && !sound_bgm_volume());
}

static void test_modes(void)
{
    reset(1); assert(sound_mgr_create()); assert(device_open && device_mono); sound_mgr_destroy();
    reset(2); assert(sound_mgr_create()); assert(device_open && !device_mono); sound_mgr_destroy();
    /* Any other mode opens the device and plays the music, but no effects, as before. */
    reset(3); assert(sound_mgr_create()); assert(device_open && !device_mono);
    sound_play_wave("one.wav", 1.0f); assert(!voice_starts && !strcmp(last_wave, "one.wav"));
    sound_play_stream("bgm/one.mp3", 0); assert(music_open);
    sound_mgr_destroy(); assert(close_count == 1 && !allocations);
}

static void test_streams(void)
{
    char name[] = "bgm/borrowed.mp3";
    int attempts;
    reset(1); assert(sound_mgr_create()); assert(!sound_mgr_create());
    fail_music = 1; sound_play_stream("bgm/retry.mp3", 1);
    assert(!sound_stream_done() && music_attempts == 1 && !music_open);
    fail_music = 0; sound_play_stream("bgm/retry.mp3", 1);
    assert(music_open && music_attempts == 2 && music_loops == 1 && music_gain == 100);
    assert(!sound_stream_done()); music_done_answer = 1; assert(sound_stream_done()); music_done_answer = 0;
    sound_play_stream("bgm/retry.mp3", 0); assert(music_attempts == 2);
    allocation_failure = 1; sound_play_stream("bgm/other.mp3", 0);
    assert(music_attempts == 2 && music_open); allocation_failure = 0;
    g_sound_settings.bgm_on = 0; sound_play_bgm(name); name[4] = 'X';
    assert(music_attempts == 2 && !strcmp(last_music, "bgm/retry.mp3"));
    sound_stop_stream(); assert(!sound_stream_done() && !music_open);
    sound_play_stream(NULL, 0); assert(!strcmp(last_music, "bgm/borrowed.mp3") && music_loops == 0);
    sound_set_bgm_volume(999); assert(sound_bgm_volume() == 127 && music_gain == 127);
    sound_set_bgm_volume(-1); assert(sound_bgm_volume() == 0 && music_gain == 0);
    sound_set_bgm_volume(64); assert(sound_bgm_volume() == 64 && music_gain == 64);
    sound_set_bgm_fade_target(999); assert(sound_bgm_fade_target() == 127);
    sound_set_effect_volume(-1); assert(sound_effect_volume() == 0);
    sound_set_effect_volume(999); assert(sound_effect_volume() == 127);
    attempts = music_attempts;
    allocation_failure = 1;
    sound_play_bgm(NULL);
    assert(!sound_stream_done() && !music_open && !sound_manager->bgm_name && !sound_manager->stream_name);
    sound_play_stream(NULL, 0);
    assert(music_attempts == attempts);
    allocation_failure = 0; g_sound_settings.bgm_on = 1;
    sound_play_bgm("bgm/restored.mp3");
    assert(music_open && music_attempts == attempts + 1 && music_loops == 0 && music_gain == 64);
    sound_play_bgm("");
    assert(!music_open && !sound_manager->bgm_name && !sound_manager->stream_name);
    sound_play_bgm("bgm/last.mp3"); assert(music_open);
    sound_mgr_destroy(); assert(close_count == 1 && !music_open && !allocations);
    cleanup(); assert(close_count == 1 && registrations == 1);
}

static void test_effects(void)
{
    int i;
    reset(2); voice_limit = 3; assert(sound_mgr_create());
    /* No position: every effect plays at the effect volume times its own volume. */
    sound_play_wave("one.wav", 0.5f);
    assert(voice_starts == 1 && fabsf(last_gain - 50.0f / 127) < 1e-6f);
    sound_set_effect_volume(127); sound_play_wave("one.wav", 1.0f);
    assert(voice_starts == 2 && last_gain == 1.0f);
    sound_set_effect_volume(100); sound_play_wave("one.wav", 1e30f);
    assert(voice_starts == 3 && last_gain == 1.0f);
    voice_playing[0] = 0; sound_play_wave("one.wav", -2.0f);
    assert(voice_starts == 4 && last_gain == 0.0f && voice_live[0]);
    voice_playing[0] = 0; sound_play_wave("one.wav", NAN);
    assert(voice_starts == 5 && last_gain == 0.0f);
    /* every voice busy: the effect is dropped, as when all of the 48 samples played */
    sound_play_wave("one.wav", 1.0f); assert(voice_starts == 5);
    g_sound_settings.effects_on = 0;
    voice_playing[1] = 0; sound_play_wave("one.wav", 1.0f); assert(voice_starts == 5);
    g_sound_settings.effects_on = 1;
    sound_play_wave("버튼소리.wav", 1.0f); assert(voice_starts == 6 && !strcmp(last_wave, "버튼소리.wav"));
    wave_missing = 1; voice_playing[1] = 0; sound_play_wave("two.wav", 1.0f); assert(voice_starts == 6);
    wave_missing = 0; undecodable = 1; sound_play_wave("two.wav", 1.0f); assert(voice_starts == 6);
    undecodable = 0; sound_play_wave(NULL, 1.0f); assert(voice_starts == 6);
    sound_end_samples();
    for (i = 0; i < VOICES; ++i) assert(!voice_live[i]);
    sound_mgr_destroy(); assert(!allocations && close_count == 1);

    reset(1); assert(sound_mgr_create());
    for (i = 0; i < 60; ++i)
        sound_play_wave("one.wav", 1.0f);
    assert(voice_starts == 48);
    for (i = 0; i < VOICES; ++i)
        voice_playing[i] = 0;
    for (i = 0; i < 60; ++i)
        sound_play_wave("one.wav", 1.0f);
    assert(voice_starts == 96);
    sound_mgr_destroy(); assert(!allocations && close_count == 1);
}

static void wait_volume(int target)
{
    int i;
    for (i = 0; i < 300 && sound_bgm_volume() != target; ++i)
        Sleep(10);
    assert(sound_bgm_volume() == target);
}

static unsigned __stdcall release_backend(void *unused)
{
    (void)unused;
    Sleep(80);
    assert(music_open && device_open && !close_count);
    SetEvent(gain_release);
    return 0;
}

static void test_fades(void)
{
    HANDLE helper;
    int i;
    reset(1); assert(sound_mgr_create()); sound_play_bgm("track");
    sound_set_bgm_volume(0); sound_set_bgm_fade_target(3); sound_bgm_fade_in(); wait_volume(3);
    assert(music_gain == 3);
    sound_bgm_fade_out(); wait_volume(0); assert(music_gain == 0);
    thread_failure = 1; sound_bgm_fade_in(); assert(sound_bgm_volume() == 0); thread_failure = 0;
    for (i = 0; i < 20; ++i) { sound_bgm_fade_in(); sound_bgm_fade_out(); }
    sound_bgm_fade_out(); wait_volume(0);
    gain_entered = CreateEventW(NULL, TRUE, FALSE, NULL);
    gain_release = CreateEventW(NULL, TRUE, FALSE, NULL);
    assert(gain_entered && gain_release);
    InterlockedExchange(&block_gain, 1); sound_bgm_fade_in();
    assert(WaitForSingleObject(gain_entered, 2000) == WAIT_OBJECT_0);
    helper = (HANDLE)_beginthreadex(NULL, 0, release_backend, NULL, 0, NULL); assert(helper);
    sound_mgr_destroy();
    assert(WaitForSingleObject(helper, 2000) == WAIT_OBJECT_0); CloseHandle(helper);
    CloseHandle(gain_entered); CloseHandle(gain_release);
    assert(!allocations && close_count == 1);
}

int main(void)
{
    int cycle;
    test_failures(); test_modes(); test_streams(); test_effects(); test_fades();
    for (cycle = 0; cycle < 20; ++cycle) {
        reset(1); assert(sound_mgr_create()); sound_play_bgm("cycle"); sound_play_wave("one.wav", 1.0f);
        sound_bgm_fade_out();
        sound_mgr_destroy(); sound_mgr_destroy(); assert(!allocations && close_count == 1);
    }
    assert(registrations == 1);
    puts("Sound manager: modes, effects without position at the effect volume, the 48-voice cap, music retry, "
         "volumes, rollback, real fades and joined shutdown passed");
    return 0;
}
