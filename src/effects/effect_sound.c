#include "contents/sound.h"
#include "effects/effect.h"
#include "game/context.h"

void effect_play_wave(const char *wave)
{
    if (game_current_player())
        sound_play_wave(wave, 1.0f);
}
