#include "platform/miles_posix.c"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#define SINE_FRAMES 6615
#define MP3_FRAME 1152

static const char *g_fixtures;

static unsigned char *read_fixture(const char *name, size_t *size)
{
    char path[4096];
    unsigned char *bytes;
    FILE *file;
    long length;
    snprintf(path, sizeof path, "%s/%s", g_fixtures, name);
    file = fopen(path, "rb");
    assert(file);
    assert(!fseek(file, 0, SEEK_END));
    length = ftell(file);
    assert(length > 0);
    assert(!fseek(file, 0, SEEK_SET));
    bytes = malloc((size_t)length);
    assert(bytes && fread(bytes, 1, (size_t)length, file) == (size_t)length);
    fclose(file);
    *size = (size_t)length;
    return bytes;
}

static unsigned char *make_wav(int channels, unsigned rate, int bits, const void *pcm, uint32_t frames)
{
    uint32_t bytes = frames * (uint32_t)channels * (uint32_t)bits / 8;
    unsigned char *image = calloc(1, 12 + 24 + 12 + 8 + bytes + 1);
    assert(image);
    memcpy(image, "RIFF", 4);
    put_u32(image + 4, 4 + 24 + 12 + 8 + bytes);
    memcpy(image + 8, "WAVEfmt ", 8);
    put_u32(image + 16, 16);
    put_u16(image + 20, WAVE_FORMAT_PCM);
    put_u16(image + 22, (uint32_t)channels);
    put_u32(image + 24, rate);
    put_u32(image + 28, rate * (uint32_t)channels * (uint32_t)bits / 8);
    put_u16(image + 32, (uint32_t)channels * (uint32_t)bits / 8);
    put_u16(image + 34, (uint32_t)bits);
    memcpy(image + 36, "LIST", 4);
    put_u32(image + 40, 3);
    memcpy(image + 44, "abc", 4);
    memcpy(image + 48, "data", 4);
    put_u32(image + 52, bytes);
    memcpy(image + 56, pcm, bytes);
    return image;
}

static unsigned char *make_tone(int channels, unsigned rate, int16_t left, int16_t right, uint32_t frames)
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
    image = make_wav(channels, rate, 16, pcm, frames);
    free(pcm);
    return image;
}

static void expect_frames(const int16_t *out, int channels, int from, int to, int left, int right)
{
    int i;
    for (i = from; i < to; ++i) {
        assert(out[i * channels] == left);
        if (channels == 2)
            assert(out[i * channels + 1] == right);
    }
}

static int sounding_frames(const int16_t *out, int channels, int frames)
{
    int i;
    for (i = frames; i > 0; --i)
        if (out[(i - 1) * channels] || (channels == 2 && out[(i - 1) * channels + 1]))
            break;
    return i;
}

static void test_wave_parsing(void)
{
    static const int16_t pcm[4] = { 1, -2, 3, -4 };
    unsigned char *image = make_wav(2, 22050, 16, pcm, 2), garbage[16] = "not a sound";
    unsigned char frame[4] = { 0xff, 0xfb, 0x90, 0x00 };
    struct AilSoundInfo info = {0};
    struct WaveFormat wave;
    void *out = &info;
    unsigned out_size = 1;
    assert(wave_parse(image, &wave));
    assert(wave.format == WAVE_FORMAT_PCM && wave.channels == 2 && wave.rate == 22050 && wave.bits == 16);
    assert(wave.block_size == 4 && wave.length == 8 && !memcmp(wave.data, pcm, 8));
    assert(_AIL_file_type(image, 64) == FILE_TYPE_PCM_WAV);
    assert(_AIL_file_type(image, 40) == FILE_TYPE_UNKNOWN);
    assert(_AIL_WAV_info(image, &info) && info.samples == 2 && info.dataLen == 8 && info.initial == image);
    assert(!_AIL_decompress_ADPCM(&info, &out, &out_size) && !out && !out_size);
    put_u16(image + 20, 0x55);
    assert(_AIL_file_type(image, 64) == FILE_TYPE_OTHER_WAV);
    put_u32(image + 52, 9);
    assert(!wave_parse(image, &wave));
    free(image);
    assert(_AIL_file_type(garbage, sizeof garbage) == FILE_TYPE_UNKNOWN);
    assert(_AIL_file_type(frame, sizeof frame) == AIL_FILE_ASI);
    frame[1] = 0xfd;
    assert(_AIL_file_type(frame, sizeof frame) == FILE_TYPE_UNKNOWN);
    assert(!_AIL_WAV_info(garbage, &info));
}

static void check_adpcm(const char *wav_name, const char *pcm_name, int format, int channels, uint32_t blocks)
{
    size_t wav_size, pcm_size, cut;
    unsigned char *wav = read_fixture(wav_name, &wav_size), *reference = read_fixture(pcm_name, &pcm_size);
    struct AilSoundInfo info = {0};
    struct WaveFormat out;
    unsigned decoded_size;
    void *decoded;
    uint32_t per_block, partial;
    assert(_AIL_file_type(wav, (unsigned)wav_size) == AIL_FILE_ADPCM);
    assert(_AIL_WAV_info(wav, &info));
    assert(info.format == format && info.channels == channels && info.rate == 22050 && info.bits == 4);
    assert(info.blockSize == 1024 && info.dataLen == blocks * 1024 && info.initial == wav);
    assert((size_t)info.samples * (size_t)channels * 2 == pcm_size);
    per_block = info.samples / blocks;
    assert(_AIL_decompress_ADPCM(&info, &decoded, &decoded_size));
    assert(decoded_size == WAV_HEADER_SIZE + pcm_size);
    assert(_AIL_file_type(decoded, decoded_size) == FILE_TYPE_PCM_WAV);
    assert(wave_parse(decoded, &out));
    assert(out.format == WAVE_FORMAT_PCM && out.channels == channels && out.rate == 22050 && out.bits == 16);
    assert(out.length == pcm_size && !memcmp(out.data, reference, pcm_size));
    _AIL_mem_free_lock(decoded);

    cut = 1024 / 2 + 16;
    put_u32(wav + 4, get_u32(wav + 4) - (uint32_t)cut);
    put_u32((unsigned char *)info.data - 4, info.dataLen - (uint32_t)cut);
    partial = adpcm_block_frames((unsigned)format, channels, 1024 - cut);
    assert(partial > 2 && partial < per_block);
    assert(_AIL_WAV_info(wav, &info) && info.samples == (blocks - 1) * per_block + partial);
    assert(_AIL_decompress_ADPCM(&info, &decoded, &decoded_size));
    assert(wave_parse(decoded, &out) && out.length == info.samples * (uint32_t)channels * 2);
    assert(!memcmp(out.data, reference, out.length));
    _AIL_mem_free_lock(decoded);

    info.samples = 3;
    assert(_AIL_decompress_ADPCM(&info, &decoded, &decoded_size));
    assert(decoded_size == WAV_HEADER_SIZE + 3 * (unsigned)channels * 2);
    assert(!memcmp((unsigned char *)decoded + WAV_HEADER_SIZE, reference, 3 * (size_t)channels * 2));
    _AIL_mem_free_lock(decoded);
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

static void test_mp3_decode(void)
{
    size_t size, offset;
    unsigned char *mp3 = read_fixture("audio_sine.mp3", &size);
    struct WaveFormat wave;
    unsigned decoded_size;
    void *decoded;
    int frames;
    assert(_AIL_file_type(mp3, (unsigned)size) == AIL_FILE_ASI);
    offset = 10 + ((size_t)mp3[6] << 21 | (size_t)mp3[7] << 14 | (size_t)mp3[8] << 7 | mp3[9]);
    assert(offset < size && _AIL_file_type(mp3 + offset, (unsigned)(size - offset)) == AIL_FILE_ASI);
    assert(_AIL_decompress_ASI(mp3, (unsigned)size, "audio_sine.mp3", &decoded, &decoded_size, NULL));
    assert(wave_parse(decoded, &wave) && decoded_size == WAV_HEADER_SIZE + wave.length);
    assert(wave.format == WAVE_FORMAT_PCM && wave.channels == 1 && wave.rate == 22050 && wave.bits == 16);
    frames = (int)(wave.length / 2);
    assert(abs(frames - SINE_FRAMES) <= MP3_FRAME);
    check_tone((const int16_t *)wave.data, 1, MP3_FRAME, frames - MP3_FRAME, 440, 22050, 32767 * 0.75);
    _AIL_mem_free_lock(decoded);
    assert(!_AIL_decompress_ASI(mp3 + size / 2, 16, "cut.mp3", &decoded, &decoded_size, NULL));
    assert(!decoded && !decoded_size);
    free(mp3);
}

static void test_samples(void)
{
    struct MilesDriver *driver = driver_create(22050, 2), *mono;
    unsigned char *tone = make_tone(1, 22050, 10000, 0, 1000), *loud = make_tone(1, 22050, 30000, 0, 1000);
    unsigned char *stereo = make_tone(2, 22050, 1000, -1000, 100), *slow = make_tone(1, 11025, 8000, 0, 500);
    unsigned char *adpcm, u8[100];
    int16_t out[2400 * 2];
    void *a, *b;
    size_t size;
    assert(driver);
    a = _AIL_allocate_sample_handle(driver);
    b = _AIL_allocate_sample_handle(driver);
    assert(a && b && _AIL_sample_status(a) == MSS_SAMPLE_DONE);
    _AIL_start_sample(a);
    assert(_AIL_sample_status(a) == MSS_SAMPLE_DONE);
    driver_mix(driver, out, 16);
    expect_frames(out, 2, 0, 16, 0, 0);

    _AIL_init_sample(a);
    assert(_AIL_set_sample_file(a, tone, 0));
    _AIL_set_sample_volume(a, 127);
    _AIL_start_sample(a);
    assert(_AIL_sample_status(a) == MSS_STREAM_PLAYING);
    driver_mix(driver, out, 600);
    expect_frames(out, 2, 0, 600, 10000, 10000);
    assert(_AIL_sample_status(a) == MSS_STREAM_PLAYING);
    driver_mix(driver, out, 600);
    expect_frames(out, 2, 0, 400, 10000, 10000);
    expect_frames(out, 2, 400, 600, 0, 0);
    assert(_AIL_sample_status(a) == MSS_SAMPLE_DONE);

    _AIL_set_sample_volume(a, 64);
    _AIL_start_sample(a);
    driver_mix(driver, out, 10);
    expect_frames(out, 2, 0, 10, 5039, 5039);
    _AIL_set_sample_volume(a, 500);
    driver_mix(driver, out, 10);
    expect_frames(out, 2, 0, 10, 10000, 10000);
    _AIL_set_sample_volume(a, -5);
    driver_mix(driver, out, 10);
    expect_frames(out, 2, 0, 10, 0, 0);
    _AIL_set_sample_volume(a, 127);
    _AIL_end_sample(a);
    assert(_AIL_sample_status(a) == MSS_SAMPLE_DONE);
    driver_mix(driver, out, 10);
    expect_frames(out, 2, 0, 10, 0, 0);

    _AIL_start_sample(a);
    assert(_AIL_set_sample_file(b, loud, 0));
    _AIL_start_sample(b);
    driver_mix(driver, out, 10);
    expect_frames(out, 2, 0, 10, 32767, 32767);
    _AIL_end_sample(a);
    _AIL_end_sample(b);

    assert(_AIL_set_sample_file(a, stereo, 0));
    _AIL_start_sample(a);
    driver_mix(driver, out, 120);
    expect_frames(out, 2, 0, 100, 1000, -1000);
    expect_frames(out, 2, 100, 120, 0, 0);

    memset(u8, 0xc0, sizeof u8);
    free(stereo);
    stereo = make_wav(1, 22050, 8, u8, sizeof u8);
    assert(_AIL_set_sample_file(a, stereo, 0));
    _AIL_start_sample(a);
    driver_mix(driver, out, 110);
    expect_frames(out, 2, 0, 100, 16384, 16384);
    expect_frames(out, 2, 100, 110, 0, 0);

    assert(_AIL_set_sample_file(a, slow, 0));
    _AIL_start_sample(a);
    driver_mix(driver, out, 1200);
    assert(abs(sounding_frames(out, 2, 1200) - 1000) <= 16);
    assert(out[400 * 2] > 7900 && out[400 * 2] < 8100);
    assert(_AIL_sample_status(a) == MSS_SAMPLE_DONE);

    adpcm = read_fixture("audio_ms_adpcm_mono.wav", &size);
    assert(!_AIL_set_sample_file(a, adpcm, 0));
    assert(!_AIL_set_sample_file(a, NULL, 0));
    _AIL_start_sample(a);
    assert(_AIL_sample_status(a) == MSS_SAMPLE_DONE);
    free(adpcm);

    _AIL_release_sample_handle(a);
    _AIL_release_sample_handle(b);
    assert(!driver->samples);
    driver_destroy(driver);
    assert(!g_provider.driver);

    mono = driver_create(22050, 1);
    a = _AIL_allocate_sample_handle(mono);
    free(stereo);
    stereo = make_tone(2, 22050, 1000, 3000, 50);
    assert(_AIL_set_sample_file(a, stereo, 0));
    _AIL_start_sample(a);
    driver_mix(mono, out, 60);
    expect_frames(out, 1, 0, 50, 2000, 0);
    expect_frames(out, 1, 50, 60, 0, 0);
    driver_destroy(mono);

    free(tone);
    free(loud);
    free(stereo);
    free(slow);
}

static void play_3d(struct MilesDriver *driver, void *sample, float x, float y, float z, int left, int right)
{
    int16_t out[64 * 2];
    _AIL_set_3D_position(sample, x, y, z);
    _AIL_start_3D_sample(sample);
    driver_mix(driver, out, 64);
    expect_frames(out, 2, 0, 64, left, right);
    _AIL_end_3D_sample(sample);
}

static void test_positional(void)
{
    struct MilesDriver *driver = driver_create(22050, 2);
    unsigned char *tone = make_tone(1, 22050, 10000, 0, 200), *stereo = make_tone(2, 22050, 4000, 8000, 200);
    void *next = NULL, *provider = NULL, *listener, *sample;
    char *name = NULL;
    int16_t out[300 * 2];
    assert(_AIL_enumerate_3D_providers(&next, &provider, &name));
    assert(provider && !strcmp(name, "Miles Fast 2D Positional Audio"));
    assert(!_AIL_enumerate_3D_providers(&next, &provider, &name));
    assert(_AIL_open_3D_provider(provider) == PROVIDER_OK);
    assert(_AIL_3D_speaker_type(provider) == -1 && _AIL_3D_room_type(provider) == -1);
    listener = _AIL_open_3D_listener(provider);
    sample = _AIL_allocate_3D_sample_handle(provider);
    assert(listener && sample && _AIL_3D_sample_status(sample) == MSS_SAMPLE_DONE);
    assert(_AIL_set_3D_sample_file(sample, tone));
    _AIL_set_3D_sample_effects_level(sample, 0.0f);
    _AIL_set_3D_sample_volume(sample, 127);
    _AIL_set_3D_sample_distances(sample, 250.0f, 40.0f);
    _AIL_set_3D_orientation(sample, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);

    play_3d(driver, sample, 0.0f, 0.0f, 0.0f, 10000, 10000);
    play_3d(driver, sample, 0.0f, 0.0f, 20.0f, 10000, 10000);
    play_3d(driver, sample, 0.0f, 0.0f, 80.0f, 5000, 5000);
    play_3d(driver, sample, 0.0f, -100.0f, 0.0f, 4000, 4000);
    play_3d(driver, sample, 300.0f, 0.0f, 0.0f, 0, 0);
    play_3d(driver, sample, 40.0f, 0.0f, 0.0f, 0, 10000);
    play_3d(driver, sample, -40.0f, 0.0f, 0.0f, 10000, 0);
    play_3d(driver, sample, 20.0f, 0.0f, 20.0f, 2929, 10000);

    _AIL_set_3D_orientation(listener, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    play_3d(driver, sample, 40.0f, 0.0f, 0.0f, 10000, 10000);
    play_3d(driver, sample, 0.0f, 0.0f, 40.0f, 10000, 0);
    _AIL_set_3D_position(listener, 0.0f, 0.0f, 40.0f);
    play_3d(driver, sample, 0.0f, 0.0f, 40.0f, 10000, 10000);
    _AIL_close_3D_listener(listener);
    play_3d(driver, sample, 40.0f, 0.0f, 0.0f, 0, 10000);

    _AIL_set_3D_sample_volume(sample, 64);
    play_3d(driver, sample, 0.0f, 0.0f, 0.0f, 5039, 5039);
    _AIL_set_3D_sample_volume(sample, 127);

    _AIL_set_3D_position(sample, 300.0f, 0.0f, 0.0f);
    _AIL_start_3D_sample(sample);
    assert(_AIL_3D_sample_status(sample) == MSS_STREAM_PLAYING);
    driver_mix(driver, out, 300);
    expect_frames(out, 2, 0, 300, 0, 0);
    assert(_AIL_3D_sample_status(sample) == MSS_SAMPLE_DONE);

    assert(_AIL_set_3D_sample_file(sample, stereo));
    play_3d(driver, sample, 0.0f, 0.0f, 0.0f, 6000, 6000);
    assert(!_AIL_set_3D_sample_file(sample, NULL));

    _AIL_release_3D_sample_handle(sample);
    _AIL_close_3D_listener(listener);
    _AIL_close_3D_provider(provider);
    driver_destroy(driver);
    next = NULL;
    assert(_AIL_enumerate_3D_providers(&next, &provider, &name));
    assert(_AIL_open_3D_provider(provider) == PROVIDER_ERROR);
    assert(!_AIL_open_3D_listener(provider) && !_AIL_allocate_3D_sample_handle(provider));
    free(tone);
    free(stereo);
}

static int render_until_done(struct MilesDriver *driver, void *stream, int limit, int *peak)
{
    int16_t out[256 * 2];
    int frames = 0, i;
    *peak = 0;
    while (_AIL_stream_status(stream) == MSS_STREAM_PLAYING && frames < limit) {
        driver_mix(driver, out, 256);
        for (i = 0; i < 256 * 2; ++i)
            if (abs(out[i]) > *peak)
                *peak = abs(out[i]);
        frames += 256;
    }
    return frames;
}

static void test_streams(void)
{
    struct MilesDriver *driver = driver_create(22050, 2);
    char path[4096], directory[4096];
    int16_t out[256 * 2];
    void *stream;
    int frames, peak, i;
    snprintf(path, sizeof path, "%s/audio_sine.mp3", g_fixtures);
    assert(!_AIL_open_stream(driver, "missing\\nothing.mp3", 0));
    stream = _AIL_open_stream(driver, path, 0);
    assert(stream && _AIL_stream_status(stream) == MSS_SAMPLE_DONE);
    driver_mix(driver, out, 256);
    expect_frames(out, 2, 0, 256, 0, 0);

    _AIL_set_stream_volume(stream, 127);
    _AIL_set_stream_loop_count(stream, 2);
    _AIL_start_stream(stream);
    assert(_AIL_stream_status(stream) == MSS_STREAM_PLAYING);
    frames = render_until_done(driver, stream, 22050 * 10, &peak);
    assert(_AIL_stream_status(stream) == MSS_SAMPLE_DONE);
    assert(abs(frames - 2 * SINE_FRAMES) <= 2 * MP3_FRAME);
    assert(peak > 20000);

    _AIL_set_stream_loop_count(stream, 1);
    _AIL_start_stream(stream);
    frames = render_until_done(driver, stream, 22050 * 10, &peak);
    assert(abs(frames - SINE_FRAMES) <= MP3_FRAME + 256);

    _AIL_set_stream_loop_count(stream, 0);
    _AIL_start_stream(stream);
    frames = render_until_done(driver, stream, SINE_FRAMES * 4, &peak);
    assert(frames >= SINE_FRAMES * 4 && _AIL_stream_status(stream) == MSS_STREAM_PLAYING);
    _AIL_set_stream_volume(stream, 0);
    driver_mix(driver, out, 256);
    expect_frames(out, 2, 0, 256, 0, 0);
    _AIL_set_stream_volume(stream, 64);
    peak = 0;
    for (i = 0; i < 8; ++i) {
        int j;
        driver_mix(driver, out, 256);
        for (j = 0; j < 256 * 2; ++j)
            if (abs(out[j]) > peak)
                peak = abs(out[j]);
    }
    assert(peak > 10000 && peak < 14000);
    _AIL_close_stream(stream);
    assert(!driver->streams);

    assert(getcwd(directory, sizeof directory));
    assert(!chdir(g_fixtures) && !chdir(".."));
    stream = _AIL_open_stream(driver, "FIXTURES\\AUDIO_SINE.MP3", 0);
    assert(!chdir(directory));
    assert(stream);
    _AIL_start_stream(stream);
    assert(_AIL_stream_status(stream) == MSS_STREAM_PLAYING);
    driver_destroy(driver);
}

static void test_device(void)
{
    unsigned char *tone = make_tone(1, 22050, 10000, 0, 1000);
    char path[4096];
    void *driver, *sample, *stream;
    int waited;
    assert(_AIL_startup());
    _AIL_set_redist_directory(".\\");
    assert(_AIL_set_preference(15, 0) == 0);
    driver = _AIL_open_digital_driver(22050, 16, 2, 1);
    assert(driver);
    assert(!_AIL_open_digital_driver(22050, 16, 2, 1));
    assert(!_AIL_open_digital_driver(22050, 8, 2, 1) && !_AIL_open_digital_driver(22050, 16, 3, 1));
    sample = _AIL_allocate_sample_handle(driver);
    assert(sample && _AIL_set_sample_file(sample, tone, 0));
    _AIL_start_sample(sample);
    for (waited = 0; _AIL_sample_status(sample) != MSS_SAMPLE_DONE && waited < 5000; waited += 10)
        SDL_Delay(10);
    assert(_AIL_sample_status(sample) == MSS_SAMPLE_DONE);

    snprintf(path, sizeof path, "%s/audio_sine.mp3", g_fixtures);
    stream = _AIL_open_stream(driver, path, 0);
    assert(stream);
    _AIL_set_stream_loop_count(stream, 1);
    _AIL_start_stream(stream);
    for (waited = 0; _AIL_stream_status(stream) == MSS_STREAM_PLAYING && waited < 5000; waited += 10)
        SDL_Delay(10);
    assert(_AIL_stream_status(stream) == MSS_SAMPLE_DONE);
    _AIL_close_stream(stream);
    _AIL_start_sample(sample);
    _AIL_close_digital_driver(driver);
    _AIL_shutdown();
    free(tone);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    g_fixtures = argv[1];
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    assert(_AIL_startup());
    test_wave_parsing();
    check_adpcm("audio_ms_adpcm_mono.wav", "audio_ms_adpcm_mono.pcm", WAVE_FORMAT_MS_ADPCM, 1, 2);
    check_adpcm("audio_ms_adpcm_stereo.wav", "audio_ms_adpcm_stereo.pcm", WAVE_FORMAT_MS_ADPCM, 2, 3);
    check_adpcm("audio_ima_adpcm_mono.wav", "audio_ima_adpcm_mono.pcm", WAVE_FORMAT_IMA_ADPCM, 1, 2);
    check_adpcm("audio_ima_adpcm_stereo.wav", "audio_ima_adpcm_stereo.pcm", WAVE_FORMAT_IMA_ADPCM, 2, 3);
    test_mp3_decode();
    test_samples();
    test_positional();
    test_streams();
    _AIL_shutdown();
    test_device();
    puts("miles_posix: ok");
    return 0;
}
