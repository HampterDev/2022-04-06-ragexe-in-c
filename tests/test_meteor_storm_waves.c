/* Meteor storm (RagEffect 90): the waves and quakes of frames 0-300 must be those of the
   original's case 90, which plays wizard_meteo and hunter_blastmine at frame 0, hunter_blastmine
   at 10, wizard_meteo at 20, 50, 80, 100, 130, 140, 180 and 200, and quakes every 50 frames from
   50 to 250. */
#include "effects/rag/rag_effect.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GAME_MSG_QUAKE 0x7a
#define METEO "effect\\wizard_meteo.wav"
#define BLASTMINE "effect\\hunter_blastmine.wav"

static struct RagEffect effect;
static char played[400];
static int quakes[400], steps;

void effect_play_wave_at(const char *wave, const Vec3 *position, float height)
{
    size_t length = strlen(played);
    assert(position == &effect.base.pos && height == 0.0f);
    assert(length + strlen(wave) + 32 < sizeof played);
    sprintf(played + length, "%d:%s ", effect.frame, wave);
}

static void mode_message(int msg, int par1)
{
    assert(msg == GAME_MSG_QUAKE && par1 == 0 && effect.frame < 400);
    quakes[effect.frame]++;
}

static unsigned char step(struct RagEffect *self)
{
    assert(self == &effect);
    steps++;
    return 1;
}

#include "meteor_storm_waves.inc"

int main(void)
{
    static const char expected[] =
        "0:" METEO " 0:" BLASTMINE " 10:" BLASTMINE " 20:" METEO " 50:" METEO " 80:" METEO
        " 100:" METEO " 130:" METEO " 140:" METEO " 180:" METEO " 200:" METEO " ";
    int frame;

    for (frame = 0; frame <= 300; frame++) {
        effect.frame = frame;
        assert(rag_meteor_storm_waves(&effect) == 1);
    }
    if (strcmp(played, expected)) {
        printf("Meteor storm waves differ from the original:\n  got      %s\n  expected %s\n", played, expected);
        return 1;
    }
    for (frame = 0; frame <= 300; frame++)
        assert(quakes[frame] == (frame >= 50 && frame % 50 == 0 && frame < 300));
    assert(steps == 301);
    puts("Meteor storm waves: frame 0 plays both waves as in the original, meteo at 180 not 170, quakes every 50 frames.");
    return 0;
}
