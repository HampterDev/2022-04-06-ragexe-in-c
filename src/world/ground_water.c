#include "render/texture_cache.h"
#include "world/ground.h"
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

static int water_initialize(struct GndWater *water,
    const struct GroundWaterSettings *settings, int from_scene)
{
    if (settings->frameDelay <= 0 || !isfinite(settings->level)
            || !isfinite(settings->waveHeight) || !isfinite(settings->waveSpeed)
            || !isfinite(settings->wavePitch)
            || fabs((double)settings->level) + fabs((double)settings->waveHeight) > FLT_MAX)
        return 0;
    water->level = settings->level;
    water->type = settings->type;
    water->waveHeight = settings->waveHeight;
    water->phaseStep = fmodf(from_scene ? truncf(settings->waveSpeed) : settings->waveSpeed, 360.0f);
    water->wavePitch = from_scene ? truncf(settings->wavePitch) : settings->wavePitch;
    water->stepsPerPicture = settings->frameDelay;
    return 1;
}

int c3d_ground15_load_water(struct C3dGround15 *self, const struct GndRes *gnd,
    const struct GroundWaterSettings *scene)
{
    size_t i, count;
    struct GndWater *waters = NULL;
    int from_file;
    if (!gnd)
        return 0;
    from_file = gnd->major == 1 && gnd->minor >= 8;
    if (!from_file && !scene)
        return 0;
    count = from_file ? gnd->waterCount : 1;
    if (count > SIZE_MAX / sizeof *waters || (from_file && count && !gnd->waters))
        return 0;
    if (count) {
        waters = calloc(count, sizeof *waters);
        if (!waters)
            return 0;
    }
    for (i = 0; i < count; ++i) {
        struct GroundWaterSettings settings;
        if (from_file) {
            const struct GndWaterRec *record = &gnd->waters[i];
            settings.level = record->level;
            settings.type = record->type;
            settings.waveHeight = record->waveHeight;
            settings.waveSpeed = record->waveSpeed;
            settings.wavePitch = record->wavePitch;
            settings.frameDelay = record->animSpeed;
        } else {
            settings = *scene;
        }
        if (!water_initialize(&waters[i], &settings, !from_file)) {
            free(waters);
            return 0;
        }
    }
    free(self->waters);
    self->waters = waters;
    self->waterCount = count;
    self->waterPerBlock = count > 1;
    self->blocksX = from_file ? gnd->waterSplitW : 1;
    self->blocksY = from_file ? gnd->waterSplitH : 1;
    self->waterType = from_file ? gnd->water.type : scene->type;
    return 1;
}

void c3d_ground15_update_water(struct C3dGround15 *self)
{
    char name[32];
    size_t i;
    for (i = 0; i < self->waterCount; ++i) {
        struct GndWater *water = &self->waters[i];
        if (water->step == water->stepsPerPicture - 1) {
            water->step = 0;
            water->picture = (water->picture + 1) % 32;
        } else {
            ++water->step;
        }
        _snprintf_s(name, sizeof name, _TRUNCATE, "워터\\water%d%02d.jpg", water->type, water->picture);
        water->texture = texture_cache_get(&g_textureCache, name, 0, 0);
        water->phase += water->phaseStep;
        if (water->phase > 180.0f)
            water->phase -= 360.0f;
        else if (water->phase < -180.0f)
            water->phase += 360.0f;
    }
}

float ground_water_height(const struct GndWater *water, int64_t diagonal)
{
    float angle;
    int degrees;
    if (water->waveHeight == 0.0f)
        return water->level;
    angle = (float)diagonal * water->wavePitch + water->phase;
    if (isfinite(angle) && (double)angle >= INT_MIN && (double)angle <= INT_MAX) {
        degrees = (int)angle;
    } else {
        double wide = (double)diagonal * water->wavePitch + water->phase;
        degrees = (int)fmod(trunc(wide), 360.0);
    }
    return get_sin_deg(degrees) * water->waveHeight + water->level;
}

int c3d_ground15_get_block_index(struct C3dGround15 *self, int x, int y)
{
    int spanX, spanY, bx, by;
    int64_t index;
    if (x < 0 || y < 0 || self->width <= 0 || self->height <= 0
            || self->blocksX <= 0 || self->blocksY <= 0)
        return -1;
    spanX = self->width / self->blocksX;
    spanY = self->height / self->blocksY;
    if (!spanX || !spanY)
        return -1;
    bx = x / spanX;
    by = y / spanY;
    if (bx >= self->blocksX)
        bx = self->blocksX - 1;
    if (by >= self->blocksY)
        by = self->blocksY - 1;
    index = (int64_t)by * self->blocksX + bx;
    return index <= INT_MAX ? (int)index : -1;
}
