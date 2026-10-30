#include "contents/sound.h"
#include "effects/render_object.h"
#include "game/context.h"

void effect_play_wave_at(const char *wave, const Vec3 *position, float height)
{
    const struct RenderObject *player = (const struct RenderObject *)game_current_player();
    if (player)
        sound_play_wave(wave, position->x - player->pos.x, height,
                        position->z - player->pos.z, 250, 40, 1.0f);
}
