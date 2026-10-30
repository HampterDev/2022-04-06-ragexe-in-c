#include "contents/sound.h"
#include "effects/rag/rag_skills_classic.h"
#include <assert.h>
#include <stdio.h>

static struct RagEffect effect;
static struct RenderObject listeners[2];
static struct Player *listener;
static int calls, change_listener;
static const char *waves[2];
static float positions[2][3];

struct Player *game_current_player(void)
{
    return listener;
}

void sound_play_wave(const char *wave, float x, float y, float z,
                     int max_distance, int full_distance, float volume)
{
    assert(max_distance == 250 && full_distance == 40 && volume == 1.0f);
    assert(calls < 2);
    waves[calls] = wave;
    positions[calls][0] = x;
    positions[calls][1] = y;
    positions[calls][2] = z;
    ++calls;
    if (change_listener == 1)
        listener = NULL;
    else if (change_listener == 2)
        listener = (struct Player *)&listeners[1];
}

int main(void)
{
    effect.base.pos.x = 10.0f;
    effect.base.pos.z = -20.0f;
    listeners[0].pos.x = 3.0f;
    listeners[0].pos.z = -4.0f;
    listeners[1].pos.x = -8.0f;
    listeners[1].pos.z = 2.0f;

    rag_play_wave(&effect, "one.wav", 6.0f);
    rag_play_two_waves(&effect, "one.wav", "two.wav");
    assert(calls == 0);

    listener = (struct Player *)&listeners[0];
    rag_play_wave(&effect, "one.wav", 6.0f);
    assert(calls == 1 && strcmp(waves[0], "one.wav") == 0);
    assert(positions[0][0] == 7.0f && positions[0][1] == 6.0f);
    assert(positions[0][2] == -16.0f);

    calls = 0;
    rag_play_two_waves(&effect, "one.wav", "two.wav");
    assert(calls == 2 && strcmp(waves[1], "two.wav") == 0);
    assert(positions[0][1] == 0.0f && positions[1][1] == 0.0f);
    assert(positions[1][0] == 7.0f && positions[1][2] == -16.0f);

    calls = 0;
    change_listener = 1;
    rag_play_two_waves(&effect, "one.wav", "two.wav");
    assert(calls == 1);

    calls = 0;
    change_listener = 2;
    listener = (struct Player *)&listeners[0];
    rag_play_two_waves(&effect, "one.wav", "two.wav");
    assert(calls == 2);
    assert(positions[0][0] == 7.0f && positions[0][2] == -16.0f);
    assert(positions[1][0] == 18.0f && positions[1][2] == -22.0f);
    puts("effect sound: absent, active, removed and replaced listeners passed");
    return 0;
}
