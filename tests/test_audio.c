/* The audio module (platform/audio_sdl.c) on SDL's own decoders and mixer.

   test_audio FIXTURES [GAME]

   - Decoding: the ADPCM fixtures against their reference decodes, PCM WAVs byte for byte, the
     MP3 fixture as a tone; sample rates and lengths beyond the module's bounds are refused; every
     prefix of each file and thousands of damaged copies decode or fail without reading outside
     the file, and what decodes converts.
   - Mixing, through SDL's disk driver (its default 16-bit stereo, into files in the current
     folder): a voice plays its whole sound once, the music its loops, a gain scales, a stereo
     sound stays stereo, and with mono both channels carry the mix.
   - With GAME, a game folder with data.grf and BGM: real PCM, 8-bit, stereo and ADPCM sounds and
     an MP3 decode, and two effects with the music mix into output that is not silent and as long
     as the effects. Without it that part is skipped. */
#include "../src/platform/audio_sdl.c"
#include "fileio/grf.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define RATE 48000

static int g_rate;
#define MP3_FRAME 1152

static const char *g_fixtures;
static unsigned g_decoded, g_refused;

static unsigned char *read_file(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    unsigned char *bytes;
    long length;
    if (!file)
        return NULL;
    assert(!fseek(file, 0, SEEK_END));
    length = ftell(file);
    assert(length >= 0 && !fseek(file, 0, SEEK_SET));
    bytes = malloc((size_t)length + 1);
    assert(bytes && fread(bytes, 1, (size_t)length, file) == (size_t)length);
    fclose(file);
    *size = (size_t)length;
    return bytes;
}

static unsigned char *read_fixture(const char *name, size_t *size)
{
    char path[4096];
    unsigned char *bytes;
    snprintf(path, sizeof path, "%s/%s", g_fixtures, name);
    bytes = read_file(path, size);
    assert(bytes);
    return bytes;
}

static void put_u16(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
}

static void put_u32(unsigned char *p, uint32_t value)
{
    put_u16(p, value & 0xffff);
    put_u16(p + 2, value >> 16);
}

static uint32_t get_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* a PCM WAV with an odd LIST chunk between fmt and data */
static unsigned char *make_wav(int channels, unsigned rate, int bits, const void *pcm, uint32_t frames, size_t *size)
{
    uint32_t bytes = frames * (uint32_t)channels * (uint32_t)bits / 8;
    unsigned char *image = calloc(1, 56 + bytes + 1);
    assert(image);
    memcpy(image, "RIFF", 4);
    put_u32(image + 4, 48 + bytes);
    memcpy(image + 8, "WAVEfmt ", 8);
    put_u32(image + 16, 16);
    put_u16(image + 20, 1);
    put_u16(image + 22, (unsigned)channels);
    put_u32(image + 24, rate);
    put_u32(image + 28, rate * (uint32_t)channels * (uint32_t)bits / 8);
    put_u16(image + 32, (unsigned)(channels * bits / 8));
    put_u16(image + 34, (unsigned)bits);
    memcpy(image + 36, "LIST", 4);
    put_u32(image + 40, 3);
    memcpy(image + 44, "abc", 4);
    memcpy(image + 48, "data", 4);
    put_u32(image + 52, bytes);
    memcpy(image + 56, pcm, bytes);
    *size = 56 + bytes;
    return image;
}

static unsigned char *make_tone(int channels, unsigned rate, int16_t left, int16_t right, uint32_t frames, size_t *size)
{
    int16_t *pcm = malloc((size_t)frames * (size_t)channels * sizeof *pcm);
    unsigned char *image;
    uint32_t i;
    assert(pcm);
    for (i = 0; i < frames; ++i) {
        pcm[i * (uint32_t)channels] = left;
        if (channels == 2)
            pcm[i * 2 + 1] = right;
    }
    image = make_wav(channels, rate, 16, pcm, frames, size);
    free(pcm);
    return image;
}

/* The data chunk of a WAV, walked within the file. */
static const unsigned char *data_chunk(const unsigned char *file, size_t size, uint32_t *length)
{
    size_t at = 12;
    while (at + 8 <= size) {
        uint32_t chunk = get_u32(file + at + 4);
        if (!memcmp(file + at, "data", 4)) {
            *length = chunk <= size - at - 8 ? chunk : (uint32_t)(size - at - 8);
            return file + at + 8;
        }
        if (chunk > size - at - 8)
            break;
        at += 8 + (size_t)chunk + (chunk & 1);
    }
    return NULL;
}

static int frame_size(const struct AudioSound *sound)
{
    return SDL_AUDIO_FRAMESIZE(sound->spec);
}

/* What decodes must be whole frames of a format SDL converts; converting it must work. */
static void check_decoded(struct AudioSound *sound)
{
    SDL_AudioSpec out = { SDL_AUDIO_S16, 2, RATE };
    SDL_AudioStream *stream;
    static Uint8 sink[1 << 16];
    double seconds;
    if (!sound) {
        ++g_refused;
        return;
    }
    ++g_decoded;
    assert(sound->pcm && sound->size > 0 && sound->spec.channels >= 1 && sound->spec.freq >= 1);
    assert(frame_size(sound) > 0 && sound->size % frame_size(sound) == 0);
    seconds = (double)sound->size / frame_size(sound) / sound->spec.freq;
    stream = SDL_CreateAudioStream(&sound->spec, &out);
    if (!stream || seconds > 10.0) {
        SDL_DestroyAudioStream(stream);
        audio_sound_free(sound);
        return;
    }
    assert(SDL_PutAudioStreamData(stream, sound->pcm, sound->size) && SDL_FlushAudioStream(stream));
    while (SDL_GetAudioStreamData(stream, sink, sizeof sink) > 0) {
    }
    SDL_DestroyAudioStream(stream);
    audio_sound_free(sound);
}

/* Decodes a copy that ends where readable memory ends. */
static struct AudioSound *decode_guarded(const unsigned char *data, size_t size)
{
#ifdef _WIN32
    static unsigned char *pages;
    static size_t span;
    SYSTEM_INFO info;
    DWORD old;
    if (!pages) {
        GetSystemInfo(&info);
        span = (size_t)info.dwPageSize * 16;
        pages = VirtualAlloc(NULL, span + info.dwPageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        assert(pages && VirtualProtect(pages + span, info.dwPageSize, PAGE_NOACCESS, &old));
    }
    if (size <= span) {
        memcpy(pages + span - size, data, size);
        return audio_sound_decode(pages + span - size, size);
    }
#endif
    {
        unsigned char *copy = malloc(size ? size : 1);
        struct AudioSound *sound;
        assert(copy);
        memcpy(copy, data, size);
        sound = audio_sound_decode(copy, size);
        free(copy);
        return sound;
    }
}

static uint32_t g_seed = 0x9e3779b9u;
static uint32_t next_random(void)
{
    g_seed ^= g_seed << 13;
    g_seed ^= g_seed >> 17;
    g_seed ^= g_seed << 5;
    return g_seed;
}

/* Every prefix, then damaged copies: bytes flipped, a 32-bit field set to an edge value, cut. */
static void damage(const unsigned char *file, size_t size, unsigned rounds)
{
    static const uint32_t edges[] = { 0, 1, 2, 3, 0x7fffffff, 0x80000000u, 0xffffffffu, 0xfffffff8u, 0x10000 };
    unsigned char *copy = malloc(size);
    size_t n;
    unsigned round;
    assert(copy);
    for (n = 0; n <= size; ++n)
        check_decoded(decode_guarded(file, n));
    for (round = 0; round < rounds; ++round) {
        size_t length = size, at;
        unsigned k, changes = 1 + next_random() % 6;
        memcpy(copy, file, size);
        for (k = 0; k < changes; ++k) {
            uint32_t r = next_random();
            size_t head = size < 96 ? size : 96;
            at = (r >> 3) % ((r & 1) ? head : size);
            if (((r >> 1) & 3) == 0 && at + 4 <= size)
                put_u32(copy + at, edges[next_random() % (sizeof edges / sizeof *edges)]);
            else
                copy[at] ^= (unsigned char)(1u << (next_random() % 8));
        }
        if (next_random() % 4 == 0)
            length = next_random() % (size + 1);
        check_decoded(decode_guarded(copy, length));
    }
    free(copy);
}

static void check_adpcm(const char *wav_name, const char *pcm_name, int channels)
{
    size_t wav_size, pcm_size;
    unsigned char *wav = read_fixture(wav_name, &wav_size), *reference = read_fixture(pcm_name, &pcm_size);
    struct AudioSound *sound = audio_sound_decode(wav, wav_size);
    assert(sound && sound->spec.format == SDL_AUDIO_S16 && sound->spec.channels == channels);
    assert(sound->spec.freq == 22050);
    assert((size_t)sound->size == pcm_size && !memcmp(sound->pcm, reference, pcm_size));
    audio_sound_free(sound);
    damage(wav, wav_size, 1500);
    free(wav);
    free(reference);
}

static void check_tone(const int16_t *pcm, int channels, int from, int to, double hz, double rate,
                       double amplitude)
{
    double a = 0, b = 0, energy = 0, residual = 0, found;
    int n, count = to - from;
    for (n = from; n < to; ++n) {
        double phase = 2 * 3.14159265358979323846 * hz * n / rate;
        a += pcm[n * channels] * cos(phase);
        b += pcm[n * channels] * sin(phase);
    }
    a *= 2.0 / count;
    b *= 2.0 / count;
    found = sqrt(a * a + b * b);
    assert(fabs(found - amplitude) < amplitude * 0.1);
    for (n = from; n < to; ++n) {
        double phase = 2 * 3.14159265358979323846 * hz * n / rate;
        double error = pcm[n * channels] - (a * cos(phase) + b * sin(phase));
        energy += (double)pcm[n * channels] * pcm[n * channels];
        residual += error * error;
    }
    assert(residual < energy * 0.05);
}

static int g_mp3_frames;

/* A sound decodes at 1,000 to 384,000 Hz and up to five minutes long: a damaged rate of 1 Hz, which
   SDL would play, would hold a voice for hours. */
static int bounded(unsigned rate, uint32_t frames)
{
    static unsigned char samples[300 * 1000 + 1];
    size_t size;
    unsigned char *image;
    struct AudioSound *sound;
    assert(frames <= sizeof samples);
    image = make_wav(1, rate, 8, samples, frames, &size);
    sound = audio_sound_decode(image, size);
    free(image);
    if (!sound)
        return 0;
    assert(sound->spec.freq == (int)rate && sound->size == (int)frames);
    audio_sound_free(sound);
    return 1;
}

static void test_bounds(void)
{
    assert(!bounded(1, 100) && !bounded(999, 100) && bounded(1000, 100));
    assert(bounded(384000, 100) && !bounded(384001, 100) && !bounded(0xffffffffu, 100));
    assert(bounded(1000, 300 * 1000) && !bounded(1000, 300 * 1000 + 1));
}

static void test_decoding(void)
{
    static const int16_t pcm16[4] = { 1, -2, 3, -4 };
    static const unsigned char pcm8[3] = { 0x80, 0xff, 0x00 };
    unsigned char frame[4] = { 0xff, 0xfb, 0x90, 0x00 }, garbage[16] = "not a sound";
    unsigned char *image, *mp3;
    size_t size, mp3_size;
    struct AudioSound *sound;

    image = make_wav(2, 22050, 16, pcm16, 2, &size);
    sound = audio_sound_decode(image, size);
    assert(sound && sound->spec.format == SDL_AUDIO_S16LE && sound->spec.channels == 2);
    assert(sound->spec.freq == 22050 && sound->size == 8 && !memcmp(sound->pcm, pcm16, 8));
    audio_sound_free(sound);
    damage(image, size, 1500);
    /* the RIFF size counting the whole file, or nothing; the data chunk running past the file */
    put_u32(image + 4, (uint32_t)size);
    sound = audio_sound_decode(image, size);
    assert(sound && sound->size == 8);
    audio_sound_free(sound);
    put_u32(image + 4, 0xffffffffu);
    sound = audio_sound_decode(image, size);
    assert(sound && sound->size == 8);
    audio_sound_free(sound);
    put_u32(image + 4, 48 + 8);
    put_u32(image + 52, 0xffffffffu);
    sound = audio_sound_decode(image, size);
    assert(sound && sound->size == 8);
    audio_sound_free(sound);
    /* an empty data chunk, an MPEG Layer 3 format tag, no channels: nothing to play */
    put_u32(image + 52, 0);
    assert(!audio_sound_decode(image, size));
    put_u32(image + 52, 8);
    put_u16(image + 20, 0x55);
    assert(!audio_sound_decode(image, size));
    put_u16(image + 20, 1);
    put_u16(image + 22, 0);
    assert(!audio_sound_decode(image, size));
    free(image);
    test_bounds();

    image = make_wav(1, 11025, 8, pcm8, 3, &size);
    sound = audio_sound_decode(image, size);
    assert(sound && sound->spec.format == SDL_AUDIO_U8 && sound->spec.channels == 1);
    assert(sound->spec.freq == 11025 && sound->size == 3 && !memcmp(sound->pcm, pcm8, 3));
    audio_sound_free(sound);
    damage(image, size, 1500);
    free(image);

    assert(!audio_sound_decode(garbage, sizeof garbage) && !audio_sound_decode(NULL, 0));
    assert(!audio_sound_decode(frame, sizeof frame) && !audio_sound_decode(garbage, 0));

    check_adpcm("audio_ms_adpcm_mono.wav", "audio_ms_adpcm_mono.pcm", 1);
    check_adpcm("audio_ms_adpcm_stereo.wav", "audio_ms_adpcm_stereo.pcm", 2);
    check_adpcm("audio_ima_adpcm_mono.wav", "audio_ima_adpcm_mono.pcm", 1);
    check_adpcm("audio_ima_adpcm_stereo.wav", "audio_ima_adpcm_stereo.pcm", 2);

    mp3 = read_fixture("audio_sine.mp3", &mp3_size);
    sound = audio_sound_decode(mp3, mp3_size);
    assert(sound && sound->spec.format == SDL_AUDIO_S16 && sound->spec.channels == 1 && sound->spec.freq == 22050);
    g_mp3_frames = sound->size / 2;
    assert(abs(g_mp3_frames - 6615) <= MP3_FRAME);
    check_tone((const int16_t *)sound->pcm, 1, MP3_FRAME, g_mp3_frames - MP3_FRAME, 440, 22050, 32767 * 0.75);
    audio_sound_free(sound);
    /* a raw MP3 frame stream without its ID3 tag is MP3 too */
    size = 10 + ((size_t)mp3[6] << 21 | (size_t)mp3[7] << 14 | (size_t)mp3[8] << 7 | mp3[9]);
    assert(size < mp3_size);
    sound = audio_sound_decode(mp3 + size, mp3_size - size);
    assert(sound && sound->size / 2 > g_mp3_frames - MP3_FRAME);
    audio_sound_free(sound);
    damage(mp3, mp3_size, 400);
    free(mp3);
    printf("audio: decoding passed (%u damaged or cut files decoded and converted, %u refused)\n", g_decoded, g_refused);
}

/* ---- mixing through the disk driver ---- */

struct Output {
    int16_t *pcm;
    long frames;
};

static void open_device(const char *file, int mono)
{
    remove(file);
    SDL_AudioSpec spec;
    SDL_SetHint(SDL_HINT_AUDIO_DISK_OUTPUT_FILE, file);
    assert(audio_open(mono));
    assert(SDL_GetAudioDeviceFormat(g_device, &spec, NULL));
    assert(spec.format == SDL_AUDIO_S16 && spec.channels == 2 && spec.freq >= 22050);
    g_rate = spec.freq;
}

static struct Output close_device(const char *file)
{
    struct Output out;
    size_t size;
    audio_close();
    out.pcm = (int16_t *)read_file(file, &size);
    assert(out.pcm);
    out.frames = (long)(size / 4);
    return out;
}

static void wait_voice(struct AudioVoice *voice, Uint32 limit_ms)
{
    Uint64 start = SDL_GetTicks();
    if (!voice)
        fprintf(stderr, "no voice: %s", SDL_GetError());
    assert(voice);
    while (audio_voice_playing(voice)) {
        assert(SDL_GetTicks() - start < limit_ms);
        SDL_Delay(2);
    }
}

/* The sounding stretches of the output: frames louder than threshold, gaps under 0.1 s joined. */
struct Span {
    long first, last;
};

static int spans(const struct Output *out, int threshold, struct Span *found, int capacity)
{
    long i, gap = g_rate / 10;
    int count = 0;
    for (i = 0; i < out->frames; ++i) {
        if (abs(out->pcm[2 * i]) <= threshold && abs(out->pcm[2 * i + 1]) <= threshold)
            continue;
        if (count && i - found[count - 1].last <= gap) {
            found[count - 1].last = i;
        } else {
            assert(count < capacity);
            found[count].first = found[count].last = i;
            ++count;
        }
    }
    return count;
}

/* the frames of a mono 16-bit sound from its first to its last louder than threshold */
static long sounding(const struct AudioSound *sound, int threshold)
{
    const int16_t *pcm = (const int16_t *)sound->pcm;
    long frames = sound->size / 2, first = 0, last = frames - 1;
    while (first < frames && abs(pcm[first]) <= threshold)
        ++first;
    while (last > first && abs(pcm[last]) <= threshold)
        --last;
    return last - first;
}

static double mean(const struct Output *out, const struct Span *span, int channel)
{
    long length = span->last - span->first, i, from = span->first + length / 4, to = span->last - length / 4;
    double sum = 0;
    for (i = from; i < to; ++i)
        sum += out->pcm[2 * i + channel];
    return sum / (double)(to - from);
}

static void test_mixing(void)
{
    size_t size, tone_size;
    unsigned char *wav = read_fixture("audio_ima_adpcm_mono.wav", &size);
    unsigned char *tone = make_tone(2, 22050, 8000, 0, 5512, &tone_size);
    struct AudioSound *sound = audio_sound_decode(wav, size), *stereo = audio_sound_decode(tone, tone_size);
    struct AudioMusic *music;
    struct Output out;
    struct Span found[8];
    char path[4096];
    long expected;
    Uint64 start;
    assert(sound && stereo);

    /* one voice, then the music twice, with silence around them */
    open_device("audio_play.raw", 0);
    SDL_Delay(150);
    wait_voice(audio_voice_play(sound, 1.0f), 5000);
    SDL_Delay(250);
    snprintf(path, sizeof path, "%s/audio_sine.mp3", g_fixtures);
    assert(!audio_music_play("missing\\nothing.mp3", 1, 1.0f));
    music = audio_music_play(path, 2, 1.0f);
    assert(music && !audio_music_done(music));
    for (start = SDL_GetTicks(); !audio_music_done(music); SDL_Delay(2))
        assert(SDL_GetTicks() - start < 5000);
    audio_music_stop(music);
    SDL_Delay(150);
    out = close_device("audio_play.raw");
    assert(spans(&out, 64, found, 8) == 2);
    expected = (long)(sounding(sound, 64) * (double)g_rate / 22050);
    assert(labs((found[0].last - found[0].first) - expected) < 300);
    expected = (long)(2.0 * g_mp3_frames * g_rate / 22050);
    assert(labs((found[1].last - found[1].first) - expected) < (long)((2 * MP3_FRAME + 300) * (double)g_rate / 22050));
    assert(found[1].first - found[0].last > g_rate / 5);
    free(out.pcm);

    /* a stereo sound stays as it is, a gain scales it, and mono mixes both channels into each */
    open_device("audio_stereo.raw", 0);
    SDL_Delay(150);
    wait_voice(audio_voice_play(stereo, 1.0f), 5000);
    SDL_Delay(200);
    wait_voice(audio_voice_play(stereo, 0.5f), 5000);
    SDL_Delay(150);
    out = close_device("audio_stereo.raw");
    assert(spans(&out, 64, found, 8) == 2);
    assert(fabs(mean(&out, &found[0], 0) - 8000) < 80 && fabs(mean(&out, &found[0], 1)) < 40);
    assert(fabs(mean(&out, &found[1], 0) - 4000) < 40 && fabs(mean(&out, &found[1], 1)) < 40);
    expected = (long)(5512.0 * g_rate / 22050);
    assert(labs((found[0].last - found[0].first) - expected) < 300);
    free(out.pcm);
    open_device("audio_mono.raw", 1);
    SDL_Delay(150);
    wait_voice(audio_voice_play(stereo, 1.0f), 5000);
    SDL_Delay(150);
    out = close_device("audio_mono.raw");
    assert(spans(&out, 64, found, 8) == 1);
    assert(fabs(mean(&out, &found[0], 0) - 4000) < 40 && fabs(mean(&out, &found[0], 1) - 4000) < 40);
    free(out.pcm);

    /* nothing plays on a closed device */
    assert(!audio_voice_play(sound, 1.0f) && !audio_music_play(path, 1, 1.0f));
    audio_sound_free(sound);
    audio_sound_free(stereo);
    free(wav);
    free(tone);
    puts("audio: mixing passed (a voice plays its sound once, music loops, gain, stereo and mono)");
}

/* ---- the game's own sounds ---- */

static struct AudioSound *grf_sound(struct GrfArchive *grf, const char *name, int format, int channels, int rate)
{
    struct GrfEntry entry;
    struct Hash key;
    unsigned char *file;
    struct AudioSound *sound;
    const unsigned char *data;
    uint32_t length = 0;
    hash_construct(&entry.key, "");
    hash_construct(&key, name);
    assert(grf_archive_find(grf, &key, &entry));
    file = malloc(entry.size);
    assert(file && grf_archive_read(grf, &entry, file, entry.size));
    sound = audio_sound_decode(file, entry.size);
    assert(sound && sound->spec.channels == channels && sound->spec.freq == rate);
    data = data_chunk(file, entry.size, &length);
    assert(data);
    if (format == 1) {
        /* PCM is the data chunk as it is */
        assert((uint32_t)sound->size == length - length % (uint32_t)frame_size(sound));
        assert(!memcmp(sound->pcm, data, (size_t)sound->size));
    } else {
        /* ADPCM: 16-bit, every whole block and what a last block cut short holds */
        const unsigned char *fmt = file + 20;
        unsigned block = (unsigned)fmt[12] | (unsigned)fmt[13] << 8, header = format == 2 ? 7u : 4u;
        uint32_t per_block = format == 2 ? 2 + (block - header * (unsigned)channels) * 2 / (unsigned)channels
                                         : 1 + (block - header * (unsigned)channels) * 2 / (unsigned)channels;
        uint32_t frames = (uint32_t)sound->size / ((uint32_t)channels * 2), whole = length / block * per_block;
        assert(sound->spec.format == SDL_AUDIO_S16);
        assert(frames >= whole && frames < whole + per_block && (length % block != 0) == (frames > whole));
    }
    free(file);
    return sound;
}

static double rms(const struct Output *out)
{
    double sum = 0;
    long i;
    for (i = 0; i < out->frames * 2; ++i)
        sum += (double)out->pcm[i] * out->pcm[i];
    return out->frames ? sqrt(sum / (double)(out->frames * 2)) : 0;
}

static void test_game(const char *game)
{
    char path[4096];
    unsigned char *bgm;
    size_t bgm_size;
    struct GrfArchive *grf;
    struct AudioSound *welcome, *button, *sound;
    struct AudioVoice *a, *b;
    struct AudioMusic *music;
    struct Output out;
    struct Span found[64];
    double longest;
    int peak = 0;
    long i;
    Uint64 start, took;

    snprintf(path, sizeof path, "%s/data.grf", game);
    grf = grf_archive_create(path);
    assert(grf);
    button = grf_sound(grf, "data\\wav\\plasma_damage.wav", 1, 1, 22050);
    welcome = grf_sound(grf, "data\\wav\\welcome.wav", 17, 1, 44100);
    audio_sound_free(grf_sound(grf, "data\\wav\\glass.wav", 17, 2, 44100));
    audio_sound_free(grf_sound(grf, "data\\wav\\se_frog02.wav", 2, 1, 22050));
    sound = grf_sound(grf, "data\\wav\\dying.wav", 1, 1, 22050);
    assert(sound->spec.format == SDL_AUDIO_U8);
    audio_sound_free(sound);
    sound = grf_sound(grf, "data\\wav\\CHUNITHM_std.wav", 1, 2, 48000);
    assert(sound->spec.format == SDL_AUDIO_S16LE);
    audio_sound_free(sound);
    grf_archive_destroy(grf);
    snprintf(path, sizeof path, "%s/BGM/01.mp3", game);
    bgm = read_file(path, &bgm_size);
    assert(bgm);
    sound = audio_sound_decode(bgm, bgm_size);
    /* 01.mp3 is MPEG-1 Layer III, 48 kHz joint stereo, over a minute long */
    assert(sound && sound->spec.format == SDL_AUDIO_S16 && sound->spec.channels == 2 && sound->spec.freq == 48000);
    assert(sound->size / 4 > 48000 * 60);
    audio_sound_free(sound);
    free(bgm);

    /* two effects and the music at once */
    longest = (double)welcome->size / 2 / 44100;
    if ((double)button->size / 2 / 22050 > longest)
        longest = (double)button->size / 2 / 22050;
    open_device("audio_game.raw", 0);
    music = audio_music_play(path, 0, 0.5f);
    assert(music);
    start = SDL_GetTicks();
    a = audio_voice_play(welcome, 1.0f);
    b = audio_voice_play(button, 1.0f);
    assert(a && b);
    while (audio_voice_playing(a) || audio_voice_playing(b))
        SDL_Delay(2);
    took = SDL_GetTicks() - start;
    assert(took >= (Uint64)(longest * 900) && took < (Uint64)(longest * 1000) + 3000);
    SDL_Delay(300);
    assert(!audio_music_done(music));
    audio_voice_stop(a);
    audio_voice_stop(b);
    audio_music_stop(music);
    out = close_device("audio_game.raw");
    assert(out.frames >= (long)(longest * g_rate));
    for (i = 0; i < out.frames * 2; ++i)
        if (abs(out.pcm[i]) > peak)
            peak = abs(out.pcm[i]);
    assert(peak > 8000 && rms(&out) > 500);
    assert(spans(&out, 64, found, 64) >= 1 && found[0].last - found[0].first >= (long)(longest * g_rate));
    printf("audio: game sounds passed (PCM, 8-bit, stereo, IMA and MS ADPCM from data.grf, BGM\\01.mp3; "
           "two effects and the music mixed %ld frames, %.2f s of effects in %llu ms, peak %d, RMS %.0f)\n",
           out.frames, longest, (unsigned long long)took, peak, rms(&out));
    free(out.pcm);
    audio_sound_free(welcome);
    audio_sound_free(button);
}

int main(int argc, char **argv)
{
    char probe[4096];
    FILE *file;
    assert(argc == 2 || argc == 3);
    setvbuf(stdout, NULL, _IONBF, 0);
    g_fixtures = argv[1];
    /* Decoding needs no device and no audio subsystem, as when the client has sound off. */
    test_decoding();
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "disk");
    test_mixing();
    if (argc == 3) {
        snprintf(probe, sizeof probe, "%s/data.grf", argv[2]);
        file = fopen(probe, "rb");
        if (file) {
            fclose(file);
            test_game(argv[2]);
        } else {
            printf("audio: game sounds skipped: no data.grf in \"%s\"\n", argv[2]);
        }
    } else {
        puts("audio: game sounds skipped: no game folder given");
    }
    puts("audio: ok");
    return 0;
}
