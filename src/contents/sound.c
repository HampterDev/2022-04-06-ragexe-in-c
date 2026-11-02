#include "contents/sound.h"
#include "resources/wave.h"
#include "fileio/resource_api.h"
#include "platform/client_options.h"
#include "platform/miles_api.h"
#include "render/math3d.h"
#include "platform/thread.h"
#include <stdlib.h>
#include <string.h>

#define SOUND_SAMPLES 48
#define FADE_STEPS 1000
#define FADE_STEP_MS 10

struct SoundManager {
    struct PlatformMutex *stream_lock;
    struct PlatformThread *fade_thread;
    struct PlatformEvent *fade_cancel;
    int fade_in;
    int miles_started;
    void *driver;
    void *stream;
    void *samples[SOUND_SAMPLES];
    void *samples_3d[SOUND_SAMPLES];
    void *provider;
    void *listener;
    int bgm_fade_target;
    int bgm_volume;
    int effect_volume;
    int effect_volume_3d;
    char *stream_name;
    char *bgm_name;
};

static struct SoundManager *sound_manager;
static int cleanup_registered;
struct SoundSettings g_sound_settings = { 1, 1, 0 };
int g_sound_speaker_type;

static int clamp_volume(int volume)
{
    if (volume > SOUND_VOLUME_MAX)
        return SOUND_VOLUME_MAX;
    return volume < 0 ? 0 : volume;
}

static char *copy_name(const char *name)
{
    size_t size = strlen(name) + 1;
    char *copy = malloc(size);
    if (copy)
        memcpy(copy, name, size);
    return copy;
}

static void close_stream(struct SoundManager *mgr)
{
    if (mgr->stream) {
        _AIL_close_stream(mgr->stream);
        mgr->stream = NULL;
    }
    free(mgr->stream_name);
    mgr->stream_name = NULL;
}

static void play_stream(struct SoundManager *mgr, const char *name, int loop_count)
{
    char *copy;
    if (!mgr->driver || !name || !*name)
        return;
    if (mgr->stream && strcmp(name, mgr->stream_name) == 0)
        return;
    copy = copy_name(name);
    if (!copy)
        return;
    close_stream(mgr);
    mgr->stream = _AIL_open_stream(mgr->driver, copy, 0);
    if (!mgr->stream) {
        free(copy);
        return;
    }
    mgr->stream_name = copy;
    _AIL_set_stream_volume(mgr->stream, mgr->bgm_volume);
    _AIL_set_stream_loop_count(mgr->stream, loop_count);
    _AIL_start_stream(mgr->stream);
}

static void set_bgm_volume(struct SoundManager *mgr, int volume)
{
    mgr->bgm_volume = clamp_volume(volume);
    if (mgr->stream)
        _AIL_set_stream_volume(mgr->stream, mgr->bgm_volume);
}

static void sound_fade_thread(void *context)
{
    struct SoundManager *mgr = context;
    int step;
    for (step = 0; step < FADE_STEPS; ++step) {
        int done;
        if (platform_event_wait(mgr->fade_cancel, 0))
            break;
        platform_mutex_lock(mgr->stream_lock);
        done = mgr->fade_in ? mgr->bgm_volume >= mgr->bgm_fade_target : mgr->bgm_volume <= 0;
        if (!done)
            set_bgm_volume(mgr, mgr->bgm_volume + (mgr->fade_in ? 1 : -1));
        platform_mutex_unlock(mgr->stream_lock);
        if (done || platform_event_wait(mgr->fade_cancel, FADE_STEP_MS))
            break;
    }
}

static void stop_fade(struct SoundManager *mgr)
{
    if (mgr->fade_thread) {
        platform_event_set(mgr->fade_cancel);
        platform_thread_join(mgr->fade_thread);
        mgr->fade_thread = NULL;
    }
}

static void start_fade(int fade_in)
{
    struct SoundManager *mgr = sound_manager;
    if (!mgr)
        return;
    stop_fade(mgr);
    platform_event_reset(mgr->fade_cancel);
    mgr->fade_in = fade_in;
    mgr->fade_thread = platform_thread_create(sound_fade_thread, mgr);
}

static void destroy_manager(struct SoundManager *mgr)
{
    int i;
    stop_fade(mgr);
    close_stream(mgr);
    for (i = 0; i < SOUND_SAMPLES; ++i) {
        if (mgr->samples_3d[i])
            _AIL_release_3D_sample_handle(mgr->samples_3d[i]);
        if (mgr->samples[i])
            _AIL_release_sample_handle(mgr->samples[i]);
    }
    if (mgr->listener)
        _AIL_close_3D_listener(mgr->listener);
    if (mgr->provider)
        _AIL_close_3D_provider(mgr->provider);
    if (mgr->driver)
        _AIL_close_digital_driver(mgr->driver);
    if (mgr->miles_started)
        _AIL_shutdown();
    if (mgr->fade_cancel)
        platform_event_destroy(mgr->fade_cancel);
    platform_mutex_destroy(mgr->stream_lock);
    free(mgr->bgm_name);
    free(mgr);
}

static int open_manager(struct SoundManager *mgr)
{
    int i, channels;
    void *next = NULL, *provider = NULL;
    char *name = NULL;
    if (g_soundMode == 0)
        return 1;
    _AIL_set_redist_directory(".\\");
    _AIL_startup();
    mgr->miles_started = 1;
    channels = g_soundMode == 1 ? 1 : 2;
    _AIL_set_preference(15, 0);
    mgr->driver = _AIL_open_digital_driver(22050, 16, channels, 1);
    if (!mgr->driver)
        return 0;
    if (g_soundMode == 1) {
        for (i = 0; i < SOUND_SAMPLES; ++i) {
            mgr->samples[i] = _AIL_allocate_sample_handle(mgr->driver);
            if (!mgr->samples[i])
                break;
        }
    } else if (g_soundMode == 2) {
        while (_AIL_enumerate_3D_providers(&next, &provider, &name)) {
            if (name && strcmp(name, "Miles Fast 2D Positional Audio") == 0) {
                if (_AIL_open_3D_provider(provider) != 0)
                    return 0;
                mgr->provider = provider;
                if (_AIL_3D_speaker_type(provider) != -1)
                    _AIL_set_3D_speaker_type(provider, g_sound_speaker_type);
                mgr->listener = _AIL_open_3D_listener(provider);
                if (!mgr->listener)
                    return 0;
                for (i = 0; i < SOUND_SAMPLES; ++i) {
                    mgr->samples_3d[i] = _AIL_allocate_3D_sample_handle(provider);
                    if (!mgr->samples_3d[i])
                        break;
                }
                break;
            }
        }
    }
    return 1;
}

int sound_mgr_create(void)
{
    struct SoundManager *mgr;
    if (sound_manager)
        return 0;
    mgr = calloc(1, sizeof *mgr);
    if (!mgr)
        return 0;
    mgr->stream_lock = platform_mutex_create();
    mgr->fade_cancel = platform_event_create();
    mgr->bgm_fade_target = mgr->bgm_volume = 100;
    mgr->effect_volume = mgr->effect_volume_3d = 100;
    if (!mgr->fade_cancel || !open_manager(mgr)) {
        destroy_manager(mgr);
        return 0;
    }
    if (!cleanup_registered) {
        if (atexit(sound_mgr_destroy) != 0) {
            destroy_manager(mgr);
            return 0;
        }
        cleanup_registered = 1;
    }
    g_sound_settings = (struct SoundSettings){ 1, 1, 0 };
    sound_manager = mgr;
    return 1;
}

void sound_mgr_destroy(void)
{
    struct SoundManager *mgr = sound_manager;
    if (!mgr)
        return;
    sound_manager = NULL;
    destroy_manager(mgr);
}

void sound_play_stream(const char *name, int loop_count)
{
    struct SoundManager *mgr = sound_manager;
    if (!mgr || !g_soundMode)
        return;
    platform_mutex_lock(mgr->stream_lock);
    play_stream(mgr, name ? name : mgr->bgm_name, loop_count);
    platform_mutex_unlock(mgr->stream_lock);
}

void sound_play_bgm(const char *name)
{
    struct SoundManager *mgr = sound_manager;
    char *copy = NULL;
    if (!mgr)
        return;
    if (name && *name) {
        copy = copy_name(name);
        if (!copy)
            return;
    }
    platform_mutex_lock(mgr->stream_lock);
    free(mgr->bgm_name);
    mgr->bgm_name = copy;
    if (!copy)
        close_stream(mgr);
    else if (g_sound_settings.bgm_on && g_soundMode)
        play_stream(mgr, copy, 0);
    platform_mutex_unlock(mgr->stream_lock);
    sound_set_room_type(0);
}

void sound_stop_stream(void)
{
    struct SoundManager *mgr = sound_manager;
    if (!mgr)
        return;
    platform_mutex_lock(mgr->stream_lock);
    close_stream(mgr);
    platform_mutex_unlock(mgr->stream_lock);
}

int sound_stream_playing(void)
{
    struct SoundManager *mgr = sound_manager;
    int playing;
    if (!mgr || !g_soundMode)
        return 0;
    platform_mutex_lock(mgr->stream_lock);
    playing = mgr->stream && _AIL_stream_status(mgr->stream) == MSS_STREAM_PLAYING;
    platform_mutex_unlock(mgr->stream_lock);
    return playing;
}

void sound_bgm_fade_in(void) { start_fade(1); }
void sound_bgm_fade_out(void) { start_fade(0); }

static char sound_play_2d(struct SoundManager *self, void *file, const float *x, const float *y,
                          const float *z, int max_distance, int full_distance, float volume_scale)
{
    float flat = float_sqrt(*x * *x + *z * *z);
    float distance = float_sqrt(*y * *y + flat * flat);
    int volume, i;

    if (distance >= (float)max_distance)
        return 0;
    volume = self->effect_volume;
    if (!((float)full_distance >= distance))
        volume = (int)((float)volume * (float)full_distance / distance);
    volume = (int)((float)volume * volume_scale);
    for (i = 0; i < SOUND_SAMPLES; i++) {
        void *sample = self->samples[i];

        if (sample && _AIL_sample_status(sample) == MSS_SAMPLE_DONE) {
            _AIL_init_sample(sample);
            _AIL_set_sample_file(sample, file, 0);
            _AIL_set_sample_volume(sample, volume);
            _AIL_start_sample(sample);
            return 1;
        }
    }
    return 0;
}

static char sound_play_3d(struct SoundManager *self, void *file, const float *x, const float *y,
                              const float *z, int max_distance, int full_distance, float volume_scale)
{
    int volume = (int)((float)self->effect_volume_3d * volume_scale);
    int i;

    if (!self->provider)
        return sound_play_2d(self, file, x, y, z, 250, 40, 1.0f);
    for (i = 0; i < SOUND_SAMPLES; i++) {
        void *sample = self->samples_3d[i];

        if (!sample || _AIL_3D_sample_status(sample) != MSS_SAMPLE_DONE)
            continue;
        if (!_AIL_set_3D_sample_file(sample, file))
            return 0;
        _AIL_set_3D_sample_effects_level(sample, 0.0f);
        _AIL_set_3D_position(sample, *x, *y, *z);
        _AIL_set_3D_orientation(sample, -*x, -*y, -*z, 0.0f, 1.0f, 0.0f);
        _AIL_set_3D_sample_volume(sample, volume);
        _AIL_set_3D_sample_distances(sample, (float)max_distance, (float)full_distance);
        _AIL_start_3D_sample(sample);
        return 1;
    }
    return 0;
}

void sound_play_wave(const char *name, float x, float y, float z,
                                int max_distance, int full_distance, float volume_scale)
{
    struct SoundManager *self = sound_manager;
    struct Wave *wave;

    if (!self || !name || g_soundMode == 0 || !g_sound_settings.effects_on)
        return;
    wave = (struct Wave *)res_mgr_get(g_resMgr(), name);
    if (!wave)
        return;
    if (g_soundMode == 1)
        sound_play_2d(self, wave->data, &x, &y, &z, max_distance, full_distance, volume_scale);
    else if (g_soundMode == 2)
        sound_play_3d(self, wave->data, &x, &y, &z, max_distance, full_distance, volume_scale);
}

void sound_end_samples(void)
{
    struct SoundManager *self = sound_manager;
    int i;
    if (!self)
        return;

    for (i = 0; i < SOUND_SAMPLES; i++)
        if (self->samples[i])
            _AIL_end_sample(self->samples[i]);
    if (!self->provider)
        return;
    for (i = 0; i < SOUND_SAMPLES; i++)
        if (self->samples_3d[i])
            _AIL_end_3D_sample(self->samples_3d[i]);
}

int sound_bgm_volume(void)
{
    struct SoundManager *mgr = sound_manager;
    int volume;
    if (!mgr)
        return 0;
    platform_mutex_lock(mgr->stream_lock);
    volume = mgr->bgm_volume;
    platform_mutex_unlock(mgr->stream_lock);
    return volume;
}

int sound_bgm_fade_target(void)
{
    return sound_manager ? sound_manager->bgm_fade_target : 0;
}

int sound_effect_volume(void)
{
    return sound_manager ? sound_manager->effect_volume : 0;
}

void sound_set_bgm_volume(int volume)
{
    struct SoundManager *mgr = sound_manager;
    if (!mgr)
        return;
    platform_mutex_lock(mgr->stream_lock);
    set_bgm_volume(mgr, volume);
    platform_mutex_unlock(mgr->stream_lock);
}

void sound_set_bgm_fade_target(int volume)
{
    struct SoundManager *mgr = sound_manager;
    if (!mgr)
        return;
    platform_mutex_lock(mgr->stream_lock);
    mgr->bgm_fade_target = clamp_volume(volume);
    platform_mutex_unlock(mgr->stream_lock);
}

void sound_set_effect_volume(int volume)
{
    if (sound_manager)
        sound_manager->effect_volume = clamp_volume(volume);
}

void sound_set_effect_volume_3d(int volume)
{
    if (sound_manager)
        sound_manager->effect_volume_3d = clamp_volume(volume);
}

void sound_set_listener_angle(float angle)
{
    struct SoundManager *mgr = sound_manager;
    int degrees = (int)(180.0f - angle);
    if (mgr && mgr->listener)
        _AIL_set_3D_orientation(mgr->listener, get_sin_deg(degrees), 0.0f,
                               -get_cos_deg(degrees), 0.0f, 1.0f, 0.0f);
}

void sound_set_room_type(int room_type)
{
    struct SoundManager *mgr = sound_manager;
    if (mgr && mgr->provider && _AIL_3D_room_type(mgr->provider) != -1)
        _AIL_set_3D_room_type(mgr->provider, room_type);
}
