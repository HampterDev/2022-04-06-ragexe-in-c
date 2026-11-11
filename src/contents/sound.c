#include "contents/sound.h"
#include "resources/wave.h"
#include "fileio/resource_api.h"
#include "platform/audio.h"
#include "platform/client_options.h"
#include "platform/thread.h"
#include <stdlib.h>
#include <string.h>

#define SOUND_VOICES 48
#define FADE_STEPS 1000
#define FADE_STEP_MS 10

struct SoundManager {
    struct PlatformMutex *stream_lock;
    struct PlatformThread *fade_thread;
    struct PlatformEvent *fade_cancel;
    int fade_in;
    int audio_opened;
    struct AudioMusic *music;
    struct AudioVoice *voices[SOUND_VOICES];
    int bgm_fade_target;
    int bgm_volume;
    int effect_volume;
    char *stream_name;
    char *bgm_name;
};

static struct SoundManager *sound_manager;
static int cleanup_registered;
struct SoundSettings g_sound_settings = { 1, 1, 0 };

static int clamp_volume(int volume)
{
    if (volume > SOUND_VOLUME_MAX)
        return SOUND_VOLUME_MAX;
    return volume < 0 ? 0 : volume;
}

static float volume_gain(int volume)
{
    return (float)volume / SOUND_VOLUME_MAX;
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
    audio_music_stop(mgr->music);
    mgr->music = NULL;
    free(mgr->stream_name);
    mgr->stream_name = NULL;
}

static void play_stream(struct SoundManager *mgr, const char *name, int loop_count)
{
    char *copy;
    if (!mgr->audio_opened || !name || !*name)
        return;
    if (mgr->music && strcmp(name, mgr->stream_name) == 0)
        return;
    copy = copy_name(name);
    if (!copy)
        return;
    close_stream(mgr);
    mgr->music = audio_music_play(copy, loop_count, volume_gain(mgr->bgm_volume));
    if (!mgr->music) {
        free(copy);
        return;
    }
    mgr->stream_name = copy;
}

static void set_bgm_volume(struct SoundManager *mgr, int volume)
{
    mgr->bgm_volume = clamp_volume(volume);
    audio_music_set_gain(mgr->music, volume_gain(mgr->bgm_volume));
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
    for (i = 0; i < SOUND_VOICES; ++i)
        audio_voice_stop(mgr->voices[i]);
    if (mgr->audio_opened)
        audio_close();
    if (mgr->fade_cancel)
        platform_event_destroy(mgr->fade_cancel);
    platform_mutex_destroy(mgr->stream_lock);
    free(mgr->bgm_name);
    free(mgr);
}

static int open_manager(struct SoundManager *mgr)
{
    if (g_soundMode == 0)
        return 1;
    mgr->audio_opened = audio_open(g_soundMode == 1);
    return mgr->audio_opened;
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
    mgr->effect_volume = 100;
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

int sound_stream_done(void)
{
    struct SoundManager *mgr = sound_manager;
    int done;
    if (!mgr || !g_soundMode)
        return 0;
    platform_mutex_lock(mgr->stream_lock);
    done = audio_music_done(mgr->music);
    platform_mutex_unlock(mgr->stream_lock);
    return done;
}

void sound_bgm_fade_in(void) { start_fade(1); }
void sound_bgm_fade_out(void) { start_fade(0); }

static int scaled_volume(const struct SoundManager *self, float scale)
{
    float volume = (float)self->effect_volume * scale;
    return volume >= SOUND_VOLUME_MAX ? SOUND_VOLUME_MAX : volume > 0.0f ? (int)volume : 0;
}

void sound_play_wave(const char *name, float volume)
{
    struct SoundManager *self = sound_manager;
    struct Wave *wave;
    int i;

    if (!self || !name || g_soundMode == 0 || !g_sound_settings.effects_on)
        return;
    wave = (struct Wave *)res_mgr_get(g_resMgr(), name);
    if (!wave || !wave->sound || (g_soundMode != 1 && g_soundMode != 2))
        return;
    for (i = 0; i < SOUND_VOICES; i++) {
        if (!audio_voice_playing(self->voices[i])) {
            audio_voice_stop(self->voices[i]);
            self->voices[i] = audio_voice_play(wave->sound, volume_gain(scaled_volume(self, volume)));
            return;
        }
    }
}

void sound_end_samples(void)
{
    struct SoundManager *self = sound_manager;
    int i;
    if (!self)
        return;
    for (i = 0; i < SOUND_VOICES; i++) {
        audio_voice_stop(self->voices[i]);
        self->voices[i] = NULL;
    }
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
