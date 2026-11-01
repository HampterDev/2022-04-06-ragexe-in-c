#include <stdint.h>
#include <stdio.h>

/* Keep production request logic; replace only clock and packet boundaries. */
typedef uint32_t DWORD;
struct GameMode { unsigned emotion_time; };
_Static_assert(sizeof(unsigned) == sizeof(uint32_t), "32-bit client tick state");
static DWORD clock_now;
static int sent, sent_emotion, failures;
static unsigned short sent_id;

#define CHECK(value) do { if (!(value)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #value); ++failures; \
} } while (0)

static DWORD timeGetTime(void) { return clock_now; }
static void send_u8(unsigned short id, int emotion)
{
    ++sent;
    sent_id = id;
    sent_emotion = emotion;
}

#include "emote_timing.inc"

static void check_interval(uint32_t previous, uint32_t elapsed, int should_send)
{
    struct GameMode mode = {previous};
    clock_now = previous + elapsed;
    sent = 0;
    game_mode_msg_emotion(&mode, 58);
    CHECK(sent == should_send);
    CHECK(mode.emotion_time == (should_send ? clock_now : previous));
    if (should_send) {
        CHECK(sent_id == 0xbf && sent_emotion == 58);
        /* No animation-completion callback is needed for the next request. */
        clock_now += 10000;
        game_mode_msg_emotion(&mode, 2);
        CHECK(sent == 2 && sent_emotion == 2 && mode.emotion_time == clock_now);
    }
}

int main(void)
{
    static const uint32_t starts[] = {
        0, 10000, UINT32_MAX - 2500, UINT32_MAX - 2499,
        UINT32_MAX - 1000, UINT32_MAX - 1, UINT32_MAX
    };
    static const struct { uint32_t elapsed; int send; } cases[] = {
        {0, 0}, {1, 0}, {2499, 0}, {2500, 0}, {2501, 1},
        {5000, 1}, {100000, 1}
    };
    struct GameMode mode = {10000};
    unsigned i, j;
    for (i = 0; i < sizeof starts / sizeof *starts; ++i)
        for (j = 0; j < sizeof cases / sizeof *cases; ++j)
            check_interval(starts[i], cases[j].elapsed, cases[j].send);
    sent = 0;
    clock_now = 20000;
    game_mode_msg_emotion(&mode, 93);
    CHECK(sent == 0 && mode.emotion_time == 10000);
    game_mode_msg_emotion(&mode, 2);
    CHECK(sent == 1 && sent_emotion == 2 && mode.emotion_time == 20000);
    clock_now = 22500;
    game_mode_msg_emotion(&mode, 3);
    CHECK(sent == 1 && mode.emotion_time == 20000);
    clock_now = 22501;
    game_mode_msg_emotion(&mode, 3);
    CHECK(sent == 2 && sent_emotion == 3 && mode.emotion_time == 22501);
    /* Rejected Yut requests cannot consume or prolong the cooldown. */
    clock_now = 40000;
    for (i = 81; i <= 87; ++i) {
        game_mode_msg_emotion(&mode, (int)i);
        CHECK(sent == 2 && mode.emotion_time == 22501);
    }
    game_mode_msg_emotion(&mode, 58);
    CHECK(sent == 3 && sent_emotion == 58 && mode.emotion_time == 40000);
    if (failures) return 1;
    puts("Emote timing: 49 normal/wrapping clock cases, strict 2.5-second boundary, repeated requests without animation completion, Yut rejection and packets passed.");
    return 0;
}
