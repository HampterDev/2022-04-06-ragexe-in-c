#include "platform/win_types.h"
#ifndef PLATFORM_RAD_DLLS
#include "platform/miles_api.h"
#include "platform/filesystem.h"
#include "platform/thread.h"
#include "../../vendor/dr_libs/dr_mp3.h"
#include <SDL3/SDL.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VOLUME_MAX 127
#define MIX_CHUNK 512
#define MP3_CHUNK 1152
#define WAV_HEADER_SIZE 44
#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_MS_ADPCM 2
#define WAVE_FORMAT_IMA_ADPCM 17
#define AIL_FILE_UNKNOWN 0
#define AIL_FILE_PCM_WAV 1
#define AIL_FILE_OTHER_WAV 3
#define PROVIDER_OK 0
#define PROVIDER_ERROR 3
#define PROVIDER_FEATURE_UNSUPPORTED (-1)
#define DEFAULT_MIN_DISTANCE 1.0f
#define DEFAULT_MAX_DISTANCE 1.0e9f

enum { MILES_DONE = MSS_SAMPLE_DONE, MILES_PLAYING = MSS_STREAM_PLAYING };

struct MilesDriver;

struct MilesObject {
    struct MilesDriver *driver;
    float position[3];
    float face[3];
    float up[3];
};

struct MilesSample {
    struct MilesObject object;
    struct MilesSample *next;
    int16_t *pcm;
    uint32_t frames;
    uint32_t cursor;
    int channels;
    int positional;
    int volume;
    unsigned status;
    float min_distance;
    float max_distance;
};

struct MilesStream {
    struct MilesDriver *driver;
    struct MilesStream *next;
    unsigned char *file;
    drmp3 mp3;
    SDL_AudioStream *convert;
    int volume;
    int loop_count;
    int plays;
    int decoded;
    int ending;
    int status;
};

struct MilesDriver {
    struct PlatformMutex *lock;
    SDL_AudioStream *device;
    unsigned rate;
    int channels;
    struct MilesSample *samples;
    struct MilesStream *streams;
    struct MilesObject listener;
};

struct MilesProvider {
    struct MilesDriver *driver;
};

struct WaveFormat {
    unsigned format;
    int channels;
    unsigned rate;
    int bits;
    unsigned block_size;
    const unsigned char *data;
    uint32_t length;
};

struct MsAdpcmChannel {
    int coef1;
    int coef2;
    int delta;
    int sample1;
    int sample2;
};

static const int16_t ima_steps[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428,
    4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767,
};
static const int8_t ima_index_steps[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };
static const int ms_adaptation[16] = { 230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230, 230 };
static const int ms_coef1[7] = { 256, 512, 0, 192, 240, 460, 392 };
static const int ms_coef2[7] = { 0, -256, 0, 64, 0, -208, -232 };

static char g_provider_name[] = "Miles Fast 2D Positional Audio";
static struct MilesProvider g_provider;
static int g_audio_started;

static uint32_t get_u16(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}

static uint32_t get_u32(const unsigned char *p)
{
    return get_u16(p) | get_u16(p + 2) << 16;
}

static void put_u16(unsigned char *p, uint32_t value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
}

static void put_u32(unsigned char *p, uint32_t value)
{
    put_u16(p, value);
    put_u16(p + 2, value >> 16);
}

static int clamp16(int value)
{
    return value > INT16_MAX ? INT16_MAX : value < INT16_MIN ? INT16_MIN : value;
}

static int16_t mix_to_s16(float value)
{
    if (value >= (float)INT16_MAX)
        return INT16_MAX;
    if (value <= (float)INT16_MIN)
        return INT16_MIN;
    return (int16_t)lrintf(value);
}

static int clamp_volume(int volume)
{
    return volume > VOLUME_MAX ? VOLUME_MAX : volume < 0 ? 0 : volume;
}

static int wave_parse(const void *image, struct WaveFormat *wave)
{
    const unsigned char *bytes = image;
    size_t end, offset = 12;
    int have_format = 0;
    if (memcmp(bytes, "RIFF", 4) || memcmp(bytes + 8, "WAVE", 4))
        return 0;
    memset(wave, 0, sizeof *wave);
    end = (size_t)get_u32(bytes + 4) + 8;
    while (offset + 8 <= end) {
        const unsigned char *chunk = bytes + offset;
        uint32_t length = get_u32(chunk + 4);
        offset += 8;
        if (length > end - offset)
            return 0;
        if (!memcmp(chunk, "fmt ", 4) && length >= 16) {
            wave->format = get_u16(chunk + 8);
            wave->channels = (int)get_u16(chunk + 10);
            wave->rate = get_u32(chunk + 12);
            wave->block_size = get_u16(chunk + 20);
            wave->bits = (int)get_u16(chunk + 22);
            have_format = 1;
        } else if (!memcmp(chunk, "data", 4)) {
            wave->data = chunk + 8;
            wave->length = length;
        }
        offset += length + (length & 1);
    }
    return have_format && wave->data;
}

static uint32_t adpcm_block_frames(unsigned format, int channels, size_t size)
{
    size_t header = (format == WAVE_FORMAT_IMA_ADPCM ? 4 : 7) * (size_t)channels;
    if (size < header)
        return 0;
    if (format == WAVE_FORMAT_IMA_ADPCM)
        return (uint32_t)(1 + (size - header) / (4 * (size_t)channels) * 8);
    return (uint32_t)(2 + (size - header) * 2 / (size_t)channels);
}

static uint32_t wave_frames(const struct WaveFormat *wave)
{
    uint64_t frames;
    if (wave->channels <= 0 || !wave->block_size)
        return 0;
    if (wave->format == WAVE_FORMAT_PCM)
        return wave->length / wave->block_size;
    if (wave->format != WAVE_FORMAT_IMA_ADPCM && wave->format != WAVE_FORMAT_MS_ADPCM)
        return 0;
    frames = (uint64_t)(wave->length / wave->block_size) *
             adpcm_block_frames(wave->format, wave->channels, wave->block_size);
    frames += adpcm_block_frames(wave->format, wave->channels, wave->length % wave->block_size);
    return frames > UINT32_MAX ? UINT32_MAX : (uint32_t)frames;
}

static unsigned char *wav_alloc(int channels, uint64_t frames)
{
    uint64_t bytes = frames * (uint64_t)channels * sizeof(int16_t);
    if (!frames || bytes > UINT32_MAX - WAV_HEADER_SIZE)
        return NULL;
    return malloc(WAV_HEADER_SIZE + (size_t)bytes);
}

static unsigned wav_header(unsigned char *image, int channels, unsigned rate, uint32_t frames)
{
    uint32_t block = (uint32_t)channels * sizeof(int16_t), bytes = frames * block;
    memcpy(image, "RIFF", 4);
    put_u32(image + 4, WAV_HEADER_SIZE - 8 + bytes);
    memcpy(image + 8, "WAVEfmt ", 8);
    put_u32(image + 16, 16);
    put_u16(image + 20, WAVE_FORMAT_PCM);
    put_u16(image + 22, (uint32_t)channels);
    put_u32(image + 24, rate);
    put_u32(image + 28, rate * block);
    put_u16(image + 32, block);
    put_u16(image + 34, 16);
    memcpy(image + 36, "data", 4);
    put_u32(image + 40, bytes);
    return WAV_HEADER_SIZE + bytes;
}

static int16_t ima_nibble(int *predictor, int *index, int nibble)
{
    int step = ima_steps[*index], diff = step >> 3;
    if (nibble & 4)
        diff += step;
    if (nibble & 2)
        diff += step >> 1;
    if (nibble & 1)
        diff += step >> 2;
    *predictor = clamp16(nibble & 8 ? *predictor - diff : *predictor + diff);
    *index += ima_index_steps[nibble & 7];
    *index = *index < 0 ? 0 : *index > 88 ? 88 : *index;
    return (int16_t)*predictor;
}

static uint32_t ima_block(const unsigned char *block, size_t size, int channels, int16_t *out, uint32_t limit)
{
    int predictor[2], index[2], c, i;
    size_t header = 4 * (size_t)channels, groups, g;
    uint32_t frames = 1;
    if (size < header)
        return 0;
    for (c = 0; c < channels; ++c) {
        predictor[c] = (int16_t)get_u16(block + 4 * c);
        index[c] = block[4 * c + 2] > 88 ? 88 : block[4 * c + 2];
        out[c] = (int16_t)predictor[c];
    }
    groups = (size - header) / (4 * (size_t)channels);
    for (g = 0; g < groups && frames < limit; ++g) {
        int16_t decoded[2][8];
        for (c = 0; c < channels; ++c) {
            const unsigned char *bytes = block + header + (g * (size_t)channels + (size_t)c) * 4;
            for (i = 0; i < 4; ++i) {
                decoded[c][2 * i] = ima_nibble(&predictor[c], &index[c], bytes[i] & 15);
                decoded[c][2 * i + 1] = ima_nibble(&predictor[c], &index[c], bytes[i] >> 4);
            }
        }
        for (i = 0; i < 8 && frames < limit; ++i, ++frames)
            for (c = 0; c < channels; ++c)
                out[(size_t)frames * channels + c] = decoded[c][i];
    }
    return frames;
}

static int16_t ms_nibble(struct MsAdpcmChannel *channel, int nibble)
{
    int predicted = (channel->sample1 * channel->coef1 + channel->sample2 * channel->coef2) / 256;
    predicted += (nibble & 8 ? nibble - 16 : nibble) * channel->delta;
    channel->sample2 = channel->sample1;
    channel->sample1 = clamp16(predicted);
    channel->delta = ms_adaptation[nibble] * channel->delta / 256;
    if (channel->delta < 16)
        channel->delta = 16;
    if (channel->delta > INT_MAX / 768)
        channel->delta = INT_MAX / 768;
    return (int16_t)channel->sample1;
}

static uint32_t ms_block(const unsigned char *block, size_t size, int channels, int16_t *out, uint32_t limit)
{
    struct MsAdpcmChannel state[2];
    size_t header = 7 * (size_t)channels, nibbles, n;
    uint32_t frames;
    int c;
    if (size < header)
        return 0;
    for (c = 0; c < channels; ++c) {
        if (block[c] > 6)
            return 0;
        state[c].coef1 = ms_coef1[block[c]];
        state[c].coef2 = ms_coef2[block[c]];
        state[c].delta = (int16_t)get_u16(block + channels + 2 * c);
        state[c].sample1 = (int16_t)get_u16(block + 3 * channels + 2 * c);
        state[c].sample2 = (int16_t)get_u16(block + 5 * channels + 2 * c);
        out[c] = (int16_t)state[c].sample2;
        if (limit > 1)
            out[channels + c] = (int16_t)state[c].sample1;
    }
    frames = limit < 2 ? limit : 2;
    nibbles = (size - header) * 2;
    for (n = 0; n < nibbles && frames < limit; ++n) {
        unsigned char byte = block[header + n / 2];
        c = (int)(n % (size_t)channels);
        out[(size_t)frames * channels + c] = ms_nibble(&state[c], n & 1 ? byte & 15 : byte >> 4);
        if (c == channels - 1)
            ++frames;
    }
    return frames;
}

static void object_reset(struct MilesObject *object)
{
    memset(object->position, 0, sizeof object->position);
    memset(object->face, 0, sizeof object->face);
    memset(object->up, 0, sizeof object->up);
    object->face[2] = 1.0f;
    object->up[1] = 1.0f;
}

static float dot3(const float *a, const float *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void sample_gains(const struct MilesSample *sample, float gain[2])
{
    const struct MilesObject *listener = &sample->object.driver->listener;
    float level = (float)sample->volume / VOLUME_MAX, offset[3], right[3], distance, side, pan;
    int i;
    gain[0] = gain[1] = level;
    if (!sample->positional)
        return;
    for (i = 0; i < 3; ++i)
        offset[i] = sample->object.position[i] - listener->position[i];
    distance = sqrtf(dot3(offset, offset));
    if (distance >= sample->max_distance)
        level = 0.0f;
    else if (distance > sample->min_distance)
        level *= sample->min_distance / distance;
    right[0] = listener->up[1] * listener->face[2] - listener->up[2] * listener->face[1];
    right[1] = listener->up[2] * listener->face[0] - listener->up[0] * listener->face[2];
    right[2] = listener->up[0] * listener->face[1] - listener->up[1] * listener->face[0];
    side = sqrtf(dot3(right, right));
    pan = distance > 0.0f && side > 0.0f ? dot3(offset, right) / (distance * side) : 0.0f;
    gain[0] = level * (pan > 0.0f ? 1.0f - pan : 1.0f);
    gain[1] = level * (pan < 0.0f ? 1.0f + pan : 1.0f);
    if (sample->object.driver->channels == 1)
        gain[0] = level;
}

static void sample_mix(struct MilesSample *sample, float *mix, int frames)
{
    int channels = sample->object.driver->channels, c;
    uint32_t left = sample->frames - sample->cursor, count, i;
    const int16_t *pcm = sample->pcm + (size_t)sample->cursor * sample->channels;
    float gain[2];
    count = (uint32_t)frames < left ? (uint32_t)frames : left;
    sample_gains(sample, gain);
    for (i = 0; i < count; ++i)
        for (c = 0; c < channels; ++c)
            mix[i * channels + c] += pcm[i * sample->channels + (sample->channels == 1 ? 0 : c)] * gain[c];
    sample->cursor += count;
    if (sample->cursor >= sample->frames)
        sample->status = MILES_DONE;
}

static void stream_decode(struct MilesStream *stream, int bytes)
{
    int16_t pcm[MP3_CHUNK * 2];
    while (!stream->ending && SDL_GetAudioStreamAvailable(stream->convert) < bytes) {
        drmp3_uint64 frames = drmp3_read_pcm_frames_s16(&stream->mp3, MP3_CHUNK, pcm);
        if (frames) {
            SDL_PutAudioStreamData(stream->convert, pcm,
                                   (int)(frames * stream->mp3.channels * sizeof(int16_t)));
            stream->decoded = 1;
        } else if (stream->decoded && (stream->loop_count == 0 || ++stream->plays < stream->loop_count) &&
                   drmp3_seek_to_pcm_frame(&stream->mp3, 0)) {
            stream->decoded = 0;
        } else {
            SDL_FlushAudioStream(stream->convert);
            stream->ending = 1;
        }
    }
}

static void stream_mix(struct MilesStream *stream, float *mix, int frames)
{
    int16_t pcm[MIX_CHUNK * 2];
    int bytes = frames * stream->driver->channels * (int)sizeof(int16_t), got, i;
    float gain = (float)stream->volume / VOLUME_MAX;
    stream_decode(stream, bytes);
    got = SDL_GetAudioStreamData(stream->convert, pcm, bytes);
    if (got < 0)
        got = 0;
    for (i = 0; i < got / (int)sizeof(int16_t); ++i)
        mix[i] += pcm[i] * gain;
    if (got < bytes && stream->ending)
        stream->status = MILES_DONE;
}

static void driver_mix(struct MilesDriver *driver, int16_t *out, int frames)
{
    float mix[MIX_CHUNK * 2];
    struct MilesSample *sample;
    struct MilesStream *stream;
    int done, count, i;
    platform_mutex_lock(driver->lock);
    for (done = 0; done < frames; done += count) {
        count = frames - done < MIX_CHUNK ? frames - done : MIX_CHUNK;
        memset(mix, 0, (size_t)count * driver->channels * sizeof *mix);
        for (sample = driver->samples; sample; sample = sample->next)
            if (sample->status == MILES_PLAYING)
                sample_mix(sample, mix, count);
        for (stream = driver->streams; stream; stream = stream->next)
            if (stream->status == MILES_PLAYING)
                stream_mix(stream, mix, count);
        for (i = 0; i < count * driver->channels; ++i)
            out[done * driver->channels + i] = mix_to_s16(mix[i]);
    }
    platform_mutex_unlock(driver->lock);
}

static void SDLCALL driver_feed(void *context, SDL_AudioStream *device, int additional, int total)
{
    struct MilesDriver *driver = context;
    int16_t block[MIX_CHUNK * 2];
    int frame_bytes = driver->channels * (int)sizeof(int16_t);
    int frames = (additional + frame_bytes - 1) / frame_bytes;
    (void)total;
    while (frames > 0) {
        int count = frames < MIX_CHUNK ? frames : MIX_CHUNK;
        driver_mix(driver, block, count);
        SDL_PutAudioStreamData(device, block, count * frame_bytes);
        frames -= count;
    }
}

static SDL_AudioSpec driver_spec(const struct MilesDriver *driver, int channels)
{
    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_S16;
    spec.channels = channels;
    spec.freq = (int)driver->rate;
    return spec;
}

static struct MilesDriver *driver_create(unsigned rate, int channels)
{
    struct MilesDriver *driver = calloc(1, sizeof *driver);
    if (!driver)
        return NULL;
    driver->lock = platform_mutex_create();
    if (!driver->lock) {
        free(driver);
        return NULL;
    }
    driver->rate = rate;
    driver->channels = channels;
    driver->listener.driver = driver;
    object_reset(&driver->listener);
    g_provider.driver = driver;
    return driver;
}

static struct MilesSample *sample_create(struct MilesDriver *driver, int positional)
{
    struct MilesSample *sample;
    if (!driver)
        return NULL;
    sample = calloc(1, sizeof *sample);
    if (!sample)
        return NULL;
    sample->object.driver = driver;
    object_reset(&sample->object);
    sample->positional = positional;
    sample->channels = positional ? 1 : driver->channels;
    sample->volume = VOLUME_MAX;
    sample->status = MILES_DONE;
    sample->min_distance = DEFAULT_MIN_DISTANCE;
    sample->max_distance = DEFAULT_MAX_DISTANCE;
    platform_mutex_lock(driver->lock);
    sample->next = driver->samples;
    driver->samples = sample;
    platform_mutex_unlock(driver->lock);
    return sample;
}

static void sample_release(struct MilesSample *sample)
{
    struct MilesDriver *driver;
    struct MilesSample **link;
    if (!sample)
        return;
    driver = sample->object.driver;
    platform_mutex_lock(driver->lock);
    for (link = &driver->samples; *link && *link != sample; link = &(*link)->next)
        ;
    if (*link)
        *link = sample->next;
    platform_mutex_unlock(driver->lock);
    SDL_free(sample->pcm);
    free(sample);
}

static Uint8 *wave_convert(const void *file, const SDL_AudioSpec *target, int *size)
{
    struct WaveFormat wave;
    SDL_AudioSpec source;
    Uint8 *pcm = NULL;
    uint32_t frame_bytes;
    *size = 0;
    if (!file || !wave_parse(file, &wave) || wave.format != WAVE_FORMAT_PCM || (wave.bits != 8 && wave.bits != 16) ||
        wave.channels < 1 || wave.channels > 2 || !wave.rate || wave.rate > INT_MAX || wave.length > INT_MAX)
        return NULL;
    frame_bytes = (uint32_t)wave.channels * (uint32_t)wave.bits / 8;
    source.format = wave.bits == 8 ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE;
    source.channels = wave.channels;
    source.freq = (int)wave.rate;
    if (!SDL_ConvertAudioSamples(&source, wave.data, (int)(wave.length - wave.length % frame_bytes), target, &pcm,
                                 size)) {
        *size = 0;
        return NULL;
    }
    return pcm;
}

static int sample_load(struct MilesSample *sample, const void *file)
{
    SDL_AudioSpec target;
    Uint8 *pcm;
    int size;
    if (!sample)
        return 0;
    target = driver_spec(sample->object.driver, sample->channels);
    pcm = wave_convert(file, &target, &size);
    platform_mutex_lock(sample->object.driver->lock);
    SDL_free(sample->pcm);
    sample->pcm = (int16_t *)pcm;
    sample->frames = (uint32_t)size / ((uint32_t)sample->channels * sizeof(int16_t));
    sample->cursor = 0;
    sample->status = MILES_DONE;
    platform_mutex_unlock(sample->object.driver->lock);
    return pcm != NULL;
}

static void sample_start(struct MilesSample *sample)
{
    if (!sample)
        return;
    platform_mutex_lock(sample->object.driver->lock);
    if (sample->pcm && sample->frames) {
        sample->cursor = 0;
        sample->status = MILES_PLAYING;
    }
    platform_mutex_unlock(sample->object.driver->lock);
}

static void sample_end(struct MilesSample *sample)
{
    if (!sample)
        return;
    platform_mutex_lock(sample->object.driver->lock);
    sample->status = MILES_DONE;
    platform_mutex_unlock(sample->object.driver->lock);
}

static void sample_set_volume(struct MilesSample *sample, int volume)
{
    if (!sample)
        return;
    platform_mutex_lock(sample->object.driver->lock);
    sample->volume = clamp_volume(volume);
    platform_mutex_unlock(sample->object.driver->lock);
}

static unsigned sample_status(struct MilesSample *sample)
{
    unsigned status;
    if (!sample)
        return MILES_DONE;
    platform_mutex_lock(sample->object.driver->lock);
    status = sample->status;
    platform_mutex_unlock(sample->object.driver->lock);
    return status;
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

static void stream_close(struct MilesStream *stream)
{
    struct MilesDriver *driver = stream->driver;
    struct MilesStream **link;
    platform_mutex_lock(driver->lock);
    for (link = &driver->streams; *link && *link != stream; link = &(*link)->next)
        ;
    if (*link)
        *link = stream->next;
    platform_mutex_unlock(driver->lock);
    SDL_DestroyAudioStream(stream->convert);
    drmp3_uninit(&stream->mp3);
    free(stream->file);
    free(stream);
}

static void driver_destroy(struct MilesDriver *driver)
{
    if (driver->device)
        SDL_DestroyAudioStream(driver->device);
    while (driver->samples)
        sample_release(driver->samples);
    while (driver->streams)
        stream_close(driver->streams);
    if (g_provider.driver == driver)
        g_provider.driver = NULL;
    platform_mutex_destroy(driver->lock);
    free(driver);
}

void __stdcall _AIL_set_redist_directory(const char *dir)
{
    (void)dir;
}

int __stdcall _AIL_startup(void)
{
    if (!g_audio_started)
        g_audio_started = SDL_InitSubSystem(SDL_INIT_AUDIO);
    return g_audio_started;
}

void __stdcall _AIL_shutdown(void)
{
    if (g_audio_started)
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    g_audio_started = 0;
}

int __stdcall _AIL_set_preference(unsigned number, int value)
{
    (void)number;
    (void)value;
    return 0;
}

void *__stdcall _AIL_open_digital_driver(unsigned rate, int bits, int channels, unsigned flags)
{
    struct MilesDriver *driver;
    SDL_AudioSpec spec;
    (void)flags;
    if (bits != 16 || channels < 1 || channels > 2 || !rate || rate > INT_MAX || g_provider.driver)
        return NULL;
    driver = driver_create(rate, channels);
    if (!driver)
        return NULL;
    spec = driver_spec(driver, channels);
    driver->device = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, driver_feed, driver);
    if (!driver->device || !SDL_ResumeAudioStreamDevice(driver->device)) {
        driver_destroy(driver);
        return NULL;
    }
    return driver;
}

void __stdcall _AIL_close_digital_driver(void *driver)
{
    if (driver)
        driver_destroy(driver);
}

void *__stdcall _AIL_allocate_sample_handle(void *driver)
{
    return sample_create(driver, 0);
}

void __stdcall _AIL_release_sample_handle(void *sample)
{
    sample_release(sample);
}

void __stdcall _AIL_init_sample(void *handle)
{
    struct MilesSample *sample = handle;
    if (!sample)
        return;
    platform_mutex_lock(sample->object.driver->lock);
    SDL_free(sample->pcm);
    sample->pcm = NULL;
    sample->frames = sample->cursor = 0;
    sample->volume = VOLUME_MAX;
    sample->status = MILES_DONE;
    platform_mutex_unlock(sample->object.driver->lock);
}

int __stdcall _AIL_set_sample_file(void *sample, const void *file, int block)
{
    (void)block;
    return sample_load(sample, file);
}

void __stdcall _AIL_set_sample_volume(void *sample, int volume)
{
    sample_set_volume(sample, volume);
}

void __stdcall _AIL_start_sample(void *sample)
{
    sample_start(sample);
}

void __stdcall _AIL_end_sample(void *sample)
{
    sample_end(sample);
}

unsigned __stdcall _AIL_sample_status(void *sample)
{
    return sample_status(sample);
}

void *__stdcall _AIL_open_stream(void *handle, const char *name, int stream_memory)
{
    struct MilesDriver *driver = handle;
    struct MilesStream *stream;
    SDL_AudioSpec source, target;
    size_t size;
    (void)stream_memory;
    if (!driver || !name)
        return NULL;
    stream = calloc(1, sizeof *stream);
    if (!stream)
        return NULL;
    stream->file = file_load(name, &size);
    if (!stream->file || !drmp3_init_memory(&stream->mp3, stream->file, size, NULL)) {
        free(stream->file);
        free(stream);
        return NULL;
    }
    source.format = SDL_AUDIO_S16;
    source.channels = (int)stream->mp3.channels;
    source.freq = (int)stream->mp3.sampleRate;
    target = driver_spec(driver, driver->channels);
    stream->convert = SDL_CreateAudioStream(&source, &target);
    if (!stream->convert) {
        drmp3_uninit(&stream->mp3);
        free(stream->file);
        free(stream);
        return NULL;
    }
    stream->driver = driver;
    stream->volume = VOLUME_MAX;
    stream->loop_count = 1;
    stream->status = MILES_DONE;
    platform_mutex_lock(driver->lock);
    stream->next = driver->streams;
    driver->streams = stream;
    platform_mutex_unlock(driver->lock);
    return stream;
}

void __stdcall _AIL_close_stream(void *stream)
{
    if (stream)
        stream_close(stream);
}

void __stdcall _AIL_start_stream(void *handle)
{
    struct MilesStream *stream = handle;
    if (!stream)
        return;
    platform_mutex_lock(stream->driver->lock);
    SDL_ClearAudioStream(stream->convert);
    if (drmp3_seek_to_pcm_frame(&stream->mp3, 0)) {
        stream->plays = 0;
        stream->decoded = 0;
        stream->ending = 0;
        stream->status = MILES_PLAYING;
    }
    platform_mutex_unlock(stream->driver->lock);
}

void __stdcall _AIL_set_stream_volume(void *handle, int volume)
{
    struct MilesStream *stream = handle;
    if (!stream)
        return;
    platform_mutex_lock(stream->driver->lock);
    stream->volume = clamp_volume(volume);
    platform_mutex_unlock(stream->driver->lock);
}

void __stdcall _AIL_set_stream_loop_count(void *handle, int count)
{
    struct MilesStream *stream = handle;
    if (!stream)
        return;
    platform_mutex_lock(stream->driver->lock);
    stream->loop_count = count;
    platform_mutex_unlock(stream->driver->lock);
}

int __stdcall _AIL_stream_status(void *handle)
{
    struct MilesStream *stream = handle;
    int status;
    if (!stream)
        return MILES_DONE;
    platform_mutex_lock(stream->driver->lock);
    status = stream->status;
    platform_mutex_unlock(stream->driver->lock);
    return status;
}

int __stdcall _AIL_enumerate_3D_providers(void **next, void **dest, char **name)
{
    if (*next)
        return 0;
    *next = &g_provider;
    *dest = &g_provider;
    *name = g_provider_name;
    return 1;
}

int __stdcall _AIL_open_3D_provider(void *provider)
{
    struct MilesProvider *owner = provider;
    return owner == &g_provider && owner->driver ? PROVIDER_OK : PROVIDER_ERROR;
}

void __stdcall _AIL_close_3D_provider(void *provider)
{
    (void)provider;
}

int __stdcall _AIL_3D_speaker_type(void *provider)
{
    (void)provider;
    return PROVIDER_FEATURE_UNSUPPORTED;
}

void __stdcall _AIL_set_3D_speaker_type(void *provider, int type)
{
    (void)provider;
    (void)type;
}

int __stdcall _AIL_3D_room_type(void *provider)
{
    (void)provider;
    return PROVIDER_FEATURE_UNSUPPORTED;
}

void __stdcall _AIL_set_3D_room_type(void *provider, int type)
{
    (void)provider;
    (void)type;
}

void *__stdcall _AIL_open_3D_listener(void *provider)
{
    struct MilesDriver *driver = provider ? ((struct MilesProvider *)provider)->driver : NULL;
    if (!driver)
        return NULL;
    platform_mutex_lock(driver->lock);
    object_reset(&driver->listener);
    platform_mutex_unlock(driver->lock);
    return &driver->listener;
}

void __stdcall _AIL_close_3D_listener(void *listener)
{
    struct MilesObject *object = listener;
    if (!object)
        return;
    platform_mutex_lock(object->driver->lock);
    object_reset(object);
    platform_mutex_unlock(object->driver->lock);
}

void *__stdcall _AIL_allocate_3D_sample_handle(void *provider)
{
    return provider ? sample_create(((struct MilesProvider *)provider)->driver, 1) : NULL;
}

void __stdcall _AIL_release_3D_sample_handle(void *sample)
{
    sample_release(sample);
}

int __stdcall _AIL_set_3D_sample_file(void *sample, const void *file)
{
    return sample_load(sample, file);
}

void __stdcall _AIL_set_3D_sample_effects_level(void *sample, float level)
{
    (void)sample;
    (void)level;
}

void __stdcall _AIL_set_3D_position(void *handle, float x, float y, float z)
{
    struct MilesObject *object = handle;
    if (!object)
        return;
    platform_mutex_lock(object->driver->lock);
    object->position[0] = x;
    object->position[1] = y;
    object->position[2] = z;
    platform_mutex_unlock(object->driver->lock);
}

void __stdcall _AIL_set_3D_orientation(void *handle, float face_x, float face_y, float face_z,
                                       float up_x, float up_y, float up_z)
{
    struct MilesObject *object = handle;
    if (!object)
        return;
    platform_mutex_lock(object->driver->lock);
    object->face[0] = face_x;
    object->face[1] = face_y;
    object->face[2] = face_z;
    object->up[0] = up_x;
    object->up[1] = up_y;
    object->up[2] = up_z;
    platform_mutex_unlock(object->driver->lock);
}

void __stdcall _AIL_set_3D_sample_volume(void *sample, int volume)
{
    sample_set_volume(sample, volume);
}

void __stdcall _AIL_set_3D_sample_distances(void *handle, float max_distance, float min_distance)
{
    struct MilesSample *sample = handle;
    if (!sample)
        return;
    platform_mutex_lock(sample->object.driver->lock);
    sample->max_distance = max_distance;
    sample->min_distance = min_distance;
    platform_mutex_unlock(sample->object.driver->lock);
}

void __stdcall _AIL_start_3D_sample(void *sample)
{
    sample_start(sample);
}

void __stdcall _AIL_end_3D_sample(void *sample)
{
    sample_end(sample);
}

unsigned __stdcall _AIL_3D_sample_status(void *sample)
{
    return sample_status(sample);
}

int __stdcall _AIL_file_type(const void *data, unsigned size)
{
    const unsigned char *bytes = data;
    struct WaveFormat wave;
    if (size >= 12 && !memcmp(bytes, "RIFF", 4) && !memcmp(bytes + 8, "WAVE", 4)) {
        if (get_u32(bytes + 4) > size - 8 || !wave_parse(bytes, &wave))
            return AIL_FILE_UNKNOWN;
        if (wave.format == WAVE_FORMAT_PCM)
            return AIL_FILE_PCM_WAV;
        if (wave.format == WAVE_FORMAT_IMA_ADPCM || wave.format == WAVE_FORMAT_MS_ADPCM)
            return AIL_FILE_ADPCM;
        return AIL_FILE_OTHER_WAV;
    }
    if (size >= 3 && !memcmp(bytes, "ID3", 3))
        return AIL_FILE_ASI;
    if (size >= 2 && bytes[0] == 0xff && (bytes[1] & 0xe6) == 0xe2)
        return AIL_FILE_ASI;
    return AIL_FILE_UNKNOWN;
}

int __stdcall _AIL_WAV_info(const void *data, struct AilSoundInfo *info)
{
    struct WaveFormat wave;
    if (!data || !info || !wave_parse(data, &wave))
        return 0;
    info->format = (int)wave.format;
    info->data = wave.data;
    info->dataLen = wave.length;
    info->rate = wave.rate;
    info->bits = wave.bits;
    info->channels = wave.channels;
    info->samples = wave_frames(&wave);
    info->blockSize = wave.block_size;
    info->initial = data;
    return 1;
}

int __stdcall _AIL_decompress_ADPCM(const struct AilSoundInfo *info, void **out, unsigned *outSize)
{
    const unsigned char *block, *end;
    unsigned char *image;
    int16_t *pcm;
    uint32_t done = 0;
    *out = NULL;
    *outSize = 0;
    if ((info->format != WAVE_FORMAT_IMA_ADPCM && info->format != WAVE_FORMAT_MS_ADPCM) || info->bits != 4 ||
        info->channels < 1 || info->channels > 2 || !info->blockSize || !info->data)
        return 0;
    image = wav_alloc(info->channels, info->samples);
    if (!image)
        return 0;
    pcm = (int16_t *)(image + WAV_HEADER_SIZE);
    block = info->data;
    end = block + info->dataLen;
    while (done < info->samples && block < end) {
        size_t size = (size_t)(end - block) < info->blockSize ? (size_t)(end - block) : info->blockSize;
        int16_t *at = pcm + (size_t)done * info->channels;
        uint32_t made = info->format == WAVE_FORMAT_IMA_ADPCM
                            ? ima_block(block, size, info->channels, at, info->samples - done)
                            : ms_block(block, size, info->channels, at, info->samples - done);
        if (!made)
            break;
        done += made;
        block += size;
    }
    if (!done) {
        free(image);
        return 0;
    }
    *outSize = wav_header(image, info->channels, info->rate, done);
    *out = image;
    return 1;
}

int __stdcall _AIL_decompress_ASI(const void *data, unsigned size, const char *name, void **out,
                                  unsigned *outSize, void *callback)
{
    drmp3_config config;
    drmp3_uint64 frames = 0;
    drmp3_int16 *pcm;
    unsigned char *image;
    (void)name;
    (void)callback;
    *out = NULL;
    *outSize = 0;
    pcm = drmp3_open_memory_and_read_pcm_frames_s16(data, size, &config, &frames, NULL);
    if (!pcm)
        return 0;
    image = wav_alloc((int)config.channels, frames);
    if (image) {
        memcpy(image + WAV_HEADER_SIZE, pcm, (size_t)frames * config.channels * sizeof(int16_t));
        *outSize = wav_header(image, (int)config.channels, config.sampleRate, (uint32_t)frames);
        *out = image;
    }
    drmp3_free(pcm, NULL);
    return image != NULL;
}

void __stdcall _AIL_mem_free_lock(void *p)
{
    free(p);
}
#endif
