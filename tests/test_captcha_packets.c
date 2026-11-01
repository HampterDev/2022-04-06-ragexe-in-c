#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/game/captcha_packets.c"

static unsigned calls;
static int seen_id, seen_a, seen_b;
static unsigned char seen_key[4], seen_chunk[1024];
static void record(int id, int a, int b, const void *key, const void *chunk)
{
    ++calls; seen_id = id; seen_a = a; seen_b = b;
    if (key) memcpy(seen_key, key, 4);
    if (chunk) { assert(b > 0 && b <= 1024); memcpy(seen_chunk, chunk, b); }
}

void game_mode_zc_ack_apply_macro_detector(struct GameMode *g, int result)
{ (void)g; record(0xa57, result, 0, NULL, NULL); }
void game_mode_zc_apply_macro_detector(struct GameMode *g, int size, const char *key)
{ (void)g; record(0xa58, size, 0, key, NULL); }
void game_mode_zc_apply_macro_detector_captcha(struct GameMode *g, const char *key, const void *data, int size)
{ (void)g; record(0xa59, 0, size, key, data); }
void game_mode_on_preview_macro_detector_captcha(struct GameMode *g, const char *key, const void *data, unsigned short size)
{ (void)g; record(0xa6b, 0, size, key, data); }
void game_mode_on_req_answer_macro_detector(struct GameMode *g, int count, int timeout)
{ (void)g; record(0xa5b, count, timeout, NULL, NULL); }
void game_mode_zc_close_macro_detector(struct GameMode *g, int result)
{ (void)g; record(0xa5d, result, 0, NULL, NULL); }
void game_mode_zc_ack_preview_macro_detector(struct GameMode *g, int result, int size, const char *key)
{ (void)g; record(0xa6a, result, size, key, NULL); }

int main(void)
{
    static const struct { int id; size_t size; } fixed[] = {
        {0xa57, 6}, {0xa58, 8},
        {0xa5b, 7}, {0xa5d, 6}, {0xa6a, 12}
    };
    unsigned char p[1033];
    size_t i, n;
    int id;
    memset(p, 0, sizeof p);
    for (i = 0; i < sizeof fixed / sizeof *fixed; ++i) {
        calls = 0;
        for (n = 0; n < fixed[i].size; ++n)
            game_mode_dispatch_captcha_packet(NULL, fixed[i].id, p, n);
        game_mode_dispatch_captcha_packet(NULL, fixed[i].id, NULL, fixed[i].size);
        assert(!calls);
        game_mode_dispatch_captcha_packet(NULL, fixed[i].id, p, fixed[i].size);
        assert(calls == 1 && seen_id == fixed[i].id);
    }
    calls = 0;
    game_mode_dispatch_captcha_packet(NULL, 0xa53, p, sizeof p);
    game_mode_dispatch_captcha_packet(NULL, 0xa55, p, sizeof p);
    assert(!calls);
    p[2] = 3; p[3] = 0x78; p[4] = 0x56; p[5] = 0x34; p[6] = 0x12;
    game_mode_dispatch_captcha_packet(NULL, 0xa5b, p, 7);
    assert(seen_a == 3 && seen_b == 0x12345678);
    p[2] = 0xff; p[3] = 0xff;
    game_mode_dispatch_captcha_packet(NULL, 0xa58, p, 8);
    assert(seen_a == 65535 && !memcmp(seen_key, p + 4, 4));
    memset(p + 2, 0xff, 4); p[6] = 0x34; p[7] = 0x12;
    game_mode_dispatch_captcha_packet(NULL, 0xa6a, p, 12);
    assert(seen_a == -1 && seen_b == 0x1234 && !memcmp(seen_key, p + 8, 4));
    for (id = 0xa59; id <= 0xa6b; id += 0x12) {
        for (n = 0; n <= 1033; ++n) {
            calls = 0; p[2] = (unsigned char)n; p[3] = (unsigned char)(n >> 8);
            game_mode_dispatch_captcha_packet(NULL, id, p, n);
            assert(calls == (n >= 9 && n <= 1032));
            if (calls) {
                assert(seen_id == id && seen_b == n - 8);
                assert(!memcmp(seen_key, p + 4, 4) && !memcmp(seen_chunk, p + 8, n - 8));
            }
            calls = 0;
            if (n) game_mode_dispatch_captcha_packet(NULL, id, p, n - 1);
            assert(!calls);
        }
        calls = 0; p[2] = p[3] = 0xff;
        game_mode_dispatch_captcha_packet(NULL, id, p, sizeof p);
        assert(!calls);
    }
    puts("Captcha packets: all seven retained messages, every truncation and chunk boundary passed");
    return 0;
}
