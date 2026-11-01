#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "fileio/res_mgr.h"
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
static HANDLE WINAPI create_event(LPSECURITY_ATTRIBUTES attributes, BOOL manual, BOOL state, LPCWSTR name)
{
    return event_failure ? NULL : CreateEventW(attributes, manual, state, name);
}
static uintptr_t begin_thread(void *security, unsigned size, unsigned (__stdcall *entry)(void *),
                              void *argument, unsigned flags, unsigned *id)
{
    return thread_failure ? 0 : _beginthreadex(security, size, entry, argument, flags, id);
}

#define MSS_IMPORT
#define malloc guard_alloc
#define calloc guard_calloc
#define free guard_free
#define atexit register_cleanup
#define CreateEventW create_event
#define _beginthreadex begin_thread
#include "../src/contents/sound.c"
#undef malloc
#undef calloc
#undef free
#undef atexit
#undef CreateEventW
#undef _beginthreadex

uint32_t g_soundMode;
static int started, driver_open, provider_open, listener_open, stream_open;
static int fail_driver, fail_provider, fail_listener, fail_stream, no_provider;
static int sample_limit, sample_allocated, sample_3d_allocated, enumerations, provider_index;
static int stream_attempts, stream_starts, sample_starts, sample_3d_starts;
static int volume_2d, volume_3d, loop_count, expected_rate, expected_bits, expected_channels;
static int release_count, shutdown_count, room_type, wave_missing, reject_3d;
static int sample_storage[48], sample_3d_storage[48], driver_token, provider_token, listener_token, stream_token;
static float last_position[3], last_orientation[3], distances[2];
static char last_stream[128], last_wave[128];
static struct Wave wave_resource;
static struct ResMgr resource_manager;
static HANDLE volume_entered, volume_release;
static volatile LONG block_volume, stream_volume;

struct ResMgr *g_resMgr(void) { return &resource_manager; }
void *res_mgr_get(struct ResMgr *mgr, const char *name)
{
    assert(mgr == &resource_manager);
    strcpy_s(last_wave, sizeof last_wave, name);
    return wave_missing ? NULL : &wave_resource;
}
float float_sqrt(float x) { return sqrtf(x); }
float get_sin_deg(int x) { return sinf((float)x * 0.017453292519943295f); }
float get_cos_deg(int x) { return cosf((float)x * 0.017453292519943295f); }

void __stdcall _AIL_set_redist_directory(const char *dir) { assert(strcmp(dir, ".\\") == 0); }
int __stdcall _AIL_startup(void) { assert(!started); started = 1; return 1; }
void __stdcall _AIL_shutdown(void)
{
    assert(started && !driver_open && !stream_open && !provider_open && !listener_open);
    assert(!sample_allocated && !sample_3d_allocated);
    started = 0; ++shutdown_count;
}
int __stdcall _AIL_set_preference(unsigned n, int value) { assert(n == 15 && value == 0); return 0; }
void *__stdcall _AIL_open_digital_driver(unsigned rate, int bits, int channels, unsigned flags)
{
    assert(started && rate == (unsigned)expected_rate && bits == expected_bits);
    assert(channels == expected_channels && flags == 1);
    driver_open = !fail_driver;
    return driver_open ? &driver_token : NULL;
}
void __stdcall _AIL_close_digital_driver(void *driver)
{
    assert(driver == &driver_token && driver_open && !stream_open);
    assert(!sample_allocated && !sample_3d_allocated && !provider_open);
    driver_open = 0;
}
void *__stdcall _AIL_allocate_sample_handle(void *driver)
{
    assert(driver == &driver_token && driver_open);
    if (sample_allocated == sample_limit)
        return NULL;
    return &sample_storage[sample_allocated++];
}
void __stdcall _AIL_release_sample_handle(void *sample)
{
    assert(sample >= (void *)sample_storage && sample < (void *)(sample_storage + 48));
    assert(sample_allocated > 0 && driver_open); --sample_allocated; ++release_count;
}
void __stdcall _AIL_init_sample(void *sample) { assert(sample && driver_open); }
int __stdcall _AIL_set_sample_file(void *sample, const void *file, int block)
{ assert(sample && file == wave_resource.data && block == 0); return 1; }
void __stdcall _AIL_set_sample_volume(void *sample, int volume) { assert(sample); volume_2d = volume; }
void __stdcall _AIL_start_sample(void *sample) { assert(sample); ++sample_starts; }
void __stdcall _AIL_end_sample(void *sample) { assert(sample && driver_open); }
unsigned __stdcall _AIL_sample_status(void *sample) { assert(sample && driver_open); return 2; }
void *__stdcall _AIL_open_stream(void *driver, const char *name, int memory)
{
    assert(driver == &driver_token && driver_open && !stream_open && memory == 0);
    ++stream_attempts; strcpy_s(last_stream, sizeof last_stream, name);
    stream_open = !fail_stream;
    return stream_open ? &stream_token : NULL;
}
void __stdcall _AIL_close_stream(void *stream)
{ assert(stream == &stream_token && stream_open); stream_open = 0; }
void __stdcall _AIL_start_stream(void *stream)
{ assert(stream == &stream_token && stream_open); ++stream_starts; }
void __stdcall _AIL_set_stream_volume(void *stream, int volume)
{
    assert(stream == &stream_token && stream_open && volume >= 0 && volume <= 127);
    InterlockedExchange(&stream_volume, volume);
    if (InterlockedCompareExchange(&block_volume, 0, 1) == 1) {
        SetEvent(volume_entered);
        assert(WaitForSingleObject(volume_release, 5000) == WAIT_OBJECT_0);
        assert(stream_open && driver_open && started);
    }
}
void __stdcall _AIL_set_stream_loop_count(void *stream, int count)
{ assert(stream == &stream_token && stream_open); loop_count = count; }
int __stdcall _AIL_stream_status(void *stream)
{ assert(stream == &stream_token && stream_open); return 4; }
int __stdcall _AIL_enumerate_3D_providers(void **next, void **dest, char **name)
{
    int index = (int)(INT_PTR)*next;
    assert(started);
    ++enumerations;
    if (index > provider_index)
        return 0;
    *next = (void *)(INT_PTR)(index + 1);
    *dest = &provider_token;
    *name = !no_provider && index == provider_index ? "Miles Fast 2D Positional Audio" : "Other provider";
    return 1;
}
int __stdcall _AIL_open_3D_provider(void *provider)
{ assert(provider == &provider_token && !provider_open); provider_open = !fail_provider; return fail_provider; }
void __stdcall _AIL_close_3D_provider(void *provider)
{ assert(provider == &provider_token && provider_open && !listener_open && !sample_3d_allocated); provider_open = 0; }
int __stdcall _AIL_3D_speaker_type(void *provider) { assert(provider && provider_open); return 0; }
void __stdcall _AIL_set_3D_speaker_type(void *provider, int type) { assert(provider && provider_open && type == 0); }
int __stdcall _AIL_3D_room_type(void *provider) { assert(provider && provider_open); return room_type; }
void __stdcall _AIL_set_3D_room_type(void *provider, int type) { assert(provider && provider_open); room_type = type; }
void *__stdcall _AIL_open_3D_listener(void *provider)
{ assert(provider && provider_open); listener_open = !fail_listener; return listener_open ? &listener_token : NULL; }
void __stdcall _AIL_close_3D_listener(void *listener)
{ assert(listener == &listener_token && listener_open && !sample_3d_allocated); listener_open = 0; }
void *__stdcall _AIL_allocate_3D_sample_handle(void *provider)
{
    assert(provider && provider_open);
    if (sample_3d_allocated == sample_limit)
        return NULL;
    return &sample_3d_storage[sample_3d_allocated++];
}
void __stdcall _AIL_release_3D_sample_handle(void *sample)
{ assert(sample && sample_3d_allocated > 0 && provider_open); --sample_3d_allocated; ++release_count; }
int __stdcall _AIL_set_3D_sample_file(void *sample, const void *file)
{ assert(sample && provider_open && file == wave_resource.data); return !reject_3d; }
void __stdcall _AIL_set_3D_sample_effects_level(void *sample, float level) { assert(sample && level == 0); }
void __stdcall _AIL_set_3D_position(void *object, float x, float y, float z)
{ assert(object); last_position[0] = x; last_position[1] = y; last_position[2] = z; }
void __stdcall _AIL_set_3D_orientation(void *object, float x, float y, float z, float ux, float uy, float uz)
{ assert(object && ux == 0 && uy == 1 && uz == 0); last_orientation[0] = x; last_orientation[1] = y; last_orientation[2] = z; }
void __stdcall _AIL_set_3D_sample_volume(void *sample, int volume) { assert(sample); volume_3d = volume; }
void __stdcall _AIL_set_3D_sample_distances(void *sample, float max, float min)
{ assert(sample); distances[0] = max; distances[1] = min; }
void __stdcall _AIL_start_3D_sample(void *sample) { assert(sample && provider_open); ++sample_3d_starts; }
void __stdcall _AIL_end_3D_sample(void *sample) { assert(sample && provider_open); }
unsigned __stdcall _AIL_3D_sample_status(void *sample) { assert(sample && provider_open); return 2; }

static void reset(int mode)
{
    sound_mgr_destroy();
    assert(!allocations && !started && !stream_open && !driver_open && !provider_open);
    allocation_failure = event_failure = thread_failure = registration_failure = 0;
    fail_driver = fail_provider = fail_listener = fail_stream = no_provider = 0;
    stream_attempts = stream_starts = sample_starts = sample_3d_starts = 0;
    enumerations = release_count = shutdown_count = room_type = wave_missing = reject_3d = 0;
    provider_index = 0; sample_limit = 48;
    g_soundMode = mode; g_sound_sample_type = g_sound_rate_type = g_sound_bits_type = 0;
    expected_rate = 22050; expected_bits = 16; expected_channels = mode == 1 ? 1 : 2;
    wave_resource.data = &wave_resource;
}

static void test_failures(void)
{
    reset(1); allocation_failure = 1; assert(!sound_mgr_create()); assert(!allocations);
    reset(1); event_failure = 1; assert(!sound_mgr_create()); assert(!allocations && !shutdown_count);
    reset(1); fail_driver = 1; assert(!sound_mgr_create()); assert(!allocations && shutdown_count == 1);
    reset(2); fail_provider = 1; assert(!sound_mgr_create()); assert(!allocations && shutdown_count == 1);
    reset(2); fail_listener = 1; assert(!sound_mgr_create()); assert(!allocations && shutdown_count == 1);
    reset(1); registration_failure = 1; assert(!sound_mgr_create()); assert(!allocations && shutdown_count == 1);
    reset(0); assert(sound_mgr_create()); assert(!started && registrations == 1);
    sound_mgr_destroy(); sound_mgr_destroy(); assert(!allocations);
    sound_bgm_fade_in(); sound_bgm_fade_out(); sound_stop_stream(); sound_end_samples();
    assert(!sound_stream_playing() && !sound_bgm_volume());
}

static void test_streams(void)
{
    char name[] = "bgm/borrowed.mp3";
    reset(1); assert(sound_mgr_create()); assert(!sound_mgr_create());
    fail_stream = 1; sound_play_stream("bgm/retry.mp3", 1);
    assert(!sound_stream_playing() && !stream_starts && stream_attempts == 1);
    fail_stream = 0; sound_play_stream("bgm/retry.mp3", 1);
    assert(sound_stream_playing() && stream_starts == 1 && stream_attempts == 2 && loop_count == 1);
    sound_play_stream("bgm/retry.mp3", 0); assert(stream_attempts == 2);
    allocation_failure = 1; sound_play_stream("bgm/other.mp3", 0);
    assert(stream_attempts == 2 && sound_stream_playing()); allocation_failure = 0;
    g_sound_settings.bgm_on = 0; sound_play_bgm(name); name[4] = 'X';
    sound_stop_stream(); assert(!sound_stream_playing());
    sound_play_stream(NULL, 0); assert(strcmp(last_stream, "bgm/borrowed.mp3") == 0);
    sound_set_bgm_volume(999); assert(sound_bgm_volume() == 127 && stream_volume == 127);
    sound_set_bgm_volume(-1); assert(sound_bgm_volume() == 0 && stream_volume == 0);
    sound_set_bgm_fade_target(999); assert(sound_bgm_fade_target() == 127);
    sound_set_effect_volume(-1); assert(sound_effect_volume() == 0);
    sound_set_effect_volume(999); assert(sound_effect_volume() == 127);
    g_soundMode = 0; sound_mgr_destroy(); assert(shutdown_count == 1 && !allocations);
    cleanup(); assert(shutdown_count == 1 && registrations == 1);
}

static void test_effects(void)
{
    reset(1); sample_limit = 3; assert(sound_mgr_create());
    sound_play_wave("one.wav", 0, 0, 80, 250, 40, 0.5f);
    assert(sample_starts == 1 && volume_2d == 25);
    sound_play_wave("one.wav", 250, 0, 0, 250, 40, 1); assert(sample_starts == 1);
    g_sound_settings.effects_on = 0;
    sound_play_wave("one.wav", 0, 0, 0, 250, 40, 1); assert(sample_starts == 1);
    g_sound_settings.effects_on = 1; g_sound_settings.button_sound_only = 1;
    sound_play_wave("one.wav", 0, 0, 0, 250, 40, 1); assert(sample_starts == 1);
    sound_play_wave("버튼소리.wav", 0, 0, 0, 250, 40, 1); assert(sample_starts == 2);
    wave_missing = 1; sound_play_wave("버튼소리.wav", 0, 0, 0, 250, 40, 1); assert(sample_starts == 2);
    sound_end_samples(); sound_mgr_destroy(); assert(release_count == 3 && !allocations);
    reset(2); provider_index = 130; sample_limit = 2; assert(sound_mgr_create());
    assert(enumerations == 131 && sample_3d_allocated == 2);
    sound_set_effect_volume_3d(80); sound_play_wave("three.wav", 1, 2, 3, 250, 40, 0.5f);
    assert(sample_3d_starts == 1 && volume_3d == 40);
    assert(last_position[0] == 1 && last_position[1] == 2 && last_position[2] == 3);
    assert(last_orientation[0] == -1 && last_orientation[1] == -2 && last_orientation[2] == -3);
    assert(distances[0] == 250 && distances[1] == 40);
    reject_3d = 1; sound_play_wave("three.wav", 0, 0, 0, 250, 40, 1); assert(sample_3d_starts == 1);
    sound_set_room_type(7); assert(room_type == 7); sound_play_bgm("track"); assert(room_type == 0);
    sound_set_listener_angle(90); assert(fabsf(last_orientation[0] - 1) < 0.00001f);
    sound_end_samples(); g_soundMode = 0; sound_mgr_destroy(); assert(release_count == 2 && !allocations);
    reset(2); no_provider = 1; assert(sound_mgr_create()); assert(!provider_open);
    sound_play_wave("three.wav", 0, 0, 0, 250, 40, 1); assert(!sample_starts && !sample_3d_starts);
    sound_mgr_destroy(); assert(!allocations);
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
    assert(stream_open && driver_open && started && !shutdown_count);
    SetEvent(volume_release);
    return 0;
}

static void test_fades(void)
{
    HANDLE helper;
    int i;
    reset(1); assert(sound_mgr_create()); sound_play_bgm("track");
    sound_set_bgm_volume(0); sound_set_bgm_fade_target(3); sound_bgm_fade_in(); wait_volume(3);
    sound_bgm_fade_out(); wait_volume(0);
    thread_failure = 1; sound_bgm_fade_in(); assert(sound_bgm_volume() == 0); thread_failure = 0;
    for (i = 0; i < 20; ++i) { sound_bgm_fade_in(); sound_bgm_fade_out(); }
    sound_bgm_fade_out(); wait_volume(0);
    volume_entered = CreateEventW(NULL, TRUE, FALSE, NULL);
    volume_release = CreateEventW(NULL, TRUE, FALSE, NULL);
    assert(volume_entered && volume_release);
    InterlockedExchange(&block_volume, 1); sound_bgm_fade_in();
    assert(WaitForSingleObject(volume_entered, 2000) == WAIT_OBJECT_0);
    helper = (HANDLE)_beginthreadex(NULL, 0, release_backend, NULL, 0, NULL); assert(helper);
    sound_mgr_destroy();
    assert(WaitForSingleObject(helper, 2000) == WAIT_OBJECT_0); CloseHandle(helper);
    CloseHandle(volume_entered); CloseHandle(volume_release);
    assert(!allocations && shutdown_count == 1);
}

int main(void)
{
    int mode, type, cycle;
    test_failures(); test_streams(); test_effects(); test_fades();
    for (mode = 0; mode <= 2; ++mode) {
        for (type = 0; type < 3; ++type) {
            reset(mode); g_sound_sample_type = g_sound_rate_type = type;
            g_sound_bits_type = type != 0;
            expected_rate = type == 0 ? 22050 : type == 1 ? 11025 : 8000;
            expected_bits = type == 0 ? 16 : 8;
            assert(sound_mgr_create());
            if (mode) assert(sample_allocated + sample_3d_allocated == (type == 0 ? 48 : type == 1 ? 32 : 16));
            sound_mgr_destroy(); assert(!allocations);
        }
    }
    for (cycle = 0; cycle < 20; ++cycle) {
        reset(1); assert(sound_mgr_create()); sound_play_bgm("cycle"); sound_bgm_fade_out();
        sound_mgr_destroy(); sound_mgr_destroy(); assert(!allocations && shutdown_count == 1);
    }
    assert(registrations == 1);
    puts("Sound manager: options, samples, stream retry, provider enumeration, rollback, real fades and joined shutdown passed");
    return 0;
}
