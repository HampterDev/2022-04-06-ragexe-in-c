#include "contents/sound.h"
#include "effects/rag/rag_skills_classic.h"
#include <assert.h>
#include <stdio.h>

static struct RenderObject listeners[2];
static struct Player *listener;
static int calls, change_listener;
static const char *waves[2];

struct Player *game_current_player(void)
{
    return listener;
}

void sound_play_wave(const char *wave, float volume)
{
    assert(volume == 1.0f);
    assert(calls < 2);
    waves[calls] = wave;
    ++calls;
    if (change_listener == 1)
        listener = NULL;
    else if (change_listener == 2)
        listener = (struct Player *)&listeners[1];
}

int main(void)
{
    effect_play_wave("one.wav");
    rag_play_two_waves("one.wav", "two.wav");
    assert(calls == 0);

    listener = (struct Player *)&listeners[0];
    effect_play_wave("one.wav");
    assert(calls == 1 && strcmp(waves[0], "one.wav") == 0);

    calls = 0;
    rag_play_two_waves("one.wav", "two.wav");
    assert(calls == 2 && strcmp(waves[0], "one.wav") == 0 && strcmp(waves[1], "two.wav") == 0);

    calls = 0;
    change_listener = 1;
    rag_play_two_waves("one.wav", "two.wav");
    assert(calls == 1);

    calls = 0;
    change_listener = 2;
    listener = (struct Player *)&listeners[0];
    rag_play_two_waves("one.wav", "two.wav");
    assert(calls == 2);
    puts("effect sound: a wave plays while there is a player, wherever it is; absent, removed and replaced players passed");
    return 0;
}
