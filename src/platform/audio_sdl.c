#include "platform/audio.h"
#include "platform/filesystem.h"
#include "../../vendor/dr_libs/dr_mp3.h"
#include <SDL3/SDL.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MUSIC_CHUNK 1152
/* What a sound file may decode to: sample rates around the 8 to 48 kHz of the game's files, and at
   most five minutes, seven times the longest WAV of data.grf (40 s), so a damaged header cannot
   hold one of the client's voices for hours. */
#define SOUND_RATE_MIN 1000
#define SOUND_RATE_MAX 384000
#define SOUND_SECONDS_MAX 300

struct AudioSound {
    SDL_AudioSpec spec;
    Uint8 *pcm;
    int size;
};

struct AudioMusic {
    SDL_AudioStream *stream;
    unsigned char *file;
    drmp3 mp3;
    int loop_count;
    int plays;
    int decoded;
    int ended;
};

static SDL_AudioDeviceID g_device;
static int g_wave_hint_set;

static void SDLCALL mix_to_mono(void *userdata, const SDL_AudioSpec *spec, float *buffer, int size)
{
    int frames = size / (int)sizeof *buffer / spec->channels, i;
    (void)userdata;
    if (spec->channels < 2)
        return;
    for (i = 0; i < frames; ++i, buffer += spec->channels)
        buffer[0] = buffer[1] = (buffer[0] + buffer[1]) * 0.5f;
}

int audio_open(int mono)
{
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        return 0;
    g_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (g_device && (!mono || SDL_SetAudioPostmixCallback(g_device, mix_to_mono, NULL)))
        return 1;
    audio_close();
    return 0;
}

void audio_close(void)
{
    if (g_device)
        SDL_CloseAudioDevice(g_device);
    g_device = 0;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

static int is_mp3(const unsigned char *bytes, size_t size)
{
    if (size >= 3 && !memcmp(bytes, "ID3", 3))
        return 1;
    return size >= 2 && bytes[0] == 0xff && (bytes[1] & 0xe6) == 0xe2;
}

static Uint8 *decode_mp3(const void *data, size_t size, SDL_AudioSpec *spec, Uint32 *length)
{
    drmp3_config config;
    drmp3_uint64 frames = 0;
    drmp3_int16 *pcm = drmp3_open_memory_and_read_pcm_frames_s16(data, size, &config, &frames, NULL);
    Uint8 *copy = NULL;
    /* an MP3 has one or two channels */
    if (pcm && frames && frames <= INT_MAX / 2 / sizeof *pcm) {
        *length = (Uint32)(frames * config.channels * sizeof *pcm);
        copy = SDL_malloc(*length);
        if (copy)
            memcpy(copy, pcm, *length);
        spec->format = SDL_AUDIO_S16;
        spec->channels = (int)config.channels;
        spec->freq = (int)config.sampleRate;
    }
    drmp3_free(pcm, NULL);
    return copy;
}

struct AudioSound *audio_sound_decode(const void *data, size_t size)
{
    struct AudioSound *sound;
    Uint32 length = 0;
    int frame;
    if (!data)
        return NULL;
    sound = calloc(1, sizeof *sound);
    if (!sound)
        return NULL;
    if (size >= 12 && !memcmp(data, "RIFF", 4) && !memcmp((const char *)data + 8, "WAVE", 4)) {
        /* keep the whole frames of an ADPCM block that the file cuts short, whatever the environment
           says */
        if (!g_wave_hint_set)
            g_wave_hint_set =
                SDL_SetHintWithPriority(SDL_HINT_WAVE_TRUNCATION, "dropframe", SDL_HINT_OVERRIDE);
        if (!SDL_LoadWAV_IO(SDL_IOFromConstMem(data, size), true, &sound->spec, &sound->pcm, &length))
            sound->pcm = NULL;
    } else if (is_mp3(data, size)) {
        sound->pcm = decode_mp3(data, size, &sound->spec, &length);
    }
    /* SDL takes PCM's block size from the header, which need not be a whole frame */
    frame = sound->pcm && sound->spec.channels > 0 ? SDL_AUDIO_FRAMESIZE(sound->spec) : 0;
    if (frame > 0)
        length -= length % (Uint32)frame;
    if (frame <= 0 || !length || length > INT_MAX || sound->spec.freq < SOUND_RATE_MIN ||
        sound->spec.freq > SOUND_RATE_MAX ||
        length / (Uint32)frame > (Uint32)sound->spec.freq * SOUND_SECONDS_MAX) {
        audio_sound_free(sound);
        return NULL;
    }
    sound->size = (int)length;
    return sound;
}

void audio_sound_free(struct AudioSound *sound)
{
    if (!sound)
        return;
    SDL_free(sound->pcm);
    free(sound);
}

/* A voice is the SDL_AudioStream it plays from, bound to the device until it is stopped. */
struct AudioVoice *audio_voice_play(const struct AudioSound *sound, float gain)
{
    SDL_AudioStream *stream;
    if (!g_device || !sound)
        return NULL;
    /* The stream takes the whole sound before the device sees it; binding gives it the device's
       format. */
    stream = SDL_CreateAudioStream(&sound->spec, &sound->spec);
    if (!stream)
        return NULL;
    if (!SDL_SetAudioStreamGain(stream, gain) || !SDL_PutAudioStreamData(stream, sound->pcm, sound->size) ||
        !SDL_FlushAudioStream(stream) || !SDL_BindAudioStream(g_device, stream)) {
        SDL_DestroyAudioStream(stream);
        return NULL;
    }
    return (struct AudioVoice *)stream;
}

int audio_voice_playing(struct AudioVoice *voice)
{
    return voice && SDL_GetAudioStreamAvailable((SDL_AudioStream *)voice) > 0;
}

void audio_voice_stop(struct AudioVoice *voice)
{
    if (voice)
        SDL_DestroyAudioStream((SDL_AudioStream *)voice);
}

static unsigned char *file_load(const char *name, size_t *size)
{
    struct PlatformFile *file = platform_file_open_read(name);
    unsigned char *data = NULL;
    int64_t length;
    if (!file)
        return NULL;
    if (platform_file_size(file, &length) && length > 0 && (uint64_t)length <= SIZE_MAX) {
        data = malloc((size_t)length);
        if (data && !platform_file_read(file, data, (size_t)length)) {
            free(data);
            data = NULL;
        }
    }
    platform_file_close(file);
    *size = data ? (size_t)length : 0;
    return data;
}

/* Runs on the device's thread with the stream locked, whenever the device wants more of it. */
static void SDLCALL music_feed(void *userdata, SDL_AudioStream *stream, int additional, int total)
{
    struct AudioMusic *music = userdata;
    drmp3_int16 pcm[MUSIC_CHUNK * 2];
    (void)total;
    while (additional > 0 && !music->ended) {
        drmp3_uint64 frames = drmp3_read_pcm_frames_s16(&music->mp3, MUSIC_CHUNK, pcm);
        if (frames) {
            int bytes = (int)frames * (int)music->mp3.channels * (int)sizeof *pcm;
            SDL_PutAudioStreamData(stream, pcm, bytes);
            additional -= bytes;
            music->decoded = 1;
        } else if (music->decoded && (!music->loop_count || ++music->plays < music->loop_count) &&
                   drmp3_seek_to_pcm_frame(&music->mp3, 0)) {
            music->decoded = 0;
        } else {
            music->ended = 1;
            SDL_FlushAudioStream(stream);
        }
    }
}

struct AudioMusic *audio_music_play(const char *path, int loop_count, float gain)
{
    struct AudioMusic *music;
    SDL_AudioSpec spec;
    size_t size;
    if (!g_device || !path)
        return NULL;
    music = calloc(1, sizeof *music);
    if (!music)
        return NULL;
    music->file = file_load(path, &size);
    if (!music->file || !drmp3_init_memory(&music->mp3, music->file, size, NULL)) {
        free(music->file);
        free(music);
        return NULL;
    }
    music->loop_count = loop_count;
    spec.format = SDL_AUDIO_S16;
    spec.channels = (int)music->mp3.channels;
    spec.freq = (int)music->mp3.sampleRate;
    music->stream = SDL_CreateAudioStream(&spec, NULL);
    if (!music->stream || !SDL_SetAudioStreamGain(music->stream, gain) ||
        !SDL_SetAudioStreamGetCallback(music->stream, music_feed, music) ||
        !SDL_BindAudioStream(g_device, music->stream)) {
        audio_music_stop(music);
        return NULL;
    }
    return music;
}

void audio_music_set_gain(struct AudioMusic *music, float gain)
{
    if (music)
        SDL_SetAudioStreamGain(music->stream, gain);
}

int audio_music_done(struct AudioMusic *music)
{
    int done;
    if (!music)
        return 0;
    SDL_LockAudioStream(music->stream);
    done = music->ended && SDL_GetAudioStreamAvailable(music->stream) <= 0;
    SDL_UnlockAudioStream(music->stream);
    return done;
}

void audio_music_stop(struct AudioMusic *music)
{
    if (!music)
        return;
    SDL_DestroyAudioStream(music->stream);
    drmp3_uninit(&music->mp3);
    free(music->file);
    free(music);
}
