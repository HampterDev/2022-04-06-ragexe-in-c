#ifndef RENDER_SPRITE_ATLAS_H
#define RENDER_SPRITE_ATLAS_H

#include "resources/sprite.h"
#include <stddef.h>

struct Renderer;

struct SpriteRegion {
    float u, v, u1, v1;
    int x, y;
};

struct Texture *sprite_atlas_resolve(struct Renderer *renderer,
    const struct SprImg *image, const DWORD *palette, struct SpriteRegion *region);

void sprite_atlas_invalidate_images(struct Renderer *renderer, const struct SprImg *images, size_t count);
void sprite_atlas_invalidate_palette(struct Renderer *renderer, const DWORD *palette);
void sprite_atlas_initialize(struct Renderer *renderer);
void sprite_atlas_destroy(struct Renderer *renderer);
void sprite_atlas_clear(struct Renderer *renderer);
void sprite_atlas_purge(struct Renderer *renderer);
void sprite_atlas_finish_frame(struct Renderer *renderer);

int texture_draw_sprite_frame(struct Texture *texture, int x, int y, int w, int h,
    const struct SprImg *frame, const DWORD *palette);

#endif
